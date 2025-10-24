# WebSocket API

This document describes the WebSocket API used by the matching engine. All messages are JSON objects.

Subscriptions

- subscribe_market_data
  - request: { "type": "subscribe_market_data", "symbol": "SYMBOL" }
  - snapshot response: { "type": "market_data_snapshot", "symbol": "SYMBOL", "timestamp": "ISO8601 UTC", "seqNum": N, "bids": [[price,qty],...], "asks": [[price,qty],...], "bestBidPrice": X, "bestBidQuantity": Y, "bestAskPrice": A, "bestAskQuantity": B }
  - incremental response: { "type": "market_data_increment", "symbol": "SYMBOL", "timestamp": "ISO8601 UTC", "seqNum": N, "prevSeqNum": M, "gap": true|false, "bids_changes": [{"op":"add|update|remove","price":P, "quantity":Q?}, ...], "asks_changes": [...] , "bestBidPrice": X, "bestBidQuantity": Y, "bestAskPrice": A, "bestAskQuantity": B }

Notes on incremental updates:
- Clients should track `seqNum` and `prevSeqNum`. If `prevSeqNum + 1 != seqNum`, a gap exists and the client should request a fresh snapshot (or re-subscribe).
- `bids_changes` and `asks_changes` contain ordered per-level operations. `op` is one of `add`, `update`, `remove`. `quantity` is omitted for `remove`.

Trades

- subscribe_trades
  - request: { "type": "subscribe_trades", "symbol": "SYMBOL" }
  - response: { "type": "trade", "trade_id": "T123", "symbol": "SYMBOL", "price": P, "quantity": Q, "maker_order_id": "O1", "taker_order_id": "O2", "maker_fee": F1, "taker_fee": F2, "aggressor_side": "buy|sell", "timestamp": "ISO8601 UTC", "seqNum": N }

Order entry / modification

- submit_order: { "type": "submit_order", "id": "client-id", "symbol": "SYMBOL", "side": "buy|sell", "order_type": "limit|market|ioc|fok", "price": 100.0, "quantity": 1.0 }
- cancel_order: { "type": "cancel_order", "order_id": "O123" }
- modify_order: { "type": "modify_order", "order_id": "O123", "new_quantity": 0.5 }

Responses typically follow the shape { "status": "ok" | "error", "message": "...", ... } or contain structured error codes.

Timestamps are UTC ISO-8601 strings with microsecond precision.

Example: Applying an INCREMENT on the client
-------------------------------------------
1) Client receives a `market_data_snapshot` with `seqNum = 100` and builds internal book.
2) Client receives `market_data_increment` with `prevSeqNum = 100`, `seqNum = 101` and a small set of `bids_changes`.
   - Apply ops in order: for `add` insert price level, for `update` change quantity, for `remove` delete the price level.
3) If `prevSeqNum + 1 != seqNum` (gap == true), the client should request a full snapshot or re-subscribe and replay from the latest snapshot.

Error responses
---------------
- Common shape: { "status": "error", "code": "<error_code>", "message": "human readable" }
- Examples:
  - { "status": "error", "code": "invalid_request", "message": "missing required fields" }
  - { "status": "error", "code": "invalid_params", "message": "invalid order_id or quantity" }

Schema notes
------------
- Prices should be treated as exact decimal values by the client. If the client represents prices as floating point numbers, consider converting to integer tick units for exact comparisons when applying diffs.

# WebSocket API (Go-Quant)

This document describes the WebSocket request/response schemas used by the matching engine.

All messages use JSON. Timestamps are ISO-8601 UTC strings with microsecond precision, e.g. `2025-10-24T01:23:45.123456Z`.

## Requests

- submit_order
  - Description: Submit an order. The client supplies an `id` (client-generated or unique), `symbol`, `side`, `order_type`, `price` (for limit), and `quantity`.
  - Example:
    {
      "type": "submit_order",
      "id": "O123",
      "symbol": "BTC-USDT",
      "side": "buy",
      "order_type": "limit", // market|limit|ioc|fok|stop_loss|stop_limit|take_profit
      "price": 50000.0,       // required for limit and stop_limit
      "quantity": 0.01
    }

- cancel_order
  - Description: Cancel an existing order by ID.
  - Example:
    { "type": "cancel_order", "order_id": "O123" }

- modify_order
  - Description: Reduce (or set) remaining quantity for an existing order. Engine validates parameters.
  - Example:
    { "type": "modify_order", "order_id": "O123", "new_quantity": 0.005 }

- subscribe_market_data
  - Description: Subscribe to L2 market data (bids/asks, BBO, seqNum).
  - Example:
    { "type": "subscribe_market_data", "symbol": "BTC-USDT" }

- subscribe_trades
  - Description: Subscribe to trade execution feed.
  - Example:
    { "type": "subscribe_trades", "symbol": "BTC-USDT" }

- get_metrics
  - Description: Fetch engine metrics (JSON).
  - Example:
    { "type": "get_metrics" }

## Responses / Push messages

- market_data (SNAPSHOT)
  - Sent on subscription and when the book changes.
  - Fields:
    - type: "market_data"
    - symbol
    - timestamp: ISO-8601 UTC
    - seqNum: per-symbol increasing sequence number
    - bestBidPrice, bestBidQuantity, bestAskPrice, bestAskQuantity (BBO)
    - bids: L2 array, each entry [price, quantity]
    - asks: L2 array, each entry [price, quantity]
  - Example:
    {
      "type":"market_data",
      "symbol":"BTC-USDT",
      "timestamp":"2025-10-24T01:23:45.123456Z",
      "seqNum": 123,
      "bestBidPrice": 49900.0,
      "bestBidQuantity": 0.5,
      "bestAskPrice": 50000.0,
      "bestAskQuantity": 0.3,
      "bids":[[49900.0,0.5],[49800.0,1.0]],
      "asks":[[50000.0,0.3],[50100.0,2.0]]
    }

- market_data (INCREMENT)
  - Sent when only some levels change. The `type` is `market_data` and the payload's `type` field within the engine `MarketDataUpdate` is `INCREMENT`.
  - Fields (only changed sides may be present): seqNum, timestamp, bestBidPrice, bestAskPrice, bids, asks
  - Example (only bids changed):
    {
      "type":"market_data",
      "symbol":"BTC-USDT",
      "timestamp":"2025-10-24T01:24:00.000100Z",
      "seqNum": 124,
      "bestBidPrice": 49950.0,
      "bestBidQuantity": 0.2,
      "bids":[[49950.0,0.2]]
    }

- trade
  - Emitted for each executed trade.
  - Fields:
    - type: "trade"
    - trade_id
    - symbol
    - price, quantity
    - maker_order_id, taker_order_id
    - maker_fee, taker_fee
    - aggressor_side
    - timestamp (ISO-8601 UTC)
    - seqNum (per-symbol trade sequence)
  - Example:
    {
      "type":"trade",
      "trade_id":"TRD123",
      "symbol":"BTC-USDT",
      "price":50000.0,
      "quantity":0.01,
      "maker_order_id":"O1",
      "taker_order_id":"O2",
      "maker_fee":-0.005,
      "taker_fee":0.01,
      "aggressor_side":"buy",
      "timestamp":"2025-10-24T01:24:01.123456Z",
      "seqNum": 55
    }

## Error responses

Structured error responses follow the shape:
{ "status": "error", "code": "<error_code>", "message": "..." }

Common codes: invalid_request, invalid_params, invalid_order_type, not_found, processing_error

## Notes
- Sequence numbers (`seqNum`) are per-symbol and monotonic. Use them to order messages and detect gaps.
- Market data `INCREMENT` messages contain only changed sides (bids or asks) to reduce payload size; consumers should apply increments in order (seqNum) and request a SNAPSHOT if gaps are detected.
- Stop/trigger orders: `order_type` may be `stop_loss`, `stop_limit`, or `take_profit`. These are stored as trigger orders and will be activated when the last trade price satisfies their trigger condition. On activation the engine converts them to a MARKET (stop_loss/take_profit) or LIMIT (stop_limit) order with the same id and quantity.

If you want, I can also generate TypeScript/JSON schema files from these examples.
