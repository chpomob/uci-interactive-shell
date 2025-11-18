#include <string.h>

#include "../include/uci_cli.h"
#include "../include/uci_cmd_analysis.h"
#include "../include/uci_cmd_core.h"
#include "../include/uci_cmd_hardware.h"
#include "../include/uci_cmd_session.h"
#include "../include/uci_cmd_session_config.h"
#include "../include/uci_command_framework.h"
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
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* type_param = uci_cmd_get_parsed_param(1);

    if (session_param && session_param->present &&
        type_param && type_param->present &&
        type_param->type == PARAM_TYPE_SESSION_TYPE) {
        return handle_session_init_command_values(session_param->value.session_id,
                                                  (SessionType)type_param->value.session_type);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* session_type_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_init_command(session_id_str, session_type_str);
}

int cmd_session_deinit(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_deinit_command_value(session_param->value.session_id);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_deinit_command(session_id_str);
}

int cmd_session_start(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_start_command_value(session_param->value.session_id);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_start_command(session_id_str);
}

int cmd_session_stop(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_stop_command_value(session_param->value.session_id);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_stop_command(session_id_str);
}

int cmd_session_send_data(int argc, char** argv) {
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

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* destination_str = (argc > 2) ? argv[2] : NULL;
    char* sequence_str = (argc > 3) ? argv[3] : NULL;
    char* payload_str = (argc > 4) ? argv[4] : NULL;
    return handle_session_send_data_command(session_id_str, destination_str, sequence_str, payload_str);
}

int cmd_session_logical_link_create(int argc, char** argv) {
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

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    char* mode_str = (argc > 3) ? argv[3] : NULL;
    char* credit_str = (argc > 4) ? argv[4] : NULL;
    return handle_session_logical_link_create_command(session_id_str, link_id_str, mode_str, credit_str);
}

int cmd_session_logical_link_close(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* link_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && link_param && link_param->present) {
        return handle_session_logical_link_close_command_value(session_param->value.session_id,
                                                               link_param->value.u8);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_logical_link_close_command(session_id_str, link_id_str);
}

int cmd_session_logical_link_get_param(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* link_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && link_param && link_param->present) {
        return handle_session_logical_link_get_param_command_value(session_param->value.session_id,
                                                                   link_param->value.u8);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* link_id_str = (argc > 2) ? argv[2] : NULL;
    return handle_session_logical_link_get_param_command(session_id_str, link_id_str);
}

int cmd_get_session_state(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_get_session_state_command_value(session_param->value.session_id);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_get_session_state_command(session_id_str);
}

int cmd_set_app_config(int argc, char** argv) {
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

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_name = (argc > 2) ? argv[2] : NULL;
    char* value_str = (argc > 3) ? argv[3] : NULL;
    return handle_set_app_config_command(session_id_str, config_name, value_str);
}

int cmd_get_app_config(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* config_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present &&
        config_param && config_param->present && config_param->raw_value) {
        return handle_get_app_config_command_value(session_param->value.session_id,
                                                   config_param->raw_value);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char** config_names = (argc > 2) ? &argv[2] : NULL;
    int config_count = (argc > 2) ? (argc - 2) : 0;
    return handle_get_app_config_command(session_id_str, config_names, config_count);
}

int cmd_session_update_multicast_list(int argc, char** argv) {
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

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* action_str = (argc > 2) ? argv[2] : NULL;
    char* short_address_str = (argc > 3) ? argv[3] : NULL;
    char* subsession_id_str = (argc > 4) ? argv[4] : NULL;
    return handle_update_multicast_list_command(session_id_str, action_str, short_address_str, subsession_id_str);
}

int cmd_session_update_dt_tag_rounds(int argc, char** argv) {
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

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char** round_values = (argc > 2) ? &argv[2] : NULL;
    int round_count = (argc > 2) ? (argc - 2) : 0;
    return handle_session_update_dt_tag_rounds_command(session_id_str, round_values, round_count);
}

int cmd_session_data_transfer_phase_config(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* repetition_param = uci_cmd_get_parsed_param(1);
    const uci_cmd_parsed_param_t* control_param = uci_cmd_get_parsed_param(2);
    const uci_cmd_parsed_param_t* size_param = uci_cmd_get_parsed_param(3);
    const uci_cmd_parsed_param_t* payload_param = uci_cmd_get_parsed_param(4);
    int legacy_payload_count = (argc > 5) ? (argc - 5) : 0;

    if (session_param && session_param->present &&
        repetition_param && repetition_param->present &&
        control_param && control_param->present &&
        size_param && size_param->present) {
        if ((payload_param && payload_param->present) ||
            (size_param->value.u8 == 0 && legacy_payload_count == 0)) {
            const unsigned char* payload_bytes = NULL;
            size_t payload_len = 0;
            if (payload_param && payload_param->present) {
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

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* repetition_str = (argc > 2) ? argv[2] : NULL;
    char* control_str = (argc > 3) ? argv[3] : NULL;
    char* size_str = (argc > 4) ? argv[4] : NULL;
    char** payload_values = (argc > 5) ? &argv[5] : NULL;
    int payload_count = (argc > 5) ? (argc - 5) : 0;
    return handle_session_data_transfer_phase_config_command(session_id_str,
                                                             repetition_str,
                                                             control_str,
                                                             size_str,
                                                             payload_values,
                                                             payload_count);
}

int cmd_session_set_hybrid_controller_config(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* data_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && data_param && data_param->present) {
        return handle_session_set_hybrid_controller_config_command_value(
            session_param->value.session_id,
            data_param->raw_value,
            data_param->raw_length);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_data_str = (argc > 2) ? argv[2] : NULL;
    int config_len = config_data_str ? (int)strlen(config_data_str) : 0;
    return handle_session_set_hybrid_controller_config_command(session_id_str,
                                                               (unsigned char*)config_data_str,
                                                               config_len);
}

int cmd_session_set_hybrid_controlee_config(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    const uci_cmd_parsed_param_t* data_param = uci_cmd_get_parsed_param(1);
    if (session_param && session_param->present && data_param && data_param->present) {
        return handle_session_set_hybrid_controlee_config_command_value(
            session_param->value.session_id,
            data_param->raw_value,
            data_param->raw_length);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    char* config_data_str = (argc > 2) ? argv[2] : NULL;
    int config_len = config_data_str ? (int)strlen(config_data_str) : 0;
    return handle_session_set_hybrid_controlee_config_command(session_id_str,
                                                              (unsigned char*)config_data_str,
                                                              config_len);
}

int cmd_session_query_data_size_in_ranging(int argc, char** argv) {
    const uci_cmd_parsed_param_t* session_param = uci_cmd_get_parsed_param(0);
    if (session_param && session_param->present) {
        return handle_session_query_data_size_in_ranging_command_value(session_param->value.session_id);
    }

    char* session_id_str = (argc > 1) ? argv[1] : NULL;
    return handle_session_query_data_size_in_ranging_command(session_id_str);
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
