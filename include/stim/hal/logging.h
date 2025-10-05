#pragma once
#include <stim/config.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int stim_log_level_t;

// 用户可设置自己的 log 回调
typedef void (*stim_log_callback_t)(stim_log_level_t level, const char* msg);

void stim_set_log_callback(stim_log_callback_t cb);

// SDK 内部使用的 log 函数
void stim_log(stim_log_level_t level, const char* fmt, ...);

#define stim_log_debug(fmt, ...) \
    do { \
        if (stim_LOG_LEVEL <= stim_LOG_DEBUG) { \
            stim_log(stim_LOG_DEBUG, fmt, ##__VA_ARGS__); \
        } \
    } while(0)
#define stim_log_info(fmt, ...) \
    do { \
        if (stim_LOG_LEVEL <= stim_LOG_INFO) { \
            stim_log(stim_LOG_INFO, fmt, ##__VA_ARGS__); \
        } \
} while(0)
#define stim_log_warn(fmt, ...) \
    do { \
        if (stim_LOG_LEVEL <= stim_LOG_WARN) { \
            stim_log(stim_LOG_WARN, fmt, ##__VA_ARGS__); \
        } \
    } while(0)
#define stim_log_error(fmt, ...) \
    do { \
        if (stim_LOG_LEVEL <= stim_LOG_ERROR) { \
            stim_log(stim_LOG_ERROR, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif
