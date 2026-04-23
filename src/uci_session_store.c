#include "../include/uci_session_store.h"

#include <string.h>

#include "../include/uci_functions.h"
#include "../include/uci_standardized_error_handling.h"

void uci_session_store_reset_all(void) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        uci_sessions[i].session_id = 0;
        uci_sessions[i].session_type = 0;
        uci_sessions[i].is_allocated = 0;
        uci_sessions[i].session_state = SESSION_STATE_DEINIT;
        uci_sessions[i].num_configs = 0;
        uci_sessions[i].session_handle = 0;
        uci_sessions[i].ranging_count = 0;
        memset(uci_sessions[i].configs, 0, sizeof(uci_sessions[i].configs));
        uci_sessions[i].multicast_count = 0;
        memset(uci_sessions[i].multicast_entries, 0, sizeof(uci_sessions[i].multicast_entries));
        uci_sessions[i].dt_tag_round_count = 0;
        memset(uci_sessions[i].dt_tag_round_indexes, 0, sizeof(uci_sessions[i].dt_tag_round_indexes));
        uci_sessions[i].dtp_repetition = 0;
        uci_sessions[i].dtp_control = 0;
        uci_sessions[i].dtp_size = 0;
        uci_sessions[i].dtp_payload_len = 0;
        memset(uci_sessions[i].dtp_payload, 0, sizeof(uci_sessions[i].dtp_payload));
        uci_sessions[i].logical_link_count = 0;
        memset(uci_sessions[i].logical_links, 0, sizeof(uci_sessions[i].logical_links));
        uci_sessions[i].last_data_sequence = 0;
        uci_sessions[i].last_data_length = 0;
        uci_sessions[i].last_data_destination = 0;
        uci_sessions[i].last_data_preview_len = 0;
        memset(uci_sessions[i].last_data_preview, 0,
               sizeof(uci_sessions[i].last_data_preview));
    }
}

int find_free_session_slot() {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!uci_sessions[i].is_allocated) {
            return i;
        }
    }
    return -1;
}

int find_session_by_id(uci_uint32 session_id) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (uci_sessions[i].is_allocated && uci_sessions[i].session_id == session_id) {
            return i;
        }
    }
    return -1;
}

int find_session_by_handle(uci_uint32 session_handle) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (uci_sessions[i].is_allocated && uci_sessions[i].session_handle == session_handle) {
            return i;
        }
    }
    return -1;
}

int find_session_by_token_or_id(unsigned int identifier) {
    int idx = find_session_by_handle(identifier);
    if (idx >= 0) {
        return idx;
    }
    return find_session_by_id(identifier);
}

int get_allocated_session_count() {
    int count = 0;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (uci_sessions[i].is_allocated) {
            count++;
        }
    }
    return count;
}

void increment_session_ranging_count(int session_idx) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) {
        return;
    }
    if (!uci_sessions[session_idx].is_allocated) {
        return;
    }
    if (uci_sessions[session_idx].ranging_count < 0xFFFF) {
        uci_sessions[session_idx].ranging_count++;
    }
}

void uci_session_clear_logical_links(struct uci_session* session) {
    session->logical_link_count = 0;
    memset(session->logical_links, 0, sizeof(session->logical_links));
}

uci_logical_link_entry* uci_session_find_logical_link(struct uci_session* session,
                                                      unsigned char link_id) {
    for (unsigned char i = 0; i < session->logical_link_count; i++) {
        uci_logical_link_entry* entry = &session->logical_links[i];
        if (entry->active && entry->link_id == link_id) {
            return entry;
        }
    }
    return NULL;
}

uci_logical_link_entry* uci_session_allocate_logical_link(struct uci_session* session,
                                                          unsigned char requested_id,
                                                          unsigned char* assigned_id) {
    if (session->logical_link_count >= MAX_LOGICAL_LINKS) {
        return NULL;
    }

    if (requested_id != 0xFF) {
        *assigned_id = requested_id;
    } else {
        unsigned char candidate = 0;
        while (uci_session_find_logical_link(session, candidate) && candidate < 0xFF) {
            candidate++;
        }
        if (candidate == 0xFF && uci_session_find_logical_link(session, candidate)) {
            return NULL;
        }
        *assigned_id = candidate;
    }

    uci_logical_link_entry* entry = &session->logical_links[session->logical_link_count++];
    memset(entry, 0, sizeof(*entry));
    entry->link_id = *assigned_id;
    entry->active = 1;
    return entry;
}

static int uci_session_find_multicast_entry_idx(const struct uci_session* session,
                                                unsigned short short_address,
                                                unsigned int subsession_id) {
    if (!session) {
        return -1;
    }
    for (int i = 0; i < session->multicast_count; i++) {
        const uci_multicast_entry* entry = &session->multicast_entries[i];
        if (entry->short_address == short_address && entry->subsession_id == subsession_id) {
            return i;
        }
    }
    return -1;
}

unsigned char uci_session_add_multicast_entry(struct uci_session* session,
                                              unsigned short short_address,
                                              unsigned int subsession_id,
                                              const unsigned char* key,
                                              unsigned char key_len) {
    if (!session) {
        return UCI_STATUS_INVALID_PARAM;
    }

    if (key_len > sizeof(session->multicast_entries[0].key)) {
        key_len = sizeof(session->multicast_entries[0].key);
    }

    int existing_idx = uci_session_find_multicast_entry_idx(session, short_address, subsession_id);
    if (existing_idx >= 0) {
        uci_multicast_entry* entry = &session->multicast_entries[existing_idx];
        entry->key_len = key_len;
        if (key_len > 0 && key) {
            memcpy(entry->key, key, key_len);
        } else {
            memset(entry->key, 0, sizeof(entry->key));
        }
        return UCI_STATUS_OK;
    }

    if (session->multicast_count >= MAX_MULTICAST_CONTROLEES) {
        return UCI_STATUS_MULTICAST_LIST_FULL;
    }

    uci_multicast_entry* entry = &session->multicast_entries[session->multicast_count++];
    entry->short_address = short_address;
    entry->subsession_id = subsession_id;
    entry->key_len = key_len;
    if (key_len > 0 && key) {
        memcpy(entry->key, key, key_len);
    } else {
        memset(entry->key, 0, sizeof(entry->key));
    }
    return UCI_STATUS_OK;
}

unsigned char uci_session_remove_multicast_entry(struct uci_session* session,
                                                 unsigned short short_address,
                                                 unsigned int subsession_id) {
    if (!session) {
        return UCI_STATUS_INVALID_PARAM;
    }

    int idx = uci_session_find_multicast_entry_idx(session, short_address, subsession_id);
    if (idx < 0) {
        return UCI_STATUS_ADDRESS_NOT_FOUND;
    }

    int last = session->multicast_count - 1;
    if (idx != last) {
        session->multicast_entries[idx] = session->multicast_entries[last];
    }
    memset(&session->multicast_entries[last], 0, sizeof(session->multicast_entries[last]));
    session->multicast_count--;
    return UCI_STATUS_OK;
}

void store_session_config(int session_idx, uci_uint8 cfg_id, uci_uint8* value, uci_uint8 len) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) {
        UCI_LOG_ERROR("Invalid session index %d in store_session_config", session_idx);
        uci_log_error(__func__, "Invalid session index in store_session_config", UCI_ERROR_INVALID_PARAM);
        return;
    }
    if (!value) {
        UCI_LOG_ERROR("Null value pointer in store_session_config");
        uci_log_error(__func__, "Null value pointer in store_session_config", UCI_ERROR_INVALID_PARAM);
        return;
    }
    if (len == 0) {
        UCI_LOG_ERROR("Zero length configuration value in store_session_config");
        uci_log_error(__func__, "Zero length configuration value", UCI_ERROR_INVALID_PARAM);
        return;
    }

    struct uci_session* session = &uci_sessions[session_idx];

    for (int i = 0; i < MAX_SESSION_CONFIGS; i++) {
        uci_session_config_entry* entry = &session->configs[i];
        if (entry->in_use && entry->cfg_id == cfg_id) {
            entry->length = len;
            memcpy(entry->value, value, len);
            return;
        }
    }

    for (int i = 0; i < MAX_SESSION_CONFIGS; i++) {
        uci_session_config_entry* entry = &session->configs[i];
        if (!entry->in_use) {
            entry->in_use = 1;
            entry->cfg_id = cfg_id;
            entry->length = len;
            memcpy(entry->value, value, len);
            if (session->num_configs < MAX_SESSION_CONFIGS) {
                session->num_configs++;
            }
            return;
        }
    }

    UCI_LOG_WARNING("No space to store session config 0x%02X", cfg_id);
}

int get_session_config(int session_idx, uci_uint8 cfg_id, uci_uint8* value, uci_uint8* len) {
    if (session_idx < 0 || session_idx >= MAX_SESSIONS) {
        UCI_LOG_ERROR("Invalid session index %d in get_session_config", session_idx);
        uci_log_error(__func__, "Invalid session index in get_session_config", UCI_ERROR_INVALID_PARAM);
        return 0;
    }
    if (!len) {
        UCI_LOG_ERROR("Null length pointer in get_session_config");
        uci_log_error(__func__, "Null length pointer in get_session_config", UCI_ERROR_INVALID_PARAM);
        return 0;
    }

    struct uci_session* session = &uci_sessions[session_idx];
    for (int i = 0; i < MAX_SESSION_CONFIGS; i++) {
        uci_session_config_entry* entry = &session->configs[i];
        if (entry->in_use && entry->cfg_id == cfg_id) {
            unsigned char available = *len;
            *len = entry->length;
            if (value && available > 0) {
                size_t copy_len = entry->length;
                if (copy_len > available) {
                    copy_len = available;
                    UCI_LOG_WARNING("Buffer too small for session config 0x%02X (need %u bytes)",
                                    cfg_id, entry->length);
                }
                memcpy(value, entry->value, copy_len);
            }
            return 1;
        }
    }

    *len = 0;
    return 0;
}
