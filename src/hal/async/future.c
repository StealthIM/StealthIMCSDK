#include "stim/hal/async/future.h"

#include "stim/hal/async/loop.h"

static void stim_real_invoke_callback(loop_t *loop, void *data) {
    stim_callback_data_t *callback = data;
    callback->cb(callback->future, callback->userdata);
    free(data);
}

static void stim_real_cleanup(loop_t *loop, void *data) {
    stim_future_t *fut = data;
    stim_future_destroy(fut);
}

static void stim_future_invoke_callbacks(stim_future_t *fut) {
    stim_future_cb_node_t *node = fut->callbacks;
    while (node) {
        stim_callback_data_t *data = calloc(1, sizeof(stim_callback_data_t));
        data->userdata = node->userdata;
        data->future = fut;
        data->cb = node->cb;
        stim_loop_call_soon(fut->loop, stim_real_invoke_callback, data);
        node = node->next;
    }
    stim_loop_call_soon(fut->loop, stim_real_cleanup, fut);
}

stim_future_t *stim_future_create(loop_t *loop) {
    stim_future_t *f = (stim_future_t*)calloc(1, sizeof(stim_future_t));
    f->state = FUTURE_PENDING;
    f->loop = loop;
    return f;
}

void stim_future_destroy(stim_future_t *fut) {
    stim_future_cb_node_t *node = fut->callbacks;
    while (node) {
        stim_future_cb_node_t *next = node->next;
        free(node);
        node = next;
    }
    free(fut);
}

void stim_future_done(stim_future_t *fut, void *result) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_DONE;
    fut->result = result;
    stim_future_invoke_callbacks(fut);
}

void stim_future_reject(stim_future_t *fut, void *value) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_REJECTED;
    fut->result = value;
    stim_future_invoke_callbacks(fut);
}

void stim_future_cancel(stim_future_t *fut) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_CANCELLED;
    stim_future_invoke_callbacks(fut);
}

void stim_future_add_done_callback(stim_future_t *fut, stim_future_cb_t cb, void *userdata) {
    stim_future_cb_node_t *node = (stim_future_cb_node_t*)malloc(sizeof(stim_future_cb_node_t));
    node->cb = cb;
    node->userdata = userdata;
    node->next = NULL;

    // append to list
    if (!fut->callbacks) {
        fut->callbacks = node;
    } else {
        stim_future_cb_node_t *cur = fut->callbacks;
        while (cur->next) cur = cur->next;
        cur->next = node;
    }

    // 如果已经完成，立即触发一次
    if (fut->state != FUTURE_PENDING) {
        cb(fut, userdata);
    }
}
