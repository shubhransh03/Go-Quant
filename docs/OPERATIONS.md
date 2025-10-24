# Go-Quant Operations Guide

This guide covers deployment, configuration, monitoring, and operations for the Go-Quant matching engine.

## Table of Contents
- [Quick Start](#quick-start)
- [Building from Source](#building-from-source)
- [Docker Deployment](#docker-deployment)
- [Configuration](#configuration)
- [Monitoring](#monitoring)
- [Performance Tuning](#performance-tuning)
- [Troubleshooting](#troubleshooting)

---

## Quick Start

### Prerequisites
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.20+
- Boost 1.70+
- nlohmann-json
- GoogleTest (for tests)
- prometheus-cpp (for metrics)

### macOS Installation
```bash
# Install dependencies via Homebrew
brew install cmake boost nlohmann-json googletest prometheus-cpp

# Clone and build
git clone https://github.com/shubhransh03/Go-Quant.git
cd Go-Quant
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4

# Run
./matching_engine
```

### Linux Installation (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake \
    libboost-all-dev nlohmann-json3-dev \
    libgtest-dev libprometheus-cpp-dev

# Clone and build
git clone https://github.com/shubhransh03/Go-Quant.git
cd Go-Quant
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4

# Run
./matching_engine
```

---

## Building from Source

### Build Types

#### Release Build (Production)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
```

#### Debug Build (Development)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
```

#### With Benchmarks
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build . -j4
```

### Build Options
- `BUILD_BENCHMARKS`: Enable benchmark executables (default: ON)
- `CMAKE_BUILD_TYPE`: Release, Debug, RelWithDebInfo
- `CMAKE_CXX_COMPILER`: Specify compiler (g++, clang++)

---

## Docker Deployment

### Building the Docker Image
```bash
cd Go-Quant
docker build -t goquant:latest -f docker/Dockerfile .
```

### Running with Docker
```bash
# Run matching engine
docker run -p 8080:8080 -p 9090:9090 goquant:latest

# Run with custom configuration
docker run -p 8080:8080 -p 9090:9090 \
    -v $(pwd)/config:/app/config \
    goquant:latest
```

### Docker Compose (with Prometheus & Grafana)
```yaml
# docker-compose.yml
version: '3.8'
services:
  matching-engine:
    build:
      context: .
      dockerfile: docker/Dockerfile
    ports:
      - "8080:8080"  # WebSocket API
      - "9090:9090"  # Prometheus metrics
    restart: unless-stopped

  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9091:9090"
    volumes:
      - ./config/prometheus/prometheus.yml:/etc/prometheus/prometheus.yml
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
    restart: unless-stopped

  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    volumes:
      - ./config/grafana:/etc/grafana/provisioning
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
    restart: unless-stopped
```

Run with:
```bash
docker-compose up -d
```

---

## Configuration

### Port Configuration
- **WebSocket API**: Port 8080 (configurable in `src/main.cpp`)
- **Prometheus Metrics**: Port 9090 (default)

### Rate Limiting
Configure per-symbol rate limits in code:
```cpp
// In main.cpp or initialization code
RateLimiterManager::instance().addSymbol("BTC-USDT", 
    1000,  // orders per second
    2000   // max burst
);
```

### Fee Model
```cpp
// Set fee schedule per symbol
FeeModel feeModel;
feeModel.setFeeSchedule("BTC-USDT", 
    FeeSchedule(-0.0002, 0.0005)  // -0.02% maker, 0.05% taker
);
engine.setFeeModel(&feeModel);
```

### WAL (Write-Ahead Log)
```cpp
// Enable WAL for durability
engine.startWAL("/path/to/wal.log");

// Replay on restart
engine.replayWAL("/path/to/wal.log");
```

### Persistence
```cpp
// Save state periodically
engine.saveState("/path/to/snapshots");

// Load on startup
engine.loadState("/path/to/snapshots");
```

---

## Monitoring

### Prometheus Metrics Endpoint
Access metrics at: `http://localhost:9090/metrics`

### Available Metrics
```
# Order metrics
matching_engine_orders_received_total{symbol="BTC-USDT"}
matching_engine_orders_cancelled_total{symbol="BTC-USDT"}
matching_engine_orders_matched_total{symbol="BTC-USDT"}
matching_engine_trades_executed_total{symbol="BTC-USDT"}

# Latency histograms
matching_engine_latency_microseconds{type="order_processing"}

# Rate limiter
rate_limiter_allowed_total{symbol="BTC-USDT"}
rate_limiter_rejected_total{symbol="BTC-USDT"}
rate_limiter_tokens{symbol="BTC-USDT"}

# Memory pool
memory_pool_usage{type="order_capacity"}
memory_pool_usage{type="order_used"}

# Book depth
order_book_depth{symbol="BTC-USDT"}

# System metrics
system_cpu_usage_percent
system_memory_usage_mb
```

### Grafana Dashboard
1. Access Grafana: `http://localhost:3000` (admin/admin)
2. Import dashboard from `config/grafana/dashboards/`
3. Configure Prometheus datasource: `http://prometheus:9090`

### Key Dashboard Panels
- Order Processing Latency (p50, p95, p99)
- Order Throughput (orders/second)
- Trade Execution Rate
- Rate Limiter Status
- Memory Pool Utilization
- Order Book Depth by Symbol

---

## Performance Tuning

### CPU Optimization
```bash
# Linux: Disable CPU frequency scaling
sudo cpupower frequency-set --governor performance

# Pin to specific CPUs
taskset -c 0-3 ./matching_engine
```

### Memory
```bash
# Increase file descriptors
ulimit -n 65536

# Disable swap
sudo swapoff -a
```

### Network
```bash
# Increase socket buffer sizes
sysctl -w net.core.rmem_max=134217728
sysctl -w net.core.wmem_max=134217728
```

### Build Optimizations
```bash
# Profile-guided optimization
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -flto"
```

---

## Troubleshooting

### Common Issues

#### Port Already in Use
```bash
# Find process using port 8080
lsof -i :8080
# Kill the process or change port in code
```

#### Prometheus Metrics Not Showing
```bash
# Check if metrics endpoint is accessible
curl http://localhost:9090/metrics

# Verify prometheus.yml configuration
# Ensure target matches your setup
```

#### High Latency
1. Check CPU usage: `top` or `htop`
2. Verify build type is Release
3. Check for lock contention in logs
4. Monitor rate limiter rejections
5. Review order book depth

#### Memory Leaks
```bash
# Use Valgrind (Debug build)
valgrind --leak-check=full ./matching_engine

# Monitor with system metrics
watch -n 1 'ps aux | grep matching_engine'
```

#### Test Failures
```bash
# Run specific test with verbose output
cd build
ctest -R test_name --verbose

# Check logs
cat matching_engine.log
```

### Logging
Logs are written to `matching_engine.log` in the current directory.

Log levels:
- INFO: Normal operations
- WARN: Recoverable issues
- ERROR: Critical errors

### Support
- GitHub Issues: https://github.com/shubhransh03/Go-Quant/issues
- Documentation: See `docs/` directory
- API Reference: `docs/WEBSOCKET_API.md`
