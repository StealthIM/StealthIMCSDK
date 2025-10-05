#include <stdio.h>
#include "stim/hal/async/future.h"
#include "stim/hal/async/loop.h"

#include "tools.h"

static void on_future_done(stim_future_t *fut, void *userdata) {
    const char *name = (const char*)userdata;
    printf("[%s] completed, state=%d, result=%s, error=%d\n",
           name,
           stim_future_state(fut),
           stim_future_result(fut) ? (char*)stim_future_result(fut) : "(null)",
           stim_future_is_rejected(fut));
}

static void on_timer_set(loop_t *loop, void *userdata) {
    stim_future_t *fut = (stim_future_t*)userdata;
    printf("Timer fired -> setting result\n");
    stim_future_done(fut, "Hello from future!");
}

static void on_timer_cancel(loop_t *loop, void *userdata) {
    stim_future_t *fut = (stim_future_t*)userdata;
    printf("Timer fired -> cancelling future\n");
    stim_future_cancel(fut);
}

int test_future() {
    loop_t *loop = stim_loop_create();

    // --- Future 1: 正常完成
    stim_future_t *f1 = stim_future_create(loop);
    stim_future_add_done_callback(f1, on_future_done, "f1-cb1");
    stim_future_add_done_callback(f1, on_future_done, "f1-cb2");
    stim_loop_add_timer(loop, 1000, on_timer_set, f1);

    // --- Future 2: 被取消
    stim_future_t *f2 = stim_future_create(loop);
    stim_future_add_done_callback(f2, on_future_done, "f2-cb1");
    stim_loop_add_timer(loop, 500, on_timer_cancel, f2);

    stim_loop_run(loop);

    sleep_ms(2000); // 让事件循环跑一会儿

    stim_loop_destroy(loop);
    return 0;
}
