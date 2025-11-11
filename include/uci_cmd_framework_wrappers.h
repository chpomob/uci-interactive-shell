#ifndef UCI_CMD_FRAMEWORK_WRAPPERS_H
#define UCI_CMD_FRAMEWORK_WRAPPERS_H

#include "uci_command_framework.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define DEFINE_CMD_WRAPPER(fn)                                                          \
    static int fn##_framework_adapter(const char* cmd_name,                             \
                                      int argc,                                         \
                                      char** argv,                                      \
                                      const uci_param_def_t* params,                    \
                                      int param_count) {                                \
        (void)cmd_name;                                                                  \
        (void)params;                                                                    \
        (void)param_count;                                                               \
        return fn(argc, argv);                                                           \
    }

#endif /* UCI_CMD_FRAMEWORK_WRAPPERS_H */
