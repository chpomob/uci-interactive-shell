#!/bin/bash

echo "Testing UCI Interactive Shell Commands"
echo "======================================"

# Build the project
make

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "1. Testing get_device_info command:"
echo "get_device_info" | timeout 2s ./uci-shell

echo ""
echo "2. Testing get_caps_info command:"
echo "get_caps_info" | timeout 2s ./uci-shell

echo ""
echo "3. Testing session_init command:"
echo "session_init" | timeout 2s ./uci-shell

echo ""
echo "4. Testing set_config command:"
echo "set_config" | timeout 2s ./uci-shell

echo ""
echo "5. Testing device_reset command:"
echo "device_reset" | timeout 2s ./uci-shell

echo ""
echo "6. Testing get_device_state command:"
echo "get_device_state" | timeout 2s ./uci-shell

echo ""
echo "All tests completed."