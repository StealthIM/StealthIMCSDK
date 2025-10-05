#include <stdio.h>

#include "tools.h"
#include "stim/hal/async/task.h"

int sub_async(stim_task_t *task, stim_task_ctx_t *ctx) {
    stim_task_local_var();
    stim_task_enter();
    printf("3\n");
    stim_task_return(4);
    stim_task_end();
}

// 打印并执行下一步
int task_callback(stim_task_t *task, stim_task_ctx_t *ctx) {
    stim_task_local_var(int a; int b);
    stim_task_enter();
    printf("1\n");
    stim_task_await_future(async_sleep(500));
    printf("2\n");
    stim_task_await_task(sub_async, NULL);
    printf(":\n");
    stim_task_var(b) = stim_task_await_res(int);
    stim_task_await_future(async_sleep(500));
    printf("%d\n", stim_task_var(b));

    for (stim_task_var(a)=0; stim_task_var(a)<5; stim_task_var(a)++) {
        stim_task_await_future(async_sleep(500));
        printf("loop %d\n", stim_task_var(a));
    }

    stim_task_return(6);
    stim_task_end();
}

int test_task() {
    loop_t *loop = stim_loop_create();

    // 创建并执行一个任务
    stim_task_t *task = stim_task_create(loop, task_callback, NULL);
    stim_task_run(task);

    stim_loop_run(loop);

    sleep_ms(5000);

    if (stim_task_finished(task)) {
        printf("Result: %d\n", (int)(intptr_t)stim_task_result(task));
    }

    stim_task_destroy(task);

    stim_loop_destroy(loop);
    return 0;
}
