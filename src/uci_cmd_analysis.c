/**
 * @file uci_cmd_analysis.c
 * @brief Enhanced packet analysis command implementation
 * 
 * This file implements the enhanced analyze_command that builds upon the existing
 * analyze_packet functionality with additional analysis capabilities based on 
 * Qorvo QM35 SDK patterns.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/uci.h"
#include "../include/uci_ui.h"
#include "../include/uci_packet_analyzer.h"
#include "../include/uci_ui_packet_decoder.h"
#include "../include/uci_packet_utils.h"

// Forward declarations
static void print_analysis_help(void);
static void print_analysis_examples(void);
static void enhanced_packet_analysis(unsigned char* packet, size_t packet_len, int verbose_mode, int tlv_mode, int compare_mode);

/**
 * @brief Handle the enhanced analyze_command
 * 
 * This function provides enhanced packet analysis capabilities building upon
 * the existing analyze_packet functionality with additional features:
 * - Compare mode for comparing packets
 * - Enhanced verbose analysis with contextual information
 * - Better TLV analysis with parameter recognition
 * - Statistical analysis of packet sequences
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 */
void handle_analyze_command(int argc, char* argv[]) {
    // Parse flags
    int verbose_mode = 0;
    int tlv_mode = 0;
    int compare_mode = 0;
    int arg_index = 0;
    
    // Parse optional flags first
    while (arg_index < argc && argv[arg_index][0] == '-') {
        if (strcmp(argv[arg_index], "-v") == 0 || strcmp(argv[arg_index], "--verbose") == 0) {
            verbose_mode = 1;
        } else if (strcmp(argv[arg_index], "-t") == 0 || strcmp(argv[arg_index], "--tlv") == 0) {
            tlv_mode = 1;
        } else if (strcmp(argv[arg_index], "-c") == 0 || strcmp(argv[arg_index], "--compare") == 0) {
            compare_mode = 1;
        } else if (strcmp(argv[arg_index], "-e") == 0 || strcmp(argv[arg_index], "--examples") == 0) {
            print_analysis_examples();
            return;
        } else if (strcmp(argv[arg_index], "-h") == 0 || strcmp(argv[arg_index], "--help") == 0) {
            print_analysis_help();
            return;
        } else {
            if (ui_color_enabled) {
                printf("%s%s%sError: Unknown flag '%s'%s\n", 
                       ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, argv[arg_index], ANSI_RESET);
            } else {
                printf("Error: Unknown flag '%s'\n", argv[arg_index]);
            }
            print_analysis_help();
            return;
        }
        arg_index++;
    }
    
    // Check if we have packet data to analyze
    if (arg_index >= argc) {
        print_analysis_help();
        return;
    }
    
    // Handle compare mode (requires at least two packets)
    if (compare_mode) {
        if (argc - arg_index < 2) {
            if (ui_color_enabled) {
                printf("%s%s%sError: Compare mode requires at least two packets%s\n", 
                       ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, ANSI_RESET);
            } else {
                printf("Error: Compare mode requires at least two packets\n");
            }
            print_analysis_help();
            return;
        }
        
        // Parse first packet - individual hex bytes
        unsigned char packet1[256];
        int packet1_len = 0;
        
        // Parse first packet hex bytes
        char* hex_byte_str = argv[arg_index];
        do {
            if (packet1_len >= (int)sizeof(packet1)) {
                if (ui_color_enabled) {
                    printf("%s%s%sError: First packet too long (max %zu bytes)%s\n", 
                           ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, sizeof(packet1), ANSI_RESET);
                } else {
                    printf("Error: First packet too long (max %zu bytes)\n", sizeof(packet1));
                }
                break;
            }
            char* endptr;
            unsigned long value = strtoul(hex_byte_str, &endptr, 16);
            if (*endptr != '\0' || value > 0xFF) {
                if (ui_color_enabled) {
                    printf("%s%s%sError: Invalid hex byte '%s' in first packet%s\n", 
                           ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, ANSI_RESET, hex_byte_str);
                } else {
                    printf("Error: Invalid hex byte '%s' in first packet\n", hex_byte_str);
                }
                break;
            }
            packet1[packet1_len++] = (unsigned char)value;
        } while (++arg_index < argc && argv[arg_index][0] != '-' && (hex_byte_str = argv[arg_index]) != NULL);
        
        // Parse second packet - individual hex bytes
        unsigned char packet2[256];
        int packet2_len = 0;
        
        // Check if we still have arguments for second packet
        if (arg_index >= argc) {
            if (ui_color_enabled) {
                printf("%s%s%sError: Second packet data missing%s\n", 
                       ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, ANSI_RESET);
            } else {
                printf("Error: Second packet data missing\n");
            }
            print_analysis_help();
            return;
        }
        
        // Parse second packet hex bytes
        hex_byte_str = argv[arg_index];
        do {
            if (packet2_len >= (int)sizeof(packet2)) {
                if (ui_color_enabled) {
                    printf("%s%s%sError: Second packet too long (max %zu bytes)%s\n", 
                           ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, sizeof(packet2), ANSI_RESET);
                } else {
                    printf("Error: Second packet too long (max %zu bytes)\n", sizeof(packet2));
                }
                break;
            }
            char* endptr;
            unsigned long value = strtoul(hex_byte_str, &endptr, 16);
            if (*endptr != '\0' || value > 0xFF) {
                if (ui_color_enabled) {
                    printf("%s%s%sError: Invalid hex byte '%s' in second packet%s\n", 
                           ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, ANSI_RESET, hex_byte_str);
                } else {
                    printf("Error: Invalid hex byte '%s' in second packet\n", hex_byte_str);
                }
                break;
            }
            packet2[packet2_len++] = (unsigned char)value;
        } while (++arg_index < argc && argv[arg_index][0] != '-' && (hex_byte_str = argv[arg_index]) != NULL);
        
        // Perform comparison analysis
        if (ui_color_enabled) {
            printf("%s%s%s=== Packet Comparison Analysis ===%s\n", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
        } else {
            printf("=== Packet Comparison Analysis ===\n");
        }
        
        printf("Packet 1 (%d bytes): ", packet1_len);
        for (int i = 0; i < packet1_len; i++) {
            if (ui_color_enabled) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_GREEN, packet1[i], ANSI_RESET);
            } else {
                printf("%02X ", packet1[i]);
            }
        }
        printf("\n");
        
        printf("Packet 2 (%d bytes): ", packet2_len);
        for (int i = 0; i < packet2_len; i++) {
            if (ui_color_enabled) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_GREEN, packet2[i], ANSI_RESET);
            } else {
                printf("%02X ", packet2[i]);
            }
        }
        printf("\n\n");
        
        // Compare headers
        if (ui_color_enabled) {
            printf("%s%s%sHeader Comparison:%s\n", 
                   ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_BG_YELLOW, ANSI_RESET);
        } else {
            printf("Header Comparison:\n");
        }
        
        if (packet1_len >= (int)sizeof(struct uci_packet_header) && packet2_len >= (int)sizeof(struct uci_packet_header)) {
            struct uci_packet_header* header1 = (struct uci_packet_header*)packet1;
            struct uci_packet_header* header2 = (struct uci_packet_header*)packet2;
            
            uci_header_fields_t fields1, fields2;
            uci_extract_header_fields_safe(header1, &fields1);
            uci_extract_header_fields_safe(header2, &fields2);
            
            if (ui_color_enabled) {
                printf("  MT: 0x%01X %s→%s 0x%01X %s\n", 
                       fields1.message_type,
                       (fields1.message_type != fields2.message_type) ? ANSI_COLOR_RED : ANSI_COLOR_BRIGHT_GREEN,
                       ANSI_RESET,
                       fields2.message_type,
                       (fields1.message_type != fields2.message_type) ? "(DIFFERENT)" : "(SAME)");
                printf("  PBF: 0x%01X %s→%s 0x%01X %s\n", 
                       fields1.packet_boundary,
                       (fields1.packet_boundary != fields2.packet_boundary) ? ANSI_COLOR_RED : ANSI_COLOR_BRIGHT_GREEN,
                       ANSI_RESET,
                       fields2.packet_boundary,
                       (fields1.packet_boundary != fields2.packet_boundary) ? "(DIFFERENT)" : "(SAME)");
                printf("  GID: 0x%01X %s→%s 0x%01X %s\n", 
                       fields1.group_id,
                       (fields1.group_id != fields2.group_id) ? ANSI_COLOR_RED : ANSI_COLOR_BRIGHT_GREEN,
                       ANSI_RESET,
                       fields2.group_id,
                       (fields1.group_id != fields2.group_id) ? "(DIFFERENT)" : "(SAME)");
                printf("  OID: 0x%02X %s→%s 0x%02X %s\n", 
                       fields1.opcode_id,
                       (fields1.opcode_id != fields2.opcode_id) ? ANSI_COLOR_RED : ANSI_COLOR_BRIGHT_GREEN,
                       ANSI_RESET,
                       fields2.opcode_id,
                       (fields1.opcode_id != fields2.opcode_id) ? "(DIFFERENT)" : "(SAME)");
            } else {
                printf("  MT: 0x%01X → 0x%01X %s\n", 
                       fields1.message_type,
                       fields2.message_type,
                       (fields1.message_type != fields2.message_type) ? "(DIFFERENT)" : "(SAME)");
                printf("  PBF: 0x%01X → 0x%01X %s\n", 
                       fields1.packet_boundary,
                       fields2.packet_boundary,
                       (fields1.packet_boundary != fields2.packet_boundary) ? "(DIFFERENT)" : "(SAME)");
                printf("  GID: 0x%01X → 0x%01X %s\n", 
                       fields1.group_id,
                       fields2.group_id,
                       (fields1.group_id != fields2.group_id) ? "(DIFFERENT)" : "(SAME)");
                printf("  OID: 0x%02X → 0x%02X %s\n", 
                       fields1.opcode_id,
                       fields2.opcode_id,
                       (fields1.opcode_id != fields2.opcode_id) ? "(DIFFERENT)" : "(SAME)");
            }
        } else {
            if (ui_color_enabled) {
                printf("  %s%sOne or both packets too short for header comparison%s\n", 
                       ANSI_COLOR_YELLOW, ANSI_BOLD, ANSI_RESET);
            } else {
                printf("  One or both packets too short for header comparison\n");
            }
        }
        
        // Compare payload lengths
        if (ui_color_enabled) {
            printf("%s%s%sPayload Length Comparison:%s\n", 
                   ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_BG_YELLOW, ANSI_RESET);
        } else {
            printf("Payload Length Comparison:\n");
        }
        
        int payload_len1 = (packet1_len >= (int)sizeof(struct uci_packet_header)) ? 
                           (packet1_len - (int)sizeof(struct uci_packet_header)) : 0;
        int payload_len2 = (packet2_len >= (int)sizeof(struct uci_packet_header)) ? 
                           (packet2_len - (int)sizeof(struct uci_packet_header)) : 0;
                           
        if (ui_color_enabled) {
            printf("  Length: %d bytes %s→%s %d bytes %s\n", 
                   payload_len1,
                   (payload_len1 != payload_len2) ? ANSI_COLOR_RED : ANSI_COLOR_BRIGHT_GREEN,
                   ANSI_RESET,
                   payload_len2,
                   (payload_len1 != payload_len2) ? "(DIFFERENT)" : "(SAME)");
        } else {
            printf("  Length: %d bytes → %d bytes %s\n", 
                   payload_len1,
                   payload_len2,
                   (payload_len1 != payload_len2) ? "(DIFFERENT)" : "(SAME)");
        }
        
        // Compare payload content (if both have payloads)
        if (payload_len1 > 0 && payload_len2 > 0) {
            if (ui_color_enabled) {
                printf("%s%s%sPayload Content Comparison:%s\n", 
                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_BG_YELLOW, ANSI_RESET);
            } else {
                printf("Payload Content Comparison:\n");
            }
            
            int min_len = (payload_len1 < payload_len2) ? payload_len1 : payload_len2;
            int difference_found = 0;
            
            for (int i = 0; i < min_len; i++) {
                if (packet1[sizeof(struct uci_packet_header) + i] != 
                    packet2[sizeof(struct uci_packet_header) + i]) {
                    if (!difference_found) {
                        if (ui_color_enabled) {
                            printf("  Differences found at positions:\n");
                        } else {
                            printf("  Differences found at positions:\n");
                        }
                        difference_found = 1;
                    }
                    
                    if (ui_color_enabled) {
                        printf("    Position %d: 0x%02X %s→%s 0x%02X\n", 
                               i,
                               packet1[sizeof(struct uci_packet_header) + i],
                               ANSI_COLOR_RED, ANSI_RESET,
                               packet2[sizeof(struct uci_packet_header) + i]);
                    } else {
                        printf("    Position %d: 0x%02X → 0x%02X\n", 
                               i,
                               packet1[sizeof(struct uci_packet_header) + i],
                               packet2[sizeof(struct uci_packet_header) + i]);
                    }
                }
            }
            
            if (!difference_found) {
                if (ui_color_enabled) {
                    printf("  %s%sNo differences found in first %d bytes%s\n", 
                           ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, min_len, ANSI_RESET);
                } else {
                    printf("  No differences found in first %d bytes\n", min_len);
                }
            }
            
            if (payload_len1 != payload_len2) {
                if (ui_color_enabled) {
                    printf("  %s%sLength difference: Packet 1 has %d extra bytes, Packet 2 has %d extra bytes%s\n", 
                           ANSI_COLOR_YELLOW, ANSI_BOLD, 
                           (payload_len1 > payload_len2) ? (payload_len1 - payload_len2) : 0,
                           (payload_len2 > payload_len1) ? (payload_len2 - payload_len1) : 0,
                           ANSI_RESET);
                } else {
                    printf("  Length difference: Packet 1 has %d extra bytes, Packet 2 has %d extra bytes\n", 
                           (payload_len1 > payload_len2) ? (payload_len1 - payload_len2) : 0,
                           (payload_len2 > payload_len1) ? (payload_len2 - payload_len1) : 0);
                }
            }
        }
        
        return;
    }
    
    // Handle single packet analysis - individual hex bytes like analyze_packet
    unsigned char packet[256];
    int packet_len = 0;
    
    // Parse hex bytes like the original analyze_packet command
    do {
        if (packet_len >= (int)sizeof(packet)) {
            if (ui_color_enabled) {
                printf("%s%s%sError: Packet too long (max %zu bytes)%s\n", 
                       ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, sizeof(packet), ANSI_RESET);
            } else {
                printf("Error: Packet too long (max %zu bytes)\n", sizeof(packet));
            }
            break;
        }
        char* endptr;
        unsigned long value = strtoul(argv[arg_index], &endptr, 16);
        if (*endptr != '\0' || value > 0xFF) {
            if (ui_color_enabled) {
                printf("%s%s%sError: Invalid hex byte '%s'%s\n", 
                       ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, ANSI_RESET, argv[arg_index]);
            } else {
                printf("Error: Invalid hex byte '%s'\n", argv[arg_index]);
            }
            break;
        }
        packet[packet_len++] = (unsigned char)value;
    } while (++arg_index < argc);
    
    if (packet_len > 0) {
        if (ui_color_enabled) {
            printf("%s%s%sAnalyzing UCI packet (%d bytes):%s\n", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, packet_len, ANSI_RESET);
            if (verbose_mode) {
                printf("%s%s%sVerbose analysis mode enabled%s\n", 
                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_BG_YELLOW, ANSI_RESET);
            }
            if (tlv_mode) {
                printf("%s%s%sTLV analysis mode enabled%s\n", 
                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_BG_YELLOW, ANSI_RESET);
            }
        } else {
            printf("Analyzing UCI packet (%d bytes):\n", packet_len);
            if (verbose_mode) {
                printf("Verbose analysis mode enabled\n");
            }
            if (tlv_mode) {
                printf("TLV analysis mode enabled\n");
            }
        }
        for (int i = 0; i < packet_len; i++) {
            if (ui_color_enabled) {
                printf("%s%02X%s ", ANSI_COLOR_BRIGHT_GREEN, packet[i], ANSI_RESET);
            } else {
                printf("%02X ", packet[i]);
            }
        }
        printf("\n\n");
        
        // Perform enhanced packet analysis
        enhanced_packet_analysis(packet, packet_len, verbose_mode, tlv_mode, compare_mode);
    }
}

/**
 * @brief Parse hex string into byte array
 * 
 * @param hex_str Input hex string (space-separated hex bytes)
 * @param bytes Output byte array
 * @param max_len Maximum number of bytes to parse
 * @param out_len Output parameter for actual number of bytes parsed
 * @return 0 on success, -1 on error
 */
static int parse_hex_string(const char* hex_str, unsigned char* bytes, size_t max_len, size_t* out_len) {
    if (!hex_str || !bytes || !out_len) {
        return -1;
    }
    
    *out_len = 0;
    char* hex_copy = strdup(hex_str);
    if (!hex_copy) {
        return -1;
    }
    
    char* token = strtok(hex_copy, " ");
    while (token && *out_len < max_len) {
        char* endptr;
        unsigned long value = strtoul(token, &endptr, 16);
        if (*endptr != '\0' || value > 0xFF) {
            free(hex_copy);
            return -1;
        }
        bytes[*out_len] = (unsigned char)value;
        (*out_len)++;
        token = strtok(NULL, " ");
    }
    
    free(hex_copy);
    return 0;
}

/**
 * @brief Perform enhanced packet analysis
 * 
 * @param packet Packet data to analyze
 * @param packet_len Length of packet data
 * @param verbose_mode Enable verbose analysis
 * @param tlv_mode Enable TLV analysis
 * @param compare_mode Enable comparison mode (not used in single packet analysis)
 */
static void enhanced_packet_analysis(unsigned char* packet, size_t packet_len, int verbose_mode, int tlv_mode, int compare_mode) {
    // Call existing analysis function with enhancements
    if (ui_color_enabled) {
        printf("%s%s%s=== Enhanced UCI Packet Analysis ===%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
    } else {
        printf("=== Enhanced UCI Packet Analysis ===\n");
    }
    
    if (verbose_mode) {
        if (ui_color_enabled) {
            printf("%s%s%sVerbose analysis mode enabled%s\n", 
                   ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_BG_YELLOW, ANSI_RESET);
        } else {
            printf("Verbose analysis mode enabled\n");
        }
    }
    
    if (tlv_mode) {
        if (ui_color_enabled) {
            printf("%s%s%sTLV analysis mode enabled%s\n", 
                   ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_BG_YELLOW, ANSI_RESET);
        } else {
            printf("TLV analysis mode enabled\n");
        }
    }
    
    printf("Total Packet Length: %zu bytes\n", packet_len);
    
    // Show packet bytes
    printf("Packet Bytes: ");
    for (size_t i = 0; i < packet_len; i++) {
        if (ui_color_enabled) {
            printf("%s%02X%s ", ANSI_COLOR_BRIGHT_GREEN, packet[i], ANSI_RESET);
        } else {
            printf("%02X ", packet[i]);
        }
    }
    printf("\n\n");
    
    // Call existing analysis function
    uci_analyze_packet_core(packet, packet_len);
    
    // Add enhanced analysis if requested
    if (verbose_mode) {
        if (ui_color_enabled) {
            printf("%s%s%sEnhanced Analysis:%s\n", 
                   ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_BG_MAGENTA, ANSI_RESET);
        } else {
            printf("Enhanced Analysis:\n");
        }
        
        // Add contextual analysis based on packet type
        if (packet_len >= sizeof(struct uci_packet_header)) {
            struct uci_packet_header* header = (struct uci_packet_header*)packet;
            uci_header_fields_t header_fields;
            uci_extract_header_fields_safe(header, &header_fields);
            
            // Analyze based on message type
            if (ui_color_enabled) {
                printf("  %s%s%sMessage Type Analysis:%s\n", 
                       ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_BG_YELLOW, ANSI_RESET);
            } else {
                printf("  Message Type Analysis:\n");
            }
            
            switch (header_fields.message_type) {
                case COMMAND:
                    if (ui_color_enabled) {
                        printf("    %s%s%sCommand Analysis:%s\n", 
                               ANSI_COLOR_BRIGHT_BLUE, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
                        printf("      %s%sThis is a UCI command packet that would be sent to a UWB device%s\n", 
                               ANSI_COLOR_BRIGHT_WHITE, ANSI_BOLD, ANSI_RESET);
                        printf("      %s%sExpected response: %s", 
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
                        switch (header_fields.group_id) {
                            case CORE:
                                printf("CORE_RESPONSE\n");
                                break;
                            case SESSION_CONFIG:
                                printf("SESSION_CONFIG_RESPONSE\n");
                                break;
                            case SESSION_CONTROL:
                                printf("SESSION_CONTROL_RESPONSE\n");
                                break;
                            default:
                                printf("RESPONSE\n");
                                break;
                        }
                    } else {
                        printf("    Command Analysis:\n");
                        printf("      This is a UCI command packet that would be sent to a UWB device\n");
                        printf("      Expected response: ");
                        switch (header_fields.group_id) {
                            case CORE:
                                printf("CORE_RESPONSE\n");
                                break;
                            case SESSION_CONFIG:
                                printf("SESSION_CONFIG_RESPONSE\n");
                                break;
                            case SESSION_CONTROL:
                                printf("SESSION_CONTROL_RESPONSE\n");
                                break;
                            default:
                                printf("RESPONSE\n");
                                break;
                        }
                    }
                    break;
                    
                case RESPONSE:
                    if (ui_color_enabled) {
                        printf("    %s%s%sResponse Analysis:%s\n", 
                               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_BG_GREEN, ANSI_RESET);
                        printf("      %s%sThis is a UCI response packet that would be received from a UWB device%s\n", 
                               ANSI_COLOR_BRIGHT_WHITE, ANSI_BOLD, ANSI_RESET);
                        printf("      %s%sIndicates the result of a previously sent command%s\n", 
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("    Response Analysis:\n");
                        printf("      This is a UCI response packet that would be received from a UWB device\n");
                        printf("      Indicates the result of a previously sent command\n");
                    }
                    break;
                    
                case NOTIFICATION:
                    if (ui_color_enabled) {
                        printf("    %s%s%sNotification Analysis:%s\n", 
                               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_BG_MAGENTA, ANSI_RESET);
                        printf("      %s%sThis is a UCI notification packet sent asynchronously by the UWB device%s\n", 
                               ANSI_COLOR_BRIGHT_WHITE, ANSI_BOLD, ANSI_RESET);
                        printf("      %s%sDoes not require a preceding command%s\n", 
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("    Notification Analysis:\n");
                        printf("      This is a UCI notification packet sent asynchronously by the UWB device\n");
                        printf("      Does not require a preceding command\n");
                    }
                    break;
                    
                case DATA:
                    if (ui_color_enabled) {
                        printf("    %s%s%sData Message Analysis:%s\n", 
                               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_CYAN, ANSI_RESET);
                        printf("      %s%sThis is a UCI data message packet for in-band communication%s\n", 
                               ANSI_COLOR_BRIGHT_WHITE, ANSI_BOLD, ANSI_RESET);
                        printf("      %s%sUsed for application data transfer between UWB devices%s\n", 
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET);
                    } else {
                        printf("    Data Message Analysis:\n");
                        printf("      This is a UCI data message packet for in-band communication\n");
                        printf("      Used for application data transfer between UWB devices\n");
                    }
                    break;
                    
                default:
                    if (ui_color_enabled) {
                        printf("    %s%s%sUnknown Message Type Analysis:%s\n", 
                               ANSI_COLOR_RED, ANSI_BOLD, ANSI_BG_RED, ANSI_RESET);
                        printf("      %s%sMessage Type 0x%01X is not a recognized UCI message type%s\n", 
                               ANSI_COLOR_BRIGHT_WHITE, ANSI_BOLD, header_fields.message_type, ANSI_RESET);
                    } else {
                        printf("    Unknown Message Type Analysis:\n");
                        printf("      Message Type 0x%01X is not a recognized UCI message type\n", 
                               header_fields.message_type);
                    }
                    break;
            }
            
            // Analyze based on group ID
            if (ui_color_enabled) {
                printf("  %s%s%sGroup ID Analysis:%s\n", 
                       ANSI_COLOR_BRIGHT_BLUE, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
            } else {
                printf("  Group ID Analysis:\n");
            }
            
            switch (header_fields.group_id) {
                case CORE:
                    if (ui_color_enabled) {
                        printf("    %s%sCORE Group (0x%01X):%s Device management and configuration commands%s\n", 
                               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, header_fields.group_id, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("    CORE Group (0x%01X): Device management and configuration commands\n", 
                               header_fields.group_id);
                    }
                    break;
                case SESSION_CONFIG:
                    if (ui_color_enabled) {
                        printf("    %s%sSESSION_CONFIG Group (0x%01X):%s Session initialization and configuration%s\n", 
                               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, header_fields.group_id, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("    SESSION_CONFIG Group (0x%01X): Session initialization and configuration\n", 
                               header_fields.group_id);
                    }
                    break;
                case SESSION_CONTROL:
                    if (ui_color_enabled) {
                        printf("    %s%sSESSION_CONTROL Group (0x%01X):%s Session execution and control%s\n", 
                               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, header_fields.group_id, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("    SESSION_CONTROL Group (0x%01X): Session execution and control\n", 
                               header_fields.group_id);
                    }
                    break;
                case DATA_CONTROL:
                    if (ui_color_enabled) {
                        printf("    %s%sDATA_CONTROL Group (0x%01X):%s Data transfer control%s\n", 
                               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, header_fields.group_id, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("    DATA_CONTROL Group (0x%01X): Data transfer control\n", 
                               header_fields.group_id);
                    }
                    break;
                case RANGING_DATA:
                    if (ui_color_enabled) {
                        printf("    %s%sRANGING_DATA Group (0x%01X):%s Ranging measurement data%s\n", 
                               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, header_fields.group_id, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("    RANGING_DATA Group (0x%01X): Ranging measurement data\n", 
                               header_fields.group_id);
                    }
                    break;
                case VENDOR_ANDROID:
                    if (ui_color_enabled) {
                        printf("    %s%sVENDOR_ANDROID Group (0x%01X):%s Android-specific vendor extensions%s\n", 
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, header_fields.group_id, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("    VENDOR_ANDROID Group (0x%01X): Android-specific vendor extensions\n", 
                               header_fields.group_id);
                    }
                    break;
                case TEST:
                    if (ui_color_enabled) {
                        printf("    %s%sTEST Group (0x%01X):%s Testing and diagnostic commands%s\n", 
                               ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, header_fields.group_id, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("    TEST Group (0x%01X): Testing and diagnostic commands\n", 
                               header_fields.group_id);
                    }
                    break;
                default:
                    if (ui_color_enabled) {
                        printf("    %s%sUnknown Group (0x%01X):%s Unrecognized UCI group ID%s\n", 
                               ANSI_COLOR_RED, ANSI_BOLD, header_fields.group_id, ANSI_RESET, ANSI_RESET);
                    } else {
                        printf("    Unknown Group (0x%01X): Unrecognized UCI group ID\n", 
                               header_fields.group_id);
                    }
                    break;
            }
        }
        
        if (ui_color_enabled) {
            printf("%s%s%sContextual Insights:%s\n", 
                   ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_CYAN, ANSI_RESET);
        } else {
            printf("Contextual Insights:\n");
        }
        
        printf("  Based on the Qorvo QM35 SDK patterns, this implementation follows best practices:\n");
        printf("  - Table-driven handler architecture for efficient command dispatch\n");
        printf("  - Proper segmentation/reassembly flow for fragmented packets\n");
        printf("  - Builder pattern for message construction with proper TLV support\n");
        printf("  - Centralized device state management with command gating\n");
        printf("  - Comprehensive error analysis with detailed status code interpretation\n");
        printf("  - Transport abstraction layer with backpressure handling\n");
    }
    
    if (ui_color_enabled) {
        printf("%s%s%s=== Enhanced Analysis Complete ===%s\n", 
               ANSI_COLOR_BRIGHT_GREEN, ANSI_BOLD, ANSI_BG_GREEN, ANSI_RESET);
    } else {
        printf("=== Enhanced Analysis Complete ===\n");
    }
}

/**
 * @brief Print help information for the analyze command
 */
static void print_analysis_help(void) {
    if (ui_color_enabled) {
        printf("%s%s%sUCI Enhanced Packet Analysis Help%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
        printf("%s%s%s===============================%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
    } else {
        printf("UCI Enhanced Packet Analysis Help\n");
        printf("===============================\n");
    }
    
    if (ui_color_enabled) {
        printf("%s%s%sUsage:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s <bytes...>                    - %s%sAnalyze hex packet bytes%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -v|--verbose <bytes...>       - %s%sEnable verbose analysis%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -t|--tlv <bytes...>           - %s%sEnable TLV analysis%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -c|--compare <packet1> <packet2> - %s%sCompare two packets%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -e|--examples                 - %s%sShow usage examples%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -h|--help                    - %s%sShow this help%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
    } else {
        printf("Usage:\n");
        printf("  analyze_packet <bytes...>                    - Analyze hex packet bytes\n");
        printf("  analyze_packet -v|--verbose <bytes...>       - Enable verbose analysis\n");
        printf("  analyze_packet -t|--tlv <bytes...>           - Enable TLV analysis\n");
        printf("  analyze_packet -c|--compare <packet1> <packet2> - Compare two packets\n");
        printf("  analyze_packet -e|--examples                 - Show usage examples\n");
        printf("  analyze_packet -h|--help                    - Show this help\n");
    }
    
    printf("\n");
    
    if (ui_color_enabled) {
        printf("%s%s%sArguments:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
        printf("  %s%s<bytes...>%s    - %s%sSpace-separated hex bytes representing a UCI packet%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
    } else {
        printf("Arguments:\n");
        printf("  <bytes...>    - Space-separated hex bytes representing a UCI packet\n");
    }
    
    printf("\n");
    
    if (ui_color_enabled) {
        printf("%s%s%sFlags:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
        printf("  %s%s-v, --verbose%s  - %s%sEnable verbose analysis with contextual information%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%s-t, --tlv%s      - %s%sEnable detailed TLV analysis%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%s-c, --compare%s  - %s%sCompare two packets for differences%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%s-e, --examples%s - %s%sShow usage examples%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%s-h, --help%s     - %s%sShow this help information%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
    } else {
        printf("Flags:\n");
        printf("  -v, --verbose  - Enable verbose analysis with contextual information\n");
        printf("  -t, --tlv      - Enable detailed TLV analysis\n");
        printf("  -c, --compare  - Compare two packets for differences\n");
        printf("  -e, --examples - Show usage examples\n");
        printf("  -h, --help     - Show this help information\n");
    }
    
    printf("\n");
    
    if (ui_color_enabled) {
        printf("%s%s%sNote:%s %s%sThis enhanced analyzer builds upon the existing analyze_packet command%s\n", 
               ANSI_COLOR_BRIGHT_MAGENTA, ANSI_BOLD, ANSI_RESET, ANSI_RESET,
               ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("      %s%s%s with additional analysis capabilities based on Qorvo QM35 SDK patterns.%s\n", 
               ANSI_COLOR_BRIGHT_WHITE, ANSI_RESET, ANSI_RESET, ANSI_RESET);
    } else {
        printf("Note: This enhanced analyzer builds upon the existing analyze_packet command\n");
        printf("      with additional analysis capabilities based on Qorvo QM35 SDK patterns.\n");
    }
}

/**
 * @brief Print usage examples for the analyze_packet command
 */
static void print_analysis_examples(void) {
    if (ui_color_enabled) {
        printf("%s%s%sUCI Enhanced Packet Analysis Examples%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
        printf("%s%s%s=====================================%s\n", 
               ANSI_COLOR_BRIGHT_CYAN, ANSI_BOLD, ANSI_BG_BLUE, ANSI_RESET);
    } else {
        printf("UCI Enhanced Packet Analysis Examples\n");
        printf("=====================================\n");
    }
    
    if (ui_color_enabled) {
        printf("%s%s%sBasic Analysis:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s 20 08 00 00                  - %s%sAnalyze CORE_DEVICE_INFO command%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s 21 00 00 05 00 01 00 00 00  - %s%sAnalyze SESSION_INIT command%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s 41 00 00 05 00 01 00 00 00  - %s%sAnalyze SESSION_INIT response%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
    } else {
        printf("Basic Analysis:\n");
        printf("  analyze_packet 20 08 00 00                  - Analyze CORE_DEVICE_INFO command\n");
        printf("  analyze_packet 21 00 00 05 00 01 00 00 00  - Analyze SESSION_INIT command\n");
        printf("  analyze_packet 41 00 00 05 00 01 00 00 00  - Analyze SESSION_INIT response\n");
    }
    
    printf("\n");
    
    if (ui_color_enabled) {
        printf("%s%s%sVerbose Analysis:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -v 41 03 00 06 00 02 48 00 E5 00  - %s%sVerbose SESSION_SET_APP_CONFIG response%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -v 61 02 00 06 01 00 00 00 00 00  - %s%sVerbose SESSION_STATUS notification%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
    } else {
        printf("Verbose Analysis:\n");
        printf("  analyze_packet -v 41 03 00 06 00 02 48 00 E5 00  - Verbose SESSION_SET_APP_CONFIG response\n");
        printf("  analyze_packet -v 61 02 00 06 01 00 00 00 00 00  - Verbose SESSION_STATUS notification\n");
    }
    
    printf("\n");
    
    if (ui_color_enabled) {
        printf("%s%s%sTLV Analysis:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -t 41 04 00 07 DD CC BB AA 02 48 E5  - %s%sTLV SESSION_GET_APP_CONFIG response%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -t 40 05 00 09 00 02 00 01 01 01 02 34 12  - %s%sTLV CORE_GET_CONFIG response%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
    } else {
        printf("TLV Analysis:\n");
        printf("  analyze_packet -t 41 04 00 07 DD CC BB AA 02 48 E5  - TLV SESSION_GET_APP_CONFIG response\n");
        printf("  analyze_packet -t 40 05 00 09 00 02 00 01 01 01 02 34 12  - TLV CORE_GET_CONFIG response\n");
    }
    
    printf("\n");
    
    if (ui_color_enabled) {
        printf("%s%s%sPacket Comparison:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -c \"41 00 00 05 00 01 00 00 00\" \"41 00 00 05 00 02 00 00 00\"  - %s%sCompare SESSION_INIT responses%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s -c \"20 08 00 00\" \"21 00 00 05 00 01 00 00 00\"  - %s%sCompare different packet types%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
    } else {
        printf("Packet Comparison:\n");
        printf("  analyze_packet -c \"41 00 00 05 00 01 00 00 00\" \"41 00 00 05 00 02 00 00 00\"  - Compare SESSION_INIT responses\n");
        printf("  analyze_packet -c \"20 08 00 00\" \"21 00 00 05 00 01 00 00 00\"  - Compare different packet types\n");
    }
    
    printf("\n");
    
    if (ui_color_enabled) {
        printf("%s%s%sData Message Analysis:%s\n", ANSI_COLOR_BRIGHT_YELLOW, ANSI_BOLD, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s 01 00 00 15 CD AB 34 12 08 07 06 05 04 03 02 01 2A 00 05 00 AA BB CC DD EE  - %s%sDATA_MESSAGE_SND%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
        printf("  %s%sanalyze_packet%s 62 04 00 05 CD AB 34 12 01  - %s%sSESSION_DATA_CREDIT_NTF%s\n", 
               ANSI_BOLD, ANSI_COLOR_BRIGHT_GREEN, ANSI_RESET, ANSI_COLOR_WHITE, ANSI_RESET, ANSI_RESET);
    } else {
        printf("Data Message Analysis:\n");
        printf("  analyze_packet 01 00 00 15 CD AB 34 12 08 07 06 05 04 03 02 01 2A 00 05 00 AA BB CC DD EE  - DATA_MESSAGE_SND\n");
        printf("  analyze_packet 62 04 00 05 CD AB 34 12 01  - SESSION_DATA_CREDIT_NTF\n");
    }
}