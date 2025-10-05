#!/usr/bin/env python3
"""
Extract and analyze UCI test packets from the PDL specification file
"""

import re
import sys
from pathlib import Path

def extract_test_packets_from_pdl(pdl_file):
    """Extract test packets from the PDL file"""
    with open(pdl_file, 'r') as f:
        content = f.read()
    
    # Find test packets using regex
    test_pattern = r'test\s+(\w+)\s*{([^}]*(?:\{[^}]*\}[^}]*)*)}'
    matches = re.findall(test_pattern, content, re.MULTILINE | re.DOTALL)
    
    test_packets = []
    for match in matches:
        packet_type = match[0]
        test_content = match[1]
        
        # Find hex string patterns in the test content
        hex_pattern = r'"\\x([0-9a-fA-F\\x]+)"'
        hex_matches = re.findall(hex_pattern, test_content)
        
        for hex_match in hex_matches:
            # Clean up the hex string by removing \\x patterns
            clean_hex = hex_match.replace('\\x', ' ')
            clean_hex = ' '.join(clean_hex.split())  # Normalize spaces
            hex_bytes = [b for b in clean_hex.split() if b.strip()]
            hex_string = ''.join(hex_bytes)
            
            if len(hex_string) > 0 and len(hex_string) % 2 == 0:  # Valid hex string
                test_packets.append((packet_type, hex_string))
    
    # Alternative pattern for some test formats
    alt_test_pattern = r'\"([\\x0-9a-fA-F]+)\"'
    alt_matches = re.findall(alt_test_pattern, content)
    
    return test_packets

def parse_uci_header(packet_hex):
    """Parse UCI packet header"""
    if len(packet_hex) < 8:
        return None
    
    header_bytes = bytes.fromhex(packet_hex[:8])
    first_byte = header_bytes[0]
    
    # Extract fields based on UCI specification
    gid = first_byte & 0x0F           # Lower 4 bits
    pbf = (first_byte >> 4) & 0x01    # Bit 4
    mt = (first_byte >> 5) & 0x07     # Upper 3 bits
    opcode = header_bytes[1] & 0x3F   # Lower 6 bits of second byte
    payload_len = header_bytes[3]
    
    # GID mapping based on the PDL spec
    gid_names = {
        0x00: 'CORE',
        0x01: 'SESSION_CONFIG',
        0x02: 'SESSION_CONTROL', 
        0x03: 'DATA_CONTROL',
        0x09: 'VENDOR_RESERVED_9',
        0x0a: 'VENDOR_RESERVED_A', 
        0x0b: 'VENDOR_RESERVED_B',  # This is often used for RANGING_DATA
        0x0c: 'VENDOR_ANDROID',
        0x0d: 'TEST',
        0x0e: 'VENDOR_RESERVED_E',
        0x0f: 'VENDOR_RESERVED_F'
    }
    
    # MT mapping
    mt_names = {
        0x00: 'DATA',
        0x01: 'COMMAND',
        0x02: 'RESPONSE',
        0x03: 'NOTIFICATION'
    }
    
    return {
        'gid': gid,
        'gid_name': gid_names.get(gid, f'UNKNOWN(0x{gid:02x})'),
        'pbf': pbf,
        'mt': mt,
        'mt_name': mt_names.get(mt, f'UNKNOWN({mt})'),
        'opcode': opcode,
        'payload_len': payload_len,
        'total_len': len(packet_hex) // 2
    }

def main():
    pdl_file = Path(__file__).parent / 'uci_analysis/uwb/src/rust/uwb_uci_packets/uci_packets.pdl'
    
    if not pdl_file.exists():
        print(f"PDL file not found: {pdl_file}")
        return
    
    print("UCI Test Packets from PDL Specification")
    print("=" * 50)
    print()
    
    test_packets = extract_test_packets_from_pdl(pdl_file)
    
    if not test_packets:
        print("No test packets found in PDL file")
        return
    
    print(f"Found {len(test_packets)} test packets in the PDL specification:")
    print()
    
    # Group packets by type
    packet_types = {}
    for packet_type, hex_string in test_packets:
        if packet_type not in packet_types:
            packet_types[packet_type] = []
        packet_types[packet_type].append(hex_string)
    
    # Display grouped packets
    for packet_type, hex_strings in packet_types.items():
        print(f"{packet_type}:")
        for i, hex_string in enumerate(hex_strings):
            header_info = parse_uci_header(hex_string)
            if header_info:
                print(f"  #{i+1}: {header_info['mt_name']} -> {header_info['gid_name']} (0x{header_info['opcode']:02x})")
                print(f"       Length: {header_info['total_len']} bytes")
                print(f"       Raw: {hex_string}")
            else:
                print(f"  #{i+1}: {hex_string}")
        print()
    
    print("Packet Analysis Summary:")
    print("-" * 23)
    
    # Analyze packet statistics
    mt_counts = {}
    gid_counts = {}
    
    for packet_type, hex_string in test_packets:
        header_info = parse_uci_header(hex_string)
        if header_info:
            mt_name = header_info['mt_name']
            gid_name = header_info['gid_name']
            
            mt_counts[mt_name] = mt_counts.get(mt_name, 0) + 1
            gid_counts[gid_name] = gid_counts.get(gid_name, 0) + 1
    
    print("\nMessage Type Distribution:")
    for mt, count in mt_counts.items():
        print(f"  {mt}: {count} packets")
    
    print("\nGroup ID Distribution:")
    for gid, count in gid_counts.items():
        print(f"  {gid}: {count} packets")
    
    print("\nThese test packets from the official UCI PDL specification provide")
    print("realistic examples of UCI packet formats used in actual UWB implementations.")

if __name__ == "__main__":
    main()