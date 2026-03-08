#include "test_runner.h"
#include "../include/uci.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_cmd_session.h"
#include "../include/uci_functions.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci_packet_utils.h"
#include <stdlib.h>
#include <string.h>

typedef int (*parity_command_fn)(void);

typedef struct {
    int called;
    size_t packet_len;
    unsigned char packet[512 + sizeof(struct uci_packet_header)];
} captured_packet_t;

static captured_packet_t g_sim_packet;
static captured_packet_t g_hw_packet;

static void reset_captured_packet(captured_packet_t* packet) {
    memset(packet, 0, sizeof(*packet));
}

static int command_capture_hook(uci_uint8 mt,
                                uci_uint8 pbf,
                                uci_uint8 gid,
                                uci_uint8 oid,
                                const uci_uint8* payload,
                                int payload_len) {
    size_t packet_len = 0;
    unsigned char* packet = create_uci_packet(mt,
                                              pbf,
                                              gid,
                                              oid,
                                              payload,
                                              (payload_len > 0) ? (size_t)payload_len : 0,
                                              &packet_len);

    g_sim_packet.called++;
    g_sim_packet.packet_len = packet_len;
    if (packet && packet_len > 0) {
        size_t copy_len = packet_len;
        if (copy_len > sizeof(g_sim_packet.packet)) {
            copy_len = sizeof(g_sim_packet.packet);
        }
        memcpy(g_sim_packet.packet, packet, copy_len);
        free(packet);
    }

    return 0;
}

int uci_hw_chardev_init(uci_hw_chardev_t* hw, const char* device_path) {
    if (!hw || !device_path) {
        return -1;
    }

    memset(hw, 0, sizeof(*hw));
    strncpy(hw->device_path, device_path, sizeof(hw->device_path) - 1);
    hw->device_path[sizeof(hw->device_path) - 1] = '\0';
    hw->fd = 1;
    return 0;
}

int uci_hw_chardev_open(uci_hw_chardev_t* hw) {
    if (!hw) {
        return -1;
    }

    hw->is_open = 1;
    return 0;
}

int uci_hw_chardev_close(uci_hw_chardev_t* hw) {
    if (!hw) {
        return -1;
    }

    hw->is_open = 0;
    return 0;
}

int uci_hw_chardev_send(uci_hw_chardev_t* hw, const unsigned char* data, size_t length) {
    (void)hw;

    g_hw_packet.called++;
    g_hw_packet.packet_len = length;
    if (data && length > 0) {
        size_t copy_len = length;
        if (copy_len > sizeof(g_hw_packet.packet)) {
            copy_len = sizeof(g_hw_packet.packet);
        }
        memcpy(g_hw_packet.packet, data, copy_len);
    }

    return (int)length;
}

int uci_hw_chardev_receive(uci_hw_chardev_t* hw, unsigned char* buffer, size_t buffer_size, int timeout_ms) {
    (void)hw;
    (void)buffer;
    (void)buffer_size;
    (void)timeout_ms;
    return 0;
}

void uci_hw_chardev_set_verbose(uci_hw_chardev_t* hw, int verbose) {
    if (!hw) {
        return;
    }

    hw->verbose = verbose;
}

int uci_hw_chardev_is_connected(uci_hw_chardev_t* hw) {
    return hw && hw->is_open;
}

const char* uci_hw_chardev_get_device_path(uci_hw_chardev_t* hw) {
    return hw ? hw->device_path : "";
}

static int capture_simulation_packet(parity_command_fn fn) {
    reset_captured_packet(&g_sim_packet);
    uci_disable_hardware_mode();
    uci_set_command_capture_hook(command_capture_hook);
    return fn();
}

static int capture_hardware_packet(parity_command_fn fn) {
    reset_captured_packet(&g_hw_packet);
    uci_set_command_capture_hook(NULL);
    uci_enable_hardware_mode("/dev/fake-uwb");
    if (!uci_is_hardware_mode_enabled()) {
        return -1;
    }

    int rc = fn();
    uci_disable_hardware_mode();
    return rc;
}

static int packets_match(void) {
    if (g_sim_packet.called != 1 || g_hw_packet.called != 1) {
        return 0;
    }

    if (g_sim_packet.packet_len != g_hw_packet.packet_len) {
        return 0;
    }

    return memcmp(g_sim_packet.packet, g_hw_packet.packet, g_sim_packet.packet_len) == 0;
}

static int run_get_device_info_command(void) {
    handle_get_device_info_command();
    return 0;
}

static int run_device_reset_command(void) {
    handle_device_reset_command();
    return 0;
}

static int run_session_init_command(void) {
    return handle_session_init_command_values(0x12345678, FIRA_RANGING_SESSION);
}

static int run_session_start_command(void) {
    return handle_session_start_command_value(0x12345678);
}

static int run_get_config_command(void) {
    return handle_get_config_command("device_state");
}

int main(void) {
    TEST_SUITE(transport_parity);

    uci_set_command_capture_hook(NULL);

#define test_case_end test_case_end_core_device_info
    TEST_CASE(core_device_info_packet_matches_across_transports);
    {
        ASSERT_EQUAL(0, capture_simulation_packet(run_get_device_info_command));
        ASSERT_EQUAL(0, capture_hardware_packet(run_get_device_info_command));
        ASSERT_EQUAL(1, g_sim_packet.called);
        ASSERT_EQUAL(1, g_hw_packet.called);
        ASSERT_EQUAL((int)g_sim_packet.packet_len, (int)g_hw_packet.packet_len);
        ASSERT_TRUE(packets_match());

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_core_device_reset
    TEST_CASE(core_device_reset_packet_matches_across_transports);
    {
        ASSERT_EQUAL(0, capture_simulation_packet(run_device_reset_command));
        ASSERT_EQUAL(0, capture_hardware_packet(run_device_reset_command));
        ASSERT_EQUAL(1, g_sim_packet.called);
        ASSERT_EQUAL(1, g_hw_packet.called);
        ASSERT_EQUAL((int)g_sim_packet.packet_len, (int)g_hw_packet.packet_len);
        ASSERT_TRUE(packets_match());

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_session_init
    TEST_CASE(session_init_packet_matches_across_transports);
    {
        ASSERT_EQUAL(0, capture_simulation_packet(run_session_init_command));
        ASSERT_EQUAL(0, capture_hardware_packet(run_session_init_command));
        ASSERT_EQUAL(1, g_sim_packet.called);
        ASSERT_EQUAL(1, g_hw_packet.called);
        ASSERT_EQUAL((int)g_sim_packet.packet_len, (int)g_hw_packet.packet_len);
        ASSERT_TRUE(packets_match());

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_session_start
    TEST_CASE(session_start_packet_matches_across_transports);
    {
        ASSERT_EQUAL(0, capture_simulation_packet(run_session_start_command));
        ASSERT_EQUAL(0, capture_hardware_packet(run_session_start_command));
        ASSERT_EQUAL(1, g_sim_packet.called);
        ASSERT_EQUAL(1, g_hw_packet.called);
        ASSERT_EQUAL((int)g_sim_packet.packet_len, (int)g_hw_packet.packet_len);
        ASSERT_TRUE(packets_match());

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

#define test_case_end test_case_end_core_get_config
    TEST_CASE(core_get_config_packet_matches_across_transports);
    {
        ASSERT_EQUAL(0, capture_simulation_packet(run_get_config_command));
        ASSERT_EQUAL(0, capture_hardware_packet(run_get_config_command));
        ASSERT_EQUAL(1, g_sim_packet.called);
        ASSERT_EQUAL(1, g_hw_packet.called);
        ASSERT_EQUAL((int)g_sim_packet.packet_len, (int)g_hw_packet.packet_len);
        ASSERT_TRUE(packets_match());

        TEST_PASS();
    }
    test_case_end:;
#undef test_case_end

    uci_disable_hardware_mode();
    uci_set_command_capture_hook(NULL);

    TEST_SUITE_END();
}
