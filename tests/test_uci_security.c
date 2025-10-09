#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/uci_security.h"

// Test helper functions
static int test_passed = 0;
static int test_failed = 0;

#define RUN_TEST(test_func) do { \
    printf("Running %s... ", #test_func); \
    int result = test_func(); \
    if (result == 0) { \
        printf("PASSED\n"); \
        test_passed++; \
    } else { \
        printf("FAILED\n"); \
        test_failed++; \
    } \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return -1; \
    } \
} while(0)

// Test functions
static int test_sec_malloc() {
    // Test normal allocation
    void* ptr = uci_sec_malloc(100);
    ASSERT(ptr != NULL);
    free(ptr);
    
    // Test zero size allocation
    ptr = uci_sec_malloc(0);
    ASSERT(ptr == NULL);
    
    // Test large size allocation (should fail gracefully)
    ptr = uci_sec_malloc(SIZE_MAX);
    ASSERT(ptr == NULL);
    
    return 0;
}

static int test_sec_calloc() {
    // Test normal allocation
    void* ptr = uci_sec_calloc(10, 10);
    ASSERT(ptr != NULL);
    
    // Check that memory is zero-initialized
    unsigned char* bytes = (unsigned char*)ptr;
    for (int i = 0; i < 100; i++) {
        ASSERT(bytes[i] == 0);
    }
    
    free(ptr);
    
    // Test zero count allocation
    ptr = uci_sec_calloc(0, 10);
    ASSERT(ptr == NULL);
    
    // Test zero size allocation
    ptr = uci_sec_calloc(10, 0);
    ASSERT(ptr == NULL);
    
    // Test overflow
    ptr = uci_sec_calloc(SIZE_MAX / 2, SIZE_MAX / 2);
    ASSERT(ptr == NULL);
    
    return 0;
}

static int test_sec_realloc() {
    // Test normal reallocation
    void* ptr = uci_sec_malloc(50);
    ASSERT(ptr != NULL);
    
    void* new_ptr = uci_sec_realloc(ptr, 100);
    ASSERT(new_ptr != NULL);
    free(new_ptr);
    
    // Test reallocation to zero (should free)
    ptr = uci_sec_malloc(50);
    ASSERT(ptr != NULL);
    
    new_ptr = uci_sec_realloc(ptr, 0);
    ASSERT(new_ptr == NULL);
    
    // Test reallocation of NULL pointer (should allocate)
    new_ptr = uci_sec_realloc(NULL, 100);
    ASSERT(new_ptr != NULL);
    free(new_ptr);
    
    return 0;
}

static int test_sec_strcpy() {
    char dest[10];
    
    // Test normal copy
    ASSERT(uci_sec_strcpy(dest, sizeof(dest), "hello") == UCI_SEC_SUCCESS);
    ASSERT(strcmp(dest, "hello") == 0);
    
    // Test buffer overflow
    ASSERT(uci_sec_strcpy(dest, sizeof(dest), "this is too long") == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_strcpy(NULL, sizeof(dest), "hello") == UCI_SEC_ERROR_INVALID_PARAM);
    ASSERT(uci_sec_strcpy(dest, sizeof(dest), NULL) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_strcpy_trunc() {
    char dest[10];
    
    // Test normal copy
    ASSERT(uci_sec_strcpy_trunc(dest, sizeof(dest), "hello") == UCI_SEC_SUCCESS);
    ASSERT(strcmp(dest, "hello") == 0);
    
    // Test truncation
    ASSERT(uci_sec_strcpy_trunc(dest, sizeof(dest), "this is too long") == UCI_SEC_SUCCESS);
    ASSERT(strlen(dest) == 9); // Should be truncated to 9 chars plus null terminator
    
    // Test null parameters
    ASSERT(uci_sec_strcpy_trunc(NULL, sizeof(dest), "hello") == UCI_SEC_ERROR_INVALID_PARAM);
    ASSERT(uci_sec_strcpy_trunc(dest, sizeof(dest), NULL) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_memcpy() {
    unsigned char src[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char dest[10];
    
    // Test normal copy
    ASSERT(uci_sec_memcpy(dest, sizeof(dest), src, sizeof(src)) == UCI_SEC_SUCCESS);
    ASSERT(memcmp(dest, src, sizeof(src)) == 0);
    
    // Test buffer overflow
    ASSERT(uci_sec_memcpy(dest, 5, src, 10) == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_memcpy(NULL, sizeof(dest), src, sizeof(src)) == UCI_SEC_ERROR_INVALID_PARAM);
    ASSERT(uci_sec_memcpy(dest, sizeof(dest), NULL, sizeof(src)) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_memcpy_safe() {
    unsigned char src[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char dest[5];
    
    // Test safe copy with truncation
    ASSERT(uci_sec_memcpy_safe(dest, sizeof(dest), src, sizeof(src)) == UCI_SEC_SUCCESS);
    ASSERT(memcmp(dest, src, sizeof(dest)) == 0);
    
    // Test null parameters
    ASSERT(uci_sec_memcpy_safe(NULL, sizeof(dest), src, sizeof(src)) == UCI_SEC_ERROR_INVALID_PARAM);
    ASSERT(uci_sec_memcpy_safe(dest, sizeof(dest), NULL, sizeof(src)) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_memcmp_consttime() {
    unsigned char a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char b[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char c[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 11};
    
    // Test equal memory
    ASSERT(uci_sec_memcmp_consttime(a, b, sizeof(a)) == true);
    
    // Test unequal memory
    ASSERT(uci_sec_memcmp_consttime(a, c, sizeof(a)) == false);
    
    // Test null parameters
    ASSERT(uci_sec_memcmp_consttime(NULL, b, sizeof(b)) == false);
    ASSERT(uci_sec_memcmp_consttime(a, NULL, sizeof(a)) == false);
    
    return 0;
}

static int test_sec_read_bytes() {
    unsigned char src[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char dest[5];
    
    // Test normal read
    ASSERT(uci_sec_read_bytes(src, sizeof(src), 2, dest, 5) == UCI_SEC_SUCCESS);
    ASSERT(dest[0] == 3 && dest[1] == 4 && dest[2] == 5 && dest[3] == 6 && dest[4] == 7);
    
    // Test buffer overflow
    ASSERT(uci_sec_read_bytes(src, sizeof(src), 8, dest, 5) == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_read_bytes(NULL, sizeof(src), 0, dest, 5) == UCI_SEC_ERROR_INVALID_PARAM);
    ASSERT(uci_sec_read_bytes(src, sizeof(src), 0, NULL, 5) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_read_u16_le() {
    unsigned char buffer[10] = {0x34, 0x12, 0x78, 0x56, 0xAB, 0xCD, 0xEF, 0xBE, 0xAD, 0xDE};
    uint16_t result;
    
    // Test normal read
    ASSERT(uci_sec_read_u16_le(buffer, sizeof(buffer), 0, &result) == UCI_SEC_SUCCESS);
    ASSERT(result == 0x1234);
    
    ASSERT(uci_sec_read_u16_le(buffer, sizeof(buffer), 2, &result) == UCI_SEC_SUCCESS);
    ASSERT(result == 0x5678);
    
    // Test buffer overflow
    ASSERT(uci_sec_read_u16_le(buffer, sizeof(buffer), 9, &result) == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_read_u16_le(NULL, sizeof(buffer), 0, &result) == UCI_SEC_ERROR_INVALID_PARAM);
    ASSERT(uci_sec_read_u16_le(buffer, sizeof(buffer), 0, NULL) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_read_u32_le() {
    unsigned char buffer[10] = {0x78, 0x56, 0x34, 0x12, 0xBE, 0xEF, 0xCD, 0xAB, 0xDE, 0xAD};
    uint32_t result;
    
    // Test normal read
    ASSERT(uci_sec_read_u32_le(buffer, sizeof(buffer), 0, &result) == UCI_SEC_SUCCESS);
    ASSERT(result == 0x12345678);
    
    ASSERT(uci_sec_read_u32_le(buffer, sizeof(buffer), 4, &result) == UCI_SEC_SUCCESS);
    ASSERT(result == 0xABCDEFBE);
    
    // Test buffer overflow
    ASSERT(uci_sec_read_u32_le(buffer, sizeof(buffer), 7, &result) == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_read_u32_le(NULL, sizeof(buffer), 0, &result) == UCI_SEC_ERROR_INVALID_PARAM);
    ASSERT(uci_sec_read_u32_le(buffer, sizeof(buffer), 0, NULL) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_read_u64_le() {
    unsigned char buffer[10] = {0x78, 0x56, 0x34, 0x12, 0xBE, 0xEF, 0xCD, 0xAB, 0xDE, 0xAD};
    uint64_t result;
    
    // Test normal read
    ASSERT(uci_sec_read_u64_le(buffer, sizeof(buffer), 0, &result) == UCI_SEC_SUCCESS);
    ASSERT(result == 0xABCDEFBE12345678ULL);
    
    // Test buffer overflow
    ASSERT(uci_sec_read_u64_le(buffer, sizeof(buffer), 3, &result) == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_read_u64_le(NULL, sizeof(buffer), 0, &result) == UCI_SEC_ERROR_INVALID_PARAM);
    ASSERT(uci_sec_read_u64_le(buffer, sizeof(buffer), 0, NULL) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_write_u16_le() {
    unsigned char buffer[10] = {0};
    uint16_t value = 0x1234;
    
    // Test normal write
    ASSERT(uci_sec_write_u16_le(buffer, sizeof(buffer), 0, value) == UCI_SEC_SUCCESS);
    ASSERT(buffer[0] == 0x34 && buffer[1] == 0x12);
    
    // Test buffer overflow
    ASSERT(uci_sec_write_u16_le(buffer, sizeof(buffer), 9, value) == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_write_u16_le(NULL, sizeof(buffer), 0, value) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_write_u32_le() {
    unsigned char buffer[10] = {0};
    uint32_t value = 0x12345678;
    
    // Test normal write
    ASSERT(uci_sec_write_u32_le(buffer, sizeof(buffer), 0, value) == UCI_SEC_SUCCESS);
    ASSERT(buffer[0] == 0x78 && buffer[1] == 0x56 && buffer[2] == 0x34 && buffer[3] == 0x12);
    
    // Test buffer overflow
    ASSERT(uci_sec_write_u32_le(buffer, sizeof(buffer), 7, value) == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_write_u32_le(NULL, sizeof(buffer), 0, value) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

static int test_sec_write_u64_le() {
    unsigned char buffer[10] = {0};
    uint64_t value = 0x12345678ABCDEFBEULL;
    
    // Test normal write
    ASSERT(uci_sec_write_u64_le(buffer, sizeof(buffer), 0, value) == UCI_SEC_SUCCESS);
    ASSERT(buffer[0] == 0xBE && buffer[1] == 0xEF && buffer[2] == 0xCD && buffer[3] == 0xAB &&
           buffer[4] == 0x78 && buffer[5] == 0x56 && buffer[6] == 0x34 && buffer[7] == 0x12);
    
    // Test buffer overflow
    ASSERT(uci_sec_write_u64_le(buffer, sizeof(buffer), 3, value) == UCI_SEC_ERROR_BUFFER_OVERFLOW);
    
    // Test null parameters
    ASSERT(uci_sec_write_u64_le(NULL, sizeof(buffer), 0, value) == UCI_SEC_ERROR_INVALID_PARAM);
    
    return 0;
}

int main() {
    printf("Running UCI Security Tests\n");
    printf("==========================\n\n");
    
    RUN_TEST(test_sec_malloc);
    RUN_TEST(test_sec_calloc);
    RUN_TEST(test_sec_realloc);
    RUN_TEST(test_sec_strcpy);
    RUN_TEST(test_sec_strcpy_trunc);
    RUN_TEST(test_sec_memcpy);
    RUN_TEST(test_sec_memcpy_safe);
    RUN_TEST(test_sec_memcmp_consttime);
    RUN_TEST(test_sec_read_bytes);
    RUN_TEST(test_sec_read_u16_le);
    RUN_TEST(test_sec_read_u32_le);
    RUN_TEST(test_sec_read_u64_le);
    RUN_TEST(test_sec_write_u16_le);
    RUN_TEST(test_sec_write_u32_le);
    RUN_TEST(test_sec_write_u64_le);
    
    printf("\nTest Results:\n");
    printf("=============\n");
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("Total:  %d\n", test_passed + test_failed);
    
    if (test_failed == 0) {
        printf("\nAll tests PASSED!\n");
        return 0;
    } else {
        printf("\n%d tests FAILED!\n", test_failed);
        return 1;
    }
}