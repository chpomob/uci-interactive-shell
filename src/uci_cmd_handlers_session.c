#include <stdio.h>

#include "../include/uci_cmd_session.h"
#include "../include/uci_cmd_session_config.h"
#include "../include/uci_cmd_session_config_ext.h"
#include "../include/uci_cmd_session_typed.h"
#include "../include/uci_command_framework.h"
#include "../include/uci_ui.h"
#include "../include/uci_pdl.h"

static int report_missing_param(const char* cmd_name, const char* requirement) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s requires %s", cmd_name, requirement);
    ui_print_error(buffer);
    return -1;
}

int handle_session_init_command_typed(const char* cmd_name,
                                      int argc,
                                      char** argv,
                                      const uci_param_def_t* params,
                                      int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* type_param = uci_cmd_get_parsed_param(1);

    if (session_param && session_param->present &&
        type_param && type_param->present &&
        type_param->type == PARAM_TYPE_SESSION_TYPE) {
        return handle_session_init_command_values(session_param->value.session_id,
                                                  (SessionType)type_param->value.session_type);
    }
    return report_missing_param("session_init", "session_id and session_type");
}

int handle_session_deinit_command_typed(const char* cmd_name,
                                        int argc,
                                        char** argv,
                                        const uci_param_def_t* params,
                                        int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_deinit_command_value(session_param->value.session_id);
    }
    return report_missing_param("session_deinit", "session_id");
}

int handle_session_start_command_typed(const char* cmd_name,
                                       int argc,
                                       char** argv,
                                       const uci_param_def_t* params,
                                       int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_start_command_value(session_param->value.session_id);
    }
    return report_missing_param("session_start", "session_id");
}

int handle_session_stop_command_typed(const char* cmd_name,
                                      int argc,
                                      char** argv,
                                      const uci_param_def_t* params,
                                      int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_stop_command_value(session_param->value.session_id);
    }
    return report_missing_param("session_stop", "session_id");
}

int handle_session_send_data_command_typed(const char* cmd_name,
                                           int argc,
                                           char** argv,
                                           const uci_param_def_t* params,
                                           int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* dest_param = uci_cmd_get_parsed_param(1);
    const uci_cmd_parsed_param_t* seq_param = uci_cmd_get_parsed_param(2);
    const uci_cmd_parsed_param_t* payload_param = uci_cmd_get_parsed_param(3);
    if (session_param && session_param->present &&
        dest_param && dest_param->present &&
        seq_param && seq_param->present &&
        payload_param && payload_param->present) {
        return handle_session_send_data_command_values(
            session_param->value.session_id,
            dest_param->value.u64,
            seq_param->value.u16,
            payload_param->value.hex_bytes,
            payload_param->parsed_length);
    }
    return report_missing_param("session_send_data",
                                "session_id, destination, sequence, and payload bytes");
}

int handle_session_logical_link_create_command_typed(const char* cmd_name,
                                                     int argc,
                                                     char** argv,
                                                     const uci_param_def_t* params,
                                                     int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* link_param = uci_cmd_get_parsed_param(1);
    const uci_cmd_parsed_param_t* mode_param = uci_cmd_get_parsed_param(2);
    const uci_cmd_parsed_param_t* credit_param = uci_cmd_get_parsed_param(3);
    if (session_param && session_param->present && link_param && link_param->present) {
        bool mode_present = (mode_param && mode_param->present);
        bool credit_present = (credit_param && credit_param->present);
        unsigned char mode_value = mode_present ? mode_param->value.u8 : 0;
        unsigned char credit_value = credit_present ? credit_param->value.u8 : 0;
        return handle_session_logical_link_create_command_values(
            session_param->value.session_id,
            link_param->value.u8,
            mode_present,
            mode_value,
            credit_present,
            credit_value);
    }
    return report_missing_param("session_logical_link_create", "session_id and link_id");
}

int handle_session_logical_link_close_command_typed(const char* cmd_name,
                                                    int argc,
                                                    char** argv,
                                                    const uci_param_def_t* params,
                                                    int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* link_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && link_param && link_param->present) {
        return handle_session_logical_link_close_command_value(session_param->value.session_id,
                                                               link_param->value.u8);
    }
    return report_missing_param("session_logical_link_close", "session_id and link_id");
}

int handle_session_logical_link_get_param_command_typed(const char* cmd_name,
                                                        int argc,
                                                        char** argv,
                                                        const uci_param_def_t* params,
                                                        int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* link_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && link_param && link_param->present) {
        return handle_session_logical_link_get_param_command_value(session_param->value.session_id,
                                                                   link_param->value.u8);
    }
    return report_missing_param("session_logical_link_get_param", "session_id and link_id");
}

int handle_get_session_state_command_typed(const char* cmd_name,
                                           int argc,
                                           char** argv,
                                           const uci_param_def_t* params,
                                           int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_get_session_state_command_value(session_param->value.session_id);
    }
    return report_missing_param("get_session_state", "session_id");
}

int handle_set_app_config_command_typed(const char* cmd_name,
                                        int argc,
                                        char** argv,
                                        const uci_param_def_t* params,
                                        int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* name_param = uci_cmd_get_parsed_param(1);
    const uci_cmd_parsed_param_t* value_param = uci_cmd_get_parsed_param(2);
    if (session_param && session_param->present &&
        name_param && name_param->present && name_param->raw_value &&
        value_param && value_param->present && value_param->raw_value) {
        return handle_set_app_config_command_value(session_param->value.session_id,
                                                   name_param->raw_value,
                                                   value_param->raw_value);
    }
    return report_missing_param("set_app_config", "session_id, config name, and value");
}

int handle_get_app_config_command_typed(const char* cmd_name,
                                        int argc,
                                        char** argv,
                                        const uci_param_def_t* params,
                                        int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        int config_count = (argc > 2) ? (argc - 2) : 0;
        const char* const* config_names = (argc > 2) ? (const char* const*)&argv[2] : NULL;
        return handle_get_app_config_command_values(session_param->value.session_id,
                                                    config_count,
                                                    config_names);
    }
    return report_missing_param("get_app_config", "session_id");
}

int handle_session_update_multicast_list_command_typed(const char* cmd_name,
                                                       int argc,
                                                       char** argv,
                                                       const uci_param_def_t* params,
                                                       int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* action_param = uci_cmd_get_parsed_param(1);
    const uci_cmd_parsed_param_t* short_param = uci_cmd_get_parsed_param(2);
    const uci_cmd_parsed_param_t* subsession_param = uci_cmd_get_parsed_param(3);
    if (session_param && session_param->present &&
        action_param && action_param->present && action_param->raw_value &&
        short_param && short_param->present &&
        subsession_param && subsession_param->present) {
        return handle_update_multicast_list_command_values(session_param->value.session_id,
                                                           action_param->raw_value,
                                                           short_param->value.u16,
                                                           subsession_param->value.u32);
    }
    return report_missing_param("session_update_multicast_list",
                                "session_id, action, short address, subsession_id");
}

int handle_session_update_dt_tag_rounds_command_typed(const char* cmd_name,
                                                      int argc,
                                                      char** argv,
                                                      const uci_param_def_t* params,
                                                      int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* rounds_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && rounds_param && rounds_param->present) {
        const unsigned char* round_bytes =
            (rounds_param->parsed_length > 0) ? rounds_param->value.hex_bytes : NULL;
        return handle_session_update_dt_tag_rounds_command_values(
            session_param->value.session_id,
            round_bytes,
            rounds_param->parsed_length);
    }
    return report_missing_param("session_update_dt_tag_rounds", "session_id and round bytes");
}

int handle_session_data_transfer_phase_config_command_typed(const char* cmd_name,
                                                            int argc,
                                                            char** argv,
                                                            const uci_param_def_t* params,
                                                            int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* repetition_param = uci_cmd_get_parsed_param(1);
    const uci_cmd_parsed_param_t* control_param = uci_cmd_get_parsed_param(2);
    const uci_cmd_parsed_param_t* size_param = uci_cmd_get_parsed_param(3);
    const uci_cmd_parsed_param_t* payload_param = uci_cmd_get_parsed_param(4);

    if (session_param && session_param->present &&
        repetition_param && repetition_param->present &&
        control_param && control_param->present &&
        size_param && size_param->present) {
        const bool has_payload = payload_param && payload_param->present;
        if (has_payload || size_param->value.u8 == 0) {
            const unsigned char* payload_bytes = has_payload ? payload_param->value.hex_bytes : NULL;
            size_t payload_len = has_payload ? payload_param->parsed_length : 0;
            return handle_session_data_transfer_phase_config_command_values(
                session_param->value.session_id,
                repetition_param->value.u8,
                control_param->value.u8,
                size_param->value.u8,
                payload_bytes,
                payload_len);
        }
    }

    return report_missing_param("session_data_transfer_phase_config",
                                "session_id, repetition, control, size, payload bytes");
}

int handle_session_set_hybrid_controller_config_command_typed(const char* cmd_name,
                                                              int argc,
                                                              char** argv,
                                                              const uci_param_def_t* params,
                                                              int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* data_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && data_param && data_param->present) {
        return handle_session_set_hybrid_controller_config_command_value(
            session_param->value.session_id,
            data_param->raw_value,
            data_param->raw_length);
    }
    return report_missing_param("session_set_hybrid_controller_config",
                                "session_id and hex config data");
}

int handle_session_set_hybrid_controlee_config_command_typed(const char* cmd_name,
                                                             int argc,
                                                             char** argv,
                                                             const uci_param_def_t* params,
                                                             int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* data_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && data_param && data_param->present) {
        return handle_session_set_hybrid_controlee_config_command_value(
            session_param->value.session_id,
            data_param->raw_value,
            data_param->raw_length);
    }
    return report_missing_param("session_set_hybrid_controlee_config",
                                "session_id and hex config data");
}

int handle_session_query_data_size_in_ranging_command_typed(const char* cmd_name,
                                                            int argc,
                                                            char** argv,
                                                            const uci_param_def_t* params,
                                                            int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_query_data_size_in_ranging_command_value(session_param->value.session_id);
    }
    return report_missing_param("session_query_data_size_in_ranging", "session_id");
}
