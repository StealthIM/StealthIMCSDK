#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "future.h"
#include "loop.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_PENDING 0
#define TASK_ENDED 1

typedef struct stealthim_task_s stealthim_task_t;
typedef struct stealthim_task_ctx_s stealthim_task_ctx_t;
typedef int (*task_cb_t)(stealthim_task_t *task, stealthim_task_ctx_t *userdata);

typedef struct stealthim_task_ctx_s {
    void *userdata;
    unsigned int state;
    stealthim_future_t *waiting_for;
    void *future_result;
    void *local_data;
    void *result;
    bool finished;
} stealthim_task_ctx_t;

typedef struct stealthim_task_s {
    stealthim_future_t *future;
    loop_t *loop;
    task_cb_t cb;
    stealthim_task_ctx_t *ctx;
} stealthim_task_t;

// 创建task
stealthim_task_t *stealthim_task_create(loop_t *loop, task_cb_t cb, void *userdata);
stealthim_task_t *stealthim_task_create_in_task(loop_t *loop, task_cb_t cb, void *userdata);

// 销毁task
void stealthim_task_destroy(stealthim_task_t *task);
void stealthim_task_destroy_loop(loop_t *loop, void *task);
void stealthim_task_destroy_fut(stealthim_future_t *fut, void *task);

#define stealthim_task_future(task) ((task)->future)
#define stealthim_task_loop(task) ((task)->loop)

// 启动task
void stealthim_task_run(stealthim_task_t *task);

#define stealthim_task_local_var(vars) \
    stealthim_task_t *__task = (task); \
    stealthim_task_ctx_t *__ctx = (ctx); \
    typedef struct __local_vars_s { vars; } __local_vars_t; \
    if(__ctx->state==0) \
        __ctx->local_data=calloc(1, sizeof(__local_vars_t));

#define stealthim_task_enter() \
    switch(__ctx->state) { \
        case 0: \
        case __LINE__:

#define stealthim_task_userdata(type) ((type)__ctx->userdata)

#define stealthim_task_await_future(fut) \
    __ctx->waiting_for = (fut); \
    __ctx->state = __LINE__; \
    return TASK_PENDING; \
    case __LINE__:

#define _CONNECT1(x,y) x##y
#define _CONNECT2(x,y) _CONNECT1(x,y)
#define stealthim_task_await_task(func, arg) \
    stealthim_task_t * _CONNECT2(__task_, __LINE__) = (stealthim_task_create_in_task(stealthim_task_loop(__task), func, arg)); \
    stealthim_task_run(_CONNECT2(__task_, __LINE__)); \
    stealthim_task_await_future(stealthim_task_future(_CONNECT2(__task_, __LINE__)));

#define stealthim_task_var(val) \
    (((__local_vars_t*)(__ctx->local_data))-> val)

#define stealthim_task_await_res(type) ((type)__ctx->future_result)

#define stealthim_task_return(val) __ctx->result = (void *)(val); return TASK_ENDED;
#define stealthim_task_end() default: stealthim_task_return(NULL);}
#define stealthim_task_finished(task) ((task)->ctx->finished)
#define stealthim_task_result(task) ((task)->ctx->result)

// async_sleep的实现
stealthim_future_t *_async_sleep(loop_t *loop, uint64_t ms);
#define async_sleep(ms) _async_sleep(stealthim_task_loop(__task), (ms))

#ifdef __cplusplus
}
#endif
