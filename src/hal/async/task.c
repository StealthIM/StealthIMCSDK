#include <stdio.h>
#include <stdlib.h>
#include <stealthim/hal/async/task.h>

#include "stealthim/hal/logging.h"

stealthim_task_t *_stealthim_task_create(loop_t *loop, task_cb_t cb, void *userdata, bool auto_destroy) {
    stealthim_task_t *t = (stealthim_task_t*)calloc(1, sizeof(stealthim_task_t));
    if (!t) return NULL;
    t->future = stealthim_future_create(loop);
    if (!t->future) {
        free(t);
        return NULL;
    }
    if (auto_destroy) stealthim_future_add_done_callback(t->future, stealthim_task_destroy_fut, t);
    t->loop = loop;
    t->cb = cb;
    stealthim_task_ctx_t *ctx = calloc(1, sizeof(stealthim_task_ctx_t));
    if (!ctx) {
        stealthim_future_destroy(t->future);
        free(t);
        return NULL;
    }
    ctx->userdata = userdata;
    t->ctx = ctx;
    return t;
}

stealthim_task_t *stealthim_task_create(loop_t *loop, task_cb_t cb, void *userdata) {
    return _stealthim_task_create(loop, cb, userdata, false);
}
stealthim_task_t *stealthim_task_create_in_task(loop_t *loop, task_cb_t cb, void *userdata) {
    return _stealthim_task_create(loop, cb, userdata, true);
}

void stealthim_task_destroy(stealthim_task_t *task) {
    if (!task) return;
    if (!task->ctx->finished) stealthim_future_destroy(task->future);
    free(task->ctx->local_data);
    free(task->ctx);
    free(task);
}

void stealthim_task_destroy_loop(loop_t *loop, void *task) {
    stealthim_task_destroy(task);
}

void stealthim_task_destroy_fut(stealthim_future_t *fut, void *task) {
    stealthim_loop_call_soon(fut->loop, stealthim_task_destroy_loop, task);
}

void task_run_cb(loop_t *loop, void *userdata) {
    stealthim_task_t *task = (stealthim_task_t*)userdata;
    if (!task || !task->future) return;

    if (task->ctx->waiting_for != NULL) {
        task->ctx->future_result = stealthim_future_result(task->ctx->waiting_for);
    }

    int ret = task->cb(task, task->ctx);
    if (ret == TASK_ENDED) {
        stealthim_future_done(task->future, task->ctx->result);
        task->ctx->finished = true;
    } else if (ret == TASK_PENDING) {
        stealthim_future_add_done_callback(task->ctx->waiting_for, (stealthim_future_cb_t)task_run_cb, task);
    }
}

void stealthim_task_run(stealthim_task_t *task) {
    if (!task || !task->loop) return;
    stealthim_loop_call_soon(task->loop, task_run_cb, task);
}

void sleep_cb(loop_t *loop, void *userdata) {
    stealthim_future_t *fut = (stealthim_future_t*)userdata;
    if (fut) {
        stealthim_future_done(fut, NULL);
    }
}

stealthim_future_t *_async_sleep(loop_t *loop, uint64_t ms) {
    stealthim_future_t *fut = stealthim_future_create(loop);
    if (!fut) return NULL;
    stealthim_loop_add_timer(loop, ms, sleep_cb, fut);
    return fut;
}
