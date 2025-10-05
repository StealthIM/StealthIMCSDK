#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stim/config.h>

typedef struct loop_s loop_t;
typedef struct recv_data_s {
    void *userdata;
    char *data;
    unsigned long len;
} recv_data_t;
typedef void (*loop_cb_t)(loop_t *loop, void *userdata);

#define LOOP_EV_READ 0x01
#define LOOP_EV_WRITE 0x02

// create backend by name, NULL => auto
loop_t *stim_loop_create();
void stim_loop_destroy(loop_t *loop);

void stim_loop_run(loop_t *loop);
void stim_loop_stop(loop_t *loop);

int stim_loop_register_handle(loop_t *loop, void *handle, loop_cb_t cb, void *userdata);
int stim_loop_unregister_handle(loop_t *loop, void *handle, bool close_socket);

// timers
typedef int32_t timer_id_t;
timer_id_t stim_loop_add_timer(loop_t *loop, uint64_t when_ms, loop_cb_t cb, void *userdata);
int stim_loop_cancel_timer(loop_t *loop, timer_id_t id);

// call soon / thread-safe post
void stim_loop_call_soon(loop_t *loop, loop_cb_t cb, void *userdata); // caller is loop thread ok
void stim_loop_post(loop_t *loop, loop_cb_t cb, void *userdata); // thread-safe

// convenience
uint64_t stim_loop_time_ms(loop_t *loop);

#ifdef __cplusplus
}
#endif
