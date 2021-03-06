#include "substitute-internal.h"
#ifdef TARGET_DIS_SUPPORTED

#include "substitute.h"
#include "dis.h"
#include <stdbool.h>
#include <stdint.h>

#define P(x) transform_dis_##x

struct transform_dis_ctx {
    /* outputs */
    bool modify;
    int err;

    uintptr_t pc_patch_start;
    uintptr_t pc_patch_end;
    uintptr_t pc;
    int op_size;
    unsigned op;
    unsigned newop;
    unsigned newval[4];

    const void *ptr;
    void **rewritten_ptr_ptr;
    void *write_newop_here;

    struct arch_dis_ctx arch;
};

#define tdis_ctx struct transform_dis_ctx *
#define TDIS_CTX_MODIFY(ctx) ((ctx)->modify)
#define TDIS_CTX_NEWVAL(ctx, n) ((ctx)->newval[n])
#define TDIS_CTX_NEWOP(ctx) ((ctx)->newop)
#define TDIS_CTX_SET_NEWOP(ctx, new) ((ctx)->newop = (new))

/* largely similar to jump_dis */

static INLINE UNUSED void transform_dis_ret(struct transform_dis_ctx *ctx) {
    /* ret is okay if it's at the end of the patch */
    if (ctx->pc + ctx->op_size < ctx->pc_patch_end)
        ctx->err = SUBSTITUTE_ERR_FUNC_TOO_SHORT;
}

static INLINE UNUSED void transform_dis_branch(struct transform_dis_ctx *ctx,
        uintptr_t dpc, UNUSED bool conditional) {
#ifdef TRANSFORM_DIS_VERBOSE
    printf("transform_dis (%p): branch => %p\n", (void *) ctx->pc, (void *) dpc);
#endif
    if (dpc >= ctx->pc_patch_start && dpc < ctx->pc_patch_end) {
        /* don't support this for now */
        ctx->err = SUBSTITUTE_ERR_FUNC_BAD_INSN_AT_START;
    }
    /* branch out of bounds is fine */
    /* XXX just kidding, the instruction needs to be rewritten obviously.  what
     * was I thinking? */
}

static INLINE UNUSED void transform_dis_unidentified(UNUSED struct transform_dis_ctx *ctx) {
#ifdef TRANSFORM_DIS_VERBOSE
    printf("transform_dis (%p): unidentified\n", (void *) ctx->pc);
#endif
    /* this isn't exhaustive, so unidentified is fine */
}

static INLINE UNUSED void transform_dis_bad(struct transform_dis_ctx *ctx) {
    ctx->err = SUBSTITUTE_ERR_FUNC_BAD_INSN_AT_START;
}


static void transform_dis_dis(struct transform_dis_ctx *ctx);

int transform_dis_main(const void *restrict code_ptr,
                       void **restrict rewritten_ptr_ptr,
                       uintptr_t pc_patch_start,
                       uintptr_t pc_patch_end,
                       struct arch_dis_ctx initial_arch_ctx,
                       int *offset_by_pcdiff) {
    struct transform_dis_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pc_patch_start = pc_patch_start;
    ctx.pc_patch_end = pc_patch_end;
    ctx.pc = pc_patch_start;
    ctx.arch = initial_arch_ctx;
    /* data is written to rewritten both by this function directly and, in case
     * additional scaffolding is needed, by arch-specific transform_dis_* */
    ctx.rewritten_ptr_ptr = rewritten_ptr_ptr;
    void *rewritten_start = *rewritten_ptr_ptr;
    int written_pcdiff = 0;
    offset_by_pcdiff[written_pcdiff++] = 0;
    while (ctx.pc < ctx.pc_patch_end) {
        ctx.modify = false;
        ctx.err = 0;
        ctx.ptr = code_ptr + (ctx.pc - pc_patch_start);
        void *rewritten_ptr = *rewritten_ptr_ptr;
        ctx.write_newop_here = rewritten_ptr;
        transform_dis_dis(&ctx);

        if (ctx.err)
            return ctx.err;
        if (ctx.write_newop_here != NULL) {
            if (!ctx.modify)
                ctx.newop = ctx.op;
            if (ctx.op_size == 4)
                *(uint32_t *) ctx.write_newop_here = ctx.newop;
            else if (ctx.op_size == 2)
                *(uint16_t *) ctx.write_newop_here = ctx.newop;
            else
                __builtin_abort();
            if (*rewritten_ptr_ptr == rewritten_ptr)
                *rewritten_ptr_ptr += ctx.op_size;
        }
        ctx.pc += ctx.op_size;
        int pcdiff = ctx.pc - ctx.pc_patch_start;
        while (written_pcdiff < pcdiff)
            offset_by_pcdiff[written_pcdiff++] = -1;
        offset_by_pcdiff[written_pcdiff++] = (int) (*rewritten_ptr_ptr - rewritten_start);
    }
    return SUBSTITUTE_OK;
}

#include TARGET_TRANSFORM_DIS_HEADER
#include TARGET_DIS_HEADER

#endif /* TARGET_DIS_SUPPORTED */
