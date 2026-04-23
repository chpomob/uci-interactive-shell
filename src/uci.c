#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_session_store.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_hw.h"
#include "../include/uci_hw_interface.h"
#include "../include/uci_hw_chardev.h"
#include "../include/uci_tcp_transport.h"
#include "../include/uci_ui.h"
#include "../include/uci_ui_main_patch.h"
#include "../include/uci_response_core.h"
#include "../include/uci_decode_utils.h"
#include "../include/uci_packet_analyzer.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_plain_decoders_internal.h"
#include "../include/uci_globals.h"
#include "../include/uci_standardized_error_handling.h"
#include "../include/uci_types.h"
#include <errno.h>

static uci_command_capture_hook_t g_command_capture_hook = NULL;
static uci_data_message_hook_t g_data_message_hook = NULL;

#define UCI_MAX_SEGMENTED_MESSAGE_PAYLOAD_SIZE 4096

typedef struct {
    int active;
    unsigned char first_header[sizeof(struct uci_packet_header)];
    uci_header_fields_t fields;
    size_t payload_len;
    unsigned char payload[UCI_MAX_SEGMENTED_MESSAGE_PAYLOAD_SIZE];
} uci_segmented_message_state_t;

static uci_segmented_message_state_t g_segmented_message_state = {0};

static void reset_segmented_message_state(void) {
    memset(&g_segmented_message_state, 0, sizeof(g_segmented_message_state));
}

void uci_reset_packet_parser_state(void) {
    reset_segmented_message_state();
}

static int segmented_message_matches(const uci_header_fields_t* fields) {
    return g_segmented_message_state.active &&
           fields &&
           g_segmented_message_state.fields.message_type == fields->message_type &&
           g_segmented_message_state.fields.group_id == fields->group_id &&
           g_segmented_message_state.fields.opcode_id == fields->opcode_id;
}
static uci_notification_callback_t g_notification_callback = NULL;

void uci_set_command_capture_hook(uci_command_capture_hook_t hook) {
    g_command_capture_hook = hook;
}

void uci_set_data_message_hook(uci_data_message_hook_t hook) {
    g_data_message_hook = hook;
}

void uci_set_notification_callback(uci_notification_callback_t callback) {
    g_notification_callback = callback;
}

int is_valid_device_config_id(unsigned char cfg_id) {
    return uci_config_get_device_param_name((DeviceConfigId)cfg_id) != NULL;
}

static const char* get_device_config_name(DeviceConfigId cfg_id);

#define MAX_RESPONSE_PAYLOAD_LEN 255
#define UCI_SIM_MAX_QUEUED_PACKETS 8
#define UCI_SIM_PACKET_STORAGE (sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN)

struct sim_packet_entry {
    size_t len;
    unsigned char data[UCI_SIM_PACKET_STORAGE];
};

static struct sim_packet_entry g_sim_packet_queue[UCI_SIM_MAX_QUEUED_PACKETS];
static size_t g_sim_queue_head = 0;
static size_t g_sim_queue_tail = 0;
static size_t g_sim_queue_count = 0;
static unsigned char g_sim_device_state = DEVICE_STATE_READY;

void enqueue_notification(unsigned char gid, unsigned char oid, const unsigned char* payload, size_t payload_len);
void uci_process_pending_notifications(void);
static void sim_handle_data_message_send(const unsigned char *payload, size_t payload_len);
static void log_outbound_fragment(const char *target_label,
                                  unsigned char mt,
                                  unsigned char pbf,
                                  unsigned char gid,
                                  unsigned char oid,
                                  const unsigned char *payload,
                                  size_t payload_len);
static void log_control_packet_bytes(const char *target_label,
                                     const unsigned char *packet,
                                     size_t packet_len,
                                     int empty_payload_as_text);

static void sim_queue_reset(void)
{
    g_sim_queue_head = 0;
    g_sim_queue_tail = 0;
    g_sim_queue_count = 0;
}

static void sim_queue_drop_oldest(void)
{
    if (g_sim_queue_count == 0)
        return;
    g_sim_queue_head = (g_sim_queue_head + 1) % UCI_SIM_MAX_QUEUED_PACKETS;
    g_sim_queue_count--;
}

static int sim_queue_enqueue(const unsigned char *packet, size_t len)
{
    if (len > UCI_SIM_PACKET_STORAGE)
        return -1;

    if (g_sim_queue_count == UCI_SIM_MAX_QUEUED_PACKETS) {
        sim_queue_drop_oldest();
    }

    struct sim_packet_entry *slot = &g_sim_packet_queue[g_sim_queue_tail];
    memcpy(slot->data, packet, len);
    slot->len = len;

    g_sim_queue_tail = (g_sim_queue_tail + 1) % UCI_SIM_MAX_QUEUED_PACKETS;
    g_sim_queue_count++;
    return 0;
}

static int sim_queue_dequeue(unsigned char *buffer, size_t buffer_size)
{
    if (g_sim_queue_count == 0)
        return 0;

    struct sim_packet_entry *slot = &g_sim_packet_queue[g_sim_queue_head];
    if (slot->len > buffer_size)
        return -1;

    memcpy(buffer, slot->data, slot->len);
    g_sim_queue_head = (g_sim_queue_head + 1) % UCI_SIM_MAX_QUEUED_PACKETS;
    g_sim_queue_count--;
    return (int)slot->len;
}

static void simulator_flush_queue(void)
{
    unsigned char packet[UCI_SIM_PACKET_STORAGE];
    int len;
    while ((len = sim_queue_dequeue(packet, sizeof(packet))) > 0) {
        parse_uci_packet(packet, (size_t)len);
    }
    uci_process_pending_notifications();
}

static void sim_update_device_state(unsigned char new_state, bool notify)
{
    if (g_sim_device_state == new_state)
        return;

    g_sim_device_state = new_state;
    if (notify) {
        enqueue_notification(CORE, CORE_DEVICE_STATUS_NTF, &g_sim_device_state, 1);
    }
}

static bool sim_command_allowed(uint8_t gid, uint8_t oid)
{
    if (g_sim_device_state != DEVICE_STATE_ERROR)
        return true;

    if (gid == CORE && oid == CORE_DEVICE_RESET)
        return true;

    return false;
}

static void log_outbound_fragment(const char *target_label,
                                  unsigned char mt,
                                  unsigned char pbf,
                                  unsigned char gid,
                                  unsigned char oid,
                                  const unsigned char *payload,
                                  size_t payload_len)
{
    (void)target_label;

    unsigned char packet[sizeof(struct uci_packet_header) + UCI_MAX_DATA_PACKET_PAYLOAD_SIZE];
    struct uci_packet_header *header = (struct uci_packet_header *)packet;
    if (set_header_values_safe(header, mt, pbf, gid, oid, (uci_uint16)payload_len) != UCI_SUCCESS) {
        printf("  Error: Failed to encode UCI header for outbound fragment.\n");
        return;
    }

    if (payload_len > 0 && payload) {
        memcpy(packet + sizeof(struct uci_packet_header), payload, payload_len);
    }

    unsigned char *header_bytes = (unsigned char *)header;
    printf("  Header: %02X %02X %02X %02X\n", header_bytes[0], header_bytes[1],
           header_bytes[2], header_bytes[3]);

    if (payload_len > 0 && payload) {
        printf("  Payload: ");
        for (size_t i = 0; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    } else {
        printf("  Payload: <empty>\n");
    }

    uci_analyze_packet_core(packet, sizeof(struct uci_packet_header) + payload_len);
}
static char g_hardware_device_path[256] = "/dev/ttyUSB0";  // Default device path (to be replaced by global approach)
unsigned long long g_fake_timestamp = 0;
static unsigned int g_session_handle_counter = 1;

#define MAX_PENDING_NOTIFICATIONS 16
typedef struct {
    unsigned char data[sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN];
    size_t length;
} notification_item_t;

static notification_item_t g_notification_queue[MAX_PENDING_NOTIFICATIONS];
static size_t g_notification_head = 0;
static size_t g_notification_tail = 0;

void enqueue_notification(unsigned char gid, unsigned char oid, const unsigned char* payload, size_t payload_len);

static void enqueue_session_status_notification(const struct uci_session* session, unsigned char new_state, unsigned char reason_code) {
    unsigned char payload[6];
    write_u32_le(payload, session->session_handle);
    payload[4] = new_state;
    payload[5] = reason_code;
    enqueue_notification(SESSION_CONFIG, SESSION_STATUS_NTF, payload, sizeof(payload));
}

static int notification_queue_empty() {
    return g_notification_head == g_notification_tail;
}

static int notification_queue_full() {
    return ((g_notification_tail + 1) % MAX_PENDING_NOTIFICATIONS) == g_notification_head;
}

void enqueue_notification(unsigned char gid, unsigned char oid, const unsigned char* payload, size_t payload_len) {
    if (payload_len > MAX_RESPONSE_PAYLOAD_LEN) {
        payload_len = MAX_RESPONSE_PAYLOAD_LEN;
    }

    if (notification_queue_full()) {
        printf("Notification queue full, dropping notification (GID=0x%02X OID=0x%02X)\n", gid, oid);
        return;
    }

    notification_item_t* item = &g_notification_queue[g_notification_tail];
    struct uci_packet_header* header = (struct uci_packet_header*)item->data;
    if (set_header_values_safe(header, NOTIFICATION, COMPLETE, gid, oid, (uci_uint16)payload_len) != UCI_SUCCESS) {
        return;
    }
    if (payload_len > 0 && payload) {
        memcpy(item->data + sizeof(struct uci_packet_header), payload, payload_len);
    }
    item->length = sizeof(struct uci_packet_header) + payload_len;
    g_notification_tail = (g_notification_tail + 1) % MAX_PENDING_NOTIFICATIONS;
}

void uci_process_pending_notifications() {
    while (!notification_queue_empty()) {
        notification_item_t* item = &g_notification_queue[g_notification_head];
        parse_uci_packet(item->data, item->length);
        g_notification_head = (g_notification_head + 1) % MAX_PENDING_NOTIFICATIONS;
    }
}

static void log_control_packet_bytes(const char *target_label,
                                     const unsigned char *packet,
                                     size_t packet_len,
                                     int empty_payload_as_text)
{
    const struct uci_packet_header *header = (const struct uci_packet_header *)packet;
    uci_header_fields_t fields;
    const unsigned char *payload;

    if (!packet || packet_len < sizeof(struct uci_packet_header)) {
        return;
    }

    uci_extract_header_fields_safe(header, &fields);
    payload = packet + sizeof(struct uci_packet_header);

    ui_print_sending_uci_packet(target_label);
    printf("  Header: %02X %02X %02X %02X\n", packet[0], packet[1], packet[2], packet[3]);

    if (fields.payload_length == 0 && empty_payload_as_text) {
        printf("  Payload: <empty>\n");
        return;
    }

    printf("  Payload: ");
    for (unsigned char i = 0; i < fields.payload_length; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");
}

// Function to analyze and display a UCI packet in human-readable format
void analyze_uci_packet(unsigned char* packet, size_t packet_len) {
    // Temporarily disable colors for plain text output
    int saved_color_setting = ui_color_enabled;
    ui_color_enabled = 0;

    // Call unified analyzer
    uci_analyze_packet_core(packet, packet_len);

    // Restore color setting
    ui_color_enabled = saved_color_setting;
}


// Function to enable hardware mode
void uci_enable_hardware_mode(const char* device_path) {
    extern int g_hardware_mode;
    extern uci_transport_mode_t g_transport_mode;

    if (!device_path || device_path[0] == '\0') {
        UCI_LOG_WARNING("Cannot enable hardware mode without a device path");
        g_hardware_mode = 0;
        g_transport_mode = UCI_TRANSPORT_MODE_SIMULATION;
        return;
    }

    uci_tcp_transport_disconnect();
    if (uci_hw_interface_init(device_path) == 0) {
        g_hardware_mode = 1;
        g_transport_mode = UCI_TRANSPORT_MODE_HARDWARE;
        printf("Hardware mode enabled with device: %s\n", device_path);
    } else {
        UCI_LOG_WARNING("Failed to initialize hardware device: %s", device_path);
        g_hardware_mode = 0;
        g_transport_mode = UCI_TRANSPORT_MODE_SIMULATION;
    }
}

// Function to disable hardware mode
void uci_disable_hardware_mode() {
    extern int g_hardware_mode;
    extern uci_transport_mode_t g_transport_mode;
    uci_hw_interface_cleanup();
    g_hardware_mode = 0;
    if (g_transport_mode == UCI_TRANSPORT_MODE_HARDWARE) {
        g_transport_mode = UCI_TRANSPORT_MODE_SIMULATION;
    }
    printf("Hardware mode disabled\n");
}

// Function to check if hardware mode is enabled
int uci_is_hardware_mode_enabled() {
    extern int g_hardware_mode;
    return g_hardware_mode;
}

int uci_enable_tcp_mode(const char* host, uci_uint16 port) {
    extern int g_hardware_mode;
    extern uci_transport_mode_t g_transport_mode;

    if (!host || host[0] == '\0' || port == 0) {
        return -1;
    }

    uci_hw_interface_cleanup();
    g_hardware_mode = 0;

    if (uci_tcp_transport_connect(host, port) != 0) {
        g_transport_mode = UCI_TRANSPORT_MODE_SIMULATION;
        return -1;
    }

    g_transport_mode = UCI_TRANSPORT_MODE_TCP;
    printf("TCP mode enabled with endpoint: %s\n", uci_get_tcp_endpoint());
    return 0;
}

void uci_disable_tcp_mode(void) {
    extern uci_transport_mode_t g_transport_mode;

    if (g_transport_mode == UCI_TRANSPORT_MODE_TCP) {
        uci_tcp_transport_disconnect();
        g_transport_mode = UCI_TRANSPORT_MODE_SIMULATION;
        printf("TCP mode disabled\n");
    } else {
        uci_tcp_transport_disconnect();
    }
}

int uci_is_tcp_mode_enabled(void) {
    extern uci_transport_mode_t g_transport_mode;
    return g_transport_mode == UCI_TRANSPORT_MODE_TCP;
}

const char* uci_get_tcp_endpoint(void) {
    const char* endpoint = uci_tcp_transport_get_endpoint();
    return (endpoint && endpoint[0] != '\0') ? endpoint : "127.0.0.1:9000";
}

uci_transport_mode_t uci_get_transport_mode(void) {
    extern uci_transport_mode_t g_transport_mode;
    return g_transport_mode;
}

int uci_is_external_transport_enabled(void) {
    uci_transport_mode_t mode = uci_get_transport_mode();
    return mode == UCI_TRANSPORT_MODE_HARDWARE || mode == UCI_TRANSPORT_MODE_TCP;
}

// Function to get the current hardware device path
const char* uci_get_hardware_device_path() {
    const char* path = uci_hw_interface_get_device_path();
    if (!path || path[0] == '\0') {
        return "/dev/ttyUSB0";
    }
    return path;
}

// Global session storage
struct uci_session uci_sessions[MAX_SESSIONS];

typedef int (*uci_sim_command_handler_fn)(unsigned char *response_payload,
                                          size_t max_len,
                                          const unsigned char *payload,
                                          size_t payload_len);

struct uci_command_handler_entry {
    uint8_t gid;
    uint8_t oid;
    uci_sim_command_handler_fn handler;
};

static int handle_core_device_info(unsigned char *response_payload, size_t max_len,
                                   const unsigned char *payload, size_t payload_len);
static int handle_core_get_caps_info(unsigned char *response_payload, size_t max_len,
                                     const unsigned char *payload, size_t payload_len);
static int handle_core_query_timestamp(unsigned char *response_payload, size_t max_len,
                                       const unsigned char *payload, size_t payload_len);
static int handle_core_get_config(unsigned char *response_payload, size_t max_len,
                                  const unsigned char *payload, size_t payload_len);
static int handle_core_set_config(unsigned char *response_payload, size_t max_len,
                                  const unsigned char *payload, size_t payload_len);
static int handle_core_device_reset(unsigned char *response_payload, size_t max_len,
                                    const unsigned char *payload, size_t payload_len);
static int handle_core_device_suspend(unsigned char *response_payload, size_t max_len,
                                      const unsigned char *payload, size_t payload_len);
static int handle_session_init(unsigned char *response_payload, size_t max_len,
                               const unsigned char *payload, size_t payload_len);
static int handle_session_deinit(unsigned char *response_payload, size_t max_len,
                                 const unsigned char *payload, size_t payload_len);
static int handle_session_start(unsigned char *response_payload, size_t max_len,
                                const unsigned char *payload, size_t payload_len);
static int handle_session_stop(unsigned char *response_payload, size_t max_len,
                               const unsigned char *payload, size_t payload_len);
static int handle_session_get_state(unsigned char *response_payload, size_t max_len,
                                    const unsigned char *payload, size_t payload_len);
static int handle_session_get_count(unsigned char *response_payload, size_t max_len,
                                    const unsigned char *payload, size_t payload_len);
static int handle_session_get_ranging_count(unsigned char *response_payload, size_t max_len,
                                            const unsigned char *payload, size_t payload_len);
static int handle_session_query_data_size_in_ranging(unsigned char *response_payload, size_t max_len,
                                                     const unsigned char *payload, size_t payload_len);
static int handle_session_update_controller_multicast_list(unsigned char *response_payload, size_t max_len,
                                                           const unsigned char *payload, size_t payload_len);
static int handle_session_update_active_rounds_dt_tag(unsigned char *response_payload, size_t max_len,
                                                      const unsigned char *payload, size_t payload_len);
static int handle_session_data_transfer_phase_config(unsigned char *response_payload, size_t max_len,
                                                     const unsigned char *payload, size_t payload_len);
static int handle_session_logical_link_create(unsigned char *response_payload, size_t max_len,
                                              const unsigned char *payload, size_t payload_len);
static int handle_session_logical_link_close(unsigned char *response_payload, size_t max_len,
                                             const unsigned char *payload, size_t payload_len);
static int handle_session_logical_link_get_param(unsigned char *response_payload, size_t max_len,
                                                 const unsigned char *payload, size_t payload_len);
static int handle_session_set_app_config(unsigned char *response_payload, size_t max_len,
                                         const unsigned char *payload, size_t payload_len);
static int handle_session_get_app_config(unsigned char *response_payload, size_t max_len,
                                         const unsigned char *payload, size_t payload_len);
static int handle_test_rf_set_config(unsigned char *response_payload, size_t max_len,
                                     const unsigned char *payload, size_t payload_len);
static int handle_test_rf_simple_status(unsigned char *response_payload, size_t max_len,
                                        const unsigned char *payload, size_t payload_len);
static int handle_vendor_android_get_power_stats(unsigned char *response_payload, size_t max_len,
                                                 const unsigned char *payload, size_t payload_len);
static int handle_vendor_android_set_country_code(unsigned char *response_payload, size_t max_len,
                                                  const unsigned char *payload, size_t payload_len);
static int handle_vendor_android_radar_set_app_config(unsigned char *response_payload, size_t max_len,
                                                      const unsigned char *payload, size_t payload_len);
static int handle_vendor_android_radar_get_app_config(unsigned char *response_payload, size_t max_len,
                                                      const unsigned char *payload, size_t payload_len);

static const struct uci_command_handler_entry k_sim_handlers[] = {
    {CORE, CORE_DEVICE_INFO, handle_core_device_info},
    {CORE, CORE_GET_CAPS_INFO, handle_core_get_caps_info},
    {CORE, CORE_QUERY_UWBS_TIMESTAMP, handle_core_query_timestamp},
    {CORE, CORE_GET_CONFIG, handle_core_get_config},
    {CORE, CORE_SET_CONFIG, handle_core_set_config},
    {CORE, CORE_DEVICE_RESET, handle_core_device_reset},
    {CORE, CORE_DEVICE_SUSPEND, handle_core_device_suspend},
    {SESSION_CONFIG, SESSION_INIT, handle_session_init},
    {SESSION_CONFIG, SESSION_DEINIT, handle_session_deinit},
    {SESSION_CONFIG, SESSION_GET_STATE, handle_session_get_state},
    {SESSION_CONFIG, SESSION_GET_COUNT, handle_session_get_count},
    {SESSION_CONFIG, SESSION_QUERY_DATA_SIZE_IN_RANGING, handle_session_query_data_size_in_ranging},
    {SESSION_CONFIG, SESSION_UPDATE_CONTROLLER_MULTICAST_LIST, handle_session_update_controller_multicast_list},
    {SESSION_CONFIG, SESSION_UPDATE_ACTIVE_ROUNDS_DT_TAG, handle_session_update_active_rounds_dt_tag},
    {SESSION_CONFIG, SESSION_DATA_TRANSFER_PHASE_CONFIG, handle_session_data_transfer_phase_config},
    {SESSION_CONFIG, SESSION_SET_APP_CONFIG, handle_session_set_app_config},
    {SESSION_CONFIG, SESSION_GET_APP_CONFIG, handle_session_get_app_config},
    {SESSION_CONTROL, SESSION_START, handle_session_start},
    {SESSION_CONTROL, SESSION_STOP, handle_session_stop},
    {SESSION_CONTROL, SESSION_GET_RANGING_COUNT, handle_session_get_ranging_count},
    {SESSION_CONTROL, SESSION_LOGICAL_LINK_CREATE, handle_session_logical_link_create},
    {SESSION_CONTROL, SESSION_LOGICAL_LINK_CLOSE, handle_session_logical_link_close},
    {SESSION_CONTROL, SESSION_LOGICAL_LINK_GET_PARAM, handle_session_logical_link_get_param},
    {TEST, TEST_RF_SET_CONFIG, handle_test_rf_set_config},
    {TEST, TEST_RF_PERIODIC_TX, handle_test_rf_simple_status},
    {TEST, TEST_RF_PER_RX, handle_test_rf_simple_status},
    {TEST, TEST_RF_RX, handle_test_rf_simple_status},
    {TEST, TEST_RF_STOP, handle_test_rf_simple_status},
    {ANDROID, ANDROID_GET_POWER_STATS, handle_vendor_android_get_power_stats},
    {ANDROID, ANDROID_SET_COUNTRY_CODE, handle_vendor_android_set_country_code},
    {ANDROID, ANDROID_RADAR_SET_APP_CONFIG, handle_vendor_android_radar_set_app_config},
    {ANDROID, ANDROID_RADAR_GET_APP_CONFIG, handle_vendor_android_radar_get_app_config},
};

static const size_t k_sim_handlers_count = sizeof(k_sim_handlers) / sizeof(k_sim_handlers[0]);

static const struct uci_command_handler_entry *find_sim_handler(uint8_t gid, uint8_t oid);
static bool sim_gid_is_known(uint8_t gid);
static void send_sim_status(uint8_t gid, uint8_t oid, uint8_t status);

static const struct uci_command_handler_entry *find_sim_handler(uint8_t gid, uint8_t oid) {
    for (size_t i = 0; i < k_sim_handlers_count; ++i) {
        if (k_sim_handlers[i].gid == gid && k_sim_handlers[i].oid == oid) {
            return &k_sim_handlers[i];
        }
    }
    return NULL;
}

static bool sim_gid_is_known(uint8_t gid) {
    for (size_t i = 0; i < k_sim_handlers_count; ++i) {
        if (k_sim_handlers[i].gid == gid) {
            return true;
        }
    }
    return false;
}

static void send_sim_status(uint8_t gid, uint8_t oid, uint8_t status) {
    unsigned char response_packet[sizeof(struct uci_packet_header) + 1];
    struct uci_packet_header *response_header = (struct uci_packet_header *)response_packet;

    (void)set_header_values_safe(response_header, RESPONSE, COMPLETE, gid, oid, 1);
    response_packet[sizeof(struct uci_packet_header)] = status;

    if (sim_queue_enqueue(response_packet, sizeof(struct uci_packet_header) + 1) == 0) {
        simulator_flush_queue();
    } else {
        parse_uci_packet(response_packet, sizeof(struct uci_packet_header) + 1);
    }
}

void send_uci_command(uci_uint8 mt, uci_uint8 pbf, uci_uint8 gid, uci_uint8 oid,
                      uci_uint8 *payload, int payload_len) {
    static int initialized = 0;
    uci_transport_mode_t transport_mode;
    if (!initialized) {
        init_uci_sessions();
        sim_queue_reset();
        initialized = 1;
    }

    size_t packet_len = 0;
    unsigned char *packet = NULL;
    const unsigned char *packet_payload = NULL;

    // Validate input parameters
    if (payload_len < 0 || payload_len > UCI_MAX_CONTROL_PAYLOAD_SIZE) {
        UCI_LOG_ERROR("Invalid payload length: %d", payload_len);
        uci_log_error(__func__, "Invalid payload length", UCI_ERROR_INVALID_PARAM);
        return;
    }

    if (g_command_capture_hook) {
        if (g_command_capture_hook(mt, pbf, gid, oid, payload, payload_len) == 0) {
            return;
        }
    }

    packet = create_uci_packet(mt,
                               pbf,
                               gid,
                               oid,
                               payload,
                               (payload_len > 0) ? (size_t)payload_len : 0,
                               &packet_len);
    if (!packet) {
        UCI_LOG_ERROR("Failed to construct UCI control packet", UCI_ERROR_OUT_OF_MEMORY);
        uci_log_error(__func__, "Packet construction failed", UCI_ERROR_OUT_OF_MEMORY);
        return;
    }

    packet_payload = packet + sizeof(struct uci_packet_header);
    transport_mode = uci_get_transport_mode();

    if (transport_mode == UCI_TRANSPORT_MODE_HARDWARE) {
        printf("[HARDWARE MODE] ");
        const char* hw_path = uci_get_hardware_device_path();
        log_control_packet_bytes(hw_path, packet, packet_len, 1);

        if (uci_hw_interface_send_packet(packet, packet_len) != 0) {
            ui_print_error("Failed to send command to hardware");
            UCI_LOG_ERROR("Hardware send failed", UCI_ERROR_INVALID_PARAM);
            free(packet);
            return;
        }

        printf("  -> Sent command to hardware device %s\n", hw_path);
        printf("  <- Waiting for response from hardware device...\n");
        if (uci_receive_hardware_packets(1000) == 0) {
            ui_print_warning("No response received from hardware (timeout)");
        }
        free(packet);
        return;
    }

    if (transport_mode == UCI_TRANSPORT_MODE_TCP) {
        const char* endpoint = uci_get_tcp_endpoint();
        printf("[TCP MODE] ");
        log_control_packet_bytes(endpoint, packet, packet_len, 1);

        if (uci_tcp_transport_send_packet(packet, packet_len) < 0) {
            ui_print_error("Failed to send command to TCP endpoint");
            UCI_LOG_ERROR("TCP send failed", UCI_ERROR_INVALID_PARAM);
            free(packet);
            return;
        }

        printf("  -> Sent command to TCP endpoint %s\n", endpoint);
        printf("  <- Waiting for response from TCP endpoint...\n");
        if (uci_receive_tcp_packets(1000) == 0) {
            ui_print_warning("No response received from TCP endpoint (timeout)");
        }
        free(packet);
        return;
    }

    // In simulation mode, continue with existing behavior
    log_control_packet_bytes("simulator", packet, packet_len, 0);

    printf("Simulating UCI response...\n");

    if (!sim_command_allowed(gid, oid)) {
        send_sim_status(gid, oid, UCI_STATUS_REJECTED);
        UCI_LOG_ERROR("Command not allowed", UCI_ERROR_INVALID_PARAM);
        free(packet);
        return;
    }

    const struct uci_command_handler_entry *handler = find_sim_handler(gid, oid);
    if (!handler) {
        uint8_t status = sim_gid_is_known(gid) ? UCI_STATUS_UNKNOWN_OID : UCI_STATUS_UNKNOWN_GID;
        send_sim_status(gid, oid, status);
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "Handler not found for GID: 0x%02X, OID: 0x%02X", gid, oid);
        UCI_LOG_ERROR(error_msg, UCI_ERROR_UNSUPPORTED_OPERATION);
        free(packet);
        return;
    }

    unsigned char response_packet[sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN];
    struct uci_packet_header *response_header = (struct uci_packet_header *)response_packet;
    unsigned char *response_payload = response_packet + sizeof(struct uci_packet_header);

    int generated_len = handler->handler(response_payload,
                                         MAX_RESPONSE_PAYLOAD_LEN,
                                         packet_payload,
                                         (size_t)payload_len);
    if (generated_len < 0) {
        send_sim_status(gid, oid, UCI_STATUS_FAILED);
        UCI_LOG_ERROR("Handler function failed", UCI_ERROR_INVALID_PARAM);
        free(packet);
        return;
    }

    if ((size_t)generated_len > MAX_RESPONSE_PAYLOAD_LEN) {
        send_sim_status(gid, oid, UCI_STATUS_INVALID_MSG_SIZE);
        UCI_LOG_ERROR("Response payload too large", UCI_ERROR_BUFFER_OVERFLOW);
        free(packet);
        return;
    }

    uci_error_t result = set_header_values_safe(response_header, RESPONSE, COMPLETE, gid, oid,
                                                (uci_uint16)generated_len);
    if (result != UCI_SUCCESS) {
        UCI_LOG_ERROR("Failed to set response header values (error=%d)", result);
        uci_log_error(__func__, "Response header validation failed", result);
        free(packet);
        return;
    }

    size_t total_len = sizeof(struct uci_packet_header) + (size_t)generated_len;
    if (sim_queue_enqueue(response_packet, total_len) == 0) {
        simulator_flush_queue();
    } else {
        parse_uci_packet(response_packet, total_len);
    }

    free(packet);
}

static void sim_handle_data_message_send(const unsigned char *payload, size_t payload_len)
{
    if (!payload || payload_len < UCI_DATA_MESSAGE_SND_HEADER) {
        UCI_LOG_ERROR("DATA_MESSAGE_SND payload too short (len=%zu)", payload_len);
        uci_log_error(__func__, "DATA_MESSAGE_SND payload too short", UCI_ERROR_INVALID_PARAM);
        return;
    }

    if (g_data_message_hook) {
        if (g_data_message_hook(payload, payload_len) == 0) {
            return;
        }
    }

    uint32_t payload_identifier = read_u32_le(payload);
    uint64_t destination = read_u64_le(payload + 4);
    uint16_t sequence_number = read_u16_le(payload + 12);
    uint16_t declared_length = read_u16_le(payload + 14);

    size_t available = payload_len - UCI_DATA_MESSAGE_SND_HEADER;
    if (declared_length > available) {
        declared_length = (uint16_t)available;
    }

    const unsigned char *app_data = payload + UCI_DATA_MESSAGE_SND_HEADER;

    int session_idx = find_session_by_token_or_id(payload_identifier);
    struct uci_session *session = (session_idx >= 0) ? &uci_sessions[session_idx] : NULL;

    uint8_t transfer_status = session ? UCI_DATA_TRANSFER_STATUS_OK
                                      : UCI_DATA_TRANSFER_STATUS_ERROR_REJECTED;
    uint32_t notification_session = session ? session->session_handle : payload_identifier;

    if (session) {
        session->last_data_sequence = sequence_number;
        session->last_data_length = declared_length;
        session->last_data_destination = destination;
        if (declared_length > 0) {
            size_t preview_len = declared_length;
            if (preview_len > sizeof(session->last_data_preview)) {
                preview_len = sizeof(session->last_data_preview);
            }
            memcpy(session->last_data_preview, app_data, preview_len);
            session->last_data_preview_len = (unsigned char)preview_len;
        } else {
            session->last_data_preview_len = 0;
        }
    }

    if (session) {
        unsigned char credit_payload[5];
        write_u32_le(credit_payload, session->session_handle);
        credit_payload[4] = 1; // credit available again
        enqueue_notification(SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, credit_payload,
                             sizeof(credit_payload));
    }

    unsigned char status_payload[8];
    write_u32_le(status_payload, notification_session);
    write_u16_le(&status_payload[4], sequence_number);
    status_payload[6] = transfer_status;
    status_payload[7] = 1; // tx count
    enqueue_notification(SESSION_CONTROL, SESSION_DATA_TRANSFER_STATUS_NTF, status_payload,
                         sizeof(status_payload));
}

void uci_send_data_message(uint32_t identifier,
                           uint64_t destination_address,
                           uint16_t sequence_number,
                           const unsigned char *app_data,
                           size_t app_data_len)
{
    // Validate input parameters
    if (app_data_len > UCI_MAX_APPLICATION_DATA_PAYLOAD_SIZE) {
        UCI_LOG_ERROR("Application data length too large", UCI_ERROR_INVALID_PARAM);
        return;
    }
    
    size_t payload_capacity = UCI_DATA_MESSAGE_SND_HEADER + app_data_len;
    
    // Check for potential integer overflow
    if (app_data_len > SIZE_MAX - UCI_DATA_MESSAGE_SND_HEADER || payload_capacity < UCI_DATA_MESSAGE_SND_HEADER) {
        UCI_LOG_ERROR("Integer overflow in payload capacity calculation", UCI_ERROR_INVALID_PARAM);
        return;
    }

    unsigned char *payload = safe_malloc(payload_capacity);
    if (!payload) {
        UCI_LOG_ERROR("Failed to allocate DATA_MESSAGE_SND payload buffer (requested %zu bytes)",
                      payload_capacity);
        uci_log_error(__func__, "Memory allocation failure for payload", UCI_ERROR_OUT_OF_MEMORY);
        return;
    }

    int session_idx = find_session_by_token_or_id(identifier);
    uint32_t session_identifier = identifier;
    if (session_idx >= 0 && uci_sessions[session_idx].session_handle != 0) {
        session_identifier = uci_sessions[session_idx].session_handle;
    }

    size_t payload_len =
        uci_build_data_message_snd_payload(payload, payload_capacity, session_identifier,
                                           destination_address, sequence_number,
                                           app_data, app_data_len);
    if (payload_len == 0) {
        UCI_LOG_ERROR("Failed to compose DATA_MESSAGE_SND payload");
        uci_log_error(__func__, "Failed to build data message payload", UCI_ERROR_INVALID_PARAM);
        free(payload);
        return;
    }

    const char *target = "simulator";
    if (uci_get_transport_mode() == UCI_TRANSPORT_MODE_HARDWARE) {
        target = g_hardware_device_path;
    } else if (uci_get_transport_mode() == UCI_TRANSPORT_MODE_TCP) {
        target = uci_get_tcp_endpoint();
    }
    ui_print_sending_uci_packet(target);

    size_t remaining = payload_len;
    size_t offset = 0;

    while (remaining > 0) {
        size_t chunk = remaining;
        if (chunk > UCI_MAX_DATA_PACKET_PAYLOAD_SIZE) {
            chunk = UCI_MAX_DATA_PACKET_PAYLOAD_SIZE;
        }
        unsigned char fragment_pbf = ((offset + chunk) < payload_len) ? NOT_COMPLETE : COMPLETE;

        log_outbound_fragment(target, DATA, fragment_pbf, DATA_PACKET_FORMAT_SEND, 0x00,
                              payload + offset, chunk);

        if (uci_get_transport_mode() == UCI_TRANSPORT_MODE_TCP) {
            size_t packet_len = 0;
            unsigned char *packet =
                create_uci_packet(DATA, fragment_pbf, DATA_PACKET_FORMAT_SEND, 0x00,
                                  payload + offset, chunk, &packet_len);
            if (!packet) {
                UCI_LOG_ERROR("Failed to create DATA packet for TCP transport");
                uci_log_error(__func__, "Failed to build TCP data packet", UCI_ERROR_INVALID_PARAM);
                free(payload);
                return;
            }
            if (uci_tcp_transport_send_packet(packet, packet_len) < 0) {
                ui_print_error("Failed to send data message to TCP endpoint");
                UCI_LOG_ERROR("TCP data send failed", UCI_ERROR_INVALID_PARAM);
                free(packet);
                free(payload);
                return;
            }
            free(packet);
        }

        offset += chunk;
        remaining -= chunk;
    }

    if (uci_get_transport_mode() == UCI_TRANSPORT_MODE_HARDWARE) {
        printf("  -> Would send to hardware device %s\n", g_hardware_device_path);
    } else if (uci_get_transport_mode() == UCI_TRANSPORT_MODE_TCP) {
        printf("  -> Sent data message to TCP endpoint %s\n", target);
        printf("  <- Waiting for notifications from TCP endpoint...\n");
        if (uci_receive_tcp_packets(1000) == 0) {
            ui_print_warning("No notifications received from TCP endpoint (timeout)");
        }
        free(payload);
        return;
    }

    sim_handle_data_message_send(payload, payload_len);
    uci_process_pending_notifications();

    free(payload);
}

int uci_receive_hardware_packets(int timeout_ms) {
    unsigned char response_buffer[1024];
    int received_packets = 0;

    while (1) {
        int response_len = uci_hw_interface_receive_response(response_buffer,
                                                             sizeof(response_buffer),
                                                             timeout_ms);
        if (response_len < 0) {
            return (received_packets > 0) ? received_packets : -1;
        }
        if (response_len == 0) {
            return received_packets;
        }

        printf("Received %d bytes from hardware:\n", response_len);
        printf("  ");
        for (int i = 0; i < response_len; i++) {
            printf("%02X ", response_buffer[i]);
        }
        printf("\n");
        parse_uci_packet(response_buffer, (size_t)response_len);
        received_packets++;
    }
}

int uci_receive_tcp_packets(int timeout_ms) {
    unsigned char response_buffer[1024];
    int received_packets = 0;

    while (1) {
        int response_len = uci_tcp_transport_receive_packet(response_buffer,
                                                            sizeof(response_buffer),
                                                            timeout_ms);
        if (response_len < 0) {
            return (received_packets > 0) ? received_packets : -1;
        }
        if (response_len == 0) {
            return received_packets;
        }

        printf("Received %d bytes from TCP endpoint:\n", response_len);
        printf("  ");
        for (int i = 0; i < response_len; i++) {
            printf("%02X ", response_buffer[i]);
        }
        printf("\n");
        parse_uci_packet(response_buffer, (size_t)response_len);
        received_packets++;
        timeout_ms = 100;
    }
}

static int handle_core_device_info(unsigned char *response_payload, size_t max_len,
                                   const unsigned char *payload, size_t payload_len) {
    (void)payload;
    (void)payload_len;
    
    if (!response_payload || max_len < 1) {
        UCI_LOG_ERROR("Invalid parameters in CORE_DEVICE_INFO handler");
        uci_log_error(__func__, "Invalid parameters", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    if (payload_len != 0) {
        UCI_LOG_WARNING("CORE_DEVICE_INFO command has unexpected payload of %zu bytes", payload_len);
        // This is just a warning, we still process the command
    }
    
    int result = build_core_device_info_response(response_payload, max_len);
    
    if (result < 0) {
        UCI_LOG_ERROR("Failed to build CORE_DEVICE_INFO response");
        uci_log_error(__func__, "Failed to build response", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    return result;
}

static int handle_core_get_caps_info(unsigned char *response_payload, size_t max_len,
                                     const unsigned char *payload, size_t payload_len) {
    (void)payload;
    (void)payload_len;
    
    if (!response_payload || max_len < 2) {  // At least status + num_tlvs
        UCI_LOG_ERROR("Invalid parameters in CORE_GET_CAPS_INFO handler");
        uci_log_error(__func__, "Invalid parameters", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    if (payload_len != 0) {
        UCI_LOG_WARNING("CORE_GET_CAPS_INFO command has unexpected payload of %zu bytes", payload_len);
        // This is just a warning, we still process the command
    }
    
    int result = build_core_get_caps_info_response(response_payload, max_len);
    
    if (result < 0) {
        UCI_LOG_ERROR("Failed to build CORE_GET_CAPS_INFO response");
        uci_log_error(__func__, "Failed to build response", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    return result;
}

static int handle_core_query_timestamp(unsigned char *response_payload, size_t max_len,
                                       const unsigned char *payload, size_t payload_len) {
    (void)payload;
    (void)payload_len;
    
    if (!response_payload || max_len < 10) {  // At least status + 8-byte timestamp
        UCI_LOG_ERROR("Invalid parameters in CORE_QUERY_UWBS_TIMESTAMP handler");
        uci_log_error(__func__, "Invalid parameters", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    if (payload_len != 0) {
        UCI_LOG_WARNING("CORE_QUERY_UWBS_TIMESTAMP command has unexpected payload of %zu bytes", payload_len);
        // This is just a warning, we still process the command
    }
    
    int result = build_core_query_timestamp_response(response_payload, max_len);
    
    if (result < 0) {
        UCI_LOG_ERROR("Failed to build CORE_QUERY_UWBS_TIMESTAMP response");
        uci_log_error(__func__, "Failed to build response", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    return result;
}

static int handle_core_get_config(unsigned char *response_payload, size_t max_len,
                                  const unsigned char *payload, size_t payload_len) {
    if (!response_payload || !payload || max_len < 2) {  // At least status + num_tlvs
        UCI_LOG_ERROR("Invalid parameters in CORE_GET_CONFIG handler");
        uci_log_error(__func__, "Invalid parameters", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    if (payload_len == 0) {
        UCI_LOG_WARNING("CORE_GET_CONFIG called without parameters - will return all configs");
    } else if (payload_len < 1) {
        UCI_LOG_ERROR("CORE_GET_CONFIG command has insufficient payload (need at least 1 byte for config ID)");
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        response_payload[1] = 0;
        return 2;
    }
    
    // Validate the number of config IDs provided in the payload
    if (payload_len > MAX_RESPONSE_PAYLOAD_LEN - 2) {  // Leave space for status and count
        UCI_LOG_ERROR("CORE_GET_CONFIG command has too many config IDs requested");
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        response_payload[1] = 0;
        return 2;
    }
    
    int result = build_core_get_config_response(response_payload, max_len, payload,
                                                (int)payload_len);
    
    if (result < 0) {
        UCI_LOG_ERROR("Failed to build CORE_GET_CONFIG response");
        uci_log_error(__func__, "Failed to build response", UCI_ERROR_INVALID_PARAM);
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        response_payload[1] = 0;
        return 2;
    }
    
    return result;
}

static int handle_core_set_config(unsigned char *response_payload, size_t max_len,
                                  const unsigned char *payload, size_t payload_len) {
    if (!response_payload || !payload || max_len < 1) {
        UCI_LOG_ERROR("Invalid parameters in CORE_SET_CONFIG handler");
        uci_log_error(__func__, "Invalid parameters", UCI_ERROR_INVALID_PARAM);
        return -1;
    }
    
    if (payload_len < 1) {
        UCI_LOG_ERROR("CORE_SET_CONFIG command has insufficient payload (need at least 1 byte for declared count)");
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }
    
    // Validate the declared count against reasonable limits
    unsigned char declared = payload[0];
    if (declared > MAX_SESSION_CONFIGS) {  // Using MAX_SESSION_CONFIGS as an upper bound for validation
        UCI_LOG_ERROR("CORE_SET_CONFIG command declares too many configs (%d, max %d)",
                      declared, MAX_SESSION_CONFIGS);
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }
    
    if (payload_len < (size_t)(1 + declared)) {  // Need at least 1 byte for declared count + IDs
        UCI_LOG_ERROR("CORE_SET_CONFIG command has insufficient payload (need at least %zu bytes for %d config IDs)", 
                      (size_t)(1 + declared), declared);
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }
    
    int result = build_core_set_config_response(response_payload, max_len, payload,
                                               (int)payload_len);

    if (result > 0 && response_payload[0] == UCI_STATUS_OK && payload && payload_len >= 1) {
        unsigned char declared = payload[0];
        const unsigned char *tlv_buffer = (payload_len > 1) ? &payload[1] : NULL;
        size_t tlv_length = (payload_len > 1) ? (size_t)(payload_len - 1) : 0;
        struct uci_tlv_reader reader;
        uci_tlv_reader_init(&reader, tlv_buffer, tlv_length);

        for (unsigned char i = 0; i < declared; i++) {
            unsigned char cfg_id = 0;
            unsigned char cfg_len = 0;
            const unsigned char *value_ptr = NULL;

            int res = uci_tlv_reader_next(&reader, &cfg_id, &value_ptr, &cfg_len);
            if (res <= 0) {
                break;
            }

            // Validate that the config ID is supported
            if (!is_valid_device_config_id(cfg_id)) {
                UCI_LOG_WARNING("CORE_SET_CONFIG received request for unsupported config ID 0x%02X", cfg_id);
                continue;
            }

            if (cfg_id == DEVICE_STATE && cfg_len >= 1) {
                sim_update_device_state(value_ptr[0], true);
            }
        }
    }

    return result;
}

static int handle_core_device_reset(unsigned char *response_payload, size_t max_len,
                                    const unsigned char *payload, size_t payload_len) {
    (void)payload;
    (void)payload_len;
    int len = build_core_device_reset_response(response_payload, max_len);
    init_uci_sessions();
    sim_update_device_state(DEVICE_STATE_READY, true);
    return len;
}

static int handle_core_device_suspend(unsigned char *response_payload, size_t max_len,
                                      const unsigned char *payload, size_t payload_len) {
    (void)payload;
    (void)payload_len;
    return build_core_device_suspend_response(response_payload, max_len);
}

static int handle_session_init(unsigned char *response_payload, size_t max_len,
                               const unsigned char *payload, size_t payload_len) {
    if (!response_payload || !payload || max_len < 5) {
        UCI_LOG_ERROR("Invalid parameters in SESSION_INIT handler");
        uci_log_error(__func__, "Invalid parameters", UCI_ERROR_INVALID_PARAM);
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }

    if (payload_len < 5) {
        UCI_LOG_ERROR("SESSION_INIT command has insufficient payload (need at least 5 bytes)");
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }

    unsigned int session_id = read_u32_le(payload);
    SessionType session_type = (SessionType)payload[4];

    // Validate session type is valid
    if (session_type > 0x05) {
        UCI_LOG_ERROR("Invalid session_type %d in SESSION_INIT command", session_type);
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }

    int session_idx = find_free_session_slot();
    if (session_idx < 0) {
        UCI_LOG_ERROR("No free session slots available for SESSION_INIT");
        response_payload[0] = UCI_STATUS_MAX_SESSIONS_EXCEEDED;
        return 1;
    }

    unsigned int session_handle = g_session_handle_counter++;
    if (session_handle == 0) {
        session_handle = g_session_handle_counter++;
    }

    struct uci_session *session = &uci_sessions[session_idx];
    session->session_id = session_id;
    session->session_type = session_type;
    session->session_state = SESSION_STATE_INIT;
    session->is_allocated = 1;
    session->session_handle = session_handle;
    session->ranging_count = 0;
    session->num_configs = 0;
    session->multicast_count = 0;
    session->dt_tag_round_count = 0;
    session->dtp_repetition = 0;
    session->dtp_control = 0;
    session->dtp_size = 0;
    session->dtp_payload_len = 0;

    memset(session->configs, 0, sizeof(session->configs));
    memset(session->multicast_entries, 0, sizeof(session->multicast_entries));
    memset(session->dt_tag_round_indexes, 0, sizeof(session->dt_tag_round_indexes));
    memset(session->dtp_payload, 0, sizeof(session->dtp_payload));
    uci_session_clear_logical_links(session);
    session->last_data_sequence = 0;
    session->last_data_length = 0;
    session->last_data_destination = 0;
    session->last_data_preview_len = 0;
    memset(session->last_data_preview, 0, sizeof(session->last_data_preview));

    response_payload[0] = UCI_STATUS_OK;
    write_u32_le(&response_payload[1], session_handle);

    enqueue_session_status_notification(session, SESSION_STATE_INIT,
                                        STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);

    return 5;
}

static int handle_session_deinit(unsigned char *response_payload, size_t max_len,
                                 const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    if (!payload || payload_len < 4) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }

    unsigned int identifier = read_u32_le(payload);
    int session_idx = find_session_by_token_or_id(identifier);
    if (session_idx >= 0) {
        struct uci_session *session = &uci_sessions[session_idx];
        enqueue_session_status_notification(session, SESSION_STATE_DEINIT,
                                            STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);
        session->session_state = SESSION_STATE_DEINIT;
        session->is_allocated = 0;
        session->session_id = 0;
        session->session_type = 0;
        session->session_handle = 0;
        session->ranging_count = 0;
        session->num_configs = 0;
        session->multicast_count = 0;
        session->dt_tag_round_count = 0;
        session->dtp_repetition = 0;
        session->dtp_control = 0;
        session->dtp_size = 0;
        session->dtp_payload_len = 0;

        memset(session->configs, 0, sizeof(session->configs));
        memset(session->multicast_entries, 0, sizeof(session->multicast_entries));
        memset(session->dt_tag_round_indexes, 0, sizeof(session->dt_tag_round_indexes));
        memset(session->dtp_payload, 0, sizeof(session->dtp_payload));
        uci_session_clear_logical_links(session);
        session->last_data_sequence = 0;
        session->last_data_length = 0;
        session->last_data_destination = 0;
        session->last_data_preview_len = 0;
        memset(session->last_data_preview, 0, sizeof(session->last_data_preview));
    } else {
        unsigned char err = UCI_STATUS_INVALID_PARAM;
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    response_payload[0] = UCI_STATUS_OK;
    return 1;
}

static int handle_session_start(unsigned char *response_payload, size_t max_len,
                                const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    if (!payload || payload_len < 4) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }

    unsigned int identifier = read_u32_le(payload);
    int session_idx = find_session_by_token_or_id(identifier);
    if (session_idx >= 0) {
        struct uci_session *session = &uci_sessions[session_idx];
        session->session_state = SESSION_STATE_ACTIVE;
        session->ranging_count = 0;
        enqueue_session_status_notification(session, SESSION_STATE_ACTIVE,
                                            STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);
        response_payload[0] = UCI_STATUS_OK;
    } else {
        unsigned char err = UCI_STATUS_INVALID_PARAM;
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
    }

    return 1;
}

static int handle_session_stop(unsigned char *response_payload, size_t max_len,
                               const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    if (!payload || payload_len < 4) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        return 1;
    }

    unsigned int identifier = read_u32_le(payload);
    int session_idx = find_session_by_token_or_id(identifier);
    if (session_idx >= 0) {
        struct uci_session *session = &uci_sessions[session_idx];
        session->session_state = SESSION_STATE_IDLE;
        increment_session_ranging_count(session_idx);
        enqueue_session_status_notification(session, SESSION_STATE_IDLE,
                                            STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS);
        response_payload[0] = UCI_STATUS_OK;
    } else {
        unsigned char err = UCI_STATUS_INVALID_PARAM;
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
    }

    return 1;
}

static int handle_session_get_state(unsigned char *response_payload, size_t max_len,
                                    const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    if (!payload || payload_len < 4) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        response_payload[1] = 0;
        return 2;
    }

    unsigned int identifier = read_u32_le(payload);
    int session_idx = find_session_by_token_or_id(identifier);
    unsigned char session_state = (session_idx >= 0)
                                      ? uci_sessions[session_idx].session_state
                                      : SESSION_STATE_DEINIT;

    response_payload[0] = UCI_STATUS_OK;
    response_payload[1] = session_state;
    return 2;
}

static int handle_session_get_count(unsigned char *response_payload, size_t max_len,
                                    const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    (void)payload;
    (void)payload_len;
    response_payload[0] = UCI_STATUS_OK;
    response_payload[1] = (unsigned char)get_allocated_session_count();
    return 2;
}

static int handle_session_get_ranging_count(unsigned char *response_payload, size_t max_len,
                                            const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    if (!payload || payload_len < 4) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        write_u32_le(&response_payload[1], 0);
        return 5;
    }

    unsigned int identifier = read_u32_le(payload);
    int session_idx = find_session_by_token_or_id(identifier);
    unsigned char status = UCI_STATUS_OK;
    unsigned int ranging_count = 0;

    if (session_idx >= 0) {
        ranging_count = uci_sessions[session_idx].ranging_count;
    } else {
        status = UCI_STATUS_INVALID_PARAM;
    }

    response_payload[0] = status;
    write_u32_le(&response_payload[1], ranging_count);
    return 5;
}

static int handle_session_query_data_size_in_ranging(unsigned char *response_payload,
                                                     size_t max_len,
                                                     const unsigned char *payload,
                                                     size_t payload_len) {
    (void)max_len;
    if (!payload || payload_len < 4) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        write_u16_le(&response_payload[1], 0);
        return 3;
    }

    unsigned int identifier = read_u32_le(payload);
    int session_idx = find_session_by_token_or_id(identifier);
    unsigned short max_data_size = 0x0200;
    unsigned char status = (session_idx >= 0) ? UCI_STATUS_OK : UCI_STATUS_INVALID_PARAM;

    response_payload[0] = status;
    write_u16_le(&response_payload[1], max_data_size);

    if (status != UCI_STATUS_OK) {
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &status, 1);
    }

    return 3;
}

static int handle_session_update_controller_multicast_list(
    unsigned char *response_payload, size_t max_len, const unsigned char *payload,
    size_t payload_len) {
    typedef struct {
        unsigned short short_address;
        unsigned int subsession_id;
        unsigned char status;
    } multicast_result_t;

    multicast_result_t results[MAX_MULTICAST_CONTROLEES];
    memset(results, 0, sizeof(results));

    unsigned char overall_status = UCI_STATUS_OK;
    unsigned char processed = 0;
    unsigned char action = 0;
    unsigned char declared = 0;
    unsigned int identifier = 0;
    int session_idx = -1;
    int offset = 6;

    if (!payload || payload_len < 6) {
        overall_status = UCI_STATUS_INVALID_PARAM;
    } else {
        identifier = read_u32_le(payload);
        action = payload[4];
        declared = payload[5];
        session_idx = find_session_by_token_or_id(identifier);
        if (session_idx < 0) {
            overall_status = UCI_STATUS_INVALID_PARAM;
        }
    }

    if (overall_status == UCI_STATUS_OK) {
        struct uci_session *session = &uci_sessions[session_idx];
        for (unsigned char i = 0; i < declared; i++) {
            if (offset + 2 > (int)payload_len) {
                overall_status = UCI_STATUS_INVALID_PARAM;
                break;
            }

            unsigned short short_address = read_u16_le(&payload[offset]);
            offset += 2;

            unsigned int subsession_id = 0;
            unsigned char key_len_expected = 0;
            unsigned char key_buffer[32] = {0};

            int additional_bytes = 0;
            switch (action) {
            case MULTICAST_ACTION_ADD:
            case MULTICAST_ACTION_REMOVE:
                additional_bytes = 4;
                break;
            case MULTICAST_ACTION_ADD_SHORT_KEY:
                additional_bytes = 4 + 16;
                key_len_expected = 16;
                break;
            case MULTICAST_ACTION_ADD_LONG_KEY:
                additional_bytes = 4 + 32;
                key_len_expected = 32;
                break;
            default:
                overall_status = UCI_STATUS_INVALID_PARAM;
                break;
            }

            if (overall_status != UCI_STATUS_OK) {
                break;
            }

            if (offset + additional_bytes > (int)payload_len) {
                overall_status = UCI_STATUS_INVALID_PARAM;
                break;
            }

            subsession_id = read_u32_le(&payload[offset]);
            offset += 4;

            if (key_len_expected > 0) {
                memcpy(key_buffer, &payload[offset], key_len_expected);
                offset += key_len_expected;
            }

            unsigned char per_status;
            if (action == MULTICAST_ACTION_REMOVE) {
                per_status =
                    uci_session_remove_multicast_entry(session, short_address, subsession_id);
            } else {
                per_status = uci_session_add_multicast_entry(session, short_address, subsession_id,
                                                             key_len_expected ? key_buffer : NULL,
                                                             key_len_expected);
            }

            if (processed < MAX_MULTICAST_CONTROLEES) {
                results[processed].short_address = short_address;
                results[processed].subsession_id = subsession_id;
                results[processed].status = per_status;
                processed++;
            }

            if (per_status != UCI_STATUS_OK && overall_status == UCI_STATUS_OK) {
                overall_status = per_status;
            }
        }
    }

    if (max_len < 2) {
        return -1;
    }

    response_payload[0] = overall_status;
    response_payload[1] = processed;
    size_t rsp_offset = 2;

    for (unsigned char i = 0; i < processed; i++) {
        if (rsp_offset + 7 > max_len) {
            response_payload[0] = UCI_STATUS_INVALID_MSG_SIZE;
            response_payload[1] = 0;
            rsp_offset = 2;
            break;
        }
        write_u16_le(&response_payload[rsp_offset], results[i].short_address);
        rsp_offset += 2;
        write_u32_le(&response_payload[rsp_offset], results[i].subsession_id);
        rsp_offset += 4;
        response_payload[rsp_offset++] = results[i].status;
    }

    if (response_payload[0] != UCI_STATUS_OK) {
        unsigned char err = response_payload[0];
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    return (int)rsp_offset;
}

static int handle_session_update_active_rounds_dt_tag(unsigned char *response_payload,
                                                      size_t max_len,
                                                      const unsigned char *payload,
                                                      size_t payload_len) {
    (void)max_len;
    unsigned char status = UCI_STATUS_OK;
    unsigned char stored_count = 0;
    struct uci_session *session = NULL;

    if (!payload || payload_len < 5) {
        status = UCI_STATUS_INVALID_PARAM;
    } else {
        unsigned int identifier = read_u32_le(payload);
        unsigned char declared = payload[4];
        if (payload_len < (size_t)(5 + declared)) {
            status = UCI_STATUS_INVALID_PARAM;
        } else {
            int session_idx = find_session_by_token_or_id(identifier);
            if (session_idx < 0) {
                status = UCI_STATUS_INVALID_PARAM;
            } else if (declared > MAX_DT_TAG_ROUNDS) {
                status = UCI_STATUS_INVALID_PARAM;
            } else {
                session = &uci_sessions[session_idx];
                session->dt_tag_round_count = declared;
                if (declared > 0) {
                    memcpy(session->dt_tag_round_indexes, &payload[5], declared);
                }
                stored_count = declared;
            }
        }
    }

    response_payload[0] = status;
    response_payload[1] = (status == UCI_STATUS_OK) ? stored_count : 0;
    size_t response_len = 2;

    if (status == UCI_STATUS_OK && stored_count > 0) {
        if (response_len + stored_count > max_len) {
            response_payload[0] = UCI_STATUS_INVALID_MSG_SIZE;
            response_payload[1] = 0;
            return 2;
        }
        memcpy(&response_payload[2], session->dt_tag_round_indexes, stored_count);
        response_len += stored_count;
    }

    if (status != UCI_STATUS_OK) {
        unsigned char err = status;
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    return (int)response_len;
}

static int handle_session_data_transfer_phase_config(unsigned char *response_payload,
                                                     size_t max_len,
                                                     const unsigned char *payload,
                                                     size_t payload_len) {
    (void)max_len;
    unsigned char status = UCI_STATUS_OK;
    struct uci_session *session = NULL;
    unsigned char dtp_repetition = 0;
    unsigned char dtp_control = 0;
    unsigned char dtp_size = 0;
    unsigned char extra_len = 0;
    unsigned char extra_payload[64] = {0};

    if (!payload || payload_len < 7) {
        status = UCI_STATUS_INVALID_PARAM;
    } else {
        unsigned int identifier = read_u32_le(payload);
        dtp_repetition = payload[4];
        dtp_control = payload[5];
        dtp_size = payload[6];
        extra_len = (unsigned char)((payload_len > 7) ? (payload_len - 7) : 0);
        if (extra_len > sizeof(extra_payload)) {
            status = UCI_STATUS_INVALID_MSG_SIZE;
        } else {
            if (extra_len > 0) {
                memcpy(extra_payload, &payload[7], extra_len);
            }

            int session_idx = find_session_by_token_or_id(identifier);
            if (session_idx < 0) {
                status = UCI_STATUS_INVALID_PARAM;
            } else {
                session = &uci_sessions[session_idx];
            }
        }
    }

    if (status == UCI_STATUS_OK && session) {
        session->dtp_repetition = dtp_repetition;
        session->dtp_control = dtp_control;
        session->dtp_size = dtp_size;
        session->dtp_payload_len = extra_len;
        if (extra_len > 0) {
            memcpy(session->dtp_payload, extra_payload, extra_len);
        } else {
            memset(session->dtp_payload, 0, sizeof(session->dtp_payload));
        }
    }

    if (status != UCI_STATUS_OK) {
        unsigned char err = status;
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    response_payload[0] = status;
    return 1;
}

static void enqueue_logical_link_notification(struct uci_session* session,
                                              unsigned char opcode,
                                              unsigned char link_id,
                                              unsigned char reason_or_credit) {
    unsigned char notif_payload[6];
    write_u32_le(notif_payload, session->session_handle);
    notif_payload[4] = link_id;
    notif_payload[5] = reason_or_credit;
    enqueue_notification(SESSION_CONTROL, opcode, notif_payload, sizeof(notif_payload));
}

static int handle_session_logical_link_create(unsigned char *response_payload, size_t max_len,
                                              const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    unsigned char status = UCI_STATUS_OK;
    struct uci_session *session = NULL;
    unsigned char requested_id = 0xFF;
    unsigned char mode = 0;
    unsigned char credit = 1;
    unsigned char assigned_id = 0xFF;
    uci_logical_link_entry* entry = NULL;

    if (!payload || payload_len < 4) {
        status = UCI_STATUS_INVALID_PARAM;
    } else {
        unsigned int identifier = read_u32_le(payload);
        if (payload_len >= 5) {
            requested_id = payload[4];
        }
        if (payload_len >= 6) {
            mode = payload[5];
        }
        if (payload_len >= 7) {
            credit = payload[6];
        }

        int session_idx = find_session_by_token_or_id(identifier);
        if (session_idx < 0) {
            status = UCI_STATUS_INVALID_PARAM;
        } else {
            session = &uci_sessions[session_idx];
        }
    }

    if (status == UCI_STATUS_OK && session) {
        if (requested_id != 0xFF && uci_session_find_logical_link(session, requested_id)) {
            status = UCI_STATUS_INVALID_PARAM;
        }

        if (status == UCI_STATUS_OK) {
            if (session->logical_link_count >= MAX_LOGICAL_LINKS) {
                status = UCI_STATUS_MULTICAST_LIST_FULL;
            } else {
                entry = uci_session_allocate_logical_link(session, requested_id, &assigned_id);
                if (!entry) {
                    status = UCI_STATUS_INVALID_PARAM;
                } else {
                    entry->mode = mode;
                    entry->credit = credit;
                }
            }
        }
    }

    response_payload[0] = status;
    if (status == UCI_STATUS_OK && entry) {
        response_payload[1] = assigned_id;
        response_payload[2] = entry->credit;
        enqueue_logical_link_notification(session, SESSION_LOGICAL_LINK_UWBS_CREATE,
                                          assigned_id, entry->credit);
    } else {
        response_payload[1] = (status == UCI_STATUS_OK) ? assigned_id : 0xFF;
        response_payload[2] = 0;
        if (status != UCI_STATUS_OK) {
            unsigned char err = status;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
        }
    }

    return 3;
}

static int handle_session_logical_link_close(unsigned char *response_payload, size_t max_len,
                                             const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    unsigned char status = UCI_STATUS_OK;
    struct uci_session *session = NULL;
    unsigned char link_id = 0xFF;

    if (!payload || payload_len < 5) {
        status = UCI_STATUS_INVALID_PARAM;
    } else {
        unsigned int identifier = read_u32_le(payload);
        link_id = payload[4];
        int session_idx = find_session_by_token_or_id(identifier);
        if (session_idx < 0) {
            status = UCI_STATUS_INVALID_PARAM;
        } else {
            session = &uci_sessions[session_idx];
        }
    }

    if (status == UCI_STATUS_OK && session) {
        uci_logical_link_entry* entry = uci_session_find_logical_link(session, link_id);
        if (!entry) {
            status = UCI_STATUS_INVALID_PARAM;
        } else {
            // compact array by shifting remaining entries
            size_t idx = entry - session->logical_links;
            for (size_t i = idx; i + 1 < session->logical_link_count; i++) {
                session->logical_links[i] = session->logical_links[i + 1];
            }
            if (session->logical_link_count > 0) {
                session->logical_link_count--;
            }
            if (session->logical_link_count < MAX_LOGICAL_LINKS) {
                memset(&session->logical_links[session->logical_link_count], 0,
                       sizeof(session->logical_links[session->logical_link_count]));
            }
            enqueue_logical_link_notification(session, SESSION_LOGICAL_LINK_UWBS_CLOSE,
                                              link_id, 0x00);
        }
    }

    if (status != UCI_STATUS_OK) {
        unsigned char err = status;
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    response_payload[0] = status;
    response_payload[1] = link_id;
    return 2;
}

static int handle_session_logical_link_get_param(unsigned char *response_payload, size_t max_len,
                                                 const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    unsigned char status = UCI_STATUS_OK;
    struct uci_session *session = NULL;
    uci_logical_link_entry* entry = NULL;
    unsigned char link_id = 0xFF;

    if (!payload || payload_len < 5) {
        status = UCI_STATUS_INVALID_PARAM;
    } else {
        unsigned int identifier = read_u32_le(payload);
        link_id = payload[4];
        int session_idx = find_session_by_token_or_id(identifier);
        if (session_idx < 0) {
            status = UCI_STATUS_INVALID_PARAM;
        } else {
            session = &uci_sessions[session_idx];
        }
    }

    if (status == UCI_STATUS_OK && session) {
        entry = uci_session_find_logical_link(session, link_id);
        if (!entry) {
            status = UCI_STATUS_INVALID_PARAM;
        }
    }

    if (status != UCI_STATUS_OK) {
        unsigned char err = status;
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    response_payload[0] = status;
    response_payload[1] = link_id;
    response_payload[2] = (entry && status == UCI_STATUS_OK) ? entry->mode : 0;
    response_payload[3] = (entry && status == UCI_STATUS_OK) ? entry->credit : 0;
    return 4;
}

static int handle_session_set_app_config(unsigned char *response_payload, size_t max_len,
                                         const unsigned char *payload,
                                         size_t payload_len) {
    unsigned char cfg_ids[MAX_RESPONSE_PAYLOAD_LEN] = {0};
    unsigned char processed_tlvs = 0;
    unsigned char declared_tlvs = 0;
    unsigned int identifier = 0;
    int session_idx = -1;

    if (payload && payload_len >= 5) {
        identifier = read_u32_le(payload);
        declared_tlvs = payload[4];
        session_idx = find_session_by_token_or_id(identifier);

        const unsigned char *tlv_buffer = (payload_len > 5) ? &payload[5] : NULL;
        size_t tlv_length = (payload_len > 5) ? (size_t)(payload_len - 5) : 0;
        struct uci_tlv_reader reader;
        uci_tlv_reader_init(&reader, tlv_buffer, tlv_length);

        for (unsigned char i = 0; i < declared_tlvs; i++) {
            unsigned char cfg_id = 0;
            unsigned char cfg_len = 0;
            const unsigned char *value_ptr = NULL;

            int res = uci_tlv_reader_next(&reader, &cfg_id, &value_ptr, &cfg_len);
            if (res <= 0) {
                break;
            }

            if (session_idx >= 0) {
                store_session_config(session_idx, cfg_id,
                                     (unsigned char *)value_ptr, cfg_len);
            }

            if (processed_tlvs < (MAX_RESPONSE_PAYLOAD_LEN - 2) / 2) {
                cfg_ids[processed_tlvs] = cfg_id;
                processed_tlvs++;
            }
        }
    }

    unsigned char status = UCI_STATUS_OK;
    if (!payload || payload_len < 5 || processed_tlvs != declared_tlvs || session_idx < 0) {
        status = UCI_STATUS_INVALID_PARAM;
        if (session_idx < 0) {
            processed_tlvs = 0;
        }
    }

    size_t max_entries = (max_len > 2) ? (max_len - 2) / 2 : 0;
    if (processed_tlvs > max_entries) {
        processed_tlvs = (unsigned char)max_entries;
        status = UCI_STATUS_INVALID_PARAM;
    }

    size_t response_len = 2 + (processed_tlvs * 2);
    if (response_len > max_len) {
        return -1;
    }

    response_payload[0] = status;
    response_payload[1] = processed_tlvs;
    for (unsigned char i = 0; i < processed_tlvs; i++) {
        response_payload[2 + (i * 2)] = cfg_ids[i];
        response_payload[2 + (i * 2) + 1] = UCI_STATUS_OK;
    }

    if (status != UCI_STATUS_OK) {
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &status, 1);
    }

    return (int)response_len;
}

static int handle_session_get_app_config(unsigned char *response_payload, size_t max_len,
                                         const unsigned char *payload,
                                         size_t payload_len) {
    unsigned int identifier = 0;
    unsigned char declared_cfgs = 0;
    int session_idx = -1;
    unsigned char cfg_count = 0;

    if (payload && payload_len >= 5) {
        identifier = read_u32_le(payload);
        declared_cfgs = payload[4];
        session_idx = find_session_by_token_or_id(identifier);

        unsigned int available_ids = payload_len - 5;
        if (declared_cfgs > available_ids) {
            declared_cfgs = (unsigned char)available_ids;
        }

        unsigned int max_cfg_entries = (max_len > 2) ? (max_len - 2) / 3 : 0;
        if (declared_cfgs > max_cfg_entries) {
            declared_cfgs = (unsigned char)max_cfg_entries;
        }

        cfg_count = declared_cfgs;
    }

    if (session_idx < 0) {
        cfg_count = 0;
    }

    unsigned char status = (session_idx >= 0) ? UCI_STATUS_OK : UCI_STATUS_INVALID_PARAM;
    unsigned char returned_cfgs = 0;
    size_t response_len = 2;

    if (status != UCI_STATUS_OK) {
        unsigned char err = UCI_STATUS_INVALID_PARAM;
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    for (unsigned char i = 0; i < cfg_count; i++) {
        unsigned char cfg_id = payload[5 + i];
        unsigned char value_buf[MAX_SESSION_CONFIG_VALUE_SIZE];
        unsigned char value_len = MAX_SESSION_CONFIG_VALUE_SIZE;
        unsigned char copy_len = 0;

        if (session_idx >= 0 &&
            get_session_config(session_idx, cfg_id, value_buf, &value_len) && value_len > 0) {
            copy_len = value_len;
        }

        size_t required = 2 + copy_len;
        if (response_len + required > max_len) {
            status = UCI_STATUS_INVALID_PARAM;
            unsigned char err = UCI_STATUS_INVALID_PARAM;
            enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
            break;
        }

        response_payload[response_len] = cfg_id;
        response_payload[response_len + 1] = copy_len;
        if (copy_len > 0) {
            memcpy(&response_payload[response_len + 2], value_buf, copy_len);
        }
        response_len += required;
        returned_cfgs++;
    }

    response_payload[0] = status;
    response_payload[1] = returned_cfgs;
    return (int)response_len;
}

static int handle_test_rf_set_config(unsigned char *response_payload, size_t max_len,
                                     const unsigned char *payload, size_t payload_len) {
    (void)payload;
    (void)payload_len;
    if (max_len < 2) {
        return -1;
    }
    response_payload[0] = UCI_STATUS_OK;
    response_payload[1] = 0x00;
    return 2;
}

static int handle_test_rf_simple_status(unsigned char *response_payload, size_t max_len,
                                        const unsigned char *payload, size_t payload_len) {
    (void)payload;
    (void)payload_len;
    if (max_len < 1) {
        return -1;
    }
    response_payload[0] = UCI_STATUS_OK;
    return 1;
}

static int handle_vendor_android_get_power_stats(unsigned char *response_payload,
                                                 size_t max_len,
                                                 const unsigned char *payload,
                                                 size_t payload_len) {
    (void)payload;
    (void)payload_len;
    if (max_len < 17) {
        return -1;
    }
    memset(response_payload, 0, 17);
    response_payload[0] = UCI_STATUS_OK;
    return 17;
}

static int handle_vendor_android_set_country_code(unsigned char *response_payload, size_t max_len,
                                                  const unsigned char *payload, size_t payload_len) {
    (void)max_len;
    (void)payload;
    (void)payload_len;
    response_payload[0] = UCI_STATUS_OK;
    response_payload[1] = 0;
    return 2;
}

static int handle_vendor_android_radar_set_app_config(unsigned char *response_payload,
                                                      size_t max_len,
                                                      const unsigned char *payload,
                                                      size_t payload_len) {
    (void)payload;
    (void)payload_len;
    if (max_len < 2) {
        return -1;
    }
    response_payload[0] = UCI_STATUS_OK;
    response_payload[1] = 0x00;
    return 2;
}

static int handle_vendor_android_radar_get_app_config(unsigned char *response_payload,
                                                      size_t max_len,
                                                      const unsigned char *payload,
                                                      size_t payload_len) {
    (void)payload;
    (void)payload_len;
    if (max_len < 2) {
        return -1;
    }
    response_payload[0] = UCI_STATUS_OK;
    response_payload[1] = 0x00;
    return 2;
}

void handle_core_device_suspend_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_DEVICE_SUSPEND_RSP payload too short.\n");
        return;
    }
    unsigned char status = payload[0];
    printf("  Status: 0x%02X (%s)\n", status, status == UCI_STATUS_OK ? "OK" : "ERROR");
    if (status == UCI_STATUS_OK) {
        printf("  Device suspended successfully.\n");
    } else {
        printf("  Device suspend failed.\n");
    }
}

void handle_core_query_uwbs_timestamp_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 9) { // status (1) + timestamp (8)
        printf("Error: CORE_QUERY_UWBS_TIMESTAMP_RSP payload too short. Expected at least 9 bytes, got %d.\n", payload_len);
        return;
    }
    unsigned char status = payload[0];
    unsigned long long timestamp = read_u64_le(&payload[1]);
    
    printf("  Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: 
            printf(" (OK)\n");
            printf("  Timestamp: %llu\n", timestamp);
            break;
        case UCI_STATUS_REJECTED: 
            printf(" (REJECTED)\n");
            break;
        case UCI_STATUS_FAILED: 
            printf(" (FAILED)\n");
            break;
        default: 
            printf(" (UNKNOWN)\n");
            break;
    }
}

void handle_core_device_info_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 9) { // status (1) + uci_version (2) + mac_version (2) + phy_version (2) + uci_test_version (2)
        printf("Error: CORE_DEVICE_INFO_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned short uci_version = read_u16_le(&payload[1]);
    unsigned short mac_version = read_u16_le(&payload[3]);
    unsigned short phy_version = read_u16_le(&payload[5]);
    unsigned short uci_test_version = read_u16_le(&payload[7]);
    // Remaining bytes are vendor_spec_info (may be 0 or more bytes)

    printf("  Status: 0x%02X\n", status);
    printf("  UCI Version: 0x%04X\n", uci_version);
    printf("  MAC Version: 0x%04X\n", mac_version);
    printf("  PHY Version: 0x%04X\n", phy_version);
    printf("  UCI Test Version: 0x%04X\n", uci_test_version);
    
    if (payload_len > 9) {
        printf("  Vendor Specific Info: ");
        for (int i = 9; i < payload_len; i++) {
            printf("%02X ", payload[i]);
        }
        printf("\n");
    }
}

void handle_core_get_caps_info_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_tlvs (1)
        printf("Error: CORE_GET_CAPS_INFO_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    printf("  Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)\n"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)\n"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)\n"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)\n"); break;
        default: printf(" (UNKNOWN)\n"); break;
    }
    printf("  Number of TLVs: %d\n", num_tlvs);

    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete TLV in CORE_GET_CAPS_INFO_RSP payload.\n");
            return;
        }
        CapTlvType tlv_type = (CapTlvType)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        
        // Print TLV type with descriptive name
        printf("    TLV %d:\n", i);
        printf("      Type: 0x%02X", tlv_type);
        switch(tlv_type) {
            case SUPPORTED_V1_FIRA_PHY_VERSION_RANGE_V2_MAX_MESSAGE_SIZE: printf(" (FIRA_PHY_VERSION_RANGE)\n"); break;
            case SUPPORTED_V1_FIRA_MAC_VERSION_RANGE_V2_MAX_DATA_PAYLOAD_SIZE: printf(" (FIRA_MAC_VERSION_RANGE)\n"); break;
            case SUPPORTED_V1_DEVICE_ROLES_V2_FIRA_PHY_VERSION_RANGE: printf(" (DEVICE_ROLES)\n"); break;
            case SUPPORTED_V1_RANGING_METHOD_V2_FIRA_MAC_VERSION_RANGE: printf(" (RANGING_METHOD)\n"); break;
            case SUPPORTED_V1_STS_CONFIG_V2_DEVICE_TYPE: printf(" (STS_CONFIG)\n"); break;
            case SUPPORTED_V1_MULTI_NODE_MODES_V2_DEVICE_ROLES: printf(" (MULTI_NODE_MODES)\n"); break;
            case SUPPORTED_V1_RANGING_TIME_STRUCT_V2_RANGING_METHOD: printf(" (RANGING_TIME_STRUCT)\n"); break;
            case SUPPORTED_V1_SCHEDULED_MODE_V2_STS_CONFIG: printf(" (SCHEDULED_MODE)\n"); break;
            case SUPPORTED_V1_HOPPING_MODE_V2_MULTI_NODE_MODE: printf(" (HOPPING_MODE)\n"); break;
            case SUPPORTED_V1_BLOCK_STRIDING_V2_RANGING_TIME_STRUCT: printf(" (BLOCK_STRIDING)\n"); break;
            case SUPPORTED_V1_UWB_INITIATION_TIME_V2_SCHEDULE_MODE: printf(" (UWB_INITIATION_TIME)\n"); break;
            case SUPPORTED_V1_CHANNELS_V2_HOPPING_MODE: printf(" (CHANNELS)\n"); break;
            case SUPPORTED_V1_RFRAME_CONFIG_V2_BLOCK_STRIDING: printf(" (RFRAME_CONFIG)\n"); break;
            case SUPPORTED_V1_CC_CONSTRAINT_LENGTH_V2_UWB_INITIATION_TIME: printf(" (CC_CONSTRAINT_LENGTH)\n"); break;
            case SUPPORTED_V1_BPRF_PARAMETER_SETS_V2_CHANNELS: printf(" (BPRF_PARAMETER_SETS)\n"); break;
            case SUPPORTED_V1_HPRF_PARAMETER_SETS_V2_RFRAME_CONFIG: printf(" (HPRF_PARAMETER_SETS)\n"); break;
            case SUPPORTED_V1_AOA_V2_AOA_SUPPORT: printf(" (AOA_SUPPORT)\n"); break;
            case SUPPORTED_V1_EXTENDED_MAC_ADDRESS_V2_EXTENDED_MAC_ADDRESS: printf(" (EXTENDED_MAC_ADDRESS)\n"); break;
            case SUPPORTED_V1_MAX_MESSAGE_SIZE_V2_ASSIGNED: printf(" (MAX_MESSAGE_SIZE)\n"); break;
            case SUPPORTED_V1_MAX_DATA_PACKET_PAYLOAD_SIZE_V2_SESSION_KEY_LENGTH: printf(" (MAX_DATA_PACKET_PAYLOAD_SIZE)\n"); break;
            case SUPPORTED_V2_EXTENDED_MAC_ADDRESS: printf(" (V2_EXTENDED_MAC_ADDRESS)\n"); break;
            case SUPPORTED_V2_ASSIGNED: printf(" (V2_ASSIGNED)\n"); break;
            case SUPPORTED_V2_SESSION_KEY_LENGTH: printf(" (V2_SESSION_KEY_LENGTH)\n"); break;
            case SUPPORTED_V2_DT_ANCHOR_MAX_ACTIVE_RR: printf(" (DT_ANCHOR_MAX_ACTIVE_RR)\n"); break;
            case SUPPORTED_V2_DT_TAG_MAX_ACTIVE_RR: printf(" (DT_TAG_MAX_ACTIVE_RR)\n"); break;
            case SUPPORTED_V2_DT_TAG_BLOCK_SHIPPING: printf(" (DT_TAG_BLOCK_SHIPPING)\n"); break;
            case SUPPORTED_V2_PSDU_LENGTH_SUPPORT: printf(" (PSDU_LENGTH_SUPPORT)\n"); break;
            case CCC_SUPPORTED_CHAPS_PER_SLOT: printf(" (CCC_CHAPS_PER_SLOT)\n"); break;
            case CCC_SUPPORTED_SYNC_CODES: printf(" (CCC_SYNC_CODES)\n"); break;
            case CCC_SUPPORTED_HOPPING_CONFIG_MODES_AND_SEQUENCES: printf(" (CCC_HOPPING_CONFIG_MODES)\n"); break;
            case CCC_SUPPORTED_CHANNELS: printf(" (CCC_CHANNELS)\n"); break;
            case CCC_SUPPORTED_VERSIONS: printf(" (CCC_VERSIONS)\n"); break;
            case CCC_SUPPORTED_UWB_CONFIGS: printf(" (CCC_UWB_CONFIGS)\n"); break;
            case CCC_SUPPORTED_PULSE_SHAPE_COMBOS: printf(" (CCC_PULSE_SHAPE_COMBOS)\n"); break;
            case CCC_SUPPORTED_RAN_MULTIPLIER: printf(" (CCC_RAN_MULTIPLIER)\n"); break;
            case CCC_SUPPORTED_MAX_RANGING_SESSION_NUMBER: printf(" (CCC_MAX_RANGING_SESSION_NUMBER)\n"); break;
            case CCC_SUPPORTED_MIN_UWB_INITIATION_TIME_MS: printf(" (CCC_MIN_UWB_INITIATION_TIME_MS)\n"); break;
            case CCC_PRIORITIZED_CHANNEL_LIST: printf(" (CCC_PRIORITIZED_CHANNEL_LIST)\n"); break;
            case CCC_SUPPORTED_UWBS_MAX_PPM: printf(" (CCC_UWBS_MAX_PPM)\n"); break;
            case ALIRO_SUPPORTED_MAC_MODES: printf(" (ALIRO_MAC_MODES)\n"); break;
            case RADAR_SUPPORT: printf(" (RADAR_SUPPORT)\n"); break;
            case SUPPORTED_POWER_STATS: printf(" (POWER_STATS)\n"); break;
            case SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING: printf(" (AOA_RESULT_REQ_ANTENNA_INTERLEAVING)\n"); break;
            case SUPPORTED_MIN_RANGING_INTERVAL_MS: printf(" (MIN_RANGING_INTERVAL_MS)\n"); break;
            case SUPPORTED_RANGE_DATA_NTF_CONFIG: printf(" (RANGE_DATA_NTF_CONFIG)\n"); break;
            case SUPPORTED_RSSI_REPORTING: printf(" (RSSI_REPORTING)\n"); break;
            case SUPPORTED_DIAGNOSTICS: printf(" (DIAGNOSTICS)\n"); break;
            case SUPPORTED_MIN_SLOT_DURATION_RSTU: printf(" (MIN_SLOT_DURATION_RSTU)\n"); break;
            case SUPPORTED_MAX_RANGING_SESSION_NUMBER: printf(" (MAX_RANGING_SESSION_NUMBER)\n"); break;
            default: printf(" (UNKNOWN)\n"); break;
        }
        printf("      Length: %d bytes\n", tlv_len);
        
        if (offset + tlv_len > payload_len) {
            printf("Error: Incomplete TLV value in CORE_GET_CAPS_INFO_RSP payload.\n");
            return;
        }
        
        // Print value with interpretation based on TLV type
        printf("      Value: ");
        for (int j = 0; j < tlv_len; j++) {
            printf("%02X ", payload[offset + j]);
        }
        printf("\n");
        
        // Interpret value based on TLV type and length
        if (tlv_len > 0) {
            printf("      Interpreted Value: ");
            if (tlv_len == 1) {
                unsigned char val = payload[offset];
                printf("%u", val);
                // Special interpretations for boolean-like values
                if (tlv_type == SUPPORTED_V1_AOA_V2_AOA_SUPPORT ||
                    tlv_type == SUPPORTED_V2_DT_TAG_BLOCK_SHIPPING ||
                    tlv_type == RADAR_SUPPORT) {
                    printf(" (%s)", val ? "SUPPORTED" : "NOT_SUPPORTED");
                }
            } else if (tlv_len == 2) {
                unsigned short val = payload[offset] | (payload[offset + 1] << 8); // Little-endian
                printf("%u", val);
            } else if (tlv_len == 4) {
                unsigned int val = payload[offset] | 
                                  (payload[offset + 1] << 8) | 
                                  (payload[offset + 2] << 16) | 
                                  (payload[offset + 3] << 24); // Little-endian
                printf("%u", val);
            } else {
                // For longer values, show as hex dump with interpretation
                printf("(complex value)");
            }
            printf("\n");
        }
        
        offset += tlv_len;
    }
}

void handle_core_set_config_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_cfg_status (1)
        printf("Error: CORE_SET_CONFIG_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_cfg_status = payload[1];
    printf("  Status: 0x%02X\n", status);
    printf("  Number of Config Status: %d\n", num_cfg_status);

    int offset = 2;
    for (int i = 0; i < num_cfg_status; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete Config Status in CORE_SET_CONFIG_RSP payload.\n");
            return;
        }
        DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
        unsigned char cfg_status = payload[offset + 1];
        const char* cfg_name = get_device_config_name(cfg_id);
        printf("    Config ID: 0x%02X (%s), Status: 0x%02X", cfg_id,
               cfg_name ? cfg_name : "UNKNOWN", cfg_status);
        if (cfg_status == UCI_STATUS_OK) {
            printf(" (OK)");
        } else if (cfg_status == UCI_STATUS_INVALID_PARAM) {
            printf(" (Invalid Parameter)");
        } else if (cfg_status == UCI_STATUS_REJECTED) {
            printf(" (Rejected)");
        }
        printf("\n");
        offset += 2;
    }
}

// Helper function to print device config ID name
static const char* get_device_config_name(DeviceConfigId cfg_id) {
    const char* name = uci_config_get_device_param_name(cfg_id);
    return name ? name : "UNKNOWN";
}

// Helper function to interpret and print device state value
void print_device_state_value(unsigned char value) {
    switch(value) {
        case DEVICE_STATE_READY: printf("(READY)"); break;
        case DEVICE_STATE_ACTIVE: printf("(ACTIVE)"); break;
        case DEVICE_STATE_ERROR: printf("(ERROR)"); break;
        default: printf("(UNKNOWN: 0x%02X)", value); break;
    }
}

static void print_device_config_value(DeviceConfigId cfg_id, const unsigned char* value, size_t len) {
    if (!value || len == 0) {
        return;
    }

    switch (cfg_id) {
        case DEVICE_STATE:
            if (len == 1) {
                print_device_state_value(value[0]);
            }
            break;
        case LOW_POWER_MODE:
        case DEVICE_PAN_COORD:
        case DEVICE_PROMISCUOUS:
            if (len == 1) {
                printf("(%s)", value[0] ? "ON" : "OFF");
            }
            break;
        case DEVICE_CHANNEL:
            if (len == 1) {
                printf("(Channel %u)", value[0]);
            }
            break;
        case DEVICE_PREAMBLE_CODE:
            if (len == 1) {
                printf("(Preamble Code %u)", value[0]);
            }
            break;
        case DEVICE_PAN_ID:
        case DEVICE_SHORT_ADDR:
            if (len == 2) {
                printf("(0x%04X)", (unsigned int)read_u16_le(value));
            }
            break;
        case DEVICE_EXTENDED_ADDR:
            if (len == 8) {
                unsigned long long addr = read_u64_le(value);
                printf("(0x%016llX)", addr);
            }
            break;
        case DEVICE_FRAME_RETRIES:
            if (len == 1) {
                printf("(%u retries)", value[0]);
            }
            break;
        case DEVICE_TRACES:
            if (len == 4) {
                printf("(Trace mask 0x%08X)", read_u32_le(value));
            }
            break;
        case DEVICE_PM_MIN_INACTIVITY_S4:
            if (len == 4) {
                printf("(%u ms)", read_u32_le(value));
            }
            break;
        default:
            break;
    }
}


void handle_core_device_reset_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_DEVICE_RESET_RSP payload too short.\n");
        return;
    }
    unsigned char status = payload[0];
    printf("  Status: 0x%02X (%s)\n", status, status == UCI_STATUS_OK ? "OK" : "ERROR");
    if (status == UCI_STATUS_OK) {
        printf("  Device reset successfully.\n");
    } else {
        printf("  Device reset failed.\n");
    }
}

void handle_core_get_config_rsp(unsigned char* payload, int payload_len) {
    if (payload_len < 2) { // status (1) + num_tlvs (1)
        printf("Error: CORE_GET_CONFIG_RSP payload too short.\n");
        return;
    }

    unsigned char status = payload[0];
    unsigned char num_tlvs = payload[1];
    printf("  Status: 0x%02X\n", status);
    printf("  Number of TLVs: %d\n", num_tlvs);

    int offset = 2;
    for (int i = 0; i < num_tlvs; i++) {
        if (offset + 2 > payload_len) {
            printf("Error: Incomplete TLV in CORE_GET_CONFIG_RSP payload.\n");
            return;
        }
        DeviceConfigId cfg_id = (DeviceConfigId)payload[offset];
        unsigned char tlv_len = payload[offset + 1];
        offset += 2;
        if (offset + tlv_len > payload_len) {
            printf("Error: Incomplete TLV value in CORE_GET_CONFIG_RSP payload.\n");
            return;
        }
        
        const char* cfg_name = get_device_config_name(cfg_id);
        printf("    Config ID: 0x%02X (%s), Length: %d, Value: ", cfg_id,
               cfg_name ? cfg_name : "UNKNOWN", tlv_len);
        
        for (int j = 0; j < tlv_len; j++) {
            printf("%02X ", payload[offset + j]);
        }
        
        print_device_config_value(cfg_id, &payload[offset], tlv_len);
        
        printf("\n");
        offset += tlv_len;
    }
}

// Helper function to initialize session storage
void init_uci_sessions() {
    g_session_handle_counter = 1;
    g_notification_head = 0;
    g_notification_tail = 0;
    uci_session_store_reset_all();
    uci_report_error(__func__, "initialized session storage", UCI_SUCCESS);
}

// Notification handlers
void handle_core_device_status_ntf(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_DEVICE_STATUS_NTF payload too short.\n");
        return;
    }

    unsigned char device_state = payload[0];
    printf("  Device State: 0x%02X", device_state);
    switch(device_state) {
        case DEVICE_STATE_READY: printf(" (READY)"); break;
        case DEVICE_STATE_ACTIVE: printf(" (ACTIVE)"); break;
        case 0xFF: printf(" (ERROR)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");
}

void handle_core_generic_error_ntf(unsigned char* payload, int payload_len) {
    if (payload_len < 1) {
        printf("Error: CORE_GENERIC_ERROR_NTF payload too short.\n");
        return;
    }
    
    unsigned char status = payload[0];
    printf("  Generic Error Status: 0x%02X", status);
    switch(status) {
        case UCI_STATUS_OK: printf(" (OK)"); break;
        case UCI_STATUS_REJECTED: printf(" (REJECTED)"); break;
        case UCI_STATUS_FAILED: printf(" (FAILED)"); break;
        case UCI_STATUS_SYNTAX_ERROR: printf(" (SYNTAX_ERROR)"); break;
        case UCI_STATUS_INVALID_PARAM: printf(" (INVALID_PARAM)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");
}

void handle_generic_notification(unsigned char gid, unsigned char opcode, unsigned char* payload, int payload_len) {
    printf("  [Generic Notification - GID: 0x%02X, OID: 0x%02X]\n", gid, opcode);
    printf("  Payload: ");
    for (int i = 0; i < payload_len; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");
}

// Session Configuration Notifications
void handle_session_config_ntf(unsigned char opcode, unsigned char* payload, int payload_len) {
    if (opcode == SESSION_STATUS_NTF) {
        if (payload_len < 6) { // session_id(4) + session_state(1) + reason_code(1)
            printf("  Error: SESSION_STATUS_NTF payload too short.\n");
            return;
        }
        
        unsigned int session_token = read_u32_le(payload);
        unsigned char session_state = payload[4];
        unsigned char reason_code = payload[5];
        
        printf("  Session Token: 0x%08X\n", session_token);
        printf("  Session State: 0x%02X", session_state);
        switch(session_state) {
            case SESSION_STATE_INIT: printf(" (INIT)"); break;
            case SESSION_STATE_DEINIT: printf(" (DEINIT)"); break;
            case SESSION_STATE_ACTIVE: printf(" (ACTIVE)"); break;
            case SESSION_STATE_IDLE: printf(" (IDLE)"); break;
            default: printf(" (UNKNOWN)"); break;
        }
        printf("\n");
        printf("  Reason Code: 0x%02X", reason_code);
        switch(reason_code) {
            case STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS: printf(" (SESSION_MANAGEMENT_COMMAND)"); break;
            case MAX_RANGING_ROUND_RETRY_COUNT_REACHED: printf(" (MAX_RETRY_COUNT_REACHED)"); break;
            case MAX_NUMBER_OF_MEASUREMENTS_REACHED: printf(" (MAX_MEASUREMENTS_REACHED)"); break;
            case SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL: printf(" (SUSPENDED_INBAND_SIGNAL)"); break;
            case SESSION_RESUMED_DUE_TO_INBAND_SIGNAL: printf(" (RESUMED_INBAND_SIGNAL)"); break;
            case SESSION_STOPPED_DUE_TO_INBAND_SIGNAL: printf(" (STOPPED_INBAND_SIGNAL)"); break;
            default: printf(" (UNKNOWN_REASON)"); break;
        }
        printf("\n");
    } else {
        handle_generic_notification(SESSION_CONFIG, opcode, payload, payload_len);
    }
}

// Session Control Notifications
void handle_session_control_ntf(unsigned char opcode, unsigned char* payload, int payload_len) {
    if (opcode == SESSION_DATA_CREDIT_NTF) {
        if (payload_len < 5) { // session_token(4) + credit_availability(1)
            printf("  Error: SESSION_DATA_CREDIT_NTF payload too short.\n");
            return;
        }
        
        unsigned int session_token = read_u32_le(payload);
        unsigned char credit_availability = payload[4];
        
        printf("  Session Token: 0x%08X\n", session_token);
        printf("  Credit Availability: %s\n", credit_availability ? "AVAILABLE" : "NOT_AVAILABLE");
    } else if (opcode == SESSION_DATA_TRANSFER_STATUS_NTF) {
        if (payload_len < 6) { // session_token(4) + uci_seq_num(2) + status(1) + tx_count(1)
            printf("  Error: SESSION_DATA_TRANSFER_STATUS_NTF payload too short.\n");
            return;
        }
        
        unsigned int session_token = read_u32_le(payload);
        unsigned short uci_sequence_number = read_u16_le(&payload[4]);
        unsigned char status = payload[6];
        unsigned char tx_count = payload[7];
        
        printf("  Session Token: 0x%08X\n", session_token);
        printf("  UCI Sequence Number: %d\n", uci_sequence_number);
        printf("  Status: 0x%02X", status);
        switch(status) {
            case UCI_DATA_TRANSFER_STATUS_REPETITION_OK: printf(" (REPETITION_OK)"); break;
            case UCI_DATA_TRANSFER_STATUS_OK: printf(" (OK)"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER: printf(" (ERROR_DATA_TRANSFER)"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE: printf(" (ERROR_NO_CREDIT_AVAILABLE)"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_REJECTED: printf(" (ERROR_REJECTED)"); break;
            case UCI_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED: printf(" (SESSION_TYPE_NOT_SUPPORTED)"); break;
            case UCI_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING: printf(" (ERROR_DATA_TRANSFER_IS_ONGOING)"); break;
            case UCI_DATA_TRANSFER_STATUS_INVALID_FORMAT: printf(" (INVALID_FORMAT)"); break;
            default: printf(" (UNKNOWN)"); break;
        }
        printf("\n");
        printf("  TX Count: %d\n", tx_count);
    } else if (opcode == SESSION_INFO_NTF) {
        // Handle ranging/session info notifications - the core UWB functionality
        handle_session_info_ntf(payload, payload_len);
    } else {
        handle_generic_notification(SESSION_CONTROL, opcode, payload, payload_len);
    }
}

// Cherry models SESSION_INFO_NTF as the range-data notification surface.
void handle_session_info_ntf(unsigned char* payload, int payload_len) {
    if (!payload || payload_len < 25) {
        printf("  Error: SESSION_INFO_NTF payload too short. Need at least 25 bytes, got %d.\n", payload_len);
        return;
    }

    // Parse header fields according to Cherry's range-data notification model.
    unsigned int sequence_number = read_u32_le(&payload[0]);
    unsigned int session_handle = read_u32_le(&payload[4]);
    unsigned char reserved_byte = payload[8];
    unsigned int current_ranging_interval = read_u32_le(&payload[9]);
    unsigned char ranging_measurement_type = payload[13];
    // unsigned char reserved1 = payload[14]; // Skip reserved byte at position 14
    unsigned char mac_address_indicator = payload[15];
    unsigned int hus_primary_session_id = read_u32_le(&payload[16]);
    // unsigned int reserved2 = read_u32_le(&payload[20]); // Skip reserved 4 bytes at position 20-23
    unsigned char measurement_count = payload[24];

    printf("  Sequence Number: %u\n", sequence_number);
    printf("  Session Handle: 0x%08X\n", session_handle);
    printf("  Reserved Byte: 0x%02X\n", reserved_byte);
    printf("  Current Ranging Interval: %u ms\n", current_ranging_interval);
    printf("  Ranging Measurement Type: 0x%02X", ranging_measurement_type);
    switch(ranging_measurement_type) {
        case 0x00: printf(" (ONE_WAY)"); break;
        case 0x01: printf(" (TWO_WAY)"); break;
        case 0x02: printf(" (DL_TDOA)"); break;
        case 0x03: printf(" (OWR_AOA)"); break;
        default: printf(" (UNKNOWN)"); break;
    }
    printf("\n");
    printf("  MAC Address Indicator: %s\n", mac_address_indicator ? "EXTENDED_ADDRESS" : "SHORT_ADDRESS");
    printf("  Primary Session ID: 0x%08X\n", hus_primary_session_id);
    printf("  Measurement Count: %u\n", measurement_count);

    // Parse ranging measurements
    int offset = 25; // Cherry INFO_NTF header size: 4+4+1+4+1+1+1+4+4+1
    if (offset >= payload_len) {
        printf("  No ranging measurements in notification (offset=%d, payload_len=%d).\n", offset, payload_len);
        return;
    }
    
    printf("  Ranging Measurements (offset=%d, payload_len=%d):\n", offset, payload_len);
    
    if (ranging_measurement_type == 0x01) { // TWO_WAY
        printf("    Number of Two-Way Measurements: %d\n", measurement_count);
        
        for (int i = 0; i < measurement_count && offset < payload_len; i++) {
            if (offset + 20 > payload_len) { // Size for SHORT_ADDRESS two-way measurement = 20 bytes
                printf("    Warning: Incomplete measurement data at index %d (need offset+%d=%d but have %d)\n", 
                       i, 20, offset + 20, payload_len);
                break;
            }
            
            printf("    Measurement %d:\n", i + 1);

            if (mac_address_indicator == 0) { // SHORT_ADDRESS
                if (offset + 20 > payload_len) {
                    printf("      Error: Insufficient data for SHORT_ADDRESS measurement\n");
                    break;
                }
                print_short_address_measurement(&payload[offset]);
                offset += 20;
            } else { // EXTENDED_ADDRESS
                if (offset + 26 > payload_len) {
                    printf("      Error: Insufficient data for EXTENDED_ADDRESS measurement\n");
                    break;
                }
                print_extended_address_measurement(&payload[offset]);
                offset += 26;
            }
        }
    } else if (ranging_measurement_type == 0x03) { // OWR_AOA
        printf("    Number of OWR-AoA Measurements: %d\n", measurement_count);
        
        for (int i = 0; i < measurement_count && offset + 13 <= payload_len; i++) {
            printf("    OWR-AoA Measurement %d:\n", i + 1);
            
            if (mac_address_indicator == 0x00) { // SHORT_ADDRESS
                unsigned short mac_address = read_u16_le(&payload[offset]);
                unsigned char status = payload[offset + 2];
                unsigned char nlos = payload[offset + 3];
                unsigned char frame_sequence_number = payload[offset + 4];
                unsigned short block_index = read_u16_le(&payload[offset + 5]);
                unsigned short aoa_azimuth = read_u16_le(&payload[offset + 7]);
                unsigned char aoa_azimuth_fom = payload[offset + 9];
                unsigned short aoa_elevation = read_u16_le(&payload[offset + 10]);
                unsigned char aoa_elevation_fom = payload[offset + 12];
                
                printf("      MAC Address: 0x%04X\n", mac_address);
                printf("      Status: 0x%02X", status);
                if (status == 0x00) printf(" (OK)");
                printf("\n");
                printf("      NLOS: %s\n", nlos ? "YES" : "NO");
                printf("      Frame Sequence Number: %u\n", frame_sequence_number);
                printf("      Block Index: %u\n", block_index);
                printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                
                offset += 13;
            } else { // EXTENDED_ADDRESS
                if (offset + 19 <= payload_len) {
                    unsigned long long mac_address = read_u64_le(&payload[offset]);
                    unsigned char status = payload[offset + 8];
                    unsigned char nlos = payload[offset + 9];
                    unsigned char frame_sequence_number = payload[offset + 10];
                    unsigned short block_index = read_u16_le(&payload[offset + 11]);
                    unsigned short aoa_azimuth = read_u16_le(&payload[offset + 13]);
                    unsigned char aoa_azimuth_fom = payload[offset + 15];
                    unsigned short aoa_elevation = read_u16_le(&payload[offset + 16]);
                    unsigned char aoa_elevation_fom = payload[offset + 18];
                    
                    printf("      MAC Address: 0x%016llX\n", mac_address);
                    printf("      Status: 0x%02X", status);
                    if (status == 0x00) printf(" (OK)");
                    printf("\n");
                    printf("      NLOS: %s\n", nlos ? "YES" : "NO");
                    printf("      Frame Sequence Number: %u\n", frame_sequence_number);
                    printf("      Block Index: %u\n", block_index);
                    printf("      AoA Azimuth: %u degrees (FoM: %u)\n", aoa_azimuth, aoa_azimuth_fom);
                    printf("      AoA Elevation: %u degrees (FoM: %u)\n", aoa_elevation, aoa_elevation_fom);
                    
                    offset += 19;
                } else {
                    printf("        ERROR: Insufficient data for EXTENDED_ADDRESS OWR-AoA measurement\n");
                    break;
                }
            }
        }
    } else {
        printf("    Unsupported ranging measurement type: 0x%02X\n", ranging_measurement_type);
    }

    if (offset < payload_len) {
        printf("  Vendor-specific Range Data (%d bytes):\n", payload_len - offset);
        decode_range_vendor_data(&payload[offset], payload_len - offset);
    }
}

void parse_uci_packet(uci_uint8* packet, size_t packet_len) {
    const unsigned char* payload_ptr = NULL;
    const unsigned char* header_bytes = NULL;
    int segmented_reassembled = 0;

    if (!packet) {
        UCI_LOG_ERROR("parse_uci_packet: NULL packet pointer", UCI_ERROR_INVALID_PARAM);
        return;
    }
    
    if (packet_len < sizeof(struct uci_packet_header)) {
        printf("Error: UCI packet too short to contain a header (got %zu, need at least %zu).\n", 
               packet_len, sizeof(struct uci_packet_header));
        UCI_LOG_ERROR("parse_uci_packet: Packet too short to contain header", UCI_ERROR_INVALID_PARAM);
        return;
    }

    // Validate packet length against maximum allowed size to prevent potential attacks
    size_t header_only_max = sizeof(struct uci_packet_header) +
                             uci_get_message_max_payload_length(get_mt((struct uci_packet_header*)packet));
    if (packet_len > header_only_max) {
        printf("Error: UCI packet too large (got %zu, max %zu).\n", packet_len, header_only_max);
        UCI_LOG_ERROR("parse_uci_packet: Packet too large", UCI_ERROR_INVALID_PARAM);
        return;
    }

    struct uci_packet_header* header = (struct uci_packet_header*)packet;
    uci_header_fields_t header_fields;
    uci_error_t result = uci_extract_header_fields_safe(header, &header_fields);
    if (result != UCI_SUCCESS) {
        printf("Error: Failed to extract header fields from UCI packet (error code: %d).\n", result);
        UCI_LOG_ERROR("parse_uci_packet: Header field extraction failed", result);
        return;
    }

    size_t available_payload = packet_len - sizeof(struct uci_packet_header);
    size_t payload_len = header_fields.payload_length;
    if (payload_len > available_payload) {
        printf("  Warning: Header payload length %zu exceeds available data %zu. Clamping.\n",
               payload_len, available_payload);
        UCI_LOG_WARNING("parse_uci_packet: Payload length exceeds available data, clamping from %zu to %zu", 
                        payload_len, available_payload);
        payload_len = available_payload;
    }
    payload_ptr = packet + sizeof(struct uci_packet_header);

    if (header_fields.message_type != DATA && header_fields.packet_boundary == NOT_COMPLETE) {
        if (!segmented_message_matches(&header_fields)) {
            if (g_segmented_message_state.active) {
                printf("Warning: Dropping incomplete segmented UCI message due to fragment mismatch.\n");
                UCI_LOG_WARNING("parse_uci_packet: Dropping incomplete segmented message due to fragment mismatch");
                reset_segmented_message_state();
            }

            g_segmented_message_state.active = 1;
            g_segmented_message_state.fields = header_fields;
            memcpy(g_segmented_message_state.first_header, packet, sizeof(struct uci_packet_header));
            g_segmented_message_state.payload_len = 0;
        }

        if (g_segmented_message_state.payload_len + payload_len > sizeof(g_segmented_message_state.payload)) {
            printf("Error: Segmented UCI message exceeds reassembly buffer (%zu + %zu).\n",
                   g_segmented_message_state.payload_len, payload_len);
            UCI_LOG_ERROR("parse_uci_packet: Segmented message exceeds reassembly buffer", UCI_ERROR_INVALID_PARAM);
            reset_segmented_message_state();
            return;
        }

        memcpy(g_segmented_message_state.payload + g_segmented_message_state.payload_len,
               payload_ptr,
               payload_len);
        g_segmented_message_state.payload_len += payload_len;

        printf("Buffered segmented UCI fragment:\n");
        printf("  MT: 0x%01X\n", header_fields.message_type);
        printf("  GID: 0x%01X\n", header_fields.group_id);
        printf("  Opcode: 0x%02X\n", header_fields.opcode_id);
        printf("  Fragment Length: %zu\n", payload_len);
        printf("  Reassembled Length So Far: %zu\n", g_segmented_message_state.payload_len);
        return;
    }

    if (g_segmented_message_state.active && header_fields.message_type != DATA) {
        if (!segmented_message_matches(&header_fields)) {
            printf("Warning: Dropping incomplete segmented UCI message due to final-fragment mismatch.\n");
            UCI_LOG_WARNING("parse_uci_packet: Final fragment mismatch, dropping segmented message");
            reset_segmented_message_state();
        } else {
            if (g_segmented_message_state.payload_len + payload_len > sizeof(g_segmented_message_state.payload)) {
                printf("Error: Segmented UCI message exceeds reassembly buffer (%zu + %zu).\n",
                       g_segmented_message_state.payload_len, payload_len);
                UCI_LOG_ERROR("parse_uci_packet: Segmented message exceeds reassembly buffer", UCI_ERROR_INVALID_PARAM);
                reset_segmented_message_state();
                return;
            }

            memcpy(g_segmented_message_state.payload + g_segmented_message_state.payload_len,
                   payload_ptr,
                   payload_len);
            g_segmented_message_state.payload_len += payload_len;
            header_fields.packet_boundary = COMPLETE;
            header_fields.payload_length = (uci_uint16)g_segmented_message_state.payload_len;
            payload_ptr = g_segmented_message_state.payload;
            payload_len = g_segmented_message_state.payload_len;
            header_bytes = g_segmented_message_state.first_header;
            segmented_reassembled = 1;
        }
    }

    if (!header_bytes) {
        header_bytes = packet;
    }

    if (header_fields.message_type == NOTIFICATION && g_notification_callback) {
        g_notification_callback(header, &header_fields, payload_ptr, payload_len);
    }

    if (segmented_reassembled) {
        printf("Received reassembled segmented UCI packet:\n");
    } else {
        printf("Received UCI packet:\n");
    }
    printf("  MT: 0x%01X\n", header_fields.message_type);
    printf("  PBF: 0x%01X\n", header_fields.packet_boundary);
    printf("  GID: 0x%01X\n", header_fields.group_id);
    printf("  Opcode: 0x%02X\n", header_fields.opcode_id);
    printf("  Payload Length: %zu\n", payload_len);

    if (payload_len > 0) {
        printf("  Payload: ");
        for (size_t i = 0; i < payload_len; i++) {
            printf("%02X ", payload_ptr[i]);
        }
        printf("\n");
    }

    // Use unified packet analyzer for consistent decoding across the codebase
    printf("\n");  // Add blank line before detailed analysis
    uci_analyze_packet_fields(&header_fields,
                              header_bytes,
                              payload_ptr,
                              payload_len,
                              segmented_reassembled);
    
    // Log detailed packet information for debugging
    UCI_LOG_DEBUG("parse_uci_packet: Received packet - MT:0x%02X GID:0x%02X OID:0x%02X PBF:0x%02X Len:%zu", 
                  header_fields.message_type, header_fields.group_id, header_fields.opcode_id,
                  header_fields.packet_boundary, payload_len);

    if (segmented_reassembled) {
        reset_segmented_message_state();
    }
}
