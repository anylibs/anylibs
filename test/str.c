#include "anylibs/str.h"
#include "anylibs/vec.h"

#include <utest.h>

typedef struct CStrBufTest {
  CAllocator* allocator;
  CStrBuf*    str_ascii;
  CStrBuf*    str_utf8;
} CStrBufTest;

UTEST_F_SETUP(CStrBufTest)
{
  utest_fixture->allocator = c_allocator_default();
  ASSERT_TRUE(utest_fixture->allocator);

  utest_fixture->str_ascii = c_str_create_from_raw(CSTR("Open source :)"), true,
                                                   utest_fixture->allocator);
  ASSERT_TRUE(utest_fixture->str_ascii);

  utest_fixture->str_utf8 = c_str_create_from_raw(CSTR("مفتوح المصدر :)"), true,
                                                  utest_fixture->allocator);
  ASSERT_TRUE(utest_fixture->str_utf8);
}

UTEST_F_TEARDOWN(CStrBufTest)
{
  (void)utest_result;

  c_str_destroy(utest_fixture->str_ascii);
  c_str_destroy(utest_fixture->str_utf8);
}

UTEST_F(CStrBufTest, iter_next_ascii)
{
  char  gt[]       = "Open source :)";
  CIter iter_ascii = c_str_iter(utest_fixture->str_ascii);

  CStr   result;
  size_t counter = 0;
  while (c_str_iter_next(&iter_ascii, &result)) {
    EXPECT_EQ(gt[counter], *result.data);
    EXPECT_EQ(sizeof(char), result.len);
    counter++;
  }
}

UTEST_F(CStrBufTest, iter_rev_ascii)
{
  CIter iter_ascii = c_str_iter(utest_fixture->str_ascii);

  char gt[] = "Open source :)";

  CStr   result;
  size_t counter = sizeof(gt) - 2;
  while (c_str_iter_prev(&iter_ascii, &result)) {
    EXPECT_EQ(*result.data, gt[counter]);
    EXPECT_EQ(result.len, sizeof(char));
    counter--;
  }
}

UTEST_F(CStrBufTest, iter_next_utf8)
{
  CIter iter_utf8 = c_str_iter(utest_fixture->str_utf8);

  char*  gt[]       = {"م", "ف", "ت", "و", "ح", " ", "ا", "ل",
                       "م", "ص", "د", "ر", " ", ":", ")"};
  size_t gt_sizes[] = {2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1};

  CStr   result;
  size_t counter = 0;
  while (c_str_iter_next(&iter_utf8, &result)) {
    EXPECT_STRNEQ(result.data, gt[counter], result.len);
    EXPECT_EQ(result.len, gt_sizes[counter]);
    counter++;
  }
}

UTEST_F(CStrBufTest, iter_rev_utf8)
{
  CIter iter_utf8 = c_str_iter(utest_fixture->str_utf8);

  char*  gt[]       = {"م", "ف", "ت", "و", "ح", " ", "ا", "ل", "م", "ص", "د", "ر", " ", ":", ")"};
  size_t gt_sizes[] = {2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1};
  size_t gt_size    = 15;

  CStr   result;
  size_t counter = gt_size - 1;
  while (c_str_iter_prev(&iter_utf8, &result)) {
    EXPECT_STRNEQ(result.data, gt[counter], result.len);
    EXPECT_EQ(result.len, gt_sizes[counter]);
    counter--;
  }
}

UTEST_F(CStrBufTest, reverse_ascii)
{
  char gt[] = "): ecruos nepO";

  CStrBuf* reversed_str = c_str_reverse(utest_fixture->str_ascii);
  EXPECT_TRUE(reversed_str);
  EXPECT_STRNEQ(reversed_str->data, gt, sizeof(gt) - 1);

  c_str_destroy(reversed_str);
}

UTEST_F(CStrBufTest, reverse_utf8)
{
  char gt[] = "): ردصملا حوتفم";

  CStrBuf* reversed_str = c_str_reverse(utest_fixture->str_utf8);
  EXPECT_TRUE(reversed_str);
  EXPECT_STRNEQ(reversed_str->data, (gt), sizeof(gt) - 1);

  c_str_destroy(reversed_str);
}

UTEST_F(CStrBufTest, count)
{
  int count = c_str_count(utest_fixture->str_utf8);
  EXPECT_EQ(count, 15);
}

UTEST_F(CStrBufTest, search)
{
  CStr result;
  bool status = c_str_find(utest_fixture->str_utf8, CSTR("ح"), &result);
  EXPECT_TRUE(status);

  EXPECT_STREQ(result.data, &utest_fixture->str_utf8->data[8]);
}

UTEST_F(CStrBufTest, pop)
{
  CChar result = {0};

  EXPECT_TRUE(c_str_pop(utest_fixture->str_utf8, &result));
  EXPECT_STREQ(")", result.data);

  EXPECT_TRUE(c_str_pop(utest_fixture->str_utf8, &result));
  EXPECT_STREQ(":", result.data);

  EXPECT_TRUE(c_str_pop(utest_fixture->str_utf8, &result));
  EXPECT_STREQ(" ", result.data);

  EXPECT_TRUE(c_str_pop(utest_fixture->str_utf8, &result));
  EXPECT_STREQ("ر", result.data);
}

UTEST(CStrBuf, trim_start)
{
  CAllocator* allocator = c_allocator_default();
  ASSERT_TRUE(allocator);

  CStrBuf* str = c_str_create_from_raw(CSTR("  اللغة العربية"), false, allocator);
  ASSERT_TRUE(str);

  CStr trimmed_str = c_str_trim_start(str);
  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(str);
}

UTEST(CStrBuf, trim_end)
{
  CAllocator* allocator = c_allocator_default();
  ASSERT_TRUE(allocator);

  CStrBuf* str = c_str_create_from_raw(CSTR("اللغة العربية  "), false, allocator);
  ASSERT_TRUE(str);

  CStr trimmed_str = c_str_trim_end(str);
  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(str);
}

UTEST(CStrBuf, trim)
{
  CAllocator* allocator = c_allocator_default();
  ASSERT_TRUE(allocator);

  CStrBuf* str = c_str_create_from_raw(CSTR("  اللغة العربية  "), false, allocator);
  ASSERT_TRUE(str);

  CStr trimmed_str = c_str_trim(str);
  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(str);
}

UTEST_F(CStrBufTest, split)
{
  CChar             delimeters[]   = {(CChar){" ", 1}};
  size_t            delimeters_len = sizeof(delimeters) / sizeof(*delimeters);
  size_t            counter        = 0;
  char const* const gt[]           = {"مفتوح", "المصدر", ":)"};

  CStr  result;
  CIter iter = c_str_iter(utest_fixture->str_utf8);
  while (c_str_iter_split(&iter, delimeters, delimeters_len, &result)) {
    EXPECT_STRNEQ(gt[counter], result.data, result.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));
}

UTEST_F(CStrBufTest, split_invalid)
{
  CChar  delimeters[]   = {(CChar){" ", 1}};
  size_t delimeters_len = sizeof(delimeters) / sizeof(*delimeters);
  size_t counter        = 0;

  CIter iter        = c_str_iter(utest_fixture->str_utf8);
  CStr  last_result = c_str_iter_last(&iter);
  EXPECT_STREQ(")", last_result.data);

  CStr result;
  while (c_str_iter_split(&iter, delimeters, delimeters_len, &result)) {
    counter++;
  }
  EXPECT_EQ(counter, 0U);
}

UTEST_F(CStrBufTest, split_by_whitespace)
{
  size_t            counter = 0;
  char const* const gt[]    = {"مفتوح", "المصدر", ":)"};

  CIter iter = c_str_iter(utest_fixture->str_utf8);
  CStr  result;
  while (c_str_iter_split_by_whitespace(&iter, &result)) {
    EXPECT_STRNEQ(gt[counter], result.data, result.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));
}

UTEST_F(CStrBufTest, split_by_line)
{
  size_t            counter     = 0;
  char const* const gt[]        = {"مفتوح", "المصدر", ":)"};
  char              input_raw[] = "مفتوح\nالمصدر\r\n:)";

  CStrBuf* input = c_str_create_from_raw(CSTR(input_raw), true, utest_fixture->allocator);
  ASSERT_TRUE(input);

  CStr  result;
  CIter iter = c_str_iter(input);
  while (c_str_iter_split_by_line(&iter, &result)) {
    EXPECT_STRNEQ(gt[counter], result.data, result.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));

  c_str_destroy(input);
}

UTEST_F(CStrBufTest, ascii_uppercase)
{
  CStrBuf* str = c_str_create_from_raw(CSTR("open source :)"), true, utest_fixture->allocator);
  ASSERT_TRUE(str);

  c_str_to_ascii_uppercase(str);
  EXPECT_STREQ("OPEN SOURCE :)", str->data);

  c_str_destroy(str);
}

UTEST_F(CStrBufTest, ascii_lowercase)
{
  CStrBuf* str = c_str_create_from_raw(CSTR("OPEN SOURCE :)"), true, utest_fixture->allocator);
  ASSERT_TRUE(str);

  c_str_to_ascii_lowercase(str);
  EXPECT_STREQ("open source :)", str->data);

  c_str_destroy(str);
}

UTEST_F(CStrBufTest, utf16)
{
  uint16_t gt[]    = {0x0645, 0x0641, 0x062a, 0x0648, 0x062d, 0x0020, 0x0627, 0x0644, 0x0645, 0x0635, 0x062f, 0x0631, 0x0020, 0x003a, 0x029};
  CVec*    u16_vec = c_str_to_utf16(utest_fixture->str_utf8);
  EXPECT_TRUE(u16_vec);

  CIter iter = c_vec_iter(u16_vec);

  void*  result;
  size_t counter = 0;
  while (c_iter_next(&iter, &result)) {
    EXPECT_EQ(gt[counter], *(uint16_t*)result);
    counter++;
  }

  CStrBuf* str = c_str_from_utf16(u16_vec);
  ASSERT_TRUE(str);

  EXPECT_EQ(c_str_compare(str, utest_fixture->str_utf8), 0);

  c_vec_destroy(u16_vec);
  c_str_destroy(str);
}

UTEST(CStrBuf, format)
{
  char     gt[] = "Name: SomeOne Junior, Age: 99, Height: 199CM";
  CStrBuf* str  = c_str_create_with_capacity(10, NULL, false);
  ASSERT_TRUE(str);

  char const format[] = "Name: %s, Age: %u, Height: %uCM";
  bool       status   = c_str_format(str, 0, sizeof(format) - 1, format,
                                     "SomeOne Junior", 99U, 199U);
  EXPECT_TRUE(status);

  EXPECT_STRNEQ(gt, str->data, c_str_len(str));

  c_str_destroy(str);
}

/// this test case will not even compile (at least on linux)
/// and this is sooooo good
// UTEST(CStrBuf, format_invalid)
// {
//   CStrBuf* str;
//   int      err = c_str_create_with_capacity(10, NULL, false, &str);
//   ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
//   char gt[] = "Name: SomeOne Junior, Age: 99, Height: 199CM";

//   char const format[] = "Name: %s, Age: %u, Height: %uCM";
//   err = c_str_format(str, 0, sizeof(format) - 1, format, "SomeOne Junior",
//   99U); ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

//   size_t str_len;
//   c_str_len(str, &str_len);
//   EXPECT_STRNEQ(gt, str->data, str_len);

//   c_str_destroy(str);
// }
