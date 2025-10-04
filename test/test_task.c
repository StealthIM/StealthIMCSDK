#include <stdio.h>

#include "tools.h"
#include "stealthim/hal/async/task.h"

int sub_async(stealthim_task_t *task, stealthim_task_ctx_t *ctx) {
    stealthim_task_local_var();
    stealthim_task_enter();
    printf("3\n");
    stealthim_task_return(4);
    stealthim_task_end();
}

// 打印并执行下一步
int task_callback(stealthim_task_t *task, stealthim_task_ctx_t *ctx) {
    stealthim_task_local_var(int a; int b);
    stealthim_task_enter();
    printf("1\n");
    stealthim_task_await_future(async_sleep(500));
    printf("2\n");
    stealthim_task_await_task(sub_async, NULL);
    printf(":\n");
    stealthim_task_var(b) = stealthim_task_await_res(int);
    stealthim_task_await_future(async_sleep(500));
    printf("%d\n", stealthim_task_var(b));

    for (stealthim_task_var(a)=0; stealthim_task_var(a)<5; stealthim_task_var(a)++) {
        stealthim_task_await_future(async_sleep(500));
        printf("loop %d\n", stealthim_task_var(a));
    }

    stealthim_task_return(6);
    stealthim_task_end();
}

int test_task() {
    loop_t *loop = stealthim_loop_create();

    // 创建并执行一个任务
    stealthim_task_t *task = stealthim_task_create(loop, task_callback, NULL);
    stealthim_task_run(task);

    stealthim_loop_run(loop);

    sleep_ms(5000);

    if (stealthim_task_finished(task)) {
        printf("Result: %d\n", (int)(intptr_t)stealthim_task_result(task));
    }

    stealthim_task_destroy(task);

    stealthim_loop_destroy(loop);
    return 0;
}
