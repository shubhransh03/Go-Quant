"""
Simple example WebSocket client to interact with the matching engine.
Requires: Python 3.8+, websockets, asyncio
Install dependencies:
    pip install websockets

This script connects to ws://localhost:8080 and demonstrates:
 - subscribe to market data
 - subscribe to trades
 - submit a limit sell order
 - submit a market buy order

Run:
    python tools/example_ws_client.py
"""
import asyncio
import json
import websockets

URI = "ws://localhost:8080"

async def main():
    async with websockets.connect(URI) as ws:
        # Subscribe to market data and trades for the symbol
        await ws.send(json.dumps({"type": "subscribe_market_data", "symbol": "BTC-USDT"}))
        print(await ws.recv())

        await ws.send(json.dumps({"type": "subscribe_trades", "symbol": "BTC-USDT"}))
        print(await ws.recv())

        # Submit a limit sell order (maker)
        limit_sell = {
            "type": "submit_order",
            "id": "sell1",
            "symbol": "BTC-USDT",
            "side": "sell",
            "order_type": "limit",
            "quantity": 1.0,
            "price": 100.0
        }
        await ws.send(json.dumps(limit_sell))
        print(await ws.recv())

        # Submit a market buy order (taker)
        market_buy = {
            "type": "submit_order",
            "id": "buy1",
            "symbol": "BTC-USDT",
            "side": "buy",
            "order_type": "market",
            "quantity": 1.0
        }
        await ws.send(json.dumps(market_buy))
        print(await ws.recv())

        # Read a few messages (market updates and trades)
        for _ in range(5):
            try:
                msg = await asyncio.wait_for(ws.recv(), timeout=2.0)
                print("RECV:", msg)
            except asyncio.TimeoutError:
                break

if __name__ == '__main__':
    asyncio.run(main())
