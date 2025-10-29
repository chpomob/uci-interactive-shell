#ifndef UCI_CLI_COMPLETION_H
#define UCI_CLI_COMPLETION_H

void cli_initialize_readline(void);
void cli_print_completion_suggestions(const char* input);

// Conditional declaration for readline completion function
#ifdef HAVE_READLINE
char* cli_completion_generator(const char* text, int state);
#endif

#endif // UCI_CLI_COMPLETION_H