#include <stdio.h>

#include "anylibs/map.h"

#include <utest.h>

typedef struct CMapTest {
  CMap* map;
} CMapTest;

UTEST_F_SETUP(CMapTest)
{
  utest_fixture->map = c_map_create(sizeof(char[20]), sizeof(int), NULL);
  ASSERT_TRUE(utest_fixture->map);

  bool status = c_map_insert(utest_fixture->map, (char[20]){"abc"}, &(int){1});
  EXPECT_TRUE(status);
  status = c_map_insert(utest_fixture->map, (char[20]){"ahmed here"}, &(int){2});
  EXPECT_TRUE(status);
  status = c_map_insert(utest_fixture->map, (char[20]){"abcd"}, &(int){3});
  EXPECT_TRUE(status);
  status = c_map_insert(utest_fixture->map, (char[20]){"abc"}, &(int){4}); // test override
  EXPECT_TRUE(status);
}

UTEST_F_TEARDOWN(CMapTest)
{
  (void)utest_result;
  c_map_destroy(utest_fixture->map, NULL, NULL);
}

UTEST_F(CMapTest, get)
{
  bool status;
  {
    int* value = NULL;

    status = c_map_get(utest_fixture->map, (char[20]){"abc"}, (void**)&value);
    EXPECT_TRUE(status);
    EXPECT_TRUE(value && *value == 4);

    status = c_map_get(utest_fixture->map, (char[20]){"abcd"}, (void**)&value);
    EXPECT_TRUE(status);
    EXPECT_TRUE(value && *value == 3);

    status = c_map_get(utest_fixture->map, (char[20]){"xyz"}, (void**)&value); // test not exist
    EXPECT_TRUE(status);
    EXPECT_TRUE(!value);
  }
}

UTEST_F(CMapTest, foreach)
{
  char* gt[] = {"", "abc", "ahmed here", "abcd", "abc"};

  CMapIter iter  = c_map_iter(utest_fixture->map);
  char*    key   = NULL;
  int*     value = NULL;
  while (c_map_iter_next(&iter, (void**)&key, (void**)&value)) {
    EXPECT_STREQ(gt[*value], key);
  }
}

// test: remove
UTEST_F(CMapTest, remove)
{
  bool status = c_map_insert(utest_fixture->map, (char[20]){"new bucket"}, &(int){100});
  EXPECT_TRUE(status);

  int* value;
  status = c_map_remove(utest_fixture->map, (char[20]){"new bucket"}, (void**)&value);
  EXPECT_TRUE(status);
  EXPECT_TRUE(*value == 100);
}

void c_map_handler(void* key, void* value, void* extra_data)
{
  (void)key;
  *(int*)extra_data += *(int*)value;
}
