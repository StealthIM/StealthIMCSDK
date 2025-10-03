#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stealthim/config.h>

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
loop_t *loop_create(const char *backend_name);
void loop_destroy(loop_t *loop);

void loop_run(loop_t *loop);
void loop_stop(loop_t *loop);

// register a HANDLE/SOCKET with the loop; userdata returned when completion arrives
// note: on Windows, registering a SOCKET/FILE handle just associates it with IOCP
int loop_register_handle(loop_t *loop, void *handle, loop_cb_t cb, void *userdata);
int loop_unregister_handle(loop_t *loop, void *handle);

// timers
typedef int32_t timer_id_t;
timer_id_t loop_add_timer(loop_t *loop, uint64_t when_ms, loop_cb_t cb, void *userdata);
int loop_cancel_timer(loop_t *loop, timer_id_t id);

// call soon / thread-safe post
void loop_call_soon(loop_t *loop, loop_cb_t cb, void *userdata); // caller is loop thread ok
void loop_post(loop_t *loop, loop_cb_t cb, void *userdata); // thread-safe

// convenience
uint64_t loop_time_ms(loop_t *loop);

#ifdef __cplusplus
}
#endif
