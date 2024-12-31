#include "anylibs/allocator.h"
#include "anylibs/error.h"

#include <utest.h>

#include <stdalign.h>

UTEST(CAllocator, default_general)
{
  CAllocator* a = c_allocator_default();

  void* mem = c_allocator_alloc(a, c_allocator_alignas(int, 10), true);
  EXPECT_TRUE_MSG(mem, c_error_to_str(c_error_get()));

  EXPECT_EQ(c_allocator_mem_alignment(mem), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem), sizeof(int) * 10);
  EXPECT_EQ(memcmp(mem, (int[]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, sizeof(int) * 10), 0);

  ((int*)mem)[5] = 10;
  EXPECT_EQ(((int*)mem)[5], 10);

  void* mem2 = c_allocator_alloc(a, c_allocator_alignas(int, 3), true);
  EXPECT_TRUE_MSG(mem2, c_error_to_str(c_error_get()));

  EXPECT_EQ(c_allocator_mem_alignment(mem2), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem2), sizeof(int) * 3);
  EXPECT_EQ(memcmp(mem2, (int[]){0, 0, 0}, sizeof(int) * 3), 0);

  c_allocator_free(a, mem);
  c_allocator_free(a, mem2);
}

UTEST(CAllocator, default_realloc)
{
  CAllocator* a = c_allocator_default();

  void* mem = c_allocator_alloc(a, c_allocator_alignas(int, 10), true);
  EXPECT_TRUE_MSG(mem, c_error_to_str(c_error_get()));
  mem = c_allocator_resize(a, mem, sizeof(int) * 100);
  EXPECT_TRUE_MSG(mem, c_error_to_str(c_error_get()));

  EXPECT_EQ(c_allocator_mem_alignment(mem), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem), sizeof(int) * 100);

  c_allocator_free(a, mem);
}

UTEST(CAllocator, arena_general)
{
  CAllocator* a = c_allocator_arena_create(1000);
  EXPECT_TRUE_MSG(a, c_error_to_str(c_error_get()));

  void* mem = c_allocator_alloc(a, c_allocator_alignas(int, 10), true);
  EXPECT_TRUE_MSG(mem, c_error_to_str(c_error_get()));

  EXPECT_EQ(c_allocator_mem_alignment(mem), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem), sizeof(int) * 10);
  EXPECT_EQ(memcmp(mem, (int[]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, sizeof(int) * 10), 0);

  ((int*)mem)[5] = 10;
  EXPECT_EQ(((int*)mem)[5], 10);

  void* mem2 = c_allocator_alloc(a, c_allocator_alignas(int, 3), true);
  EXPECT_TRUE_MSG(mem2, c_error_to_str(c_error_get()));

  EXPECT_EQ(c_allocator_mem_alignment(mem2), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem2), sizeof(int) * 3);
  EXPECT_EQ(memcmp(mem2, (int[]){0, 0, 0}, sizeof(int) * 3), 0);

  c_allocator_free(a, mem);
  c_allocator_free(a, mem2);
  c_allocator_arena_destroy(a);
}

UTEST(CAllocator, arena_realloc)
{
  CAllocator* a = c_allocator_arena_create(1000);
  EXPECT_TRUE_MSG(a, c_error_to_str(c_error_get()));

  void* mem = c_allocator_alloc(a, c_allocator_alignas(int, 10), true);
  EXPECT_TRUE_MSG(mem, c_error_to_str(c_error_get()));
  void* new_mem = c_allocator_resize(a, mem, sizeof(int) * 100);
  EXPECT_TRUE_MSG(new_mem, c_error_to_str(c_error_get()));
  mem = new_mem;

  EXPECT_EQ(c_allocator_mem_alignment(mem), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem), sizeof(int) * 100);

  c_allocator_free(a, mem);
  c_allocator_arena_destroy(a);
}

UTEST(CAllocator, fixed_buffer_general)
{
  int         buf[1000];
  CAllocator* a = c_allocator_fixed_buffer_create(buf, 1000);
  EXPECT_TRUE_MSG(a, c_error_to_str(c_error_get()));

  void* mem = c_allocator_alloc(a, c_allocator_alignas(int, 10), true);
  EXPECT_TRUE_MSG(mem, c_error_to_str(c_error_get()));

  EXPECT_EQ(c_allocator_mem_alignment(mem), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem), sizeof(int) * 10);
  EXPECT_EQ(
      memcmp(mem, (int[]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, sizeof(int) * 10), 0);

  ((int*)mem)[5] = 10;
  EXPECT_EQ(((int*)mem)[5], 10);

  void* mem2 = c_allocator_alloc(a, c_allocator_alignas(int, 3), true);
  EXPECT_TRUE_MSG(mem2, c_error_to_str(c_error_get()));

  EXPECT_EQ(c_allocator_mem_alignment(mem2), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem2), sizeof(int) * 3);
  EXPECT_EQ(memcmp(mem2, (int[]){0, 0, 0}, sizeof(int) * 3), 0);

  c_allocator_free(a, mem);
  c_allocator_free(a, mem2);
  c_allocator_fixed_buffer_destroy(a);
}

UTEST(CAllocator, fixed_buffer_realloc)
{
  int         buf[1000];
  CAllocator* a = c_allocator_fixed_buffer_create(buf, 1000);
  EXPECT_TRUE_MSG(a, c_error_to_str(c_error_get()));

  void* mem = c_allocator_alloc(a, c_allocator_alignas(int, 10), true);
  EXPECT_TRUE_MSG(mem, c_error_to_str(c_error_get()));
  mem = c_allocator_resize(a, mem, sizeof(int) * 100);
  EXPECT_TRUE_MSG(mem, c_error_to_str(c_error_get()));

  EXPECT_EQ(c_allocator_mem_alignment(mem), alignof(int));
  EXPECT_EQ(c_allocator_mem_size(mem), sizeof(int) * 100);

  c_allocator_free(a, mem);
  c_allocator_fixed_buffer_destroy(a);
}
