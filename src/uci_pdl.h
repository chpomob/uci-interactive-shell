#ifndef UCI_PDL_H
#define UCI_PDL_H

// Message Type (MT) definitions
#define COMMAND         0x00
#define RESPONSE        0x01
#define NOTIFICATION    0x02

// Packet Boundary Flag (PBF) definitions
#define COMPLETE        0x00
#define START           0x01
#define CONT            0x02
#define END             0x03

// Group ID (GID) definitions
#define CORE            0x00
#define SESSION_CONFIG  0x01
#define SESSION_CONTROL 0x02
#define MAC_CONFIG      0x03
#define TEST_CONFIG     0x04

// OID (Opcode ID) definitions for CORE group
#define CORE_DEVICE_INFO        0x01
#define CORE_GET_CAPS_INFO      0x02
#define CORE_DEVICE_RESET       0x03
#define CORE_SET_CONFIG         0x04
#define CORE_GET_CONFIG         0x05
#define CORE_DEVICE_STATUS_NTF  0x06

// OID (Opcode ID) definitions for Session Config group
#define SESSION_INIT            0x01
#define SESSION_DEINIT          0x02

// OID (Opcode ID) definitions for Session Control group
#define SESSION_START           0x01
#define SESSION_STOP            0x02

// Status definitions
#define UCI_STATUS_OK           0x00
#define UCI_STATUS_REJECT       0x01
#define UCI_STATUS_INVALID_CF   0x02
#define UCI_STATUS_FAILED       0x03

// Device state definitions
#define DEVICE_STATE_INIT       0x00
#define DEVICE_STATE_READY      0x01
#define DEVICE_STATE_ACTIVE     0x02
#define DEVICE_STATE_ERROR      0x03

// Reset type definitions
#define UWBS_RESET              0x00

// Capability TLV types
typedef enum {
    SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE = 0x01,
} CapTlvType;

// Device configuration IDs
typedef enum {
    DEVICE_STATE = 0x01,
} DeviceConfigId;

// Session types
typedef enum {
    FIRA_RANGING_SESSION = 0x00,
} SessionType;

#endif // UCI_PDL_H