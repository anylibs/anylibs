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

#include "anylibs/allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <stdalign.h>
#endif

typedef struct CAllocator CAllocator;
typedef struct CAllocatorVTable {
  void* (*alloc)(CAllocator* self, size_t size, size_t align);
  void* (*resize)(CAllocator* self,
                  void*       mem,
                  size_t      old_size,
                  size_t      new_size,
                  size_t      align);
  void (*free)(CAllocator* self, void* mem);
} CAllocatorVTable;

typedef struct CAllocator {
  struct {
    void*  buf;
    size_t current_size;
    size_t capacity;
  } main_mem;
  CAllocatorVTable vtable;
} CAllocator;

#ifdef _WIN32
#define c_mem_alloc(size, align) _aligned_malloc((size), (align))
#define c_mem_resize(mem, old_size, new_size, align)                           \
  _aligned_realloc(mem, new_size, align)
#define c_mem_free(mem)
#else
#define c_mem_alloc(size, align) aligned_alloc((align), (size))
#define c_mem_resize(mem, old_size, new_size, align)                           \
  c_internal_allocator_default_posix_resize((mem), (old_size), (new_size),     \
                                            (align))
#define c_mem_free(mem) free(mem)
#endif

#ifndef _WIN32
static void* c_internal_allocator_default_posix_resize(void*  mem,
                                                       size_t old_size,
                                                       size_t new_size,
                                                       size_t align);
#endif

#define C_INTERNAL_ALLOCATOR_DEFINE(name)                                      \
  static void* c_internal_allocator_##name##_alloc(CAllocator* self,           \
                                                   size_t size, size_t align); \
  static void* c_internal_allocator_##name##_resize(                           \
      CAllocator* self, void* mem, size_t old_size, size_t new_size,           \
      size_t align);                                                           \
  static void c_internal_allocator_##name##_free(CAllocator* self, void* mem);

C_INTERNAL_ALLOCATOR_DEFINE(default)
C_INTERNAL_ALLOCATOR_DEFINE(arena)
C_INTERNAL_ALLOCATOR_DEFINE(fixed_buffer)

/// default allocator
static CAllocator c_allocator_default__ = {
    .main_mem = {0},
    .vtable   = (CAllocatorVTable){.alloc  = c_internal_allocator_default_alloc,
                                   .resize = c_internal_allocator_default_resize,
                                   .free   = c_internal_allocator_default_free}};

/// Allocators
/// @brief get the default allocator (it will use (malloc, free, ...) family)
/// @note this will never return error, it made like that only for consistency
/// @param[out] out_allocator
/// @return error (any value but zero is treated as an error)
c_error_t
c_allocator_default(CAllocator** out_allocator)
{
  if (out_allocator) *out_allocator = &c_allocator_default__;
  return C_ERROR_none;
}

/// Arena

/// @brief create arena allocator (this is a fixed big memory allocation that
///        you can use @ref CAllocator::main_mem::buf for smaller allocations
///        and at the end free the whole arena all at once)
/// @param[in] capacity
/// @param[out] out_allocator
/// @return error (any value but zero is treated as an error)
c_error_t
c_allocator_arena_create(size_t capacity, CAllocator** out_allocator)
{
  if (!out_allocator) return C_ERROR_none;

  *out_allocator = malloc(sizeof(CAllocator));
  if (!*out_allocator) return C_ERROR_mem_allocation;

  (*out_allocator)->main_mem.buf = malloc(capacity);
  if (!(*out_allocator)->main_mem.buf) return C_ERROR_mem_allocation;
  (*out_allocator)->main_mem.capacity     = capacity;
  (*out_allocator)->main_mem.current_size = 0;
  (*out_allocator)->vtable
      = (CAllocatorVTable){.alloc  = c_internal_allocator_arena_alloc,
                           .resize = c_internal_allocator_arena_resize,
                           .free   = c_internal_allocator_arena_free};

  return C_ERROR_none;
}

/// @brief
/// @note this could handle self as NULL
/// @param[in] self
void
c_allocator_arena_destroy(CAllocator** self)
{
  if (self && *self) {
    free((*self)->main_mem.buf);
    **self = (CAllocator){0};
    free(*self);
    *self = NULL;
  }
}

/// Fixed buffer

/// @brief craete fixed buffer allocator (it is the same like arena, but you
///        will provide the memory to be used instead of allocating one)
/// @param[in] buffer this will be used as a big memory like the arena
/// @param[in] buffer_size the memory capacity
/// @param[out] out_allocator
/// @return error (any value but zero is treated as an error)
c_error_t
c_allocator_fixed_buffer_create(void*        buffer,
                                size_t       buffer_size,
                                CAllocator** out_allocator)
{
  if (!out_allocator) return C_ERROR_none;

  *out_allocator = malloc(sizeof(CAllocator));
  if (!*out_allocator) return C_ERROR_mem_allocation;

  (*out_allocator)->main_mem.buf          = buffer;
  (*out_allocator)->main_mem.capacity     = buffer_size;
  (*out_allocator)->main_mem.current_size = 0;
  (*out_allocator)->vtable
      = (CAllocatorVTable){.alloc  = c_internal_allocator_fixed_buffer_alloc,
                           .resize = c_internal_allocator_fixed_buffer_resize,
                           .free   = c_internal_allocator_fixed_buffer_free};

  return C_ERROR_none;
}

/// @brief
/// @note this could handle self as NULL
/// @param[in] self
void
c_allocator_fixed_buffer_destroy(CAllocator** self)
{
  if (self && *self) {
    **self = (CAllocator){0};
    free(*self);
    *self = NULL;
  }
}

/// Allocators generic functions
/// @brief this is like malloc (but is has many other features)
/// @param[in] self
/// @param[in] size required size to be allocated
/// @param[in] alignment check
///                      https://en.cppreference.com/w/c/memory/aligned_alloc
/// @param[in] zero_initialized
/// @param[out] out_memory
/// @return error (any value but zero is treated as an error)
c_error_t
c_allocator_alloc(CAllocator* self,
                  size_t      size,
                  size_t      alignment,
                  bool        zero_initialized,
                  CMemory*    out_memory)
{
  assert(self);

  if ((size == 0) || (alignment == 0)) return C_ERROR_none;
  if (!out_memory) return C_ERROR_none;
  if (size % alignment != 0) return C_ERROR_wrong_alignment;

  void* new_mem = self->vtable.alloc(self, size, alignment);
  if (!new_mem) return C_ERROR_mem_allocation;

  if (zero_initialized) memset(new_mem, 0, size);
  out_memory->data      = new_mem;
  out_memory->size      = size;
  out_memory->alignment = alignment;

  return C_ERROR_none;
}

/// @brief this is like realloc
/// @note Default Allocator will ALWAYS create new memory on resize to keep the
///       alignment
/// @param[in] self
/// @param[in,out] memory the input memory that need to be resized, also the
///                       resulted new memory will be returned here
/// @param[in] new_size
/// @return error (any value but zero is treated as an error)
c_error_t
c_allocator_resize(CAllocator* self, CMemory* memory, size_t new_size)
{
  assert(self);
  if (new_size == 0) return C_ERROR_none;
  if (!memory) return C_ERROR_none;

  void* new_mem = self->vtable.resize(self, memory->data, memory->size,
                                      new_size, memory->alignment);
  if (!new_mem) return C_ERROR_mem_allocation;

  memory->data = new_mem;
  memory->size = new_size;

  return C_ERROR_none;
}

/// @brief free an allocated memory
/// @note this could handle @p memory as NULL
/// @param[in] self
/// @param[in] memory
void
c_allocator_free(CAllocator* self, CMemory* memory)
{
  assert(self);

  if (!memory) return;

  self->vtable.free(self, memory->data);
  *memory = (CMemory){0};
}

/******************************************************************************/
/********************************** Internal **********************************/
/******************************************************************************/

#ifndef _WIN32
void*
c_internal_allocator_default_posix_resize(void*  mem,
                                          size_t old_size,
                                          size_t new_size,
                                          size_t align)
{
  if (new_size <= old_size) return mem;

  void* new_mem = c_mem_alloc(new_size, align);
  if (new_mem) {
    memcpy(new_mem, mem, old_size);
    c_mem_free(mem);
  }

  return new_mem;
}
#endif

void*
c_internal_allocator_default_alloc(CAllocator* self, size_t size, size_t align)
{
  (void)self;
  return c_mem_alloc(size, align);
}

void*
c_internal_allocator_default_resize(
    CAllocator* self, void* mem, size_t old_size, size_t new_size, size_t align)
{
  (void)self;
  return c_mem_resize(mem, old_size, new_size, align);
}

void
c_internal_allocator_default_free(CAllocator* self, void* mem)
{
  (void)self;
  c_mem_free(mem);
}

void*
c_internal_allocator_arena_alloc(CAllocator* self, size_t size, size_t align)
{
  // get aligned address first
  size_t new_data_index = self->main_mem.current_size;
  while (
      (new_data_index < self->main_mem.capacity)
      && ((intptr_t)((char*)self->main_mem.buf + new_data_index) % align != 0))
    new_data_index++;

  if ((self->main_mem.capacity - (new_data_index - size)) < size) return NULL;
  self->main_mem.current_size += size;

  return &((char*)self->main_mem.buf)[self->main_mem.current_size - size];
}

void*
c_internal_allocator_arena_resize(
    CAllocator* self, void* mem, size_t old_size, size_t new_size, size_t align)
{
  // this is the last allocated block
  if ((char*)self->main_mem.buf + self->main_mem.current_size - old_size
      == mem) {
    self->main_mem.current_size
        = (self->main_mem.current_size - old_size) + new_size;
    return mem;
  }

  void* new_mem = c_internal_allocator_arena_alloc(self, new_size, align);
  if (!new_mem) return NULL;
  memcpy(new_mem, mem, old_size);

  return new_mem;
}

void
c_internal_allocator_arena_free(CAllocator* self, void* mem)
{
  (void)self->main_mem;
  (void)mem;
}

void*
c_internal_allocator_fixed_buffer_alloc(CAllocator* self,
                                        size_t      size,
                                        size_t      align)
{
  return c_internal_allocator_arena_alloc(self, size, align);
}

void*
c_internal_allocator_fixed_buffer_resize(
    CAllocator* self, void* mem, size_t old_size, size_t new_size, size_t align)
{
  return c_internal_allocator_arena_resize(self, mem, old_size, new_size,
                                           align);
}

void
c_internal_allocator_fixed_buffer_free(CAllocator* self, void* mem)
{
  c_internal_allocator_arena_free(self, mem);
}
