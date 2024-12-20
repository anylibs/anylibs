/**
 * @file allocator.h
 * @author Mohamed A. Elmeligy
 * @date 2024-2025
 * @copyright MIT License
 *
 * Permission is hereby granted, free of charge, to use, copy, modify, and
 * distribute this software, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO MERCHANTABILITY OR FITNESS FOR
 * A PARTICULAR PURPOSE. See the License for details.
 *
 * @brief this module will be used to deal with heap management
 *        it currently has DefaultAllocator (wrapper around malloc/free),
 *        ArenaAllocator, and FixBufferAllocator
 */

#ifndef ANYLIBS_ALLOCATOR_H
#define ANYLIBS_ALLOCATOR_H

#include "def.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct CAllocator CAllocator;

/// Default Allocator
CAllocator* c_allocator_default(void);

/// Arena Allocator
CAllocator* c_allocator_arena_create(size_t capacity);
void        c_allocator_arena_destroy(CAllocator* self);

/// Fixed buffer Allocator
CAllocator* c_allocator_fixed_buffer_create(void* buffer, size_t buffer_size);
void        c_allocator_fixed_buffer_destroy(CAllocator* self);

///-------------------------------
/// Allocators generic functions
///-------------------------------

/// @brief this macro is used to calculate size and alignment parameters
///        of @ref c_allocator_alloc (check unit tests for an example)
#define c_allocator_alignas(type__, count__)                                   \
  sizeof(type__) * (count__), alignof(type__)

void* c_allocator_alloc(CAllocator* self,
                        size_t      size,
                        size_t      alignment,
                        bool        zero_initialized);
CResultVoidPtr
     c_allocator_resize(CAllocator* self, void* memory, size_t new_size);
void c_allocator_free(CAllocator* self, void* memory);

size_t c_allocator_mem_size(void* memory);
size_t c_allocator_mem_alignment(void* memory);

#endif // ANYLIBS_ALLOCATOR_H
