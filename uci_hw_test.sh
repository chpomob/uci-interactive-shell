#!/bin/bash

# UCI Hardware Test Script
# Tests communication with real UWB hardware via character device files

echo "=== UCI Hardware Test Script ==="
echo ""

# Check if device path is provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <device_path> [baud_rate]"
    echo "  Example: $0 /dev/ttyUSB0 115200"
    echo ""
    echo "Available UWB devices:"
    ls /dev/ttyUSB* 2>/dev/null || echo "  None found"
    echo ""
    exit 1
fi

DEVICE_PATH="$1"
BAUD_RATE="${2:-115200}"

# Check if device exists
if [ ! -e "$DEVICE_PATH" ]; then
    echo "Error: Device $DEVICE_PATH does not exist"
    exit 1
fi

# Check permissions
if [ ! -r "$DEVICE_PATH" ] || [ ! -w "$DEVICE_PATH" ]; then
    echo "Warning: You may not have read/write permissions for $DEVICE_PATH"
    echo "You might need to run with sudo or add yourself to the dialout group:"
    echo "  sudo usermod -a -G dialout $USER"
    echo ""
fi

echo "Testing UCI communication with device: $DEVICE_PATH at $BAUD_RATE baud"
echo ""

# Test with stty to configure the device
echo "Configuring device..."
stty -F "$DEVICE_PATH" $BAUD_RATE raw -echo -echoe -echok 2>/dev/null || {
    echo "Warning: Failed to configure device with stty"
    echo "This is normal if the device is not a serial terminal"
    echo ""
}

# Test basic communication by sending a simple UCI command
echo "Sending CORE_DEVICE_INFO command..."
echo -ne "\x20\x08\x00\x00" > "$DEVICE_PATH" 2>/dev/null || {
    echo "Error: Failed to send command to device"
    exit 1
}

echo "Waiting for response..."
# Try to read response with timeout
timeout 2 cat < "$DEVICE_PATH" > /tmp/uci_response.tmp 2>/dev/null &

# Give it a moment to read
sleep 1

# Kill the background process
kill %1 2>/dev/null || true

# Check if we got a response
if [ -s /tmp/uci_response.tmp ]; then
    echo "Received response:"
    hexdump -C /tmp/uci_response.tmp
    rm -f /tmp/uci_response.tmp
else
    echo "No response received (timeout)"
    rm -f /tmp/uci_response.tmp
fi

echo ""
echo "=== Hardware Test Complete ==="