#pragma once
#include <stim/config.h>

#include "stim/hal/async/future.h"
#include "stim/hal/async/task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stim_tls_ctx stim_tls_ctx_t;

// 初始化 TLS 库（一次即可）
int stim_tls_init(void);

// 清理 TLS 库
int stim_tls_cleanup(void);

// 创建 TLS 上下文
stim_tls_ctx_t* stim_tls_create(void);

// 销毁上下文
void stim_tls_destroy(stim_tls_ctx_t* ctx);

// 连接到 host:port
int stim_tls_connect(stim_tls_ctx_t* ctx, const char* host, int port);

stim_task_t *stim_tls_connect_async(loop_t *loop, stim_tls_ctx_t* ctx, const char* host, int port);

// 发送数据
int stim_tls_send(stim_tls_ctx_t* ctx, const char* buf, int len);

stim_future_t *stim_tls_send_async(loop_t *loop, stim_tls_ctx_t* ctx, const char* buf, int len);

// 接收数据
int stim_tls_recv(stim_tls_ctx_t* ctx, char* buf, int maxlen);

stim_future_t *stim_tls_recv_async(loop_t *loop, stim_tls_ctx_t* ctx, char* buf, int maxlen);

// 关闭连接
void stim_tls_close(stim_tls_ctx_t* ctx);

#ifdef __cplusplus
}
#endif
