#!/bin/bash

# UCI Hardware Interface Test Script
# Demonstrates how to use the UCI Interactive Shell with real UWB hardware

echo "=== UCI Hardware Interface Test Script ==="
echo ""

# Check if we have a UWB device connected
echo "Checking for connected UWB devices..."
UWB_DEVICES=$(ls /dev/ttyUSB* 2>/dev/null || echo "none")

if [ "$UWB_DEVICES" = "none" ]; then
    echo "No UWB devices found at /dev/ttyUSB*"
    echo "This is normal if you don't have real hardware connected."
    echo ""
    echo "To simulate hardware communication without real hardware:"
    echo "  1. Create a pseudo-terminal pair using socat:"
    echo "     socat -d -d pty,raw,echo=0,link=/tmp/uwb_master pty,raw,echo=0,link=/tmp/uwb_slave &"
    echo "  2. Run the UCI shell:"
    echo "     ./uci-shell"
    echo "  3. Connect to the slave PTY:"
    echo "     > hw_connect /tmp/uwb_slave"
    echo "  4. Send commands:"
    echo "     > get_device_info"
    echo "     > device_reset"
    echo ""
    echo "For real hardware, connect your UWB device and run:"
    echo "  ./uci-shell"
    echo "  > hw_connect /dev/ttyUSB0  # or whatever device path your UWB chip uses"
    echo "  > get_device_info"
    echo "  > device_reset"
    echo ""
else
    echo "Found UWB devices:"
    echo "$UWB_DEVICES"
    echo ""
    echo "To test with real hardware, run:"
    echo "  ./uci-shell"
    echo "  > hw_connect $UWB_DEVICES"
    echo "  > get_device_info"
    echo "  > device_reset"
    echo ""
fi

echo "=== Test Script Complete ==="