#!/bin/bash

# Initialize the matching engine with configuration if provided
if [ -f "/config/engine.json" ]; then
    echo "Loading configuration from /config/engine.json"
    exec "$@" --config /config/engine.json
else
    echo "Starting with default configuration"
    exec "$@"
fi