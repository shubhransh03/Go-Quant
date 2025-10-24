#include <chrono>
#include <iostream>
#include <thread>
#include "engine/fee_model.h"
#include "engine/matching_engine.h"

void printOrderBook(const MarketDataUpdate &data) {
    std::cout << "\nOrder Book for " << data.symbol << ":\n";
    std::cout << "Timestamp: " << std::chrono::system_clock::to_time_t(data.timestamp) << "\n\n";

    std::cout << "Asks:\n";
    for (const auto &level : data.asks) { std::cout << level.first << " -> " << level.second << "\n"; }

    std::cout << "\nBids:\n";
    for (const auto &level : data.bids) { std::cout << level.first << " -> " << level.second << "\n"; }
    std::cout << "\n";
}

void printTrade(const Trade &trade) {
    std::cout << "Trade Executed:\n"
              << "ID: " << trade.tradeId << "\n"
              << "Symbol: " << trade.symbol << "\n"
              << "Price: " << trade.price << "\n"
              << "Quantity: " << trade.quantity << "\n"
              << "Maker Order: " << trade.makerOrderId << "\n"
              << "Taker Order: " << trade.takerOrderId << "\n"
              << "Maker Fee: " << trade.makerFee << "\n"
              << "Taker Fee: " << trade.takerFee << "\n\n";
}

int main() {
    // Initialize the engine
    MatchingEngine engine;
    FeeModel feeModel;

    const std::string SYMBOL = "BTC-USDT";

    // Set up fee model
    feeModel.setFeeSchedule(SYMBOL, FeeSchedule(-0.0002, 0.0005)); // -0.02% maker, 0.05% taker
    engine.setFeeModel(&feeModel);

    // Subscribe to market data
    engine.subscribeToMarketData(SYMBOL, printOrderBook);
    engine.subscribeToTrades(SYMBOL, printTrade);

    std::cout << "Creating initial liquidity...\n";

    // Add some initial liquidity
    for (int i = 0; i < 5; i++) {
        // Buy orders
        engine.submitOrder(std::make_shared<Order>("BUY_" + std::to_string(i), SYMBOL, Order::Side::BUY,
                                                   Order::Type::LIMIT,
                                                   50000.0 - i * 10.0, // Prices from 50000 down
                                                   1.0));

        // Sell orders
        engine.submitOrder(std::make_shared<Order>("SELL_" + std::to_string(i), SYMBOL, Order::Side::SELL,
                                                   Order::Type::LIMIT,
                                                   50000.0 + i * 10.0, // Prices from 50000 up
                                                   1.0));
    }

    std::cout << "\nSubmitting market orders...\n";

    // Submit some market orders
    engine.submitOrder(std::make_shared<Order>("MARKET_BUY_1", SYMBOL, Order::Side::BUY, Order::Type::MARKET,
                                               0.0, // Price not needed for market orders
                                               2.0));

    std::cout << "\nSubmitting stop orders...\n";

    // Add a stop-loss order (use the order price as the trigger price)
    auto stopLoss = std::make_shared<Order>("STOP_1", SYMBOL, Order::Side::SELL, Order::Type::STOP_LOSS, 49950.0, 1.0);
    engine.submitOrder(stopLoss);

    // Add a take-profit order (use the order price as the trigger price)
    auto takeProfit = std::make_shared<Order>("TP_1", SYMBOL, Order::Side::SELL, Order::Type::TAKE_PROFIT, 50100.0, 1.0);
    engine.submitOrder(takeProfit);

    // Wait a bit to see the results
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\nFinal market data:\n";
    auto finalData = engine.getMarketData(SYMBOL);
    printOrderBook(finalData);

    return 0;
}