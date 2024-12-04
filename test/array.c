/**
 * @file array.c
 * @copyright (C) 2024-2025 Mohamed A. Elmeligy
 * MIT License
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

#include "anylibs/array.h"

#include <stdio.h>

#include <utest.h>

static int cmp(void const* a, void const* b);
static int cmp_inv(void const* a, void const* b);

UTEST_F_SETUP(CArray)
{
  int err = c_array_init(sizeof(int), utest_fixture);
  ASSERT_EQ(err, 0);

  err = c_array_push(utest_fixture, &(int){12});
  EXPECT_EQ(err, 0);
  err = c_array_push(utest_fixture, &(int){13});
  EXPECT_EQ(err, 0);
  err = c_array_push(utest_fixture, &(int){14});
  EXPECT_EQ(err, 0);
  err = c_array_push(utest_fixture, &(int){15});
  EXPECT_EQ(err, 0);
  err = c_array_push(utest_fixture, &(int){16});
  EXPECT_EQ(err, 0);
  EXPECT_EQ(c_array_len(utest_fixture), 5U);
}

UTEST_F_TEARDOWN(CArray)
{
  (void)(utest_result);
  c_array_deinit(utest_fixture);
}

UTEST_F(CArray, pop)
{
  int data = 0;
  int err  = c_array_pop(utest_fixture, &data);
  EXPECT_EQ(err, 0);
  EXPECT_EQ(data, 16);
}

UTEST_F(CArray, remove_range)
{
  int err = c_array_remove_range(utest_fixture, 1U, 3U);
  EXPECT_EQ(err, 0);
  EXPECT_EQ(c_array_len(utest_fixture), 2U);
  EXPECT_EQ(((int*)utest_fixture->data)[0], 12);
}

UTEST_F(CArray, insert)
{
  int err = c_array_insert(utest_fixture, &(int){20}, 0);
  EXPECT_EQ(err, 0);
  EXPECT_EQ(((int*)utest_fixture->data)[0], 20);
  EXPECT_EQ(((int*)utest_fixture->data)[1], 12);
}

UTEST_F(CArray, insert_range)
{
  int err = c_array_insert_range(utest_fixture, 1, &(int[]){1, 2, 3}, 3);
  EXPECT_EQ(err, 0);
  EXPECT_EQ(c_array_len(utest_fixture), 8U);

  EXPECT_EQ(((int*)utest_fixture->data)[0], 12);
  EXPECT_EQ(((int*)utest_fixture->data)[1], 1);
  EXPECT_EQ(((int*)utest_fixture->data)[2], 2);
  EXPECT_EQ(((int*)utest_fixture->data)[3], 3);
  EXPECT_EQ(((int*)utest_fixture->data)[4], 13);
}

UTEST_F(CArray, iter)
{
  size_t index   = 0;
  size_t counter = 0;
  int*   element;
  int    gt[] = {12, 13, 14, 15, 16};
  while (c_array_iter(utest_fixture, &index, (void**)&element)) {
    EXPECT_EQ(*element, gt[counter++]);
  }
}

UTEST_F(CArray, slice)
{
  CArray slice;
  int    err = c_array_slice(utest_fixture, 1, 3, &slice);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(((int*)slice.data)[0], 13);
  EXPECT_EQ(((int*)slice.data)[1], 14);
  EXPECT_EQ(((int*)slice.data)[2], 15);
}

UTEST_F(CArray, clone)
{
  CArray clone;
  int    err = c_array_clone(utest_fixture, true, &clone);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(c_array_len(&clone), c_array_len(utest_fixture));
  EXPECT_EQ(c_array_capacity(&clone), c_array_len(utest_fixture));
  EXPECT_EQ(c_array_element_size(&clone), c_array_element_size(utest_fixture));

  EXPECT_EQ(memcmp(utest_fixture->data, clone.data, c_array_len(&clone)), 0);

  c_array_deinit(&clone);
}

UTEST_F(CArray, reverse)
{
  int err = c_array_reverse(utest_fixture);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(((int*)utest_fixture->data)[0], 16);
  EXPECT_EQ(((int*)utest_fixture->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->data)[4], 12);
}

UTEST_F(CArray, search)
{
  size_t index;
  int    err = c_array_search(utest_fixture, &(int){13}, cmp, &index);
  EXPECT_EQ(err, 0);
  EXPECT_EQ(index, 1U);
  err = c_array_binary_search(utest_fixture, &(int){13}, cmp, &index);
  EXPECT_EQ(err, 0);
  EXPECT_EQ(index, 1U);

  err = c_array_search(utest_fixture, &(int){20}, cmp, &index);
  EXPECT_NE(err, 0);
  err = c_array_binary_search(utest_fixture, &(int){20}, cmp, &index);
  EXPECT_NE(err, 0);
}

UTEST_F(CArray, sort)
{
  EXPECT_TRUE(c_array_is_sorted(utest_fixture, cmp));
  c_array_sort(utest_fixture, cmp_inv);
  EXPECT_TRUE(c_array_is_sorted_inv(utest_fixture, cmp));

  EXPECT_EQ(((int*)utest_fixture->data)[0], 16);
  EXPECT_EQ(((int*)utest_fixture->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->data)[4], 12);
}

UTEST_F(CArray, starts_with)
{
  bool starts_with
      = c_array_starts_with(utest_fixture, (int[]){12, 13, 14}, 3, cmp);
  EXPECT_TRUE(starts_with);

  starts_with = c_array_starts_with(utest_fixture, (int[]){12, 12, 12}, 3, cmp);
  EXPECT_FALSE(starts_with);
}

UTEST_F(CArray, ends_with)
{
  bool starts_with
      = c_array_ends_with(utest_fixture, (int[]){14, 15, 16}, 3, cmp);
  EXPECT_TRUE(starts_with);

  starts_with = c_array_ends_with(utest_fixture, (int[]){12, 12, 12}, 3, cmp);
  EXPECT_FALSE(starts_with);
}

UTEST_F(CArray, rotate_right)
{
  int err = c_array_rotate_right(utest_fixture, 3);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(((int*)utest_fixture->data)[0], 14);
  EXPECT_EQ(((int*)utest_fixture->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->data)[2], 16);
  EXPECT_EQ(((int*)utest_fixture->data)[3], 12);
  EXPECT_EQ(((int*)utest_fixture->data)[4], 13);
}

UTEST_F(CArray, rotate_left)
{
  int err = c_array_rotate_left(utest_fixture, 3);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(((int*)utest_fixture->data)[0], 15);
  EXPECT_EQ(((int*)utest_fixture->data)[1], 16);
  EXPECT_EQ(((int*)utest_fixture->data)[2], 12);
  EXPECT_EQ(((int*)utest_fixture->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->data)[4], 14);
}

UTEST_F(CArray, concatenate)
{
  CArray arr2;
  int    err = c_array_init_with_capacity(sizeof(int), 3, true, &arr2);
  EXPECT_EQ(err, 0);
  c_array_fill(&arr2, &(int){1});

  err = c_array_concatenate(utest_fixture, &arr2);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(((int*)utest_fixture->data)[0], 12);
  EXPECT_EQ(((int*)utest_fixture->data)[1], 13);
  EXPECT_EQ(((int*)utest_fixture->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->data)[3], 15);
  EXPECT_EQ(((int*)utest_fixture->data)[4], 16);
  EXPECT_EQ(((int*)utest_fixture->data)[5], 1);
  EXPECT_EQ(((int*)utest_fixture->data)[6], 1);
  EXPECT_EQ(((int*)utest_fixture->data)[7], 1);

  c_array_deinit(&arr2);
}

UTEST(CArray, general)
{
  CArray array2;
  int    err = c_array_init(sizeof(char), &array2);
  EXPECT_EQ(err, 0);

  err = c_array_push(&array2, &(char){'\0'});
  EXPECT_EQ(err, 0);
  err = c_array_insert(&array2, &(char){'a'}, 0);
  EXPECT_EQ(err, 0);
  EXPECT_EQ(((char*)array2.data)[0], 'a');
  EXPECT_EQ(((char*)array2.data)[1], '\0');

  c_array_deinit(&array2);
}

UTEST(CArray, wrong_index)
{
  CArray array;
  int    err = c_array_init(sizeof(char), &array);
  EXPECT_EQ(err, 0);

  err = c_array_push(&array, &(char){'\0'});
  EXPECT_EQ(err, 0);
  err = c_array_insert(&array, &(char){'a'}, 1);
  EXPECT_EQ(err, C_ERROR_wrong_index);

  c_array_deinit(&array);
}

UTEST(CArray, shrint_to_fit)
{
  CArray array;
  int    err = c_array_init_with_capacity(sizeof(int), 100, true, &array);
  EXPECT_EQ(err, 0);

  err = c_array_push(&array, &(int){1});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){2});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){3});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){4});
  EXPECT_EQ(err, 0);

  err = c_array_shrink_to_fit(&array);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(c_array_capacity(&array), 4U);
  EXPECT_EQ(c_array_len(&array), 4U);

  EXPECT_EQ(((int*)array.data)[0], 1);
  EXPECT_EQ(((int*)array.data)[1], 2);
  EXPECT_EQ(((int*)array.data)[2], 3);
  EXPECT_EQ(((int*)array.data)[3], 4);

  c_array_deinit(&array);
}

UTEST(CArray, dedup)
{
  CArray array;
  int    err = c_array_init_with_capacity(sizeof(int), 100, true, &array);
  EXPECT_EQ(err, 0);

  err = c_array_push(&array, &(int){1});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){2});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){2});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){3});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){4});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){4});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){4});
  EXPECT_EQ(err, 0);
  err = c_array_push(&array, &(int){4});
  EXPECT_EQ(err, 0);

  err = c_array_deduplicate(&array, cmp);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(c_array_len(&array), 4U);

  EXPECT_EQ(((int*)array.data)[0], 1);
  EXPECT_EQ(((int*)array.data)[1], 2);
  EXPECT_EQ(((int*)array.data)[2], 3);
  EXPECT_EQ(((int*)array.data)[3], 4);

  c_array_deinit(&array);
}

UTEST(CArray, fill)
{
  size_t const arr_cap = 10;
  int          gt[]    = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  CArray       array;
  int          err = c_array_init(sizeof(int), &array);
  EXPECT_EQ(err, 0);

  err = c_array_set_capacity(&array, 10);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(c_array_len(&array), 0U);
  c_array_fill(&array, &(int){1});
  EXPECT_EQ(c_array_len(&array), 10U);

  EXPECT_EQ(memcmp(array.data, gt, arr_cap), 0);

  c_array_deinit(&array);
}

UTEST(CArray, fill_with_repeat)
{
  size_t const arr_cap = 10;
  int          gt[]    = {1, 2, 3, 1, 2, 3, 1, 2, 3};
  CArray       array;
  int err = c_array_init_with_capacity(sizeof(int), arr_cap, true, &array);
  EXPECT_EQ(err, 0);

  EXPECT_EQ(c_array_len(&array), 0U);
  c_array_fill_with_repeat(&array, (int[]){1, 2, 3}, 3);
  EXPECT_EQ(c_array_len(&array), 9U);
  EXPECT_EQ(c_array_capacity(&array), 10U);

  EXPECT_EQ(memcmp(array.data, gt, arr_cap), 0);

  c_array_deinit(&array);
}

/******************************************************************************/

int
cmp(void const* a, void const* b)
{
  return *(int*)a - *(int*)b;
}

int
cmp_inv(void const* a, void const* b)
{
  return *(int*)b - *(int*)a;
}
