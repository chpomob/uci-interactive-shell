#include <stdio.h>

#include "../include/uci_cmd_hardware_typed.h"
#include "../include/uci_cmd_hardware.h"
#include "../include/uci_ui.h"

static const char* get_optional_string_param(int index) {
    const uci_cmd_parsed_param_t* param = uci_cmd_get_parsed_param(index);
    if (!param || !param->present || !param->raw_value || param->raw_value[0] == '\0') {
        return NULL;
    }
    return param->raw_value;
}

static int get_hex_byte_param(int index, const char* label, unsigned char* out_value) {
    const uci_cmd_parsed_param_t* param = uci_cmd_get_parsed_param(index);
    if (!param || !param->present) {
        if (label) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Missing required parameter: %s", label);
            ui_print_error(buffer);
        }
        return -1;
    }
    *out_value = param->value.u8;
    return 0;
}

int handle_mode_sim_command_typed(const char* cmd_name,
                                int argc,
                                char** argv,
                                const uci_param_def_t* params,
                                int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_mode_sim_command();
    return 0;
}

int handle_mode_hw_command_typed(const char* cmd_name,
                               int argc,
                               char** argv,
                               const uci_param_def_t* params,
                               int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

    const char* device_path = get_optional_string_param(0);
    return handle_mode_hw_command((char*)device_path);
}

int handle_mode_tcp_command_typed(const char* cmd_name,
                                int argc,
                                char** argv,
                                const uci_param_def_t* params,
                                int param_count) {
    const uci_cmd_parsed_param_t* host_param;
    const uci_cmd_parsed_param_t* port_param;

    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

    host_param = uci_cmd_get_parsed_param(0);
    port_param = uci_cmd_get_parsed_param(1);
    if (!host_param || !host_param->present || !port_param || !port_param->present) {
        ui_print_error("mode_tcp requires host and port");
        return -1;
    }

    return handle_mode_tcp_command((char*)host_param->raw_value, port_param->value.u16);
}

int handle_mode_info_command_typed(const char* cmd_name,
                                 int argc,
                                 char** argv,
                                 const uci_param_def_t* params,
                                 int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;
    handle_mode_info_command();
    return 0;
}

int handle_hw_init_command_typed(const char* cmd_name,
                               int argc,
                               char** argv,
                               const uci_param_def_t* params,
                               int param_count) {
    (void)cmd_name;
    (void)argc;
    (void)argv;
    (void)params;
    (void)param_count;

    const char* device_path = get_optional_string_param(0);
    return handle_hw_init_command((char*)device_path);
}

int handle_hw_send_command_typed(const char* cmd_name,
                               int argc,
                               char** argv,
                               const uci_param_def_t* params,
                               int param_count) {
    (void)cmd_name;
    (void)params;

    unsigned char mt = 0;
    unsigned char pbf = 0;
    unsigned char gid = 0;
    unsigned char oid = 0;

    if (get_hex_byte_param(0, "mt", &mt) != 0 ||
        get_hex_byte_param(1, "pbf", &pbf) != 0 ||
        get_hex_byte_param(2, "gid", &gid) != 0 ||
        get_hex_byte_param(3, "oid", &oid) != 0) {
        return -1;
    }

    char mt_str[3];
    char pbf_str[3];
    char gid_str[3];
    char oid_str[3];
    snprintf(mt_str, sizeof(mt_str), "%02X", mt);
    snprintf(pbf_str, sizeof(pbf_str), "%02X", pbf);
    snprintf(gid_str, sizeof(gid_str), "%02X", gid);
    snprintf(oid_str, sizeof(oid_str), "%02X", oid);

    char** payload_tokens = NULL;
    int payload_count = 0;
    if (argc > param_count + 1) {
        payload_tokens = &argv[param_count + 1];
        payload_count = argc - (param_count + 1);
    }

    return handle_hw_send_command(mt_str,
                                  pbf_str,
                                  gid_str,
                                  oid_str,
                                  payload_tokens,
                                  payload_count);
}
