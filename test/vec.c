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
  utest_fixture->vec = c_vec_create(sizeof(int), NULL);
  ASSERT_TRUE(utest_fixture->vec);

  EXPECT_TRUE(c_vec_push(utest_fixture->vec, &(int){12}));
  EXPECT_TRUE(c_vec_push(utest_fixture->vec, &(int){13}));
  EXPECT_TRUE(c_vec_push(utest_fixture->vec, &(int){14}));
  EXPECT_TRUE(c_vec_push(utest_fixture->vec, &(int){15}));
  EXPECT_TRUE(c_vec_push(utest_fixture->vec, &(int){16}));

  EXPECT_EQ(5U, c_vec_len(utest_fixture->vec));
}

UTEST_F_TEARDOWN(CVecTest)
{
  (void)(utest_result);
  c_vec_destroy(utest_fixture->vec);
}

UTEST_F(CVecTest, pop)
{
  int data;
  EXPECT_TRUE(c_vec_pop(utest_fixture->vec, &data));
  EXPECT_TRUE(data);
  EXPECT_EQ(16, data);
}

UTEST_F(CVecTest, remove_range)
{
  EXPECT_TRUE(c_vec_remove_range(utest_fixture->vec, 1U, 3U));

  EXPECT_EQ(2U, c_vec_len(utest_fixture->vec));
  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 12);
}

UTEST_F(CVecTest, insert)
{
  EXPECT_TRUE(c_vec_insert(utest_fixture->vec, 0, &(int){20}));
  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 20);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 12);
}

UTEST_F(CVecTest, insert_range)
{
  EXPECT_TRUE(c_vec_insert_range(utest_fixture->vec, 1, &(int[]){1, 2, 3}, 3));
  EXPECT_EQ(8U, c_vec_len(utest_fixture->vec));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 12);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 1);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 2);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 3);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 13);
}

UTEST_F(CVecTest, iter)
{
  size_t counter = 0;
  int    gt[]    = {12, 13, 14, 15, 16};

  void* result;
  CIter iter = c_vec_iter(utest_fixture->vec);
  while (c_iter_next(&iter, &result)) {
    EXPECT_EQ(*(int*)result, gt[counter++]);
  }
}

UTEST_F(CVecTest, slice)
{
  CVec* slice = c_vec_slice(utest_fixture->vec, 1, 3);
  EXPECT_TRUE(slice);

  EXPECT_EQ(((int*)slice->data)[0], 13);
  EXPECT_EQ(((int*)slice->data)[1], 14);
  EXPECT_EQ(((int*)slice->data)[2], 15);

  c_vec_destroy(slice);
}

UTEST_F(CVecTest, clone)
{
  CVec* clone = c_vec_clone(utest_fixture->vec, true);
  EXPECT_TRUE(clone);

  EXPECT_EQ(c_vec_len(utest_fixture->vec), c_vec_len(clone));
  EXPECT_NE(c_vec_capacity(utest_fixture->vec), c_vec_capacity(clone));
  EXPECT_EQ(c_vec_element_size(utest_fixture->vec), c_vec_element_size(clone));

  EXPECT_EQ(memcmp(utest_fixture->vec->data, clone->data, c_vec_len(clone)), 0);

  c_vec_destroy(clone);
}

UTEST_F(CVecTest, reverse)
{
  EXPECT_TRUE(c_vec_reverse(utest_fixture->vec));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 12);
}

UTEST_F(CVecTest, search)
{
  void* result;
  EXPECT_TRUE(c_vec_find(utest_fixture->vec, &(int){13}, cmp, &result));
  EXPECT_TRUE(result);
  EXPECT_EQ(13, *(int*)result);

  c_vec_binary_find(utest_fixture->vec, &(int){13}, cmp, &result);
  EXPECT_TRUE(result);
  EXPECT_EQ(13, *(int*)result);

  bool status = c_vec_find(utest_fixture->vec, &(int){20}, cmp, &result);
  EXPECT_FALSE(status);
  status = c_vec_binary_find(utest_fixture->vec, &(int){20}, cmp, &result);
  EXPECT_FALSE(status);
}

UTEST_F(CVecTest, sort)
{
  EXPECT_TRUE(c_vec_is_sorted(utest_fixture->vec, cmp) == 0);
  EXPECT_TRUE(c_vec_sort(utest_fixture->vec, cmp_inv));
  EXPECT_TRUE(c_vec_is_sorted(utest_fixture->vec, cmp_inv) == 0);

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 12);
}

UTEST_F(CVecTest, starts_with)
{
  int starts_with = c_vec_starts_with(utest_fixture->vec, (int[]){12, 13, 14}, 3, cmp);
  EXPECT_TRUE(starts_with == 0);

  starts_with = c_vec_starts_with(utest_fixture->vec, (int[]){12, 12, 12}, 3, cmp);
  EXPECT_FALSE(starts_with == 0);
}

UTEST_F(CVecTest, ends_with)
{
  int ends_with = c_vec_ends_with(utest_fixture->vec, (int[]){14, 15, 16}, 3, cmp);
  EXPECT_TRUE(ends_with == 0);

  ends_with = c_vec_ends_with(utest_fixture->vec, (int[]){12, 12, 12}, 3, cmp);
  EXPECT_FALSE(ends_with == 0);
}

UTEST_F(CVecTest, rotate_right)
{
  EXPECT_TRUE(c_vec_rotate_right(utest_fixture->vec, 3));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 14);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 12);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 13);
}

UTEST_F(CVecTest, rotate_left)
{
  EXPECT_TRUE(c_vec_rotate_left(utest_fixture->vec, 3));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 12);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 13);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 14);
}

UTEST_F(CVecTest, concatenate)
{
  CVec* vec2 = c_vec_create_with_capacity(sizeof(int), 3, true, NULL);
  EXPECT_TRUE(vec2);
  c_vec_fill(vec2, &(int){1});

  EXPECT_TRUE(c_vec_concatenate(utest_fixture->vec, vec2));

  EXPECT_EQ(((int*)utest_fixture->vec->data)[0], 12);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[1], 13);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[2], 14);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[3], 15);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[4], 16);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[5], 1);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[6], 1);
  EXPECT_EQ(((int*)utest_fixture->vec->data)[7], 1);

  c_vec_destroy(vec2);
}

UTEST(CVec, general)
{
  CVec* vec2 = c_vec_create(sizeof(char), NULL);
  EXPECT_TRUE(vec2);

  EXPECT_TRUE(c_vec_push(vec2, &(char){'\0'}));
  EXPECT_TRUE(c_vec_insert(vec2, 0, &(char){'a'}));
  EXPECT_EQ(((char*)vec2->data)[0], 'a');
  EXPECT_EQ(((char*)vec2->data)[1], '\0');

  c_vec_destroy(vec2);
}

UTEST(CVec, wrong_index)
{
  CVec* vec = c_vec_create(sizeof(char), NULL);
  EXPECT_TRUE(vec);

  EXPECT_TRUE(c_vec_push(vec, &(char){'\0'}));
  EXPECT_FALSE(c_vec_insert(vec, 1, &(char){'a'}));

  c_vec_destroy(vec);
}

UTEST(CVec, shrint_to_fit)
{
  CVec* vec = c_vec_create_with_capacity(sizeof(int), 100, true, NULL);
  EXPECT_TRUE(vec);

  EXPECT_TRUE(c_vec_push(vec, &(int){1}));
  EXPECT_TRUE(c_vec_push(vec, &(int){2}));
  EXPECT_TRUE(c_vec_push(vec, &(int){3}));
  EXPECT_TRUE(c_vec_push(vec, &(int){4}));

  EXPECT_TRUE(c_vec_shrink_to_fit(vec));
  EXPECT_EQ(4U, c_vec_len(vec));
  EXPECT_EQ(4U, c_vec_capacity(vec));

  EXPECT_EQ(((int*)vec->data)[0], 1);
  EXPECT_EQ(((int*)vec->data)[1], 2);
  EXPECT_EQ(((int*)vec->data)[2], 3);
  EXPECT_EQ(((int*)vec->data)[3], 4);

  c_vec_destroy(vec);
}

UTEST(CVec, dedup)
{
  CVec* vec = c_vec_create_with_capacity(sizeof(int), 100, true, NULL);
  EXPECT_TRUE(vec);

  EXPECT_TRUE(c_vec_push(vec, &(int){1}));
  EXPECT_TRUE(c_vec_push(vec, &(int){2}));
  EXPECT_TRUE(c_vec_push(vec, &(int){2}));
  EXPECT_TRUE(c_vec_push(vec, &(int){3}));
  EXPECT_TRUE(c_vec_push(vec, &(int){4}));
  EXPECT_TRUE(c_vec_push(vec, &(int){4}));
  EXPECT_TRUE(c_vec_push(vec, &(int){4}));
  EXPECT_TRUE(c_vec_push(vec, &(int){4}));

  EXPECT_TRUE(c_vec_deduplicate(vec, cmp));
  EXPECT_EQ(4U, c_vec_len(vec));

  EXPECT_EQ(((int*)vec->data)[0], 1);
  EXPECT_EQ(((int*)vec->data)[1], 2);
  EXPECT_EQ(((int*)vec->data)[2], 3);
  EXPECT_EQ(((int*)vec->data)[3], 4);

  c_vec_destroy(vec);
}

UTEST(CVec, fill)
{
  size_t const vec_cap = 10;
  int          gt[]    = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  CVec*        vec     = c_vec_create(sizeof(int), NULL);
  EXPECT_TRUE(vec);

  EXPECT_TRUE(c_vec_set_capacity(vec, 10));
  EXPECT_EQ(0U, c_vec_len(vec));

  c_vec_fill(vec, &(int){1});
  EXPECT_EQ(10U, c_vec_len(vec));

  EXPECT_EQ(memcmp(vec->data, gt, vec_cap), 0);

  c_vec_destroy(vec);
}

UTEST(CVec, fill_with_repeat)
{
  size_t const vec_cap = 10;
  int const    gt[]    = {1, 2, 3, 1, 2, 3, 1, 2, 3};

  CVec* vec = c_vec_create_with_capacity(sizeof(int), vec_cap, true, NULL);
  EXPECT_TRUE(vec);
  EXPECT_EQ(0U, c_vec_len(vec));

  EXPECT_TRUE(c_vec_fill_with_repeat(vec, (int[]){1, 2, 3}, 3));
  EXPECT_EQ(9U, c_vec_len(vec));
  EXPECT_EQ(10U, c_vec_capacity(vec));

  EXPECT_EQ(memcmp(vec->data, gt, vec_cap), 0);

  c_vec_destroy(vec);
}

UTEST(CVec, replace)
{
  int const gt[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  CVec*     vec  = c_vec_create_from_raw((int[]){1, 2, 0, 0, 5, 6, 7, 8, 9, 0}, 10,
                                         sizeof(int), true, NULL);
  EXPECT_TRUE(vec);

  EXPECT_TRUE(c_vec_replace(vec, 2, 2, (int[]){3, 4}, 2));
  EXPECT_EQ(0, memcmp(gt, vec->data, sizeof(gt)));

  c_vec_destroy(vec);
}

UTEST(CVec, replace_with_smaller_data)
{
  int const gt[] = {1, 2, 3, 5, 6, 7, 8, 9, 0};
  CVec*     vec  = c_vec_create_from_raw((int[]){1, 2, 0, 0, 5, 6, 7, 8, 9, 0}, 10,
                                         sizeof(int), true, NULL);
  EXPECT_TRUE(vec);

  EXPECT_TRUE(c_vec_replace(vec, 2, 2, (int[]){3}, 1));
  EXPECT_EQ(0, memcmp(gt, vec->data, sizeof(gt)));

  c_vec_destroy(vec);
}

UTEST(CVec, replace_with_larger_data)
{
  int const gt[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
  CVec*     vec  = c_vec_create_from_raw((int[]){1, 2, 0, 0, 7, 8, 9, 0}, 8,
                                         sizeof(int), true, NULL);
  EXPECT_TRUE(vec);

  EXPECT_TRUE(c_vec_replace(vec, 2, 2, (int[]){3, 4, 5, 6}, 4));
  EXPECT_EQ(0, memcmp(gt, vec->data, sizeof(gt)));

  c_vec_destroy(vec);
}

/******************************************************************************/

int cmp(void const* a, void const* b)
{
  return *(int*)a - *(int*)b;
}

int cmp_inv(void const* a, void const* b)
{
  return *(int*)b - *(int*)a;
}
