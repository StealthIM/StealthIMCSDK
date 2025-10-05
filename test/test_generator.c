#include <stddef.h>
#include <stdio.h>
#include <stim/hal/async/generator.h>

stim_gen_ret_t _sub_gen(stim_gen_ctx_t *ctx, void *arg) {
    stim_gen_dec_vars(int i);
    stim_gen_begin(ctx);
    for (stim_gen_var(i)=0; stim_gen_var(i)<5; stim_gen_var(i)++) {
        printf("Sub loop %d\n", stim_gen_var(i));
        stim_gen_yield(100 + stim_gen_var(i));
    }
    stim_gen_end(200);
}

stim_gen_t *sub_gen() {
    stim_gen_t *gen = stim_gen_create(_sub_gen, NULL);
    return gen;
}

stim_gen_ret_t _gen(stim_gen_ctx_t *ctx, void *arg) {
    stim_gen_dec_vars(int val; int i; stim_gen_t *sub);
    stim_gen_begin(ctx);
    printf("Userdata: %d\n", (int)stim_gen_userdata());
    stim_gen_yield(1);
    stim_gen_var(val) = (int)arg;
    stim_gen_var(sub) = sub_gen();
    stim_gen_next(stim_gen_var(sub));
    stim_gen_yield_from(stim_gen_var(sub));
    stim_gen_yield_from(sub_gen());
    stim_gen_yield(2);
    for (stim_gen_var(i)=3; stim_gen_var(i)<9; stim_gen_var(i)++) {
        printf("Loop %d\n", stim_gen_var(i));
        stim_gen_yield(stim_gen_var(i));
    }
    printf("Last val: %d\n", stim_gen_var(val));
    // stim_gen_cleanup();
    // if (stim_gen_var(sub)) {
    //
    // }
    printf("Do clean up\n");
    stim_gen_end(10);
}

stim_gen_t *gen(int data) {
    stim_gen_t *gen = stim_gen_create(_gen, (void*)data);
    return gen;
}

int test_gen() {
    stim_gen_t *generator = gen(42);
    if (!generator) return -1;
    printf("Next: %d\n", (int)stim_gen_next(generator));
    printf("Next: %d\n", (int)stim_gen_send(generator, 9));
    stim_gen_for(int, i, generator) {
        printf("Next: %d\n", i);
    }
    stim_gen_destroy(generator);
    return 0;
}