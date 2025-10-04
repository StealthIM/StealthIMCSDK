#include <stdio.h>
#include "stealthim/hal/async/future.h"
#include "stealthim/hal/async/loop.h"

#include "tools.h"

static void on_future_done(stealthim_future_t *fut, void *userdata) {
    const char *name = (const char*)userdata;
    printf("[%s] completed, state=%d, result=%s, error=%d\n",
           name,
           stealthim_future_state(fut),
           stealthim_future_result(fut) ? (char*)stealthim_future_result(fut) : "(null)",
           stealthim_future_is_rejected(fut));
}

static void on_timer_set(loop_t *loop, void *userdata) {
    stealthim_future_t *fut = (stealthim_future_t*)userdata;
    printf("Timer fired -> setting result\n");
    stealthim_future_done(fut, "Hello from future!");
}

static void on_timer_cancel(loop_t *loop, void *userdata) {
    stealthim_future_t *fut = (stealthim_future_t*)userdata;
    printf("Timer fired -> cancelling future\n");
    stealthim_future_cancel(fut);
}

int test_future() {
    loop_t *loop = stealthim_loop_create();

    // --- Future 1: 正常完成
    stealthim_future_t *f1 = stealthim_future_create(loop);
    stealthim_future_add_done_callback(f1, on_future_done, "f1-cb1");
    stealthim_future_add_done_callback(f1, on_future_done, "f1-cb2");
    stealthim_loop_add_timer(loop, 1000, on_timer_set, f1);

    // --- Future 2: 被取消
    stealthim_future_t *f2 = stealthim_future_create(loop);
    stealthim_future_add_done_callback(f2, on_future_done, "f2-cb1");
    stealthim_loop_add_timer(loop, 500, on_timer_cancel, f2);

    stealthim_loop_run(loop);

    sleep_ms(2000); // 让事件循环跑一会儿

    stealthim_loop_destroy(loop);
    return 0;
}
