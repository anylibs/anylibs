/**
 * @file allocator.c
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

#include <utest.h>

#include <stdalign.h>

UTEST(CAllocator, default_general)
{
  CAllocator* a;
  int         err = c_allocator_default(&a);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  CMemory mem;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 10), true, &mem);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem.data, NULL);
  EXPECT_EQ(mem.alignment, alignof(int));
  EXPECT_EQ(mem.size, sizeof(int) * 10);
  EXPECT_EQ(
      memcmp(mem.data, (int[]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, sizeof(int) * 10),
      0);

  ((int*)mem.data)[5] = 10;
  EXPECT_EQ(((int*)mem.data)[5], 10);

  CMemory mem2;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 3), true, &mem2);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem2.data, NULL);
  EXPECT_EQ(mem2.alignment, alignof(int));
  EXPECT_EQ(mem2.size, sizeof(int) * 3);
  EXPECT_EQ(memcmp(mem2.data, (int[]){0, 0, 0}, sizeof(int) * 3), 0);

  c_allocator_free(a, &mem);
  c_allocator_free(a, &mem2);
}

UTEST(CAllocator, default_realloc)
{
  CAllocator* a;
  int         err = c_allocator_default(&a);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  CMemory mem;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 10), true, &mem);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_allocator_resize(a, &mem, sizeof(int) * 100);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem.data, NULL);
  EXPECT_EQ(mem.alignment, alignof(int));
  EXPECT_EQ(mem.size, sizeof(int) * 100);

  c_allocator_free(a, &mem);
}

UTEST(CAllocator, arena_general)
{
  CAllocator* a;
  int         err = c_allocator_arena_create(1000, &a);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  CMemory mem;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 10), true, &mem);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem.data, NULL);
  EXPECT_EQ(mem.alignment, alignof(int));
  EXPECT_EQ(mem.size, sizeof(int) * 10);
  EXPECT_EQ(
      memcmp(mem.data, (int[]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, sizeof(int) * 10),
      0);

  ((int*)mem.data)[5] = 10;
  EXPECT_EQ(((int*)mem.data)[5], 10);

  CMemory mem2;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 3), true, &mem2);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem2.data, NULL);
  EXPECT_EQ(mem2.alignment, alignof(int));
  EXPECT_EQ(mem2.size, sizeof(int) * 3);
  EXPECT_EQ(memcmp(mem2.data, (int[]){0, 0, 0}, sizeof(int) * 3), 0);

  c_allocator_free(a, &mem);
  c_allocator_free(a, &mem2);
  c_allocator_arena_destroy(&a);
}

UTEST(CAllocator, arena_realloc)
{
  CAllocator* a;
  int         err = c_allocator_arena_create(1000, &a);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  CMemory mem;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 10), true, &mem);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_allocator_resize(a, &mem, sizeof(int) * 100);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem.data, NULL);
  EXPECT_EQ(mem.alignment, alignof(int));
  EXPECT_EQ(mem.size, sizeof(int) * 100);

  c_allocator_free(a, &mem);
  c_allocator_arena_destroy(&a);
}

UTEST(CAllocator, fixed_buffer_general)
{
  CAllocator* a;
  int         buf[1000];
  int         err = c_allocator_fixed_buffer_create(buf, 1000, &a);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  CMemory mem;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 10), true, &mem);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem.data, NULL);
  EXPECT_EQ(mem.alignment, alignof(int));
  EXPECT_EQ(mem.size, sizeof(int) * 10);
  EXPECT_EQ(
      memcmp(mem.data, (int[]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, sizeof(int) * 10),
      0);

  ((int*)mem.data)[5] = 10;
  EXPECT_EQ(((int*)mem.data)[5], 10);

  CMemory mem2;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 3), true, &mem2);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem2.data, NULL);
  EXPECT_EQ(mem2.alignment, alignof(int));
  EXPECT_EQ(mem2.size, sizeof(int) * 3);
  EXPECT_EQ(memcmp(mem2.data, (int[]){0, 0, 0}, sizeof(int) * 3), 0);

  c_allocator_free(a, &mem);
  c_allocator_free(a, &mem2);
  c_allocator_fixed_buffer_destroy(&a);
}

UTEST(CAllocator, fixed_buffer_realloc)
{
  CAllocator* a;
  int         buf[1000];
  int         err = c_allocator_fixed_buffer_create(buf, 1000, &a);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  CMemory mem;
  err = c_allocator_alloc(a, c_allocator_alignas(int, 10), true, &mem);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_allocator_resize(a, &mem, sizeof(int) * 100);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_NE(mem.data, NULL);
  EXPECT_EQ(mem.alignment, alignof(int));
  EXPECT_EQ(mem.size, sizeof(int) * 100);

  c_allocator_free(a, &mem);
  c_allocator_fixed_buffer_destroy(&a);
}
