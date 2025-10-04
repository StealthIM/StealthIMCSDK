#include "stealthim/hal/async/future.h"

#include "stealthim/hal/async/loop.h"

static void stealthim_real_invoke_callback(loop_t *loop, void *data) {
    stealthim_callback_data_t *callback = data;
    callback->cb(callback->future, callback->userdata);
    free(data);
}

static void stealthim_real_cleanup(loop_t *loop, void *data) {
    stealthim_future_t *fut = data;
    stealthim_future_destroy(fut);
}

static void stealthim_future_invoke_callbacks(stealthim_future_t *fut) {
    stealthim_future_cb_node_t *node = fut->callbacks;
    while (node) {
        stealthim_callback_data_t *data = calloc(1, sizeof(stealthim_callback_data_t));
        data->userdata = node->userdata;
        data->future = fut;
        data->cb = node->cb;
        stealthim_loop_call_soon(fut->loop, stealthim_real_invoke_callback, data);
        node = node->next;
    }
    stealthim_loop_call_soon(fut->loop, stealthim_real_cleanup, fut);
}

stealthim_future_t *stealthim_future_create(loop_t *loop) {
    stealthim_future_t *f = (stealthim_future_t*)calloc(1, sizeof(stealthim_future_t));
    f->state = FUTURE_PENDING;
    f->loop = loop;
    return f;
}

void stealthim_future_destroy(stealthim_future_t *fut) {
    stealthim_future_cb_node_t *node = fut->callbacks;
    while (node) {
        stealthim_future_cb_node_t *next = node->next;
        free(node);
        node = next;
    }
    free(fut);
}

void stealthim_future_done(stealthim_future_t *fut, void *result) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_DONE;
    fut->result = result;
    stealthim_future_invoke_callbacks(fut);
}

void stealthim_future_reject(stealthim_future_t *fut, void *value) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_REJECTED;
    fut->result = value;
    stealthim_future_invoke_callbacks(fut);
}

void stealthim_future_cancel(stealthim_future_t *fut) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_CANCELLED;
    stealthim_future_invoke_callbacks(fut);
}

void stealthim_future_add_done_callback(stealthim_future_t *fut, stealthim_future_cb_t cb, void *userdata) {
    stealthim_future_cb_node_t *node = (stealthim_future_cb_node_t*)malloc(sizeof(stealthim_future_cb_node_t));
    node->cb = cb;
    node->userdata = userdata;
    node->next = NULL;

    // append to list
    if (!fut->callbacks) {
        fut->callbacks = node;
    } else {
        stealthim_future_cb_node_t *cur = fut->callbacks;
        while (cur->next) cur = cur->next;
        cur->next = node;
    }

    // 如果已经完成，立即触发一次
    if (fut->state != FUTURE_PENDING) {
        cb(fut, userdata);
    }
}
