#pragma once
#include <stealthim/config.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int stealthim_log_level_t;

// 用户可设置自己的 log 回调
typedef void (*stealthim_log_callback_t)(stealthim_log_level_t level, const char* msg);

void stealthim_set_log_callback(stealthim_log_callback_t cb);

// SDK 内部使用的 log 函数
void stealthim_log(stealthim_log_level_t level, const char* fmt, ...);

#define stealthim_log_debug(fmt, ...) \
    do { \
        if (STEALTHIM_LOG_LEVEL <= STEALTHIM_LOG_DEBUG) { \
            stealthim_log(STEALTHIM_LOG_DEBUG, fmt, ##__VA_ARGS__); \
        } \
    } while(0)
#define stealthim_log_info(fmt, ...) \
    do { \
        if (STEALTHIM_LOG_LEVEL <= STEALTHIM_LOG_INFO) { \
            stealthim_log(STEALTHIM_LOG_INFO, fmt, ##__VA_ARGS__); \
        } \
} while(0)
#define stealthim_log_warn(fmt, ...) \
    do { \
        if (STEALTHIM_LOG_LEVEL <= STEALTHIM_LOG_WARN) { \
            stealthim_log(STEALTHIM_LOG_WARN, fmt, ##__VA_ARGS__); \
        } \
    } while(0)
#define stealthim_log_error(fmt, ...) \
    do { \
        if (STEALTHIM_LOG_LEVEL <= STEALTHIM_LOG_ERROR) { \
            stealthim_log(STEALTHIM_LOG_ERROR, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif
