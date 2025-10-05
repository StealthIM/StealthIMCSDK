#pragma once
#include <stdlib.h>

#include "loop.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FUTURE_PENDING,
    FUTURE_DONE,
    FUTURE_REJECTED,
    FUTURE_CANCELLED
} stim_future_state_t;

typedef struct stim_future_s stim_future_t;
typedef void (*stim_future_cb_t)(stim_future_t *fut, void *userdata);

typedef struct stim_future_cb_node_s {
    stim_future_cb_t cb;
    void *userdata;
    struct stim_future_cb_node_s *next;
} stim_future_cb_node_t;

struct stim_future_s {
    loop_t *loop;
    stim_future_state_t state;
    void *result;

    stim_future_cb_node_t *callbacks;
};

typedef struct stim_callback_data_s {
    stim_future_cb_t cb;
    stim_future_t *future;
    void *userdata;
} stim_callback_data_t;

stim_future_t *stim_future_create(loop_t *loop);
void stim_future_destroy(stim_future_t *fut);

void stim_future_done(stim_future_t *fut, void *result);
void stim_future_reject(stim_future_t *fut, void *value);
void stim_future_cancel(stim_future_t *fut);

void stim_future_add_done_callback(stim_future_t *fut, stim_future_cb_t cb, void *userdata);

#define stim_future_state(fut) ((fut)->state)
#define stim_future_is_finished(fut) (stim_future_state(fut) != FUTURE_PENDING)
#define stim_future_result(fut) ((fut)->result)
#define stim_future_is_pending(fut) (stim_future_state(fut) == FUTURE_PENDING)
#define stim_future_is_done(fut) (stim_future_state(fut) == FUTURE_DONE)
#define stim_future_is_rejected(fut) (stim_future_state(fut) == FUTURE_REJECTED)
#define stim_future_is_cancelled(fut) (stim_future_state(fut) == FUTURE_CANCELLED)

#ifdef __cplusplus
}
#endif
