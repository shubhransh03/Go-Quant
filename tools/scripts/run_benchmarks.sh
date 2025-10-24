#!/bin/bash

# This script automates the execution of benchmark tests for the matching engine.

# Navigate to the benchmarks directory
cd ../benchmarks

# Compile the benchmark files
g++ -o latency_benchmark latency_benchmark.cpp -I ../include
g++ -o throughput_benchmark throughput_benchmark.cpp -I ../include

# Run the benchmarks
echo "Running Latency Benchmark..."
./latency_benchmark

echo "Running Throughput Benchmark..."
./throughput_benchmark

# Clean up the compiled benchmark files
rm latency_benchmark throughput_benchmark

echo "Benchmark tests completed."