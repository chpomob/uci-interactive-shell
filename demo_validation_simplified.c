/**
 * @file demo_validation_simplified.c
 * @brief Simplified demonstration of the command validation utilities
 * 
 * This program demonstrates the new command validation utilities that were
 * added to improve code quality and reduce duplication.
 */

#include "include/uci_command_utils.h"
#include <stdio.h>

int main() {
    printf("=== UCI Command Validation Utilities Demo ===\n\n");
    
    // Test 1: Validate minimum arguments
    printf("Test 1: Argument validation\n");
    if (!validate_min_args(1, 3, "test_cmd", "test_cmd <arg1> <arg2>")) {
        printf("  ✗ Validation correctly failed for insufficient arguments\n");
    }
    
    if (validate_min_args(3, 3, "test_cmd", "test_cmd <arg1> <arg2>")) {
        printf("  ✓ Validation correctly passed for sufficient arguments\n");
    }
    
    // Test 2: Validate numeric range
    printf("\nTest 2: Numeric range validation\n");
    long value;
    if (validate_numeric_range("42", -100, 100, "test_value", &value)) {
        printf("  ✓ Valid number %ld accepted\n", value);
    }
    
    if (!validate_numeric_range("abc", -100, 100, "test_value", NULL)) {
        printf("  ✓ Invalid number 'abc' correctly rejected\n");
    }
    
    if (!validate_numeric_range("999", -100, 100, "test_value", NULL)) {
        printf("  ✓ Out-of-range number '999' correctly rejected\n");
    }
    
    // Test 3: Validate hex strings
    printf("\nTest 3: Hex string validation\n");
    if (validate_hex_string("AABBCCDD", 0)) {
        printf("  ✓ Valid hex string 'AABBCCDD' accepted\n");
    }
    
    if (!validate_hex_string("XYZ", 0)) {
        printf("  ✓ Invalid hex string 'XYZ' correctly rejected\n");
    }
    
    if (!validate_hex_string("ABC", 0)) {
        printf("  ✓ Odd-length hex string 'ABC' correctly rejected\n");
    }
    
    // Test 4: Validate session IDs (placeholder)
    printf("\nTest 4: Session ID validation (placeholder)\n");
    printf("  Note: validate_session_id function not yet implemented\n");
    
    // Test 5: Error reporting
    printf("\nTest 5: Error reporting\n");
    report_error(404, "Command not found: %s", "invalid_command");
    report_error(0, "Generic error occurred");
    
    printf("\n=== Demo completed successfully ===\n");
    return 0;
}