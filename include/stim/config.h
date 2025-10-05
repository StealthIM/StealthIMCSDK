#pragma once

// 选择网络实现
// #define stim_NETWORK_WIN32 1
#define stim_NETWORK_POSIX 1
// #define stim_NETWORK_LWIP 1


// 选择日志实现
#define stim_LOG_ENABLED 1

#define stim_LOG_STDIO 1
// #define stim_LOG_CUSTOM 1
// #define stim_LOG_NONE 1

#define stim_LOG_DEBUG 0
#define stim_LOG_INFO 1
#define stim_LOG_WARN 2
#define stim_LOG_ERROR 3
#define stim_LOG_NONE 4

#define stim_LOG_LEVEL stim_LOG_DEBUG

// 选择 TLS 实现
#define stim_TLS_OPENSSL 1
// #define stim_TLS_MBEDTLS 1

// 选择 WebSocket 实现
// #define stim_WS_WIN32 1
#define stim_WS_POSIX 1
// #define stim_WS_NONE 1

// 选择 Async 实现
// #define stim_ASYNC_WIN32 1
#define stim_ASYNC_LINUX_EPULL 1
// #define stim_ASYNC_LINUX_SELECT 1
// #define stim_ASYNC_BSD 1
