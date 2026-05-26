/*
 * Tests for uci_packet_structures.c - all serialization, deserialization,
 * validation, and lookup table functions with happy + error paths.
 */
#include "test_runner.h"
#include "../include/uci_packet_structures.h"
#include "../include/uci.h"
#include <stdlib.h>
#include <string.h>

static int test_session_init_ok(void) {
    uci_session_init_payload_t p = { .session_id = 0x0A0B0C0D, .session_type = 3 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_init(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (rc=%d)\n", rc); return 0; }
    /* write_u32_le stores bytes in little-endian: 0x0D, 0x0C, 0x0B, 0x0A */
    if (buf[0] != 0x0D || buf[1] != 0x0C || buf[2] != 0x0B || buf[3] != 0x0A) { printf(" FAIL (session_id bytes)\n"); return 0; }
    if (buf[4] != 3) { printf(" FAIL (session_type=%d)\n", buf[4]); return 0; }
    return 1;
}

static int test_session_init_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_init(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_init_null_dst(void) {
    uci_session_init_payload_t p = { .session_id = 1, .session_type = 2 };
    return uci_serialize_session_init(&p, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_init_dst_too_small(void) {
    uci_session_init_payload_t p = { .session_id = 0x12345678, .session_type = 2 };
    unsigned char buf[4];
    /* buf is only 4 bytes, but payload needs 5 bytes.
     * Use explicit size 4 to pass to the serialize function. */
    return uci_serialize_session_init(&p, buf, 4) == UCI_ERROR_BUFFER_OVERFLOW ? 1 : 0;
}

static int test_session_deinit_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_deinit(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_deinit_null_dst(void) {
    uci_session_deinit_payload_t p = { .session_id = 1 };
    return uci_serialize_session_deinit(&p, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_control_ok(void) {
    uci_session_control_payload_t p = { .session_id = 0x56789ABC };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_control(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (rc=%d)\n", rc); return 0; }
    /* write_u32_le stores bytes in little-endian: 0xBC, 0x9A, 0x78, 0x56 */
    if (buf[0] != 0xBC || buf[1] != 0x9A || buf[2] != 0x78 || buf[3] != 0x56) { printf(" FAIL (session_id bytes)\n"); return 0; }
    return 1;
}

static int test_session_control_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_control(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_control_null_dst(void) {
    uci_session_control_payload_t p = { .session_id = 1 };
    return uci_serialize_session_control(&p, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_get_state_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_get_state(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_get_state_null_dst(void) {
    uci_session_get_state_payload_t p = { .session_id = 1 };
    return uci_serialize_session_get_state(&p, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_get_ranging_count_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_get_ranging_count(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_get_ranging_count_null_dst(void) {
    uci_session_get_ranging_count_payload_t p = { .session_id = 1, .ranging_count = 5 };
    return uci_serialize_session_get_ranging_count(&p, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_app_config_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_app_config(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_app_config_null_dst(void) {
    uci_session_app_config_payload_t p = { .session_id = 1, .num_tlvs = 0 };
    return uci_serialize_session_app_config(&p, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_logical_link_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_logical_link(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_logical_link_null_dst(void) {
    uci_session_logical_link_payload_t p = { .session_id = 1, .link_id = 0, .mode = 0 };
    return uci_serialize_session_logical_link(&p, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_multicast_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_multicast(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_multicast_null_dst(void) {
    return uci_serialize_session_multicast(NULL, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_dtp_config_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_dtp_config(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_dtp_config_null_dst(void) {
    return uci_serialize_session_dtp_config(NULL, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_hus_config_null_src(void) {
    unsigned char buf[32];
    return uci_serialize_session_hus_config(NULL, buf, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_session_hus_config_null_dst(void) {
    return uci_serialize_session_hus_config(NULL, NULL, 32) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

/* deserialization */
static int test_deserialize_session_init_ok(void) {
    uci_session_init_payload_t p = { .session_id = 0xDEADBEEF, .session_type = 1 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_init(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_init_payload_t out = { 0 };
    rc = uci_deserialize_session_init(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0xDEADBEEF || out.session_type != 1) { printf(" FAIL (round-trip mismatch)\n"); return 0; }
    return 1;
}

static int test_deserialize_session_init_src_too_short(void) {
    unsigned char buf[2] = { 0 };
    uci_session_init_payload_t out = { 0 };
    /* src_size < 5 returns UCI_ERROR_INVALID_PARAM (not MALFORMED) */
    return uci_deserialize_session_init(buf, 2, &out) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_init_null_src(void) {
    uci_session_init_payload_t out = { 0 };
    return uci_deserialize_session_init(NULL, 5, &out) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_init_null_dst(void) {
    unsigned char buf[32];
    uci_session_init_payload_t p = { .session_id = 1, .session_type = 0 };
    uci_error_t rc = uci_serialize_session_init(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    return uci_deserialize_session_init(buf, 32, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_deinit_ok(void) {
    uci_session_deinit_payload_t p = { .session_id = 0x42 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_deinit(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_deinit_payload_t out = { 0 };
    rc = uci_deserialize_session_deinit(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0x42) { printf(" FAIL (session_id mismatch)\n"); return 0; }
    return 1;
}

static int test_deserialize_session_deinit_null_out(void) {
    unsigned char buf[32];
    uci_session_deinit_payload_t p = { .session_id = 1 };
    uci_error_t rc = uci_serialize_session_deinit(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    return uci_deserialize_session_deinit(buf, 32, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_control_null_out(void) {
    unsigned char buf[32];
    uci_session_control_payload_t p = { .session_id = 1 };
    uci_error_t rc = uci_serialize_session_control(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    return uci_deserialize_session_control(buf, 32, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_get_state_ok(void) {
    uci_session_get_state_payload_t p = { .session_id = 0x55667788 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_get_state(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_get_state_payload_t out = { 0 };
    rc = uci_deserialize_session_get_state(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0x55667788) { printf(" FAIL (session_id mismatch)\n"); return 0; }
    return 1;
}

static int test_deserialize_session_get_ranging_count_ok(void) {
    uci_session_get_ranging_count_payload_t p = { .session_id = 0x99, .ranging_count = 42 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_get_ranging_count(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_get_ranging_count_payload_t out = { 0 };
    rc = uci_deserialize_session_get_ranging_count(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0x99 || out.ranging_count != 42) { printf(" FAIL (round-trip mismatch)\n"); return 0; }
    return 1;
}

static int test_deserialize_session_get_ranging_count_null_out(void) {
    unsigned char buf[32];
    uci_session_get_ranging_count_payload_t p = { .session_id = 1, .ranging_count = 5 };
    uci_error_t rc = uci_serialize_session_get_ranging_count(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    return uci_deserialize_session_get_ranging_count(buf, 32, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_app_config_null_out(void) {
    unsigned char buf[32];
    uci_session_app_config_payload_t p = { .session_id = 1, .num_tlvs = 0 };
    uci_error_t rc = uci_serialize_session_app_config(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    return uci_deserialize_session_app_config(buf, 32, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_logical_link_null_out(void) {
    unsigned char buf[32];
    uci_session_logical_link_payload_t p = { .session_id = 1, .link_id = 0, .mode = 0 };
    uci_error_t rc = uci_serialize_session_logical_link(&p, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    return uci_deserialize_session_logical_link(buf, 32, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_multicast_null_out(void) {
    return uci_deserialize_session_multicast(NULL, 0, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_dtp_config_null_out(void) {
    return uci_deserialize_session_dtp_config(NULL, 0, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_deserialize_session_hus_config_null_out(void) {
    return uci_deserialize_session_hus_config(NULL, 0, NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

/* round-trip */
static int test_roundtrip_session_control(void) {
    uci_session_control_payload_t in = { .session_id = 0x01020304 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_control(&in, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_control_payload_t out = { 0 };
    rc = uci_deserialize_session_control(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0x01020304) { printf(" FAIL (session_id mismatch)\n"); return 0; }
    return 1;
}

static int test_roundtrip_session_get_ranging_count(void) {
    uci_session_get_ranging_count_payload_t in = { .session_id = 0x99, .ranging_count = 42 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_get_ranging_count(&in, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_get_ranging_count_payload_t out = { 0 };
    rc = uci_deserialize_session_get_ranging_count(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0x99 || out.ranging_count != 42) { printf(" FAIL (round-trip mismatch)\n"); return 0; }
    return 1;
}

static int test_roundtrip_session_get_state(void) {
    uci_session_get_state_payload_t in = { .session_id = 0xDEADBEEF };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_get_state(&in, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_get_state_payload_t out = { 0 };
    rc = uci_deserialize_session_get_state(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0xDEADBEEF) { printf(" FAIL (session_id mismatch)\n"); return 0; }
    return 1;
}

static int test_roundtrip_session_init(void) {
    uci_session_init_payload_t in = { .session_id = 0xABCD1234, .session_type = 5 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_init(&in, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_init_payload_t out = { 0 };
    rc = uci_deserialize_session_init(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0xABCD1234 || out.session_type != 5) { printf(" FAIL (round-trip mismatch)\n"); return 0; }
    return 1;
}

static int test_roundtrip_session_deinit(void) {
    uci_session_deinit_payload_t in = { .session_id = 0x55 };
    unsigned char buf[32];
    uci_error_t rc = uci_serialize_session_deinit(&in, buf, 32);
    if (rc != UCI_SUCCESS) { printf(" FAIL (serialize rc=%d)\n", rc); return 0; }
    uci_session_deinit_payload_t out = { 0 };
    rc = uci_deserialize_session_deinit(buf, 32, &out);
    if (rc != UCI_SUCCESS) { printf(" FAIL (deserialize rc=%d)\n", rc); return 0; }
    if (out.session_id != 0x55) { printf(" FAIL (session_id mismatch)\n"); return 0; }
    return 1;
}

/* validation */
static int test_validate_session_init_valid(void) {
    uci_session_init_payload_t p = { .session_id = 0x1, .session_type = 4 };
    return uci_validate_session_init(&p) == UCI_SUCCESS ? 1 : 0;
}

static int test_validate_session_init_invalid_type(void) {
    uci_session_init_payload_t p = { .session_id = 0x1, .session_type = 5 };
    return uci_validate_session_init(&p) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_deinit_valid(void) {
    uci_session_deinit_payload_t p = { .session_id = 0 };
    return uci_validate_session_deinit(&p) == UCI_SUCCESS ? 1 : 0;
}

static int test_validate_session_deinit_null(void) {
    return uci_validate_session_deinit(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_control_valid(void) {
    uci_session_control_payload_t p = { .session_id = 1 };
    return uci_validate_session_control(&p) == UCI_SUCCESS ? 1 : 0;
}

static int test_validate_session_control_null(void) {
    return uci_validate_session_control(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_get_state_valid(void) {
    /* session_id must be < MAX_SESSIONS (10) since validate_session_get_state delegates to validate_session_control which checks < MAX_SESSIONS */
    uci_session_get_state_payload_t p = { .session_id = 9 };
    return uci_validate_session_get_state(&p) == UCI_SUCCESS ? 1 : 0;
}

static int test_validate_session_get_state_null(void) {
    return uci_validate_session_get_state(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_get_ranging_count_valid(void) {
    uci_session_get_ranging_count_payload_t p = { .session_id = 1, .ranging_count = 0 };
    return uci_validate_session_get_ranging_count(&p) == UCI_SUCCESS ? 1 : 0;
}

static int test_validate_session_get_ranging_count_null(void) {
    return uci_validate_session_get_ranging_count(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_app_config_valid(void) {
    uci_session_app_config_payload_t p = { .session_id = 1, .num_tlvs = 1 };
    return uci_validate_session_app_config(&p) == UCI_SUCCESS ? 1 : 0;
}

static int test_validate_session_app_config_num_tlvs_too_large(void) {
    uci_session_app_config_payload_t p = { .session_id = 1, .num_tlvs = 33 };
    return uci_validate_session_app_config(&p) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_app_config_null(void) {
    return uci_validate_session_app_config(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_logical_link_valid(void) {
    uci_session_logical_link_payload_t p = { .session_id = 1, .link_id = 0, .mode = 0 };
    return uci_validate_session_logical_link(&p) == UCI_SUCCESS ? 1 : 0;
}

static int test_validate_session_logical_link_invalid_mode(void) {
    uci_session_logical_link_payload_t p = { .session_id = 1, .link_id = 0, .mode = 3 };
    return uci_validate_session_logical_link(&p) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_logical_link_null(void) {
    return uci_validate_session_logical_link(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_multicast_null(void) {
    return uci_validate_session_multicast(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_dtp_config_null(void) {
    return uci_validate_session_dtp_config(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

static int test_validate_session_hus_config_null(void) {
    return uci_validate_session_hus_config(NULL) == UCI_ERROR_INVALID_PARAM ? 1 : 0;
}

/* lookup tables */
static int test_get_payload_format_session_init(void) {
    const uci_payload_format_def_t* fmt = uci_get_payload_format(0x01, 0x01);
    return (fmt != NULL && fmt->fields != NULL && fmt->num_fields > 0) ? 1 : 0;
}

static int test_get_payload_format_unknown(void) {
    const uci_payload_format_def_t* fmt = uci_get_payload_format(0xFF, 0xFF);
    return (fmt == NULL) ? 1 : 0;
}

static int test_get_packet_context_session_init(void) {
    const uci_packet_ctx_t* ctx = uci_get_packet_context(0x01, 0x01);
    return (ctx != NULL && ctx->serialize_fn != NULL && ctx->deserialize_fn != NULL) ? 1 : 0;
}

static int test_get_packet_context_unknown(void) {
    const uci_packet_ctx_t* ctx = uci_get_packet_context(0xFF, 0xFF);
    return (ctx == NULL) ? 1 : 0;
}

int main() {
    TEST_SUITE(packet_structures);

    /* serialization */
    TEST_CASE(serialize_session_init_ok); if (test_session_init_ok()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_init_null_src); if (test_session_init_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_init_null_dst); if (test_session_init_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_init_dst_too_small); if (test_session_init_dst_too_small()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_deinit_null_src); if (test_session_deinit_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_deinit_null_dst); if (test_session_deinit_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_control_ok); if (test_session_control_ok()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_control_null_src); if (test_session_control_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_control_null_dst); if (test_session_control_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_get_state_null_src); if (test_session_get_state_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_get_state_null_dst); if (test_session_get_state_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_get_ranging_count_null_src); if (test_session_get_ranging_count_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_get_ranging_count_null_dst); if (test_session_get_ranging_count_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_app_config_null_src); if (test_session_app_config_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_app_config_null_dst); if (test_session_app_config_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_logical_link_null_src); if (test_session_logical_link_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_logical_link_null_dst); if (test_session_logical_link_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_multicast_null_src); if (test_session_multicast_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_multicast_null_dst); if (test_session_multicast_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_dtp_config_null_src); if (test_session_dtp_config_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_dtp_config_null_dst); if (test_session_dtp_config_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_hus_config_null_src); if (test_session_hus_config_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(serialize_session_hus_config_null_dst); if (test_session_hus_config_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }

    /* deserialization */
    TEST_CASE(deserialize_session_init_ok); if (test_deserialize_session_init_ok()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_init_src_too_short); if (test_deserialize_session_init_src_too_short()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_init_null_src); if (test_deserialize_session_init_null_src()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_init_null_dst); if (test_deserialize_session_init_null_dst()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_deinit_ok); if (test_deserialize_session_deinit_ok()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_deinit_null_out); if (test_deserialize_session_deinit_null_out()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_control_null_out); if (test_deserialize_session_control_null_out()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_get_state_ok); if (test_deserialize_session_get_state_ok()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_get_ranging_count_ok); if (test_deserialize_session_get_ranging_count_ok()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_get_ranging_count_null_out); if (test_deserialize_session_get_ranging_count_null_out()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_app_config_null_out); if (test_deserialize_session_app_config_null_out()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_logical_link_null_out); if (test_deserialize_session_logical_link_null_out()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_multicast_null_out); if (test_deserialize_session_multicast_null_out()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_dtp_config_null_out); if (test_deserialize_session_dtp_config_null_out()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(deserialize_session_hus_config_null_out); if (test_deserialize_session_hus_config_null_out()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }

    /* round-trip */
    TEST_CASE(roundtrip_session_control); if (test_roundtrip_session_control()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(roundtrip_session_get_ranging_count); if (test_roundtrip_session_get_ranging_count()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(roundtrip_session_get_state); if (test_roundtrip_session_get_state()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(roundtrip_session_init); if (test_roundtrip_session_init()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(roundtrip_session_deinit); if (test_roundtrip_session_deinit()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }

    /* validation */
    TEST_CASE(validate_session_init_valid); if (test_validate_session_init_valid()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_init_invalid_type); if (test_validate_session_init_invalid_type()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_deinit_valid); if (test_validate_session_deinit_valid()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_deinit_null); if (test_validate_session_deinit_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_control_valid); if (test_validate_session_control_valid()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_control_null); if (test_validate_session_control_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_get_state_valid); if (test_validate_session_get_state_valid()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_get_state_null); if (test_validate_session_get_state_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_get_ranging_count_valid); if (test_validate_session_get_ranging_count_valid()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_get_ranging_count_null); if (test_validate_session_get_ranging_count_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_app_config_valid); if (test_validate_session_app_config_valid()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_app_config_num_tlvs_too_large); if (test_validate_session_app_config_num_tlvs_too_large()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_app_config_null); if (test_validate_session_app_config_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_logical_link_valid); if (test_validate_session_logical_link_valid()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_logical_link_invalid_mode); if (test_validate_session_logical_link_invalid_mode()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_logical_link_null); if (test_validate_session_logical_link_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_multicast_null); if (test_validate_session_multicast_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_dtp_config_null); if (test_validate_session_dtp_config_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(validate_session_hus_config_null); if (test_validate_session_hus_config_null()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }

    /* lookup tables */
    TEST_CASE(get_payload_format_session_init); if (test_get_payload_format_session_init()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(get_payload_format_unknown); if (test_get_payload_format_unknown()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(get_packet_context_session_init); if (test_get_packet_context_session_init()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }
    TEST_CASE(get_packet_context_unknown); if (test_get_packet_context_unknown()) { total_tests_passed++; printf(" PASSED\n"); } else { total_tests_failed++; }

    TEST_SUITE_END();
    printf("\nResults: %d passed, %d failed out of %d total tests\n", total_tests_passed, total_tests_failed, total_tests_passed + total_tests_failed);
    return total_tests_failed;
}
