#pragma once
#include "stealthim/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STEALTHIM_WS_OK = 0,
    STEALTHIM_WS_ERR = -1,
    STEALTHIM_WS_CLOSED = -2
} stealthim_ws_status_t;

// WebSocket 句柄（不透明指针）
typedef struct stealthim_ws_t stealthim_ws_t;

void stealthim_ws_init();

/**
 * 创建一个WebSocket连接
 * @param url   WebSocket URL (例如 "ws://echo.websocket.org:80/")
 * @return 连接对象 (NULL = 失败)
 */
stealthim_ws_t* stealthim_ws_connect(const char* url);

/**
 * 发送消息
 * @param ws    连接对象
 * @param data  数据
 * @param len   数据长度
 * @param is_text  是否文本帧 (1 = 文本, 0 = 二进制)
 */
stealthim_ws_status_t stealthim_ws_send(stealthim_ws_t* ws, const void* data, int len, int is_text);

/**
 * 接收消息（阻塞）
 * @param ws    连接对象
 * @param buffer 缓存区
 * @param maxlen 缓存大小
 * @param is_text 输出参数：是否文本帧
 * @return 接收的字节数，负数 = 错误
 */
int stealthim_ws_recv(stealthim_ws_t* ws, void* buffer, int maxlen, int* is_text);

/**
 * 关闭连接
 */
void stealthim_ws_close(stealthim_ws_t* ws);

#ifdef __cplusplus
}
#endif
