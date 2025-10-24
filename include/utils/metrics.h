#ifndef METRICS_H
#define METRICS_H

#include <cstdint>
#include <chrono>

class Metrics {
public:
    Metrics();

    void recordOrderSubmission();
    void recordOrderExecution();
    void recordOrderCancellation();
    
    uint64_t getTotalOrdersSubmitted() const;
    uint64_t getTotalOrdersExecuted() const;
    uint64_t getTotalOrdersCancelled() const;

    double getAverageOrderSubmissionTime() const;
    double getAverageOrderExecutionTime() const;

private:
    uint64_t totalOrdersSubmitted;
    uint64_t totalOrdersExecuted;
    uint64_t totalOrdersCancelled;

    std::chrono::steady_clock::time_point lastSubmissionTime;
    std::chrono::steady_clock::time_point lastExecutionTime;

    double totalSubmissionTime;
    double totalExecutionTime;
    uint64_t submissionCount;
    uint64_t executionCount;
};

#endif // METRICS_H