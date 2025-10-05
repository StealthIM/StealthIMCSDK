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

typedef struct stim_task_s stim_task_t;
typedef struct stim_task_ctx_s stim_task_ctx_t;
typedef int (*task_cb_t)(stim_task_t *task, stim_task_ctx_t *userdata);

typedef struct stim_task_ctx_s {
    void *userdata;
    unsigned int state;
    stim_future_t *waiting_for;
    void *future_result;
    void *local_data;
    void *result;
    bool finished;
} stim_task_ctx_t;

typedef struct stim_task_s {
    stim_future_t *future;
    loop_t *loop;
    task_cb_t cb;
    stim_task_ctx_t *ctx;
} stim_task_t;

// 创建task
stim_task_t *stim_task_create(loop_t *loop, task_cb_t cb, void *userdata);
stim_task_t *stim_task_create_in_task(loop_t *loop, task_cb_t cb, void *userdata);

// 销毁task
void stim_task_destroy(stim_task_t *task);
void stim_task_destroy_loop(loop_t *loop, void *task);
void stim_task_destroy_fut(stim_future_t *fut, void *task);

#define stim_task_future(task) ((task)->future)
#define stim_task_loop(task) ((task)->loop)

// 启动task
void stim_task_run(stim_task_t *task);
void _stim_task_run_cb(loop_t *loop, void *userdata);

#define stim_task_register_handle(handle) \
    stim_loop_register_handle(stim_task_loop(__task), (handle), _stim_task_run_cb, __task); \
    _stim_task_await_common()

#define stim_task_unregister_handle(handle, close_socket) \
    stim_loop_unregister_handle(stim_task_loop(__task), (handle), (close_socket))

#define stim_task_local_var(vars) \
    stim_task_t *__task = (task); \
    stim_task_ctx_t *__ctx = (ctx); \
    typedef struct __local_vars_s { vars; } __local_vars_t; \
    if(__ctx->state==0) \
        __ctx->local_data=calloc(1, sizeof(__local_vars_t));

#define stim_task_enter() \
    switch(__ctx->state) { \
        case 0: \
        case __LINE__:

#define stim_task_userdata(type) ((type)__ctx->userdata)

#define _stim_task_await_common() \
    __ctx->state = __LINE__; \
    return TASK_PENDING; \
    case __LINE__:

#define stim_task_await_future(fut) \
    __ctx->waiting_for = (fut); \
    _stim_task_await_common()

#define _CONNECT1(x,y) x##y
#define _CONNECT2(x,y) _CONNECT1(x,y)
#define stim_task_await_task(func, arg) \
    stim_task_t * _CONNECT2(__task_, __LINE__) = (stim_task_create_in_task(stim_task_loop(__task), func, arg)); \
    stim_task_run(_CONNECT2(__task_, __LINE__)); \
    stim_task_await_future(stim_task_future(_CONNECT2(__task_, __LINE__)));

#define stim_task_var(val) \
    (((__local_vars_t*)(__ctx->local_data))-> val)

#define stim_task_await_res(type) ((type)__ctx->future_result)

#define stim_task_return(val) __ctx->result = (void *)(val); return TASK_ENDED;
#define stim_task_end() default: stim_task_return(NULL);}
#define stim_task_finished(task) ((task)->ctx->finished)
#define stim_task_result(task) ((task)->ctx->result)

// async_sleep的实现
stim_future_t *_async_sleep(loop_t *loop, uint64_t ms);
#define async_sleep(ms) _async_sleep(stim_task_loop(__task), (ms))

#ifdef __cplusplus
}
#endif
