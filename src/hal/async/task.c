#include <stdio.h>
#include <stdlib.h>
#include <stealthim/hal/async/task.h>

task_t *task_create(loop_t *loop, task_cb_t cb, void *userdata) {
    task_t *t = (task_t*)calloc(1, sizeof(task_t));
    if (!t) return NULL;
    t->future = future_create(loop);
    if (!t->future) {
        free(t);
        return NULL;
    }
    t->loop = loop;
    t->cb = cb;
    task_ctx_t *ctx = (task_ctx_t*)malloc(sizeof(task_ctx_t));
    if (!ctx) {
        future_destroy(t->future);
        free(t);
        return NULL;
    }
    ctx->userdata = userdata;
    ctx->state = 0;
    ctx->waiting_for = NULL;
    ctx->local_data = NULL;
    t->ctx = ctx;
    return t;
}

void task_destroy(task_t *task) {
    if (!task) return;
    future_destroy(task->future);
    free(task->ctx->local_data);
    free(task->ctx);
    free(task);
}

void task_run_cb(loop_t *loop, void *userdata) {
    task_t *task = (task_t*)userdata;
    if (!task || !task->future) return;

    if (task->ctx->waiting_for != NULL) {
        task->ctx->future_result = future_result(task->ctx->waiting_for);
        task->ctx->waiting_for = NULL;
    }

    int ret = task->cb(task, task->ctx);
    if (ret == TASK_ENDED) {
        future_set_result(task->future, task->ctx->result);
        task->ctx->finished = true;
    } else if (ret == TASK_PENDING) {
        future_add_done_callback(task->ctx->waiting_for, (future_cb_t)task_run_cb, task);
    }
}

void task_run(task_t *task) {
    if (!task || !task->loop) return;
    stealthim_loop_call_soon(task->loop, task_run_cb, task);
}

void sleep_cb(loop_t *loop, void *userdata) {
    future_t *fut = (future_t*)userdata;
    if (fut) {
        future_set_result(fut, NULL);
    }
}

future_t *_async_sleep(loop_t *loop, uint64_t ms) {
    future_t *fut = future_create(loop);
    if (!fut) return NULL;
    stealthim_loop_add_timer(loop, ms, sleep_cb, fut);
    return fut;
}
