#include <stdio.h>
#include <string.h>
#include "../include/uci.h"
#include "../include/uci_functions.h"
#include "../include/uci_config_manager.h"
#include "../include/uci_packet_utils.h"

#define MAX_RESPONSE_PAYLOAD_LEN 255

// External globals from uci.c
extern unsigned long long g_fake_timestamp;

// Helper functions from uci.c
extern int is_valid_device_config_id(unsigned char cfg_id);
extern void enqueue_notification(unsigned char gid, unsigned char oid, unsigned char* payload, int payload_len);

int build_core_device_info_response(unsigned char* response_payload, size_t max_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);

    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0 ||
        uci_payload_builder_put_u16_le(&builder, 0x0100) < 0 ||  // uci_version 1.0
        uci_payload_builder_put_u16_le(&builder, 0x0200) < 0 ||  // mac_version 2.0
        uci_payload_builder_put_u16_le(&builder, 0x0200) < 0 ||  // phy_version 2.0
        uci_payload_builder_put_u16_le(&builder, 0x0100) < 0 ||  // uci_test_version 1.0
        uci_payload_builder_put_u8(&builder, 0x00) < 0) {        // vendor_spec_info count
        return 0;
    }

    return (int)uci_payload_builder_length(&builder);
}

int build_core_get_caps_info_response(unsigned char* response_payload, size_t max_len) {
    size_t caps_payload_len = uci_build_core_capabilities_payload(response_payload, max_len);

    if (caps_payload_len == 0) {
        // Fallback to simple status to avoid emitting a malformed packet
        response_payload[0] = UCI_STATUS_FAILED;
        return 1;
    }

    return (int)caps_payload_len;
}

int build_core_query_timestamp_response(unsigned char* response_payload, size_t max_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);

    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0 ||
        uci_payload_builder_put_u64_le(&builder, g_fake_timestamp++) < 0) {
        return 0;
    }

    return (int)uci_payload_builder_length(&builder);
}

int build_core_get_config_response(unsigned char* response_payload, size_t max_len,
                                   const unsigned char* payload, int payload_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);

    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0 ||
        uci_payload_builder_put_u8(&builder, 0x00) < 0) {
        return 0;
    }

    unsigned char cfg_count = 0;
    unsigned char status = UCI_STATUS_OK;

    if (!payload || payload_len < 1) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        response_payload[1] = 0;
        return 2;
    }

    unsigned char requested = payload[0];
    unsigned int available_ids = payload_len > 1 ? (payload_len - 1) : 0;
    if (requested > available_ids) {
        requested = (unsigned char)available_ids;
    }

    for (unsigned char i = 0; i < requested; i++) {
        unsigned char cfg_id = payload[1 + i];
        if (!is_valid_device_config_id(cfg_id)) {
            status = UCI_STATUS_INVALID_PARAM;
            continue;
        }

        unsigned char value_buffer[256] = {0};
        size_t value_len = sizeof(value_buffer);
        if (uci_config_get_device_param((DeviceConfigId)cfg_id, value_buffer, &value_len) != 0 ||
            value_len == 0 || value_len > 255) {
            status = UCI_STATUS_INVALID_PARAM;
            continue;
        }

        if (uci_payload_builder_put_u8(&builder, cfg_id) < 0 ||
            uci_payload_builder_put_u8(&builder, (unsigned char)value_len) < 0 ||
            uci_payload_builder_put_mem(&builder, value_buffer, value_len) < 0) {
            status = UCI_STATUS_INVALID_MSG_SIZE;
            break;
        }
        cfg_count++;
    }

    if (cfg_count == 0 && status == UCI_STATUS_OK) {
        status = UCI_STATUS_INVALID_PARAM;
    }

    response_payload[0] = (cfg_count > 0) ? status : UCI_STATUS_INVALID_PARAM;
    response_payload[1] = cfg_count;

    if (uci_payload_builder_length(&builder) == 2) {
        unsigned char err = response_payload[0];
        enqueue_notification(CORE, CORE_GENERIC_ERROR_NTF, &err, 1);
    }

    return (int)uci_payload_builder_length(&builder);
}

int build_core_set_config_response(unsigned char* response_payload, size_t max_len,
                                   const unsigned char* payload, int payload_len) {
    unsigned char cfg_ids[MAX_RESPONSE_PAYLOAD_LEN] = {0};
    unsigned char cfg_status[MAX_RESPONSE_PAYLOAD_LEN] = {0};
    unsigned char processed_tlvs = 0;
    unsigned char status = UCI_STATUS_OK;

    if (!payload || payload_len < 1) {
        response_payload[0] = UCI_STATUS_INVALID_PARAM;
        response_payload[1] = 0;
        return 2;
    }

    unsigned char declared_tlvs = payload[0];
    int offset = 1;

    for (unsigned char i = 0; i < declared_tlvs && offset < payload_len; i++) {
        if (offset + 2 > payload_len) {
            cfg_status[i] = UCI_STATUS_INVALID_MSG_SIZE;
            status = UCI_STATUS_INVALID_MSG_SIZE;
            break;
        }

        unsigned char cfg_id = payload[offset++];
        unsigned char len = payload[offset++];
        cfg_ids[i] = cfg_id;

        if (offset + len > payload_len) {
            cfg_status[i] = UCI_STATUS_INVALID_MSG_SIZE;
            status = UCI_STATUS_INVALID_MSG_SIZE;
            break;
        }

        if (!is_valid_device_config_id(cfg_id)) {
            cfg_status[i] = UCI_STATUS_INVALID_PARAM;
            status = UCI_STATUS_INVALID_PARAM;
            offset += len;
            processed_tlvs++;
            continue;
        }

        if (uci_config_set_device_param((DeviceConfigId)cfg_id, &payload[offset], len) != 0) {
            cfg_status[i] = UCI_STATUS_FAILED;
            status = UCI_STATUS_FAILED;
        } else {
            cfg_status[i] = UCI_STATUS_OK;
        }

        offset += len;
        processed_tlvs++;
    }

    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);

    if (uci_payload_builder_put_u8(&builder, status) < 0 ||
        uci_payload_builder_put_u8(&builder, processed_tlvs) < 0) {
        return 0;
    }

    for (unsigned char i = 0; i < processed_tlvs; i++) {
        if (uci_payload_builder_put_u8(&builder, cfg_ids[i]) < 0 ||
            uci_payload_builder_put_u8(&builder, cfg_status[i]) < 0) {
            return 0;
        }
    }

    response_payload[0] = status;
    response_payload[1] = processed_tlvs;

    return (int)uci_payload_builder_length(&builder);
}

int build_core_device_reset_response(unsigned char* response_payload, size_t max_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);
    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0) {
        return 0;
    }
    return (int)uci_payload_builder_length(&builder);
}

int build_core_device_suspend_response(unsigned char* response_payload, size_t max_len) {
    struct uci_payload_builder builder;
    uci_payload_builder_init(&builder, response_payload, max_len);
    if (uci_payload_builder_put_u8(&builder, UCI_STATUS_OK) < 0) {
        return 0;
    }
    return (int)uci_payload_builder_length(&builder);
}
