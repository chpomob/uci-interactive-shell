#!/bin/bash

# Test script for UCI hardware communication with simulated UWB device
# Creates a pseudo-terminal pair to simulate real UWB hardware communication

echo "Setting up simulated UWB device communication test..."

# Create a temporary directory for our test
TEMP_DIR=$(mktemp -d)
PTY_MASTER="$TEMP_DIR/master"
PTY_SLAVE="$TEMP_DIR/slave"

# Clean up on exit
trap "rm -rf $TEMP_DIR" EXIT

# Create a pseudo-terminal pair using socat
echo "Creating pseudo-terminal pair..."
socat -d -d pty,raw,echo=0,link=$PTY_MASTER pty,raw,echo=0,link=$PTY_SLAVE &
SOCAT_PID=$!

# Wait for the pseudo-terminals to be created
sleep 1

# Verify the pseudo-terminals were created
if [ ! -e "$PTY_MASTER" ] || [ ! -e "$PTY_SLAVE" ]; then
    echo "Error: Failed to create pseudo-terminals"
    kill $SOCAT_PID 2>/dev/null
    exit 1
fi

echo "Pseudo-terminal pair created:"
echo "  Master (simulate device): $PTY_MASTER"
echo "  Slave (connect to): $PTY_SLAVE"

# In one terminal, run the UCI shell connecting to the slave PTY
echo ""
echo "Starting UCI shell connected to simulated UWB device..."
echo "Commands to try:"
echo "  hw_connect $PTY_SLAVE     # Connect to the simulated device"
echo "  get_device_info          # Send device info command"
echo "  device_reset             # Reset the device"
echo "  quit                     # Exit the shell"
echo ""

# Run the UCI shell in the background
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell
timeout 30s ./uci-shell <<< $"hw_connect $PTY_SLAVE
get_device_info
device_reset
quit" &

UCI_SHELL_PID=$!

# In another terminal, simulate a UWB device responding on the master PTY
echo "Simulating UWB device responses on $PTY_MASTER..."
{
    # Wait a moment for the UCI shell to connect
    sleep 2
    
    # Send a response to the device info command
    echo -ne "\x40\x02\x00\x09\x00\x01\x00\x02\x00\x02\x00\x02\x00\x01\x00" > $PTY_MASTER
    
    # Send a response to the device reset command
    echo -ne "\x40\x00\x00\x01\x00" > $PTY_MASTER
    
    # Send a device status notification
    echo -ne "\x60\x01\x00\x01\x02" > $PTY_MASTER
    
} &

SIM_PID=$!

# Wait for the UCI shell to finish
wait $UCI_SHELL_PID

# Clean up
kill $SOCAT_PID $SIM_PID 2>/dev/null

echo ""
echo "Test completed."