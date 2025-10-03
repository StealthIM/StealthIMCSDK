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

typedef struct task_s task_t;
typedef struct task_ctx_s task_ctx_t;
typedef int (*task_cb_t)(task_t *task, task_ctx_t *userdata);

typedef struct task_ctx_s {
    void *userdata;
    unsigned int state;
    future_t *waiting_for;
    void *future_result;
    void *local_data;
    void *result;
    bool finished;
} task_ctx_t;

typedef struct task_s {
    future_t *future;
    loop_t *loop;
    task_cb_t cb;
    task_ctx_t *ctx;
} task_t;

// 创建task
task_t *task_create(loop_t *loop, task_cb_t cb, void *userdata);

// 销毁task
void task_destroy(task_t *task);

#define task_future(task) ((task)->future)
#define task_loop(task) ((task)->loop)

// 启动task
void task_run(task_t *task);

#define task_local_var(vars) \
    task_t *__task = (task); \
    task_ctx_t *__ctx = (ctx); \
    typedef struct __local_vars_s { vars; } __local_vars_t; \
    if(__ctx->state==0) \
        __ctx->local_data=calloc(1, sizeof(__local_vars_t));

#define task_enter(task, ctx) \
    switch(__ctx->state) { \
        case 0: \
        case __LINE__:

#define task_await_future(fut) \
    __ctx->waiting_for = (fut); \
    __ctx->state = __LINE__; \
    return TASK_PENDING; \
    case __LINE__:

#define _CONNECT1(x,y) x##y
#define _CONNECT2(x,y) _CONNECT1(x,y)
#define task_await_task(task) \
    task_t * _CONNECT2(__task_, __LINE__) = (task); \
    task_run(_CONNECT2(__task_, __LINE__)); \
    task_await_future(task_future(_CONNECT2(__task_, __LINE__)));

#define task_var(val) \
    (((__local_vars_t*)(__ctx->local_data))-> val)

#define task_await_res(type) (type)__ctx->future_result

#define task_return(val) __ctx->result = (void *)(val); return TASK_ENDED;
#define task_end() default: task_return(NULL);}
#define task_finished(task) ((task)->ctx->finished)
#define task_result(task) ((task)->ctx->result)

// async_sleep的实现
future_t *_async_sleep(loop_t *loop, uint64_t ms);
#define async_sleep(ms) _async_sleep(task_loop(__task), (ms))

#ifdef __cplusplus
}
#endif
