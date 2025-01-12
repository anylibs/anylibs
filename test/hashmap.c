#include <stdio.h>

#include "anylibs/hashmap.h"

#include <utest.h>

typedef struct CHashMapTest {
  CHashMap* map;
} CHashMapTest;

UTEST_F_SETUP(CHashMapTest)
{
  utest_fixture->map = c_hashmap_create(sizeof(char[20]), sizeof(int), NULL);
  ASSERT_TRUE(utest_fixture->map);

  bool status = c_hashmap_insert(utest_fixture->map, (char[20]){"abc"}, &(int){1});
  EXPECT_TRUE(status);
  status = c_hashmap_insert(utest_fixture->map, (char[20]){"ahmed here"}, &(int){2});
  EXPECT_TRUE(status);
  status = c_hashmap_insert(utest_fixture->map, (char[20]){"abcd"}, &(int){3});
  EXPECT_TRUE(status);
  status = c_hashmap_insert(utest_fixture->map, (char[20]){"abc"}, &(int){4}); // test override
  EXPECT_TRUE(status);
}

UTEST_F_TEARDOWN(CHashMapTest)
{
  (void)utest_result;
  c_hashmap_destroy(utest_fixture->map, NULL, NULL);
}

UTEST_F(CHashMapTest, get)
{
  bool status;
  {
    int* value = NULL;

    status = c_hashmap_get(utest_fixture->map, (char[20]){"abc"}, (void**)&value);
    EXPECT_TRUE(status);
    EXPECT_TRUE(value && *value == 4);

    status = c_hashmap_get(utest_fixture->map, (char[20]){"abcd"}, (void**)&value);
    EXPECT_TRUE(status);
    EXPECT_TRUE(value && *value == 3);

    status = c_hashmap_get(utest_fixture->map, (char[20]){"xyz"}, (void**)&value); // test not exist
    EXPECT_TRUE(status);
    EXPECT_TRUE(!value);
  }
}

UTEST_F(CHashMapTest, foreach)
{
  char* gt[] = {"", "abc", "ahmed here", "abcd", "abc"};

  CHashMapIter iter  = c_hashmap_iter(utest_fixture->map);
  char*        key   = NULL;
  int*         value = NULL;
  while (c_hashmap_iter_next(&iter, (void**)&key, (void**)&value)) {
    EXPECT_STREQ(gt[*value], key);
  }
}

// test: remove
UTEST_F(CHashMapTest, remove)
{
  bool status = c_hashmap_insert(utest_fixture->map, (char[20]){"new bucket"}, &(int){100});
  EXPECT_TRUE(status);

  int* value;
  status = c_hashmap_remove(utest_fixture->map, (char[20]){"new bucket"}, (void**)&value);
  EXPECT_TRUE(status);
  EXPECT_TRUE(*value == 100);
}

void c_hashmap_handler(void* key, void* value, void* extra_data)
{
  (void)key;
  *(int*)extra_data += *(int*)value;
}
