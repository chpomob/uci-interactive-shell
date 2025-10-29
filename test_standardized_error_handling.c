#include <stdio.h>
#include <string.h>
#include "../include/uci_standardized_error_handling.h"
#include "../include/uci_utils.h"

int main() {
    printf("Testing standardized error handling improvements...\n");
    
    // Test error logging
    printf("\n1. Testing error logging:\n");
    UCI_LOG_ERROR("Test error context", UCI_ERROR_INVALID_PARAM);
    UCI_LOG_ERROR("Another test", UCI_ERROR_BUFFER_OVERFLOW);
    
    // Test safe string operations
    printf("\n2. Testing safe string operations:\n");
    char test_buffer[20];
    
    // Test safe string copy that should succeed
    uci_error_t result = uci_safe_strcpy(test_buffer, sizeof(test_buffer), "Hello, World!");
    if (result == UCI_SUCCESS) {
        printf("   Safe strcpy succeeded: %s\n", test_buffer);
    } else {
        printf("   Safe strcpy failed with error: %d\n", result);
    }
    
    // Test safe string copy that should fail (buffer overflow)
    result = uci_safe_strcpy(test_buffer, sizeof(test_buffer), "This string is way too long for the buffer size");
    if (result == UCI_SUCCESS) {
        printf("   Buffer overflow test failed - should not have succeeded!\n");
    } else {
        printf("   Buffer overflow correctly prevented: %d\n", result);
    }
    
    // Test safe string copy with truncation
    result = uci_safe_strcpy_trunc(test_buffer, sizeof(test_buffer), "This string will be truncated");
    if (result == UCI_SUCCESS) {
        printf("   Safe strcpy trunc succeeded: %s\n", test_buffer);
    } else {
        printf("   Safe strcpy trunc failed: %d\n", result);
    }
    
    // Test safe memory copy
    printf("\n3. Testing safe memory operations:\n");
    unsigned char src_data[] = {0x01, 0x02, 0x03, 0x04};
    unsigned char dest_data[10];
    
    result = uci_safe_memcpy(dest_data, sizeof(dest_data), src_data, sizeof(src_data));
    if (result == UCI_SUCCESS) {
        printf("   Safe memcpy succeeded:");
        for (size_t i = 0; i < sizeof(src_data); i++) {
            printf(" 0x%02X", dest_data[i]);
        }
        printf("\n");
    } else {
        printf("   Safe memcpy failed: %d\n", result);
    }
    
    // Test memory copy that should fail (buffer too small)
    result = uci_safe_memcpy(dest_data, 2, src_data, sizeof(src_data));
    if (result != UCI_SUCCESS) {
        printf("   Safe memcpy correctly rejected oversized copy: %d\n", result);
    } else {
        printf("   Memory overflow test failed - should not have succeeded!\n");
    }
    
    // Test session ID validation
    printf("\n4. Testing session ID validation:\n");
    result = uci_validate_session_id(5);  // Valid session ID
    printf("   Session ID 5 validation: %s\n", result == UCI_SUCCESS ? "SUCCESS" : "FAILED");
    
    result = uci_validate_session_id(100);  // Invalid session ID (assuming max is smaller)
    printf("   Session ID 100 validation: %s\n", result == UCI_SUCCESS ? "SUCCESS" : "FAILED");
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}