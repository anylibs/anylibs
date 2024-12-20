#include "anylibs/dl_loader.h"

#include <utest.h>

#include <stdio.h>
#include <stdlib.h>

UTEST(CDLLoader, general)
{
#ifdef _WIN32
  char const lib_path[] = ANYLIBS_C_TEST_ASSETS "assets/mylib.dll";
#else
  char const lib_path[] = ANYLIBS_C_TEST_ASSETS "assets/libmylib.so";
#endif
  CDLLoader* loader = c_dl_loader_create(CSTR(lib_path));
  EXPECT_TRUE(loader);

  int (*add)(int, int)
      = (int (*)(int, int))c_dl_loader_get(loader, CSTR("add"));
  EXPECT_TRUE(add);
  EXPECT_EQ(add(1, 2), 3);

  c_dl_loader_destroy(loader);
}
