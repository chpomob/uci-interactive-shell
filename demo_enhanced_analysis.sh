#!/bin/bash

# UCI Interactive Shell - Enhanced Analysis Demo
# This script demonstrates the enhanced packet analysis capabilities

echo "=== UCI Interactive Shell Enhanced Analysis Demo ==="
echo

# Start the UCI shell
echo "Starting UCI Interactive Shell..."
echo

# Demonstrate basic packet analysis
echo "1. Basic packet analysis:"
echo "   > analyze_packet 20 08 00 00"
echo "   > analyze_packet 21 00 00 05 00 00 00 01 00"
echo "   > analyze_packet 21 03 00 0E DD CC BB AA 02 48 04 11 22 33 44 E5 01 05"
echo

# Demonstrate enhanced error analysis
echo "2. Enhanced error analysis:"
echo "   > analyze_packet 40 04 00 04 04 01 01 04"
echo "   (CORE_SET_CONFIG response with INVALID_PARAM error)"
echo

# Demonstrate session command analysis
echo "3. Session command analysis:"
echo "   > analyze_packet 41 00 00 05 00 01 00 00 00"
echo "   (SESSION_INIT response with success status)"
echo

# Demonstrate advanced TLV analysis
echo "4. Advanced TLV analysis:"
echo "   > analyze_packet 40 05 00 09 00 02 00 01 01 01 02 34 12"
echo "   (CORE_GET_CONFIG response with multiple TLVs)"
echo

echo "=== Demo Complete ==="
echo
echo "To try these commands yourself:"
echo "1. Run './uci-shell'"
echo "2. Enter the commands shown above"
echo "3. Observe the enhanced analysis output"