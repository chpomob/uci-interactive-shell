#include "test_runner.h"

#include "../include/uci.h"
#include "../include/uci_packet_analyzer.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_ui.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned char* packet;
    size_t packet_len;
    int saved_color;
} analyzer_capture_context_t;

typedef struct {
    const char* name;
    unsigned char mt;
    unsigned char gid;
    unsigned char oid;
    const unsigned char* payload;
    size_t payload_len;
    const char* expected_marker;
    const char* forbidden_marker;
} analyzer_dispatch_case_t;

static analyzer_capture_context_t g_capture_ctx;

static char* read_text_file(const char* path) {
    FILE* file = fopen(path, "rb");
    long size;
    char* buffer;

    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

static size_t count_substring(const char* text, const char* needle) {
    size_t count = 0;
    const char* cursor = text;
    size_t needle_len = strlen(needle);

    if (!text || !needle || needle_len == 0) {
        return 0;
    }

    while ((cursor = strstr(cursor, needle)) != NULL) {
        count++;
        cursor += needle_len;
    }

    return count;
}

static size_t capture_stdout(void (*fn)(void), char* buffer, size_t buffer_len) {
    FILE* tmp = tmpfile();
    int tmp_fd;
    int saved_stdout;
    long size;
    size_t read_bytes;

    if (!tmp || !buffer || buffer_len == 0) {
        if (tmp) {
            fclose(tmp);
        }
        return 0;
    }

    tmp_fd = fileno(tmp);
    saved_stdout = dup(STDOUT_FILENO);
    if (tmp_fd < 0 || saved_stdout < 0) {
        if (saved_stdout >= 0) {
            close(saved_stdout);
        }
        fclose(tmp);
        return 0;
    }

    fflush(stdout);
    if (dup2(tmp_fd, STDOUT_FILENO) < 0) {
        close(saved_stdout);
        fclose(tmp);
        return 0;
    }

    fn();

    fflush(stdout);
    fflush(tmp);
    size = ftell(tmp);
    if (size < 0) {
        size = 0;
    }
    rewind(tmp);

    read_bytes = (size_t)((size < (long)(buffer_len - 1)) ? size : (long)(buffer_len - 1));
    read_bytes = fread(buffer, 1, read_bytes, tmp);
    buffer[read_bytes] = '\0';

    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    fclose(tmp);
    return read_bytes;
}

static void emit_captured_packet(void) {
    ui_color_enabled = 0;
    uci_analyze_packet_core(g_capture_ctx.packet, g_capture_ctx.packet_len);
    ui_color_enabled = g_capture_ctx.saved_color;
}

static int run_dispatch_case(const analyzer_dispatch_case_t* test_case,
                             char* output,
                             size_t output_len) {
    size_t packet_len = 0;
    unsigned char* packet = create_uci_packet(test_case->mt,
                                              COMPLETE,
                                              test_case->gid,
                                              test_case->oid,
                                              test_case->payload,
                                              test_case->payload_len,
                                              &packet_len);
    if (!packet) {
        return -1;
    }

    g_capture_ctx.packet = packet;
    g_capture_ctx.packet_len = packet_len;
    g_capture_ctx.saved_color = ui_color_enabled;

    size_t captured = capture_stdout(emit_captured_packet, output, output_len);

    g_capture_ctx.packet = NULL;
    g_capture_ctx.packet_len = 0;
    free(packet);

    return captured > 0 ? 0 : -1;
}

static int run_raw_packet_case(const unsigned char* packet,
                               size_t packet_len,
                               char* output,
                               size_t output_len) {
    if (!packet || packet_len == 0) {
        return -1;
    }

    g_capture_ctx.packet = (unsigned char*)packet;
    g_capture_ctx.packet_len = packet_len;
    g_capture_ctx.saved_color = ui_color_enabled;

    size_t captured = capture_stdout(emit_captured_packet, output, output_len);

    g_capture_ctx.packet = NULL;
    g_capture_ctx.packet_len = 0;

    return captured > 0 ? 0 : -1;
}

int main(void) {
    TEST_SUITE(analyzer_dispatch);

#define test_case_end test_case_end_table_driven_dispatch
    TEST_CASE(table_driven_dispatch_routes_representative_packets);
    {
        static const unsigned char k_core_device_info_rsp[] = {
            UCI_STATUS_OK,
            0x00, 0x01,
            0x00, 0x02,
            0x00, 0x02,
            0x00, 0x01,
            0x00
        };
        static const unsigned char k_core_device_status_ntf[] = {
            DEVICE_STATE_READY
        };
        static const unsigned char k_session_init_rsp[] = {
            UCI_STATUS_OK, 0x01, 0x00, 0x00, 0x00
        };
        static const unsigned char k_session_status_ntf[] = {
            0x01, 0x00, 0x00, 0x00,
            SESSION_STATE_INIT,
            STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS
        };
        static const unsigned char k_session_start_rsp[] = {
            UCI_STATUS_OK
        };
        static const unsigned char k_session_info_ntf[] = {
            0x00, 0x00, 0x00, 0x00,
            0x02, 0x03, 0x04, 0x05,
            0x06,
            0x07, 0x08, 0x00, 0x00,
            0x01,
            0x00,
            0x01,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00
        };
        static const unsigned char k_session_start_cmd[] = {
            0x78, 0x56, 0x34, 0x12
        };
        static const analyzer_dispatch_case_t k_cases[] = {
            {
                "core_device_info_rsp",
                RESPONSE,
                CORE,
                CORE_DEVICE_INFO,
                k_core_device_info_rsp,
                sizeof(k_core_device_info_rsp),
                "CORE_DEVICE_INFO Response",
                NULL
            },
            {
                "core_device_status_ntf",
                NOTIFICATION,
                CORE,
                CORE_DEVICE_STATUS_NTF,
                k_core_device_status_ntf,
                sizeof(k_core_device_status_ntf),
                "CORE_DEVICE_STATUS_NTF:",
                NULL
            },
            {
                "session_init_rsp",
                RESPONSE,
                SESSION_CONFIG,
                SESSION_INIT,
                k_session_init_rsp,
                sizeof(k_session_init_rsp),
                "SESSION_INIT Response",
                NULL
            },
            {
                "session_status_ntf",
                NOTIFICATION,
                SESSION_CONFIG,
                SESSION_STATUS_NTF,
                k_session_status_ntf,
                sizeof(k_session_status_ntf),
                "SESSION_STATUS_NTF:",
                NULL
            },
            {
                "session_start_rsp",
                RESPONSE,
                SESSION_CONTROL,
                SESSION_START,
                k_session_start_rsp,
                sizeof(k_session_start_rsp),
                "SESSION_START Response",
                NULL
            },
            {
                "session_info_ntf",
                NOTIFICATION,
                SESSION_CONTROL,
                SESSION_INFO_NTF,
                k_session_info_ntf,
                sizeof(k_session_info_ntf),
                "SESSION_INFO_NTF - Standard FiRa Ranging Notification",
                "RANGE_DATA_NTF"
            },
            {
                "session_start_cmd",
                COMMAND,
                SESSION_CONTROL,
                SESSION_START,
                k_session_start_cmd,
                sizeof(k_session_start_cmd),
                "SESSION_START_CMD - Start Session Command",
                NULL
            },
        };
        char output[4096];

        for (size_t i = 0; i < sizeof(k_cases) / sizeof(k_cases[0]); i++) {
            if (run_dispatch_case(&k_cases[i], output, sizeof(output)) != 0) {
                TEST_FAIL(k_cases[i].name);
                goto test_case_end_table_driven_dispatch;
            }

            if (strstr(output, k_cases[i].expected_marker) == NULL) {
                TEST_FAIL(k_cases[i].name);
                goto test_case_end_table_driven_dispatch;
            }

            if (strstr(output, "No specific decoder") != NULL) {
                TEST_FAIL(k_cases[i].name);
                goto test_case_end_table_driven_dispatch;
            }

            if (k_cases[i].forbidden_marker &&
                strstr(output, k_cases[i].forbidden_marker) != NULL) {
                TEST_FAIL(k_cases[i].name);
                goto test_case_end_table_driven_dispatch;
            }
        }

        TEST_PASS();
    }
    test_case_end_table_driven_dispatch:;
#undef test_case_end

#define test_case_end test_case_end_table_driven_fallbacks
    TEST_CASE(table_driven_dispatch_reports_family_specific_fallbacks);
    {
        static const unsigned char k_minimal_payload[] = {
            0x00
        };
        static const analyzer_dispatch_case_t k_cases[] = {
            {
                "core_rsp_unknown_oid",
                RESPONSE,
                CORE,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for CORE_RESPONSE opcode 0x3F",
                "CORE_DEVICE_INFO Response"
            },
            {
                "core_ntf_unknown_oid",
                NOTIFICATION,
                CORE,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for CORE_NOTIFICATION opcode 0x3F",
                "CORE_DEVICE_STATUS_NTF:"
            },
            {
                "session_config_rsp_unknown_oid",
                RESPONSE,
                SESSION_CONFIG,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for SESSION_CONFIG_RESPONSE opcode 0x3F",
                "SESSION_INIT Response"
            },
            {
                "session_config_ntf_unknown_oid",
                NOTIFICATION,
                SESSION_CONFIG,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for SESSION_CONFIG_NOTIFICATION opcode 0x3F",
                "SESSION_STATUS_NTF:"
            },
            {
                "session_control_cmd_unknown_oid",
                COMMAND,
                SESSION_CONTROL,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for SESSION_CONTROL_COMMAND opcode 0x3F",
                "SESSION_START_CMD - Start Session Command"
            },
            {
                "session_control_rsp_unknown_oid",
                RESPONSE,
                SESSION_CONTROL,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for SESSION_CONTROL_RESPONSE opcode 0x3F",
                "SESSION_START Response"
            },
            {
                "session_control_ntf_unknown_oid",
                NOTIFICATION,
                SESSION_CONTROL,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for SESSION_CONTROL_NOTIFICATION opcode 0x3F",
                "SESSION_INFO_NTF - Standard FiRa Ranging Notification"
            },
            {
                "android_ntf_unknown_oid",
                NOTIFICATION,
                ANDROID,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for ANDROID_NOTIFICATION opcode 0x3F",
                NULL
            },
            {
                "android_rsp_generic_fallback",
                RESPONSE,
                ANDROID,
                0x3F,
                k_minimal_payload,
                sizeof(k_minimal_payload),
                "No specific decoder for MT=2, GID=12, OP=0x3F",
                NULL
            },
        };
        char output[4096];

        for (size_t i = 0; i < sizeof(k_cases) / sizeof(k_cases[0]); i++) {
            if (run_dispatch_case(&k_cases[i], output, sizeof(output)) != 0) {
                TEST_FAIL(k_cases[i].name);
                goto test_case_end_table_driven_fallbacks;
            }

            if (strstr(output, k_cases[i].expected_marker) == NULL) {
                TEST_FAIL(k_cases[i].name);
                goto test_case_end_table_driven_fallbacks;
            }

            if (k_cases[i].forbidden_marker &&
                strstr(output, k_cases[i].forbidden_marker) != NULL) {
                TEST_FAIL(k_cases[i].name);
                goto test_case_end_table_driven_fallbacks;
            }
        }

        TEST_PASS();
    }
    test_case_end_table_driven_fallbacks:;
#undef test_case_end

#define test_case_end test_case_end_dispatch_registration_audit
    TEST_CASE(dispatch_registration_routes_are_unique);
    {
        static const struct {
            const char* name;
            const char* signature;
        } k_signatures[] = {
            { "session_config_command_route", "if (mt == COMMAND && gid == SESSION_CONFIG) {" },
            { "session_control_command_route", "} else if (mt == COMMAND && gid == SESSION_CONTROL) {" },
            { "core_response_route", "} else if (mt == RESPONSE && gid == CORE) {" },
            { "core_notification_route", "} else if (mt == NOTIFICATION && gid == CORE) {" },
            { "session_config_response_route", "} else if (mt == RESPONSE && gid == SESSION_CONFIG) {" },
            { "session_config_notification_route", "} else if (mt == NOTIFICATION && gid == SESSION_CONFIG) {" },
            { "session_control_response_route", "} else if (mt == RESPONSE && gid == SESSION_CONTROL) {" },
            { "session_control_notification_route", "} else if (mt == NOTIFICATION && gid == SESSION_CONTROL) {" },
            { "android_notification_route", "} else if (mt == NOTIFICATION && gid == ANDROID) {" },
        };
        char* source = read_text_file("src/uci_packet_analyzer.c");

        if (!source) {
            TEST_FAIL("read_uci_packet_analyzer_source");
            goto test_case_end_dispatch_registration_audit;
        }

        for (size_t i = 0; i < sizeof(k_signatures) / sizeof(k_signatures[0]); i++) {
            if (count_substring(source, k_signatures[i].signature) != 1) {
                free(source);
                TEST_FAIL(k_signatures[i].name);
                goto test_case_end_dispatch_registration_audit;
            }
        }

        if (count_substring(source, "analyze_data_message_payload(gid, pbf, payload_ptr, payload_len_int);") != 1) {
            free(source);
            TEST_FAIL("data_dispatch_call_count");
            goto test_case_end_dispatch_registration_audit;
        }

        if (count_substring(source, "No specific decoder for MT=%d, GID=%d, OP=0x%02X") != 2) {
            free(source);
            TEST_FAIL("generic_fallback_message_count");
            goto test_case_end_dispatch_registration_audit;
        }

        if (count_substring(source, "case SESSION_INFO_NTF:") != 2) {
            free(source);
            TEST_FAIL("session_info_case_count");
            goto test_case_end_dispatch_registration_audit;
        }

        if (count_substring(source, "case SESSION_STATUS_NTF:") != 2) {
            free(source);
            TEST_FAIL("session_status_case_count");
            goto test_case_end_dispatch_registration_audit;
        }

        if (count_substring(source, "case SESSION_DATA_CREDIT_NTF:") != 2) {
            free(source);
            TEST_FAIL("session_data_credit_case_count");
            goto test_case_end_dispatch_registration_audit;
        }

        if (count_substring(source, "case SESSION_DATA_TRANSFER_STATUS_NTF:") != 2) {
            free(source);
            TEST_FAIL("session_data_transfer_status_case_count");
            goto test_case_end_dispatch_registration_audit;
        }

        free(source);
        TEST_PASS();
    }
    test_case_end_dispatch_registration_audit:;
#undef test_case_end

#define test_case_end test_case_end_malformed_packets
    TEST_CASE(malformed_packets_are_bounded_at_analyzer_entry);
    {
        char output[4096];
        static const unsigned char k_too_short_packet[] = {
            0x60, 0x01, 0x00
        };
        static const unsigned char k_zero_available_payload_packet[] = {
            0x60, 0x01, 0x00, 0x01
        };
        static const unsigned char k_truncated_session_status_packet[] = {
            0x61, 0x02, 0x00, 0x06,
            0x01, 0x00, 0x00, 0x00, 0x02
        };

        if (run_raw_packet_case(k_too_short_packet,
                                sizeof(k_too_short_packet),
                                output,
                                sizeof(output)) != 0) {
            TEST_FAIL("too_short_packet_capture");
            goto test_case_end_malformed_packets;
        }
        if (strstr(output, "Error: UCI packet too short to contain a header") == NULL) {
            TEST_FAIL("too_short_packet_message");
            goto test_case_end_malformed_packets;
        }

        if (run_raw_packet_case(k_zero_available_payload_packet,
                                sizeof(k_zero_available_payload_packet),
                                output,
                                sizeof(output)) != 0) {
            TEST_FAIL("zero_available_payload_capture");
            goto test_case_end_malformed_packets;
        }
        if (strstr(output, "Warning: Header payload length 1 exceeds available data 0. Clamping.") == NULL) {
            TEST_FAIL("zero_available_payload_warning");
            goto test_case_end_malformed_packets;
        }
        if (strstr(output, "CORE_DEVICE_STATUS_NTF:") != NULL) {
            TEST_FAIL("zero_available_payload_decoder_should_not_run");
            goto test_case_end_malformed_packets;
        }

        if (run_raw_packet_case(k_truncated_session_status_packet,
                                sizeof(k_truncated_session_status_packet),
                                output,
                                sizeof(output)) != 0) {
            TEST_FAIL("truncated_session_status_capture");
            goto test_case_end_malformed_packets;
        }
        if (strstr(output, "Warning: Header payload length 6 exceeds available data 5. Clamping.") == NULL) {
            TEST_FAIL("truncated_session_status_warning");
            goto test_case_end_malformed_packets;
        }
        if (strstr(output, "Error: Payload too short (5 bytes, need at least 6)") == NULL) {
            TEST_FAIL("truncated_session_status_decoder_length");
            goto test_case_end_malformed_packets;
        }
        if (strstr(output, "Session Token: 0x00000001") != NULL &&
            strstr(output, "Reason") != NULL) {
            TEST_FAIL("truncated_session_status_should_not_decode_full_fields");
            goto test_case_end_malformed_packets;
        }

        TEST_PASS();
    }
    test_case_end_malformed_packets:;
#undef test_case_end

    TEST_SUITE_END();
}
