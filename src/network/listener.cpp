#include "network/listener.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include "utils/time_utils.h"
#include <iostream>
#include <thread>

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
using json = nlohmann::json;

class Listener::Impl {
  public:
    Impl(MatchingEngine &engine, unsigned short port)
        : ioc(1), acceptor(ioc, tcp::endpoint(tcp::v4(), port)), engine(engine), port(port) {}

    void run() {
        doAccept();
        ioc.run();
    }

    void stop() {
        ioc.stop();
        if (worker.joinable()) worker.join();
    }

    void startBackground() {
        worker = std::thread([this]() { run(); });
    }

  private:
    boost::asio::io_context ioc;
    tcp::acceptor acceptor;
    MatchingEngine &engine;
    unsigned short port;
    std::thread worker;

    void doAccept() {
        acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<WebSocketSession>(std::move(socket), engine)->start();
            }
            doAccept();
        });
    }

    // Inner class for websocket sessions
    class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
      public:
        WebSocketSession(tcp::socket socket, MatchingEngine &engine)
            : ws(std::move(socket)), engine(engine) {}

        void start() {
            ws.async_accept([self = shared_from_this()](boost::system::error_code ec) {
                if (!ec) self->readLoop();
            });
        }

      private:
        websocket::stream<tcp::socket> ws;
        boost::beast::flat_buffer buffer;
        MatchingEngine &engine;

        void readLoop() {
            ws.async_read(buffer, [self = shared_from_this()](boost::system::error_code ec, std::size_t bytes) {
                if (ec) return; // connection closed or error

                try {
                    auto data = boost::beast::buffers_to_string(self->buffer.data());
                    self->buffer.consume(bytes);
                    json j = json::parse(data);

                    std::string type = j.value("type", "");
                    if (type == "submit_order") {
                        // Required fields: id, symbol, side, order_type, quantity, price (for limit)
                        if (!j.contains("id") || !j.contains("symbol") || !j.contains("side") || !j.contains("order_type") || !j.contains("quantity")) {
                            json resp = { {"status", "error"}, {"code", "invalid_request"}, {"message", "missing required fields"} };
                            self->send(resp.dump());
                        } else {
                            std::string id = j.at("id").get<std::string>();
                            std::string symbol = j.at("symbol").get<std::string>();
                            std::string side = j.at("side").get<std::string>();
                            std::string order_type = j.at("order_type").get<std::string>();
                            double quantity = j.at("quantity").get<double>();
                            double price = j.value("price", 0.0);

                            // Basic validation
                            if (id.empty() || symbol.empty() || quantity <= 0.0) {
                                json resp = { {"status", "error"}, {"code", "invalid_params"}, {"message", "invalid id/symbol/quantity"} };
                                self->send(resp.dump());
                            } else if (order_type == "limit" && price <= 0.0) {
                                json resp = { {"status", "error"}, {"code", "invalid_price"}, {"message", "limit orders require price > 0"} };
                                self->send(resp.dump());
                            } else if (side != "buy" && side != "sell") {
                                json resp = { {"status", "error"}, {"code", "invalid_side"}, {"message", "side must be 'buy' or 'sell'"} };
                                self->send(resp.dump());
                            } else {
                                Order::Side s = (side == "buy") ? Order::Side::BUY : Order::Side::SELL;
                                Order::Type t;
                                if (order_type == "market") t = Order::Type::MARKET;
                                else if (order_type == "limit") t = Order::Type::LIMIT;
                                else if (order_type == "ioc") t = Order::Type::IOC;
                                else if (order_type == "fok") t = Order::Type::FOK;
                                else {
                                    json resp = { {"status", "error"}, {"code", "invalid_order_type"}, {"message", "unsupported order_type"} };
                                    self->send(resp.dump());
                                    goto continue_read;
                                }

                                try {
                                    auto order = std::make_shared<Order>(id, symbol, s, t, price, quantity);
                                    self->engine.submitOrder(order);
                                    json resp = { {"status", "ok"}, {"message", "order_submitted"}, {"order_id", id} };
                                    self->send(resp.dump());
                                } catch (const std::exception &e) {
                                    json resp = { {"status", "error"}, {"code", "processing_error"}, {"message", e.what()} };
                                    self->send(resp.dump());
                                }
                            }
                        }
                    continue_read: ;

                    } else if (type == "cancel_order") {
                        if (!j.contains("order_id")) {
                            json resp = { {"status", "error"}, {"code", "invalid_request"}, {"message", "missing order_id"} };
                            self->send(resp.dump());
                        } else {
                            std::string orderId = j.at("order_id").get<std::string>();
                            if (orderId.empty()) {
                                json resp = { {"status", "error"}, {"code", "invalid_request"}, {"message", "order_id empty"} };
                                self->send(resp.dump());
                            } else {
                                bool success = self->engine.cancelOrder(orderId);
                                json resp = { {"status", success ? "ok" : "error"}, {"order_id", orderId} };
                                self->send(resp.dump());
                            }
                        }

                    } else if (type == "subscribe_market_data") {
                        if (!j.contains("symbol")) {
                            json resp = { {"status", "error"}, {"code", "invalid_request"}, {"message", "missing symbol"} };
                            self->send(resp.dump());
                        } else {
                            std::string symbol = j.at("symbol").get<std::string>();
                            if (symbol.empty()) {
                                json resp = { {"status", "error"}, {"code", "invalid_request"}, {"message", "symbol empty"} };
                                self->send(resp.dump());
                            } else {
                                self->engine.subscribeToMarketData(symbol, [self_weak = std::weak_ptr<WebSocketSession>(self)](const MarketDataUpdate &update) {
                                    if (auto self = self_weak.lock()) {
                                        try {
                                            json response;
                                            response["symbol"] = update.symbol;
                                            response["timestamp"] = utils::to_iso8601(update.timestamp);
                                            // Best bid/ask fields (BBO)
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
                                            self->send(response.dump());
                                        } catch (...) {
                                            // ignore send errors for now
                                        }
                                    }
                                });

                                json resp = { {"status", "ok"}, {"message", "subscribed_market_data"}, {"symbol", symbol} };
                                self->send(resp.dump());
                            }
                        }

                    } else if (type == "subscribe_trades") {
                        if (!j.contains("symbol")) {
                            json resp = { {"status", "error"}, {"code", "invalid_request"}, {"message", "missing symbol"} };
                            self->send(resp.dump());
                        } else {
                            std::string symbol = j.at("symbol").get<std::string>();
                            if (symbol.empty()) {
                                json resp = { {"status", "error"}, {"code", "invalid_request"}, {"message", "symbol empty"} };
                                self->send(resp.dump());
                            } else {
                                self->engine.subscribeToTrades(symbol, [self_weak = std::weak_ptr<WebSocketSession>(self)](const Trade &trade) {
                                    if (auto self = self_weak.lock()) {
                                        try {
                                            json response;
                                            response["type"] = "trade";
                                            response["trade_id"] = trade.tradeId;
                                            response["symbol"] = trade.symbol;
                                            response["price"] = trade.price;
                                            response["quantity"] = trade.quantity;
                                            response["maker_order_id"] = trade.makerOrderId;
                                            response["taker_order_id"] = trade.takerOrderId;
                                            response["maker_fee"] = trade.makerFee;
                                            response["taker_fee"] = trade.takerFee;
                                            response["aggressor_side"] = trade.aggressorSide;
                                            response["timestamp"] = utils::to_iso8601(trade.timestamp);
                                            response["seqNum"] = trade.seqNum;
                                            self->send(response.dump());
                                        } catch (...) {
                                            // ignore send errors for now
                                        }
                                    }
                                });

                                json resp = { {"status", "ok"}, {"message", "subscribed_trades"}, {"symbol", symbol} };
                                self->send(resp.dump());
                            }
                        }
                    } else if (type == "get_metrics") {
                        // Return metrics JSON
                        try {
                            std::string metrics = self->engine.getMetricsJSON();
                            json resp = { {"status", "ok"}, {"metrics", json::parse(metrics)} };
                            self->send(resp.dump());
                        } catch (const std::exception &e) {
                            json resp = { {"status", "error"}, {"message", e.what()} };
                            self->send(resp.dump());
                        }
                    }
                    
                    else if (type == "modify_order") {
                        // Required: order_id, new_quantity
                        if (!j.contains("order_id") || !j.contains("new_quantity")) {
                            json resp = { {"status", "error"}, {"code", "invalid_request"}, {"message", "missing order_id or new_quantity"} };
                            self->send(resp.dump());
                        } else {
                            std::string orderId = j.at("order_id").get<std::string>();
                            double newQty = j.at("new_quantity").get<double>();
                            if (orderId.empty() || newQty < 0.0) {
                                json resp = { {"status", "error"}, {"code", "invalid_params"}, {"message", "invalid order_id or quantity"} };
                                self->send(resp.dump());
                            } else {
                                try {
                                    bool ok = self->engine.modifyOrder(orderId, newQty);
                                    if (ok) {
                                        json resp = { {"status", "ok"}, {"message", "order_modified"}, {"order_id", orderId}, {"new_quantity", newQty} };
                                        self->send(resp.dump());
                                    } else {
                                        json resp = { {"status", "error"}, {"code", "not_found"}, {"message", "order not found or could not be modified"} };
                                        self->send(resp.dump());
                                    }
                                } catch (const std::exception &e) {
                                    json resp = { {"status", "error"}, {"code", "processing_error"}, {"message", e.what()} };
                                    self->send(resp.dump());
                                }
                            }
                        }
                    }
                } catch (const std::exception &e) {
                    json resp = { {"status", "error"}, {"message", e.what()} };
                    self->send(resp.dump());
                }

                // Continue reading
                self->readLoop();
            });
        }

        void send(const std::string &msg) {
            ws.text(true);
            ws.async_write(boost::asio::buffer(msg), [](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    // ignore for now
                }
            });
        }
    };
};

Listener::Listener(MatchingEngine &engine, unsigned short port)
    : impl(std::make_unique<Impl>(engine, port)) {}

Listener::~Listener() { stop(); }

bool Listener::startListening() {
    try {
        impl->startBackground();
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to start listener: " << e.what() << std::endl;
        return false;
    }
}

void Listener::stop() {
    if (impl) impl->stop();
}

void Listener::processEvents() {
    // No-op; we run io_context in background thread
}