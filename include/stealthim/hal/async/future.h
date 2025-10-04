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
} stealthim_future_state_t;

typedef struct stealthim_future_s stealthim_future_t;
typedef void (*stealthim_future_cb_t)(stealthim_future_t *fut, void *userdata);

typedef struct stealthim_future_cb_node_s {
    stealthim_future_cb_t cb;
    void *userdata;
    struct stealthim_future_cb_node_s *next;
} stealthim_future_cb_node_t;

struct stealthim_future_s {
    loop_t *loop;
    stealthim_future_state_t state;
    void *result;

    stealthim_future_cb_node_t *callbacks;
};

typedef struct stealthim_callback_data_s {
    stealthim_future_cb_t cb;
    stealthim_future_t *future;
    void *userdata;
} stealthim_callback_data_t;

stealthim_future_t *stealthim_future_create(loop_t *loop);
void stealthim_future_destroy(stealthim_future_t *fut);

void stealthim_future_done(stealthim_future_t *fut, void *result);
void stealthim_future_reject(stealthim_future_t *fut, void *value);
void stealthim_future_cancel(stealthim_future_t *fut);

void stealthim_future_add_done_callback(stealthim_future_t *fut, stealthim_future_cb_t cb, void *userdata);

#define stealthim_future_state(fut) ((fut)->state)
#define stealthim_future_is_finished(fut) (stealthim_future_state(fut) != FUTURE_PENDING)
#define stealthim_future_result(fut) ((fut)->result)
#define stealthim_future_is_pending(fut) (stealthim_future_state(fut) == FUTURE_PENDING)
#define stealthim_future_is_done(fut) (stealthim_future_state(fut) == FUTURE_DONE)
#define stealthim_future_is_rejected(fut) (stealthim_future_state(fut) == FUTURE_REJECTED)
#define stealthim_future_is_cancelled(fut) (stealthim_future_state(fut) == FUTURE_CANCELLED)

#ifdef __cplusplus
}
#endif
