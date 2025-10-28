/**
 * @file uci_cmd_analysis.h
 * @brief Enhanced packet analysis command interface
 * 
 * This file declares the enhanced analyze_command functionality that builds upon 
 * the existing analyze_packet command with additional analysis capabilities.
 */

#ifndef UCI_CMD_ANALYSIS_H
#define UCI_CMD_ANALYSIS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
void handle_analyze_command(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif /* UCI_CMD_ANALYSIS_H */