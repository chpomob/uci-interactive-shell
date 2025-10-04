#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Very simple test framework
#define TEST_SUITE(name) \
    int total_tests_passed = 0; \
    int total_tests_failed = 0; \
    printf("=== Running test suite: %s ===\n", #name);

#define TEST_CASE(name) \
    printf("  Running test case: %s...", #name);

#define TEST_PASS() \
    printf(" PASSED\n"); \
    total_tests_passed++;

#define TEST_FAIL(msg) \
    printf(" FAILED (%s)\n", msg); \
    total_tests_failed++;

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            TEST_FAIL("Expected true, but got false"); \
            goto test_case_end; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            TEST_FAIL("Expected false, but got true"); \
            goto test_case_end; \
        } \
    } while(0)

#define ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf(" FAILED (Expected %d, but got %d)", (expected), (actual)); \
            total_tests_failed++; \
            goto test_case_end; \
        } \
    } while(0)

#define ASSERT_STRING_EQUAL(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf(" FAILED (Expected \"%s\", but got \"%s\")", (expected), (actual)); \
            total_tests_failed++; \
            goto test_case_end; \
        } \
    } while(0)

#define TEST_SUITE_END() \
    printf("\n=== Test Suite Summary ===\n"); \
    printf("Passed: %d\n", total_tests_passed); \
    printf("Failed: %d\n", total_tests_failed); \
    printf("Total:  %d\n", total_tests_passed + total_tests_failed); \
    if (total_tests_failed == 0) { \
        printf("RESULT: ALL TESTS PASSED\n"); \
    } else { \
        printf("RESULT: %d TEST(S) FAILED\n", total_tests_failed); \
    } \
    return total_tests_failed;

#endif // TEST_RUNNER_H