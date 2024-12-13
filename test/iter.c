/**
 * @file iter.c
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

#include "anylibs/iter.h"

#include <utest.h>

static int    arr[]   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
static size_t arr_len = sizeof(arr) / sizeof(*arr);

UTEST(CIter, next)
{
  CIter iter;
  c_iter_create(sizeof(*arr), NULL, &iter);

  int*   data;
  size_t counter = 0;
  while (c_iter_next(&iter, arr, arr_len, (void**)&data)) {
    EXPECT_EQ(arr[counter++], *data);
  }
}

UTEST(CIter, prev)
{
  CIter iter;
  c_iter_create(sizeof(*arr), NULL, &iter);
  c_iter_rev(&iter, arr, arr_len);

  int*   data;
  size_t counter = arr_len;
  while (c_iter_next(&iter, arr, arr_len, (void**)&data)) {
    EXPECT_EQ(arr[--counter], *data);
  }
}

UTEST(CIter, nth)
{
  CIter iter;
  c_iter_create(sizeof(*arr), NULL, &iter);

  int* data;
  int  err = c_iter_nth(&iter, 3, arr, arr_len, (void**)&data);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(4, *data);
}

UTEST(CIter, peek)
{
  CIter iter;
  c_iter_create(sizeof(*arr), NULL, &iter);

  int* data;
  int  err = c_iter_nth(&iter, 3, arr, arr_len, (void**)&data);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_iter_peek(&iter, arr, arr_len, (void**)&data);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_EQ(5, *data);
}

UTEST(CIter, first_last)
{
  CIter iter;
  c_iter_create(sizeof(*arr), NULL, &iter);

  int* data;
  c_iter_first(&iter, arr, arr_len, (void**)&data);
  EXPECT_EQ(1, *data);
  c_iter_last(&iter, arr, arr_len, (void**)&data);
  EXPECT_EQ(0, *data);
}

UTEST(CIter, peek_beyond_last)
{
  CIter iter;
  c_iter_create(sizeof(*arr), NULL, &iter);

  int* data;
  c_iter_last(&iter, arr, arr_len, (void**)&data);

  int err = c_iter_peek(&iter, arr, arr_len, (void**)&data);
  EXPECT_EQ(err, C_ERROR_wrong_index);
}
