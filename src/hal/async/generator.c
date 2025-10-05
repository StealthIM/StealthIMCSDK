#include <stddef.h>
#include <stdlib.h>
#include <stim/hal/async/generator.h>

stim_gen_t *stim_gen_create(stim_gen_func *func, void *userdata) {
    stim_gen_t *gen = (stim_gen_t*)calloc(1, sizeof(stim_gen_t));
    if (!gen) return NULL;
    gen->func = func;
    gen->ctx.userdata = userdata;
    gen->ctx.lineno = 0;
    gen->ctx.state = GEN_STATE_START;
    gen->ctx.stack_vars = NULL;
    return gen;
}

void stim_gen_destroy(stim_gen_t *gen) {
    if (gen->ctx.stack_vars) free(gen->ctx.stack_vars);
    if (gen) free(gen);
}

void *_stim_gen_next(stim_gen_t *gen, void *arg) {
    if (gen->ctx.state == GEN_STATE_END) return NULL;
    if (gen->ctx.state == GEN_STATE_YIELD_FROM) {
        if (stim_gen_finished(gen->ctx.sub_gen)) {
            stim_gen_destroy(gen->ctx.sub_gen);
            gen->ctx.sub_gen = NULL;
            gen->ctx.state = GEN_STATE_MIDDLE;
        } else {
            return _stim_gen_next(gen->ctx.sub_gen, NULL);
        }
    }
    stim_gen_ret_t ret = gen->func(&gen->ctx, arg);
    if (ret == GEN_YIELD_FROM) {
        return _stim_gen_next(gen->ctx.sub_gen, NULL);
    }
    return gen->ctx.ret_val;
}

// void stim_gen_close(stim_gen_t *gen) {
//
// }
