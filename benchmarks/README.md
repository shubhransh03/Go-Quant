# Benchmarks

This folder contains performance benchmarks for the Go-Quant matching engine.

## Available Benchmarks

### 1. `bench_runner.cpp`
A minimal harness using the public MatchingEngine API to measure average latency for market orders.

### 2. `latency_benchmark.cpp`
Measures end-to-end order processing latency including:
- Order submission time
- Matching algorithm execution
- Market data update generation
- Reports median, p95, and p99 latencies

### 3. `throughput_benchmark.cpp`
Measures maximum order throughput (orders/second) under various load conditions:
- Single-threaded order submission
- Market and limit order mixes
- Different order book depths

### 4. `performance_benchmark.cpp`
Comprehensive performance suite covering:
- Order book operations
- Matching algorithm efficiency
- Memory pool performance
- Lock contention analysis

## Building Benchmarks

Benchmarks are enabled by default. To build:

```bash
# From repo root
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j4
```

To disable benchmarks:
```bash
cmake .. -DBUILD_BENCHMARKS=OFF
```

## Running Benchmarks

### macOS/Linux:
```bash
cd build
./latency_benchmark
./throughput_benchmark
./performance_benchmark
./bench_runner
```

### Windows (PowerShell):
```powershell
cd build
.\latency_benchmark.exe
.\throughput_benchmark.exe
.\performance_benchmark.exe
.\bench_runner.exe
```

## Benchmark Configuration

For accurate results:
1. **Use Release builds**: `-DCMAKE_BUILD_TYPE=Release`
2. **Disable CPU frequency scaling**: 
   - Linux: `sudo cpupower frequency-set --governor performance`
   - macOS: Energy Saver settings
3. **Pin to specific CPUs** (advanced): Use `taskset` (Linux) or process affinity APIs
4. **Close background applications**: Minimize system load
5. **Run multiple iterations**: Average results across several runs

## Expected Performance

Based on typical hardware (2020+ laptop/desktop):

| Metric | Target | Typical |
|--------|--------|---------|
| Order Processing Latency (median) | <100 μs | 50-80 μs |
| Order Processing Latency (p99) | <500 μs | 200-400 μs |
| Throughput (single-threaded) | >10K orders/sec | 15-25K orders/sec |
| Market Data Update | <50 μs | 20-30 μs |

Performance varies based on:
- Hardware (CPU, memory speed)
- Order book depth
- Order type mix
- System load

## Interpreting Results

- **Latency**: Lower is better. Watch for outliers (p99, p999)
- **Throughput**: Higher is better. Should scale near-linearly with order complexity
- **Memory**: Pooling should reduce allocation overhead
- **Consistency**: Standard deviation should be low for predictable performance

