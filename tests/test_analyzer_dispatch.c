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
} analyzer_dispatch_case_t;

static analyzer_capture_context_t g_capture_ctx;

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
                "CORE_DEVICE_INFO Response"
            },
            {
                "core_device_status_ntf",
                NOTIFICATION,
                CORE,
                CORE_DEVICE_STATUS_NTF,
                k_core_device_status_ntf,
                sizeof(k_core_device_status_ntf),
                "CORE_DEVICE_STATUS_NTF:"
            },
            {
                "session_init_rsp",
                RESPONSE,
                SESSION_CONFIG,
                SESSION_INIT,
                k_session_init_rsp,
                sizeof(k_session_init_rsp),
                "SESSION_INIT Response"
            },
            {
                "session_status_ntf",
                NOTIFICATION,
                SESSION_CONFIG,
                SESSION_STATUS_NTF,
                k_session_status_ntf,
                sizeof(k_session_status_ntf),
                "SESSION_STATUS_NTF:"
            },
            {
                "session_start_rsp",
                RESPONSE,
                SESSION_CONTROL,
                SESSION_START,
                k_session_start_rsp,
                sizeof(k_session_start_rsp),
                "SESSION_START Response"
            },
            {
                "session_info_ntf",
                NOTIFICATION,
                SESSION_CONTROL,
                SESSION_INFO_NTF,
                k_session_info_ntf,
                sizeof(k_session_info_ntf),
                "SESSION_INFO_NTF - Standard FiRa Ranging Notification"
            },
            {
                "session_start_cmd",
                COMMAND,
                SESSION_CONTROL,
                SESSION_START,
                k_session_start_cmd,
                sizeof(k_session_start_cmd),
                "SESSION_START_CMD - Start Session Command"
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
        }

        TEST_PASS();
    }
    test_case_end_table_driven_dispatch:;
#undef test_case_end

    TEST_SUITE_END();
}
