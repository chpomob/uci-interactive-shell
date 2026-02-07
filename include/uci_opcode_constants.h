#ifndef UCI_OPCODE_CONSTANTS_H
#define UCI_OPCODE_CONSTANTS_H

/* Qorvo vendor-specific opcodes (GID: QORVO_EXT2 / 0x0B). */
#define QORVO_TEST_DEBUG                0x00
#define QORVO_TEST_TX_CW                0x01
#define QORVO_TEST_PLLRF                0x02
#define QORVO_FIRA_RANGE_DIAGNOSTICS    0x03
#define QORVO_SESSION_GET               0x07
#define QORVO_FIRA_SET_ANT_FLEX_CONFIG  0x08
#define QORVO_FIRA_GET_ANT_FLEX_CONFIG  0x09
#define QORVO_CCC_SET_ANT_FLEX_CONFIG   0x0A
#define QORVO_CCC_GET_ANT_FLEX_CONFIG   0x0B
#define QORVO_CORE_PSDU_DUMP            0x22
#define QORVO_CORE_GET_MEM_STATS        0x23
#define QORVO_CORE_GET_POWER_STATS      0x24
#define QORVO_CORE_GET_CPU_STATS        0x25
#define QORVO_CORE_RESET_CPU_STATS      0x26
#define QORVO_CORE_GET_DEVICE_STATS     0x27
#define QORVO_CORE_ERASE_CERTS          0x30
#define QORVO_CORE_DEVICE_BOOT          0x31
#define QORVO_CORE_TOGGLE_GPIO_TIMESYNC 0x35
#define QORVO_CORE_QUERY_GPIO_TIMESTAMP 0x36

/* Android vendor-specific opcodes (GID: ANDROID / 0x0C). */
#define ANDROID_GET_POWER_STATS         0x00
#define ANDROID_SET_COUNTRY_CODE        0x01
#define ANDROID_FIRA_RANGE_DIAGNOSTICS  0x02
#define ANDROID_RADAR_SET_APP_CONFIG    0x11
#define ANDROID_RADAR_GET_APP_CONFIG    0x12

/* TEST group opcodes (GID: TEST / 0x0D). */
#define TEST_RF_SET_CONFIG              0x00
#define TEST_RF_PERIODIC_TX             0x02
#define TEST_RF_PER_RX                  0x03
#define TEST_RF_RX                      0x05
#define TEST_RF_STOP                    0x07

#endif /* UCI_OPCODE_CONSTANTS_H */
