#ifndef UCI_LOGGING_H
#define UCI_LOGGING_H

#include <stdio.h>
#include <stdlib.h>

// Log levels
typedef enum {
    UCI_LOG_LEVEL_ERROR = 0,
    UCI_LOG_LEVEL_WARNING,
    UCI_LOG_LEVEL_INFO,
    UCI_LOG_LEVEL_DEBUG
} uci_log_level_t;

// Global log level setting
extern uci_log_level_t g_uci_log_level;
extern int g_uci_log_use_colors;

// Initialize logging system
void uci_log_init(uci_log_level_t level, int use_colors);

// Core logging function
void uci_log_message(uci_log_level_t level, const char* file, int line, const char* func, 
                     const char* format, ...);

// Convenience macros with file, line, and function information
#define UCI_LOG_ERROR(fmt, ...) \
    uci_log_message(UCI_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define UCI_LOG_WARNING(fmt, ...) \
    uci_log_message(UCI_LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define UCI_LOG_INFO(fmt, ...) \
    uci_log_message(UCI_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define UCI_LOG_DEBUG(fmt, ...) \
    uci_log_message(UCI_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// Enhanced error logging with context
#define UCI_LOG_ERROR_CTX(ctx, fmt, ...) \
    do { \
        fprintf(stderr, "[ERROR] %s: " fmt "\n", ctx, ##__VA_ARGS__); \
        uci_log_message(UCI_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
    } while(0)

#endif // UCI_LOGGING_H