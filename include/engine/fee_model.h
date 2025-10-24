#ifndef FEE_MODEL_H
#define FEE_MODEL_H

#include <string>
#include <unordered_map>

struct FeeSchedule {
    double makerFee; // Fee rate for providing liquidity (negative for rebates)
    double takerFee; // Fee rate for taking liquidity

    FeeSchedule(double maker = 0.001, double taker = 0.002) // Default 0.1% maker, 0.2% taker
        : makerFee(maker), takerFee(taker) {}
};

class FeeModel {
  public:
    FeeModel() = default;

    void setFeeSchedule(const std::string &symbol, const FeeSchedule &schedule) { feeSchedules[symbol] = schedule; }

    FeeSchedule getFeeSchedule(const std::string &symbol) const {
        auto it = feeSchedules.find(symbol);
        return it != feeSchedules.end() ? it->second : FeeSchedule();
    }

    // Calculate fees for a trade
    struct FeeCalculation {
        double makerFee;
        double takerFee;
        double makerRebate;
    };

    FeeCalculation calculateFees(const std::string &symbol, double price, double quantity) const {
        const auto &schedule = getFeeSchedule(symbol);
        FeeCalculation calc;

        // Calculate base fees
        calc.makerFee = price * quantity * std::max(0.0, schedule.makerFee);
        calc.takerFee = price * quantity * schedule.takerFee;

        // Calculate rebates (if maker fee is negative)
        calc.makerRebate = price * quantity * std::abs(std::min(0.0, schedule.makerFee));

        return calc;
    }

  private:
    std::unordered_map<std::string, FeeSchedule> feeSchedules;
};

#endif // FEE_MODEL_H