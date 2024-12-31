#ifndef ANYLIBS_ALLOCATOR_H
#define ANYLIBS_ALLOCATOR_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct CAllocator CAllocator;

//-------------------------------
// Allocators
//-------------------------------

// -- Default Allocator
CAllocator* c_allocator_default(void); ///< this will always return valid allocator based on (malloc, free, ...) family

// -- Arena Allocator
CAllocator* c_allocator_arena_create(size_t capacity); ///< create Arena allocator (this will use malloc to create memory)
void        c_allocator_arena_destroy(CAllocator* self); ///< destroy the memory hold by the arena allocator, and the allocator itself

// -- Fixed buffer Allocator
CAllocator* c_allocator_fixed_buffer_create(void* buffer, size_t buffer_size); ///< craete fixed buffer allocator (it is the same like arena, but you will provide the memory to be used instead of allocating one)
void        c_allocator_fixed_buffer_destroy(CAllocator* self); ///< destroy the fixed buffer allocator

//-------------------------------
// Allocators generic functions
//-------------------------------

#define c_allocator_alignas(type__, count__) sizeof(type__) * (count__), alignof(type__) ///< this macro is used to calculate size and alignment parameters of @ref c_allocator_alloc (check unit tests for an example)

void*  c_allocator_alloc(CAllocator* self, size_t size, size_t alignment, bool set_mem_to_zero); ///< this is similar to malloc, for alignment, check https://en.cppreference.com/w/c/memory/aligned_alloc
void*  c_allocator_resize(CAllocator* self, void* memory, size_t new_size); ///< this is similar to realloc, note: Default Allocator will ALWAYS create new memory on resize to keep the alignment, it will return NULL on error
void   c_allocator_free(CAllocator* self, void* memory); ///< this is similar to free
size_t c_allocator_mem_size(void* memory); ///< get memory size
size_t c_allocator_mem_alignment(void* memory); ///< get alignment

#endif // ANYLIBS_ALLOCATOR_H
