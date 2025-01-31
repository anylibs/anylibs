#include "anylibs/fs.h"
#include "anylibs/error.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <utest.h>

#define CPATH(path)                      \
  (anylibs_fs_path_t)                    \
  {                                      \
    .data = (path), .size = strlen(path) \
  }

UTEST(anylibs_fs_file_t, open_folder_as_file)
{
  anylibs_fs_path_t path = CPATH(ANYLIBS_C_TEST_PLAYGROUND);
  anylibs_fs_file_t* f   = anylibs_fs_file_open(path, "r", 1);
  EXPECT_FALSE(f);
  EXPECT_EQ(0, anylibs_fs_file_close(&f));
}

UTEST(anylibs_fs_file_t, create_file)
{
  anylibs_fs_path_t fpath = CPATH(ANYLIBS_C_TEST_PLAYGROUND "file");
  anylibs_fs_file_t* f    = anylibs_fs_file_open(fpath, "w", 1);
  EXPECT_TRUE(f);
  size_t fsize;
  EXPECT_EQ(0, anylibs_fs_file_size(f, &fsize));
  EXPECT_EQ(0U, fsize);

  EXPECT_EQ(0, anylibs_fs_file_close(&f));
  EXPECT_EQ(0, anylibs_fs_delete(fpath));
}

UTEST(anylibs_fs_file_t, open_file_invalid_mode)
{
  anylibs_fs_path_t fpath = CPATH(ANYLIBS_C_TEST_PLAYGROUND "file");
  anylibs_fs_file_t* f    = anylibs_fs_file_open(fpath, "p", 1);
  EXPECT_FALSE(f);
  EXPECT_EQ(0, anylibs_fs_file_close(&f));
  (void)(utest_result);
}

UTEST(anylibs_fs_file_t, is_open)
{
  anylibs_fs_path_t fpath = CPATH(ANYLIBS_C_TEST_PLAYGROUND "file");
  anylibs_fs_file_t* f    = anylibs_fs_file_open(fpath, "w", 1);
  ASSERT_TRUE(f);
  EXPECT_EQ(0, anylibs_fs_file_is_open(f));

  EXPECT_EQ(0, anylibs_fs_file_close(&f));
  EXPECT_NE(0, anylibs_fs_file_is_open(f));
  EXPECT_EQ(0, anylibs_fs_delete(fpath));
}

UTEST(anylibs_fs_file_t, size)
{
  size_t fsize;
  EXPECT_NE(0, anylibs_fs_file_size(NULL, &fsize)); // invalid anylibs_fs_file_t

  anylibs_fs_path_t fpath = CPATH(ANYLIBS_C_TEST_PLAYGROUND "file");
  anylibs_fs_file_t* f    = anylibs_fs_file_open(fpath, "w", 1);
  ASSERT_TRUE(f);
  EXPECT_NE(0, anylibs_fs_file_size(f, NULL)); // invalid output file size
  EXPECT_EQ(0, anylibs_fs_file_size(f, &fsize));
  EXPECT_EQ(0U, fsize);
  EXPECT_EQ(1U, anylibs_fs_file_write(f, "a", 1, 1));
  EXPECT_EQ(0, anylibs_fs_file_size(f, &fsize));
  EXPECT_EQ(1U, fsize);

  EXPECT_EQ(0, anylibs_fs_file_close(&f));
  EXPECT_EQ(0, anylibs_fs_delete(fpath));
}

UTEST(anylibs_fs_file_t, read_write)
{
  // create, write, close
  anylibs_fs_path_t fpath = CPATH(ANYLIBS_C_TEST_PLAYGROUND "file");
  anylibs_fs_file_t* f    = anylibs_fs_file_open(fpath, "w", 1);
  ASSERT_TRUE(f);
  EXPECT_EQ(1U, anylibs_fs_file_write(f, "a", 1, 1));
  EXPECT_EQ(0, anylibs_fs_file_close(&f));

  // open, read, close
  enum { buf_size = 100 };
  char buf[buf_size];
  f = anylibs_fs_file_open(fpath, "r", 1);
  ASSERT_TRUE(f);
  EXPECT_EQ(1U, anylibs_fs_file_read(f, buf, 1, 1));
  EXPECT_EQ(0U, anylibs_fs_file_read(f, buf, 1, 1)); // invalid, the file is at then end already
  EXPECT_EQ(0, anylibs_fs_file_close(&f));

  // open, try read big amount, close
  f = anylibs_fs_file_open(fpath, "r", 1);
  ASSERT_TRUE(f);
  EXPECT_EQ(1U, anylibs_fs_file_read(f, buf, buf_size, 1)); // this will not fail
  EXPECT_EQ(0, anylibs_fs_file_close(&f));

  // open, try read big amount, close
  f = anylibs_fs_file_open(fpath, "r", 1);
  ASSERT_TRUE(f);
  EXPECT_EQ(0U, anylibs_fs_file_read(f, buf, 1, 4)); // this will fail
  EXPECT_EQ(0, anylibs_fs_file_close(&f));

  EXPECT_EQ(0, anylibs_fs_delete(fpath));
}

UTEST(anylibs_fs_path_t, append)
{
  enum { buf_size = 1000 };
  char buf[buf_size]           = "/folder";
  anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen(buf), buf_size);
  EXPECT_EQ(0, anylibs_fs_path_append(&pathbuf, anylibs_fs_path_create("file", strlen("file"))));

#ifdef _WIN32
  EXPECT_EQ(0, strncmp("/folder\\file", pathbuf.data, pathbuf.size));
#else
  EXPECT_EQ(0, strncmp("/folder/file", pathbuf.data, pathbuf.size));
#endif

  EXPECT_NE(0, anylibs_fs_path_append(NULL, anylibs_fs_path_create("file", strlen("file")))); // invalid
  pathbuf.size = buf_size;
  EXPECT_NE(0, anylibs_fs_path_append(&pathbuf, anylibs_fs_path_create("file", strlen("file")))); // invalid
}

UTEST(anylibs_fs_path_t, absolute)
{
  anylibs_fs_path_t path = anylibs_fs_path_create(".", strlen("."));
  enum { buf_size = 1000 };
  char buf[buf_size];
  anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, 0, buf_size);

  EXPECT_EQ(0, anylibs_fs_path_to_absolute(path, &pathbuf));
  EXPECT_LT(0U, pathbuf.size);
  EXPECT_TRUE(pathbuf.data);

  pathbuf.capacity = 0;
  EXPECT_NE(0, anylibs_fs_path_to_absolute(path, &pathbuf)); // invalid
  pathbuf.capacity = buf_size;

  path             = anylibs_fs_path_create("/folder", strlen("/folder"));
  EXPECT_NE(0, anylibs_fs_path_to_absolute(path, &pathbuf)); // invalid path

  path             = anylibs_fs_path_create(".", strlen("."));
  pathbuf.capacity = path.size + 1;
  EXPECT_NE(0, anylibs_fs_path_to_absolute(path, &pathbuf)); // not enough space
}

UTEST(anylibs_fs_path_t, is_absolute)
{
  EXPECT_EQ(1, anylibs_fs_path_is_absolute(anylibs_fs_path_create(".", strlen("."))));
  EXPECT_EQ(0, anylibs_fs_path_is_absolute(anylibs_fs_path_create(ANYLIBS_C_TEST_PLAYGROUND, strlen(ANYLIBS_C_TEST_PLAYGROUND))));
  EXPECT_EQ(0, anylibs_fs_path_is_absolute(anylibs_fs_path_create(ANYLIBS_C_TEST_PLAYGROUND "file", strlen(ANYLIBS_C_TEST_PLAYGROUND "file"))));
}

UTEST(anylibs_fs_path_t, parent)
{
  anylibs_fs_path_t path = CPATH("/folder/file");
  EXPECT_TRUE(0 == anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("/folder", path.data, strlen("/folder"));
  EXPECT_EQ(strlen("/folder"), path.size);

  path = CPATH("/folder/file////");
  EXPECT_TRUE(0 == anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("/folder", path.data, strlen("/folder"));
  EXPECT_EQ(strlen("/folder"), path.size);

  path = CPATH("/folder/////file");
  EXPECT_TRUE(0 == anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("/folder", path.data, strlen("/folder"));
  EXPECT_EQ(strlen("/folder"), path.size);

  path = CPATH("folder/file");
  EXPECT_TRUE(0 == anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("folder", path.data, strlen("folder"));
  EXPECT_EQ(strlen("folder"), path.size);

  path = CPATH("/folder");
  EXPECT_EQ(0, anylibs_fs_path_parent(&path));
  EXPECT_EQ(1U, path.size);

  path = CPATH("folder");
  EXPECT_EQ(1, anylibs_fs_path_parent(&path));

  path = CPATH("///////folder");
  EXPECT_EQ(0, anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("/", path.data, strlen("/"));
  EXPECT_EQ(strlen("/"), path.size);

  path = CPATH("///////folder/file");
  EXPECT_EQ(0, anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("///////folder", path.data, strlen("///////folder"));
  EXPECT_EQ(strlen("///////folder"), path.size);

  path = CPATH("/");
  EXPECT_FALSE(0 == anylibs_fs_path_parent(&path));
  EXPECT_EQ(0U, path.size);

  path = CPATH("/./././//");
  EXPECT_FALSE(0 == anylibs_fs_path_parent(&path));
  EXPECT_EQ(0U, path.size);

#ifdef _WIN32
  path = CPATH("C:\\");
  EXPECT_FALSE(0 == anylibs_fs_path_parent(&path));
  EXPECT_EQ(0U, path.size);

  path = CPATH("C:\\folder");
  EXPECT_TRUE(0 == anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("C:\\", path.data, strlen("C:\\"));
  EXPECT_EQ(strlen("C:\\"), path.size);

  path = CPATH("C:\\folder\\\\");
  EXPECT_TRUE(0 == anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("C:\\", path.data, strlen("C:\\"));
  EXPECT_EQ(strlen("C:\\"), path.size);

  path = CPATH("C:\\folder\\file");
  EXPECT_TRUE(0 == anylibs_fs_path_parent(&path));
  EXPECT_STRNEQ("C:\\folder", path.data, strlen("C:\\folder"));
  EXPECT_EQ(strlen("C:\\folder"), path.size);
#endif

  char* gt[]     = {"/folder1/folder2/file", "/folder1/folder2", "/folder1", "/"};
  size_t counter = 0;
  path           = CPATH("/folder1/folder2/file");
  do {
    EXPECT_EQ(strlen(gt[counter]), path.size);
    EXPECT_STRNEQ(gt[counter], path.data, path.size);
    counter++;
  } while (anylibs_fs_path_parent(&path) == 0);
}

UTEST(anylibs_fs_path_t, filename)
{
  anylibs_fs_path_t path = CPATH("/folder/file.txt");
  EXPECT_TRUE(anylibs_fs_path_filename(&path) == 0);
  EXPECT_STRNEQ("file.txt", path.data, strlen("file.txt"));
  EXPECT_EQ(strlen("file.txt"), path.size);

  path = CPATH("/folder/folder2/");
  EXPECT_TRUE(anylibs_fs_path_filename(&path) == 0);
  EXPECT_STRNEQ("folder2", path.data, strlen("folder2"));
  EXPECT_EQ(strlen("folder2"), path.size);

  path = CPATH("/folder/");
  EXPECT_TRUE(anylibs_fs_path_filename(&path) == 0);
  EXPECT_STRNEQ("folder", path.data, strlen("folder"));
  EXPECT_EQ(strlen("folder"), path.size);

  path = CPATH("/./././//");
  EXPECT_FALSE(0 == anylibs_fs_path_filename(&path));
  EXPECT_EQ(0U, path.size);

  path = CPATH("//////");
  EXPECT_FALSE(0 == anylibs_fs_path_filename(&path));
  EXPECT_EQ(0U, path.size);

  path = CPATH("foo.txt/.");
  EXPECT_TRUE(anylibs_fs_path_filename(&path) == 0);
  EXPECT_STRNEQ("foo.txt", path.data, strlen("foo.txt"));
  EXPECT_EQ(strlen("foo.txt"), path.size);

  path = CPATH("/folder1/folder2/.");
  EXPECT_TRUE(anylibs_fs_path_filename(&path) == 0);
  EXPECT_STRNEQ("folder2", path.data, strlen("folder2"));
  EXPECT_EQ(strlen("folder2"), path.size);

  path = CPATH("folder/..");
  EXPECT_EQ(1, anylibs_fs_path_filename(&path));

  path = CPATH("/");
  EXPECT_FALSE(0 == anylibs_fs_path_filename(&path));
  EXPECT_EQ(0U, path.size);
}

UTEST(anylibs_fs_path_t, filestem)
{
  anylibs_fs_path_t path = CPATH("/folder/file.txt");
  EXPECT_TRUE(anylibs_fs_path_filestem(&path) == 0);
  EXPECT_STRNEQ("file", path.data, strlen("file"));
  EXPECT_EQ(strlen("file"), path.size);

  path = CPATH("/folder/folder2/");
  EXPECT_TRUE(anylibs_fs_path_filestem(&path) == 0);
  EXPECT_STRNEQ("folder2", path.data, strlen("folder2"));
  EXPECT_EQ(strlen("folder2"), path.size);

  path = CPATH("/folder/");
  EXPECT_TRUE(anylibs_fs_path_filestem(&path) == 0);
  EXPECT_STRNEQ("folder", path.data, strlen("folder"));
  EXPECT_EQ(strlen("folder"), path.size);

  path = CPATH("/./././//");
  EXPECT_FALSE(0 == anylibs_fs_path_filestem(&path));
  EXPECT_EQ(0U, path.size);

  path = CPATH("foo.txt/.");
  EXPECT_TRUE(anylibs_fs_path_filestem(&path) == 0);
  EXPECT_STRNEQ("foo", path.data, strlen("foo"));
  EXPECT_EQ(strlen("foo"), path.size);

  path = CPATH("/");
  EXPECT_FALSE(0 == anylibs_fs_path_filestem(&path));
  EXPECT_EQ(0U, path.size);

  path = CPATH("/folder/.file");
  EXPECT_TRUE(0 == anylibs_fs_path_filestem(&path));
  EXPECT_STRNEQ(".file", path.data, strlen(".file"));
  EXPECT_EQ(strlen(".file"), path.size);
}

UTEST(anylibs_fs_path_t, file_extension)
{
  anylibs_fs_path_t path = CPATH("/folder/file.txt");
  EXPECT_TRUE(anylibs_fs_path_file_extension(&path) == 0);
  EXPECT_STRNEQ("txt", path.data, strlen("txt"));
  EXPECT_EQ(strlen("txt"), path.size);

  path = CPATH("/folder/folder2/");
  EXPECT_FALSE(anylibs_fs_path_file_extension(&path) == 0);

  path = CPATH("/folder/");
  EXPECT_FALSE(anylibs_fs_path_file_extension(&path) == 0);

  path = CPATH("/./././//");
  EXPECT_FALSE(0 == anylibs_fs_path_file_extension(&path));
  EXPECT_EQ(0U, path.size);

  path = CPATH("foo.txt/.");
  EXPECT_TRUE(anylibs_fs_path_file_extension(&path) == 0);
  EXPECT_STRNEQ("txt", path.data, strlen("txt"));
  EXPECT_EQ(strlen("txt"), path.size);

  path = CPATH("/");
  EXPECT_FALSE(0 == anylibs_fs_path_file_extension(&path));
  EXPECT_EQ(0U, path.size);

  path = CPATH("/folder/.file");
  EXPECT_TRUE(0 == anylibs_fs_path_file_extension(&path));
  EXPECT_STRNEQ("", path.data, strlen(""));
  EXPECT_EQ(0U, path.size);
}

UTEST(anylibs_fs_path_t, metadata)
{
  anylibs_fs_path_t path = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file");
  char data[]            = "Om Kulthuom\n";
  size_t data_size       = strlen(data);
  anylibs_fs_file_t* f   = anylibs_fs_file_open(path, "w", 1);
  ASSERT_TRUE(f);

  EXPECT_TRUE(anylibs_fs_file_write(f, data, data_size, sizeof(char)) > 0);
  EXPECT_TRUE(anylibs_fs_file_close(&f) == 0);

  anylibs_fs_metadata_t metadata;
  EXPECT_TRUE(anylibs_fs_path_metadata(path, &metadata) == 0);
  EXPECT_EQ((int)metadata.ftype, C_FS_FILE_TYPE_file);
  EXPECT_EQ((int)metadata.fperm, C_FS_FILE_PERMISSION_read_write);
  EXPECT_EQ(metadata.fsize, data_size);
  EXPECT_NE(0, metadata.created_time);
  EXPECT_NE(0, metadata.last_modified);
  EXPECT_NE(0, metadata.last_accessed);

  EXPECT_TRUE(anylibs_fs_delete(path) == 0);

  EXPECT_NE(0, anylibs_fs_path_metadata(path, &metadata)); // not found file error
}

UTEST(anylibs_fs_path_t, create_dir)
{
  anylibs_fs_path_t dir = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/folder2");
  ASSERT_NE(0, anylibs_fs_exists(dir));
  ASSERT_EQ(0, anylibs_fs_dir_create(dir));
  EXPECT_EQ(0, anylibs_fs_dir_is_empty(dir));
  EXPECT_EQ(0, anylibs_fs_delete(dir));

  dir = CPATH(ANYLIBS_C_TEST_PLAYGROUND); // already exists
  ASSERT_EQ(0, anylibs_fs_exists(dir));
  ASSERT_NE(0, anylibs_fs_dir_create(dir));
}

UTEST(anylibs_fs_path_t, is_dir)
{
  anylibs_fs_path_t dir = CPATH(ANYLIBS_C_TEST_PLAYGROUND);
  EXPECT_EQ(0, anylibs_fs_exists(dir));
  EXPECT_EQ(0, anylibs_fs_is_dir(dir));

  dir = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file"); // not exists
  ASSERT_NE(0, anylibs_fs_is_dir(dir));

  dir                  = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file"); // a file not a directory
  anylibs_fs_file_t* f = anylibs_fs_file_open(dir, "w", 1);
  ASSERT_TRUE(f);
  EXPECT_EQ(0, anylibs_fs_file_close(&f));
  EXPECT_NE(0, anylibs_fs_is_dir(dir));
  EXPECT_EQ(0, anylibs_fs_delete(dir));
}

UTEST(anylibs_fs_path_t, is_file)
{
  anylibs_fs_path_t file = CPATH(ANYLIBS_C_TEST_PLAYGROUND); // a directory not a file
  EXPECT_EQ(0, anylibs_fs_exists(file));
  EXPECT_NE(0, anylibs_fs_is_file(file));

  file = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file"); // not exists
  EXPECT_NE(0, anylibs_fs_is_file(file));

  file                 = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file");
  anylibs_fs_file_t* f = anylibs_fs_file_open(file, "w", 1);
  ASSERT_TRUE(f);
  EXPECT_EQ(0, anylibs_fs_file_close(&f));
  EXPECT_EQ(0, anylibs_fs_is_file(file));
  EXPECT_EQ(0, anylibs_fs_delete(file));
}

UTEST(anylibs_fs_path_t, is_symlink)
{
  anylibs_fs_path_t orig_symlink_path = CPATH(ANYLIBS_C_TEST_PLAYGROUND "playground");
  anylibs_fs_path_t symlink_path      = orig_symlink_path;
#ifdef _WIN32
  bool status = CreateSymbolicLinkA(symlink_path.data, ANYLIBS_C_TEST_PLAYGROUND, 0);
  ASSERT_TRUE(status);
#else
  int status = symlink(ANYLIBS_C_TEST_PLAYGROUND, symlink_path.data);
  ASSERT_EQ(0, status);
#endif

  EXPECT_EQ(0, anylibs_fs_exists(symlink_path));
  EXPECT_EQ(0, anylibs_fs_is_symlink(symlink_path));

  symlink_path = CPATH(ANYLIBS_C_TEST_PLAYGROUND); // a directory not a symlink
  EXPECT_EQ(0, anylibs_fs_exists(symlink_path));
  EXPECT_NE(0, anylibs_fs_is_symlink(symlink_path));

  symlink_path = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/symlink"); // not exists
  EXPECT_NE(0, anylibs_fs_is_symlink(symlink_path));

  EXPECT_EQ(0, anylibs_fs_delete(orig_symlink_path));
}

UTEST(anylibs_fs_path_t, current_dir)
{
  enum { buf_size = 1024 };
  char buf[buf_size];
  anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, 0, buf_size);

  EXPECT_EQ(0, anylibs_fs_dir_current(&pathbuf));
  EXPECT_TRUE(0 < pathbuf.size);
  EXPECT_EQ(0, anylibs_fs_is_dir(anylibs_fs_path_create(pathbuf.data, pathbuf.size)));

  pathbuf.capacity = 0;
  EXPECT_NE(0, anylibs_fs_dir_current(&pathbuf)); // invalid capacity

  // size_t old_path_size = pathbuf.size;
  // EXPECT_TRUE(anylibs_fs_path_append(&pathbuf, CPATH("..")) == 0);
  // EXPECT_TRUE(anylibs_fs_dir_change_current(anylibs_fs_path_create(pathbuf.data, pathbuf.size)) == 0);
  // pathbuf.data[old_path_size] = '\0';
  // EXPECT_TRUE(anylibs_fs_dir_change_current(anylibs_fs_path_create(pathbuf.data, old_path_size)) == 0);
}

UTEST(anylibs_fs_path_t, current_exe_path)
{
  enum { buf_size = 1024 };
  char buf[buf_size];
  anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, 0, buf_size);

  EXPECT_TRUE(anylibs_fs_dir_current_exe(&pathbuf) == 0);
  EXPECT_TRUE(pathbuf.size > 0);
  EXPECT_TRUE(anylibs_fs_is_file(anylibs_fs_path_create(pathbuf.data, pathbuf.size)) == 0);

  pathbuf.capacity = 0;
  EXPECT_NE(0, anylibs_fs_dir_current_exe(&pathbuf)); // invalid capacity
}

UTEST(anylibs_fs_path_t, change_current)
{

  enum { buf_size = 1000 };
  char old_cur_buf[buf_size];
  char abs_buf[buf_size];
  char cur_buf[buf_size];
  anylibs_fs_pathbuf_t old_cur = anylibs_fs_pathbuf_create(old_cur_buf, 0, buf_size);
  anylibs_fs_pathbuf_t abs     = anylibs_fs_pathbuf_create(abs_buf, 0, buf_size);
  anylibs_fs_pathbuf_t cur     = anylibs_fs_pathbuf_create(cur_buf, 0, buf_size);

  EXPECT_EQ(0, anylibs_fs_dir_current(&old_cur));
  EXPECT_EQ(0, anylibs_fs_path_to_absolute(CPATH(".."), &abs));

  EXPECT_EQ(0, anylibs_fs_dir_change_current(CPATH("..")));
  EXPECT_EQ(0, anylibs_fs_dir_current(&cur));
  EXPECT_EQ(0, strncmp(abs.data, cur.data, abs.size));

  EXPECT_EQ(0, anylibs_fs_dir_change_current(anylibs_fs_path_create(old_cur.data, old_cur.size)));

  ////

  EXPECT_NE(0, anylibs_fs_dir_change_current(CPATH("/none/existing/path"))); // invalid path
}

UTEST(anylibs_fs_path_t, dir_is_empty)
{
  anylibs_fs_path_t dir_path = CPATH(ANYLIBS_C_TEST_PLAYGROUND);
  EXPECT_EQ(1, anylibs_fs_dir_is_empty(dir_path));

  dir_path = CPATH(ANYLIBS_C_TEST_PLAYGROUND "folder3");
  ASSERT_EQ(0, anylibs_fs_dir_create(dir_path));
  EXPECT_EQ(0, anylibs_fs_dir_is_empty(dir_path));
  EXPECT_EQ(0, anylibs_fs_delete(dir_path));

  dir_path = CPATH(ANYLIBS_C_TEST_PLAYGROUND "folder3"); // none existing folder
  EXPECT_EQ(-1, anylibs_fs_dir_is_empty(dir_path));
}

UTEST(anylibs_fs_path_t, exists)
{
  anylibs_fs_path_t dir_path = CPATH(ANYLIBS_C_TEST_PLAYGROUND);
  EXPECT_EQ(0, anylibs_fs_exists(dir_path));

  dir_path = CPATH(ANYLIBS_C_TEST_PLAYGROUND "folder4");
  EXPECT_EQ(1, anylibs_fs_exists(dir_path));

#ifdef WIN32
  // I can't find any path that I don't have access too.
  // dir_path = CPATH("C:\\System Volume Information\\folder");
  // EXPECT_EQ(-1, anylibs_fs_exists(dir_path));
#elif defined(__APPLE__)
  dir_path = CPATH("/private/var/root/folder");
  EXPECT_EQ(-1, anylibs_fs_exists(dir_path));
#else
  dir_path = CPATH("/proc/tty/driver/folder");
  EXPECT_EQ(-1, anylibs_fs_exists(dir_path));
#endif
}

UTEST(anylibs_fs_path_t, delete)
{
  // empty directory
  anylibs_fs_path_t path = CPATH(ANYLIBS_C_TEST_PLAYGROUND "folder5");
  ASSERT_EQ(0, anylibs_fs_dir_create(path));
  EXPECT_EQ(0, anylibs_fs_delete(path));
  EXPECT_EQ(1, anylibs_fs_exists(path));

  // file
  path                 = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/file");
  anylibs_fs_file_t* f = anylibs_fs_file_open(path, "w", 1);
  ASSERT_TRUE(f);
  EXPECT_EQ(0, anylibs_fs_file_close(&f));
  EXPECT_EQ(0, anylibs_fs_delete(path));
  EXPECT_EQ(1, anylibs_fs_exists(path));

  // none existing path
  path = CPATH(ANYLIBS_C_TEST_PLAYGROUND "folder5");
  EXPECT_EQ(-1, anylibs_fs_delete(path));
  EXPECT_EQ(1, anylibs_fs_exists(path));
}

UTEST(CDir, delete_recursively)
{
  enum { buf_size = 4096 };
  char buf[buf_size]           = ANYLIBS_C_TEST_PLAYGROUND "/folder";
  anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen(buf), buf_size);
  anylibs_fs_path_t path       = anylibs_fs_path_create(buf, pathbuf.size);

  ASSERT_NE(0, anylibs_fs_exists(path));
  ASSERT_EQ(0, anylibs_fs_dir_create(path));

  anylibs_fs_file_t* f = anylibs_fs_file_open(CPATH(ANYLIBS_C_TEST_PLAYGROUND "/folder/file"), "w", 1);
  ASSERT_TRUE(f);
  EXPECT_LT(0U, anylibs_fs_file_write(f, "Hello World", sizeof("Hello World") - 1, sizeof(char)));
  EXPECT_EQ(0, anylibs_fs_file_close(&f));

  anylibs_fs_path_t dir2 = CPATH(ANYLIBS_C_TEST_PLAYGROUND "/folder/folder2");
  ASSERT_NE(0, anylibs_fs_exists(dir2));
  ASSERT_EQ(0, anylibs_fs_dir_create(dir2));

  EXPECT_TRUE(anylibs_fs_delete_recursively(&pathbuf) == 0);
  EXPECT_NE(0, anylibs_fs_exists(path));
}

UTEST(anylibs_fs_path_t, delete_recursively_file)
{
  enum { buf_size = 1024 };
  char buf[buf_size]           = ANYLIBS_C_TEST_PLAYGROUND "/file";
  anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen(buf), buf_size);
  anylibs_fs_path_t path       = anylibs_fs_path_create(buf, pathbuf.size);

  anylibs_fs_file_t* f         = anylibs_fs_file_open(path, "w", 1);
  ASSERT_TRUE(f);
  EXPECT_TRUE(anylibs_fs_file_write(f, "Hello World", sizeof("Hello World") - 1, sizeof(char)) > 0);
  EXPECT_TRUE(anylibs_fs_file_close(&f) == 0);

  EXPECT_FALSE(anylibs_fs_delete_recursively(&pathbuf) == 0);
  EXPECT_TRUE(anylibs_fs_delete(path) == 0);
}

static char const* search_for(char const* arr[], size_t arr_size, char const* needle, size_t needle_size)
{
  if (!needle_size) return NULL;

  for (size_t iii = 0; iii < arr_size; ++iii) {
    // printf("%s | %.*s\n", arr[iii], (int)needle_size, needle);
    if (strncmp(arr[iii], needle, needle_size) == 0) {
      return arr[iii];
    }
  }

  return NULL;
}

UTEST(anylibs_fs_iter_t, iter)
{
  char const* gt[] = {"file", "folder2", ".", ".."};

  enum { buf_size = 4096 };
  char buf[buf_size]           = ANYLIBS_C_TEST_PLAYGROUND "folder6";
  anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen(buf), buf_size);
  anylibs_fs_path_t path       = anylibs_fs_path_create(buf, pathbuf.size);

  ASSERT_NE(0, anylibs_fs_exists(path));
  ASSERT_EQ(0, anylibs_fs_dir_create(path));

  anylibs_fs_file_t* f = anylibs_fs_file_open(CPATH(ANYLIBS_C_TEST_PLAYGROUND "folder6/file"), "w", 1);
  ASSERT_TRUE(f);
  EXPECT_LT(0U, anylibs_fs_file_write(f, "Hello World", sizeof("Hello World") - 1, sizeof(char)));
  EXPECT_EQ(0, anylibs_fs_file_close(&f));

  anylibs_fs_path_t dir2 = CPATH(ANYLIBS_C_TEST_PLAYGROUND "folder6/folder2");
  ASSERT_NE(0, anylibs_fs_exists(dir2));
  ASSERT_EQ(0, anylibs_fs_dir_create(dir2));

  anylibs_fs_iter_t iter;
  ASSERT_EQ(0, anylibs_fs_iter_create(&pathbuf, &iter));

  int status;
  anylibs_fs_pathref_t pathref;
  while ((status = anylibs_fs_iter_next(&iter, &pathref)) == 0) {
    EXPECT_LT(0U, pathref.size);
    anylibs_fs_path_t last_component;
    EXPECT_EQ(0, anylibs_fs_path_get_last_component(*(anylibs_fs_path_t*)&pathref, &last_component));
    EXPECT_TRUE(search_for(gt, sizeof(gt) / sizeof(*gt), last_component.data, last_component.size));
  }

  EXPECT_EQ(0, anylibs_fs_iter_destroy(&iter));
  EXPECT_EQ(0, anylibs_fs_delete_recursively(&pathbuf));
}

UTEST(anylibs_fs_path_t, last_component)
{
  anylibs_fs_path_t last_component;

  anylibs_fs_path_t path = CPATH("/folder/file");
  EXPECT_EQ(0, anylibs_fs_path_get_last_component(path, &last_component));
  EXPECT_LT(0U, last_component.size);
  EXPECT_STRNEQ("file", last_component.data, last_component.size);

  path = CPATH("/folder/file/.");
  EXPECT_EQ(0, anylibs_fs_path_get_last_component(path, &last_component));
  EXPECT_LT(0U, last_component.size);
  EXPECT_STRNEQ(".", last_component.data, last_component.size);

  path = CPATH("/folder/file///");
  EXPECT_EQ(0, anylibs_fs_path_get_last_component(path, &last_component));
  EXPECT_LT(0U, last_component.size);
  EXPECT_STRNEQ("file", last_component.data, last_component.size);

  path = CPATH("file");
  EXPECT_EQ(0, anylibs_fs_path_get_last_component(path, &last_component));
  EXPECT_LT(0U, last_component.size);
  EXPECT_STRNEQ("file", last_component.data, last_component.size);

  path = CPATH("////");
  EXPECT_EQ(0, anylibs_fs_path_get_last_component(path, &last_component));
  EXPECT_LT(0U, last_component.size);
  EXPECT_STRNEQ("/", last_component.data, last_component.size);
}
