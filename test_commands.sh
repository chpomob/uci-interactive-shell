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
echo "7. Testing set_device_active command:"
echo "set_device_active" | timeout 2s ./uci-shell

echo ""
echo "8. Testing set_device_ready command:"
echo "set_device_ready" | timeout 2s ./uci-shell

echo ""
echo "9. Testing session_deinit command:"
echo "session_deinit" | timeout 2s ./uci-shell

echo ""
echo "10. Testing session_start command:"
echo "session_start" | timeout 2s ./uci-shell

echo ""
echo "11. Testing session_stop command:"
echo "session_stop" | timeout 2s ./uci-shell

echo ""
echo "12. Testing get_session_state command:"
echo "get_session_state" | timeout 2s ./uci-shell

echo ""
echo "All tests completed."