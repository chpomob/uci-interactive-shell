#ifndef UCI_CMD_HARDWARE_TYPED_H
#define UCI_CMD_HARDWARE_TYPED_H

#include "uci_command_framework.h"

int handle_mode_sim_command_typed(const char* cmd_name,
                                  int argc,
                                  char** argv,
                                  const uci_param_def_t* params,
                                  int param_count);
int handle_mode_hw_command_typed(const char* cmd_name,
                                 int argc,
                                 char** argv,
                                 const uci_param_def_t* params,
                                 int param_count);
int handle_mode_info_command_typed(const char* cmd_name,
                                   int argc,
                                   char** argv,
                                   const uci_param_def_t* params,
                                   int param_count);
int handle_hw_init_command_typed(const char* cmd_name,
                                 int argc,
                                 char** argv,
                                 const uci_param_def_t* params,
                                 int param_count);
int handle_hw_send_command_typed(const char* cmd_name,
                                 int argc,
                                 char** argv,
                                 const uci_param_def_t* params,
                                 int param_count);

#endif // UCI_CMD_HARDWARE_TYPED_H
