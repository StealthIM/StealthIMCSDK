#include <stdio.h>
#include "stealthim/hal/async/future.h"
#include "stealthim/hal/async/loop.h"

#include "tools.h"

static void on_future_done(future_t *fut, void *userdata) {
    const char *name = (const char*)userdata;
    printf("[%s] completed, state=%d, result=%s, error=%d\n",
           name,
           future_state(fut),
           future_result(fut) ? (char*)future_result(fut) : "(null)",
           future_error(fut));
}

static void on_timer_set(loop_t *loop, void *userdata) {
    future_t *fut = (future_t*)userdata;
    printf("Timer fired -> setting result\n");
    future_set_result(fut, "Hello from future!");
}

static void on_timer_cancel(loop_t *loop, void *userdata) {
    future_t *fut = (future_t*)userdata;
    printf("Timer fired -> cancelling future\n");
    future_cancel(fut);
}

int test_future() {
    loop_t *loop = stealthim_loop_create();

    // --- Future 1: 正常完成
    future_t *f1 = future_create();
    future_add_done_callback(f1, on_future_done, "f1-cb1");
    future_add_done_callback(f1, on_future_done, "f1-cb2");
    stealthim_loop_add_timer(loop, 1000, on_timer_set, f1);

    // --- Future 2: 被取消
    future_t *f2 = future_create();
    future_add_done_callback(f2, on_future_done, "f2-cb1");
    stealthim_loop_add_timer(loop, 500, on_timer_cancel, f2);

    stealthim_loop_run(loop);

    sleep_ms(2000); // 让事件循环跑一会儿

    future_destroy(f1);
    future_destroy(f2);
    stealthim_loop_destroy(loop);
    return 0;
}
