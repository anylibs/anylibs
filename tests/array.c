#include "anylibs/array.h"

#include <stdio.h>

#include <utest.h>

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
    int err = c_array_pop(utest_fixture, &data);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(data, 16);
}

UTEST_F(CArray, remove_range)
{
    int err = c_array_remove_range(utest_fixture, 1U, 3U);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(c_array_len(utest_fixture), 2U);
    EXPECT_EQ(((int *)utest_fixture->data)[0], 12);
}

UTEST_F(CArray, insert)
{
    int err = c_array_insert(utest_fixture, &(int){20}, 0);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(((int *)utest_fixture->data)[0], 20);
    EXPECT_EQ(((int *)utest_fixture->data)[1], 12);
}

UTEST_F(CArray, insert_range)
{
    int err = c_array_insert_range(utest_fixture, 1, &(int[]){1, 2, 3}, 3);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(c_array_len(utest_fixture), 8U);

    EXPECT_EQ(((int *)utest_fixture->data)[0], 12);
    EXPECT_EQ(((int *)utest_fixture->data)[1], 1);
    EXPECT_EQ(((int *)utest_fixture->data)[2], 2);
    EXPECT_EQ(((int *)utest_fixture->data)[3], 3);
    EXPECT_EQ(((int *)utest_fixture->data)[4], 13);
}

UTEST(CArray, general)
{
    CArray array2;
    int err = c_array_init(sizeof(char), &array2);
    EXPECT_EQ(err, 0);

    err = c_array_push(&array2, &(char){'\0'});
    EXPECT_EQ(err, 0);
    err = c_array_insert(&array2, &(char){'a'}, 0);
    EXPECT_EQ(err, 0);
    EXPECT_EQ(((char *)array2.data)[0], 'a');
    EXPECT_EQ(((char *)array2.data)[1], '\0');

    c_array_deinit(&array2);
}

UTEST(CArray, wrong_index)
{
    CArray array;
    int err = c_array_init(sizeof(char), &array);
    EXPECT_EQ(err, 0);

    err = c_array_push(&array, &(char){'\0'});
    EXPECT_EQ(err, 0);
    err = c_array_insert(&array, &(char){'a'}, 1);
    EXPECT_EQ(err, C_ERROR_wrong_index);

    c_array_deinit(&array);
}
