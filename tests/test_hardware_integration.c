#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "../include/uci.h"
#include "../include/uci_hw_interface.h"

#define HW_TEST_RESPONSE_BUFFER 1024
#define HW_TEST_DEFAULT_TIMEOUT_MS 1500
#define HW_TEST_MAX_RETRIES 3

typedef struct {
    const char* device_path;
    int timeout_ms;
    int include_reset;
    int verbose;
} hw_test_config_t;

typedef struct {
    unsigned char buffer[HW_TEST_RESPONSE_BUFFER];
    int length;
} hw_packet_t;

static long long monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000LL);
}

static int parse_int_env(const char* key, int default_value) {
    const char* value = getenv(key);
    char* endptr = NULL;
    long parsed;

    if (!value || !*value) {
        return default_value;
    }

    parsed = strtol(value, &endptr, 10);
    if (endptr == value || *endptr != '\0' || parsed <= 0 || parsed > 60000) {
        return default_value;
    }

    return (int)parsed;
}

static int parse_bool_env(const char* key, int default_value) {
    const char* value = getenv(key);
    if (!value || !*value) {
        return default_value;
    }

    if (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0) {
        return 1;
    }
    if (strcmp(value, "0") == 0 || strcasecmp(value, "false") == 0 || strcasecmp(value, "no") == 0) {
        return 0;
    }

    return default_value;
}

static int is_known_uci_status(unsigned char status) {
    if (status <= UCI_STATUS_NOT_APPLICABLE) {
        return 1;
    }
    if (status >= UCI_STATUS_SESSION_NOT_EXIST && status <= UCI_STATUS_OK_NEGATIVE_DISTANCE_REPORT) {
        return 1;
    }
    if (status >= UCI_STATUS_RANGING_TX_FAILED && status <= UCI_STATUS_ERROR_DL_TDOA_DEVICE_ADDRESS_NOT_MATCHING_IN_REPLY_TIME_LIST) {
        return 1;
    }
    return 0;
}

static int wait_for_response(unsigned char expected_gid,
                             unsigned char expected_oid,
                             int timeout_ms,
                             hw_packet_t* out_packet) {
    long long deadline_ms = monotonic_ms() + timeout_ms;

    while (1) {
        long long now_ms = monotonic_ms();
        int remaining_ms;
        int length;
        struct uci_packet_header* header;
        uci_header_fields_t fields;

        if (now_ms >= deadline_ms) {
            return -1;
        }

        remaining_ms = (int)(deadline_ms - now_ms);
        if (remaining_ms <= 0) {
            return -1;
        }

        length = uci_hw_interface_receive_response(out_packet->buffer,
                                                   sizeof(out_packet->buffer),
                                                   remaining_ms);
        if (length <= 0) {
            return -1;
        }

        if (length < (int)sizeof(struct uci_packet_header)) {
            fprintf(stderr, "Received short packet (%d bytes)\n", length);
            continue;
        }

        out_packet->length = length;
        header = (struct uci_packet_header*)out_packet->buffer;
        uci_extract_header_fields_safe(header, &fields);

        if ((int)(sizeof(struct uci_packet_header) + fields.payload_length) > length) {
            fprintf(stderr,
                    "Received truncated packet for MT=0x%02X GID=0x%02X OID=0x%02X (len=%d payload=%u)\n",
                    fields.message_type, fields.group_id, fields.opcode_id, length, fields.payload_length);
            continue;
        }

        if (fields.message_type == RESPONSE &&
            fields.group_id == expected_gid &&
            fields.opcode_id == expected_oid) {
            return 0;
        }

        printf("[hardware-test] Ignoring async packet MT=0x%02X GID=0x%02X OID=0x%02X\n",
               fields.message_type, fields.group_id, fields.opcode_id);
    }
}

static int send_command_expect_response(unsigned char gid,
                                        unsigned char oid,
                                        const unsigned char* payload,
                                        int payload_len,
                                        int timeout_ms,
                                        hw_packet_t* out_packet) {
    int attempt;

    for (attempt = 0; attempt < HW_TEST_MAX_RETRIES; attempt++) {
        struct uci_packet_header* header;
        uci_header_fields_t fields;
        const unsigned char* rsp_payload;

        if (uci_hw_interface_send_command(COMMAND,
                                          COMPLETE,
                                          gid,
                                          oid,
                                          (unsigned char*)payload,
                                          payload_len) != 0) {
            fprintf(stderr, "Failed to send command GID=0x%02X OID=0x%02X\n", gid, oid);
            return -1;
        }

        if (wait_for_response(gid, oid, timeout_ms, out_packet) != 0) {
            fprintf(stderr,
                    "Timed out waiting for response GID=0x%02X OID=0x%02X\n",
                    gid,
                    oid);
            return -1;
        }

        header = (struct uci_packet_header*)out_packet->buffer;
        uci_extract_header_fields_safe(header, &fields);

        rsp_payload = out_packet->buffer + sizeof(struct uci_packet_header);
        if (fields.payload_length > 0 && rsp_payload[0] == UCI_STATUS_COMMAND_RETRY) {
            printf("[hardware-test] Device requested retry for GID=0x%02X OID=0x%02X (attempt %d/%d)\n",
                   gid,
                   oid,
                   attempt + 1,
                   HW_TEST_MAX_RETRIES);
            continue;
        }

        return 0;
    }

    fprintf(stderr,
            "Device kept returning COMMAND_RETRY for GID=0x%02X OID=0x%02X\n",
            gid,
            oid);
    return -1;
}

static int validate_response_basics(const hw_packet_t* packet,
                                    unsigned char expected_gid,
                                    unsigned char expected_oid,
                                    const char* label,
                                    int require_status_ok) {
    const struct uci_packet_header* header;
    const unsigned char* payload;
    uci_header_fields_t fields;
    unsigned char status;

    if (!packet || packet->length < (int)sizeof(struct uci_packet_header)) {
        fprintf(stderr, "%s: invalid packet length\n", label);
        return -1;
    }

    header = (const struct uci_packet_header*)packet->buffer;
    uci_extract_header_fields_safe(header, &fields);

    if (fields.message_type != RESPONSE) {
        fprintf(stderr, "%s: expected RESPONSE MT, got 0x%02X\n", label, fields.message_type);
        return -1;
    }
    if (fields.group_id != expected_gid) {
        fprintf(stderr, "%s: expected GID 0x%02X, got 0x%02X\n", label, expected_gid, fields.group_id);
        return -1;
    }
    if (fields.opcode_id != expected_oid) {
        fprintf(stderr, "%s: expected OID 0x%02X, got 0x%02X\n", label, expected_oid, fields.opcode_id);
        return -1;
    }
    if ((int)(sizeof(struct uci_packet_header) + fields.payload_length) > packet->length) {
        fprintf(stderr,
                "%s: payload length mismatch (header says %u, packet len %d)\n",
                label,
                fields.payload_length,
                packet->length);
        return -1;
    }
    if (fields.payload_length < 1) {
        fprintf(stderr, "%s: response payload too short for status byte\n", label);
        return -1;
    }

    payload = packet->buffer + sizeof(struct uci_packet_header);
    status = payload[0];
    if (!is_known_uci_status(status)) {
        fprintf(stderr, "%s: unknown status byte 0x%02X\n", label, status);
        return -1;
    }

    printf("[hardware-test] %s: status=0x%02X\n", label, status);

    if (require_status_ok && status != UCI_STATUS_OK) {
        fprintf(stderr, "%s: expected status OK (0x00), got 0x%02X\n", label, status);
        return -1;
    }

    return 0;
}

static int load_config(hw_test_config_t* cfg) {
    if (!cfg) {
        return -1;
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->device_path = getenv("UCI_HW_DEVICE");
    cfg->timeout_ms = parse_int_env("UCI_HW_TIMEOUT_MS", HW_TEST_DEFAULT_TIMEOUT_MS);
    cfg->include_reset = parse_bool_env("UCI_HW_INCLUDE_RESET", 0);
    cfg->verbose = parse_bool_env("UCI_HW_VERBOSE", 0);

    return 0;
}

int main(void) {
    hw_test_config_t cfg;
    hw_packet_t packet;
    int failures = 0;
    unsigned char get_config_payload[2] = { 1, DEVICE_STATE };

    if (load_config(&cfg) != 0) {
        fprintf(stderr, "Failed to load hardware test config\n");
        return 1;
    }

    if (!cfg.device_path || cfg.device_path[0] == '\0') {
        printf("=== Hardware integration test suite ===\n");
        printf("SKIPPED: set UCI_HW_DEVICE=/dev/ttyUSB0 (or equivalent) to run against real hardware.\n");
        return 0;
    }

    printf("=== Hardware integration test suite ===\n");
    printf("Device: %s\n", cfg.device_path);
    printf("Timeout: %d ms\n", cfg.timeout_ms);
    printf("Include reset test: %s\n", cfg.include_reset ? "yes" : "no");

    uci_hw_interface_set_verbose(cfg.verbose);

    if (uci_hw_interface_init(cfg.device_path) != 0 || !uci_hw_interface_is_connected()) {
        fprintf(stderr, "FAILED: unable to connect to hardware device %s\n", cfg.device_path);
        return 1;
    }

    printf("[PASS] hardware_connect\n");

    if (send_command_expect_response(CORE,
                                     CORE_DEVICE_INFO,
                                     NULL,
                                     0,
                                     cfg.timeout_ms,
                                     &packet) != 0 ||
        validate_response_basics(&packet,
                                 CORE,
                                 CORE_DEVICE_INFO,
                                 "CORE_DEVICE_INFO",
                                 1) != 0) {
        failures++;
    } else {
        printf("[PASS] core_device_info\n");
    }

    if (send_command_expect_response(CORE,
                                     CORE_GET_CAPS_INFO,
                                     NULL,
                                     0,
                                     cfg.timeout_ms,
                                     &packet) != 0 ||
        validate_response_basics(&packet,
                                 CORE,
                                 CORE_GET_CAPS_INFO,
                                 "CORE_GET_CAPS_INFO",
                                 1) != 0) {
        failures++;
    } else {
        printf("[PASS] core_get_caps_info\n");
    }

    if (send_command_expect_response(CORE,
                                     CORE_GET_CONFIG,
                                     get_config_payload,
                                     (int)sizeof(get_config_payload),
                                     cfg.timeout_ms,
                                     &packet) != 0 ||
        validate_response_basics(&packet,
                                 CORE,
                                 CORE_GET_CONFIG,
                                 "CORE_GET_CONFIG(device_state)",
                                 1) != 0) {
        failures++;
    } else {
        printf("[PASS] core_get_config_device_state\n");
    }

    if (cfg.include_reset) {
        unsigned char reset_payload[1] = { UWBS_RESET };
        if (send_command_expect_response(CORE,
                                         CORE_DEVICE_RESET,
                                         reset_payload,
                                         (int)sizeof(reset_payload),
                                         cfg.timeout_ms,
                                         &packet) != 0 ||
            validate_response_basics(&packet,
                                     CORE,
                                     CORE_DEVICE_RESET,
                                     "CORE_DEVICE_RESET",
                                     1) != 0) {
            failures++;
        } else {
            printf("[PASS] core_device_reset\n");
        }
    } else {
        printf("[SKIP] core_device_reset (set UCI_HW_INCLUDE_RESET=1 to enable)\n");
    }

    uci_hw_interface_cleanup();

    if (failures == 0) {
        printf("RESULT: ALL HARDWARE TESTS PASSED\n");
        return 0;
    }

    printf("RESULT: %d HARDWARE TEST(S) FAILED\n", failures);
    return 1;
}
