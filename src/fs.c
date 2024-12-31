#include "anylibs/fs.h"
#include "anylibs/allocator.h"
#include "anylibs/error.h"
#include "anylibs/str.h"
#include "internal/vec.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <shlwapi.h>
#include <windows.h>
#else
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
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

// #define s ((CStrBuf*)(s))

// static c_error_t internal_c_fs_delete_recursively_handler(char path[], size_t
// path_len, void*  extra_data); static bool
// internal_c_fs_delete_recursively(CStrBuf* path);

static inline bool c_fs_path_validate(char const *path, size_t path_len) {
  /// validation
  if (!path || path_len == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return false;
  }
  if (path[path_len] != '\0') {
    c_error_set(C_ERROR_none_terminated_raw_str);
    return false;
  }

  return true;
}

// CStr c_fs_path_create(CStr path_raw, CAllocator* allocator)
// {
//   /// validation
//   if (!path_raw.data || path_raw.len == 0) {
//     c_error_set(C_ERROR_fs_invalid_path);
//     return NULL;
//   }
//   if (path_raw.data[path_raw.len] != '\0') {
//     c_error_set(C_ERROR_none_terminated_raw_str);
//     return NULL;
//   }

//   CStrBuf* path = c_str_create_from_raw(path_raw, false, allocator);
//   if (!path) return NULL;

//   return (CStr)path;
// }

// void c_fs_path_destroy(CStr self)
// {
//   if (self) c_str_destroy((CStrBuf*)self);
// }

// CStrBuf* c_fs_pathbuf_create(CStr path_raw, CAllocator* allocator)
// {
//   /// validation
//   if (!path_raw.data || path_raw.len == 0) {
//     c_error_set(C_ERROR_fs_invalid_path);
//     return NULL;
//   }
//   if (path_raw.data[path_raw.len] != '\0') {
//     c_error_set(C_ERROR_none_terminated_raw_str);
//     return NULL;
//   }

//   CStrBuf* path = c_str_create_from_raw(path_raw, true, allocator);
//   if (!path) return NULL;

//   return (CStrBuf*)path;
// }

// void c_fs_pathbuf_destroy(CStrBuf* self)
// {
//   if (self) c_str_destroy((CStrBuf*)self);
// }

CFile *c_fs_file_open(CStr path, CStr mode) {
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
  enum { MAX_MODE_LEN = 3, MAX_FINAL_MODE_LEN = 15 };
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
  FILE *f = fopen(path.data, mode_);
  if (!f) {
    c_error_set(GetLastError());
    return NULL;
  }
#else
  errno = 0;
  FILE *f = fopen(path.data, mode.data);
  if (!f) {
    c_error_set(errno);
    return NULL;
  }
#endif

  return (CFile *)f;
}

bool c_fs_file_size(CFile *self, size_t *out_file_size) {
  assert(self);

  if (!out_file_size) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  clearerr((FILE *)self);
  errno = 0;
  int fseek_state = fseek((FILE *)self, 0, SEEK_END);
  if (fseek_state != 0) {
    c_error_set(ferror((FILE *)self));
    return false;
  }

  *out_file_size = ftell((FILE *)self);
  errno = 0;
  fseek_state = fseek((FILE *)self, 0, SEEK_SET); /* same as rewind(f); */
  if (fseek_state != 0) {
    c_error_set(ferror((FILE *)self));
    return false;
  }

  return true;
}

bool c_fs_file_read(CFile *self, CStrBuf *buf, size_t *out_read_size) {
  assert(self);
  assert(buf);

  // if (!out_read_size) {
  //   c_error_set(C_ERROR_null_ptr);
  //   return false;
  // }

  clearerr((FILE *)self);
  size_t read_size = fread(buf->data, sizeof(*buf->data),
                           c_str_capacity(buf) - 1, (FILE *)self);

  if (out_read_size)
    *out_read_size = read_size;
  if (read_size > 0) {
    c_str_set_len(buf, read_size);
    return true;
  } else {
    c_error_set(ferror((FILE *)self));
    return false;
  }
}

bool c_fs_file_write(CFile *self, CStr buf, size_t *out_write_size) {
  assert(self);

  clearerr((FILE *)self);
  size_t write_size =
      fwrite(buf.data, sizeof(*buf.data), buf.len, (FILE *)self);

  if (out_write_size)
    *out_write_size = write_size;
  if (write_size > 0) {
    return true;
  } else {
    c_error_set(ferror((FILE *)self));
    return false;
  }
}

bool c_fs_file_flush(CFile *self) {
  assert(self);

  errno = 0;
  int status = fflush((FILE *)self);
  if (status) {
    c_error_set(errno);
    return false;
  }

  return true;
}

bool c_fs_file_close(CFile *self) {
  if (self) {
    clearerr((FILE *)self);
    errno = 0;
    int close_state = fclose((FILE *)self);

    if (close_state != 0) {
      c_error_set(errno);
      return false;
    }
  }

  return true;
}

bool c_fs_path_append(CStrBuf *base_path, CStr path) {
  assert(base_path);

  bool status = c_str_push(base_path, CSTR((char[]){C_FS_PATH_SEP}));
  if (!status)
    return status;

  status = c_str_push(base_path, path);
  if (!status)
    return status;

  return true;
}

CStrBuf *c_fs_path_to_absolute(CStr path, CAllocator *allocator) {
/// calculate the required capacity
#ifdef _WIN32
  SetLastError(0);
  DWORD required_len = GetFullPathName(path.data, 0, NULL, NULL);
  if (required_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return NULL;
  }
#else
  errno = 0;
  char *absolute_path = realpath(path.data, NULL);
  if (!absolute_path) {
    c_error_set((c_error_t)errno);
    return NULL;
  }
  size_t required_len = strlen(absolute_path);
#endif

  CStrBuf *abs_path =
      c_str_create_with_capacity(required_len + 1, allocator, false);
  if (!abs_path)
    return NULL;

// get the absolute path
#ifdef _WIN32
  SetLastError(0);
  DWORD abs_path_len =
      GetFullPathName(path.data, (DWORD)required_len, abs_path->data, NULL);
  return abs_path_len > 0 ? C_ERROR_none : (c_error_t)GetLastError();
#else
  memcpy(abs_path->data, absolute_path, required_len);
  c_str_set_len(abs_path, required_len);
  free(absolute_path);
  return (CStrBuf *)abs_path;
#endif
}

bool c_fs_path_is_absolute(CStr path) {
#ifdef _WIN32
  return !PathIsRelativeA(path.data);
#else
  return path.data[0] == C_FS_PATH_SEP[0];
#endif
}

// c_error_t
// c_fs_path_to_parent(CStrBuf* path)
// {
//   size_t path_len;
//   c_str_len(path, &path_len);

//   // get rid of last slashes and backslashes
//   while (path.data[--path_len] == '\\') {}
//   path_len++;
//   while (path.data[--path_len] == '/') {}
//   path_len++;
//   char old_ch          = path.data[path_len];
//   path.data[path_len] = '\0';

//   while ((--path_len > 0)
//          && (path.data[path_len] != '\\' && path.data[path_len] != '/')) {}

//   if (path_len != MAX_PATH_LEN) {
//     // path.data[path_len] = '\0'; // Terminate the string at the last
//     // backslash
//     c_error_t err = c_str_set_len(path, path_len - 1);
//     return err;
//   } else {
//     path.data[path_len] = old_ch;
//     if (out_new_path_len) { *out_new_path_len = 0; }
//     return C_ERROR_fs_invalid_open_mode;
//   }
// }

char c_fs_path_separator(void) { return C_FS_PATH_SEP[0]; }

size_t c_fs_path_max_len(void) { return MAX_PATH_LEN; }

bool c_fs_dir_create(CStr dir_path) {
#if defined(_WIN32)
  SetLastError(0);
  BOOL dir_created = CreateDirectoryA(dir_path.data, NULL);
  if (!dir_created)
    c_error_set((c_error_t)GetLastError());
  return dir_created;
#else
  errno = 0;
  int dir_status = mkdir(dir_path.data, ~umask(0));
  if (dir_status != 0)
    c_error_set((c_error_t)errno);
  return dir_status == 0;
#endif
}

int c_fs_is_dir(CStr const dir_path) {
#if defined(_WIN32)
  SetLastError(0);
  size_t path_attributes = GetFileAttributesA(dir_path.data);
  if (path_attributes == INVALID_FILE_ATTRIBUTES) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }

  else if ((path_attributes & FILE_ATTRIBUTE_DIRECTORY) ==
           FILE_ATTRIBUTE_DIRECTORY) {
    return 0;
  }

  // c_error_set((c_error_t)ERROR_DIRECTORY);
  return 1;
#else
  struct stat sb = {0};
  errno = 0;
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

CStrBuf *c_fs_dir_current(CAllocator *allocator) {
  size_t required_path_len;

#ifdef _WIN32
  SetLastError(0);
  required_path_len = (size_t)GetCurrentDirectory(NULL, 0);
  if (required_path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return NULL;
  }
#else
  errno = 0;
  char *cur_dir = getcwd(NULL, 0);
  if (!cur_dir) {
    c_error_set((c_error_t)errno);
    return NULL;
  }
  required_path_len = strlen(cur_dir);
#endif

  CStrBuf *result_cur_dir =
      c_str_create_with_capacity(required_path_len + 1, allocator, false);
  if (!result_cur_dir)
    return NULL;

  c_str_set_len(result_cur_dir, required_path_len);

#ifdef _WIN32
  SetLastError(0);
  DWORD result_path_len =
      GetCurrentDirectory(result_cur_dir.data, required_path_len);
  if (result_path_len <= 0) {
    c_error_set((c_error_t)GetLastError());
    c_str_destroy(result_cur_dir);
    return NULL;
  }
#else
  memcpy(result_cur_dir->data, cur_dir, required_path_len);
  free(cur_dir);
#endif

  return (CStrBuf *)result_cur_dir;
}

CStrBuf *c_fs_dir_current_exe(CAllocator *allocator) {
  size_t expected_path_len = 0;
  CStrBuf *cur_exe_path = c_str_create_with_capacity(255, allocator, false);
  if (!cur_exe_path)
    return NULL;
  size_t path_capacity = c_str_capacity(cur_exe_path);

#ifdef _WIN32
  SetLastError(0);
  expected_path_len =
      (size_t)GetModuleFileNameA(NULL, cur_exe_path->data, path_capacity);
  if (expected_path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    goto ON_ERROR;
  }
#else
  errno = 0;
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
    if (!status)
      goto ON_ERROR;
#ifdef _WIN32
    SetLastError(0);
    expected_path_len =
        (size_t)GetModuleFileName(NULL, path_buf, path_buf_capacity);
    if (expected_path_len == 0) {
      c_error_set((c_error_t)GetLastError());
      goto ON_ERROR;
    }
#else
    errno = 0;
    ssize_t len = readlink("/proc/self/exe", cur_exe_path->data, path_capacity);
    if (len == -1) {
      c_error_set((c_error_t)errno);
      goto ON_ERROR;
    }
    expected_path_len = len;
#endif
  }

  c_str_set_len(cur_exe_path, expected_path_len);
  return (CStrBuf *)cur_exe_path;

ON_ERROR:
  c_str_destroy(cur_exe_path);
  return NULL;
}

bool c_fs_dir_change_current(CStr new_path) {
#ifdef _WIN32
  SetLastError(0);
  BOOL status = SetCurrentDirectory(path.data);
  if (!status)
    c_error_set((c_error_t)GetLastError());
  return status;
#else
  errno = 0;
  int status = chdir(new_path.data);
  if (status)
    c_error_set((c_error_t)errno);
  return status == 0;
#endif
}

int c_fs_dir_is_empty(CStr path) {
  int result = 0;

#ifdef _WIN32
  SetLastError(0);
  WIN32_FIND_DATAA cur_file;
  HANDLE find_handler = FindFirstFileA(path.data, &cur_file);
  do {
    if (find_handler == INVALID_HANDLE_VALUE) {
      c_error_set((c_error_t)GetLastError());
      return -1;
    }

    // skip '.' and '..'
    if ((strcmp(cur_file.cFileName, ".") == 0) ||
        (strcmp(cur_file.cFileName, "..") == 0)) {
      continue;
    }

    // Found a file or directory
    result = 1;
    break;
  } while (FindNextFileA(find_handler, &cur_file));

  SetLastError(0);
  if (!FindClose(find_handler)) {
    c_error_set((c_error_t)GetLastError());
    result = -1
  } else {
    result = 0;
  }
  return result;
#else
  errno = 0;

  struct dirent *entry;
  DIR *dir = opendir(path.data);

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

int c_fs_exists(CStr const path) {
  assert(path.data);

#if defined(_WIN32)
  SetLastError(0);
  DWORD stat_status = GetFileAttributesA(path.data);
  if (stat_status == INVALID_FILE_ATTRIBUTES) {
    DWORD last_error = GetLastError();
    if (last_error == ERROR_FILE_NOT_FOUND ||
        last_error == ERROR_PATH_NOT_FOUND) {
      return 1;
    } else {
      c_error_set((c_error_t)last_error);
      return -1;
    }
  }

  return 0;
#else
  struct stat path_stats = {0};
  errno = 0;
  int stat_status = stat(path.data, &path_stats);
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

bool c_fs_delete(CStr const path) {
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

  errno = 0;
  int remove_status = remove(path.data);
  if (remove_status != 0) {
    c_error_set((c_error_t)errno);
    return false;
  }

  return true;
}

bool c_fs_delete_recursively(CStrBuf *path) {
  assert(path);

  CFsIter iter = c_fs_iter(path);

  CStr cur_path;
  while (c_fs_iter_next(&iter, &cur_path)) {
    if (c_fs_is_dir(cur_path) != 0) {
      c_fs_delete(cur_path);
    } else {
      bool status = c_fs_delete_recursively(iter.pathbuf);
      if (!status)
        return status;
      c_fs_delete(cur_path);
    }
  }

  return c_fs_iter_close(&iter);
}

CFsIter c_fs_iter(CStrBuf *path) {
  CFsIter iter = {0};
  iter.pathbuf = path;

  return iter;
}

// CResultVoidPtr c_fs_iter_default_step_callback(CFsIter* iter,
//                                                CStrBuf* cur_path,
//                                                size_t   _placeholder)
// {
//   (void)_placeholder;
//   CResultVoidPtr result = {0};

//   if (iter->base.is_reversed) {
//     c_error_set(C_ERROR_invalid_iterator);
//     return result;
//   }

//   if (iter->old_len == 0) {
//     iter->old_len = c_str_len(cur_path);
//   } else {
//     c_str_set_len(cur_path, iter->old_len);
//   }

//   CResultBool dir_exists = c_fs_dir_exists(c_CStrbuf_to_CStr(cur_path));
//   if (!dir_exists.is_ok || !dir_exists.is_true) return result;

//     // size_t orig_path_len = c_str_len(cur_path);

// #if defined(_WIN32)
//   path_buf[path_buf_len] = C_FS_PATH_SEP;
//   path_buf[path_buf_len + 1] = '*';
//   path_buf[path_buf_len + 2] = '\0';
//   path_buf_len += 2;

//   SetLastError(0);
//   WIN32_FIND_DATAA cur_file;
//   HANDLE           find_handler = FindFirstFileA(path_buf, &cur_file);
//   do {
//     if (find_handler == INVALID_HANDLE_VALUE) {
//       path_buf[path_buf_len - 2] = '\0';
//       err = (c_error_t)GetLastError();
//       goto Error;
//     }

//     // skip '.' and '..'
//     if ((strcmp(cur_file.cFileName, ".") != 0) && (strcmp(cur_file.cFileName,
//     "..") != 0)) {
//       size_t filename_len = strnlen(cur_file.cFileName, MAX_PATH_LEN);
//       strncpy(path_buf + path_buf_len - 1, cur_file.cFileName, filename_len);

//       size_t old_len = path_buf_len;
//       path_buf_len = path_buf_len - 1 + filename_len;
//       path_buf[path_buf_len] = '\0';
//       c_error_t err = handler(path_buf, path_buf_len, extra_data);
//       path_buf_len = old_len;
//       if (err.code != C_ERROR_none.code) { break; }
//     }
//   } while (FindNextFileA(find_handler, &cur_file));

//   SetLastError(0);
//   if (!FindClose(find_handler)) {
//     path_buf[orig_path_len] = '\0';
//     err = (c_error_t)GetLastError();
//     goto Error;
//   }
// #else

//   if (!iter->cur_dir) {
//     errno = 0;
//     iter->cur_dir = opendir(cur_path.data);
//     if (!iter->cur_dir) {
//       c_error_set(errno);
//       return result;
//     }
//   }

//   struct dirent* cur_dir_properties = NULL;
//   while ((cur_dir_properties = readdir(iter->cur_dir)) != NULL) {
//     if ((strcmp(cur_dir_properties->d_name, ".") == 0) ||
//     (strcmp(cur_dir_properties->d_name, "..") == 0)) {
//       continue;
//     }
//     size_t filename_len = strlen(cur_dir_properties->d_name);

//     // if (c_str_len(cur_path) + filename_len
//     //     >= c_str_capacity(cur_dir)) {
//     //   bool resized = c_str_set_capacity(
//     //       cur_path, c_str_capacity(cur_dir) + filename_len
//     //                            + sizeof(C_FS_PATH_SEP) - 1);
//     //   if (!resized) return result;
//     // }

//     // cur_path.data[c_str_len(cur_path)] = C_FS_PATH_SEP;
//     // strncpy(cur_path.data + c_str_len(cur_path) + 1,
//     //         cur_dir_properties->d_name, filename_len);

//     iter->old_len = c_str_len(cur_path);

//     bool status = c_str_push(cur_path, CSTR(C_FS_PATH_SEP));
//     if (!status) return result;
//     status = c_str_push(cur_path,
//                         (CStr){cur_dir_properties->d_name, filename_len});
//     if (!status) return result;
//     break;

//     // size_t old_len         = c_str_len(cur_path);
//     // path_buf_len           = path_buf_len + 1 + filename_len;
//     // path_buf[path_buf_len] = '\0';
//     // c_error_t handler_err  = handler(path_buf, path_buf_len, extra_data);
//     // path_buf_len           = old_len;
//     // if (handler_err.code != C_ERROR_none.code) { break; }

//     // errno = 0;
//   }

//   if (!(cur_dir_properties)) {
//     if (closedir(iter->cur_dir) != 0) {
//       // path_buf[orig_path_len] = '\0';
//       // err                     = (c_error_t)errno;
//       // goto Error;
//       c_error_set(C_ERROR_fs_close_failed);
//       return result;
//     } else {
//       result.is_ok = true;
//       return result;
//     }
//   }

//   // if (closedir(cur_dir) != 0) {
//   //   path_buf[orig_path_len] = '\0';
//   //   err                     = (c_error_t)errno;
//   //   goto Error;
//   // }
// #endif

//   // path_buf[orig_path_len] = '\0';

//   // puts(cur_path.data);

//   result.vp = cur_path;
//   result.is_ok = true;
//   return result;
// }

bool c_fs_iter_next(CFsIter *iter, CStr *out_cur_path) {
  assert(iter);

  // if (iter->old_len == 0) {
  //   iter->old_len = c_str_len(cur_path);
  // } else {
  //   c_str_set_len(cur_path, iter->old_len);
  // }

  // size_t orig_path_len = c_str_len(cur_path);

#if defined(_WIN32)
  path_buf[path_buf_len] = C_FS_PATH_SEP;
  path_buf[path_buf_len + 1] = '*';
  path_buf[path_buf_len + 2] = '\0';
  path_buf_len += 2;

  SetLastError(0);
  WIN32_FIND_DATAA cur_file;
  HANDLE find_handler = FindFirstFileA(path_buf, &cur_file);
  do {
    if (find_handler == INVALID_HANDLE_VALUE) {
      path_buf[path_buf_len - 2] = '\0';
      err = (c_error_t)GetLastError();
      goto Error;
    }

    // skip '.' and '..'
    if ((strcmp(cur_file.cFileName, ".") != 0) &&
        (strcmp(cur_file.cFileName, "..") != 0)) {
      size_t filename_len = strnlen(cur_file.cFileName, MAX_PATH_LEN);
      strncpy(path_buf + path_buf_len - 1, cur_file.cFileName, filename_len);

      size_t old_len = path_buf_len;
      path_buf_len = path_buf_len - 1 + filename_len;
      path_buf[path_buf_len] = '\0';
      c_error_t err = handler(path_buf, path_buf_len, extra_data);
      path_buf_len = old_len;
      if (err.code != C_ERROR_none.code) {
        break;
      }
    }
  } while (FindNextFileA(find_handler, &cur_file));

  SetLastError(0);
  if (!FindClose(find_handler)) {
    path_buf[orig_path_len] = '\0';
    err = (c_error_t)GetLastError();
    goto Error;
  }
#else
  if (!iter || !iter->pathbuf) {
    c_error_set(C_ERROR_invalid_iterator);
    return false;
  }
  if (!iter->cur_dir) {
    errno = 0;
    iter->cur_dir = opendir(iter->pathbuf->data);
    if (!iter->cur_dir) {
      c_error_set(errno);
      return false;
    }
  }

  // while ((cur_dir_properties = readdir(iter->cur_dir)) != NULL) {
  struct dirent *cur_dir_properties = readdir(iter->cur_dir);
  if (!(cur_dir_properties)) {
    // if (closedir(iter->cur_dir) != 0) c_error_set(C_ERROR_fs_close_failed);
    return false;
  }
  // if ((strcmp(cur_dir_properties->d_name, ".") == 0) ||
  // (strcmp(cur_dir_properties->d_name, "..") == 0)) {
  //   continue;
  // }
  size_t filename_len = strlen(cur_dir_properties->d_name);

  // if (c_str_len(cur_path) + filename_len
  //     >= c_str_capacity(cur_dir)) {
  //   bool resized = c_str_set_capacity(
  //       cur_path, c_str_capacity(cur_dir) + filename_len
  //                            + sizeof(C_FS_PATH_SEP) - 1);
  //   if (!resized) return result;
  // }

  // cur_path.data[c_str_len(cur_path)] = C_FS_PATH_SEP;
  // strncpy(cur_path.data + c_str_len(cur_path) + 1,
  //         cur_dir_properties->d_name, filename_len);

  if (iter->old_len)
    c_str_set_len(iter->pathbuf, iter->old_len);
  iter->old_len = c_str_len(iter->pathbuf);

  bool status = c_str_push(iter->pathbuf, CSTR(C_FS_PATH_SEP));
  if (!status)
    return status;
  status = c_str_push(iter->pathbuf,
                      (CStr){cur_dir_properties->d_name, filename_len});
  if (!status)
    return status;

    // size_t old_len         = c_str_len(cur_path);
    // path_buf_len           = path_buf_len + 1 + filename_len;
    // path_buf[path_buf_len] = '\0';
    // c_error_t handler_err  = handler(path_buf, path_buf_len, extra_data);
    // path_buf_len           = old_len;
    // if (handler_err.code != C_ERROR_none.code) { break; }

    // errno = 0;
    // }

    // if (closedir(cur_dir) != 0) {
    //   path_buf[orig_path_len] = '\0';
    //   err                     = (c_error_t)errno;
    //   goto Error;
    // }
#endif

  // path_buf[orig_path_len] = '\0';

  // puts(cur_path.data);

  if (out_cur_path)
    *out_cur_path = c_cstrbuf_to_cstr(iter->pathbuf);
  return true;
}

bool c_fs_iter_close(CFsIter *iter) {
  if (iter) {
    if (closedir(iter->cur_dir) != 0) {
      c_error_set(C_ERROR_fs_close_failed);
      return false;
    }
    *iter = (CFsIter){0};
  }

  return true;
}

// c_error_t
// c_fs_foreach(CStrBuf* path,
//              c_error_t handler(char* path, size_t path_len, void*
//              extra_data), void*     extra_data)
// {
//   assert(path_buf && path_buf_len > 0 && path_buf_capacity > 0);
//   assert(path_buf[path_buf_len] == '\0');
//   assert(handler);

//   bool      dir_exists;
//   c_error_t err = c_fs_dir_exists(path_buf, path_buf_len, &dir_exists);
//   if (!dir_exists) { return err; }

//   size_t orig_path_len = path_buf_len;

// #if defined(_WIN32)
//   path_buf[path_buf_len]     = C_FS_PATH_SEP;
//   path_buf[path_buf_len + 1] = '*';
//   path_buf[path_buf_len + 2] = '\0';
//   path_buf_len += 2;

//   SetLastError(0);
//   WIN32_FIND_DATAA cur_file;
//   HANDLE           find_handler = FindFirstFileA(path_buf, &cur_file);
//   do {
//     if (find_handler == INVALID_HANDLE_VALUE) {
//       path_buf[path_buf_len - 2] = '\0';
//       err                        = (c_error_t)GetLastError();
//       goto Error;
//     }

//     // skip '.' and '..'
//     if ((strcmp(cur_file.cFileName, ".") != 0)
//         && (strcmp(cur_file.cFileName, "..") != 0)) {
//       size_t filename_len = strnlen(cur_file.cFileName, MAX_PATH_LEN);
//       strncpy(path_buf + path_buf_len - 1, cur_file.cFileName, filename_len);

//       size_t old_len         = path_buf_len;
//       path_buf_len           = path_buf_len - 1 + filename_len;
//       path_buf[path_buf_len] = '\0';
//       c_error_t err          = handler(path_buf, path_buf_len, extra_data);
//       path_buf_len           = old_len;
//       if (err.code != C_ERROR_none.code) { break; }
//     }
//   } while (FindNextFileA(find_handler, &cur_file));

//   SetLastError(0);
//   if (!FindClose(find_handler)) {
//     path_buf[orig_path_len] = '\0';
//     err                     = (c_error_t)GetLastError();
//     goto Error;
//   }
// #else

//   DIR* cur_dir = opendir(path_buf);
//   if (cur_dir) {
//     errno = 0;

//     struct dirent* cur_dir_properties;
//     while ((cur_dir_properties = readdir(cur_dir)) != NULL) {
//       if ((strcmp(cur_dir_properties->d_name, ".") != 0)
//           && (strcmp(cur_dir_properties->d_name, "..") != 0)) {
//         size_t filename_len    = strlen(cur_dir_properties->d_name);
//         path_buf[path_buf_len] = C_FS_PATH_SEP;
//         strncpy(path_buf + path_buf_len + 1, cur_dir_properties->d_name,
//                 filename_len);

//         size_t old_len         = path_buf_len;
//         path_buf_len           = path_buf_len + 1 + filename_len;
//         path_buf[path_buf_len] = '\0';
//         c_error_t handler_err  = handler(path_buf, path_buf_len, extra_data);
//         path_buf_len           = old_len;
//         if (handler_err.code != C_ERROR_none.code) { break; }
//       }

//       errno = 0;
//     }

//     if (closedir(cur_dir) != 0) {
//       path_buf[orig_path_len] = '\0';
//       err                     = (c_error_t)errno;
//       goto Error;
//     }
//   }
// #endif

//   path_buf[orig_path_len] = '\0';

// Error:
//   return err;
// }

// CStr c_CStrbuf_to_CStr(CStrBuf* path_buf)
// {
//   if (!path_buf) return NULL;
//   return (CStr)path_buf;
// }

// CStrBuf* c_CStr_to_CStrbuf(CStr path)
// {
//   if (!path) return NULL;
//   return (CStrBuf*)path;
// }

// ------------------------- internal ------------------------- //
// c_error_t
// internal_c_fs_delete_recursively_handler(char   path[],
//                                          size_t path_len,
//                                          void*  extra_data)
// {
//   size_t path_capacity = *(size_t*)extra_data;

//   bool is_dir;
//   c_fs_dir_exists(path, path_len, &is_dir);

//   if (is_dir) {
//     c_error_t err
//         = internal_c_fs_delete_recursively(path, path_len, path_capacity);
//     if (err.code != C_ERROR_none.code) { return err; }
//   } else {
//     c_error_t delete_err = c_fs_delete(path, path_len);
//     if (delete_err.code != C_ERROR_none.code) { return delete_err; }
//   }

//   return C_ERROR_none;
// }

// bool
// internal_c_fs_delete_recursively(CStrBuf* path)
// {
//   bool status = c_fs_foreach(path, internal_c_fs_delete_recursively_handler);

//   if (!status) return false;

//   // delete the directory itself after deleting all his children
//   status = c_fs_delete(path);
//   return status;
// }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

////////////////////////
// struct CFsIter_v2 {
//   CStrBuf*    cur_path;
//   size_t      old_len;
//   void*       cur_dir;
//   CAllocator* allocator;
// };

// CFsIter_v2* c_fs_iter_create_v2(CStrBuf* path, CAllocator* allocator)
// {
//   CFsIter_v2* iter = c_allocator_alloc(allocator ? allocator :
//   c_allocator_default(),
//                                        c_allocator_alignas(CFsIter_v2, 1),
//                                        true);
//   if (!iter) return NULL;

//   iter->allocator = allocator ? allocator : c_allocator_default();
//   iter->cur_path = path;
//   iter->old_len = 0;

//   return iter;
// }

// void c_fs_iter_destroy_v2(CFsIter_v2* self)
// {
//   if (self) {
//     CAllocator* allocator = self->allocator;
// #ifdef _WIN32
// #else
//     if (closedir(self->cur_dir) != 0) c_error_set(C_ERROR_fs_close_failed);
// #endif
//     *self = (CFsIter_v2){0};
//     c_allocator_free(allocator, self);
//   }
// }

// bool c_fs_iter_default_step_callback_v2(CFsIter_v2* iter,
//                                         bool        is_prev,
//                                         CStr*       out_cur_path)
// {
//   if (is_prev) return false;

//   if (iter->old_len == 0) {
//     iter->old_len = c_str_len(TOSTR(iter->cur_path));
//   } else {
//     c_str_set_len(TOSTR(iter->cur_path), iter->old_len);
//   }

//   CResultBool dir_exists =
//   c_fs_dir_exists(c_CStrbuf_to_CStr(iter->cur_path)); if (!dir_exists.is_ok
//   || !dir_exists.is_true) return false;

//     // size_t orig_path_len = c_str_len(cur_path);

// #if defined(_WIN32)
//   path_buf[path_buf_len] = C_FS_PATH_SEP;
//   path_buf[path_buf_len + 1] = '*';
//   path_buf[path_buf_len + 2] = '\0';
//   path_buf_len += 2;

//   SetLastError(0);
//   WIN32_FIND_DATAA cur_file;
//   HANDLE           find_handler = FindFirstFileA(path_buf, &cur_file);
//   do {
//     if (find_handler == INVALID_HANDLE_VALUE) {
//       path_buf[path_buf_len - 2] = '\0';
//       err = (c_error_t)GetLastError();
//       goto Error;
//     }

//     // skip '.' and '..'
//     if ((strcmp(cur_file.cFileName, ".") != 0) && (strcmp(cur_file.cFileName,
//     "..") != 0)) {
//       size_t filename_len = strnlen(cur_file.cFileName, MAX_PATH_LEN);
//       strncpy(path_buf + path_buf_len - 1, cur_file.cFileName, filename_len);

//       size_t old_len = path_buf_len;
//       path_buf_len = path_buf_len - 1 + filename_len;
//       path_buf[path_buf_len] = '\0';
//       c_error_t err = handler(path_buf, path_buf_len, extra_data);
//       path_buf_len = old_len;
//       if (err.code != C_ERROR_none.code) { break; }
//     }
//   } while (FindNextFileA(find_handler, &cur_file));

//   SetLastError(0);
//   if (!FindClose(find_handler)) {
//     path_buf[orig_path_len] = '\0';
//     err = (c_error_t)GetLastError();
//     goto Error;
//   }
// #else

//   if (!iter->cur_dir) {
//     errno = 0;
//     iter->cur_dir = opendir(TOSTR(iter->cur_path)->data);
//     if (!iter->cur_dir) {
//       c_error_set(errno);
//       return false;
//     }
//   }

//   struct dirent* cur_dir_properties = NULL;
//   while ((cur_dir_properties = readdir(iter->cur_dir)) != NULL) {
//     if ((strcmp(cur_dir_properties->d_name, ".") == 0) ||
//     (strcmp(cur_dir_properties->d_name, "..") == 0)) {
//       continue;
//     }
//     size_t filename_len = strlen(cur_dir_properties->d_name);

//     // if (c_str_len(cur_path) + filename_len
//     //     >= c_str_capacity(cur_dir)) {
//     //   bool resized = c_str_set_capacity(
//     //       cur_path, c_str_capacity(cur_dir) + filename_len
//     //                            + sizeof(C_FS_PATH_SEP) - 1);
//     //   if (!resized) return result;
//     // }

//     // cur_path.data[c_str_len(cur_path)] = C_FS_PATH_SEP;
//     // strncpy(cur_path.data + c_str_len(cur_path) + 1,
//     //         cur_dir_properties->d_name, filename_len);

//     iter->old_len = c_str_len(TOSTR(iter->cur_path));

//     bool status = c_str_push(TOSTR(iter->cur_path), CSTR(C_FS_PATH_SEP));
//     if (!status) return false;
//     status = c_str_push(TOSTR(iter->cur_path),
//                         (CStr){cur_dir_properties->d_name, filename_len});
//     if (!status) return false;
//     break;

//     // size_t old_len         = c_str_len(cur_path);
//     // path_buf_len           = path_buf_len + 1 + filename_len;
//     // path_buf[path_buf_len] = '\0';
//     // c_error_t handler_err  = handler(path_buf, path_buf_len, extra_data);
//     // path_buf_len           = old_len;
//     // if (handler_err.code != C_ERROR_none.code) { break; }

//     // errno = 0;
//   }

//   if (!(cur_dir_properties)) {
//     return false;
//     // if (closedir(iter->cur_dir) != 0) {
//     //   // path_buf[orig_path_len] = '\0';
//     //   // err                     = (c_error_t)errno;
//     //   // goto Error;
//     //   c_error_set(C_ERROR_fs_close_failed);
//     //   return false;
//     // } else {
//     //   // result.is_ok = true;
//     //   return false;
//     // }
//   }

//   // if (closedir(cur_dir) != 0) {
//   //   path_buf[orig_path_len] = '\0';
//   //   err                     = (c_error_t)errno;
//   //   goto Error;
//   // }
// #endif

//   // path_buf[orig_path_len] = '\0';

//   // puts(cur_path.data);

//   // result.vp    = cur_path;
//   // result.is_ok = true;
//   if (out_cur_path) *out_cur_path = c_CStrbuf_to_CStr(iter->cur_path);
//   return true;
// }

// bool c_fs_iter_next_v2(CFsIter_v2* iter, CStr* out_cur_path)
// {
//   assert(iter);

//   bool status = c_fs_iter_default_step_callback_v2(iter, false,
//   out_cur_path);

//   return status;
// }
