#include "network/session.h"
#include "engine/matching_engine.h"
#include <iostream>
#include <string>

Session::Session(int sessionId) : sessionId(sessionId) {
    // Initialize session resources
}

#include <boost/asio/strand.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include "network/session.h"

using json = nlohmann::json;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;

class WebSocketSession : public Session, public std::enable_shared_from_this<WebSocketSession> {
  public:
    WebSocketSession(tcp::socket socket, MatchingEngine &engine) : ws_(std::move(socket)), engine_(engine) {}

    void start() {
        ws_.async_accept([self = shared_from_this()](boost::system::error_code ec) {
            if (!ec) self->readMessage();
        });
    }

  private:
    websocket::stream<tcp::socket> ws_;
    MatchingEngine &engine_;
    boost::beast::flat_buffer buffer_;

    void readMessage() {
        ws_.async_read(buffer_, [self = shared_from_this()](boost::system::error_code ec, std::size_t bytes) {
            if (!ec) {
                std::string data((char *) self->buffer_.data().data(), bytes);
                self->handleMessage(data);
                self->buffer_.consume(bytes);
                self->readMessage();
            }
        });
    }

    void handleMessage(const std::string &message) {
        try {
            auto j = json::parse(message);
            std::string type = j["type"];

            if (type == "submit_order") {
                auto order = std::make_shared<Order>(j["id"], j["symbol"],
                                                     j["side"] == "buy" ? Order::Side::BUY : Order::Side::SELL,
                                                     parseOrderType(j["order_type"]), j["price"], j["quantity"]);
                engine_.submitOrder(order);

                sendResponse({{"status", "success"}, {"message", "Order submitted"}});
            } else if (type == "cancel_order") {
                bool success = engine_.cancelOrder(j["order_id"]);
                sendResponse({{"status", success ? "success" : "error"},
                              {"message", success ? "Order cancelled" : "Order not found"}});
            } else if (type == "subscribe_market_data") {
                std::string symbol = j["symbol"];
                engine_.subscribeToMarketData(symbol, [this](const MarketDataUpdate &update) {
                    json response = {{"type", "market_data"},
                                     {"symbol", update.symbol},
                                     {"timestamp", std::chrono::system_clock::to_time_t(update.timestamp)},
                                     {"bids", update.bids},
                                     {"asks", update.asks}};
                    sendResponse(response);
                });

                sendResponse({{"status", "success"}, {"message", "Subscribed to market data"}});
            } else if (type == "subscribe_trades") {
                std::string symbol = j["symbol"];
                engine_.subscribeToTrades(symbol, [this](const Trade &trade) {
                    json response = {{"type", "trade"},
                                     {"trade_id", trade.tradeId},
                                     {"symbol", trade.symbol},
                                     {"price", trade.price},
                                     {"quantity", trade.quantity},
                                     {"maker_order_id", trade.makerOrderId},
                                     {"taker_order_id", trade.takerOrderId},
                                     {"maker_fee", trade.makerFee},
                                     {"taker_fee", trade.takerFee},
                                     {"aggressor_side", trade.aggressorSide},
                                     {"timestamp", std::chrono::system_clock::to_time_t(trade.timestamp)}};
                    sendResponse(response);
                });

                sendResponse({{"status", "success"}, {"message", "Subscribed to trades"}});
            }
        } catch (const std::exception &e) { sendResponse({{"status", "error"}, {"message", e.what()}}); }
    }

    void sendResponse(const json &response) {
        ws_.async_write(boost::asio::buffer(response.dump()), [](boost::system::error_code ec, std::size_t) {
            if (ec) {
                // Handle error
            }
        });
    }

    Order::Type parseOrderType(const std::string &type) {
        if (type == "market") return Order::Type::MARKET;
        if (type == "limit") return Order::Type::LIMIT;
        if (type == "ioc") return Order::Type::IOC;
        if (type == "fok") return Order::Type::FOK;
        throw std::invalid_argument("Invalid order type");
    }
};