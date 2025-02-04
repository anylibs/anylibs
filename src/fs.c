#include "anylibs/fs.h"
#include "anylibs/allocator.h"
#include "anylibs/error.h"
#include "anylibs/str.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <shlwapi.h>
#include <windows.h>
#else
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define C_FS_PATH_SEP "\\"
#define MAX_PATH_LEN INT16_MAX
#else
#define MAX_PATH_LEN FILENAME_MAX
#define C_FS_PATH_SEP "/"
#endif

#if _WIN32 && (!_MSC_VER || !(_MSC_VER >= 1900))
#error "You need MSVC must be higher that or equal to 1900"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // disable warning about unsafe functions
#pragma comment(lib, "Shlwapi.lib")
#endif

#define c_fs_path_validate(path, path_len)          \
  do {                                              \
    if (!(path) || (path_len) == 0) {               \
      c_error_set(C_ERROR_fs_invalid_path);         \
      return false;                                 \
    }                                               \
    if ((path)[(path_len)] != '\0') {               \
      c_error_set(C_ERROR_none_terminated_raw_str); \
      return false;                                 \
    }                                               \
  } while (0)

CFile* c_fs_file_open(CStr path, CStr mode)
{
  /// validation
  c_fs_path_validate(path.data, path.len);
  if (!mode.data || mode.len == 0) {
    c_error_set(C_ERROR_fs_invalid_open_mode);
    return NULL;
  }
  if (mode.data[mode.len] != '\0') {
    c_error_set(C_ERROR_none_terminated_raw_str);
    return NULL;
  }
  enum { MAX_MODE_LEN       = 3,
         MAX_FINAL_MODE_LEN = 15 };
  if (mode.len > MAX_MODE_LEN) {
    c_error_set(C_ERROR_fs_invalid_open_mode);
    return NULL;
  }

  // if (c_fs_is_dir(path) == 0) {
  //   c_error_set(C_ERROR_fs_is_dir);
  //   return NULL;
  // }

#if defined(_WIN32)
  char mode_[MAX_FINAL_MODE_LEN] = {0};
  memcpy(mode_, mode.data, MAX_MODE_LEN);
  memcpy(mode_ + mode.len, "b, ccs=UTF-8", MAX_FINAL_MODE_LEN - MAX_MODE_LEN);

  SetLastError(0);
  FILE* f = fopen(path.data, mode_);
  if (!f) {
    c_error_set(GetLastError());
    return NULL;
  }
#else
  errno   = 0;
  FILE* f = fopen(path.data, mode.data);
  if (!f) {
    c_error_set(errno);
    return NULL;
  }
#endif

  return (CFile*)f;
}

bool c_fs_file_size(CFile* self, size_t* out_file_size)
{
  assert(self);

  if (!out_file_size) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  clearerr((FILE*)self);
  errno           = 0;
  int fseek_state = fseek((FILE*)self, 0, SEEK_END);
  if (fseek_state != 0) {
    c_error_set(ferror((FILE*)self));
    return false;
  }

  *out_file_size = ftell((FILE*)self);
  errno          = 0;
  fseek_state    = fseek((FILE*)self, 0, SEEK_SET); /* same as rewind(f); */
  if (fseek_state != 0) {
    c_error_set(ferror((FILE*)self));
    return false;
  }

  return true;
}

bool c_fs_file_read(CFile* self, CStrBuf* buf, size_t* out_read_size)
{
  assert(self);
  assert(buf);

  // if (!out_read_size) {
  //   c_error_set(C_ERROR_null_ptr);
  //   return false;
  // }

  clearerr((FILE*)self);
  size_t read_size = fread(buf->data, sizeof(*buf->data),
                           c_str_capacity(buf) - 1, (FILE*)self);

  if (out_read_size)
    *out_read_size = read_size;
  if (read_size > 0) {
    c_str_set_len(buf, read_size);
    return true;
  } else {
    c_error_set(ferror((FILE*)self));
    return false;
  }
}

bool c_fs_file_write(CFile* self, CStr buf, size_t* out_write_size)
{
  assert(self);

  clearerr((FILE*)self);
  size_t write_size = fwrite(buf.data, sizeof(*buf.data), buf.len, (FILE*)self);

  if (out_write_size)
    *out_write_size = write_size;
  if (write_size > 0) {
    return true;
  } else {
    c_error_set(ferror((FILE*)self));
    return false;
  }
}

bool c_fs_file_flush(CFile* self)
{
  assert(self);

  errno      = 0;
  int status = fflush((FILE*)self);
  if (status) {
    c_error_set(errno);
    return false;
  }

  return true;
}

bool c_fs_file_close(CFile* self)
{
  if (self) {
    clearerr((FILE*)self);
    errno           = 0;
    int close_state = fclose((FILE*)self);

    if (close_state != 0) {
      c_error_set(errno);
      return false;
    }
  }

  return true;
}

bool c_fs_path_append(CStrBuf* base_path, CStr path)
{
  assert(base_path);

  if (!base_path->data || c_str_len(base_path) == 0 || !path.data || path.len == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return false;
  }

  bool status = c_str_push(base_path, CSTR((char[]){C_FS_PATH_SEP}));
  if (!status)
    return status;

  status = c_str_push(base_path, path);
  if (!status)
    return status;

  return true;
}

CStrBuf* c_fs_path_to_absolute(CStr path, CAllocator* allocator)
{
  c_fs_path_validate(path.data, path.len);

/// calculate the required capacity
#ifdef _WIN32
  SetLastError(0);
  DWORD required_len = GetFullPathName(path.data, 0, NULL, NULL);
  if (required_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return NULL;
  }
#else
  errno               = 0;
  char* absolute_path = realpath(path.data, NULL);
  if (!absolute_path) {
    c_error_set((c_error_t)errno);
    return NULL;
  }
  size_t required_len = strlen(absolute_path);
#endif

  CStrBuf* abs_path = c_str_create_with_capacity(required_len + 1, allocator, false);
  if (!abs_path) return NULL;

// get the absolute path
#ifdef _WIN32
  SetLastError(0);
  DWORD abs_path_len = GetFullPathName(path.data, (DWORD)required_len, abs_path->data, NULL);
  if (abs_path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return NULL;
  }
  c_str_set_len(abs_path, abs_path_len);
#else
  memcpy(abs_path->data, absolute_path, required_len);
  c_str_set_len(abs_path, required_len);
  free(absolute_path);
#endif
  return (CStrBuf*)abs_path;
}

bool c_fs_path_is_absolute(CStr path)
{
  c_fs_path_validate(path.data, path.len);

#ifdef _WIN32
  return !PathIsRelativeA(path.data);
#else
  return path.data[0] == C_FS_PATH_SEP[0];
#endif
}

#define skip_multiple_separators(ptr, orig_data)                               \
  for (; ((ptr) >= orig_data) && (*(ptr) == '/' || *(ptr) == '\\'); (ptr)--) { \
  }
#define skip_to_separator(ptr, orig_data)  \
  for (; (ptr) >= orig_data; (ptr)--) {    \
    if (*(ptr) == '/' || *(ptr) == '\\') { \
      break;                               \
    }                                      \
  }
bool c_fs_path_parent(CStr path, CStr* out_parent)
{
  if (!path.data || path.len == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return false;
  }
  if (!out_parent) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  char* ptr = path.data + path.len - 1;
  skip_multiple_separators(ptr, path.data);

  // skip_to_separator
  skip_to_separator(ptr, path.data);
  skip_multiple_separators(ptr, path.data);

  if (ptr < path.data) {
    *out_parent = (CStr){0};
    return false;
  }

  out_parent->data = path.data;
  out_parent->len  = (ptrdiff_t)ptr - (ptrdiff_t)path.data + 1;
  return true;
}

char c_fs_path_separator(void)
{
  return C_FS_PATH_SEP[0];
}

size_t c_fs_path_max_len(void)
{
  return MAX_PATH_LEN;
}

bool c_fs_path_filename(CStr path, CStr* out_filename)
{
  c_fs_path_validate(path.data, path.len);

  if (!out_filename) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  size_t counter = 0;
  char*  ptr     = path.data + path.len;
  while (--ptr >= path.data) {
    if (*ptr == '/' || *ptr == '\\') {
      out_filename->data = ptr + 1;
      out_filename->len  = counter;
      return true;
    }
    counter++;
  }

  return false;
}

bool c_fs_path_filestem(CStr path, CStr* out_filestem)
{
  bool status = c_fs_path_filename(path, out_filestem);
  if (!status)
    return status;

  size_t counter = 0;
  char*  ptr     = out_filestem->data + out_filestem->len;
  while (--ptr >= out_filestem->data) {
    if (*ptr == '.') {
      // out_filestem->data = out_filestem->data;
      out_filestem->len = counter + 1;
      return true;
    }
    counter++;
  }

  return true;
}

bool c_fs_path_file_extension(CStr path, CStr* out_file_extension)
{
  c_fs_path_validate(path.data, path.len);
  if (!out_file_extension) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  size_t counter = 0;
  char*  ptr     = path.data + path.len;
  while (--ptr >= path.data) {
    if (*ptr == '.') {
      out_file_extension->data = ptr + 1;
      out_file_extension->len  = counter + 1;
      return true;
    }
    counter++;
  }

  return false;
}

CIter c_fs_path_iter(CStr path)
{
  CIter iter     = {0};
  iter.data      = path.data;
  iter.data_size = path.len;
  iter.step_size = sizeof(char);
  iter.ptr       = path.data + path.len;

  return iter;
}

bool c_fs_path_iter_component_next(CIter* iter, CStr* out_component)
{
  if (!iter) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  out_component->len = 0;

  // skip multiple separators
  for (; ((iter->ptr) >= iter->data) &&
         (*(char*)(iter->ptr) == '/' || *(char*)(iter->ptr) == '\\');
       iter->ptr = (char*)(iter->ptr) - 1) {
  }
  // skip to separator
  for (; ((iter->ptr) >= iter->data) &&
         (*(char*)(iter->ptr) != '/' && *(char*)(iter->ptr) != '\\');
       iter->ptr = (char*)(iter->ptr) - 1) {
    out_component->len++;
  }

  if (iter->ptr < iter->data)
    return false;

  out_component->data = (char*)iter->ptr + 1;
  return true;
}

bool c_fs_path_metadata(CStr path, CFsMetadata* out_metadata)
{
  c_fs_path_validate(path.data, path.len);
  if (path.data[path.len] != '\0') {
    c_error_set(C_ERROR_none_terminated_raw_str);
    return false;
  }
  if (!out_metadata) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  CFsMetadata metadata = {0};

#ifdef _WIN32
  errno = 0;
  struct _stat s;
  int          status = _stat(path.data, &s);
  if (status) {
    c_error_set(errno);
    return false;
  }

  // file type
  metadata.ftype = C_FS_FILE_TYPE_unknown;
  if ((s.st_mode & _S_IFMT) == _S_IFDIR) {
    metadata.ftype = C_FS_FILE_TYPE_dir;
  } else if ((s.st_mode & _S_IFMT) == _S_IFREG) {
    metadata.ftype = C_FS_FILE_TYPE_file;
  } else {
    // Use FindFirstFile to check for reparse point attributes
    WIN32_FIND_DATA findFileData;
    SetLastError(0);
    HANDLE hfind = FindFirstFile(path.data, &findFileData);
    if (hfind == INVALID_HANDLE_VALUE) {
      c_error_set(GetLastError());
      return false;
    }
    FindClose(hfind);

    // Check if the file has the reparse point attribute
    if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
      // Further check if it is a symbolic link
      if (findFileData.dwReserved0 == IO_REPARSE_TAG_SYMLINK) {
        metadata.ftype = C_FS_FILE_TYPE_symlink;
      }
    }
  }
  // file size
  metadata.fsize = s.st_size;
  // permissions
  metadata.fperm = (s.st_mode & _S_IWRITE) ? C_FS_FILE_PERMISSION_read_write : C_FS_FILE_PERMISSION_read_only;
  // times
  metadata.created_time  = s.st_ctime;
  metadata.last_accessed = s.st_atime;
  metadata.last_modified = s.st_mtime;

  *out_metadata = metadata;
#else
  errno = 0;
  struct stat s;
  int         status = stat(path.data, &s);
  if (status) {
    c_error_set(errno);
    return false;
  }

  // file type
  switch (s.st_mode & S_IFMT) {
    case S_IFDIR: metadata.ftype = C_FS_FILE_TYPE_dir; break;
    case S_IFREG: metadata.ftype = C_FS_FILE_TYPE_file; break;
    case S_IFLNK: metadata.ftype = C_FS_FILE_TYPE_symlink; break;
    default:      metadata.ftype = C_FS_FILE_TYPE_unknown; break;
  }
  // file size
  metadata.fsize = s.st_size;
  // permissions
  metadata.fperm = (s.st_mode & S_IWUSR) ? C_FS_FILE_PERMISSION_read_write : C_FS_FILE_PERMISSION_read_only;
  // times
  metadata.created_time  = s.st_ctim.tv_sec;
  metadata.last_accessed = s.st_atim.tv_sec;
  metadata.last_modified = s.st_mtim.tv_sec;

  *out_metadata = metadata;
#endif

  return true;
}

bool c_fs_dir_create(CStr dir_path)
{
#if defined(_WIN32)
  SetLastError(0);
  BOOL dir_created = CreateDirectoryA(dir_path.data, NULL);
  if (!dir_created)
    c_error_set((c_error_t)GetLastError());
  return dir_created;
#else
  errno          = 0;
  int dir_status = mkdir(dir_path.data, ~umask(0));
  if (dir_status != 0)
    c_error_set((c_error_t)errno);
  return dir_status == 0;
#endif
}

int c_fs_is_dir(CStr const dir_path)
{
#if defined(_WIN32)
  SetLastError(0);
  size_t path_attributes = GetFileAttributesA(dir_path.data);
  if (path_attributes == INVALID_FILE_ATTRIBUTES) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }

  else if ((path_attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
    return 0;
  }

  // c_error_set((c_error_t)ERROR_DIRECTORY);
  return 1;
#else
  struct stat sb      = {0};
  errno               = 0;
  int path_attributes = stat(dir_path.data, &sb);
  if (path_attributes != 0) {
    c_error_set((c_error_t)errno);
    return -1;
  }

  // not a directory
  if (S_ISDIR(sb.st_mode) == 0) {
    // c_error_set((c_error_t)ENOTDIR);
    return 1;
  }

  return 0;
#endif
}

int c_fs_is_file(CStr const path)
{
#if defined(_WIN32)
  SetLastError(0);
  size_t path_attributes = GetFileAttributesA(path.data);
  if (path_attributes == INVALID_FILE_ATTRIBUTES) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }

  else if ((path_attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
    return 0;
  }

  // c_error_set((c_error_t)ERROR_DIRECTORY);
  return 1;
#else
  struct stat sb      = {0};
  errno               = 0;
  int path_attributes = stat(path.data, &sb);
  if (path_attributes != 0) {
    c_error_set((c_error_t)errno);
    return -1;
  }

  // not a file
  if (S_ISREG(sb.st_mode) == 0) return 1;

  return 0;
#endif
}

int c_fs_is_symlink(CStr const path)
{
#if defined(_WIN32)
  SetLastError(0);
  size_t path_attributes = GetFileAttributesA(path.data);
  if (path_attributes == INVALID_FILE_ATTRIBUTES) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }

  else if ((path_attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
    return 0;
  }

  return 1;
#else
  struct stat sb      = {0};
  errno               = 0;
  int path_attributes = stat(path.data, &sb);
  if (path_attributes != 0) {
    c_error_set((c_error_t)errno);
    return -1;
  }

  // not a symlink
  if (S_ISLNK(sb.st_mode) == 0) return 1;

  return 0;
#endif
}

CStrBuf* c_fs_dir_current(CAllocator* allocator)
{
  size_t required_path_len;

#ifdef _WIN32
  SetLastError(0);
  required_path_len = (size_t)GetCurrentDirectory(0, NULL);
  if (required_path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return NULL;
  }
#else
  errno         = 0;
  char* cur_dir = getcwd(NULL, 0);
  if (!cur_dir) {
    c_error_set((c_error_t)errno);
    return NULL;
  }
  required_path_len = strlen(cur_dir);
#endif

  CStrBuf* result_cur_dir = c_str_create_with_capacity(required_path_len + 1, allocator, false);
  if (!result_cur_dir) return NULL;

  c_str_set_len(result_cur_dir, required_path_len);

#ifdef _WIN32
  SetLastError(0);
  DWORD result_path_len = GetCurrentDirectory(required_path_len, result_cur_dir->data);
  if (result_path_len <= 0) {
    c_error_set((c_error_t)GetLastError());
    c_str_destroy(result_cur_dir);
    return NULL;
  }
#else
  memcpy(result_cur_dir->data, cur_dir, required_path_len);
  free(cur_dir);
#endif

  return (CStrBuf*)result_cur_dir;
}

CStrBuf* c_fs_dir_current_exe(CAllocator* allocator)
{
  size_t   expected_path_len = 0;
  CStrBuf* cur_exe_path      = c_str_create_with_capacity(255, allocator, false);
  if (!cur_exe_path)
    return NULL;
  size_t path_capacity = c_str_capacity(cur_exe_path);

#ifdef _WIN32
  SetLastError(0);
  expected_path_len = (size_t)GetModuleFileNameA(NULL, cur_exe_path->data, path_capacity);
  if (expected_path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    goto ON_ERROR;
  }
#else
  errno       = 0;
  ssize_t len = readlink("/proc/self/exe", cur_exe_path->data, path_capacity);
  if (len == -1) {
    c_error_set((c_error_t)errno);
    goto ON_ERROR;
  }
  expected_path_len = len;
#endif

  while (expected_path_len >= path_capacity) {
    path_capacity *= 2;
    bool status = c_str_set_capacity(cur_exe_path, path_capacity);
    if (!status) goto ON_ERROR;
#ifdef _WIN32
    SetLastError(0);
    expected_path_len = (size_t)GetModuleFileName(NULL, cur_exe_path->data, path_capacity);
    if (expected_path_len == 0) {
      c_error_set((c_error_t)GetLastError());
      goto ON_ERROR;
    }
#else
    errno       = 0;
    ssize_t len = readlink("/proc/self/exe", cur_exe_path->data, path_capacity);
    if (len == -1) {
      c_error_set((c_error_t)errno);
      goto ON_ERROR;
    }
    expected_path_len = len;
#endif
  }

  c_str_set_len(cur_exe_path, expected_path_len);
  return (CStrBuf*)cur_exe_path;

ON_ERROR:
  c_str_destroy(cur_exe_path);
  return NULL;
}

bool c_fs_dir_change_current(CStr new_path)
{
#ifdef _WIN32
  SetLastError(0);
  BOOL status = SetCurrentDirectory(new_path.data);
  if (!status)
    c_error_set((c_error_t)GetLastError());
  return status;
#else
  errno      = 0;
  int status = chdir(new_path.data);
  if (status)
    c_error_set((c_error_t)errno);
  return status == 0;
#endif
}

int c_fs_dir_is_empty(CStr path)
{
  int result = 0;

#ifdef _WIN32
  SetLastError(0);
  WIN32_FIND_DATAA cur_file;
  HANDLE           find_handler = FindFirstFileA(path.data, &cur_file);
  do {
    if (find_handler == INVALID_HANDLE_VALUE) {
      c_error_set((c_error_t)GetLastError());
      return -1;
    }

    // skip '.' and '..'
    if ((strcmp(cur_file.cFileName, ".") == 0) || (strcmp(cur_file.cFileName, "..") == 0)) {
      continue;
    }

    // Found a file or directory
    result = 1;
    break;
  } while (FindNextFileA(find_handler, &cur_file));

  SetLastError(0);
  if (!FindClose(find_handler)) {
    c_error_set((c_error_t)GetLastError());
    result = -1;
  } else {
    result = 0;
  }
  return result;
#else
  errno = 0;

  struct dirent* entry;
  DIR*           dir = opendir(path.data);

  if (!dir) {
    c_error_set((c_error_t)errno); // Error opening the directory
    return -1;
  }

  while ((entry = readdir(dir)) != NULL) {
    // Skip the entries "." and ".."
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Found a file or directory
    result = 1;
    break;
  }

  errno = 0;
  if (closedir(dir) != 0) {
    c_error_set((c_error_t)errno);
    result = -1;
  } else {
    result = 0;
  }
  return result;
#endif
}

int c_fs_exists(CStr const path)
{
  assert(path.data);

#if defined(_WIN32)
  SetLastError(0);
  DWORD stat_status = GetFileAttributesA(path.data);
  if (stat_status == INVALID_FILE_ATTRIBUTES) {
    DWORD last_error = GetLastError();
    if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND) {
      return 1;
    } else {
      c_error_set((c_error_t)last_error);
      return -1;
    }
  }

  return 0;
#else
  struct stat path_stats = {0};
  errno                  = 0;
  int stat_status        = stat(path.data, &path_stats);
  if (stat_status == 0) {
    return 0;
  } else if (errno == ENOENT) {
    /// WARN: this could also means dangling pointer
    return 1;
  } else {
    c_error_set((c_error_t)errno);
    return -1;
  }
#endif
}

bool c_fs_delete(CStr const path)
{
  assert(path.data);

#if defined(_WIN32)
  DWORD dwAttrib = GetFileAttributesA(path.data);

  if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
    SetLastError(0);
    BOOL remove_dir_status = RemoveDirectoryA(path.data);
    if (!remove_dir_status) {
      c_error_set((c_error_t)GetLastError());
      return false;
    }

    return true;
  }
#endif

  errno             = 0;
  int remove_status = remove(path.data);
  if (remove_status != 0) {
    c_error_set((c_error_t)errno);
    return false;
  }

  return true;
}

bool c_fs_delete_recursively(CStrBuf* path)
{
  assert(path);

  CFsIter iter = c_fs_iter(path);

  CStr cur_path = {0};
  while (c_fs_iter_next(&iter, &cur_path)) {
    // skip . and ..
    CStr filename;
    c_fs_path_filename(cur_path, &filename);
    if ((strcmp(".", filename.data) == 0) || (strcmp("..", filename.data) == 0))
      continue;

    if (c_fs_is_dir(cur_path) == 0) {
      bool status = c_fs_delete_recursively(iter.pathbuf);
      if (!status)
        return status;
    } else {
      c_fs_delete(cur_path);
    }
  }

  bool status = c_fs_iter_close(&iter);
  if (!status)
    return false;

  if (cur_path.data) {
    return c_fs_delete(cur_path);
  } else {
    c_error_set(C_ERROR_fs_invalid_path);
    return false;
  }
}

CFsIter c_fs_iter(CStrBuf* path)
{
  CFsIter iter = {0};
  iter.pathbuf = path;

  return iter;
}

bool c_fs_iter_next(CFsIter* iter, CStr* out_cur_path)
{
  assert(iter);

  if (!iter || !iter->pathbuf) {
    c_error_set(C_ERROR_invalid_iterator);
    return false;
  }

#if defined(_WIN32)
  // if (!iter->cur_dir) {
  //   errno         = 0;
  //   iter->cur_dir = opendir(iter->pathbuf->data);
  //   if (!iter->cur_dir) {
  //     c_error_set(errno);
  //     return false;
  //   }
  // }

  WIN32_FIND_DATAA cur_file;
  if (!iter->cur_dir) {
    iter->old_len = c_str_len(iter->pathbuf);
    bool status   = c_str_push(iter->pathbuf, CSTR("\\*"));
    if (!status) return false;

    SetLastError(0);
    HANDLE find_handler = FindFirstFileA(iter->pathbuf->data, &cur_file);
    if (find_handler == INVALID_HANDLE_VALUE) {
      c_error_set(GetLastError());
      return false;
    }

    iter->cur_dir = find_handler;
  } else {
    bool status = FindNextFileA((HANDLE)iter->cur_dir, &cur_file);
    if (!status) return false;
  }

  // c_str_set_len(iter->pathbuf, iter->old_len);

  // if (find_handler == INVALID_HANDLE_VALUE) {
  //   c_error_set(GetLastError());
  //   return false;
  // }

  // skip '.' and '..'
  // if ((strcmp(cur_file.cFileName, ".") == 0) || (strcmp(cur_file.cFileName, "..") == 0)) continue;

  ////
  size_t filename_len = strlen(cur_file.cFileName);
  // iter->old_len       = c_str_len(iter->pathbuf);
  // bool status         = c_str_push(iter->pathbuf, (CStr){.data = cur_file.cFileName, .len = filename_len});
  // if (!status) return false;
  ////

  c_str_set_len(iter->pathbuf, iter->old_len);
  iter->old_len = c_str_len(iter->pathbuf);

  bool status = c_str_push(iter->pathbuf, CSTR(C_FS_PATH_SEP));
  if (!status) return status;
  status = c_str_push(iter->pathbuf, (CStr){.data = cur_file.cFileName, .len = filename_len});
  if (!status) return status;

    // size_t old_len         = path_buf_len;
    // path_buf_len           = path_buf_len - 1 + filename_len;
    // path_buf[path_buf_len] = '\0';
    // c_error_t err = handler(path_buf, path_buf_len, extra_data);
    // path_buf_len  = old_len;
    // if (err.code != C_ERROR_none.code) {
    //   break;
    // }

    // SetLastError(0);
    // if (!FindClose(find_handler)) {
    //   path_buf[orig_path_len] = '\0';
    //   err                     = (c_error_t)GetLastError();
    //   goto Error;
    // }
#else
  if (!iter->cur_dir) {
    errno         = 0;
    iter->cur_dir = opendir(iter->pathbuf->data);
    if (!iter->cur_dir) {
      c_error_set(errno);
      return false;
    }
  }

  struct dirent* cur_dir_properties = readdir(iter->cur_dir);
  if (!(cur_dir_properties))
    return false;

  size_t filename_len = strlen(cur_dir_properties->d_name);

  if (iter->old_len) c_str_set_len(iter->pathbuf, iter->old_len);
  iter->old_len = c_str_len(iter->pathbuf);

  bool status = c_str_push(iter->pathbuf, CSTR(C_FS_PATH_SEP));
  if (!status) return status;
  status = c_str_push(iter->pathbuf, (CStr){cur_dir_properties->d_name, filename_len});
  if (!status) return status;
#endif

  if (out_cur_path) *out_cur_path = c_cstrbuf_to_cstr(iter->pathbuf);
  return true;
}

bool c_fs_iter_close(CFsIter* iter)
{
  if (iter && iter->cur_dir) {
#ifdef _WIN32
    SetLastError(0);
    if (!FindClose((HANDLE)iter->cur_dir)) {
      c_error_set(GetLastError());
      return false;
    }
#else
    errno = 0;
    if (closedir(iter->cur_dir) != 0) {
      c_error_set(errno);
      return false;
    }
#endif
    c_str_set_len(iter->pathbuf, iter->old_len);
    *iter = (CFsIter){0};
  }

  return true;
}

// ------------------------- internal ------------------------- //

#ifdef _MSC_VER
#pragma warning(pop)
#endif
