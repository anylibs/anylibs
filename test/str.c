#include "anylibs/str.h"
#include "anylibs/vec.h"

#include <utest.h>

typedef struct CStringTest {
  CAllocator* allocator;
  CString*    str_ascii;
  CString*    str_utf8;
} CStringTest;

UTEST_F_SETUP(CStringTest)
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

UTEST_F_TEARDOWN(CStringTest)
{
  (void)utest_result;

  c_str_destroy(utest_fixture->str_ascii);
  c_str_destroy(utest_fixture->str_utf8);
}

UTEST_F(CStringTest, iter_next_ascii)
{
  char        gt[]       = "Open source :)";
  CStringIter iter_ascii = c_str_iter(NULL);

  CResultChar result;
  size_t      counter = 0;
  while (
      (result = c_str_iter_next(utest_fixture->str_ascii, &iter_ascii)).is_ok) {
    EXPECT_EQ(*result.ch.data, gt[counter]);
    EXPECT_EQ(result.ch.size, sizeof(char));
    counter++;
  }
}

UTEST_F(CStringTest, iter_rev_ascii)
{
  CStringIter iter_ascii = c_str_iter(NULL);
  c_str_iter_rev(utest_fixture->str_ascii, &iter_ascii);

  char gt[] = "Open source :)";

  CResultChar result;
  size_t      counter = sizeof(gt) - 2;
  while (
      (result = c_str_iter_next(utest_fixture->str_ascii, &iter_ascii)).is_ok) {
    EXPECT_EQ(*result.ch.data, gt[counter]);
    EXPECT_EQ(result.ch.size, sizeof(char));
    counter--;
  }
}

UTEST_F(CStringTest, iter_next_utf8)
{
  CStringIter iter_utf8 = c_str_iter(NULL);

  char*  gt[]       = {"م", "ف", "ت", "و", "ح", " ", "ا", "ل",
                       "م", "ص", "د", "ر", " ", ":", ")"};
  size_t gt_sizes[] = {2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1};

  CResultChar result;
  size_t      counter = 0;
  while (
      (result = c_str_iter_next(utest_fixture->str_utf8, &iter_utf8)).is_ok) {
    EXPECT_STRNEQ(result.ch.data, gt[counter], result.ch.size);
    EXPECT_EQ(result.ch.size, gt_sizes[counter]);
    counter++;
  }
}

UTEST_F(CStringTest, iter_rev_utf8)
{
  CStringIter iter_utf8 = c_str_iter(NULL);
  c_str_iter_rev(utest_fixture->str_utf8, &iter_utf8);

  char*  gt[]       = {"م", "ف", "ت", "و", "ح", " ", "ا", "ل",
                       "م", "ص", "د", "ر", " ", ":", ")"};
  size_t gt_size    = 15;
  size_t gt_sizes[] = {2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1};

  CResultChar result;
  size_t      counter = gt_size - 1;
  while (
      (result = c_str_iter_next(utest_fixture->str_utf8, &iter_utf8)).is_ok) {
    EXPECT_STRNEQ(result.ch.data, gt[counter], result.ch.size);
    EXPECT_EQ(result.ch.size, gt_sizes[counter]);
    counter--;
  }
}

UTEST_F(CStringTest, reverse_ascii)
{
  char gt[] = "): ecruos nepO";

  CString* reversed_str = c_str_reverse(utest_fixture->str_ascii);
  EXPECT_TRUE(reversed_str);
  EXPECT_STRNEQ(reversed_str->data, gt, sizeof(gt) - 1);

  c_str_destroy(reversed_str);
}

UTEST_F(CStringTest, reverse_utf8)
{
  char gt[] = "): ردصملا حوتفم";

  CString* reversed_str = c_str_reverse(utest_fixture->str_utf8);
  EXPECT_TRUE(reversed_str);
  EXPECT_STRNEQ(reversed_str->data, (gt), sizeof(gt) - 1);

  c_str_destroy(reversed_str);
}

UTEST_F(CStringTest, count)
{
  CResultSizeT count = c_str_count(utest_fixture->str_utf8);
  EXPECT_TRUE(count.is_ok);
  EXPECT_EQ(count.s, 15U);
}

UTEST_F(CStringTest, search)
{
  CResultStr result = c_str_find(utest_fixture->str_utf8, CCHAR("ح"));
  EXPECT_TRUE(result.is_ok);

  EXPECT_STREQ(result.str.data, &utest_fixture->str_utf8->data[8]);
}

UTEST_F(CStringTest, pop)
{
  CResultCharOwned result = {0};

  result = c_str_pop(utest_fixture->str_utf8);
  EXPECT_TRUE(result.is_ok);
  EXPECT_STREQ(")", result.ch.data);

  result = c_str_pop(utest_fixture->str_utf8);
  EXPECT_TRUE(result.is_ok);
  EXPECT_STREQ(":", result.ch.data);

  result = c_str_pop(utest_fixture->str_utf8);
  EXPECT_TRUE(result.is_ok);
  EXPECT_STREQ(" ", result.ch.data);

  result = c_str_pop(utest_fixture->str_utf8);
  EXPECT_TRUE(result.is_ok);
  EXPECT_STREQ("ر", result.ch.data);
}

UTEST(CString, trim_start)
{
  CAllocator* allocator = c_allocator_default();
  ASSERT_TRUE(allocator);

  CString* str
      = c_str_create_from_raw(CSTR("  اللغة العربية"), false, allocator);
  ASSERT_TRUE(str);

  CStr trimmed_str = c_str_trim_start(str);
  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(str);
}

UTEST(CString, trim_end)
{
  CAllocator* allocator = c_allocator_default();
  ASSERT_TRUE(allocator);

  CString* str
      = c_str_create_from_raw(CSTR("اللغة العربية  "), false, allocator);
  ASSERT_TRUE(str);

  CStr trimmed_str = c_str_trim_end(str);
  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(str);
}

UTEST(CString, trim)
{
  CAllocator* allocator = c_allocator_default();
  ASSERT_TRUE(allocator);

  CString* str
      = c_str_create_from_raw(CSTR("  اللغة العربية  "), false, allocator);
  ASSERT_TRUE(str);

  CStr trimmed_str = c_str_trim(str);
  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(str);
}

UTEST_F(CStringTest, split)
{
  CChar             delimeters[]   = {CCHAR(" ")};
  size_t            delimeters_len = sizeof(delimeters) / sizeof(*delimeters);
  size_t            counter        = 0;
  char const* const gt[]           = {"مفتوح", "المصدر", ":)"};

  CResultStr  result;
  CStringIter iter = c_str_iter(NULL);
  while ((result = c_str_split(utest_fixture->str_utf8, delimeters,
                               delimeters_len, &iter))
             .is_ok) {
    EXPECT_STRNEQ(gt[counter], result.str.data, result.str.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));
}

UTEST_F(CStringTest, split_invalid)
{
  CChar  delimeters[]   = {CCHAR(" ")};
  size_t delimeters_len = sizeof(delimeters) / sizeof(*delimeters);
  size_t counter        = 0;

  CStringIter iter        = c_str_iter(NULL);
  CResultChar last_result = c_str_iter_last(utest_fixture->str_utf8, &iter);
  EXPECT_TRUE(last_result.is_ok);

  CResultStr result;
  while ((result = c_str_split(utest_fixture->str_utf8, delimeters,
                               delimeters_len, &iter))
             .is_ok) {
    counter++;
  }
  EXPECT_EQ(counter, 0U);
}

UTEST_F(CStringTest, split_by_whitespace)
{
  size_t            counter = 0;
  char const* const gt[]    = {"مفتوح", "المصدر", ":)"};

  CStringIter iter = c_str_iter(NULL);
  CResultStr  result;
  while ((result = c_str_split_by_whitespace(utest_fixture->str_utf8, &iter))
             .is_ok) {
    EXPECT_STRNEQ(gt[counter], result.str.data, result.str.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));
}

UTEST_F(CStringTest, split_by_line)
{
  size_t            counter     = 0;
  char const* const gt[]        = {"مفتوح", "المصدر", ":)"};
  char              input_raw[] = "مفتوح\nالمصدر\r\n:)";

  CString* input
      = c_str_create_from_raw(CSTR(input_raw), true, utest_fixture->allocator);
  ASSERT_TRUE(input);

  CResultStr  result;
  CStringIter iter = c_str_iter(NULL);
  while ((result = c_str_split_by_line(input, &iter)).is_ok) {
    EXPECT_STRNEQ(gt[counter], result.str.data, result.str.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));

  c_str_destroy(input);
}

UTEST_F(CStringTest, ascii_uppercase)
{
  CString* str = c_str_create_from_raw(CSTR("open source :)"), true,
                                       utest_fixture->allocator);
  ASSERT_TRUE(str);

  c_str_to_ascii_uppercase(str);
  EXPECT_STREQ("OPEN SOURCE :)", str->data);

  c_str_destroy(str);
}

UTEST_F(CStringTest, ascii_lowercase)
{
  CString* str = c_str_create_from_raw(CSTR("OPEN SOURCE :)"), true,
                                       utest_fixture->allocator);
  ASSERT_TRUE(str);

  c_str_to_ascii_lowercase(str);
  EXPECT_STREQ("open source :)", str->data);

  c_str_destroy(str);
}

UTEST_F(CStringTest, utf16)
{
  uint16_t gt[]
      = {0x0645, 0x0641, 0x062a, 0x0648, 0x062d, 0x0020, 0x0627, 0x0644,
         0x0645, 0x0635, 0x062f, 0x0631, 0x0020, 0x003a, 0x029};
  CVec* u16_vec = c_str_to_utf16(utest_fixture->str_utf8);
  EXPECT_TRUE(u16_vec);

  CIter iter = c_vec_iter(u16_vec, NULL);

  CResultVoidPtr result;
  size_t         counter = 0;
  while (
      (result = c_iter_next(&iter, u16_vec->data, c_vec_len(u16_vec))).is_ok) {
    EXPECT_EQ(gt[counter], *(uint16_t*)result.vp);
    counter++;
  }

  CString* str = c_str_from_utf16(u16_vec);
  ASSERT_TRUE(str);

  EXPECT_EQ(c_str_compare(str, utest_fixture->str_utf8), 0);

  c_vec_destroy(u16_vec);
  c_str_destroy(str);
}

UTEST(CString, format)
{
  char     gt[] = "Name: SomeOne Junior, Age: 99, Height: 199CM";
  CString* str  = c_str_create_with_capacity(10, NULL, false);
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
// UTEST(CString, format_invalid)
// {
//   CString* str;
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
