/**
 * @file vec.c
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

#include "anylibs/vec.h"

#include <stdio.h>

#include <utest.h>

static int cmp(void const* a, void const* b);
static int cmp_inv(void const* a, void const* b);

typedef struct CVecTest {
  CVec* vec;
} CVecTest;

UTEST_F_SETUP(CVecTest)
{
  int err = c_vec_create(sizeof(int), NULL, &utest_fixture->vec);
  ASSERT_EQ(err, 0);

  err = c_vec_push(utest_fixture->vec, &(int){12});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(utest_fixture->vec, &(int){13});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(utest_fixture->vec, &(int){14});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(utest_fixture->vec, &(int){15});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(utest_fixture->vec, &(int){16});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(c_vec_len(utest_fixture->vec), 5U);
}

UTEST_F_TEARDOWN(CVecTest)
{
  (void)(utest_result);
  c_vec_destroy(&utest_fixture->vec);
}

UTEST_F(CVecTest, pop)
{
  int data = 0;
  int err  = c_vec_pop(utest_fixture->vec, &data);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(data, 16);
}

UTEST_F(CVecTest, remove_range)
{
  int err = c_vec_remove_range(utest_fixture->vec, 1U, 3U);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(c_vec_len(utest_fixture->vec), 2U);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 12);
}

UTEST_F(CVecTest, insert)
{
  int err = c_vec_insert(utest_fixture->vec, 0, &(int){20});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 20);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 12);
}

UTEST_F(CVecTest, insert_range)
{
  int err = c_vec_insert_range(utest_fixture->vec, 1, &(int[]){1, 2, 3}, 3);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(c_vec_len(utest_fixture->vec), 8U);

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 12);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 1);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 2);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 3);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 13);
}

UTEST_F(CVecTest, iter)
{
  size_t counter = 0;
  int*   element;
  int    gt[] = {12, 13, 14, 15, 16};
  CIter  iter;
  c_vec_iter_create(utest_fixture->vec, NULL, &iter);

  while (c_iter_next(&iter, utest_fixture->vec->data,
                     c_vec_len(utest_fixture->vec), (void**)&element)) {
    EXPECT_EQ(*element, gt[counter++]);
  }
}

UTEST_F(CVecTest, slice)
{
  CVec* slice;
  int   err = c_vec_slice(utest_fixture->vec, 1, 3, &slice);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(((int*)slice->data)[0], 13);
  EXPECT_EQ(((int*)slice->data)[1], 14);
  EXPECT_EQ(((int*)slice->data)[2], 15);

  c_vec_destroy(&slice);
}

UTEST_F(CVecTest, clone)
{
  CVec* clone;
  int   err = c_vec_clone(utest_fixture->vec, true, &clone);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(c_vec_len(clone), c_vec_len(utest_fixture->vec));
  EXPECT_EQ(c_vec_capacity(clone), c_vec_len(utest_fixture->vec));
  EXPECT_EQ(c_vec_element_size(clone), c_vec_element_size(utest_fixture->vec));

  EXPECT_EQ(memcmp(utest_fixture->vec->data, clone->data, c_vec_len(clone)), 0);

  c_vec_destroy(&clone);
}

UTEST_F(CVecTest, reverse)
{
  int err = c_vec_reverse(utest_fixture->vec);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 12);
}

UTEST_F(CVecTest, search)
{
  size_t index;
  int    err = c_vec_search(utest_fixture->vec, &(int){13}, cmp, &index);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(index, 1U);
  err = c_vec_binary_search(utest_fixture->vec, &(int){13}, cmp, &index);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(index, 1U);

  err = c_vec_search(utest_fixture->vec, &(int){20}, cmp, &index);
  EXPECT_NE(err, 0);
  err = c_vec_binary_search(utest_fixture->vec, &(int){20}, cmp, &index);
  EXPECT_NE(err, 0);
}

UTEST_F(CVecTest, sort)
{
  EXPECT_TRUE(c_vec_is_sorted(utest_fixture->vec, cmp));
  c_vec_sort(utest_fixture->vec, cmp_inv);
  EXPECT_TRUE(c_vec_is_sorted_inv(utest_fixture->vec, cmp));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 12);
}

UTEST_F(CVecTest, starts_with)
{
  bool starts_with
      = c_vec_starts_with(utest_fixture->vec, (int[]){12, 13, 14}, 3, cmp);
  EXPECT_TRUE(starts_with);

  starts_with
      = c_vec_starts_with(utest_fixture->vec, (int[]){12, 12, 12}, 3, cmp);
  EXPECT_FALSE(starts_with);
}

UTEST_F(CVecTest, ends_with)
{
  bool starts_with
      = c_vec_ends_with(utest_fixture->vec, (int[]){14, 15, 16}, 3, cmp);
  EXPECT_TRUE(starts_with);

  starts_with
      = c_vec_ends_with(utest_fixture->vec, (int[]){12, 12, 12}, 3, cmp);
  EXPECT_FALSE(starts_with);
}

UTEST_F(CVecTest, rotate_right)
{
  int err = c_vec_rotate_right(utest_fixture->vec, 3);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 14);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 12);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 13);
}

UTEST_F(CVecTest, rotate_left)
{
  int err = c_vec_rotate_left(utest_fixture->vec, 3);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 12);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 14);
}

UTEST_F(CVecTest, concatenate)
{
  CVec* vec2;
  int   err = c_vec_create_with_capacity(sizeof(int), 3, true, NULL, &vec2);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  c_vec_fill(vec2, &(int){1});

  err = c_vec_concatenate(utest_fixture->vec, vec2);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 12);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 13);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[5], 1);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[6], 1);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[7], 1);

  c_vec_destroy(&vec2);
}

UTEST(CVecTest, general)
{
  CVec* vec2;
  int   err = c_vec_create(sizeof(char), NULL, &vec2);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_vec_push(vec2, &(char){'\0'});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_insert(vec2, 0, &(char){'a'});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(((char*)vec2->data)[0], 'a');
  EXPECT_EQ(((char*)vec2->data)[1], '\0');

  c_vec_destroy(&vec2);
}

UTEST(CVecTest, wrong_index)
{
  CVec* vec;
  int   err = c_vec_create(sizeof(char), NULL, &vec);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_vec_push(vec, &(char){'\0'});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_insert(vec, 1, &(char){'a'});
  EXPECT_EQ(err, C_ERROR_wrong_index);

  c_vec_destroy(&vec);
}

UTEST(CVecTest, shrint_to_fit)
{
  CVec* vec;
  int   err = c_vec_create_with_capacity(sizeof(int), 100, true, NULL, &vec);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_vec_push(vec, &(int){1});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){2});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){3});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){4});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_vec_shrink_to_fit(vec);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(c_vec_capacity(vec), 4U);
  EXPECT_EQ(c_vec_len(vec), 4U);

  EXPECT_EQ(((int*)vec->data)[0], 1);
  EXPECT_EQ(((int*)vec->data)[1], 2);
  EXPECT_EQ(((int*)vec->data)[2], 3);
  EXPECT_EQ(((int*)vec->data)[3], 4);

  c_vec_destroy(&vec);
}

UTEST(CVecTest, dedup)
{
  CVec* vec;
  int   err = c_vec_create_with_capacity(sizeof(int), 100, true, NULL, &vec);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_vec_push(vec, &(int){1});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){2});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){2});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){3});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){4});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){4});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){4});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_vec_push(vec, &(int){4});
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_vec_deduplicate(vec, cmp);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(c_vec_len(vec), 4U);

  EXPECT_EQ(((int*)vec->data)[0], 1);
  EXPECT_EQ(((int*)vec->data)[1], 2);
  EXPECT_EQ(((int*)vec->data)[2], 3);
  EXPECT_EQ(((int*)vec->data)[3], 4);

  c_vec_destroy(&vec);
}

UTEST(CVecTest, fill)
{
  size_t const vec_cap = 10;
  int          gt[]    = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  CVec*        vec;
  int          err = c_vec_create(sizeof(int), NULL, &vec);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_vec_set_capacity(vec, 10);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(c_vec_len(vec), 0U);
  c_vec_fill(vec, &(int){1});
  EXPECT_EQ(c_vec_len(vec), 10U);

  EXPECT_EQ(memcmp(vec->data, gt, vec_cap), 0);

  c_vec_destroy(&vec);
}

UTEST(CVecTest, fill_with_repeat)
{
  size_t const vec_cap = 10;
  int          gt[]    = {1, 2, 3, 1, 2, 3, 1, 2, 3};
  CVec*        vec;
  int err = c_vec_create_with_capacity(sizeof(int), vec_cap, true, NULL, &vec);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(c_vec_len(vec), 0U);
  c_vec_fill_with_repeat(vec, (int[]){1, 2, 3}, 3);
  EXPECT_EQ(c_vec_len(vec), 9U);
  EXPECT_EQ(c_vec_capacity(vec), 10U);

  EXPECT_EQ(memcmp(vec->data, gt, vec_cap), 0);

  c_vec_destroy(&vec);
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
