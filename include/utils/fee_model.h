#ifndef FEE_MODEL_H
#define FEE_MODEL_H

#include <string>

// Interface for fee calculation
class FeeModel {
public:
    virtual ~FeeModel() = default;
    
    // Calculate maker fee (taker removes liquidity, maker adds liquidity)
    virtual double getMakerFee(const std::string& symbol, double price, double quantity) const = 0;
    
    // Calculate taker fee
    virtual double getTakerFee(const std::string& symbol, double price, double quantity) const = 0;
};

// Default fee model with configurable basis points
class DefaultFeeModel : public FeeModel {
public:
    DefaultFeeModel(double makerBps = 2.0, double takerBps = 5.0)
        : makerFeeBps_(makerBps), takerFeeBps_(takerBps) {}
    
    double getMakerFee(const std::string&, double price, double quantity) const override {
        return price * quantity * (makerFeeBps_ / 10000.0);
    }
    
    double getTakerFee(const std::string&, double price, double quantity) const override {
        return price * quantity * (takerFeeBps_ / 10000.0);
    }
    
private:
    double makerFeeBps_;  // Basis points (e.g., 2.0 = 0.02%)
    double takerFeeBps_;  // Basis points (e.g., 5.0 = 0.05%)
};

// Zero-fee model for testing
class ZeroFeeModel : public FeeModel {
public:
    double getMakerFee(const std::string&, double, double) const override { return 0.0; }
    double getTakerFee(const std::string&, double, double) const override { return 0.0; }
};

#endif // FEE_MODEL_H
