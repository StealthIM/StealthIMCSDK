#pragma once
#include <stdlib.h>

#include "loop.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FUTURE_PENDING,
    FUTURE_DONE,
    FUTURE_CANCELLED
} future_state_t;

typedef struct future_s future_t;
typedef void (*future_cb_t)(future_t *fut, void *userdata);

future_t *future_create(loop_t *loop);
void future_destroy(future_t *fut);

void future_set_result(future_t *fut, void *result);
void future_set_error(future_t *fut, int err);
void future_cancel(future_t *fut);

void future_add_done_callback(future_t *fut, future_cb_t cb, void *userdata);

int future_done(future_t *fut);
future_state_t future_state(future_t *fut);
void *future_result(future_t *fut);
int future_error(future_t *fut);

#ifdef __cplusplus
}
#endif
