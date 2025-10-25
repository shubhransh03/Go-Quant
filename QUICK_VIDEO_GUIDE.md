# Quick Video Recording Guide - 5 Minute Setup

This is the FASTEST way to record your demo video. Everything is automated!

## 🚀 Setup (2 Minutes)

### Step 1: Build the Engine
```bash
cd /Users/shubhupadhyay/Downloads/Go-Quant
rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j4
```

### Step 2: Arrange Your Screen
- **Terminal 1 (Left)**: For matching engine
- **Terminal 2 (Right)**: For automated demo
- Make sure both are visible side-by-side

### Step 3: Increase Font Size
- In Terminal: `Cmd + +` to zoom in
- Make sure text is readable on recording

---

## 🎥 Recording (3 Minutes Setup)

### Using QuickTime (Simplest):

1. **Open QuickTime Player**
2. **File → New Screen Recording**
3. **Click Options:**
   - Microphone: Built-in Microphone
   - Quality: Maximum
4. **Click red Record button**
5. **Click anywhere to record full screen** (or drag to select area)

---

## ▶️ Running the Demo (15-20 Minutes)

### Terminal 1 - Start Engine:
```bash
cd /Users/shubhupadhyay/Downloads/Go-Quant/build
./matching_engine
```

**Wait for**: `WebSocket server listening on port 8080`

### Terminal 2 - Run Automated Demo:
```bash
cd /Users/shubhupadhyay/Downloads/Go-Quant
python3 tools/automated_demo.py
```

**Press Enter** when ready!

---

## 🎤 What to Say While Recording

The script will pause and show you what's happening. Just narrate it!

### Part 1: Introduction (While demo starts)
```
"Hello, I'm [Your Name]. This is my REG NMS matching engine 
built in C++ for the Go-Quant assignment. 

Let me show you how it works by running through all the features.
I'll start by connecting to the matching engine."
```

### Part 2: During Demo (Read what appears)
The script shows you:
- ▶ **ACTION**: What it's doing
- ✓ **RESULT**: What happened
- 💡 **WHAT HAPPENED**: Explanation

**Just read these out loud and explain!**

Example:
```
[Script shows: "Placing sell order: 1.0 BTC @ $50,000"]

You say: "Now I'm placing a limit sell order for 1 Bitcoin 
at 50,000 dollars. You can see the order is accepted and 
the market data updates to show this new best offer."
```

### Part 3: Code Walkthrough (After demo completes)
```bash
# Open VS Code
code /Users/shubhupadhyay/Downloads/Go-Quant
```

Show these files and explain:
1. **src/engine/matching_algorithm.cpp** - Price-time priority logic
2. **include/engine/order_book.h** - Data structures
3. **include/utils/memory_pool.h** - Performance optimizations

### Part 4: Closing
```
"That's the complete demonstration. This engine handles all 
order types, achieves 15,000+ orders per second, and maintains 
sub-100 microsecond latency. 

The code is on GitHub. Thank you for watching!"
```

---

## 🎬 What the Automated Demo Shows

The script automatically demonstrates:

1. **Subscriptions** (2 min)
   - Market data feed
   - Trade execution feed

2. **Limit Orders** (3 min)
   - Building the order book
   - Multiple price levels
   - Bid and ask sides

3. **Market Orders** (2 min)
   - Immediate execution
   - Matching with best price
   - Fee calculations

4. **IOC Orders** (2 min)
   - Partial fills
   - Immediate cancellation of remainder

5. **FOK Orders** (1 min)
   - All-or-nothing behavior
   - Complete rejection if can't fill

6. **Modifications** (1 min)
   - Changing order quantity
   - Maintaining time priority

7. **Cancellations** (1 min)
   - Removing orders
   - Order book updates

8. **Advanced Orders** (2 min)
   - Stop-loss orders
   - Take-profit orders

**Total: ~15 minutes automated + 5 minutes code walkthrough = 20 minutes**

---

## ✅ After Recording

1. **Stop Recording**: Click stop icon in menu bar
2. **Export**: File → Export As → 1080p
3. **Upload to YouTube**:
   - Go to youtube.com
   - Click "Create" → "Upload video"
   - Set to "Unlisted"
   - Title: "REG NMS Matching Engine - [Your Name]"
   - Get shareable link
4. **Test**: Open link in incognito to verify

---

## 🆘 Troubleshooting

**Engine won't start:**
```bash
cd build
cmake --build . --config Release
./matching_engine
```

**Demo can't connect:**
- Make sure engine shows "listening on port 8080"
- Check if port is blocked: `lsof -i :8080`

**Need to re-record a section:**
- Just stop, restart the engine and demo
- QuickTime lets you trim the video later

---

## 📧 Email Template

```
Subject: Backend Assignment - REG NMS Matching Engine

Dear Go-Quant Team,

I'm submitting my completed backend matching engine assignment.

DELIVERABLES:
✓ Source Code: https://github.com/shubhransh03/Go-Quant
✓ Video Demo: [Your YouTube Link]
✓ Resume: [Attached]

The matching engine implements:
- All required order types (Market, Limit, IOC, FOK)
- Bonus features (Stop-Loss, Take-Profit, Persistence)
- Performance: 15,000-25,000 orders/sec, <100μs latency
- Complete test coverage (18+ test suites)

Video timestamps:
00:00 - Introduction
02:00 - System demonstration
15:00 - Code walkthrough
20:00 - Conclusion

Thank you for your consideration.

Best regards,
[Your Name]
[Your Contact]
```

---

## 💡 Pro Tips

1. **Practice Once**: Run the demo once before recording
2. **Speak Clearly**: Don't rush, pause between sections
3. **Show Enthusiasm**: You built something awesome!
4. **Point at Screen**: Use your cursor to highlight things
5. **Don't Worry About Perfection**: Natural is better than perfect

---

**You're ready! This will take 20-25 minutes total. Good luck! 🚀**
