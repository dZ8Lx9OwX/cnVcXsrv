/*
 * Copyright (C) 2015-2018 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#include "ir3_context.h"
#include "ir3_compiler.h"
#include "ir3_image.h"
#include "ir3_nir.h"
#include "ir3_shader.h"
#include "nir.h"
#include "nir_intrinsics_indices.h"

struct ir3_context *
ir3_context_init(struct ir3_compiler *compiler, struct ir3_shader *shader,
                 struct ir3_shader_variant *so)
{
   MESA_TRACE_FUNC();

   struct ir3_context *ctx = rzalloc(NULL, struct ir3_context);

   if (compiler->gen == 4) {
      if (so->type == MESA_SHADER_VERTEX) {
         ctx->astc_srgb = so->key.vastc_srgb;
         memcpy(ctx->sampler_swizzles, so->key.vsampler_swizzles, sizeof(ctx->sampler_swizzles));
      } else if (so->type == MESA_SHADER_FRAGMENT ||
            so->type == MESA_SHADER_COMPUTE) {
         ctx->astc_srgb = so->key.fastc_srgb;
         memcpy(ctx->sampler_swizzles, so->key.fsampler_swizzles, sizeof(ctx->sampler_swizzles));
      }
   } else if (compiler->gen == 3) {
      if (so->type == MESA_SHADER_VERTEX) {
         ctx->samples = so->key.vsamples;
      } else if (so->type == MESA_SHADER_FRAGMENT) {
         ctx->samples = so->key.fsamples;
      }
   }

   if (compiler->gen >= 6) {
      ctx->funcs = &ir3_a6xx_funcs;
   } else if (compiler->gen >= 4) {
      ctx->funcs = &ir3_a4xx_funcs;
   }

   ctx->compiler = compiler;
   ctx->so = so;
   ctx->def_ht =
      _mesa_hash_table_create(ctx, _mesa_hash_pointer, _mesa_key_pointer_equal);
   ctx->block_ht =
      _mesa_hash_table_create(ctx, _mesa_hash_pointer, _mesa_key_pointer_equal);
   ctx->continue_block_ht =
      _mesa_hash_table_create(ctx, _mesa_hash_pointer, _mesa_key_pointer_equal);
   ctx->sel_cond_conversions =
      _mesa_hash_table_create(ctx, _mesa_hash_pointer, _mesa_key_pointer_equal);

   /* TODO: maybe generate some sort of bitmask of what key
    * lowers vs what shader has (ie. no need to lower
    * texture clamp lowering if no texture sample instrs)..
    * although should be done further up the stack to avoid
    * creating duplicate variants..
    */

   ctx->s = nir_shader_clone(ctx, shader->nir);
   ir3_nir_lower_variant(so, ctx->s);

   bool progress = false;
   bool needs_late_alg = false;

   /* We want to lower nir_op_imul as late as possible, to catch also
    * those generated by earlier passes (e.g,
    * nir_lower_locals_to_regs).  However, we want a final swing of a
    * few passes to have a chance at optimizing the result.
    */
   NIR_PASS(progress, ctx->s, ir3_nir_lower_imul);
   while (progress) {
      progress = false;
      NIR_PASS(progress, ctx->s, nir_opt_algebraic);
      NIR_PASS(progress, ctx->s, nir_opt_copy_prop_vars);
      NIR_PASS(progress, ctx->s, nir_opt_dead_write_vars);
      NIR_PASS(progress, ctx->s, nir_opt_dce);
      NIR_PASS(progress, ctx->s, nir_opt_constant_folding);
      needs_late_alg = true;
   }

   /* nir_opt_algebraic() above would have unfused our ffmas, re-fuse them. */
   if (needs_late_alg) {
      NIR_PASS(progress, ctx->s, nir_opt_algebraic_late);
      NIR_PASS(progress, ctx->s, nir_opt_dce);
   }

   /* Enable the texture pre-fetch feature only a4xx onwards.  But
    * only enable it on generations that have been tested:
    */
   if ((so->type == MESA_SHADER_FRAGMENT) && compiler->has_fs_tex_prefetch)
      NIR_PASS_V(ctx->s, ir3_nir_lower_tex_prefetch);

   NIR_PASS(progress, ctx->s, nir_lower_phis_to_scalar, true);

   /* Super crude heuristic to limit # of tex prefetch in small
    * shaders.  This completely ignores loops.. but that's really
    * not the worst of it's problems.  (A frag shader that has
    * loops is probably going to be big enough to not trigger a
    * lower threshold.)
    *
    *   1) probably want to do this in terms of ir3 instructions
    *   2) probably really want to decide this after scheduling
    *      (or at least pre-RA sched) so we have a rough idea about
    *      nops, and don't count things that get cp'd away
    *   3) blob seems to use higher thresholds with a mix of more
    *      SFU instructions.  Which partly makes sense, more SFU
    *      instructions probably means you want to get the real
    *      shader started sooner, but that considers where in the
    *      shader the SFU instructions are, which blob doesn't seem
    *      to do.
    *
    * This uses more conservative thresholds assuming a more alu
    * than sfu heavy instruction mix.
    */
   if (so->type == MESA_SHADER_FRAGMENT) {
      nir_function_impl *fxn = nir_shader_get_entrypoint(ctx->s);

      unsigned instruction_count = 0;
      nir_foreach_block (block, fxn) {
         instruction_count += exec_list_length(&block->instr_list);
      }

      if (instruction_count < 50) {
         ctx->prefetch_limit = 2;
      } else if (instruction_count < 70) {
         ctx->prefetch_limit = 3;
      } else {
         ctx->prefetch_limit = IR3_MAX_SAMPLER_PREFETCH;
      }
   }

   if (shader_debug_enabled(so->type, ctx->s->info.internal)) {
      mesa_logi("NIR (final form) for %s shader %s:", ir3_shader_stage(so),
                so->name);
      nir_log_shaderi(ctx->s);
   }

   ir3_ibo_mapping_init(&so->image_mapping, ctx->s->info.num_textures);

   return ctx;
}

void
ir3_context_free(struct ir3_context *ctx)
{
   ralloc_free(ctx);
}

/*
 * Misc helpers
 */

/* allocate a n element value array (to be populated by caller) and
 * insert in def_ht
 */
struct ir3_instruction **
ir3_get_dst_ssa(struct ir3_context *ctx, nir_def *dst, unsigned n)
{
   struct ir3_instruction **value =
      ralloc_array(ctx->def_ht, struct ir3_instruction *, n);
   _mesa_hash_table_insert(ctx->def_ht, dst, value);
   return value;
}

struct ir3_instruction **
ir3_get_def(struct ir3_context *ctx, nir_def *def, unsigned n)
{
   struct ir3_instruction **value = ir3_get_dst_ssa(ctx, def, n);

   compile_assert(ctx, !ctx->last_dst);
   ctx->last_dst = value;
   ctx->last_dst_n = n;

   return value;
}

struct ir3_instruction *const *
ir3_get_src(struct ir3_context *ctx, nir_src *src)
{
   struct hash_entry *entry;
   entry = _mesa_hash_table_search(ctx->def_ht, src->ssa);
   compile_assert(ctx, entry);
   return entry->data;
}

void
ir3_put_def(struct ir3_context *ctx, nir_def *def)
{
   unsigned bit_size = ir3_bitsize(ctx, def->bit_size);

   /* add extra mov if dst value is shared reg.. in some cases not all
    * instructions can read from shared regs, in cases where they can
    * ir3_cp will clean up the extra mov:
    */
   for (unsigned i = 0; i < ctx->last_dst_n; i++) {
      if (!ctx->last_dst[i])
         continue;
      if (ctx->last_dst[i]->dsts[0]->flags & IR3_REG_SHARED) {
         ctx->last_dst[i] = ir3_MOV(ctx->block, ctx->last_dst[i], TYPE_U32);
      }
   }

   if (bit_size <= 16) {
      for (unsigned i = 0; i < ctx->last_dst_n; i++) {
         struct ir3_instruction *dst = ctx->last_dst[i];
         ir3_set_dst_type(dst, true);
         ir3_fixup_src_type(dst);
         if (dst->opc == OPC_META_SPLIT) {
            ir3_set_dst_type(ssa(dst->srcs[0]), true);
            ir3_fixup_src_type(ssa(dst->srcs[0]));
            dst->srcs[0]->flags |= IR3_REG_HALF;
         }
      }
   }

   ctx->last_dst = NULL;
   ctx->last_dst_n = 0;
}

static unsigned
dest_flags(struct ir3_instruction *instr)
{
   return instr->dsts[0]->flags & (IR3_REG_HALF | IR3_REG_SHARED);
}

struct ir3_instruction *
ir3_create_collect(struct ir3_block *block, struct ir3_instruction *const *arr,
                   unsigned arrsz)
{
   struct ir3_instruction *collect;

   if (arrsz == 0)
      return NULL;

   unsigned flags = dest_flags(arr[0]);

   collect = ir3_instr_create(block, OPC_META_COLLECT, 1, arrsz);
   __ssa_dst(collect)->flags |= flags;
   for (unsigned i = 0; i < arrsz; i++) {
      struct ir3_instruction *elem = arr[i];

      /* Since arrays are pre-colored in RA, we can't assume that
       * things will end up in the right place.  (Ie. if a collect
       * joins elements from two different arrays.)  So insert an
       * extra mov.
       *
       * We could possibly skip this if all the collected elements
       * are contiguous elements in a single array.. not sure how
       * likely that is to happen.
       *
       * Fixes a problem with glamor shaders, that in effect do
       * something like:
       *
       *   if (foo)
       *     texcoord = ..
       *   else
       *     texcoord = ..
       *   color = texture2D(tex, texcoord);
       *
       * In this case, texcoord will end up as nir registers (which
       * translate to ir3 array's of length 1.  And we can't assume
       * the two (or more) arrays will get allocated in consecutive
       * scalar registers.
       *
       */
      if (elem->dsts[0]->flags & IR3_REG_ARRAY) {
         type_t type = (flags & IR3_REG_HALF) ? TYPE_U16 : TYPE_U32;
         elem = ir3_MOV(block, elem, type);
      }

      assert(dest_flags(elem) == flags);
      __ssa_src(collect, elem, flags);
   }

   collect->dsts[0]->wrmask = MASK(arrsz);

   return collect;
}

/* helper for instructions that produce multiple consecutive scalar
 * outputs which need to have a split meta instruction inserted
 */
void
ir3_split_dest(struct ir3_block *block, struct ir3_instruction **dst,
               struct ir3_instruction *src, unsigned base, unsigned n)
{
   if ((n == 1) && (src->dsts[0]->wrmask == 0x1) &&
       /* setup_input needs ir3_split_dest to generate a SPLIT instruction */
       src->opc != OPC_META_INPUT) {
      dst[0] = src;
      return;
   }

   if (src->opc == OPC_META_COLLECT) {
      assert((base + n) <= src->srcs_count);

      for (int i = 0; i < n; i++) {
         dst[i] = ssa(src->srcs[i + base]);
      }

      return;
   }

   unsigned flags = dest_flags(src);

   for (int i = 0, j = 0; i < n; i++) {
      struct ir3_instruction *split =
         ir3_instr_create(block, OPC_META_SPLIT, 1, 1);
      __ssa_dst(split)->flags |= flags;
      __ssa_src(split, src, flags);
      split->split.off = i + base;

      if (src->dsts[0]->wrmask & (1 << (i + base)))
         dst[j++] = split;
   }
}

NORETURN void
ir3_context_error(struct ir3_context *ctx, const char *format, ...)
{
   struct hash_table *errors = NULL;
   va_list ap;
   va_start(ap, format);
   if (ctx->cur_instr) {
      errors = _mesa_hash_table_create(NULL, _mesa_hash_pointer,
                                       _mesa_key_pointer_equal);
      char *msg = ralloc_vasprintf(errors, format, ap);
      _mesa_hash_table_insert(errors, ctx->cur_instr, msg);
   } else {
      mesa_loge_v(format, ap);
   }
   va_end(ap);
   nir_log_shader_annotated(ctx->s, errors);
   ralloc_free(errors);
   ctx->error = true;
   unreachable("");
}

static struct ir3_instruction *
create_addr0(struct ir3_block *block, struct ir3_instruction *src, int align)
{
   struct ir3_instruction *instr, *immed;

   instr = ir3_COV(block, src, TYPE_U32, TYPE_S16);

   switch (align) {
   case 1:
      /* src *= 1: */
      break;
   case 2:
      /* src *= 2	=> src <<= 1: */
      immed = create_immed_typed(block, 1, TYPE_S16);
      instr = ir3_SHL_B(block, instr, 0, immed, 0);
      break;
   case 3:
      /* src *= 3: */
      immed = create_immed_typed(block, 3, TYPE_S16);
      instr = ir3_MULL_U(block, instr, 0, immed, 0);
      break;
   case 4:
      /* src *= 4 => src <<= 2: */
      immed = create_immed_typed(block, 2, TYPE_S16);
      instr = ir3_SHL_B(block, instr, 0, immed, 0);
      break;
   default:
      unreachable("bad align");
      return NULL;
   }

   instr->dsts[0]->flags |= IR3_REG_HALF;

   instr = ir3_MOV(block, instr, TYPE_S16);
   instr->dsts[0]->num = regid(REG_A0, 0);

   return instr;
}

static struct ir3_instruction *
create_addr1(struct ir3_block *block, unsigned const_val)
{
   struct ir3_instruction *immed =
      create_immed_typed(block, const_val, TYPE_U16);
   struct ir3_instruction *instr = ir3_MOV(block, immed, TYPE_U16);
   instr->dsts[0]->num = regid(REG_A0, 1);
   return instr;
}

/* caches addr values to avoid generating multiple cov/shl/mova
 * sequences for each use of a given NIR level src as address
 */
struct ir3_instruction *
ir3_get_addr0(struct ir3_context *ctx, struct ir3_instruction *src, int align)
{
   struct ir3_instruction *addr;
   unsigned idx = align - 1;

   compile_assert(ctx, idx < ARRAY_SIZE(ctx->addr0_ht));

   if (!ctx->addr0_ht[idx]) {
      ctx->addr0_ht[idx] = _mesa_hash_table_create(ctx, _mesa_hash_pointer,
                                                   _mesa_key_pointer_equal);
   } else {
      struct hash_entry *entry;
      entry = _mesa_hash_table_search(ctx->addr0_ht[idx], src);
      if (entry)
         return entry->data;
   }

   addr = create_addr0(ctx->block, src, align);
   _mesa_hash_table_insert(ctx->addr0_ht[idx], src, addr);

   return addr;
}

/* Similar to ir3_get_addr0, but for a1.x. */
struct ir3_instruction *
ir3_get_addr1(struct ir3_context *ctx, unsigned const_val)
{
   struct ir3_instruction *addr;

   if (!ctx->addr1_ht) {
      ctx->addr1_ht = _mesa_hash_table_u64_create(ctx);
   } else {
      addr = _mesa_hash_table_u64_search(ctx->addr1_ht, const_val);
      if (addr)
         return addr;
   }

   addr = create_addr1(ctx->block, const_val);
   _mesa_hash_table_u64_insert(ctx->addr1_ht, const_val, addr);

   return addr;
}

struct ir3_instruction *
ir3_get_predicate(struct ir3_context *ctx, struct ir3_instruction *src)
{
   struct ir3_block *b = ctx->block;
   struct ir3_instruction *cond;

   /* NOTE: only cmps.*.* can write p0.x: */
   struct ir3_instruction *zero =
         create_immed_typed(b, 0, is_half(src) ? TYPE_U16 : TYPE_U32);
   cond = ir3_CMPS_S(b, src, 0, zero, 0);
   cond->cat2.condition = IR3_COND_NE;

   /* condition always goes in predicate register: */
   cond->dsts[0]->num = regid(REG_P0, 0);
   cond->dsts[0]->flags &= ~IR3_REG_SSA;

   return cond;
}

/*
 * Array helpers
 */

void
ir3_declare_array(struct ir3_context *ctx, nir_intrinsic_instr *decl)
{
   struct ir3_array *arr = rzalloc(ctx, struct ir3_array);
   arr->id = ++ctx->num_arrays;
   /* NOTE: sometimes we get non array regs, for example for arrays of
    * length 1.  See fs-const-array-of-struct-of-array.shader_test.  So
    * treat a non-array as if it was an array of length 1.
    *
    * It would be nice if there was a nir pass to convert arrays of
    * length 1 to ssa.
    */
   arr->length = nir_intrinsic_num_components(decl) *
                 MAX2(1, nir_intrinsic_num_array_elems(decl));

   compile_assert(ctx, arr->length > 0);
   arr->r = &decl->def;
   arr->half = ir3_bitsize(ctx, nir_intrinsic_bit_size(decl)) <= 16;
   list_addtail(&arr->node, &ctx->ir->array_list);
}

struct ir3_array *
ir3_get_array(struct ir3_context *ctx, nir_def *reg)
{
   foreach_array (arr, &ctx->ir->array_list) {
      if (arr->r == reg)
         return arr;
   }
   ir3_context_error(ctx, "bogus reg: r%d\n", reg->index);
   return NULL;
}

/* relative (indirect) if address!=NULL */
struct ir3_instruction *
ir3_create_array_load(struct ir3_context *ctx, struct ir3_array *arr, int n,
                      struct ir3_instruction *address)
{
   struct ir3_block *block = ctx->block;
   struct ir3_instruction *mov;
   struct ir3_register *src;
   unsigned flags = 0;

   mov = ir3_instr_create(block, OPC_MOV, 1, 1);
   if (arr->half) {
      mov->cat1.src_type = TYPE_U16;
      mov->cat1.dst_type = TYPE_U16;
      flags |= IR3_REG_HALF;
   } else {
      mov->cat1.src_type = TYPE_U32;
      mov->cat1.dst_type = TYPE_U32;
   }

   mov->barrier_class = IR3_BARRIER_ARRAY_R;
   mov->barrier_conflict = IR3_BARRIER_ARRAY_W;
   __ssa_dst(mov)->flags |= flags;
   src = ir3_src_create(mov, 0,
                        IR3_REG_ARRAY | COND(address, IR3_REG_RELATIV) | flags);
   src->def = (arr->last_write && arr->last_write->instr->block == block)
                 ? arr->last_write
                 : NULL;
   src->size = arr->length;
   src->array.id = arr->id;
   src->array.offset = n;
   src->array.base = INVALID_REG;

   if (address)
      ir3_instr_set_address(mov, address);

   return mov;
}

/* relative (indirect) if address!=NULL */
void
ir3_create_array_store(struct ir3_context *ctx, struct ir3_array *arr, int n,
                       struct ir3_instruction *src,
                       struct ir3_instruction *address)
{
   struct ir3_block *block = ctx->block;
   struct ir3_instruction *mov;
   struct ir3_register *dst;
   unsigned flags = 0;

   mov = ir3_instr_create(block, OPC_MOV, 1, 1);
   if (arr->half) {
      mov->cat1.src_type = TYPE_U16;
      mov->cat1.dst_type = TYPE_U16;
      flags |= IR3_REG_HALF;
   } else {
      mov->cat1.src_type = TYPE_U32;
      mov->cat1.dst_type = TYPE_U32;
   }
   mov->barrier_class = IR3_BARRIER_ARRAY_W;
   mov->barrier_conflict = IR3_BARRIER_ARRAY_R | IR3_BARRIER_ARRAY_W;
   dst = ir3_dst_create(
      mov, 0,
      IR3_REG_SSA | IR3_REG_ARRAY | flags | COND(address, IR3_REG_RELATIV));
   dst->instr = mov;
   dst->size = arr->length;
   dst->array.id = arr->id;
   dst->array.offset = n;
   dst->array.base = INVALID_REG;
   ir3_src_create(mov, 0, IR3_REG_SSA | flags)->def = src->dsts[0];

   if (arr->last_write && arr->last_write->instr->block == block)
      ir3_reg_set_last_array(mov, dst, arr->last_write);

   if (address)
      ir3_instr_set_address(mov, address);

   arr->last_write = dst;

   /* the array store may only matter to something in an earlier
    * block (ie. loops), but since arrays are not in SSA, depth
    * pass won't know this.. so keep all array stores:
    */
   array_insert(block, block->keeps, mov);
}
