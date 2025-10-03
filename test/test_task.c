#include <stdio.h>

#include "tools.h"
#include "stealthim/hal/async/task.h"

int sub_async(task_t *task, task_ctx_t *ctx) {
    task_local_var();
    task_enter(task, ctx);
    printf("3\n");
    task_return(4);
    task_end();
}

// 打印并执行下一步
int task_callback(task_t *task, task_ctx_t *ctx) {
    task_local_var(int a);
    task_enter(task, ctx);
    printf("1\n");
    task_await_future(async_sleep(500));
    printf("2\n");
    task_await_task(task_create(task_loop(task), sub_async, NULL));
    task_var(a) = task_await_res(int);
    task_await_future(async_sleep(500));
    printf("%d\n", task_var(a));
    task_return(5);
    task_end();
}

int test_task() {
    loop_t *loop = stealthim_loop_create();

    // 创建并执行一个任务
    task_t *task = task_create(loop, task_callback, NULL);
    task_run(task);

    stealthim_loop_run(loop);

    sleep_ms(2000);

    if (task_finished(task)) {
        printf("Result: %d\n", (int)(intptr_t)task_result(task));
    }

    task_destroy(task);

    stealthim_loop_destroy(loop);
    return 0;
}
