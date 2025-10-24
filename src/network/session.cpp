// Legacy WebSocket session implementation (not part of current build).
// Kept for reference and IntelliSense only.

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "network/session.h"
#include "engine/matching_engine.h"
#include "utils/time_utils.h"

Session::Session(int sessionId) : sessionId(sessionId) {
    // Initialize session resources
}

using json = nlohmann::json;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;

class WebSocketSession : public Session, public std::enable_shared_from_this<WebSocketSession> {
  public:
        WebSocketSession(tcp::socket socket, MatchingEngine &engine)
                : Session(0), // legacy placeholder session id for IntelliSense-only build
                    ws_(std::move(socket)),
                    engine_(engine) {}

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
                    json response;
                    response["type"] = "market_data";
                    response["symbol"] = update.symbol;
                    response["timestamp"] = utils::to_iso8601(update.timestamp);
                    response["bestBidPrice"] = update.bestBidPrice;
                    response["bestBidQuantity"] = update.bestBidQuantity;
                    response["bestAskPrice"] = update.bestAskPrice;
                    response["bestAskQuantity"] = update.bestAskQuantity;
                    response["seqNum"] = update.seqNum;
                    if (update.type == MarketDataUpdate::Type::SNAPSHOT) {
                        response["type"] = "market_data_snapshot";
                        response["bids"] = update.bids;
                        response["asks"] = update.asks;
                    } else {
                        response["type"] = "market_data_increment";
                        response["prevSeqNum"] = update.prevSeqNum;
                        response["gap"] = update.gap;
                        // serialize changes as objects
                        json bidsArr = json::array();
                        for (const auto &c : update.bidsChanges) {
                            json o;
                            std::string op = (c.op == MarketDataUpdate::ChangeOp::ADD) ? "add" : (c.op == MarketDataUpdate::ChangeOp::UPDATE) ? "update" : "remove";
                            o["op"] = op;
                            o["price"] = c.price;
                            if (c.op != MarketDataUpdate::ChangeOp::REMOVE) o["quantity"] = c.quantity;
                            bidsArr.push_back(o);
                        }
                        json asksArr = json::array();
                        for (const auto &c : update.asksChanges) {
                            json o;
                            std::string op = (c.op == MarketDataUpdate::ChangeOp::ADD) ? "add" : (c.op == MarketDataUpdate::ChangeOp::UPDATE) ? "update" : "remove";
                            o["op"] = op;
                            o["price"] = c.price;
                            if (c.op != MarketDataUpdate::ChangeOp::REMOVE) o["quantity"] = c.quantity;
                            asksArr.push_back(o);
                        }
                        response["bids_changes"] = bidsArr;
                        response["asks_changes"] = asksArr;
                    }
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
                                     {"timestamp", utils::to_iso8601(trade.timestamp)},
                                     {"seqNum", trade.seqNum}};
                    sendResponse(response);
                });

                sendResponse({{"status", "success"}, {"message", "Subscribed to trades"}});
            }
            else if (type == "modify_order") {
                if (!j.contains("order_id") || !j.contains("new_quantity")) {
                    sendResponse({{"status", "error"}, {"code", "invalid_request"}, {"message", "missing order_id or new_quantity"}});
                } else {
                    std::string orderId = j["order_id"];
                    double newQty = j["new_quantity"];
                    if (orderId.empty() || newQty < 0.0) {
                        sendResponse({{"status", "error"}, {"code", "invalid_params"}, {"message", "invalid order_id or quantity"}});
                    } else {
                        try {
                            bool ok = engine_.modifyOrder(orderId, newQty);
                            if (ok) sendResponse({{"status", "success"}, {"message", "order modified"}, {"order_id", orderId}});
                            else sendResponse({{"status", "error"}, {"message", "order not found"}});
                        } catch (const std::exception &e) {
                            sendResponse({{"status", "error"}, {"message", e.what()}});
                        }
                    }
                }
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