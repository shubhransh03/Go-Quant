# Video Recording Guide - REG NMS Matching Engine Demo

**Duration:** 15-20 minutes  
**Format:** MP4, 1080p  
**Tools:** QuickTime (macOS), OBS Studio, or Zoom

---

## 🎬 Pre-Recording Checklist

### 1. Build and Test Everything
```bash
cd /Users/shubhupadhyay/Downloads/Go-Quant

# Clean build
rm -rf build
mkdir build
cd build

# Configure and build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j4

# Quick test
./matching_engine --help  # Should show it runs
```

### 2. Prepare Terminal Windows

**Terminal 1: Matching Engine**
```bash
cd /Users/shubhupadhyay/Downloads/Go-Quant/build
./matching_engine
```

**Terminal 2: Python WebSocket Client**
```bash
cd /Users/shubhupadhyay/Downloads/Go-Quant
python3 tools/example_ws_client.py
```

**Terminal 3: For showing logs/monitoring**
```bash
cd /Users/shubhupadhyay/Downloads/Go-Quant/build
tail -f matching_engine.log  # If you have logging to file
```

### 3. Prepare Code Editors
Open these files in VS Code (or your preferred editor):
- `src/engine/matching_algorithm.cpp`
- `include/engine/order_book.h`
- `include/utils/memory_pool.h`
- `include/utils/ring_buffer.h`
- `src/engine/matching_engine.cpp`

### 4. Clean Up Your Desktop
- Close unnecessary applications
- Hide desktop icons (optional)
- Prepare a clean workspace
- Have the assignment document open for reference

---

## 🎥 Recording Setup

### Option 1: QuickTime (Simplest - macOS)

1. **Open QuickTime Player**
2. **File → New Screen Recording**
3. **Click Options:**
   - Microphone: Select your microphone
   - Quality: Maximum
4. **Click Record button**
5. **Select area or full screen**
6. **Start recording**
7. **Stop:** Click stop icon in menu bar when done

**Export:**
- File → Export As → 1080p

### Option 2: OBS Studio (Professional)

1. **Install OBS Studio** (if not installed)
   ```bash
   brew install --cask obs
   ```

2. **Configure OBS:**
   - Open OBS Studio
   - Settings → Video:
     - Base Resolution: 1920x1080
     - Output Resolution: 1920x1080
     - FPS: 30
   - Settings → Output:
     - Recording Quality: High Quality
     - Recording Format: mp4
     - Encoder: Apple VT H264 Hardware Encoder

3. **Add Sources:**
   - Click "+" under Sources
   - Add "Display Capture" (for screen)
   - Add "Audio Input Capture" (for microphone)

4. **Start Recording:**
   - Click "Start Recording" button
   - Click "Stop Recording" when done

**Files saved to:** ~/Movies/ by default

### Option 3: Zoom (Good for Picture-in-Picture)

1. **Open Zoom**
2. **New Meeting**
3. **Share Screen** → Select screen/window
4. **Click Record** → Record to Computer
5. **Stop when done**

**Files saved to:** ~/Documents/Zoom/

---

## 📝 Detailed Video Script

### PART 1: System Demonstration (5-7 minutes)

#### Scene 1: Introduction (30 seconds)
```
"Hello, I'm [Your Name]. Today I'll demonstrate my high-performance 
REG NMS-compliant matching engine built in C++ for the Go-Quant 
backend assignment.

This engine implements price-time priority matching, supports 
multiple order types, and achieves throughput of 15,000-25,000 
orders per second with sub-100 microsecond median latency.

Let's start by running the system."
```

#### Scene 2: Start Matching Engine (30 seconds)
**ACTION:**
```bash
# Terminal 1
cd /Users/shubhupadhyay/Downloads/Go-Quant/build
./matching_engine
```

**SAY:**
```
"I'm starting the matching engine. As you can see, it initializes
the WebSocket server on port 8080 and is ready to accept connections.
The engine uses Boost.Beast for WebSocket handling and provides
real-time market data and trade execution streams."
```

**SHOW:** Engine startup logs, WebSocket listener ready

#### Scene 3: Connect Python Client (45 seconds)
**ACTION:**
```bash
# Terminal 2
cd /Users/shubhupadhyay/Downloads/Go-Quant
python3 tools/example_ws_client.py
```

**SAY:**
```
"Now I'm connecting with a Python WebSocket client. This demonstrates
the API that traders would use to interact with the exchange.

The client first subscribes to market data and trade feeds for the
BTC-USDT symbol. You can see the subscription confirmations."
```

**SHOW:** Connection success, subscription confirmations

#### Scene 4: Submit Limit Orders (1 minute)
**ACTION:** Modify `example_ws_client.py` to submit orders interactively, or use a prepared script

**SAY:**
```
"Let me submit a limit sell order at price 50,000 for 1 BTC.
[Submit order]

Notice the engine confirms the order submission and immediately 
publishes a market data update showing the new best ask.

The BBO (Best Bid and Offer) is now visible with the sell order
at the top of the order book.

Now I'll submit a limit buy order at 49,999 for 2 BTC.
[Submit order]

You can see the updated market data with both bid and ask sides
populated."
```

**SHOW:** 
- Order submission JSON
- Market data updates with BBO
- Order book depth

#### Scene 5: Market Order Execution (45 seconds)
**ACTION:**
```python
# Submit a market buy order
{
  "type": "submit_order",
  "id": "market_buy_1",
  "symbol": "BTC-USDT",
  "side": "buy",
  "order_type": "market",
  "quantity": 1.0
}
```

**SAY:**
```
"Now I'll submit a market buy order for 1 BTC.

Watch what happens - the market order immediately matches with the
best available sell order at 50,000. 

You can see:
1. The trade execution report showing the fill
2. The maker and taker order IDs
3. The execution price and quantity
4. Fee calculations (maker gets rebate, taker pays fee)
5. Updated market data showing the ask side is now empty"
```

**SHOW:**
- Trade execution JSON
- Fee calculations
- Updated order book

#### Scene 6: IOC and FOK Orders (1 minute)
**ACTION:**
```python
# IOC Order - partial fill
{
  "type": "submit_order",
  "id": "ioc_1",
  "symbol": "BTC-USDT",
  "side": "buy",
  "order_type": "ioc",
  "quantity": 5.0,
  "price": 49,999
}

# FOK Order - can't fill completely
{
  "type": "submit_order",
  "id": "fok_1",
  "symbol": "BTC-USDT",
  "side": "sell",
  "order_type": "fok",
  "quantity": 10.0,
  "price": 50,000
}
```

**SAY:**
```
"Let me demonstrate IOC and FOK orders.

First, an IOC (Immediate-Or-Cancel) order for 5 BTC at 49,999.
Since we only have 2 BTC available on the bid side, it fills
2 BTC and cancels the remaining 3 BTC. This is the expected behavior.

Now a FOK (Fill-Or-Kill) order for 10 BTC. Since we can't fill
the entire 10 BTC immediately, the engine rejects the entire order.
FOK is all-or-nothing - no partial fills allowed."
```

**SHOW:**
- IOC partial fill
- FOK rejection
- Order book remains unchanged after FOK

#### Scene 7: Order Modification (45 seconds)
**ACTION:**
```python
# Modify existing order
{
  "type": "modify_order",
  "order_id": "existing_order_id",
  "new_quantity": 1.5
}
```

**SAY:**
```
"The engine supports order modification. I'm reducing an existing
order's quantity from 2 BTC to 1.5 BTC.

The engine updates the order book and publishes an incremental
market data update showing the quantity change at this price level.

This maintains price-time priority - the order keeps its place in
the queue, only the quantity changes."
```

**SHOW:**
- Modification confirmation
- Updated market data
- Maintained queue position

#### Scene 8: Order Cancellation (30 seconds)
**ACTION:**
```python
{
  "type": "cancel_order",
  "order_id": "order_to_cancel"
}
```

**SAY:**
```
"Now I'll cancel an order. The engine removes it from the order book
and publishes an update showing the price level now has less quantity
or is completely removed if this was the last order."
```

**SHOW:**
- Cancellation confirmation
- Updated order book

#### Scene 9: Advanced Order Types (1 minute)
**ACTION:**
```python
# Stop-Loss Order
{
  "type": "submit_order",
  "id": "stop_loss_1",
  "symbol": "BTC-USDT",
  "side": "sell",
  "order_type": "stop_loss",
  "quantity": 1.0,
  "stop_price": 49,500
}
```

**SAY:**
```
"The engine also supports advanced order types. Here's a stop-loss
sell order with a trigger price of 49,500.

The order is held in a trigger queue, not the main order book.
When a trade occurs at or below 49,500, this converts to a market
sell order and executes immediately.

This implements the bonus requirement for advanced order types
including stop-loss, stop-limit, and take-profit orders."
```

**SHOW:**
- Stop order confirmation
- Trigger queue (if visible in logs)

---

### PART 2: Code Walkthrough (8-10 minutes)

#### Scene 1: Matching Algorithm - Price-Time Priority (3 minutes)

**OPEN:** `src/engine/matching_algorithm.cpp`

**NAVIGATE TO:** `matchLimitOrder` function

**SAY:**
```
"Let me show you the core matching algorithm. This function implements
the price-time priority matching that's compliant with REG NMS principles.

[Scroll to the matching logic]

Here's how it works:

1. First, we determine which side of the book to match against.
   Buy orders match against asks, sell orders against bids.

2. We iterate through price levels in order - best price first.
   For buy orders, we take from the lowest ask prices.
   For sell orders, we take from the highest bid prices.

3. At each price level, we maintain a FIFO queue of orders.
   [Point to the code]
   
   See this loop? It processes orders at each price level in time order.
   The first order at this price gets filled first - that's the 
   time priority component.

4. We check if we can match at this price level.
   [Point to price comparison]
   
   This prevents trade-throughs. We never match at a worse price
   when a better price is available.

5. For each matched order, we create a trade execution and update
   both the maker and taker orders.

6. If the incoming order is filled completely, we're done.
   Otherwise, we move to the next price level.

This ensures we always get the best execution - matching at the
most favorable prices first, in time order at each price level."
```

**HIGHLIGHT:**
- Price level iteration
- FIFO queue processing
- Trade execution creation
- No trade-through logic

#### Scene 2: Order Book Data Structures (2 minutes)

**OPEN:** `include/engine/order_book.h`

**NAVIGATE TO:** Class definition

**SAY:**
```
"Now let's look at the data structures that make this efficient.

[Show the OrderBook class]

The order book uses a two-level structure:

1. Price Levels:
   [Point to std::map for bids/asks]
   
   We use std::map (a red-black tree) to store price levels.
   This gives us O(log n) insertion and deletion, but more importantly,
   it keeps prices sorted automatically.
   
   For bids, we use greater<> to sort descending (highest first).
   For asks, we use less<> to sort ascending (lowest first).

2. Orders at Each Price Level:
   [Point to std::vector]
   
   At each price level, we use a std::vector to maintain FIFO order.
   New orders are pushed to the back, and we process from the front.
   
   This is cache-friendly and gives us O(1) access to the next
   order to fill.

3. Order Lookup:
   [Point to std::unordered_map]
   
   We maintain a hash map of order ID to order pointer.
   This gives us O(1) lookup for cancellations and modifications.

4. BBO Maintenance:
   [Point to bestBid/bestAsk tracking]
   
   We cache pointers to the best bid and ask.
   When the best level is exhausted, we simply move to the next
   level in the map. This is O(1) for reads and O(log n) for updates.

The entire structure is designed for minimal latency on the
critical path - order matching and BBO updates."
```

**HIGHLIGHT:**
- std::map for price levels
- std::vector for FIFO
- std::unordered_map for lookups
- BBO caching

#### Scene 3: Memory Optimizations (2 minutes)

**OPEN:** `include/utils/memory_pool.h`

**SAY:**
```
"Performance optimization is crucial for a matching engine.
Let me show you the memory pooling implementation.

[Show MemoryPool class]

Instead of allocating and deallocating orders constantly,
we use an object pool:

1. Pre-allocation:
   We pre-allocate blocks of memory for order objects.
   
2. Free List:
   When an order is cancelled or filled, we return it to
   the free list instead of deallocating.
   
3. Reuse:
   When a new order comes in, we grab from the free list
   rather than allocating new memory.

This eliminates allocation overhead and reduces memory fragmentation.
Our benchmarks show 85-95% efficiency, meaning most orders are
served from the pool rather than requiring new allocations.

[Open ring_buffer.h]

For market data and trade feeds, we use lock-free ring buffers.

This is a single-producer, single-consumer queue that uses
atomic operations instead of locks.

The matching engine writes to the buffer, and the network
layer reads from it without blocking each other.

This is critical because we want order matching to proceed
at maximum speed without waiting for network I/O."
```

**HIGHLIGHT:**
- Object pooling concept
- Free list management
- Lock-free ring buffer
- Atomic operations

#### Scene 4: Trade Execution Logic (1 minute)

**OPEN:** `src/engine/matching_algorithm.cpp`

**NAVIGATE TO:** `executeTrade` function

**SAY:**
```
"Here's where we generate trade execution data.

When orders match, we create a Trade object with:
- Unique trade ID
- Both order IDs (maker and taker)
- Execution price and quantity
- Timestamp with microsecond precision
- Fee calculations using the fee model

The fee model is injectable, so we can configure different
fee schedules per symbol. By default, makers get a rebate
(negative fee) and takers pay a fee.

The trade is then published to all subscribers via the
trade feed. This all happens synchronously in the matching
path to ensure immediate confirmation."
```

**HIGHLIGHT:**
- Trade struct creation
- Fee calculation
- Trade publishing

---

### PART 3: Design Discussion (3-5 minutes)

#### Scene 1: REG NMS Compliance (1.5 minutes)

**SHOW:** Diagram or code comments

**SAY:**
```
"Let me explain how this engine implements REG NMS principles,
even though we're trading crypto, not equities.

REG NMS has three key principles that we've adopted:

1. Price-Time Priority:
   Orders are matched strictly by price first, then time.
   Better prices always execute first.
   At the same price, earlier orders execute first.
   
2. Order Protection (No Trade-Throughs):
   An incoming order must match at the best available price.
   It cannot 'trade through' better prices to hit worse prices.
   
   [Point to code]
   You saw this in the matching algorithm - we iterate through
   price levels and stop when we can't improve the price.
   
3. Internal Market Data:
   We maintain and disseminate our own BBO and order book data.
   This allows traders to see the best prices and make informed
   decisions.

These principles ensure fair and efficient price discovery,
which is what professional exchanges require."
```

#### Scene 2: Architecture Decisions (1.5 minutes)

**SAY:**
```
"Several key architecture decisions shaped this design:

1. Single-threaded matching per symbol:
   Each symbol's order book is single-threaded.
   This eliminates lock contention on the critical path.
   Different symbols can process in parallel, but within
   a symbol, we guarantee sequential consistency.

2. Async I/O for networking:
   Network operations are async to avoid blocking the
   matching engine. We use Boost.Asio for this.

3. Separation of concerns:
   - Engine layer: Pure matching logic
   - Network layer: WebSocket handling
   - Utils layer: Performance primitives
   
   This makes the code testable and maintainable.

4. JSON for messages:
   JSON is human-readable and easy to debug.
   For production, we could use Protocol Buffers or
   FlatBuffers for better performance, but JSON is
   perfect for this assignment.

5. Memory pooling:
   Given that order objects are created and destroyed
   constantly, pooling dramatically reduces allocation overhead."
```

#### Scene 3: Performance Optimizations (1 minute)

**SAY:**
```
"The key optimizations that enable 15,000-25,000 orders/sec:

1. Data structure selection:
   - std::map for sorted price levels (O(log n))
   - std::vector for FIFO queues (cache-friendly)
   - std::unordered_map for O(1) lookups
   
2. Memory management:
   - Object pooling reduces allocations by 85-95%
   - Pre-allocated buffers for strings and messages
   
3. Lock-free algorithms:
   - Ring buffers for data dissemination
   - Atomic operations where possible
   
4. BBO caching:
   - We maintain pointers to best levels
   - No searching required for every query
   
5. Minimal copying:
   - Shared pointers for orders
   - Move semantics throughout
   
6. Optimized critical path:
   - Matching algorithm is inline-heavy
   - Hot paths are optimized for CPU cache"
```

#### Scene 4: Trade-offs (1 minute)

**SAY:**
```
"Every design involves trade-offs. Here are the key ones:

1. Simplicity vs. Absolute Performance:
   We used JSON instead of binary protocols.
   This costs some performance but makes debugging easier
   and the API more accessible.
   
2. Single-threaded matching:
   This simplifies logic and eliminates race conditions,
   but means we can't parallelize within a symbol.
   Trade-off: Correctness and simplicity over maximum
   theoretical throughput.
   
3. Memory for speed:
   Object pools pre-allocate memory.
   We use more memory to avoid allocation overhead.
   
4. Synchronous trade publication:
   Trades are published in the matching path.
   This adds latency but guarantees immediate confirmation.
   We could async this, but then we'd need complex error
   handling for failed publications.

These trade-offs were made to prioritize correctness,
maintainability, and debuggability while still achieving
excellent performance."
```

---

### Closing (30 seconds)

**SAY:**
```
"To summarize, this matching engine:
- Implements all required order types plus bonus advanced orders
- Achieves 15,000-25,000 orders per second throughput
- Maintains sub-100 microsecond median latency
- Provides complete REG NMS compliance
- Includes persistence via Write-Ahead Log
- Has comprehensive test coverage

Thank you for watching. The complete source code, documentation,
and benchmarks are available in the GitHub repository.

I'm excited about the opportunity to discuss this further."
```

---

## ✅ Post-Recording Checklist

1. **Review the video**
   - Check audio quality
   - Ensure screen is readable
   - Verify all demos worked
   - Confirm 15-20 minute length

2. **Edit if needed**
   - Trim dead time
   - Add title slide (optional)
   - Add your name and contact

3. **Export**
   - Format: MP4
   - Resolution: 1080p
   - Bitrate: High quality

4. **Upload**
   - Option 1: YouTube (unlisted)
   - Option 2: Google Drive
   - Option 3: Dropbox
   - Set permissions to "anyone with link"

5. **Test the link**
   - Open in incognito
   - Verify it plays
   - Check quality

---

## 📌 Tips for a Great Recording

### Before Recording:
- ✅ Practice the demo flow once
- ✅ Close notification centers
- ✅ Clear terminal history
- ✅ Have water nearby
- ✅ Test microphone levels
- ✅ Ensure good lighting (if camera on)

### During Recording:
- 🎤 Speak clearly and at moderate pace
- 👆 Use cursor to point at code sections
- ⏸️ Pause briefly between sections
- 🔍 Zoom in on important code (Cmd+ in most editors)
- 📖 Explain "why" not just "what"

### Common Pitfalls to Avoid:
- ❌ Rushing through explanations
- ❌ Assuming too much knowledge
- ❌ Not showing results of actions
- ❌ Background noise
- ❌ Too small font size
- ❌ Going over 25 minutes

### If Something Goes Wrong:
- Don't panic!
- Pause, explain what should happen
- Show the error if it's interesting
- Restart that section if needed
- Remember: This shows problem-solving skills

---

## 🎯 Quick Recording Workflow

**Fastest Path (Using QuickTime):**

```bash
# 1. Prepare terminals
cd /Users/shubhupadhyay/Downloads/Go-Quant
# Terminal 1: Start engine
# Terminal 2: Ready for client

# 2. Open code files in VS Code
code .

# 3. Start QuickTime
# File → New Screen Recording

# 4. Click record and follow the script

# 5. Stop recording (menu bar icon)

# 6. File → Export → 1080p

# 7. Upload to YouTube/Drive

# 8. Get shareable link

# Done!
```

---

## 📧 Adding Video to Submission Email

```
VIDEO DEMONSTRATION:
Link: [Your YouTube/Drive Link]
Duration: 18 minutes
Covers: System demo, code walkthrough, design discussion

Timestamps:
00:00 - Introduction
00:30 - System startup
02:00 - Order submission demo
06:00 - Code walkthrough (matching algorithm)
14:00 - Design discussion
17:30 - Conclusion
```

---

## 🆘 Troubleshooting

**Issue: Engine won't start**
```bash
cd build
cmake --build . --config Release
./matching_engine
```

**Issue: Python client can't connect**
```bash
# Check if engine is running
lsof -i :8080

# Install websockets if missing
pip3 install websockets
```

**Issue: Video too large**
- Use HandBrake to compress
- Or upload to YouTube (no size limit)

**Issue: Screen is blurry**
- Record in full screen
- Check QuickTime/OBS settings
- Export at 1080p minimum

---

**You've got this! Your implementation is excellent - now just show it off! 🚀**
