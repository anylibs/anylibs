#include "anylibs/dl_loader.h"

#include <utest.h>

#include <stdio.h>
#include <stdlib.h>

#define STR(s) (s), sizeof(s) - 1

UTEST(CDLLoader, general)
{
  c_error_t err = C_ERROR_none;
  (void)err;

#ifdef _WIN32
  char const lib_path[] = ANYLIBS_C_TEST_ASSETS "assets/mylib.dll";
#else
  char const lib_path[] = ANYLIBS_C_TEST_ASSETS "assets/libmylib.so";
#endif
  CDLLoader* loader;
  err = c_dl_loader_create(STR(lib_path), &loader);
  EXPECT_EQ_MSG(err, 0U, c_error_to_str(err));

  int (*add)(int, int) = NULL;

  err = c_dl_loader_get(loader, STR("add"), (void**)&add);
  EXPECT_EQ_MSG(err, 0U, c_error_to_str(err));
  EXPECT_EQ(add(1, 2), 3);

  c_dl_loader_destroy(&loader);
}
