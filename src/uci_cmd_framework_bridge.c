#include <string.h>

#include "../include/uci_cmd_core_new.h"
#include "../include/uci_cmd_framework_bridge.h"
#include "../include/uci_cmd_framework_device.h"
#include "../include/uci_cmd_framework_simulation.h"
#include "../include/uci_cmd_framework_session.h"
#include "../include/uci_cmd_framework_wrappers.h"
#include "../include/uci_cmd_handlers.h"
#include "../include/uci_ui.h"

DEFINE_CMD_WRAPPER(cmd_help)
DEFINE_CMD_WRAPPER(cmd_analyze_packet)

static const uci_param_def_t k_analyze_packet_params[] = {
    {
        .name = "packet_bytes",
        .type = PARAM_TYPE_HEX_STRING,
        .flags = PARAM_FLAG_REQUIRED,
        .max_len = 1024,
        .min_value = 0,
        .max_value = 0,
        .description = "Hex encoded packet bytes",
    },
};

const uci_command_def_t g_uci_command_defs[] = {
    // General
    {
        .name = "help",
        .aliases = { NULL },
        .group = CLI_GROUP_GENERAL,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Show this help message",
        .params = NULL,
        .param_count = 0,
        .handler = cmd_help_framework_adapter,
    },

    // Analysis
    {
        .name = "analyze_packet",
        .aliases = { NULL },
        .group = CLI_GROUP_ANALYSIS,
        .flags = CLI_CMD_FLAG_NONE,
        .description = "Analyze packet bytes with enhanced decoder",
        .params = k_analyze_packet_params,
        .param_count = ARRAY_SIZE(k_analyze_packet_params),
        .handler = cmd_analyze_packet_framework_adapter,
    },

};

const int g_uci_command_defs_count = (int)ARRAY_SIZE(g_uci_command_defs);

static const uci_command_def_t* uci_cmd_framework_find_in_defs(const uci_command_def_t* defs,
                                                               int count,
                                                               const char* name) {
    if (!name || !defs || count <= 0) {
        return NULL;
    }

    for (int idx = 0; idx < count; idx++) {
        const uci_command_def_t* def = &defs[idx];
        if (strcmp(name, def->name) == 0) {
            return def;
        }

        for (size_t alias_idx = 0; alias_idx < ARRAY_SIZE(def->aliases); alias_idx++) {
            const char* alias = def->aliases[alias_idx];
            if (!alias) {
                break;
            }
            if (strcmp(name, alias) == 0) {
                return def;
            }
        }
    }

    return NULL;
}

const uci_command_def_t* uci_cmd_framework_find(const char* name) {
    const uci_command_def_t* def = uci_cmd_framework_find_in_defs(g_uci_command_defs,
                                                                  g_uci_command_defs_count,
                                                                  name);
    if (def) {
        return def;
    }

    def = uci_cmd_framework_find_in_defs(g_uci_device_command_defs,
                                         g_uci_device_command_defs_count,
                                         name);
    if (def) {
        return def;
    }

    def = uci_cmd_framework_find_in_defs(g_uci_session_command_defs,
                                         g_uci_session_command_defs_count,
                                         name);
    if (def) {
        return def;
    }

    return uci_cmd_framework_find_in_defs(g_uci_simulation_command_defs,
                                          g_uci_simulation_command_defs_count,
                                          name);
}

int uci_cmd_framework_handler(int argc, char** argv) {
    if (argc <= 0 || !argv) {
        ui_print_error("Missing command arguments");
        return -1;
    }

    const uci_command_def_t* cmd_def = uci_cmd_framework_find(argv[0]);
    if (!cmd_def) {
        ui_print_command_not_found(argv[0]);
        return -1;
    }

    return uci_cmd_dispatch(cmd_def, argc, argv);
}

typedef struct {
    const uci_command_def_t* defs;
    const int* count;
} uci_cmd_framework_list_t;

static const uci_cmd_framework_list_t k_all_command_lists[] = {
    { g_uci_command_defs, &g_uci_command_defs_count },
    { g_uci_device_command_defs, &g_uci_device_command_defs_count },
    { g_uci_session_command_defs, &g_uci_session_command_defs_count },
    { g_uci_simulation_command_defs, &g_uci_simulation_command_defs_count },
};

void uci_cmd_framework_for_each_command(uci_cmd_framework_iter_cb cb, void* ctx) {
    if (!cb) {
        return;
    }

    for (size_t list_idx = 0; list_idx < ARRAY_SIZE(k_all_command_lists); list_idx++) {
        const uci_cmd_framework_list_t* list = &k_all_command_lists[list_idx];
        int count = list->count ? *list->count : 0;
        for (int i = 0; i < count; i++) {
            cb(&list->defs[i], ctx);
        }
    }
}

int uci_cmd_framework_total_command_count(void) {
    int total = 0;
    for (size_t list_idx = 0; list_idx < ARRAY_SIZE(k_all_command_lists); list_idx++) {
        const uci_cmd_framework_list_t* list = &k_all_command_lists[list_idx];
        if (list->count) {
            total += *list->count;
        }
    }
    return total;
}
