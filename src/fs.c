#include "anylibs/fs.h"
#include "anylibs/allocator.h"
#include "anylibs/error.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
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
      return -1;                                    \
    }                                               \
    if ((path)[(path_len)] != '\0') {               \
      c_error_set(C_ERROR_none_terminated_raw_str); \
      return -1;                                    \
    }                                               \
  } while (0)

CFile* c_fs_file_open(CPath path, char const mode[], size_t mode_size)
{
  /// validation
  if (!(path.data) || (path.size) == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return NULL;
  }
  if ((path.data)[path.size] != '\0') {
    c_error_set(C_ERROR_none_terminated_raw_str);
    return NULL;
  }
  if (!mode || mode_size == 0) {
    c_error_set(C_ERROR_fs_invalid_open_mode);
    return NULL;
  }
  if (mode[mode_size] != '\0') {
    c_error_set(C_ERROR_none_terminated_raw_str);
    return NULL;
  }
  enum { MAX_MODE_LEN       = 3,
         MAX_FINAL_MODE_LEN = 15 };
  if (mode_size > MAX_MODE_LEN) {
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
  FILE* f = fopen(path.data, mode);
  if (!f) {
    c_error_set(errno);
    return NULL;
  }
#endif

  return (CFile*)f;
}

int c_fs_file_size(CFile* self, size_t* out_file_size)
{
  assert(self);

  if (!out_file_size) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }

  clearerr((FILE*)self);
  errno           = 0;
  int fseek_state = fseek((FILE*)self, 0, SEEK_END);
  if (fseek_state != 0) {
    c_error_set(ferror((FILE*)self));
    return -1;
  }

  *out_file_size = ftell((FILE*)self);
  errno          = 0;
  fseek_state    = fseek((FILE*)self, 0, SEEK_SET); /* same as rewind(f); */
  if (fseek_state != 0) {
    c_error_set(ferror((FILE*)self));
    return -1;
  }

  return 0;
}

size_t c_fs_file_read(CFile* self, char buf[], size_t elements_count, size_t element_size)
{
  assert(self);
  assert(buf);

  clearerr((FILE*)self);
  size_t read_size = fread(buf, element_size, elements_count, (FILE*)self);

  if (read_size > 0) {
    return read_size;
  } else {
    c_error_set(ferror((FILE*)self));
    return 0;
  }
}

size_t c_fs_file_write(CFile* self, char buf[], size_t elements_count, size_t element_size)
{
  assert(self);
  assert(buf);

  clearerr((FILE*)self);
  size_t write_size = fwrite(buf, element_size, elements_count, (FILE*)self);

  if (write_size > 0) {
    return write_size;
  } else {
    c_error_set(ferror((FILE*)self));
    return 0;
  }
}

int c_fs_file_flush(CFile* self)
{
  assert(self);

  errno      = 0;
  int status = fflush((FILE*)self);
  if (status) {
    c_error_set(errno);
    return -1;
  }

  return 0;
}

int c_fs_file_close(CFile* self)
{
  if (self) {
    clearerr((FILE*)self);
    errno           = 0;
    int close_state = fclose((FILE*)self);

    if (close_state != 0) {
      c_error_set(errno);
      return -1;
    }
  }

  return 0;
}

int c_fs_path_append(CPathBuf* base_path, CPath path)
{
  assert(base_path);

  if (!base_path->data || base_path->size == 0 || !path.data || path.size == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return -1;
  }
  if (((base_path->capacity - base_path->size) < path.size + 1)) {
    c_error_set(C_ERROR_invalid_capacity);
    return -1;
  }

  base_path->data[base_path->size++] = C_FS_PATH_SEP[0];
  memcpy(base_path->data + base_path->size, path.data, path.size);
  base_path->size += path.size;
  base_path->data[base_path->size] = '\0';

  return 0;
}

int c_fs_path_to_absolute(CPath path, CPathBuf* in_out_abs_path)
{
  c_fs_path_validate(path.data, path.size);
  if (!in_out_abs_path) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }
  if (in_out_abs_path->capacity < path.size) {
    c_error_set(C_ERROR_capacity_full);
    return -1;
  }

/// calculate the required capacity
#ifdef _WIN32
  SetLastError(0);
  DWORD abs_path_len = GetFullPathName(path.data, in_out_abs_path->data, in_out_abs_path->capacity, NULL);
  if (required_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  } else {
    in_out_abs_path->size = abs_path_len;
    return 0;
  }
#else
  errno               = 0;
  char* absolute_path = realpath(path.data, NULL);
  if (!absolute_path) {
    c_error_set((c_error_t)errno);
    return -1;
  }
  size_t abs_path_len = strlen(absolute_path);

  if (abs_path_len >= in_out_abs_path->capacity) {
    free(absolute_path);
    c_error_set(C_ERROR_capacity_full);
    return -1;
  }

  memcpy(in_out_abs_path->data, absolute_path, abs_path_len);
  free(absolute_path);
  in_out_abs_path->size                        = abs_path_len;
  in_out_abs_path->data[in_out_abs_path->size] = '\0';

  return 0;
#endif
}

int c_fs_path_is_absolute(CPath path)
{
  c_fs_path_validate(path.data, path.size);

#ifdef _WIN32
  return PathIsRelativeA(path.data) ? 0 : 1;
#else
  return path.data[0] == C_FS_PATH_SEP[0] ? 0 : 1;
#endif
}

int c_fs_path_parent(CPath* path)
{
  if (!path || !path->data || path->size == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return -1;
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

  char const* ptr = path->data + path->size - 1;
  skip_multiple_separators(ptr, path->data);

  // skip_to_separator
  skip_to_separator(ptr, path->data);
  skip_multiple_separators(ptr, path->data);

  if (ptr < path->data) {
    *path = (CPath){0};
    return -1;
  }

  path->size = (ptrdiff_t)ptr - (ptrdiff_t)path->data + 1;
  return 0;
}

char c_fs_path_separator(void)
{
  return C_FS_PATH_SEP[0];
}

size_t c_fs_path_max_len(void)
{
  return MAX_PATH_LEN;
}

int c_fs_path_filename(CPath* path)
{
  if (!path || !path->data || path->size == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return -1;
  }

  size_t      counter = 0;
  char const* ptr     = path->data + path->size;
  while (--ptr >= path->data) {
    if (*ptr == '/' || *ptr == '\\') {
      path->data = ptr + 1;
      path->size = counter;
      return 0;
    }
    counter++;
  }

  return -1;
}

int c_fs_path_filestem(CPath* path)
{
  int status = c_fs_path_filename(path);
  if (status) return status;

  size_t      counter = 0;
  char const* ptr     = path->data + path->size;
  while (--ptr >= path->data) {
    if (*ptr == '.') {
      path->size = counter + 1;
      return 0;
    }
    counter++;
  }

  return 0;
}

int c_fs_path_file_extension(CPath* path)
{
  if (!path || !path->data || path->size == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return -1;
  }

  size_t      counter = 0;
  char const* ptr     = path->data + path->size;
  while (--ptr >= path->data) {
    if (*ptr == '.') {
      path->data = ptr + 1;
      path->size = counter + 1;
      return 0;
    }
    counter++;
  }

  return -1;
}

// CIter c_fs_path_iter(CPath path)
// {
//   CIter iter     = {0};
//   iter.data      = path.data;
//   iter.data_size = path.size;
//   iter.step_size = sizeof(char);
//   iter.ptr       = path.data + path.size;

//   return iter;
// }

// int c_fs_path_iter_component_next(CIter* iter, CStr* out_component)
// {
//   if (!iter) {
//     c_error_set(C_ERROR_null_ptr);
//     return -1;
//   }

//   out_component->size = 0;

//   // skip multiple separators
//   for (; ((iter->ptr) >= iter->data) &&
//          (*(char*)(iter->ptr) == '/' || *(char*)(iter->ptr) == '\\');
//        iter->ptr = (char*)(iter->ptr) - 1) {
//   }
//   // skip to separator
//   for (; ((iter->ptr) >= iter->data) &&
//          (*(char*)(iter->ptr) != '/' && *(char*)(iter->ptr) != '\\');
//        iter->ptr = (char*)(iter->ptr) - 1) {
//     out_component->size++;
//   }

//   if (iter->ptr < iter->data)
//     return -1;

//   out_component->data = (char*)iter->ptr + 1;
//   return 0;
// }

int c_fs_path_metadata(CPath path, CFsMetadata* out_metadata)
{
  c_fs_path_validate(path.data, path.size);
  if (!out_metadata) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }

  CFsMetadata metadata = {0};

#ifdef _WIN32
  errno = 0;
  struct _stat s;
  int          status = _stat(path.data, &s);
  if (status) {
    c_error_set(errno);
    return -1;
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
    HANDLE          hfind = FindFirstFile(path.data, &findFileData);
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
    return -1;
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

  return 0;
}

int c_fs_dir_create(CPath dir_path)
{
#if defined(_WIN32)
  SetLastError(0);
  BOOL dir_created = CreateDirectoryA(dir_path.data, NULL);
  if (!dir_created) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }
#else
  errno          = 0;
  int dir_status = mkdir(dir_path.data, ~umask(0));
  if (dir_status != 0) {
    c_error_set((c_error_t)errno);
    return -1;
  }
#endif

  return 0;
}

int c_fs_is_dir(CPath const dir_path)
{
#if defined(_WIN32)
  SetLastError(0);
  size_t path_attributes = GetFileAttributesA(dir_path.data);
  if (path_attributes == INVALID_FILE_ATTRIBUTES) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  } else if ((path_attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
    return 0;
  } else {
    return 1;
  }
#else
  struct stat sb      = {0};
  errno               = 0;
  int path_attributes = stat(dir_path.data, &sb);
  if (path_attributes != 0) {
    c_error_set((c_error_t)errno);
    return -1;
  } else if (S_ISDIR(sb.st_mode) == 0) {
    return 1; // not a directory
  } else {
    return 0;
  }
#endif
}

int c_fs_is_file(CPath path)
{
  int is_dir = c_fs_is_dir(path);
  return is_dir != -1 ? !is_dir : -1;
}

int c_fs_is_symlink(CPath const path)
{
#if defined(_WIN32)
  SetLastError(0);
  WIN32_FIND_DATA findFileData;
  HANDLE          hfind = FindFirstFile(path.data, &findFileData);
  if (hfind == INVALID_HANDLE_VALUE) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }
  FindClose(hfind);

  // Check if the file has the reparse point attribute
  if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
    // Further check if it is a symbolic link
    if (findFileData.dwReserved0 == IO_REPARSE_TAG_SYMLINK) {
      return 0;
    }
  }

  return 1;
#else
  struct stat sb      = {0};
  errno               = 0;
  int path_attributes = stat(path.data, &sb);
  if (path_attributes != 0) {
    c_error_set((c_error_t)errno);
    return -1;
  } else if (S_ISLNK(sb.st_mode) == 0) {
    return 1; // not a symlink
  } else {
    return 0;
  }
#endif
}

int c_fs_dir_current(CPathBuf* in_out_dir_path)
{
  size_t path_len = 0;

#ifdef _WIN32
  SetLastError(0);
  path_len = (size_t)GetCurrentDirectory(in_out_dir_path->capacity, in_out_dir_path->data);
  if (path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }
#else
  errno         = 0;
  char* cur_dir = getcwd(in_out_dir_path->data, in_out_dir_path->capacity);
  if (!cur_dir) {
    c_error_set((c_error_t)errno);
    return -1;
  }
  path_len = strlen(cur_dir);
#endif
  in_out_dir_path->size = path_len;
  return 0;
}

int c_fs_dir_current_exe(CPathBuf* in_out_dir_path)
{
  size_t path_len = 0;

#ifdef _WIN32
  SetLastError(0);
  path_len = (size_t)GetModuleFileName(NULL, in_out_dir_path->data, in_out_dir_path->capacity);
  if (path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }
#else
  errno              = 0;
  ssize_t result_len = readlink("/proc/self/exe", in_out_dir_path->data, in_out_dir_path->capacity - 1);
  if (result_len == -1) {
    c_error_set((c_error_t)errno);
    return -1;
  }

  path_len                        = result_len;
  in_out_dir_path->data[path_len] = '\0';
#endif

  in_out_dir_path->size = path_len;
  return 0;
}

int c_fs_dir_change_current(CPath new_path)
{
  c_fs_path_validate(new_path.data, new_path.size);

#ifdef _WIN32
  SetLastError(0);
  BOOL status = SetCurrentDirectory(new_path.data);
  if (!status) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }
#else
  errno      = 0;
  int status = chdir(new_path.data);
  if (status) {
    c_error_set((c_error_t)errno);
    return -1;
  }
#endif

  return 0;
}

int c_fs_dir_is_empty(CPath path)
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

  FindClose(find_handler);
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

  closedir(dir);
  return result;
#endif
}

int c_fs_exists(CPath const path)
{
  assert(path.data);

#if defined(_WIN32)
  SetLastError(0);
  DWORD stat_status = GetFileAttributesA(path.data);
  if (stat_status == INVALID_FILE_ATTRIBUTES) {
    DWORD last_error = GetLastError();
    if (last_error == ERROR_PATH_NOT_FOUND) {
      return 1;
    } else {
      c_error_set(last_error);
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

int c_fs_delete(CPath path)
{
  c_fs_path_validate(path.data, path.size);

#if defined(_WIN32)
  BOOL remove_dir_status = RemoveDirectoryA(path.data);
  if (!remove_dir_status) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }

  return 0;
#endif

  errno             = 0;
  int remove_status = remove(path.data);
  if (remove_status) {
    c_error_set((c_error_t)errno);
    return -1;
  }

  return 0;
}

// int c_fs_delete_recursively(CPathBuf* path)
// {
//   assert(path);

//   CFsIter iter = c_fs_iter(path);

//   CPath cur_path = {0};
//   while (c_fs_iter_next(&iter, &cur_path)) {
//     // skip . and ..
//     CPath filename;
//     c_fs_path_filename(cur_path, &filename);
//     if ((strcmp(".", filename.data) == 0) || (strcmp("..", filename.data) == 0))
//       continue;

//     if (c_fs_is_dir(cur_path) == 0) {
//       int status = c_fs_delete_recursively(iter.pathbuf);
//       if (!status)
//         return status;
//     } else {
//       c_fs_delete(cur_path);
//     }
//   }

//   int status = c_fs_iter_close(&iter);
//   if (!status)
//     return -1;

//   if (cur_path.data) {
//     return c_fs_delete(cur_path);
//   } else {
//     c_error_set(C_ERROR_fs_invalid_path);
//     return -1;
//   }
// }

// CFsIter c_fs_iter(CPathBuf* path)
// {
//   CFsIter iter = {0};
//   iter.pathbuf = path;

//   return iter;
// }

// int c_fs_iter_next(CFsIter* iter, CStr* out_cur_path)
// {
//   assert(iter);

//   if (!iter || !iter->pathbuf) {
//     c_error_set(C_ERROR_invalid_iterator);
//     return -1;
//   }

// #if defined(_WIN32)
//   // if (!iter->cur_dir) {
//   //   errno         = 0;
//   //   iter->cur_dir = opendir(iter->pathbuf->data);
//   //   if (!iter->cur_dir) {
//   //     c_error_set(errno);
//   //     return -1;
//   //   }
//   // }

//   WIN32_FIND_DATAA cur_file;
//   if (!iter->cur_dir) {
//     iter->old_len = c_str_len(iter->pathbuf);
//     int status    = c_str_push(iter->pathbuf, CSTR("\\*"));
//     if (!status) return -1;

//     SetLastError(0);
//     HANDLE find_handler = FindFirstFileA(iter->pathbuf->data, &cur_file);
//     if (find_handler == INVALID_HANDLE_VALUE) {
//       c_error_set(GetLastError());
//       return -1;
//     }

//     iter->cur_dir = find_handler;
//   } else {
//     int status = FindNextFileA((HANDLE)iter->cur_dir, &cur_file);
//     if (!status) return -1;
//   }

//   // c_str_set_len(iter->pathbuf, iter->old_len);

//   // if (find_handler == INVALID_HANDLE_VALUE) {
//   //   c_error_set(GetLastError());
//   //   return -1;
//   // }

//   // skip '.' and '..'
//   // if ((strcmp(cur_file.cFileName, ".") == 0) || (strcmp(cur_file.cFileName, "..") == 0)) continue;

//   ////
//   size_t filename_len = strlen(cur_file.cFileName);
//   // iter->old_len       = c_str_len(iter->pathbuf);
//   // int status         = c_str_push(iter->pathbuf, (CStr){.data = cur_file.cFileName, .len = filename_len});
//   // if (!status) return -1;
//   ////

//   c_str_set_len(iter->pathbuf, iter->old_len);
//   iter->old_len = c_str_len(iter->pathbuf);

//   int status = c_str_push(iter->pathbuf, CSTR(C_FS_PATH_SEP));
//   if (!status) return status;
//   status = c_str_push(iter->pathbuf, (CStr){.data = cur_file.cFileName, .len = filename_len});
//   if (!status) return status;

//     // size_t old_len         = path_buf_len;
//     // path_buf_len           = path_buf_len - 1 + filename_len;
//     // path_buf[path_buf_len] = '\0';
//     // c_error_t err = handler(path_buf, path_buf_len, extra_data);
//     // path_buf_len  = old_len;
//     // if (err.code != C_ERROR_none.code) {
//     //   break;
//     // }

//     // SetLastError(0);
//     // if (!FindClose(find_handler)) {
//     //   path_buf[orig_path_len] = '\0';
//     //   err                     = (c_error_t)GetLastError();
//     //   goto Error;
//     // }
// #else
//   if (!iter->cur_dir) {
//     errno         = 0;
//     iter->cur_dir = opendir(iter->pathbuf->data);
//     if (!iter->cur_dir) {
//       c_error_set(errno);
//       return -1;
//     }
//   }

//   struct dirent* cur_dir_properties = readdir(iter->cur_dir);
//   if (!(cur_dir_properties))
//     return -1;

//   size_t filename_len = strlen(cur_dir_properties->d_name);

//   if (iter->old_len) c_str_set_len(iter->pathbuf, iter->old_len);
//   iter->old_len = c_str_len(iter->pathbuf);

//   int status = c_str_push(iter->pathbuf, CSTR(C_FS_PATH_SEP));
//   if (!status) return status;
//   status = c_str_push(iter->pathbuf, (CStr){cur_dir_properties->d_name, filename_len});
//   if (!status) return status;
// #endif

//   if (out_cur_path) *out_cur_path = c_cstrbuf_to_cstr(iter->pathbuf);
//   return 0;
// }

// int c_fs_iter_close(CFsIter* iter)
// {
//   if (iter && iter->cur_dir) {
// #ifdef _WIN32
//     SetLastError(0);
//     if (!FindClose((HANDLE)iter->cur_dir)) {
//       c_error_set(GetLastError());
//       return -1;
//     }
// #else
//     errno = 0;
//     if (closedir(iter->cur_dir) != 0) {
//       c_error_set(errno);
//       return -1;
//     }
// #endif
//     c_str_set_len(iter->pathbuf, iter->old_len);
//     *iter = (CFsIter){0};
//   }

//   return 0;
// }

// ------------------------- internal ------------------------- //

#ifdef _MSC_VER
#pragma warning(pop)
#endif
