#include "anylibs/fs.h"
#include "anylibs/error.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <utest.h>

#define CPATH(path)                      \
  (CPath)                                \
  {                                      \
    .data = (path), .size = strlen(path) \
  }

UTEST(CFsIter, iter)
{
  enum { buf_size = 1024 };
  char     buf[buf_size] = ANYLIBS_C_TEST_PLAYGROUND;
  CPathBuf pathbuf       = c_fs_pathbuf_create(buf, strlen(buf), buf_size);

  CFsIter iter;
  EXPECT_TRUE(c_fs_iter(&pathbuf, &iter) == 0);
  CPathRef result;
  bool     has_at_least_one_file = false;
  while (c_fs_iter_next(&iter, &result) == 0) {
    has_at_least_one_file = true;

    size_t f_size = 0;
    if (c_fs_is_dir(c_fs_cpathref_to_cpath(result)) != 0) {
      CFile* f = c_fs_file_open(c_fs_cpathref_to_cpath(result), "r", 1);
      EXPECT_TRUE(f);
      EXPECT_TRUE(c_fs_file_size(f, &f_size) == 0);
      break;
    }
  }
  EXPECT_TRUE(has_at_least_one_file);

  c_fs_iter_close(&iter);
  EXPECT_STRNEQ(ANYLIBS_C_TEST_PLAYGROUND, pathbuf.data, pathbuf.size);
}

UTEST(CFile, file)
{
  // write
  {
    CFile* f = c_fs_file_open(CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file"), "w", 1);
    ASSERT_TRUE(f);

    EXPECT_TRUE(c_fs_file_write(f, "Om Kulthuom\n", sizeof("Om Kulthuom\n") - 1, sizeof(char)));
    EXPECT_TRUE(c_fs_file_flush(f) == 0);
    EXPECT_TRUE(c_fs_file_close(f) == 0);
  }

  // read & prop
  {
    CFile* f = c_fs_file_open(CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file"), "r", 1);
    ASSERT_TRUE(f);

    enum { buf_size = 100 };
    char   buf[buf_size];
    size_t read_size = c_fs_file_read(f, buf, buf_size, sizeof(char));
    EXPECT_TRUE(read_size > 0);
    EXPECT_STRNEQ("Om Kulthuom\n", buf, read_size);

    size_t f_size;
    EXPECT_TRUE(c_fs_file_size(f, &f_size) == 0);
    EXPECT_EQ(sizeof("Om Kulthuom\n") - 1, f_size);

    EXPECT_TRUE(c_fs_file_close(f) == 0);
  }

  // delete
  {
    EXPECT_TRUE(c_fs_delete(CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file")) == 0);
  }
}

UTEST(CPath, general)
{
  // CPathBuf* path = c_str_create_from_raw(CPATH(ANYLIBS_C_TEST_PLAYGROUND), true, NULL);
  // ASSERT_TRUE(path);

  enum { buf_size = 1024 };
  char     buf[buf_size];
  CPathBuf pathbuf = c_fs_pathbuf_create(buf, 0, buf_size);

  // absolute
  EXPECT_TRUE(c_fs_path_to_absolute(CPATH("."), &pathbuf) == 0);
  CPath path = c_fs_path_create(pathbuf.data, pathbuf.size);
  EXPECT_TRUE(c_fs_path_is_absolute(path) == 0);

  size_t pathbuf_cur_size = pathbuf.size;
  EXPECT_TRUE(c_fs_path_append(&pathbuf, CPATH("folder/file")) == 0);
  EXPECT_STREQ("folder/file", pathbuf.data + pathbuf_cur_size + 1); // ends_with ?
}

UTEST(CPath, filename)
{
  CPath path = CPATH("/folder/file.txt");

  EXPECT_TRUE(c_fs_path_filename(&path) == 0);
  EXPECT_STREQ("file.txt", path.data);
}

UTEST(CPath, filestem)
{

  CPath path = CPATH("/folder/file.txt");
  EXPECT_TRUE(c_fs_path_filestem(&path) == 0);
  EXPECT_STRNEQ("file", path.data, path.size);

  path = CPATH("/folder/file");
  EXPECT_TRUE(c_fs_path_filestem(&path) == 0);
  EXPECT_STRNEQ("file", path.data, path.size);

  path = CPATH("/folder/.file");
  EXPECT_TRUE(c_fs_path_filestem(&path) == 0);
  EXPECT_STRNEQ(".file", path.data, path.size);
}

UTEST(CPath, file_extension)
{
  CPath path = CPATH("/folder/file.txt");

  EXPECT_TRUE(c_fs_path_file_extension(&path) == 0);
  EXPECT_STREQ("txt", path.data);
}

UTEST(CPath, metadata)
{
  CPath  path      = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file");
  char   data[]    = "Om Kulthuom\n";
  size_t data_size = strlen(data);
  CFile* f         = c_fs_file_open(path, "w", 1);
  ASSERT_TRUE(f);

  EXPECT_TRUE(c_fs_file_write(f, data, data_size, sizeof(char)) > 0);
  EXPECT_TRUE(c_fs_file_close(f) == 0);

  CFsMetadata metadata;
  EXPECT_TRUE(c_fs_path_metadata(path, &metadata) == 0);
  EXPECT_EQ((int)metadata.ftype, C_FS_FILE_TYPE_file);
  EXPECT_EQ((int)metadata.fperm, C_FS_FILE_PERMISSION_read_write);
  EXPECT_EQ(metadata.fsize, data_size);
  EXPECT_NE(0, metadata.created_time);
  EXPECT_NE(0, metadata.last_modified);
  EXPECT_NE(0, metadata.last_accessed);

  EXPECT_TRUE(c_fs_delete(path) == 0);
}

// UTEST(CPath, iter)
// {
//   char*  gt[] = {"file", "folder3", "folder2", "folder1"};
//   CPath  component;
//   size_t counter = 0;
//   CIter  iter    = c_fs_path_iter(CPATH("/folder1/folder2/folder3/file"));
//   while (c_fs_path_iter_component_next(&iter, &component)) {
//     EXPECT_STRNEQ(gt[counter++], component.data, component.size);
//   }
//   EXPECT_EQ(sizeof(gt) / sizeof(*gt), counter);
// }

UTEST(CDir, create_delete)
{
  CPath dir = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/folder");
  ASSERT_TRUE(c_fs_exists(dir) != 0);
  ASSERT_TRUE(c_fs_dir_create(dir) == 0);
  EXPECT_TRUE(c_fs_dir_is_empty(dir) == 0);
  EXPECT_TRUE(c_fs_delete(dir) == 0);
}

UTEST(CDir, current_dir)
{
  enum { buf_size = 1024 };
  char     buf[buf_size];
  CPathBuf pathbuf = c_fs_pathbuf_create(buf, 0, buf_size);

  EXPECT_TRUE(c_fs_dir_current(&pathbuf) == 0);
  EXPECT_TRUE(pathbuf.size > 0);
  EXPECT_TRUE(c_fs_is_dir(c_fs_path_create(pathbuf.data, pathbuf.size)) == 0);

  size_t old_path_size = pathbuf.size;
  EXPECT_TRUE(c_fs_path_append(&pathbuf, CPATH("..")) == 0);
  EXPECT_TRUE(c_fs_dir_change_current(c_fs_path_create(pathbuf.data, pathbuf.size)) == 0);
  pathbuf.data[old_path_size] = '\0';
  EXPECT_TRUE(c_fs_dir_change_current(c_fs_path_create(pathbuf.data, old_path_size)) == 0);
}

UTEST(CDir, current_exe_dir)
{
  enum { buf_size = 1024 };
  char     buf[buf_size];
  CPathBuf pathbuf = c_fs_pathbuf_create(buf, 0, buf_size);

  EXPECT_TRUE(c_fs_dir_current_exe(&pathbuf) == 0);
  EXPECT_TRUE(pathbuf.size > 0);
  EXPECT_TRUE(c_fs_is_file(c_fs_path_create(pathbuf.data, pathbuf.size)) == 0);
}

UTEST(CDir, delete_recursively)
{
  enum { buf_size = 1024 };
  char     buf[buf_size] = ANYLIBS_C_TEST_PLAYGROUND "/folder";
  CPathBuf pathbuf       = c_fs_pathbuf_create(buf, strlen(buf), buf_size);
  CPath    path          = c_fs_path_create(buf, pathbuf.size);

  ASSERT_TRUE(c_fs_exists(path) != 0);
  ASSERT_TRUE(c_fs_dir_create(path) == 0);

  CFile* f = c_fs_file_open(CPATH(ANYLIBS_C_TEST_PLAYGROUND "/folder/file"), "w", 1);
  ASSERT_TRUE(f);
  EXPECT_TRUE(c_fs_file_write(f, "Hello World", sizeof("Hello World") - 1, sizeof(char)) > 0);
  EXPECT_TRUE(c_fs_file_close(f) == 0);

  CPath dir2 = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/folder/folder2");
  ASSERT_TRUE(c_fs_exists(dir2) != 0);
  ASSERT_TRUE(c_fs_dir_create(dir2) == 0);

  EXPECT_TRUE(c_fs_delete_recursively(&pathbuf) == 0);
}

UTEST(CPath, delete_recursively_file)
{
  enum { buf_size = 1024 };
  char     buf[buf_size] = ANYLIBS_C_TEST_PLAYGROUND "/file";
  CPathBuf pathbuf       = c_fs_pathbuf_create(buf, strlen(buf), buf_size);
  CPath    path          = c_fs_path_create(buf, pathbuf.size);

  CFile* f = c_fs_file_open(path, "w", 1);
  ASSERT_TRUE(f);
  EXPECT_TRUE(c_fs_file_write(f, "Hello World", sizeof("Hello World") - 1, sizeof(char)) > 0);
  EXPECT_TRUE(c_fs_file_close(f) == 0);

  EXPECT_FALSE(c_fs_delete_recursively(&pathbuf) == 0);
  EXPECT_TRUE(c_fs_delete(path) == 0);
}

UTEST(CPath, parent)
{
  CPath path = CPATH("/folder/file");
  EXPECT_TRUE(c_fs_path_parent(&path) == 0);
  EXPECT_STRNEQ("/folder", path.data, path.size);

  path = CPATH("/folder/file////");
  EXPECT_TRUE(c_fs_path_parent(&path) == 0);
  EXPECT_STRNEQ("/folder", path.data, path.size);

  path = CPATH("/folder/////file");
  EXPECT_TRUE(c_fs_path_parent(&path) == 0);
  EXPECT_STRNEQ("/folder", path.data, path.size);

  path = CPATH("folder/file");
  EXPECT_TRUE(c_fs_path_parent(&path) == 0);
  EXPECT_STRNEQ("folder", path.data, path.size);

  path = CPATH("/folder");
  EXPECT_FALSE(c_fs_path_parent(&path) == 0);
  EXPECT_EQ(0U, path.size);

  path = CPATH("/");
  EXPECT_FALSE(c_fs_path_parent(&path) == 0);
  EXPECT_EQ(0U, path.size);

  path = CPATH("C:\\");
  EXPECT_FALSE(c_fs_path_parent(&path) == 0);
  EXPECT_EQ(0U, path.size);

  path = CPATH("C:\\folder");
  EXPECT_TRUE(c_fs_path_parent(&path) == 0);
  EXPECT_STRNEQ("C:\\", path.data, path.size);

  path = CPATH("C:\\folder\\\\");
  EXPECT_TRUE(c_fs_path_parent(&path) == 0);
  EXPECT_STRNEQ("C:\\", path.data, path.size);
}
