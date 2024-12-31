#include "anylibs/iter.h"

#include <utest.h>

static int    arr[]   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
static size_t arr_len = sizeof(arr) / sizeof(*arr);

UTEST(CIter, next)
{
  CIter iter = c_iter(arr, sizeof(arr), sizeof(*arr));

  void*  data;
  size_t counter = 0;
  while (c_iter_next(&iter, &data)) {
    EXPECT_EQ(arr[counter++], *(int*)data);
  }
}

UTEST(CIter, prev)
{
  CIter iter = c_iter(arr, sizeof(arr), sizeof(*arr));

  void*  data;
  size_t counter = arr_len;
  while (c_iter_prev(&iter, &data)) {
    EXPECT_EQ(arr[--counter], *(int*)data);
  }
}

UTEST(CIter, nth)
{
  CIter iter = c_iter(arr, sizeof(arr), sizeof(*arr));

  void* data;
  EXPECT_TRUE(c_iter_nth(&iter, 3, &data));
  EXPECT_EQ(4, *(int*)data);
}

UTEST(CIter, peek)
{
  CIter iter = c_iter(arr, sizeof(arr), sizeof(*arr));

  void* data;
  EXPECT_TRUE(c_iter_nth(&iter, 3, &data));
  EXPECT_TRUE(data);

  EXPECT_TRUE(c_iter_peek(&iter, &data));
  EXPECT_EQ(5, *(int*)data);
}

UTEST(CIter, first_last)
{
  CIter iter = c_iter(arr, sizeof(arr), sizeof(*arr));

  void* data = c_iter_first(&iter);
  EXPECT_TRUE(data);
  EXPECT_EQ(1, *(int*)data);

  data = c_iter_last(&iter);
  EXPECT_TRUE(data);
  EXPECT_EQ(0, *(int*)data);
}

UTEST(CIter, peek_beyond_last)
{
  CIter iter = c_iter(arr, sizeof(arr), sizeof(*arr));

  void* data = c_iter_last(&iter);
  EXPECT_TRUE(data);
  EXPECT_EQ(*(int*)data, 0);

  EXPECT_FALSE(c_iter_peek(&iter, &data));
}
