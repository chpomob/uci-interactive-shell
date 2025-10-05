#!/usr/bin/env python3
"""
UCI Packet Generator based on Android UWB specification
This script demonstrates how to create proper UCI packets following the Android UWB implementation
"""

import struct

# UCI Protocol Constants based on Android UWB implementation
MESSAGE_TYPE = {
    'DATA': 0,
    'COMMAND': 1,
    'RESPONSE': 2,
    'NOTIFICATION': 3
}

GROUP_ID = {
    'CORE': 0x00,
    'SESSION_CONFIG': 0x01,
    'SESSION_CONTROL': 0x02,
    'DATA_CONTROL': 0x03,
    'TEST': 0x0d,
    'VENDOR_ANDROID': 0x0c
}

CORE_OPCODE = {
    'CORE_DEVICE_RESET': 0x00,
    'CORE_DEVICE_STATUS_NTF': 0x01,
    'CORE_DEVICE_INFO': 0x02,
    'CORE_GET_CAPS_INFO': 0x03,
    'CORE_SET_CONFIG': 0x04,
    'CORE_GET_CONFIG': 0x05,
    'CORE_QUERY_UWBS_TIMESTAMP': 0x08
}

SESSION_CONFIG_OPCODE = {
    'SESSION_INIT': 0x00,
    'SESSION_DEINIT': 0x01,
    'SESSION_SET_APP_CONFIG': 0x03,
    'SESSION_GET_APP_CONFIG': 0x04,
    'SESSION_GET_STATE': 0x06
}

SESSION_CONTROL_OPCODE = {
    'SESSION_START': 0x00,
    'SESSION_STOP': 0x01,
    'SESSION_GET_RANGING_COUNT': 0x03
}

STATUS_CODE = {
    'UCI_STATUS_OK': 0x00,
    'UCI_STATUS_REJECTED': 0x01,
    'UCI_STATUS_FAILED': 0x02,
    'UCI_STATUS_INVALID_PARAM': 0x04,
    'UCI_STATUS_UNKNOWN_GID': 0x07,
    'UCI_STATUS_UNKNOWN_OID': 0x08,
    'UCI_STATUS_SESSION_NOT_EXIST': 0x11,
    'UCI_STATUS_SESSION_ACTIVE': 0x13
}

def create_uci_header(message_type, group_id, opcode, payload_length=0):
    """
    Create a UCI packet header following the Android specification
    Format based on UciPacketHal and UciControlPacketHal:
    First byte: [4 bits GID][1 bit PBF][3 bits MT] 
    Second byte: [6 bits opcode][2 reserved bits]
    Third byte: reserved
    Fourth byte: payload length
    """
    # First byte: [4 bits GID][1 bit PBF][3 bits MT]
    # In binary format: [GID:4][PBF:1][MT:3]
    # So we need: (GID & 0x0F) | ((PBF & 0x01) << 4) | ((MT & 0x07) << 5)
    first_byte = (group_id & 0x0F) | ((0 & 0x01) << 4) | ((message_type & 0x07) << 5)

    # Second byte: [6 bits opcode][2 reserved bits] with opcode in LSBs
    second_byte = (opcode & 0x3F)
    
    # For control packets: [first_byte][second_byte][reserved][payload_length]
    header = struct.pack('<BBBB', first_byte, second_byte, 0, payload_length)
    return header

def create_device_info_cmd():
    """Create a Get Device Info command packet"""
    header = create_uci_header(MESSAGE_TYPE['COMMAND'], GROUP_ID['CORE'], CORE_OPCODE['CORE_DEVICE_INFO'], 0)
    return header

def create_session_init_cmd(session_id=0x12345678, session_type=0x00):
    """Create a Session Init command packet"""
    payload = struct.pack('<IB', session_id, session_type)
    header = create_uci_header(MESSAGE_TYPE['COMMAND'], GROUP_ID['SESSION_CONFIG'], 
                              SESSION_CONFIG_OPCODE['SESSION_INIT'], len(payload))
    return header + payload

def create_device_reset_cmd(reset_config=0x00):
    """Create a Device Reset command packet"""
    payload = struct.pack('<B', reset_config)
    header = create_uci_header(MESSAGE_TYPE['COMMAND'], GROUP_ID['CORE'], 
                              CORE_OPCODE['CORE_DEVICE_RESET'], len(payload))
    return header + payload

def create_set_config_cmd(config_id, config_value):
    """Create a Set Config command with TLV format"""
    # TLV format: [Config ID][Length][Value...]
    payload = struct.pack('<BB', config_id, len(config_value)) + bytes(config_value)
    header = create_uci_header(MESSAGE_TYPE['COMMAND'], GROUP_ID['CORE'], 
                              CORE_OPCODE['CORE_SET_CONFIG'], len(payload))
    return header + payload

def print_packet(name, packet_bytes):
    """Print packet in a readable format"""
    print(f"{name}:")
    print(f"  Raw: {' '.join(f'{b:02x}' for b in packet_bytes)}")
    print(f"  Length: {len(packet_bytes)} bytes")
    if len(packet_bytes) >= 4:
        # Unpack header bytes
        first_byte, second_byte, reserved, payload_len = struct.unpack('<BBBB', packet_bytes[:4])
        
        # Extract fields according to UciControlPacketHal format
        gid = first_byte & 0x0F          # Lower 4 bits
        pbf = (first_byte >> 4) & 0x01   # Bit 4
        mt = (first_byte >> 5) & 0x07    # Upper 3 bits
        opcode = second_byte & 0x3F         # Lower 6 bits of second byte
        reserved_bits = (second_byte >> 6) & 0x03

        print(f"  First byte (0x{first_byte:02x}): GID={gid}, PBF={pbf}, MT={mt}")
        print(f"  Second byte (0x{second_byte:02x}): opcode={opcode}, reserved=0x{reserved_bits:02x}")
        print(f"  Third byte (0x{reserved:02x}): reserved")
        print(f"  Fourth byte (0x{payload_len:02x}): payload length")
        print(f"  MT: {mt} ({list(MESSAGE_TYPE.keys())[list(MESSAGE_TYPE.values()).index(mt)]})")
        print(f"  PBF: {pbf}")
        print(f"  GID: {gid} ({list(GROUP_ID.keys())[list(GROUP_ID.values()).index(gid)]})")
        print(f"  Opcode: {opcode} ({get_opcode_name(gid, opcode)})")
        print(f"  Payload Length: {payload_len}")
        if len(packet_bytes) > 4:
            payload = packet_bytes[4:]
            print(f"  Payload: {' '.join(f'{b:02x}' for b in payload)}")
    print()

def get_opcode_name(gid, opcode):
    """Get opcode name based on group ID"""
    if gid == GROUP_ID['CORE']:
        for name, value in CORE_OPCODE.items():
            if value == opcode:
                return name
    elif gid == GROUP_ID['SESSION_CONFIG']:
        for name, value in SESSION_CONFIG_OPCODE.items():
            if value == opcode:
                return name
    elif gid == GROUP_ID['SESSION_CONTROL']:
        for name, value in SESSION_CONTROL_OPCODE.items():
            if value == opcode:
                return name
    return f"Unknown (0x{opcode:02x})"

def main():
    print("UCI Packet Generator based on Android UWB Specification")
    print("=" * 55)
    
    # Example 1: Get Device Info Command
    device_info_cmd = create_device_info_cmd()
    print_packet("Get Device Info Command", device_info_cmd)
    
    # Example 2: Session Init Command
    session_init_cmd = create_session_init_cmd(0x12345678, 0x00)
    print_packet("Session Init Command", session_init_cmd)
    
    # Example 3: Device Reset Command
    device_reset_cmd = create_device_reset_cmd(0x00)
    print_packet("Device Reset Command", device_reset_cmd)
    
    # Example 4: Set Config Command (Device State = Active)
    set_config_cmd = create_set_config_cmd(0x00, [0x02])  # DEVICE_STATE = DEVICE_STATE_ACTIVE
    print_packet("Set Config Command (Device State)", set_config_cmd)

    print("For complete specification, see:")
    print("- uci_analysis/uwb/src/rust/uwb_uci_packets/uci_packets.pdl")
    print("- uci_analysis/UCI_PROTOCOL_ANALYSIS.md")

if __name__ == "__main__":
    main()
