#include "anylibs/defer.h"

#include <utest.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct S {
  size_t len;
  int*   data;
} S;

void s_free(S* self)
{
  self->len = 0;
  free(self->data);
  *self = (S){0};
}

UTEST(CDefer, general)
{
  int error_code = 0;

  c_defer_init(10);

  int* arr1 = calloc(10, sizeof(int));
  c_defer(free, arr1);

  S s    = {0};
  s.data = calloc(10, sizeof(*s.data));
  c_defer_err(s.data, s_free, &s, NULL);

  int err = 10;
  c_defer_check(err == 10, NULL);

  int* arr3 = calloc(10, sizeof(int));
  c_defer(free, arr3);
  c_defer_err(false, NULL, NULL, error_code = -1);

  c_defer_deinit();

  EXPECT_EQ(error_code, -1);
}
