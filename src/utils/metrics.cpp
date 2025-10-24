#include "utils/metrics.h"
#include <chrono>

Metrics::Metrics()
    : totalOrdersSubmitted(0),
      totalOrdersExecuted(0),
      totalOrdersCancelled(0),
      lastSubmissionTime(std::chrono::steady_clock::now()),
      lastExecutionTime(std::chrono::steady_clock::now()),
      totalSubmissionTime(0.0),
      totalExecutionTime(0.0),
      submissionCount(0),
      executionCount(0) {}

void Metrics::recordOrderSubmission() {
    auto now = std::chrono::steady_clock::now();
    if (submissionCount > 0) {
        auto delta = std::chrono::duration<double, std::micro>(now - lastSubmissionTime).count();
        totalSubmissionTime += delta;
    }
    lastSubmissionTime = now;
    ++totalOrdersSubmitted;
    ++submissionCount;
}

void Metrics::recordOrderExecution() {
    auto now = std::chrono::steady_clock::now();
    if (executionCount > 0) {
        auto delta = std::chrono::duration<double, std::micro>(now - lastExecutionTime).count();
        totalExecutionTime += delta;
    }
    lastExecutionTime = now;
    ++totalOrdersExecuted;
    ++executionCount;
}

void Metrics::recordOrderCancellation() { ++totalOrdersCancelled; }

uint64_t Metrics::getTotalOrdersSubmitted() const { return totalOrdersSubmitted; }
uint64_t Metrics::getTotalOrdersExecuted() const { return totalOrdersExecuted; }
uint64_t Metrics::getTotalOrdersCancelled() const { return totalOrdersCancelled; }

double Metrics::getAverageOrderSubmissionTime() const {
    return submissionCount > 1 ? (totalSubmissionTime / static_cast<double>(submissionCount - 1)) : 0.0;
}

double Metrics::getAverageOrderExecutionTime() const {
    return executionCount > 1 ? (totalExecutionTime / static_cast<double>(executionCount - 1)) : 0.0;
}