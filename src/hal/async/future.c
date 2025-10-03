#include "stealthim/hal/async/future.h"

#include "stealthim/hal/async/loop.h"

typedef struct future_cb_node_s {
    future_cb_t cb;
    void *userdata;
    struct future_cb_node_s *next;
} future_cb_node_t;

struct future_s {
    loop_t *loop;
    future_state_t state;
    void *result;
    int error;

    future_cb_node_t *callbacks;
};

typedef struct callback_data_s {
    future_cb_t cb;
    future_t *future;
    void *userdata;
} callback_data_t;

static void real_invoke_callback(loop_t *loop, void *data) {
    callback_data_t *callback = data;
    callback->cb(callback->future, callback->userdata);
    free(data);
}

static void future_invoke_callbacks(future_t *fut) {
    future_cb_node_t *node = fut->callbacks;
    while (node) {
        callback_data_t *data = calloc(1, sizeof(callback_data_t));
        data->userdata = node->userdata;
        data->future = fut;
        data->cb = node->cb;
        stealthim_loop_call_soon(fut->loop, real_invoke_callback, data);
        node = node->next;
    }
}

future_t *future_create(loop_t *loop) {
    future_t *f = (future_t*)calloc(1, sizeof(future_t));
    f->state = FUTURE_PENDING;
    f->loop = loop;
    return f;
}

void future_destroy(future_t *fut) {
    future_cb_node_t *node = fut->callbacks;
    while (node) {
        future_cb_node_t *next = node->next;
        free(node);
        node = next;
    }
    free(fut);
}

void future_set_result(future_t *fut, void *result) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_DONE;
    fut->result = result;
    future_invoke_callbacks(fut);
}

void future_set_error(future_t *fut, int err) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_DONE;
    fut->error = err;
    future_invoke_callbacks(fut);
}

void future_cancel(future_t *fut) {
    if (fut->state != FUTURE_PENDING) return;
    fut->state = FUTURE_CANCELLED;
    future_invoke_callbacks(fut);
}

void future_add_done_callback(future_t *fut, future_cb_t cb, void *userdata) {
    future_cb_node_t *node = (future_cb_node_t*)malloc(sizeof(future_cb_node_t));
    node->cb = cb;
    node->userdata = userdata;
    node->next = NULL;

    // append to list
    if (!fut->callbacks) {
        fut->callbacks = node;
    } else {
        future_cb_node_t *cur = fut->callbacks;
        while (cur->next) cur = cur->next;
        cur->next = node;
    }

    // 如果已经完成，立即触发一次
    if (fut->state != FUTURE_PENDING) {
        cb(fut, userdata);
    }
}

int future_done(future_t *fut) {
    return fut->state != FUTURE_PENDING;
}

future_state_t future_state(future_t *fut) {
    return fut->state;
}

void *future_result(future_t *fut) {
    return fut->result;
}

int future_error(future_t *fut) {
    return fut->error;
}
