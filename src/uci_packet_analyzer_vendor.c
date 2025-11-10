#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/uci.h"
#include "../include/uci_packet_analyzer_vendor.h"
#include "../include/uci_qorvo_utils.h"
#include "../include/uci_ui.h"

static const char* qorvo_device_boot_reason_to_string(uint8_t reason) {
    switch (reason) {
        case 0:
            return "unknown";
        case 1:
            return "fatal_reset";
        default:
            return "reserved";
    }
}

void uci_packet_analyzer_handle_qorvo_ext2(uint8_t mt,
                                           uint8_t opcode,
                                           unsigned char* payload_ptr,
                                           unsigned char payload_len_field) {
    if (mt == NOTIFICATION) {
        switch(opcode) {
            case 0x00:  // QORVO_TEST_DEBUG
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_TEST_DEBUG_NTF:%s Vendor debug notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_TEST_DEBUG_NTF: Vendor debug notification\n");
                }
                break;
            case 0x01:  // QORVO_TEST_TX_CW
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_TEST_TX_CW_NTF:%s Continuous wave transmission test notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_TEST_TX_CW_NTF: Continuous wave transmission test notification\n");
                }
                break;
            case 0x02:  // QORVO_TEST_PLLRF
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_TEST_PLLRF_NTF:%s PLL status test notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_TEST_PLLRF_NTF: PLL status test notification\n");
                }
                break;
            case 0x03:  // QORVO_FIRA_RANGE_DIAGNOSTICS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_FIRA_RANGE_DIAGNOSTICS_NTF:%s Diagnostics notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_FIRA_RANGE_DIAGNOSTICS_NTF: Diagnostics notification\n");
                }
                break;
            case 0x07:  // QORVO_SESSION_GET
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_SESSION_GET_NTF:%s Session retrieval notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_SESSION_GET_NTF: Session retrieval notification\n");
                }
                break;
            case QORVO_CORE_PSDU_DUMP:
            {
                uci_qorvo_psdu_report_t report;
                if (uci_qorvo_decode_psdu_report(payload_ptr, payload_len_field, &report) == 0) {
                    if (ui_color_enabled) {
                        printf("\n  %s%sQORVO_CORE_PSDU_DUMP_NTF:%s PSDU dump notification%s\n",
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("\n  QORVO_CORE_PSDU_DUMP_NTF: PSDU dump notification\n");
                    }
                    printf("    session_handle: 0x%08X\n", report.session_handle);
                    printf("    frames: %u\n", report.frame_count);
                    for (uint8_t i = 0; i < report.frame_count; i++) {
                        const uci_qorvo_psdu_frame_t* frame = &report.frames[i];
                        printf("      index %u length %u bytes:", frame->index, frame->length);
                        for (uint16_t b = 0; b < frame->length; b++) {
                            printf(" %02X", frame->data[b]);
                        }
                        printf("\n");
                    }
                } else {
                    printf("\n  QORVO_CORE_PSDU_DUMP_NTF: PSDU dump notification (unable to decode)\n");
                }
                break;
            }
            case 0x23:  // QORVO_CORE_GET_MEM_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_GET_MEM_STATS_NTF:%s Memory statistics notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_GET_MEM_STATS_NTF: Memory statistics notification\n");
                }
                break;
            case 0x24:  // QORVO_CORE_GET_POWER_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_GET_POWER_STATS_NTF:%s Power statistics notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_GET_POWER_STATS_NTF: Power statistics notification\n");
                }
                break;
            case 0x25:  // QORVO_CORE_GET_CPU_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_GET_CPU_STATS_NTF:%s CPU statistics notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_GET_CPU_STATS_NTF: CPU statistics notification\n");
                }
                break;
            case 0x26:  // QORVO_CORE_RESET_CPU_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_RESET_CPU_STATS_RSP:%s CPU statistics reset response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_RESET_CPU_STATS_RSP: CPU statistics reset response\n");
                }
                break;
            case 0x27:  // QORVO_CORE_GET_DEVICE_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_GET_DEVICE_STATS_NTF:%s Device statistics notification%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_GET_DEVICE_STATS_NTF: Device statistics notification\n");
                }
                break;
            case 0x31:  // QORVO_CORE_DEVICE_BOOT
            {
                uci_qorvo_device_boot_t boot;
                if (uci_qorvo_decode_device_boot(payload_ptr, payload_len_field, &boot) == 0) {
                    const char* reason = qorvo_device_boot_reason_to_string(boot.reason);
                    if (ui_color_enabled) {
                        printf("\n  %s%sQORVO_CORE_DEVICE_BOOT_NTF:%s Device boot notification%s\n",
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("\n  QORVO_CORE_DEVICE_BOOT_NTF: Device boot notification\n");
                    }
                    printf("    reason: %s (0x%02X)\n", reason, boot.reason);
                } else {
                    printf("\n  QORVO_CORE_DEVICE_BOOT_NTF: Device boot notification (unable to decode)\n");
                }
                break;
            }
            default:
                if (ui_color_enabled) {
                    printf("\n  %s%sNo specific decoder for QORVO_EXT2_NOTIFICATION opcode 0x%02X%s\n",
                           ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                } else {
                    printf("\n  No specific decoder for QORVO_EXT2_NOTIFICATION opcode 0x%02X\n", opcode);
                }
                break;
        }
        return;
    }

    if (mt == RESPONSE) {
        switch(opcode) {
            case 0x00:  // QORVO_TEST_DEBUG
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_TEST_DEBUG_RSP:%s Vendor debug response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_TEST_DEBUG_RSP: Vendor debug response\n");
                }
                break;
            case 0x01:  // QORVO_TEST_TX_CW
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_TEST_TX_CW_RSP:%s Continuous wave transmission test response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_TEST_TX_CW_RSP: Continuous wave transmission test response\n");
                }
                break;
            case 0x02:  // QORVO_TEST_PLLRF
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_TEST_PLLRF_RSP:%s PLL status test response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_TEST_PLLRF_RSP: PLL status test response\n");
                }
                break;
            case 0x03:  // QORVO_FIRA_RANGE_DIAGNOSTICS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_FIRA_RANGE_DIAGNOSTICS_RSP:%s Diagnostics response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_FIRA_RANGE_DIAGNOSTICS_RSP: Diagnostics response\n");
                }
                break;
            case 0x07:  // QORVO_SESSION_GET
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_SESSION_GET_RSP:%s Session retrieval response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_SESSION_GET_RSP: Session retrieval response\n");
                }
                break;
            case 0x08:  // QORVO_FIRA_SET_ANT_FLEX_CONFIG
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_FIRA_SET_ANT_FLEX_CONFIG_RSP:%s Antenna flexibility config response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_FIRA_SET_ANT_FLEX_CONFIG_RSP: Antenna flexibility config response\n");
                }
                break;
            case 0x09:  // QORVO_FIRA_GET_ANT_FLEX_CONFIG
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_FIRA_GET_ANT_FLEX_CONFIG_RSP:%s Antenna flexibility config retrieval response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_FIRA_GET_ANT_FLEX_CONFIG_RSP: Antenna flexibility config retrieval response\n");
                }
                break;
            case 0x0A:  // QORVO_CCC_SET_ANT_FLEX_CONFIG
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CCC_SET_ANT_FLEX_CONFIG_RSP:%s CCC antenna flexibility config response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CCC_SET_ANT_FLEX_CONFIG_RSP: CCC antenna flexibility config response\n");
                }
                break;
            case 0x0B:  // QORVO_CCC_GET_ANT_FLEX_CONFIG
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CCC_GET_ANT_FLEX_CONFIG_RSP:%s CCC antenna flexibility config retrieval response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CCC_GET_ANT_FLEX_CONFIG_RSP: CCC antenna flexibility config retrieval response\n");
                }
                break;
            case QORVO_CORE_PSDU_DUMP:
            {
                uci_qorvo_psdu_report_t report;
                if (uci_qorvo_decode_psdu_report(payload_ptr, payload_len_field, &report) == 0) {
                    if (ui_color_enabled) {
                        printf("\n  %s%sQORVO_CORE_PSDU_DUMP_RSP:%s PSDU dump response%s\n",
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("\n  QORVO_CORE_PSDU_DUMP_RSP: PSDU dump response\n");
                    }
                    printf("    session_handle: 0x%08X\n", report.session_handle);
                    printf("    frames: %u\n", report.frame_count);
                    for (uint8_t i = 0; i < report.frame_count; i++) {
                        const uci_qorvo_psdu_frame_t* frame = &report.frames[i];
                        printf("      index %u length %u bytes:", frame->index, frame->length);
                        for (uint16_t b = 0; b < frame->length; b++) {
                            printf(" %02X", frame->data[b]);
                        }
                        printf("\n");
                    }
                } else {
                    printf("\n  QORVO_CORE_PSDU_DUMP_RSP: PSDU dump response (unable to decode)\n");
                }
                break;
            }
            case 0x23:  // QORVO_CORE_GET_MEM_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_GET_MEM_STATS_RSP:%s Memory statistics response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_GET_MEM_STATS_RSP: Memory statistics response\n");
                }
                break;
            case 0x24:  // QORVO_CORE_GET_POWER_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_GET_POWER_STATS_RSP:%s Power statistics response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_GET_POWER_STATS_RSP: Power statistics response\n");
                }
                break;
            case 0x25:  // QORVO_CORE_GET_CPU_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_GET_CPU_STATS_RSP:%s CPU statistics response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_GET_CPU_STATS_RSP: CPU statistics response\n");
                }
                break;
            case 0x26:  // QORVO_CORE_RESET_CPU_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_RESET_CPU_STATS_RSP:%s CPU statistics reset response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_RESET_CPU_STATS_RSP: CPU statistics reset response\n");
                }
                break;
            case 0x27:  // QORVO_CORE_GET_DEVICE_STATS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_GET_DEVICE_STATS_RSP:%s Device statistics response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_GET_DEVICE_STATS_RSP: Device statistics response\n");
                }
                break;
            case 0x30:  // QORVO_CORE_ERASE_CERTS
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_ERASE_CERTS_RSP:%s Certificate erase response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_ERASE_CERTS_RSP: Certificate erase response\n");
                }
                break;
            case 0x31:  // QORVO_CORE_DEVICE_BOOT
            {
                uci_qorvo_device_boot_t boot;
                if (uci_qorvo_decode_device_boot(payload_ptr, payload_len_field, &boot) == 0) {
                    const char* reason = qorvo_device_boot_reason_to_string(boot.reason);
                    if (ui_color_enabled) {
                        printf("\n  %s%sQORVO_CORE_DEVICE_BOOT_RSP:%s Device boot response%s\n",
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("\n  QORVO_CORE_DEVICE_BOOT_RSP: Device boot response\n");
                    }
                    printf("    reason: %s (0x%02X)\n", reason, boot.reason);
                } else {
                    printf("\n  QORVO_CORE_DEVICE_BOOT_RSP: Device boot response (unable to decode)\n");
                }
                break;
            }
            case 0x35:  // QORVO_CORE_TOGGLE_GPIO_TIMESYNC
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_TOGGLE_GPIO_TIMESYNC_RSP:%s GPIO timesync toggle response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_TOGGLE_GPIO_TIMESYNC_RSP: GPIO timesync toggle response\n");
                }
                break;
            case 0x36:  // QORVO_CORE_QUERY_GPIO_TIMESTAMP
                if (ui_color_enabled) {
                    printf("\n  %s%sQORVO_CORE_QUERY_GPIO_TIMESTAMP_RSP:%s GPIO timestamp query response%s\n",
                           ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
                } else {
                    printf("\n  QORVO_CORE_QUERY_GPIO_TIMESTAMP_RSP: GPIO timestamp query response\n");
                }
                break;
            default:
                if (ui_color_enabled) {
                    printf("\n  %s%sNo specific decoder for QORVO_EXT2_RESPONSE opcode 0x%02X%s\n",
                           ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, opcode, ANSI_RESET);
                } else {
                    printf("\n  No specific decoder for QORVO_EXT2_RESPONSE opcode 0x%02X\n", opcode);
                }
                break;
        }
        return;
    }

    if (ui_color_enabled) {
        printf("\n  %s%sNo specific decoder for MT=%d, GID=11, OP=0x%02X%s\n",
               ANSI_COLOR_BRIGHT_BLACK, ANSI_BOLD, mt, opcode, ANSI_RESET);
    } else {
        printf("\n  No specific decoder for MT=%d, GID=11, OP=0x%02X\n", mt, opcode);
    }
}
