#include "anylibs/fs.h"
#include "anylibs/error.h"
#include "anylibs/str.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <utest.h>

UTEST(CFsIter, iter)
{
  CStrBuf* path =
      c_str_create_from_raw(CSTR(ANYLIBS_C_TEST_PLAYGROUND), true, NULL);
  ASSERT_TRUE_MSG(path, c_error_to_str(c_error_get()));

  CFsIter iter = c_fs_iter(path);
  CStr    result;
  bool    has_at_least_one_file = false;
  while (c_fs_iter_next(&iter, &result)) {
    has_at_least_one_file = true;

    size_t f_size = 0;
    if (c_fs_is_dir(result) != 0) {
      CFile* f = c_fs_file_open(result, CSTR("r"));
      EXPECT_TRUE_MSG(f, c_error_to_str(c_error_get()));
      EXPECT_TRUE_MSG(c_fs_file_size(f, &f_size),
                      c_error_to_str(c_error_get()));
    }
  }
  EXPECT_TRUE(has_at_least_one_file);

  c_fs_iter_close(&iter);
  c_str_destroy(path);
}

UTEST(CFile, file)
{
  // write
  {
    CFile* f =
        c_fs_file_open(CSTR(ANYLIBS_C_TEST_PLAYGROUND "/file"), CSTR("w"));
    ASSERT_TRUE(f);

    EXPECT_TRUE(c_fs_file_write(f, CSTR("Om Kulthuom\n"), NULL));
    EXPECT_TRUE(c_fs_file_flush(f));
    EXPECT_TRUE(c_fs_file_close(f));
  }

  // read & prop
  {
    CFile* f =
        c_fs_file_open(CSTR(ANYLIBS_C_TEST_PLAYGROUND "/file"), CSTR("r"));
    ASSERT_TRUE(f);
    CStrBuf* buf = c_str_create_with_capacity(100 * sizeof(char), NULL, false);
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
  {
    EXPECT_TRUE(c_fs_delete(CSTR(ANYLIBS_C_TEST_PLAYGROUND "/file")));
  }
}

UTEST(CPath, general)
{
  CStrBuf* path =
      c_str_create_from_raw(CSTR(ANYLIBS_C_TEST_PLAYGROUND), true, NULL);
  ASSERT_TRUE(path);

  // absolute
  CStrBuf* abs_path = c_fs_path_to_absolute(CSTR("."), NULL);
  EXPECT_TRUE(abs_path);
  EXPECT_TRUE(c_fs_path_is_absolute(c_cstrbuf_to_cstr(abs_path)));
  c_str_destroy(abs_path);

  EXPECT_TRUE(c_fs_path_append(path, CSTR("folder/file")));
  EXPECT_TRUE(c_str_ends_with(path, CSTR("folder/file")) == 0);

  c_str_destroy(path);
}

UTEST(CPath, filename)
{
  CStr path = CSTR("/folder/file.txt");
  CStr filename;

  EXPECT_TRUE(c_fs_path_filename(path, &filename));
  EXPECT_STREQ("file.txt", filename.data);
}

UTEST(CPath, filestem)
{
  CStr filestem;

  EXPECT_TRUE(c_fs_path_filestem(CSTR("/folder/file.txt"), &filestem));
  EXPECT_STRNEQ("file", filestem.data, filestem.len);

  EXPECT_TRUE(c_fs_path_filestem(CSTR("/folder/file"), &filestem));
  EXPECT_STRNEQ("file", filestem.data, filestem.len);

  EXPECT_TRUE(c_fs_path_filestem(CSTR("/folder/.file"), &filestem));
  EXPECT_STRNEQ(".file", filestem.data, filestem.len);
}

UTEST(CPath, file_extension)
{
  CStr path = CSTR("/folder/file.txt");
  CStr file_extension;

  EXPECT_TRUE(c_fs_path_file_extension(path, &file_extension));
  EXPECT_STREQ("txt", file_extension.data);
}

UTEST(CPath, metadata)
{
  CStr   path = CSTR(ANYLIBS_C_TEST_PLAYGROUND "/file");
  CStr   data = CSTR("Om Kulthuom\n");
  CFile* f    = c_fs_file_open(path, CSTR("w"));
  ASSERT_TRUE(f);

  EXPECT_TRUE(c_fs_file_write(f, data, NULL));
  EXPECT_TRUE(c_fs_file_close(f));

  CFsMetadata metadata;
  EXPECT_TRUE(c_fs_path_metadata(path, &metadata));
  EXPECT_EQ((int)metadata.ftype, C_FS_FILE_TYPE_file);
  EXPECT_EQ((int)metadata.fperm, C_FS_FILE_PERMISSION_read_write);
  EXPECT_EQ(metadata.fsize, data.len);
  EXPECT_NE(0, metadata.created_time);
  EXPECT_NE(0, metadata.last_modified);
  EXPECT_NE(0, metadata.last_accessed);

  EXPECT_TRUE(c_fs_delete(path));
}

UTEST(CPath, iter)
{
  char*  gt[] = {"file", "folder3", "folder2", "folder1"};
  CStr   component;
  size_t counter = 0;
  CIter  iter    = c_fs_path_iter(CSTR("/folder1/folder2/folder3/file"));
  while (c_fs_path_iter_component_next(&iter, &component)) {
    EXPECT_STRNEQ(gt[counter++], component.data, component.len);
  }
  EXPECT_EQ(sizeof(gt) / sizeof(*gt), counter);
}

UTEST(CDir, create_delete)
{
  CStr dir = CSTR(ANYLIBS_C_TEST_PLAYGROUND "/folder");
  ASSERT_TRUE(c_fs_exists(dir) != 0);
  ASSERT_TRUE(c_fs_dir_create(dir));
  EXPECT_TRUE(c_fs_dir_is_empty(dir) == 0);
  EXPECT_TRUE(c_fs_delete(dir));
}

UTEST(CDir, current_dir)
{
  CStrBuf* cur_dir = c_fs_dir_current(NULL);
  EXPECT_TRUE(cur_dir);
  EXPECT_TRUE(c_str_len(cur_dir) > 0);
  EXPECT_TRUE(c_fs_is_dir(c_cstrbuf_to_cstr(cur_dir)) == 0);

  EXPECT_TRUE(c_fs_path_append(cur_dir, CSTR("..")));
  EXPECT_TRUE(c_fs_dir_change_current(c_cstrbuf_to_cstr(cur_dir)));

  c_str_destroy(cur_dir);
}

UTEST(CDir, current_exe_dir)
{
  CStrBuf* cur_exe_dir = c_fs_dir_current_exe(NULL);
  EXPECT_TRUE(cur_exe_dir);
  EXPECT_TRUE(c_str_len(cur_exe_dir) > 0);
  EXPECT_FALSE(c_fs_is_dir(c_cstrbuf_to_cstr(cur_exe_dir)) == 0);
  c_str_destroy(cur_exe_dir);
}

UTEST(CDir, delete_recursively)
{
  CStr     dir     = CSTR(ANYLIBS_C_TEST_PLAYGROUND "/folder");
  CStrBuf* dir_buf = c_str_create_from_raw(dir, true, NULL);
  EXPECT_TRUE(dir_buf);

  ASSERT_TRUE(c_fs_exists(dir) != 0);
  ASSERT_TRUE(c_fs_dir_create(dir));

  CFile* f = c_fs_file_open(CSTR(ANYLIBS_C_TEST_PLAYGROUND "/folder/file"), CSTR("w"));
  ASSERT_TRUE(f);
  EXPECT_TRUE(c_fs_file_write(f, CSTR("Hello World"), NULL));
  EXPECT_TRUE(c_fs_file_close(f));

  CStr dir2 = CSTR(ANYLIBS_C_TEST_PLAYGROUND "/folder/folder2");
  ASSERT_TRUE(c_fs_exists(dir2) != 0);
  ASSERT_TRUE(c_fs_dir_create(dir2));

  EXPECT_TRUE(c_fs_delete_recursively(dir_buf));

  c_str_destroy(dir_buf);
}

UTEST(CPath, delete_recursively_file)
{
  CStr     f_path     = CSTR(ANYLIBS_C_TEST_PLAYGROUND "/file");
  CStrBuf* f_path_buf = c_str_create_from_raw(f_path, true, NULL);
  EXPECT_TRUE(f_path_buf);
  CFile* f = c_fs_file_open(f_path, CSTR("w"));
  ASSERT_TRUE(f);
  EXPECT_TRUE(c_fs_file_write(f, CSTR("Hello World"), NULL));
  EXPECT_TRUE(c_fs_file_close(f));

  EXPECT_FALSE(c_fs_delete_recursively(f_path_buf));
  EXPECT_TRUE(c_fs_delete(f_path));

  c_str_destroy(f_path_buf);
}

UTEST(CPath, parent)
{
  CStr parent;
  EXPECT_TRUE(c_fs_path_parent(CSTR("/folder/file"), &parent));
  EXPECT_STRNEQ("/folder", parent.data, parent.len);

  EXPECT_TRUE(c_fs_path_parent(CSTR("/folder/file////"), &parent));
  EXPECT_STRNEQ("/folder", parent.data, parent.len);

  EXPECT_TRUE(c_fs_path_parent(CSTR("/folder/////file"), &parent));
  EXPECT_STRNEQ("/folder", parent.data, parent.len);

  EXPECT_TRUE(c_fs_path_parent(CSTR("folder/file"), &parent));
  EXPECT_STRNEQ("folder", parent.data, parent.len);

  EXPECT_FALSE(c_fs_path_parent(CSTR("/folder"), &parent));
  EXPECT_EQ(0U, parent.len);

  EXPECT_FALSE(c_fs_path_parent(CSTR("/"), &parent));
  EXPECT_EQ(0U, parent.len);

  EXPECT_FALSE(c_fs_path_parent(CSTR("C:\\"), &parent));
  EXPECT_EQ(0U, parent.len);

  EXPECT_TRUE(c_fs_path_parent(CSTR("C:\\folder"), &parent));
  EXPECT_STRNEQ("C:\\", parent.data, parent.len);

  EXPECT_TRUE(c_fs_path_parent(CSTR("C:\\folder\\\\"), &parent));
  EXPECT_STRNEQ("C:\\", parent.data, parent.len);
}
