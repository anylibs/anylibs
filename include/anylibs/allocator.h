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
 */

#ifndef ANYLIBS_ALLOCATOR_H
#define ANYLIBS_ALLOCATOR_H

#include "anylibs/error.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct CAllocator CAllocator;

typedef struct CMemory {
  void*  data;      ///< the allocated data
  size_t size;      ///< the size of @ref CMemory::data
  size_t alignment; ///< the alignment of CMemory::data
} CMemory;

/// Default Allocator
c_error_t c_allocator_default(CAllocator** out_allocator);

/// Arena Allocator
c_error_t c_allocator_arena_create(size_t capacity, CAllocator** out_allocator);
void      c_allocator_arena_destroy(CAllocator** self);

/// Fixed buffer Allocator
c_error_t c_allocator_fixed_buffer_create(void*        buffer,
                                          size_t       buffer_size,
                                          CAllocator** out_allocator);
void      c_allocator_fixed_buffer_destroy(CAllocator** self);

/// Allocators generic functions
/// @brief this macro is used to calculate size and alignment parameters
///        of @ref c_allocator_alloc (check unit tests for an example)
#define c_allocator_alignas(type__, count__)                                   \
  sizeof(type__) * (count__), alignof(type__)
c_error_t c_allocator_alloc(CAllocator* self,
                            size_t      size,
                            size_t      alignment,
                            bool        zero_initialized,
                            CMemory*    out_memory);
c_error_t
     c_allocator_resize(CAllocator* self, CMemory* memory, size_t new_size);
void c_allocator_free(CAllocator* self, CMemory* memory);

#endif // ANYLIBS_ALLOCATOR_H
