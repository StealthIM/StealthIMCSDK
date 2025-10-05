#include <stdio.h>
#include <stdlib.h>
#include <stim/hal/async/task.h>

#include "stim/hal/logging.h"

stim_task_t *_stim_task_create(loop_t *loop, task_cb_t cb, void *userdata, bool auto_destroy) {
    stim_task_t *t = (stim_task_t*)calloc(1, sizeof(stim_task_t));
    if (!t) return NULL;
    t->future = stim_future_create(loop);
    if (!t->future) {
        free(t);
        return NULL;
    }
    if (auto_destroy) stim_future_add_done_callback(t->future, stim_task_destroy_fut, t);
    t->loop = loop;
    t->cb = cb;
    stim_task_ctx_t *ctx = calloc(1, sizeof(stim_task_ctx_t));
    if (!ctx) {
        stim_future_destroy(t->future);
        free(t);
        return NULL;
    }
    ctx->userdata = userdata;
    t->ctx = ctx;
    return t;
}

stim_task_t *stim_task_create(loop_t *loop, task_cb_t cb, void *userdata) {
    return _stim_task_create(loop, cb, userdata, false);
}
stim_task_t *stim_task_create_in_task(loop_t *loop, task_cb_t cb, void *userdata) {
    return _stim_task_create(loop, cb, userdata, true);
}

void stim_task_destroy(stim_task_t *task) {
    if (!task) return;
    if (!task->ctx->finished) stim_future_destroy(task->future);
    free(task->ctx->local_data);
    free(task->ctx);
    free(task);
}

void stim_task_destroy_loop(loop_t *loop, void *task) {
    stim_task_destroy(task);
}

void stim_task_destroy_fut(stim_future_t *fut, void *task) {
    stim_loop_call_soon(fut->loop, stim_task_destroy_loop, task);
}

void _stim_task_run_cb(loop_t *loop, void *userdata);
void _stim_task_run_cb_fut(stim_future_t *fut, void *userdata) {
    _stim_task_run_cb(fut->loop, userdata);
}

void _stim_task_run_cb(loop_t *loop, void *userdata) {
    stim_task_t *task = (stim_task_t*)userdata;
    if (!task || !task->future) return;

    if (task->ctx->waiting_for != NULL) {
        task->ctx->future_result = stim_future_result(task->ctx->waiting_for);
    }

    int ret = task->cb(task, task->ctx);
    if (ret == TASK_ENDED) {
        stim_future_done(task->future, task->ctx->result);
        task->ctx->finished = true;
    } else if (ret == TASK_PENDING) {
        stim_future_add_done_callback(task->ctx->waiting_for, _stim_task_run_cb_fut, task);
    }
}

void stim_task_run(stim_task_t *task) {
    if (!task || !task->loop) return;
    stim_loop_call_soon(task->loop, _stim_task_run_cb, task);
}

void sleep_cb(loop_t *loop, void *userdata) {
    stim_future_t *fut = (stim_future_t*)userdata;
    if (fut) {
        stim_future_done(fut, NULL);
    }
}

stim_future_t *_async_sleep(loop_t *loop, uint64_t ms) {
    stim_future_t *fut = stim_future_create(loop);
    if (!fut) return NULL;
    stim_loop_add_timer(loop, ms, sleep_cb, fut);
    return fut;
}
