#include "stealthim/hal/logging.h"
#include <stdarg.h>
#include <stdio.h>

static stealthim_log_callback_t g_log_cb = NULL;

void stealthim_set_log_callback(stealthim_log_callback_t cb) {
    g_log_cb = cb;
}

void stealthim_log(stealthim_log_level_t level, const char* fmt, ...) {
#if defined(STEALTHIM_LOG_STDIO) || defined(STEALTHIM_LOG_CUSTOM) || defined(STEALTHIM_LOG_ENABLED)
    char buffer[512];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (g_log_cb) {
        g_log_cb(level, buffer);
    } else {
        const char* level_str = "UNK";
        switch (level) {
            case STEALTHIM_LOG_DEBUG: level_str = "DEBUG"; break;
            case STEALTHIM_LOG_INFO:  level_str = "INFO";  break;
            case STEALTHIM_LOG_WARN:  level_str = "WARN";  break;
            case STEALTHIM_LOG_ERROR: level_str = "ERROR"; break;
        }
        printf("[%s] %s\n", level_str, buffer);

    }
#endif

}
