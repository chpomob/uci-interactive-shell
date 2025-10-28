#!/bin/bash

# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

# Demo script showcasing enhanced human-readable translations in UCI packet analysis
echo "=== UCI Interactive Shell Enhanced Human-Readable Translation Demo ==="
echo ""
echo "This demo showcases the enhanced packet analysis capabilities with human-readable"
echo "translations for UCI configuration parameters based on Qorvo QM35 SDK patterns."
echo ""

# Test core get config response with human-readable parameter translations
echo "1. Testing CORE_GET_CONFIG Response with Human-Readable Parameter Translations:"
echo "   > analyze_packet 40 05 00 09 00 02 00 01 01 01 02 34 12"
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell && echo -e "analyze_packet 40 05 00 09 00 02 00 01 01 01 02 34 12\nquit" | ./uci-shell | grep -A 25 "CORE_GET_CONFIG Response"

echo ""
echo "2. Testing CORE_SET_CONFIG Response with Human-Readable Parameter Translations:"
echo "   > analyze_packet 40 04 00 04 00 01 00 00"
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell && echo -e "analyze_packet 40 04 00 04 00 01 00 00\nquit" | ./uci-shell | grep -A 15 "CORE_SET_CONFIG Response"

echo ""
echo "3. Testing SESSION_GET_APP_CONFIG Response with Human-Readable Parameter Translations:"
echo "   > analyze_packet 41 04 00 0B 00 02 48 04 11 22 33 44 E5 01 05"
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell && echo -e "analyze_packet 41 04 00 0B 00 02 48 04 11 22 33 44 E5 01 05\nquit" | ./uci-shell | grep -A 20 "SESSION_GET_APP_CONFIG Response"

echo ""
echo "4. Testing SESSION_SET_APP_CONFIG Response with Human-Readable Parameter Translations:"
echo "   > analyze_packet 41 03 00 06 00 01 48 00"
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell && echo -e "analyze_packet 41 03 00 06 00 01 48 00\nquit" | ./uci-shell | grep -A 15 "SESSION_SET_APP_CONFIG Response"

echo ""
echo "5. Testing CORE_GET_CAPS_INFO Response with Human-Readable TLV Translations:"
echo "   > analyze_packet 40 02 00 49 DD CC BB AA 04 03 02 01 01 10 01 02 04 00 02 00 F6 EC 01 08 00 64 00 C8 00 2C 01 05 02 02 15 00 01 0A 00 14 00 1E 00 28 00 32 00 3C 00 07 02 04 00 AA BB CC DD 06 11 00 08 B0 FF 04 3C E1 02 A1 FF 52 B8 E3 02 C4 FF C0 B8"
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell && echo -e "analyze_packet 40 02 00 49 DD CC BB AA 04 03 02 01 01 10 01 02 04 00 02 00 F6 EC 01 08 00 64 00 C8 00 2C 01 05 02 02 15 00 01 0A 00 14 00 1E 00 28 00 32 00 3C 00 07 02 04 00 AA BB CC DD 06 11 00 08 B0 FF 04 3C E1 02 A1 FF 52 B8 E3 02 C4 FF C0 B8\nquit" | ./uci-shell | grep -A 30 "CORE_GET_CAPS_INFO Response"

echo ""
echo "6. Testing with Verbose Mode for Detailed Analysis:"
echo "   > analyze_packet -v 41 03 00 06 00 02 48 00 E5 00"
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell && echo -e "analyze_packet -v 41 03 00 06 00 02 48 00 E5 00\nquit" | ./uci-shell | head -30

echo ""
echo "7. Testing with TLV Mode for TLV-Specific Analysis:"
echo "   > analyze_packet -t 41 04 00 07 DD CC BB AA 02 48 E5"
cd /media/chpo/HDD-papa/gemini_test/uci_interactive_shell && echo -e "analyze_packet -t 41 04 00 07 DD CC BB AA 02 48 E5\nquit" | ./uci-shell | head -30

echo ""
echo "=== Enhanced Human-Readable Translation Demo Complete ==="
echo ""
echo "All enhanced analysis features are now available in the analyze_packet command:"
echo "  - Human-readable parameter name translations"
echo "  - Value interpretation for common parameters"
echo "  - Enhanced error analysis with contextual information"
echo "  - Range checking for numeric values"
echo "  - Color-coded output for better visualization"
echo ""
echo "The analyze_packet command now provides the same enhanced analysis capabilities"
echo "as found in professional-grade UCI analysis tools based on Qorvo QM35 SDK patterns."