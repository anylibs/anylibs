#include "anylibs/fs.h"
#include "anylibs/error.h"
#include "anylibs/str.h"

#include <stdbool.h>
#include <time.h>
#include <utest.h>

UTEST(CFsIter, iter) {
  CStrBuf *path =
      c_str_create_from_raw(CSTR(ANYLIBS_C_TEST_PLAYGROUND), true, NULL);
  ASSERT_TRUE_MSG(path, c_error_to_str(c_error_get()));

  CFsIter iter = c_fs_iter(path);
  CStr result;
  bool has_at_least_one_file = false;
  while (c_fs_iter_next(&iter, &result)) {
    has_at_least_one_file = true;

    size_t f_size = 0;
    if (c_fs_is_dir(result) != 0) {
      CFile *f = c_fs_file_open(result, CSTR("r"));
      EXPECT_TRUE_MSG(f, c_error_to_str(c_error_get()));
      EXPECT_TRUE_MSG(c_fs_file_size(f, &f_size),
                      c_error_to_str(c_error_get()));
    }

    // printf("%s:%zuKB: %s\n",
    //        c_fs_is_dir(result) ? "D" : "F",
    //        f_size / 1024,
    //        path->data);
  }
  EXPECT_TRUE(has_at_least_one_file);

  c_str_destroy(path);
  c_fs_iter_close(&iter);
}

UTEST(CFile, file) {
  // write
  {
    CFile *f =
        c_fs_file_open(CSTR(ANYLIBS_C_TEST_PLAYGROUND "/file"), CSTR("w"));
    ASSERT_TRUE(f);

    EXPECT_TRUE(c_fs_file_write(f, CSTR("Om Kulthuom\n"), NULL));
    EXPECT_TRUE(c_fs_file_flush(f));
    EXPECT_TRUE(c_fs_file_close(f));
  }

  // read & prop
  {
    CFile *f =
        c_fs_file_open(CSTR(ANYLIBS_C_TEST_PLAYGROUND "/file"), CSTR("r"));
    ASSERT_TRUE(f);
    CStrBuf *buf = c_str_create_with_capacity(100 * sizeof(char), NULL, false);
    EXPECT_TRUE(buf);
    EXPECT_TRUE(c_fs_file_read(f, buf, NULL));
    EXPECT_STREQ("Om Kulthuom\n", buf->data);

    size_t f_size;
    EXPECT_TRUE(c_fs_file_size(f, &f_size));
    EXPECT_EQ(sizeof("Om Kulthuom\n") - 1, f_size);

    EXPECT_TRUE(c_fs_file_close(f));
    c_str_destroy(buf);
  }

  // delete
  { EXPECT_TRUE(c_fs_delete(CSTR(ANYLIBS_C_TEST_PLAYGROUND "/file"))); }
}

UTEST(CPath, general) {
  CStrBuf *path =
      c_str_create_from_raw(CSTR(ANYLIBS_C_TEST_PLAYGROUND), true, NULL);
  ASSERT_TRUE(path);

  // absolute
  CStrBuf *abs_path = c_fs_path_to_absolute(CSTR("."), NULL);
  EXPECT_TRUE(abs_path);
  EXPECT_TRUE(c_fs_path_is_absolute(c_cstrbuf_to_cstr(abs_path)));
  c_str_destroy(abs_path);

  EXPECT_TRUE(c_fs_path_append(path, CSTR("folder/file")));
  EXPECT_TRUE(c_str_ends_with(path, CSTR("folder/file")) == 0);

  c_str_destroy(path);
}

UTEST(CDir, create_delete) {
  CStr dir = CSTR(ANYLIBS_C_TEST_PLAYGROUND "/folder");
  ASSERT_TRUE(c_fs_exists(dir) != 0);
  ASSERT_TRUE(c_fs_dir_create(dir));
  EXPECT_TRUE(c_fs_dir_is_empty(dir) == 0);
  EXPECT_TRUE(c_fs_delete(dir));
}

UTEST(CDir, current_dir) {
  CStrBuf *cur_dir = c_fs_dir_current(NULL);
  EXPECT_TRUE(cur_dir);
  EXPECT_TRUE(c_str_len(cur_dir) > 0);
  EXPECT_TRUE(c_fs_is_dir(c_cstrbuf_to_cstr(cur_dir)) == 0);

  EXPECT_TRUE(c_fs_path_append(cur_dir, CSTR("..")));
  EXPECT_TRUE(c_fs_dir_change_current(c_cstrbuf_to_cstr(cur_dir)));

  c_str_destroy(cur_dir);
}

UTEST(CDir, current_exe_dir) {
  CStrBuf *cur_exe_dir = c_fs_dir_current_exe(NULL);
  EXPECT_TRUE(cur_exe_dir);
  EXPECT_TRUE(c_str_len(cur_exe_dir) > 0);
  EXPECT_FALSE(c_fs_is_dir(c_cstrbuf_to_cstr(cur_exe_dir)) == 0);
  c_str_destroy(cur_exe_dir);
}

UTEST(CDir, delete_recursively) {
  CStr dir = CSTR(ANYLIBS_C_TEST_PLAYGROUND "/folder");
  CStrBuf *dir_buf = c_str_create_from_raw(dir, true, NULL);
  EXPECT_TRUE(dir_buf);

  ASSERT_TRUE(c_fs_exists(dir) != 0);
  ASSERT_TRUE(c_fs_dir_create(dir));

  CFile *f =
      c_fs_file_open(CSTR(ANYLIBS_C_TEST_PLAYGROUND "/folder/file"), CSTR("w"));
  ASSERT_TRUE(f);
  EXPECT_TRUE(c_fs_file_write(f, CSTR("Hello World"), NULL));
  EXPECT_TRUE(c_fs_file_close(f));

  EXPECT_TRUE(c_fs_delete_recursively(dir_buf));

  c_str_destroy(dir_buf);
}
