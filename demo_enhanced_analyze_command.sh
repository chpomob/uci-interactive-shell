#!/bin/bash

# Enhanced UCI Packet Analysis Demo
# Demonstrates the advanced analyze command capabilities

echo "=== Enhanced UCI Packet Analysis Demo ==="
echo

# Start the UCI shell and pipe commands to it
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell

echo "1. Basic packet analysis:"
echo "   > analyze 20 08 00 00"
echo 'analyze 20 08 00 00' | ./uci-shell | grep -A 20 "UCI Packet Analysis"
echo

echo "2. Verbose packet analysis:"
echo "   > analyze -v 21 00 00 05 00 01 00 00 00"
echo 'analyze -v 21 00 00 05 00 01 00 00 00' | ./uci-shell | grep -A 30 "Enhanced Analysis"
echo

echo "3. TLV packet analysis:"
echo "   > analyze -t 21 03 00 0E DD CC BB AA 02 48 04 11 22 33 44 E5 01 05"
echo 'analyze -t 21 03 00 0E DD CC BB AA 02 48 04 11 22 33 44 E5 01 05' | ./uci-shell | grep -A 25 "SESSION_SET_APP_CONFIG"
echo

echo "4. Packet comparison:"
echo "   > analyze -c \"20 08 00 00\" \"21 00 00 05 00 01 00 00 00\""
echo 'analyze -c "20 08 00 00" "21 00 00 05 00 01 00 00 00"' | ./uci-shell | grep -A 15 "Packet Comparison"
echo

echo "5. Error code analysis:"
echo "   > analyze 40 04 00 04 04 01 01 04"
echo 'analyze 40 04 00 04 04 01 01 04' | ./uci-shell | grep -A 10 "Status Code Analysis"
echo

echo "6. Notification analysis:"
echo "   > analyze 61 02 00 06 01 00 00 00 00 00"
echo 'analyze 61 02 00 06 01 00 00 00 00 00' | ./uci-shell | grep -A 10 "SESSION_STATUS_NTF"
echo

echo "=== Demo Complete ==="
echo
echo "The enhanced analyze command provides:"
echo "- Better error code interpretation"
echo "- Enhanced TLV analysis with parameter recognition"
echo "- Packet comparison capabilities"
echo "- Contextual analysis based on Qorvo QM35 SDK patterns"
echo "- Verbose mode for detailed analysis"
echo "- TLV mode for advanced configuration analysis"