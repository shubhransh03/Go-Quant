FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV CMAKE_VERSION=3.20.3

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    g++ \
    git \
    libboost-all-dev \
    libgtest-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Create a working directory
WORKDIR /app

# Copy the source code into the container
COPY . .

# Build the project
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Run tests
RUN cd build && \
    ./tests/unit/test_matching_engine && \
    ./tests/unit/test_advanced_orders

# Expose WebSocket port
EXPOSE 8080

# Set up entrypoint script
COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Default command
ENTRYPOINT ["/entrypoint.sh"]
CMD ["./build/matching_engine"]