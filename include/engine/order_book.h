#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <list>

class Order {
  public:
    enum class Side { BUY, SELL };
    enum class Type {
        MARKET,
        LIMIT,
        IOC,
        FOK,
        STOP_LOSS,  // Triggers market order when price goes below stop price
        STOP_LIMIT, // Triggers limit order when price hits stop price
        TAKE_PROFIT // Market order when price hits profit target
    };

    std::string id;
    std::string symbol;
    Side side;
    Type type;
    double price;
    double quantity;
    std::chrono::system_clock::time_point timestamp;

    Order(std::string id, std::string symbol, Side side, Type type, double price, double quantity)
        : id(id), symbol(symbol), side(side), type(type), price(price), quantity(quantity),
          timestamp(std::chrono::system_clock::now()) {}

    std::string getId() const { return id; }
    std::string getSymbol() const { return symbol; }

  // Persistence: serialize book to JSON and restore from JSON
  // Uses nlohmann::json; to avoid coupling heavy includes in header, implementations include json when needed.
    Side getSide() const { return side; }
    Type getType() const { return type; }
    double getPrice() const { return price; }
    double getQuantity() const { return quantity; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp; }

    void modifyQuantity(double newQuantity) { quantity = newQuantity; }

  private:
    // (old private fields removed) Order stores its state in the public members above.
};

struct PriceLevel {
    double price;
  std::list<std::shared_ptr<Order>> orders;
    double totalQuantity;

    PriceLevel(double p) : price(p), totalQuantity(0.0) {}
};

class OrderBook {
  public:
    OrderBook(const std::string &symbol);

    // Core order operations
    void addOrder(std::shared_ptr<Order> order);
    void modifyOrder(const std::string &orderId, double newQuantity);
    bool cancelOrder(const std::string &orderId);
  bool hasOrder(const std::string &orderId) const;

    // Market data operations
    struct BBO {
        double bidPrice;
        double bidQuantity;
        double askPrice;
        double askQuantity;
        bool valid;  // true if both bid and ask exist
        
        BBO() : bidPrice(0), bidQuantity(0), askPrice(0), askQuantity(0), valid(false) {}
    };
    
    BBO getBBO() const;  // Optimized single call for BBO
    double getBestBidPrice() const;
    double getBestAskPrice() const;
    double getBestBidQuantity() const;
    double getBestAskQuantity() const;
    std::vector<std::pair<double, double>> getTopBids(size_t levels) const;
    std::vector<std::pair<double, double>> getTopAsks(size_t levels) const;

    // Order matching
  std::vector<std::shared_ptr<Order>> getMatchingOrders(const Order &incomingOrder) const;
    bool hasMatchingOrders(const Order &order) const;
  // Decrease the quantity of an existing order by `amount`. If the order is fully filled, remove it.
  // Returns the remaining quantity after the decrease (0.0 if removed).
  double decreaseOrderQuantity(const std::string &orderId, double amount);
    size_t getOrderCount() const;
    std::string getSymbol() const { return symbol; }

    // Persistence helpers
    std::string toJson() const;
    void fromJson(const std::string &jsonStr);

  private:
    std::string symbol;
    std::map<double, PriceLevel, std::greater<double>> buyLevels; // Sorted high to low
    std::map<double, PriceLevel> sellLevels;                      // Sorted low to high
    std::unordered_map<std::string, std::shared_ptr<Order>> orderMap;
  mutable std::recursive_mutex mutex;

  // Acquire the order book mutex (RAII). Use this when performing multi-step matching
  // to ensure the entire match is executed atomically with respect to this order book.
  public:
  std::unique_lock<std::recursive_mutex> acquireLock() {
    return std::unique_lock<std::recursive_mutex>(mutex);
  }
};

#endif // ORDER_BOOK_H