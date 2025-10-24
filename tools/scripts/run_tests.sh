#!/bin/bash

# This script automates the execution of unit and integration tests.

# Navigate to the tests directory
cd ../.. || exit

# Run unit tests
echo "Running unit tests..."
mkdir -p build
cd build || exit
cmake ..
make test_unit

# Run integration tests
echo "Running integration tests..."
make test_integration

echo "All tests executed."