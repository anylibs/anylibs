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
#include "anylibs/error.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <stdalign.h>
#endif

#ifdef _WIN32
#define c_mem_alloc(size, align) _aligned_malloc((size), (align))
#define c_mem_resize(mem, old_size, new_size, align)                           \
  _aligned_realloc(mem, new_size, align)
#define c_mem_free(mem) _aligned_free(mem)
#else
#define c_mem_alloc(size, align) aligned_alloc((align), (size))
#define c_mem_resize(mem, old_size, new_size, align)                           \
  c_internal_allocator_default_posix_resize((mem), (old_size), (new_size),     \
                                            (align))
#define c_mem_free(mem) free(mem)
#endif

#define TO_CMEMORY(ptr) ((CMemory*)(ptr) - 1)

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

typedef struct CMemory {
  size_t size;      ///< the size of @ref CMemory::data
  size_t alignment; ///< the alignment of CMemory::data
  char   data[];    ///< the allocated data
} CMemory;

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
/// @return allocator that wraps malloc/free
CAllocator*
c_allocator_default(void)
{
  return &c_allocator_default__;
}

///--------------------------------- Arena --------------------------------- ///

/// @brief create arena allocator (this is a fixed big memory allocation that
///        you can use @ref CAllocator::main_mem::buf for smaller allocations
///        and at the end free the whole arena all at once)
/// @param capacity
/// @return arena allocator or NULL on error
CAllocator*
c_allocator_arena_create(size_t capacity)
{
  CAllocator* allocator = malloc(sizeof(CAllocator));
  if (!allocator) {
    c_error_set(C_ERROR_mem_allocation);
    return NULL;
  }

  allocator->main_mem.buf = malloc(capacity);
  if (!allocator->main_mem.buf) {
    free(allocator);
    c_error_set(C_ERROR_mem_allocation);
    return NULL;
  }

  allocator->main_mem.capacity     = capacity;
  allocator->main_mem.current_size = 0;
  allocator->vtable
      = (CAllocatorVTable){.alloc  = c_internal_allocator_arena_alloc,
                           .resize = c_internal_allocator_arena_resize,
                           .free   = c_internal_allocator_arena_free};

  return allocator;
}

/// @brief destroy the memory hold by the arena
/// @note this could handle self as NULL
/// @param self
void
c_allocator_arena_destroy(CAllocator* self)
{
  if (self) {
    free(self->main_mem.buf);
    *self = (CAllocator){0};
    free(self);
  }
}

///----------------------------- Fixed buffer ----------------------------- ///

/// @brief craete fixed buffer allocator (it is the same like arena, but you
///        will provide the memory to be used instead of allocating one)
/// @param buffer this will be used as a big memory like the arena
/// @param buffer_size the memory capacity
/// @return fixed buffer allocator or NULL on error
CAllocator*
c_allocator_fixed_buffer_create(void* buffer, size_t buffer_size)
{
  CAllocator* allocator = malloc(sizeof(CAllocator));
  if (!allocator) return NULL;

  allocator->main_mem.buf          = buffer;
  allocator->main_mem.capacity     = buffer_size;
  allocator->main_mem.current_size = 0;
  allocator->vtable
      = (CAllocatorVTable){.alloc  = c_internal_allocator_fixed_buffer_alloc,
                           .resize = c_internal_allocator_fixed_buffer_resize,
                           .free   = c_internal_allocator_fixed_buffer_free};

  return allocator;
}

/// @brief destroy the fixed buffer allocator
/// @note this could handle self as NULL
/// @param self
void
c_allocator_fixed_buffer_destroy(CAllocator* self)
{
  if (self) {
    *self = (CAllocator){0};
    free(self);
  }
}

/// Allocators generic functions
/// @brief this is like malloc (but is has many other features)
/// @param self
/// @param size required size to be allocated
/// @param alignment check
///                      https://en.cppreference.com/w/c/memory/aligned_alloc
/// @param zero_initialized set the allocated memory to zero
/// @return new allocated memory or NULL on error
void*
c_allocator_alloc(CAllocator* self,
                  size_t      size,
                  size_t      alignment,
                  bool        zero_initialized)
{
  assert(self);

  if (size == 0) {
    c_error_set(C_ERROR_invalid_size);
    return NULL;
  }
  if ((alignment == 0) || (size % alignment != 0)) {
    c_error_set(C_ERROR_invalid_alignment);
    return NULL;
  }

  CMemory* new_memory
      = self->vtable.alloc(self, size + sizeof(*new_memory), alignment);
  if (!new_memory) {
    c_error_set(C_ERROR_mem_allocation);
    return NULL;
  }

  new_memory->size      = size;
  new_memory->alignment = alignment;
  if (zero_initialized) memset(new_memory->data, 0, size);

  return new_memory->data;
}

/// @brief this is like realloc
/// @note Default Allocator will ALWAYS create new memory on resize to keep the
///       alignment
/// @param self
/// @param memory the input memory that need to be resized, also the
///                       resulted new memory will be returned here
/// @param new_size
/// @return is_ok[true]: return new reallocated memory with the new size
///         is_ok[false]: failed to resize, and return the old @p memory
CResultVoidPtr
c_allocator_resize(CAllocator* self, void* memory, size_t new_size)
{
  assert(self);

  CResultVoidPtr result = {.vp = memory};

  if (!memory) return result;
  if (new_size == 0) {
    c_allocator_free(self, memory);
    result.is_ok = true;
    return result;
  }

  CMemory* old_mem = TO_CMEMORY(memory);

  CMemory* new_mem
      = self->vtable.resize(self, old_mem, old_mem->size + sizeof(*old_mem),
                            new_size + sizeof(*old_mem), old_mem->alignment);
  if (!new_mem) {
    c_error_set(C_ERROR_mem_allocation);
    return result;
  }

  new_mem->size = new_size;

  result.vp    = new_mem->data;
  result.is_ok = true;
  return result;
}

/// @brief free an allocated memory
/// @note this could handle @p memory as NULL
/// @param self
/// @param memory
void
c_allocator_free(CAllocator* self, void* memory)
{
  assert(self);

  if (!memory) return;

  self->vtable.free(self, TO_CMEMORY(memory));
}

size_t
c_allocator_mem_size(void* memory)
{
  assert(memory);
  return TO_CMEMORY(memory)->size;
}

size_t
c_allocator_mem_alignment(void* memory)
{
  assert(memory);
  return TO_CMEMORY(memory)->alignment;
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
  void* resized_mem = c_mem_resize(mem, old_size, new_size, align);
  return resized_mem ? resized_mem : mem;
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

  if ((self->main_mem.capacity - (new_data_index - size)) < size) {
    c_error_set(C_ERROR_mem_allocation);
    return NULL;
  }
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
  if (!new_mem) {
    c_error_set(C_ERROR_mem_allocation);
    return NULL;
  }
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
