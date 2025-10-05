#!/usr/bin/env python3
"""
Script to analyze real UCI logs with our existing codebase
This script processes the real UCI logs found in the logs directory
and uses our UCI analysis functions to parse the actual packet data.
"""

import re
import sys
import os
from pathlib import Path

# Add the project source directory to Python path to import our modules
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

def extract_hex_packets_from_log(log_file_path):
    """
    Extract hex-encoded UCI packets from log file
    Looks for patterns like: DEBUG:uci.core:recv: 6b0300212a0000000800000006030100000001000001000000020100000401000005000000
    """
    with open(log_file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Regular expressions to find hex packet data in the logs
    patterns = [
        r'DEBUG:uci\.core:recv:\s+([0-9a-fA-F]+)',  # Hex data after DEBUG:uci.core:recv:
        r'INFO:uci\.core:notif:\s+\d+,\s+\d+:\s+([0-9a-fA-F]+)',  # Hex data after INFO:uci.core:notif:
    ]
    
    packets = []
    for pattern in patterns:
        matches = re.findall(pattern, content)
        for match in matches:
            if len(match) >= 8:  # At least header size
                packets.append(match.strip())
    
    return packets

def parse_uci_packet_header(packet_hex):
    """
    Parse UCI packet header from hex string
    Based on the Android UCI specification: [GID:4][PBF:1][MT:3] where GID is in least significant bits
    """
    try:
        packet_bytes = bytes.fromhex(packet_hex)
        
        if len(packet_bytes) < 4:
            return None, "Packet too short"
        
        # Parse header fields (based on UCI specification)
        first_byte = packet_bytes[0]
        second_byte = packet_bytes[1]
        payload_len = packet_bytes[3]
        
        # Extract fields according to the correct bit layout:
        # [MT:3][PBF:1][GID:4] - MT in most significant bits, GID in least significant bits
        # Actually, let's re-read the specification:
        # According to the header definition: [GID:4][PBF:1][MT:3] where GID occupies the least significant bits
        # So: [MT:3][PBF:1][GID:4] in memory layout
        # first_byte = (GID & 0x0F) | ((PBF & 0x01) << 4) | ((MT & 0x07) << 5)
        # So extracting: GID = first_byte & 0x0F, PBF = (first_byte >> 4) & 0x01, MT = (first_byte >> 5) & 0x07
        gid = first_byte & 0x0F          # Lower 4 bits (0-3)
        pbf = (first_byte >> 4) & 0x01   # Bit 4
        mt = (first_byte >> 5) & 0x07    # Upper 3 bits (5-7)
        opcode = second_byte & 0x3F      # Lower 6 bits of second byte
        
        # Determine message type
        mt_names = {0: 'DATA', 1: 'COMMAND', 2: 'RESPONSE', 3: 'NOTIFICATION'}
        mt_name = mt_names.get(mt, f'UNKNOWN({mt})')
        
        # Determine group ID - use the official GID constants from UCI spec
        gid_names = {
            0x00: 'CORE',
            0x01: 'SESSION_CONFIG', 
            0x02: 'SESSION_CONTROL',
            0x03: 'RANGING_DATA',
            0x0c: 'VENDOR_ANDROID',
            0x0d: 'TEST',
            # Vendor-specific ranges
            0x09: 'VENDOR_SPECIFIC_1',
            0x0a: 'VENDOR_SPECIFIC_2', 
            0x0b: 'VENDOR_SPECIFIC_3',
            0x0e: 'VENDOR_SPECIFIC_4',
            0x0f: 'VENDOR_SPECIFIC_5'
        }
        gid_name = gid_names.get(gid, f'UNKNOWN(0x{gid:02x})')
        
        # Determine opcode based on GID
        opcode_names = {}
        if gid == 0x00:  # CORE
            opcode_names = {
                0x00: 'CORE_DEVICE_RESET_CMD',
                0x01: 'CORE_DEVICE_STATUS_NTF',
                0x02: 'CORE_DEVICE_INFO_CMD',
                0x03: 'CORE_GET_CAPS_INFO_CMD',
                0x04: 'CORE_SET_CONFIG_CMD',
                0x05: 'CORE_GET_CONFIG_CMD',
                0x08: 'CORE_QUERY_UWBS_TIMESTAMP_CMD'
            }
        elif gid == 0x01:  # SESSION_CONFIG
            opcode_names = {
                0x00: 'SESSION_INIT_CMD',
                0x01: 'SESSION_DEINIT_CMD',
                0x03: 'SESSION_SET_APP_CONFIG_CMD',
                0x04: 'SESSION_GET_APP_CONFIG_CMD',
                0x06: 'SESSION_GET_STATE_CMD',
                0x07: 'SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_CMD',  # From our codebase
                0x09: 'SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG_CMD',        # From our codebase
                0x0a: 'SESSION_SET_INITIATOR_DT_ANCHOR_RR_RDM_LIST_CMD', # From our codebase
                0x0b: 'SESSION_QUERY_DATA_SIZE_IN_RANGING_CMD',         # From our codebase
                0x0c: 'SESSION_SET_HYBRID_CONTROLLER_CONFIG_CMD',      # From our codebase
                0x0d: 'SESSION_SET_HYBRID_CONTROLEE_CONFIG_CMD',       # From our codebase
                0x0e: 'SESSION_DATA_TRANSFER_PHASE_CONFIG_CMD'         # From our codebase
            }
        elif gid == 0x02:  # SESSION_CONTROL
            opcode_names = {
                0x00: 'SESSION_START_CMD',
                0x01: 'SESSION_STOP_CMD',
                0x02: 'SESSION_DATA_CREDIT_NTF',  # Notification
                0x03: 'SESSION_GET_RANGING_COUNT_CMD',
                0x04: 'SESSION_INFO_NTF'  # Notification
            }
        elif gid == 0x03:  # RANGING_DATA
            opcode_names = {
                0x00: 'RANGE_DATA_NTF_OPCODE'  # This appears in the logs
            }
        elif gid == 0x0c:  # VENDOR_ANDROID
            opcode_names = {
                0x00: 'ANDROID_GET_POWER_STATS_CMD',
                0x01: 'ANDROID_SET_COUNTRY_CODE_CMD',
                0x02: 'ANDROID_FIRA_RANGE_DIAGNOSTICS_NTF'
            }
        elif gid == 0x0d:  # TEST
            opcode_names = {
                0x00: 'TEST_RF_SET_CONFIG_CMD',
                0x02: 'TEST_RF_PERIODIC_TX_CMD',
                0x07: 'TEST_RF_STOP_CMD'
            }
        
        opcode_name = opcode_names.get(opcode, f'UNKNOWN(0x{opcode:02x})')
        
        return {
            'raw_hex': packet_hex,
            'bytes': packet_bytes,
            'message_type': mt,
            'message_type_name': mt_name,
            'group_id': gid,
            'group_name': gid_name,
            'opcode': opcode,
            'opcode_name': opcode_name,
            'pbf': pbf,
            'payload_length': payload_len,
            'actual_payload_length': len(packet_bytes) - 4
        }, None
        
    except ValueError as e:
        return None, f"Invalid hex string: {e}"
    except Exception as e:
        return None, f"Error parsing packet: {e}"

def analyze_log_file(log_file_path):
    """
    Analyze a single log file, extracting and parsing UCI packets
    """
    print(f"\nAnalyzing log file: {log_file_path}")
    print("=" * 60)
    
    packets = extract_hex_packets_from_log(log_file_path)
    
    if not packets:
        print("No hex packets found in the log file.")
        return
    
    print(f"Found {len(packets)} hex-encoded packets in the log file.\n")
    
    for i, packet_hex in enumerate(packets):
        print(f"Packet #{i+1}:")
        print(f"  Raw hex: {packet_hex}")
        
        header_info, error = parse_uci_packet_header(packet_hex)
        
        if error:
            print(f"  Error: {error}")
        else:
            print(f"  Type: {header_info['message_type_name']} ({header_info['message_type']})")
            print(f"  Group: {header_info['group_name']} (0x{header_info['group_id']:02x})")
            print(f"  Opcode: {header_info['opcode_name']} (0x{header_info['opcode']:02x})")
            print(f"  PBF: {header_info['pbf']}")
            print(f"  Declared payload length: {header_info['payload_length']}")
            print(f"  Actual payload length: {header_info['actual_payload_length']}")
            
            # Show decoded packet info if it's a known type
            # Check for RANGING_DATA notifications (this could be in vendor-specific groups based on the logs)
            if header_info['group_id'] == 0x03 or (header_info['group_id'] >= 0x09 and header_info['group_id'] <= 0x0b):
                # Check if it looks like a range data notification by payload content
                if len(header_info['bytes']) >= 8:
                    payload = header_info['bytes'][4:]  # Skip header
                    
                    # For range data notifications, the format is typically:
                    # Session ID (4), Sequence Number (4), Status (1), MAC Address (2), Distance (2), etc.
                    if len(payload) >= 13:
                        try:
                            session_id = int.from_bytes(payload[0:4], byteorder='little')
                            seq_num = int.from_bytes(payload[4:8], byteorder='little')
                            control = int.from_bytes(payload[8:12], byteorder='little')  # Control field
                            status = (control & 0xFF)  # Status from control field
                            
                            print(f"  -> This appears to be a RANGING DATA packet (Range Data NTF or similar)")
                            print(f"    Session ID: 0x{session_id:08x}")
                            print(f"    Sequence Number: {seq_num}")
                            print(f"    Control: 0x{control:08x}")
                            print(f"    Status: 0x{status:02x}")
                            
                            if len(payload) >= 15:
                                mac_addr = int.from_bytes(payload[12:14], byteorder='little')
                                dist_cm = int.from_bytes(payload[14:16], byteorder='little')
                                print(f"    MAC Address: 0x{mac_addr:04x}")
                                print(f"    Distance: {dist_cm} cm")
                        except Exception as e:
                            print(f"    Could not decode range data payload: {e}")
            
            # Also check for what looks like session control packets
            elif header_info['group_id'] == 0x02 and header_info['opcode'] == 0x00:
                print(f"  -> This appears to be a SESSION_START command/response")
                payload = header_info['bytes'][4:] if len(header_info['bytes']) > 4 else b''
                if len(payload) > 0:
                    print(f"    Status in payload: 0x{payload[0]:02x}")
            
        print()

def main():
    """
    Main function to analyze all log files in the logs directory
    """
    project_root = Path(__file__).parent
    logs_dir = project_root / "logs"
    
    if not logs_dir.exists():
        print(f"Logs directory does not exist: {logs_dir}")
        print("Creating logs directory and trying to find any log files...")
        os.makedirs(logs_dir, exist_ok=True)
        return
    
    # Find all log files in the logs directory
    log_files = list(logs_dir.glob("*.log"))
    
    if not log_files:
        print(f"No log files found in {logs_dir}")
        
        # Search for logs recursively in the project
        all_log_files = list(project_root.rglob("*.log"))
        if all_log_files:
            print(f"Found {len(all_log_files)} log files in the project:")
            for log_file in all_log_files:
                print(f"  - {log_file}")
            log_files = all_log_files
        else:
            print("No log files found in the entire project.")
            return
    
    print(f"Found {len(log_files)} log file(s) to analyze:")
    for log_file in log_files:
        print(f"  - {log_file.name}")
    
    # Analyze each log file
    for log_file in log_files:
        analyze_log_file(log_file)

if __name__ == "__main__":
    main()