# High-Performance Cryptocurrency Matching Engine

A high-performance matching engine implementing REG NMS-inspired principles for cryptocurrency trading. The system provides strict price-time priority matching, real-time market data dissemination, and comprehensive trade execution reporting.

## Architecture Overview

### Core Components

1. **Order Book (OrderBook)**

   - Implements price-time priority queue using two-level data structure:
     - Price levels (std::map for price-ordered access)
     - Time-ordered orders within each level (std::vector for FIFO)
   - O(1) best price access
   - O(log n) price level insertion/removal
   - O(1) order insertion within price level

2. **Matching Engine (MatchingEngine)**

   - Central coordinator for all trading activities
   - Manages multiple order books (one per trading pair)
   - Handles order submission, cancellation, and modification
   - Generates market data and trade execution feeds
   - Thread-safe operations with fine-grained locking

3. **Matching Algorithm (MatchingAlgorithm)**

   - Implements REG NMS-compliant matching logic
   - Prevents trade-throughs via price-time priority
   - Supports all order types (Market, Limit, IOC, FOK)
   - Advanced order support (Stop-Loss, Stop-Limit, Take-Profit)

4. **Network Layer (Session, Listener)**
   - WebSocket-based API for order submission and data streaming
   - JSON message format for client communication
   - Async I/O for high-performance networking

### Performance Optimizations

1. **Data Structures**

   - Price levels: Red-black tree (std::map) for O(log n) operations
   - Order lookup: Hash table (std::unordered_map) for O(1) access
   - FIFO queue: Vector-based implementation for cache efficiency

2. **Concurrency**

   - Fine-grained locking at order book level
   - Lock-free reads for market data dissemination
   - Async processing for non-critical operations

3. **Memory Management**
   - Object pooling for orders and trades
   - Smart pointer usage for automatic resource management
   - Pre-allocated buffers for high-frequency operations

### Bonus Features

1. **Advanced Order Types**

   - Stop-Loss: Market order triggered at stop price
   - Stop-Limit: Limit order placed at trigger
   - Take-Profit: Market order at profit target

2. **Persistence Layer**

   - JSON-based order book serialization
   - Point-in-time recovery capability
   - Efficient state restoration

3. **Fee Model**
   - Configurable maker-taker fees
   - Per-symbol fee schedules
   - Automatic fee calculation on trades

## Performance Metrics

Based on benchmark results:

- Order Processing: >10,000 orders/second
- Average Latency: <100 microseconds
- 99th Percentile Latency: <500 microseconds
- Market Data Updates: <50 microseconds

## API Specification

### Order Submission

```json
{
  "type": "submit_order",
  "symbol": "BTC-USDT",
  "order_type": "limit",
  "side": "buy",
  "quantity": "1.5",
  "price": "50000.00"
}
```

### Market Data Feed

```json
{
  "type": "market_data",
  "symbol": "BTC-USDT",
  "timestamp": "2025-10-22T10:00:00.123456Z",
  "bids": [
    ["50000.00", "1.5"],
    ["49999.00", "2.0"]
  ],
  "asks": [
    ["50001.00", "1.0"],
    ["50002.00", "2.5"]
  ]
}
```

### Trade Feed

```json
{
  "type": "trade",
  "symbol": "BTC-USDT",
  "trade_id": "T123456789",
  "price": "50000.00",
  "quantity": "1.5",
  "maker_order_id": "O123456",
  "taker_order_id": "O123457",
  "timestamp": "2025-10-22T10:00:00.123456Z"
}
```

## Build Instructions

1. Requirements:

   - C++17 compatible compiler
   - CMake 3.20+
   - Boost (system, asio, beast) and dependencies
   - nlohmann/json (for JSON handling)
   - GoogleTest for unit tests

   Tip: On Windows using vcpkg is convenient. Set `VCPKG_ROOT` to your vcpkg install and CMakeLists will pick up the toolchain file.

2. Build (PowerShell example):

```powershell
if (-Not (Test-Path build)) { mkdir build }
cd build
cmake ..
cmake --build . --config Release
```

3. Run Tests (PowerShell):

```powershell
# From build directory
ctest -C Release --verbose
```

4. Run the engine and example client

- Start the engine (listens on port 8080 by default):
  - Built binary `matching_engine.exe` will be in your build output directory.
- Example Python client (requires `websockets`):
```powershell
pip install websockets
python tools/example_ws_client.py
```

## Trade-off Decisions

1. **Memory vs. Speed**

   - Chosen to optimize for speed with higher memory usage
   - Duplicate order references for O(1) lookup
   - Pre-allocated buffers for critical paths

2. **Consistency vs. Latency**

   - Strong consistency model with synchronized order books
   - Accept slightly higher latency for guaranteed correctness
   - Fine-grained locking to minimize contention

3. **Complexity vs. Features**
   - Full feature set implementation
   - Modular design for maintainability
   - Clear separation of concerns

## License

MIT License - See LICENSE file for details

- **include/**: Contains header files for the project.
- **tests/**: Contains unit and integration tests for the engine.
- **benchmarks/**: Contains benchmarking code for latency and throughput.
- **examples/**: Provides example client code for interacting with the engine.
- **tools/**: Contains scripts for running tests and benchmarks.
- **proto/**: Defines protocol buffer messages for communication.
- **cmake/**: Contains CMake configuration files.
- **docker/**: Contains Docker configuration for the project.
- **CMakeLists.txt**: CMake build configuration file.
- **LICENSE**: Licensing information for the project.
- **.gitignore**: Specifies files to ignore in version control.
- **.clang-format**: Formatting rules for C++ code.

## Setup Instructions

1. **Clone the repository**:

   ```
   git clone <repository-url>
   cd matching-engine-cpp
   ```

2. **Build the project**:

   ```
   mkdir build
   cd build
   cmake ..
   make
   ```

3. **Run the matching engine**:
   ```
   ./matching-engine
   ```

## Usage

The matching engine provides APIs for:

- **Order Submission**: Clients can submit buy/sell orders.
- **Market Data Dissemination**: The engine broadcasts market data updates.
- **Trade Execution**: The engine generates trade execution data.

## Testing

To run unit tests:

```
cd tests/unit
./run_tests.sh
```

To run benchmarks:

```
cd benchmarks
./run_benchmarks.sh
```

## Contributing

Contributions are welcome! Please submit a pull request or open an issue for discussion.

## License

This project is licensed under the MIT License. See the LICENSE file for details.
