#include "stim/hal/logging.h"
#include <stdarg.h>
#include <stdio.h>

static stim_log_callback_t g_log_cb = NULL;

void stim_set_log_callback(stim_log_callback_t cb) {
    g_log_cb = cb;
}

void stim_log(stim_log_level_t level, const char* fmt, ...) {
#if defined(stim_LOG_STDIO) || defined(stim_LOG_CUSTOM) || defined(stim_LOG_ENABLED)
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
            case stim_LOG_DEBUG: level_str = "DEBUG"; break;
            case stim_LOG_INFO:  level_str = "INFO";  break;
            case stim_LOG_WARN:  level_str = "WARN";  break;
            case stim_LOG_ERROR: level_str = "ERROR"; break;
        }
        printf("[%s] %s\n", level_str, buffer);

    }
#endif

}
