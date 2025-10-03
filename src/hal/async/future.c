#include "stealthim/hal/async/future.h"

typedef struct future_cb_node_s {
    future_cb_t cb;
    void *userdata;
    struct future_cb_node_s *next;
} future_cb_node_t;

struct future_s {
    future_state_t state;
    void *result;
    int error;

    future_cb_node_t *callbacks;
};

static void future_invoke_callbacks(future_t *fut) {
    future_cb_node_t *node = fut->callbacks;
    while (node) {
        node->cb(fut, node->userdata);
        node = node->next;
    }
}

future_t *future_create() {
    future_t *f = (future_t*)calloc(1, sizeof(future_t));
    f->state = FUTURE_PENDING;
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
