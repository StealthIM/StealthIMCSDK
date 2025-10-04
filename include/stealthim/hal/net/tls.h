#pragma once
#include <stealthim/config.h>

#include "stealthim/hal/async/future.h"
#include "stealthim/hal/async/task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stealthim_tls_ctx stealthim_tls_ctx_t;

// 初始化 TLS 库（一次即可）
int stealthim_tls_init(void);

// 清理 TLS 库
int stealthim_tls_cleanup(void);

// 创建 TLS 上下文
stealthim_tls_ctx_t* stealthim_tls_create(void);

// 销毁上下文
void stealthim_tls_destroy(stealthim_tls_ctx_t* ctx);

// 连接到 host:port
int stealthim_tls_connect(stealthim_tls_ctx_t* ctx, const char* host, int port);

stealthim_task_t *stealthim_tls_connect_async(loop_t *loop, stealthim_tls_ctx_t* ctx, const char* host, int port);

// 发送数据
int stealthim_tls_send(stealthim_tls_ctx_t* ctx, const char* buf, int len);

stealthim_future_t *stealthim_tls_send_async(loop_t *loop, stealthim_tls_ctx_t* ctx, const char* buf, int len);

// 接收数据
int stealthim_tls_recv(stealthim_tls_ctx_t* ctx, char* buf, int maxlen);

stealthim_future_t *stealthim_tls_recv_async(loop_t *loop, stealthim_tls_ctx_t* ctx, char* buf, int maxlen);

// 关闭连接
void stealthim_tls_close(stealthim_tls_ctx_t* ctx);

#ifdef __cplusplus
}
#endif
