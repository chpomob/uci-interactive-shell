#include <string.h>

#include "../include/uci_cli.h"
#include "../include/uci_cmd_analysis.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_cmd_hardware.h"
#include "../include/uci_cmd_session.h"
#include "../include/uci_cmd_session_config.h"
#include "../include/uci_cmd_session_config_ext.h"
#include "../include/uci_command_framework.h"
#include "../include/uci_ui.h"
#include "../include/uci_pdl.h"

int cmd_hw_init(int argc, char** argv) {
    char* device_path = (argc > 1) ? argv[1] : NULL;
    return handle_hw_init_command(device_path);
}

int cmd_hw_send(int argc, char** argv) {
    char** payload_tokens = (argc > 5) ? &argv[5] : NULL;
    int payload_count = (argc > 5) ? (argc - 5) : 0;

    return handle_hw_send_command(
        (argc > 1) ? argv[1] : NULL,
        (argc > 2) ? argv[2] : NULL,
        (argc > 3) ? argv[3] : NULL,
        (argc > 4) ? argv[4] : NULL,
        payload_tokens,
        payload_count);
}

int cmd_mode_sim(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_mode_sim_command();
    return 0;
}

int cmd_mode_hw(int argc, char** argv) {
    char* device_path = (argc > 1) ? argv[1] : NULL;
    return handle_mode_hw_command(device_path);
}

int cmd_mode_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_mode_info_command();
    return 0;
}

int cmd_get_device_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_device_info_command();
    return 0;
}

int cmd_device_reset(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_reset_command();
    return 0;
}

int cmd_set_power(int argc, char** argv) {
    char* power_state = (argc > 1) ? argv[1] : NULL;
    return handle_set_power_command(power_state);
}

int cmd_device_on(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_on_command();
    return 0;
}

int cmd_device_off(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_off_command();
    return 0;
}

int cmd_get_caps_info(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_caps_info_command();
    return 0;
}

int cmd_get_config(int argc, char** argv) {
    char* config_name = (argc > 1) ? argv[1] : NULL;
    return handle_get_config_command(config_name);
}

int cmd_get_device_state(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_get_device_state_command();
    return 0;
}

int cmd_set_device_active(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_set_device_active_command();
    return 0;
}

int cmd_set_device_ready(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_set_device_ready_command();
    return 0;
}

int cmd_set_config(int argc, char** argv) {
    char* config_name = (argc > 1) ? argv[1] : NULL;
    char* value_str = (argc > 2) ? argv[2] : NULL;
    return handle_set_config_command(config_name, value_str);
}

int cmd_device_suspend(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_device_suspend_command();
    return 0;
}

int cmd_query_timestamp(int argc, char** argv) {
    (void)argc;
    (void)argv;
    handle_query_timestamp_command();
    return 0;
}

int cmd_session_init(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* type_param = uci_cmd_get_parsed_param(1);

    if (session_param && session_param->present &&
        type_param && type_param->present &&
        type_param->type == PARAM_TYPE_SESSION_TYPE) {
        return handle_session_init_command_values(session_param->value.session_id,
                                                  (SessionType)type_param->value.session_type);
    }
    ui_print_error("session_init requires a numeric session_id and session_type");
    return -1;
}

int cmd_session_deinit(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_deinit_command_value(session_param->value.session_id);
    }
    ui_print_error("session_deinit requires a session_id");
    return -1;
}

int cmd_session_start(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_start_command_value(session_param->value.session_id);
    }
    ui_print_error("session_start requires a session_id");
    return -1;
}

int cmd_session_stop(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_stop_command_value(session_param->value.session_id);
    }
    ui_print_error("session_stop requires a session_id");
    return -1;
}

int cmd_session_send_data(int argc, char** argv) {
    (void)argc;
    (void)argv;
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
    ui_print_error("session_send_data requires session_id, destination, sequence, and payload bytes");
    return -1;
}

int cmd_session_logical_link_create(int argc, char** argv) {
    (void)argc;
    (void)argv;
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
    ui_print_error("session_logical_link_create requires session_id and link_id");
    return -1;
}

int cmd_session_logical_link_close(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* link_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && link_param && link_param->present) {
        return handle_session_logical_link_close_command_value(session_param->value.session_id,
                                                               link_param->value.u8);
    }
    ui_print_error("session_logical_link_close requires session_id and link_id");
    return -1;
}

int cmd_session_logical_link_get_param(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* link_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && link_param && link_param->present) {
        return handle_session_logical_link_get_param_command_value(session_param->value.session_id,
                                                                   link_param->value.u8);
    }
    ui_print_error("session_logical_link_get_param requires session_id and link_id");
    return -1;
}

int cmd_get_session_state(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_get_session_state_command_value(session_param->value.session_id);
    }
    ui_print_error("get_session_state requires a session_id");
    return -1;
}

int cmd_set_app_config(int argc, char** argv) {
    (void)argc;
    (void)argv;
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
    ui_print_error("set_app_config requires session_id, config name, and value");
    return -1;
}

int cmd_get_app_config(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* config_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present &&
        config_param && config_param->present && config_param->raw_value) {
        return handle_get_app_config_command_value(session_param->value.session_id,
                                                   config_param->raw_value);
    }
    ui_print_error("get_app_config now expects a single config name; use hex script for multiple IDs");
    return -1;
}

int cmd_session_update_multicast_list(int argc, char** argv) {
    (void)argc;
    (void)argv;
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
    ui_print_error("session_update_multicast_list requires session_id, action, short address, subsession_id");
    return -1;
}

int cmd_session_update_dt_tag_rounds(int argc, char** argv) {
    (void)argc;
    (void)argv;
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

    ui_print_error("session_update_dt_tag_rounds now expects hex-encoded round bytes");
    return -1;
}

int cmd_session_data_transfer_phase_config(int argc, char** argv) {
    (void)argc;
    (void)argv;
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
            const unsigned char* payload_bytes = NULL;
            size_t payload_len = 0;
            if (has_payload) {
                payload_bytes = payload_param->value.hex_bytes;
                payload_len = payload_param->parsed_length;
            }

            return handle_session_data_transfer_phase_config_command_values(
                session_param->value.session_id,
                repetition_param->value.u8,
                control_param->value.u8,
                size_param->value.u8,
                payload_bytes,
                payload_len);
        }
    }

    ui_print_error("session_data_transfer_phase_config requires session_id, repetition, control, size, payload bytes");
    return -1;
}

int cmd_session_set_hybrid_controller_config(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* data_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && data_param && data_param->present) {
        return handle_session_set_hybrid_controller_config_command_value(
            session_param->value.session_id,
            data_param->raw_value,
            data_param->raw_length);
    }

    ui_print_error("session_set_hybrid_controller_config requires session_id and hex config data");
    return -1;
}

int cmd_session_set_hybrid_controlee_config(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* data_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && data_param && data_param->present) {
        return handle_session_set_hybrid_controlee_config_command_value(
            session_param->value.session_id,
            data_param->raw_value,
            data_param->raw_length);
    }

    ui_print_error("session_set_hybrid_controlee_config requires session_id and hex config data");
    return -1;
}

int cmd_session_query_data_size_in_ranging(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_query_data_size_in_ranging_command_value(session_param->value.session_id);
    }

    ui_print_error("session_query_data_size_in_ranging requires a session_id");
    return -1;
}


int cmd_analyze_packet(int argc, char** argv) {
    if (argc <= 1) {
        handle_analyze_command(0, NULL);
    } else {
        handle_analyze_command(argc - 1, &argv[1]);
    }
    return 0;
}

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    cli_print_help();
    return 0;
}
