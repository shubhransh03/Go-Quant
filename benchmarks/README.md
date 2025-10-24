Benchmarks
==========

This folder contains small benchmarks that exercise the matching engine.

bench_runner.cpp
----------------
A minimal harness that uses the public MatchingEngine API (createOrder + submitOrder) to measure average latency for market orders.

How to run locally (Windows PowerShell)
--------------------------------------
# 1. Ensure you have CMake and a C++ toolchain (e.g., Visual Studio Build Tools) installed and on PATH.
# 2. From the repo root:
if (-Not (Test-Path build)) { New-Item -ItemType Directory -Path build }
Set-Location build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -- /m

# 3. Run the bench_runner executable (path may vary):
.\build\\bench_runner.exe

Notes
-----
- The bench_runner is intentionally simple and single-threaded. For more robust production benchmarking, consider:
  - Pinning threads to CPUs and disabling frequency scaling.
  - Running in release mode with an optimized compiler.
  - Using high-resolution timers and collecting distribution percentiles.

