/*
 * Copyright © 2012 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "main/core.h"
#include "ir.h"
#include "linker.h"
#include "ir_uniform.h"
#include "link_uniform_block_active_visitor.h"
#include "util/hash_table.h"
#include "program.h"

namespace {

class ubo_visitor : public program_resource_visitor {
public:
   ubo_visitor(void *mem_ctx, gl_uniform_buffer_variable *variables,
               unsigned num_variables)
      : index(0), offset(0), buffer_size(0), variables(variables),
        num_variables(num_variables), mem_ctx(mem_ctx), is_array_instance(false)
   {
      /* empty */
   }

   void process(const glsl_type *type, const char *name)
   {
      this->offset = 0;
      this->buffer_size = 0;
      this->is_array_instance = strchr(name, ']') != NULL;
      this->program_resource_visitor::process(type, name);
   }

   unsigned index;
   unsigned offset;
   unsigned buffer_size;
   gl_uniform_buffer_variable *variables;
   unsigned num_variables;
   void *mem_ctx;
   bool is_array_instance;

private:
   virtual void visit_field(const glsl_type *type, const char *name,
                            bool row_major)
   {
      (void) type;
      (void) name;
      (void) row_major;
      assert(!"Should not get here.");
   }

   virtual void enter_record(const glsl_type *type, const char *,
                             bool row_major, const unsigned packing) {
      assert(type->is_record());
      if (packing == GLSL_INTERFACE_PACKING_STD430)
         this->offset = glsl_align(
            this->offset, type->std430_base_alignment(row_major));
      else
         this->offset = glsl_align(
            this->offset, type->std140_base_alignment(row_major));
   }

   virtual void leave_record(const glsl_type *type, const char *,
                             bool row_major, const unsigned packing) {
      assert(type->is_record());

      /* If this is the last field of a structure, apply rule #9.  The
       * GL_ARB_uniform_buffer_object spec says:
       *
       *     "The structure may have padding at the end; the base offset of
       *     the member following the sub-structure is rounded up to the next
       *     multiple of the base alignment of the structure."
       */
      if (packing == GLSL_INTERFACE_PACKING_STD430)
         this->offset = glsl_align(
            this->offset, type->std430_base_alignment(row_major));
      else
         this->offset = glsl_align(
            this->offset, type->std140_base_alignment(row_major));
   }

   virtual void visit_field(const glsl_type *type, const char *name,
                            bool row_major, const glsl_type *,
                            const unsigned packing,
                            bool last_field)
   {
      assert(this->index < this->num_variables);

      gl_uniform_buffer_variable *v = &this->variables[this->index++];

      v->Name = ralloc_strdup(mem_ctx, name);
      v->Type = type;
      v->RowMajor = type->without_array()->is_matrix() && row_major;

      if (this->is_array_instance) {
         v->IndexName = ralloc_strdup(mem_ctx, name);

         char *open_bracket = strchr(v->IndexName, '[');
         assert(open_bracket != NULL);

         char *close_bracket = strchr(open_bracket, '.') - 1;
         assert(close_bracket != NULL);

         /* Length of the tail without the ']' but with the NUL.
          */
         unsigned len = strlen(close_bracket + 1) + 1;

         memmove(open_bracket, close_bracket + 1, len);
      } else {
         v->IndexName = v->Name;
      }

      unsigned alignment = 0;
      unsigned size = 0;

      /* From ARB_program_interface_query:
       *
       *     "If the final member of an active shader storage block is array
       *      with no declared size, the minimum buffer size is computed
       *      assuming the array was declared as an array with one element."
       *
       * For that reason, we use the base type of the unsized array to calculate
       * its size. We don't need to check if the unsized array is the last member
       * of a shader storage block (that check was already done by the parser).
       */
      const glsl_type *type_for_size = type;
      if (type->is_unsized_array()) {
         assert(last_field);
         type_for_size = type->without_array();
      }

      if (packing == GLSL_INTERFACE_PACKING_STD430) {
         alignment = type->std430_base_alignment(v->RowMajor);
         size = type_for_size->std430_size(v->RowMajor);
      } else {
         alignment = type->std140_base_alignment(v->RowMajor);
         size = type_for_size->std140_size(v->RowMajor);
      }

      this->offset = glsl_align(this->offset, alignment);
      v->Offset = this->offset;

      this->offset += size;

      /* From the GL_ARB_uniform_buffer_object spec:
       *
       *     "For uniform blocks laid out according to [std140] rules, the
       *      minimum buffer object size returned by the
       *      UNIFORM_BLOCK_DATA_SIZE query is derived by taking the offset of
       *      the last basic machine unit consumed by the last uniform of the
       *      uniform block (including any end-of-array or end-of-structure
       *      padding), adding one, and rounding up to the next multiple of
       *      the base alignment required for a vec4."
       */
      this->buffer_size = glsl_align(this->offset, 16);
   }
};

class count_block_size : public program_resource_visitor {
public:
   count_block_size() : num_active_uniforms(0)
   {
      /* empty */
   }

   unsigned num_active_uniforms;

private:
   virtual void visit_field(const glsl_type *type, const char *name,
                            bool row_major)
   {
      (void) type;
      (void) name;
      (void) row_major;
      this->num_active_uniforms++;
   }
};

} /* anonymous namespace */

struct block {
   const glsl_type *type;
   bool has_instance_name;
};

static void
process_block_array(struct uniform_block_array_elements *ub_array, char **name,
                    size_t name_length, gl_uniform_block *blocks,
                    ubo_visitor *parcel, gl_uniform_buffer_variable *variables,
                    const struct link_uniform_block_active *const b,
                    unsigned *block_index, unsigned *binding_offset,
                    struct gl_context *ctx, struct gl_shader_program *prog)
{
   if (ub_array) {
      for (unsigned j = 0; j < ub_array->num_array_elements; j++) {
	 size_t new_length = name_length;

         /* Append the subscript to the current variable name */
         ralloc_asprintf_rewrite_tail(name, &new_length, "[%u]",
                                      ub_array->array_elements[j]);

         process_block_array(ub_array->array, name, new_length, blocks,
                             parcel, variables, b, block_index,
                             binding_offset, ctx, prog);
      }
   } else {
      unsigned i = *block_index;
      const glsl_type *type =  b->type->without_array();

      blocks[i].Name = ralloc_strdup(blocks, *name);
      blocks[i].Uniforms = &variables[(*parcel).index];

      /* The GL_ARB_shading_language_420pack spec says:
       *
       *     "If the binding identifier is used with a uniform block
       *     instanced as an array then the first element of the array
       *     takes the specified block binding and each subsequent
       *     element takes the next consecutive uniform block binding
       *     point."
       */
      blocks[i].Binding = (b->has_binding) ? b->binding + *binding_offset : 0;

      blocks[i].UniformBufferSize = 0;
      blocks[i]._Packing = gl_uniform_block_packing(type->interface_packing);

      parcel->process(type, blocks[i].Name);

      blocks[i].UniformBufferSize = parcel->buffer_size;

      /* Check SSBO size is lower than maximum supported size for SSBO */
      if (b->is_shader_storage &&
          parcel->buffer_size > ctx->Const.MaxShaderStorageBlockSize) {
         linker_error(prog, "shader storage block `%s' has size %d, "
                      "which is larger than than the maximum allowed (%d)",
                      b->type->name,
                      parcel->buffer_size,
                      ctx->Const.MaxShaderStorageBlockSize);
      }
      blocks[i].NumUniforms =
         (unsigned)(ptrdiff_t)(&variables[parcel->index] - blocks[i].Uniforms);
      blocks[i].IsShaderStorage = b->is_shader_storage;

      *block_index = *block_index + 1;
      *binding_offset = *binding_offset + 1;
   }
}

/* This function resizes the array types of the block so that later we can use
 * this new size to correctly calculate the offest for indirect indexing.
 */
const glsl_type *
resize_block_array(const glsl_type *type,
                   struct uniform_block_array_elements *ub_array)
{
   if (type->is_array()) {
      struct uniform_block_array_elements *child_array =
         type->fields.array->is_array() ? ub_array->array : NULL;
      const glsl_type *new_child_type =
         resize_block_array(type->fields.array, child_array);

      const glsl_type *new_type =
         glsl_type::get_array_instance(new_child_type,
                                       ub_array->num_array_elements);
      ub_array->ir->array->type = new_type;
      return new_type;
   } else {
      return type;
   }
}

unsigned
link_uniform_blocks(void *mem_ctx,
                    struct gl_context *ctx,
                    struct gl_shader_program *prog,
                    struct gl_shader **shader_list,
                    unsigned num_shaders,
                    struct gl_uniform_block **blocks_ret)
{
   /* This hash table will track all of the uniform blocks that have been
    * encountered.  Since blocks with the same block-name must be the same,
    * the hash is organized by block-name.
    */
   struct hash_table *block_hash =
      _mesa_hash_table_create(mem_ctx, _mesa_key_hash_string,
                              _mesa_key_string_equal);

   if (block_hash == NULL) {
      _mesa_error_no_memory(__func__);
      linker_error(prog, "out of memory\n");
      return 0;
   }

   /* Determine which uniform blocks are active.
    */
   link_uniform_block_active_visitor v(mem_ctx, block_hash, prog);
   for (unsigned i = 0; i < num_shaders; i++) {
      visit_list_elements(&v, shader_list[i]->ir);
   }

   /* Count the number of active uniform blocks.  Count the total number of
    * active slots in those uniform blocks.
    */
   unsigned num_blocks = 0;
   unsigned num_variables = 0;
   count_block_size block_size;
   struct hash_entry *entry;

   hash_table_foreach (block_hash, entry) {
      struct link_uniform_block_active *const b =
         (struct link_uniform_block_active *) entry->data;

      assert((b->array != NULL) == b->type->is_array());

      if (b->array != NULL &&
          (b->type->without_array()->interface_packing ==
           GLSL_INTERFACE_PACKING_PACKED)) {
         b->type = resize_block_array(b->type, b->array);
         b->var->type = b->type;
      }

      block_size.num_active_uniforms = 0;
      block_size.process(b->type->without_array(), "");

      if (b->array != NULL) {
         unsigned aoa_size = b->type->arrays_of_arrays_size();
         num_blocks += aoa_size;
         num_variables += aoa_size * block_size.num_active_uniforms;
      } else {
         num_blocks++;
         num_variables += block_size.num_active_uniforms;
      }

   }

   if (num_blocks == 0) {
      assert(num_variables == 0);
      _mesa_hash_table_destroy(block_hash, NULL);
      return 0;
   }

   assert(num_variables != 0);

   /* Allocate storage to hold all of the informatation related to uniform
    * blocks that can be queried through the API.
    */
   gl_uniform_block *blocks =
      ralloc_array(mem_ctx, gl_uniform_block, num_blocks);
   gl_uniform_buffer_variable *variables =
      ralloc_array(blocks, gl_uniform_buffer_variable, num_variables);

   /* Add each variable from each uniform block to the API tracking
    * structures.
    */
   unsigned i = 0;
   ubo_visitor parcel(blocks, variables, num_variables);

   STATIC_ASSERT(unsigned(GLSL_INTERFACE_PACKING_STD140)
                 == unsigned(ubo_packing_std140));
   STATIC_ASSERT(unsigned(GLSL_INTERFACE_PACKING_SHARED)
                 == unsigned(ubo_packing_shared));
   STATIC_ASSERT(unsigned(GLSL_INTERFACE_PACKING_PACKED)
                 == unsigned(ubo_packing_packed));
   STATIC_ASSERT(unsigned(GLSL_INTERFACE_PACKING_STD430)
                 == unsigned(ubo_packing_std430));

   hash_table_foreach (block_hash, entry) {
      const struct link_uniform_block_active *const b =
         (const struct link_uniform_block_active *) entry->data;
      const glsl_type *block_type = b->type;

      if (b->array != NULL) {
         unsigned binding_offset = 0;
         char *name = ralloc_strdup(NULL, block_type->without_array()->name);
         size_t name_length = strlen(name);

         assert(b->has_instance_name);
         process_block_array(b->array, &name, name_length, blocks, &parcel,
                             variables, b, &i, &binding_offset, ctx, prog);
         ralloc_free(name);
      } else {
         blocks[i].Name = ralloc_strdup(blocks, block_type->name);
         blocks[i].Uniforms = &variables[parcel.index];
         blocks[i].Binding = (b->has_binding) ? b->binding : 0;
         blocks[i].UniformBufferSize = 0;
         blocks[i]._Packing =
            gl_uniform_block_packing(block_type->interface_packing);

         parcel.process(block_type,
                        b->has_instance_name ? block_type->name : "");

         blocks[i].UniformBufferSize = parcel.buffer_size;

         /* Check SSBO size is lower than maximum supported size for SSBO */
         if (b->is_shader_storage &&
             parcel.buffer_size > ctx->Const.MaxShaderStorageBlockSize) {
            linker_error(prog, "shader storage block `%s' has size %d, "
                         "which is larger than than the maximum allowed (%d)",
                         block_type->name,
                         parcel.buffer_size,
                         ctx->Const.MaxShaderStorageBlockSize);
         }
         blocks[i].NumUniforms =
            (unsigned)(ptrdiff_t)(&variables[parcel.index] - blocks[i].Uniforms);

         blocks[i].IsShaderStorage = b->is_shader_storage;

         i++;
      }
   }

   assert(parcel.index == num_variables);

   _mesa_hash_table_destroy(block_hash, NULL);

   *blocks_ret = blocks;
   return num_blocks;
}

bool
link_uniform_blocks_are_compatible(const gl_uniform_block *a,
				   const gl_uniform_block *b)
{
   assert(strcmp(a->Name, b->Name) == 0);

   /* Page 35 (page 42 of the PDF) in section 4.3.7 of the GLSL 1.50 spec says:
    *
    *     "Matched block names within an interface (as defined above) must
    *     match in terms of having the same number of declarations with the
    *     same sequence of types and the same sequence of member names, as
    *     well as having the same member-wise layout qualification....if a
    *     matching block is declared as an array, then the array sizes must
    *     also match... Any mismatch will generate a link error."
    *
    * Arrays are not yet supported, so there is no check for that.
    */
   if (a->NumUniforms != b->NumUniforms)
      return false;

   if (a->_Packing != b->_Packing)
      return false;

   for (unsigned i = 0; i < a->NumUniforms; i++) {
      if (strcmp(a->Uniforms[i].Name, b->Uniforms[i].Name) != 0)
	 return false;

      if (a->Uniforms[i].Type != b->Uniforms[i].Type)
	 return false;

      if (a->Uniforms[i].RowMajor != b->Uniforms[i].RowMajor)
	 return false;
   }

   return true;
}
