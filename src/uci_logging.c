#include "../include/uci_logging.h"
#include <stdarg.h>
#include <string.h>

// Define the global variables
uci_log_level_t g_uci_log_level = UCI_LOG_LEVEL_INFO;
int g_uci_log_use_colors = 1;

// Color codes for terminal output
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void uci_log_init(uci_log_level_t level, int use_colors) {
    g_uci_log_level = level;
    g_uci_log_use_colors = use_colors;
}

void uci_log_message(uci_log_level_t level, const char* file, int line, const char* func, 
                     const char* format, ...) {
    // Don't log if below the current log level
    if (level > g_uci_log_level) {
        return;
    }

    // Extract just the filename from the full path
    const char* filename = strrchr(file, '/');
    if (filename) {
        filename++; // Skip the '/' character
    } else {
        filename = file;
    }

    // Print log level with appropriate color
    if (g_uci_log_use_colors) {
        switch (level) {
            case UCI_LOG_LEVEL_ERROR:
                fprintf(stderr, ANSI_COLOR_RED "[ERROR] " ANSI_COLOR_RESET);
                break;
            case UCI_LOG_LEVEL_WARNING:
                fprintf(stderr, ANSI_COLOR_YELLOW "[WARN]  " ANSI_COLOR_RESET);
                break;
            case UCI_LOG_LEVEL_INFO:
                fprintf(stderr, ANSI_COLOR_CYAN "[INFO]  " ANSI_COLOR_RESET);
                break;
            case UCI_LOG_LEVEL_DEBUG:
                fprintf(stderr, ANSI_COLOR_MAGENTA "[DEBUG] " ANSI_COLOR_RESET);
                break;
            default:
                fprintf(stderr, "[LOG]   ");
                break;
        }
    } else {
        switch (level) {
            case UCI_LOG_LEVEL_ERROR:
                fprintf(stderr, "[ERROR] ");
                break;
            case UCI_LOG_LEVEL_WARNING:
                fprintf(stderr, "[WARN]  ");
                break;
            case UCI_LOG_LEVEL_INFO:
                fprintf(stderr, "[INFO]  ");
                break;
            case UCI_LOG_LEVEL_DEBUG:
                fprintf(stderr, "[DEBUG] ");
                break;
            default:
                fprintf(stderr, "[LOG]   ");
                break;
        }
    }

    // Print file, line, and function info
    fprintf(stderr, "%s:%d:%s() - ", filename, line, func);

    // Print the formatted message
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    // End with newline
    fprintf(stderr, "\n");
    fflush(stderr);
}