#include "anylibs/str.h"
#include "anylibs/vec.h"

#include <utest.h>

#define STR(str) (str), sizeof(str) - 1

typedef struct CStringTest {
  CAllocator* allocator;
  CString*    str_ascii;
  CString*    str_utf8;
} CStringTest;

UTEST_F_SETUP(CStringTest)
{
  int err = c_allocator_default(&utest_fixture->allocator);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  err = c_str_create_from_raw(STR("Open source :)"), utest_fixture->allocator,
                              false, &utest_fixture->str_ascii);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_str_create_from_raw(STR("مفتوح المصدر :)"), utest_fixture->allocator,
                              false, &utest_fixture->str_utf8);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
}

UTEST_F_TEARDOWN(CStringTest)
{
  (void)utest_result;

  c_str_destroy(&utest_fixture->str_ascii);
  c_str_destroy(&utest_fixture->str_utf8);
}

UTEST_F(CStringTest, iter_next_ascii)
{
  CStringIter iter_ascii;
  c_str_iter(NULL, &iter_ascii);

  char  gt[] = "Open source :)";
  CChar ch;

  size_t counter = 0;
  while (c_str_iter_next(utest_fixture->str_ascii, &iter_ascii, &ch)) {
    EXPECT_EQ(*ch.data, gt[counter]);
    EXPECT_EQ(ch.size, sizeof(char));
    counter++;
  }
}

UTEST_F(CStringTest, iter_rev_ascii)
{
  CStringIter iter_ascii;
  c_str_iter(NULL, &iter_ascii);
  c_str_iter_rev(utest_fixture->str_ascii, &iter_ascii);

  char  gt[] = "Open source :)";
  CChar ch;

  size_t counter = sizeof(gt) - 2;
  while (c_str_iter_next(utest_fixture->str_ascii, &iter_ascii, &ch)) {
    EXPECT_EQ(*ch.data, gt[counter]);
    EXPECT_EQ(ch.size, sizeof(char));
    counter--;
  }
}

UTEST_F(CStringTest, iter_next_utf8)
{
  CStringIter iter_utf8;
  c_str_iter(NULL, &iter_utf8);

  char*  gt[]       = {"م", "ف", "ت", "و", "ح", " ", "ا", "ل",
                       "م", "ص", "د", "ر", " ", ":", ")"};
  size_t gt_sizes[] = {2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1};
  CChar  ch;

  size_t counter = 0;
  while (c_str_iter_next(utest_fixture->str_utf8, &iter_utf8, &ch)) {
    EXPECT_STRNEQ(ch.data, gt[counter], ch.size);
    EXPECT_EQ(ch.size, gt_sizes[counter]);
    counter++;
  }
}

UTEST_F(CStringTest, iter_rev_utf8)
{
  CStringIter iter_utf8;
  c_str_iter(NULL, &iter_utf8);
  c_str_iter_rev(utest_fixture->str_utf8, &iter_utf8);

  char*  gt[]       = {"م", "ف", "ت", "و", "ح", " ", "ا", "ل",
                       "م", "ص", "د", "ر", " ", ":", ")"};
  size_t gt_size    = 15;
  size_t gt_sizes[] = {2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1};
  CChar  ch;

  size_t counter = gt_size - 1;
  while (c_str_iter_next(utest_fixture->str_utf8, &iter_utf8, &ch)) {
    EXPECT_STRNEQ(ch.data, gt[counter], ch.size);
    EXPECT_EQ(ch.size, gt_sizes[counter]);
    counter--;
  }
}

UTEST_F(CStringTest, reverse_ascii)
{
  char gt[] = "): ecruos nepO";

  CString* reversed_str;
  int      err = c_str_reverse(utest_fixture->str_ascii, &reversed_str);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_STRNEQ(reversed_str->data, gt, sizeof(gt) - 1);

  c_str_destroy(&reversed_str);
}

UTEST_F(CStringTest, reverse_utf8)
{
  char gt[] = "): ردصملا حوتفم";

  CString* reversed_str;
  int      err = c_str_reverse(utest_fixture->str_utf8, &reversed_str);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_STRNEQ(reversed_str->data, (gt), sizeof(gt) - 1);

  c_str_destroy(&reversed_str);
}

UTEST_F(CStringTest, count)
{
  size_t count;
  int    err = c_str_count(utest_fixture->str_utf8, &count);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(count, 15U);
}

UTEST_F(CStringTest, search)
{
  size_t index;
  int    err = c_str_find(utest_fixture->str_utf8, (CChar){STR("ح")}, &index);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_EQ(index, 4U);
}

UTEST_F(CStringTest, pop)
{
  CCharOwned ch = {0};

  int err = c_str_pop(utest_fixture->str_utf8, &ch);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_STREQ(")", ch.data);

  err = c_str_pop(utest_fixture->str_utf8, &ch);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_STREQ(":", ch.data);

  err = c_str_pop(utest_fixture->str_utf8, &ch);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_STREQ(" ", ch.data);

  err = c_str_pop(utest_fixture->str_utf8, &ch);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  EXPECT_STREQ("ر", ch.data);
}

UTEST(CString, trim_start)
{
  CAllocator* allocator;
  int         err = c_allocator_default(&allocator);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  CString* str;
  err = c_str_create_from_raw(STR("  اللغة العربية"), false, allocator, &str);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  CStr trimmed_str;
  err = c_str_trim_start(str, &trimmed_str);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(&str);
}

UTEST(CString, trim_end)
{
  CAllocator* allocator;
  int         err = c_allocator_default(&allocator);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  CString* str;
  err = c_str_create_from_raw(STR("اللغة العربية  "), false, allocator, &str);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  CStr trimmed_str;
  err = c_str_trim_end(str, &trimmed_str);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(&str);
}

UTEST(CString, trim)
{
  CAllocator* allocator;
  int         err = c_allocator_default(&allocator);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  CString* str;
  err = c_str_create_from_raw(STR("  اللغة العربية  "), false, allocator, &str);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));
  CStr trimmed_str;
  err = c_str_trim(str, &trimmed_str);
  EXPECT_EQ_MSG(err, 0, c_error_to_str(err));

  EXPECT_STRNEQ("اللغة العربية", trimmed_str.data, trimmed_str.len);

  c_str_destroy(&str);
}

UTEST_F(CStringTest, split)
{
  CChar             delimeters[]   = {(CChar){STR(" ")}};
  size_t            delimeters_len = sizeof(delimeters) / sizeof(*delimeters);
  CStringIter       iter;
  CStr              out_slice;
  size_t            counter = 0;
  char const* const gt[]    = {"مفتوح", "المصدر", ":)"};

  c_str_iter(NULL, &iter);
  while (c_str_split(utest_fixture->str_utf8, delimeters, delimeters_len, &iter,
                     &out_slice)) {
    EXPECT_STRNEQ(gt[counter], out_slice.data, out_slice.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));
}

UTEST_F(CStringTest, split_invalid)
{
  CChar       delimeters[]   = {(CChar){STR(" ")}};
  size_t      delimeters_len = sizeof(delimeters) / sizeof(*delimeters);
  CStringIter iter;
  CStr        out_slice;
  size_t      counter = 0;

  c_str_iter(NULL, &iter);
  c_str_iter_last(utest_fixture->str_utf8, &iter, NULL);
  while (c_str_split(utest_fixture->str_utf8, delimeters, delimeters_len, &iter,
                     &out_slice)) {
    counter++;
  }
  EXPECT_EQ(counter, 0U);
}

UTEST_F(CStringTest, split_by_whitespace)
{
  CStringIter       iter;
  CStr              out_slice;
  size_t            counter = 0;
  char const* const gt[]    = {"مفتوح", "المصدر", ":)"};

  c_str_iter(NULL, &iter);
  while (
      c_str_split_by_whitespace(utest_fixture->str_utf8, &iter, &out_slice)) {
    EXPECT_STRNEQ(gt[counter], out_slice.data, out_slice.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));
}

UTEST_F(CStringTest, split_by_line)
{
  CStringIter       iter;
  CStr              out_slice;
  size_t            counter     = 0;
  char const* const gt[]        = {"مفتوح", "المصدر", ":)"};
  char              input_raw[] = "مفتوح\nالمصدر\r\n:)";
  CString*          input;

  int err = c_str_create_from_raw(STR(input_raw), true,
                                  utest_fixture->allocator, &input);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  c_str_iter(NULL, &iter);
  while (c_str_split_by_line(input, &iter, &out_slice)) {
    EXPECT_STRNEQ(gt[counter], out_slice.data, out_slice.len);
    counter++;
  }
  EXPECT_EQ(counter, sizeof(gt) / sizeof(*gt));

  c_str_destroy(&input);
}

UTEST_F(CStringTest, ascii_uppercase)
{
  CString* str;
  int      err = c_str_create_from_raw(STR("open source :)"), true,
                                       utest_fixture->allocator, &str);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  c_str_to_ascii_uppercase(str);

  EXPECT_STREQ("OPEN SOURCE :)", str->data);

  c_str_destroy(&str);
}

UTEST_F(CStringTest, ascii_lowercase)
{
  CString* str;
  int      err = c_str_create_from_raw(STR("OPEN SOURCE :)"), true,
                                       utest_fixture->allocator, &str);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  c_str_to_ascii_lowercase(str);

  EXPECT_STREQ("open source :)", str->data);

  c_str_destroy(&str);
}

UTEST_F(CStringTest, utf16)
{
  CVec* u16_vec;
  int   err = c_vec_create(sizeof(uint16_t), NULL, &u16_vec);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
  err = c_str_to_utf16(utest_fixture->str_utf8, u16_vec);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
  uint16_t gt[]
      = {0x0645, 0x0641, 0x062a, 0x0648, 0x062d, 0x0020, 0x0627, 0x0644,
         0x0645, 0x0635, 0x062f, 0x0631, 0x0020, 0x003a, 0x029};

  CIter iter;
  c_vec_iter(u16_vec, NULL, &iter);

  uint16_t* element;
  size_t    counter = 0;
  size_t    vec_len;
  c_vec_len(u16_vec, &vec_len);
  while (c_iter_next(&iter, u16_vec->data, vec_len, (void**)&element)) {
    EXPECT_EQ(gt[counter], *element);
    counter++;
  }

  CString* str;
  err = c_str_create(NULL, &str);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
  c_str_from_utf16(u16_vec, str);

  EXPECT_EQ(c_str_compare(str, utest_fixture->str_utf8), 0);

  c_vec_destroy(&u16_vec);
  c_str_destroy(&str);
}

UTEST(CString, format)
{
  CString* str;
  int      err = c_str_create_with_capacity(10, NULL, false, &str);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));
  char gt[] = "Name: SomeOne Junior, Age: 99, Height: 199CM";

  char const format[] = "Name: %s, Age: %u, Height: %uCM";
  err = c_str_format(str, 0, sizeof(format) - 1, format, "SomeOne Junior", 99U,
                     199U);
  ASSERT_EQ_MSG(err, 0, c_error_to_str(err));

  size_t str_len;
  c_str_len(str, &str_len);
  EXPECT_STRNEQ(gt, str->data, str_len);

  c_str_destroy(&str);
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

//   c_str_destroy(&str);
// }
