#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GEN_STATE_START = 0,
    GEN_STATE_MIDDLE,
    GEN_STATE_YIELD_FROM,
    GEN_STATE_END,
} stim_gen_state_t;

typedef enum {
    GEN_NORMAL = 0,
    GEN_YIELD_FROM
} stim_gen_ret_t;

typedef struct stim_gen_s stim_gen_t;

typedef struct stim_gen_ctx_s {
    int lineno;
    stim_gen_state_t state;
    void *stack_vars;
    void *userdata;
    void *ret_val;
    stim_gen_t *sub_gen;
} stim_gen_ctx_t;

typedef stim_gen_ret_t (stim_gen_func)(stim_gen_ctx_t *ctx, void *arg);

typedef struct stim_gen_s {
    stim_gen_ctx_t ctx;
    stim_gen_func *func;
} stim_gen_t;

stim_gen_t *stim_gen_create(stim_gen_func *func, void *userdata);
void stim_gen_destroy(stim_gen_t *gen);

#define stim_gen_dec_vars(vars) \
    typedef struct { vars; } __stack_vars;

#define stim_gen_var(var) (((__stack_vars*)(__ctx->stack_vars))->var)

#define stim_gen_begin(ctx) \
    stim_gen_ctx_t *__ctx = ctx; \
    switch(ctx->lineno) { \
        case 0: \
            ctx->stack_vars = calloc(1, sizeof(__stack_vars)); \
        case __LINE__:
#define stim_gen_yield(val) \
            __ctx->lineno = __LINE__; \
            __ctx->state = GEN_STATE_MIDDLE; \
            __ctx->ret_val = (void*)(val); \
            return GEN_NORMAL; \
        case __LINE__:;
// #define stim_gen_cleanup() \
//         case -1:
#define stim_gen_end(val) \
        __ctx->state = GEN_STATE_END; \
        __ctx->ret_val = (void*)(val); \
        return GEN_NORMAL; \
    }
#define stim_gen_userdata() (__ctx->userdata)
#define stim_gen_finished(gen) ((gen)->ctx.state == GEN_STATE_END)
#define stim_gen_for(type, val, gen) \
    type val; \
    for(;!stim_gen_finished(gen) && ((val = (type)stim_gen_next(gen)) || 1); )

#define _CONNECT1(x,y) x##y
#define _CONNECT2(x,y) _CONNECT1(x,y)
#define stim_gen_yield_from(gen) \
        __ctx->sub_gen = (gen); \
        __ctx->state = GEN_STATE_YIELD_FROM; \
        __ctx->lineno = __LINE__; \
        return GEN_YIELD_FROM; \
    case __LINE__:

void *_stim_gen_next(stim_gen_t *gen, void *arg);
#define stim_gen_next(gen) _stim_gen_next((gen), NULL)
#define stim_gen_send(gen, val) _stim_gen_next((gen), (void*)(val))
// void stim_gen_close(stim_gen_t *gen);

#ifdef __cplusplus
}
#endif
