#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../include/uci.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_functions.h"
#include "../include/uci_packet_utils.h"
#include "../include/uci_response_core.h"
#include "../include/uci_session_store.h"
#include "../include/uci_simulator.h"
#include "../include/uci_standardized_error_handling.h"

#define MAX_RESPONSE_PAYLOAD_LEN 255
#define UCI_SIM_MAX_QUEUED_PACKETS 8
#define UCI_SIM_PACKET_STORAGE (sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN)
#define MAX_PENDING_NOTIFICATIONS 16

struct sim_packet_entry {
    size_t len;
    unsigned char data[UCI_SIM_PACKET_STORAGE];
};

typedef struct {
    unsigned char data[sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN];
    size_t length;
} notification_item_t;

static struct sim_packet_entry g_sim_packet_queue[UCI_SIM_MAX_QUEUED_PACKETS];
static size_t g_sim_queue_head = 0;
static size_t g_sim_queue_tail = 0;
static size_t g_sim_queue_count = 0;
static unsigned char g_sim_device_state = DEVICE_STATE_READY;
static notification_item_t g_notification_queue[MAX_PENDING_NOTIFICATIONS];
static size_t g_notification_head = 0;
static size_t g_notification_tail = 0;
static unsigned int g_session_handle_counter = 1;
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

void uci_simulator_reset_runtime(void)
{
    sim_queue_reset();
    g_notification_head = 0;
    g_notification_tail = 0;
    g_session_handle_counter = 1;
}

void uci_simulator_dispatch_command(uint8_t gid,
                                    uint8_t oid,
                                    const unsigned char *payload,
                                    size_t payload_len)
{
    if (!sim_command_allowed(gid, oid)) {
        send_sim_status(gid, oid, UCI_STATUS_REJECTED);
        UCI_LOG_ERROR("Command not allowed", UCI_ERROR_INVALID_PARAM);
        return;
    }

    const struct uci_command_handler_entry *handler = find_sim_handler(gid, oid);
    if (!handler) {
        uint8_t status = sim_gid_is_known(gid) ? UCI_STATUS_UNKNOWN_OID : UCI_STATUS_UNKNOWN_GID;
        send_sim_status(gid, oid, status);
        UCI_LOG_ERROR("Handler not found for GID: 0x%02X, OID: 0x%02X", gid, oid);
        return;
    }

    unsigned char response_packet[sizeof(struct uci_packet_header) + MAX_RESPONSE_PAYLOAD_LEN];
    struct uci_packet_header *response_header = (struct uci_packet_header *)response_packet;
    unsigned char *response_payload = response_packet + sizeof(struct uci_packet_header);

    int generated_len =
        handler->handler(response_payload, MAX_RESPONSE_PAYLOAD_LEN, payload, payload_len);
    if (generated_len < 0) {
        send_sim_status(gid, oid, UCI_STATUS_FAILED);
        UCI_LOG_ERROR("Handler function failed", UCI_ERROR_INVALID_PARAM);
        return;
    }

    if ((size_t)generated_len > MAX_RESPONSE_PAYLOAD_LEN) {
        send_sim_status(gid, oid, UCI_STATUS_INVALID_MSG_SIZE);
        UCI_LOG_ERROR("Response payload too large", UCI_ERROR_BUFFER_OVERFLOW);
        return;
    }

    uci_error_t result = set_header_values_safe(response_header, RESPONSE, COMPLETE, gid, oid,
                                                (uci_uint16)generated_len);
    if (result != UCI_SUCCESS) {
        UCI_LOG_ERROR("Failed to set response header values (error=%d)", result);
        uci_log_error(__func__, "Response header validation failed", result);
        return;
    }

    size_t total_len = sizeof(struct uci_packet_header) + (size_t)generated_len;
    if (sim_queue_enqueue(response_packet, total_len) == 0) {
        simulator_flush_queue();
    } else {
        parse_uci_packet(response_packet, total_len);
    }
}

void uci_simulator_handle_data_message_send(const unsigned char *payload, size_t payload_len)
{
    if (!payload || payload_len < UCI_DATA_MESSAGE_SND_HEADER) {
        UCI_LOG_ERROR("DATA_MESSAGE_SND payload too short (len=%zu)", payload_len);
        uci_log_error(__func__, "DATA_MESSAGE_SND payload too short", UCI_ERROR_INVALID_PARAM);
        return;
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
        credit_payload[4] = 1;
        enqueue_notification(SESSION_CONTROL, SESSION_DATA_CREDIT_NTF, credit_payload,
                             sizeof(credit_payload));
    }

    unsigned char status_payload[8];
    write_u32_le(status_payload, notification_session);
    write_u16_le(&status_payload[4], sequence_number);
    status_payload[6] = transfer_status;
    status_payload[7] = 1;
    enqueue_notification(SESSION_CONTROL, SESSION_DATA_TRANSFER_STATUS_NTF, status_payload,
                         sizeof(status_payload));
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
            if (uci_config_get_device_param_name((DeviceConfigId)cfg_id) == NULL) {
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
