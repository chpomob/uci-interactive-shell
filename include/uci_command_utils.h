/**
 * @file uci_command_utils.h
 * @brief Utility functions for command validation and error handling
 *
 * Provides standardized utilities for command argument validation,
 * error reporting, and consistent user messaging across the UCI shell.
 */

#ifndef UCI_COMMAND_UTILS_H
#define UCI_COMMAND_UTILS_H

#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Standardized command argument validation
 *
 * Validates that a command has the minimum required number of arguments
 * and prints a consistent usage message if not.
 *
 * @param argc Argument count from command handler
 * @param argv Argument vector from command handler
 * @param min_args Minimum required arguments (including command name)
 * @param cmd_name Name of the command for usage message
 * @param usage_fmt Format string for usage details
 * @return true if validation passes, false if validation fails
 */
bool validate_min_args(int argc, int min_args, const char* cmd_name, const char* usage_fmt, ...);

/**
 * @brief Validate numeric argument range
 *
 * Validates that a numeric argument falls within specified bounds.
 *
 * @param value_str String representation of the numeric value
 * @param min_val Minimum acceptable value
 * @param max_val Maximum acceptable value
 * @param value_name Name of the value for error messages
 * @param out_value Optional pointer to store parsed value
 * @return true if validation passes, false if validation fails
 */
bool validate_numeric_range(const char* value_str, long min_val, long max_val, 
                           const char* value_name, long* out_value);

/**
 * @brief Validate hex string format
 *
 * Validates that a string contains only valid hexadecimal characters.
 *
 * @param hex_str String to validate
 * @param expected_len Expected length in bytes (0 for variable)
 * @return true if validation passes, false if validation fails
 */
bool validate_hex_string(const char* hex_str, size_t expected_len);

/**
 * @brief Standardized error reporting
 *
 * Provides consistent error reporting with optional error codes.
 *
 * @param error_code Numeric error code (0 for no code)
 * @param fmt Format string for error message
 */
void report_error(int error_code, const char* fmt, ...);

/**
 * @brief Validate session ID format
 *
 * Validates that a session ID string is properly formatted.
 *
 * @param session_id_str Session ID string to validate
 * @param out_session_id Optional pointer to store parsed session ID
 * @return true if validation passes, false if validation fails
 */
bool validate_session_id(const char* session_id_str, unsigned int* out_session_id);

/**
 * @brief Safely parse a hex string into a byte array
 * 
 * Converts a hexadecimal string representation into a byte array with
 * comprehensive validation and buffer overflow protection.
 * 
 * @param hex_str Input hexadecimal string (without 0x prefix)
 * @param bytes Output byte array buffer
 * @param max_len Maximum number of bytes that can be stored
 * @param out_len Output parameter for actual number of bytes parsed
 * @return true if parsing successful, false on error
 */
bool parse_hex_string_safe(const char* hex_str, unsigned char* bytes, size_t max_len, size_t* out_len);
#endif // UCI_COMMAND_UTILS_H
