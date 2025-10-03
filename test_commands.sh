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
echo "3. Testing session_init command (user-friendly):"
echo "session_init 1 fira_ranging" | timeout 2s ./uci-shell

echo ""
echo "4. Testing set_config command (user-friendly):"
echo "set_config device_state active" | timeout 2s ./uci-shell

echo ""
echo "4.1. Testing set_config for low_power_mode:"
echo "set_config low_power_mode off" | timeout 2s ./uci-shell

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
echo "session_deinit 1" | timeout 2s ./uci-shell

echo ""
echo "10. Testing session_start command:"
echo "session_start 1" | timeout 2s ./uci-shell

echo ""
echo "11. Testing session_stop command:"
echo "session_stop 1" | timeout 2s ./uci-shell

echo ""
echo "12. Testing get_session_state command:"
echo "get_session_state 1" | timeout 2s ./uci-shell

echo ""
echo "13. Testing set_app_config command:"
echo "set_app_config 1 device_type responder" | timeout 2s ./uci-shell

echo ""
echo "14. Testing device_suspend command:"
echo "device_suspend" | timeout 2s ./uci-shell

echo ""
echo "All tests completed."