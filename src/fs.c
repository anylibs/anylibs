#include "anylibs/fs.h"
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
#ifdef __APPLE__
#include <libproc.h>
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
  memcpy(mode_, mode, MAX_MODE_LEN);
  memcpy(mode_ + mode_size, "b, ccs=UTF-8", MAX_FINAL_MODE_LEN - MAX_MODE_LEN);

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
  DWORD abs_path_len = GetFullPathName(path.data, in_out_abs_path->capacity, in_out_abs_path->data, NULL);
  if (abs_path_len == 0) {
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
  return PathIsRelativeA(path.data) ? 1 : 0;
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
#ifdef __APPLE__
  metadata.created_time  = s.st_ctimespec.tv_sec;
  metadata.last_accessed = s.st_atimespec.tv_sec;
  metadata.last_modified = s.st_mtimespec.tv_sec;
#else
  metadata.created_time  = s.st_ctim.tv_sec;
  metadata.last_accessed = s.st_atim.tv_sec;
  metadata.last_modified = s.st_mtim.tv_sec;
#endif

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

int c_fs_is_dir(CPath dir_path)
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

#ifdef _WIN32
  size_t path_len = 0;
  SetLastError(0);
  path_len = (size_t)GetModuleFileName(NULL, in_out_dir_path->data, in_out_dir_path->capacity);
  if (path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }
  in_out_dir_path->size = path_len;
#elif defined(__APPLE__)
  pid_t pid = getpid(); // Get the current process ID

  errno                 = 0;
  in_out_dir_path->size = proc_pidpath(pid, in_out_dir_path->data, in_out_dir_path->capacity);
  if (in_out_dir_path->size <= 0) {
    c_error_set(errno);
    return -1;
  }
#else
  size_t path_len    = 0;
  errno              = 0;
  ssize_t result_len = readlink("/proc/self/exe", in_out_dir_path->data, in_out_dir_path->capacity - 1);
  if (result_len == -1) {
    c_error_set((c_error_t)errno);
    return -1;
  }

  path_len                        = result_len;
  in_out_dir_path->data[path_len] = '\0';
  in_out_dir_path->size           = path_len;
#endif

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
  char pbuf[c_fs_path_max_len()];
  memcpy(pbuf, path.data, path.size);
  snprintf(pbuf, c_fs_path_max_len(), "%.*s" C_FS_PATH_SEP "*", (int)path.size, path.data);

  SetLastError(0);
  WIN32_FIND_DATAA cur_file;
  HANDLE           find_handler = FindFirstFileA(pbuf, &cur_file);

  do {
    if (find_handler == INVALID_HANDLE_VALUE) {
      c_error_set((c_error_t)GetLastError());
      result = -1;
      break;
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
    if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND) {
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
#if defined(_WIN32)
  if (c_fs_is_dir(path) == 0) {
    BOOL remove_dir_status = RemoveDirectoryA(path.data);
    if (!remove_dir_status) {
      c_error_set((c_error_t)GetLastError());
      return -1;
    }
  } else {
    c_fs_path_validate(path.data, path.size);

    _doserrno         = 0;
    int remove_status = remove(path.data);
    if (remove_status) {
      c_error_set(_doserrno);
      return -1;
    }
  }
#else

  c_fs_path_validate(path.data, path.size);

  errno             = 0;
  int remove_status = remove(path.data);
  if (remove_status) {
    c_error_set(errno);
    return -1;
  }
#endif

  return 0;
}

int c_fs_delete_recursively(CPathBuf* path)
{
  assert(path);

  CFsIter iter;
  if (c_fs_iter(path, &iter) != 0) return -1;

  CPathRef cur_path_ref = {0};
  CPath    cur_path     = {0};
  int      next_status  = 0;
  while ((next_status = c_fs_iter_next(&iter, &cur_path_ref)) == 0) {
    // skip . and ..
    cur_path       = c_fs_cpathref_to_cpath(cur_path_ref);
    CPath filename = c_fs_cpathref_to_cpath(cur_path_ref);
    c_fs_path_filename(&filename);
    if ((strcmp(".", filename.data) == 0) || (strcmp("..", filename.data) == 0))
      continue;

    if (c_fs_is_dir(cur_path) == 0) {
      int err = c_fs_delete_recursively(iter.pathbuf);
      if (err == -1) goto ON_ERROR;
    } else {
      int err = c_fs_delete(cur_path);
      if (err == -1) goto ON_ERROR;
    }
  }
  if (next_status == -1) goto ON_ERROR;

  int err = c_fs_iter_close(&iter);
  if (err) return -1;

  return c_fs_delete(cur_path);
ON_ERROR:
  c_fs_iter_close(&iter);
  return -1;
}

int c_fs_iter(CPathBuf* path, CFsIter* out_iter)
{
  c_fs_path_validate(path->data, path->size);
  if (!out_iter) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }

  *out_iter         = (CFsIter){0};
  out_iter->pathbuf = path;
  out_iter->old_len = path->size;
#ifdef _WIN32
  // sizeof("\\*" + '\0')
  if (path->capacity < path->size + 3) {
    c_error_set(C_ERROR_capacity_full);
    return -1;
  }

  path->data[path->size++] = C_FS_PATH_SEP[0];
  path->data[path->size++] = '*';
  path->data[path->size]   = '\0';
#else
  errno          = 0;
  out_iter->fptr = opendir(path->data);
  if (!out_iter->fptr) {
    c_error_set(errno);
    return -1;
  }
#endif

  return 0;
}

int c_fs_iter_next(CFsIter* iter, CPathRef* out_cur_path)
{
  assert(iter);

  if (!iter || !iter->pathbuf) {
    c_error_set(C_ERROR_invalid_iterator);
    return -1;
  }

  size_t filename_len = 0;
  char*  filename;
#if defined(_WIN32)
  WIN32_FIND_DATAA cur_file;
  if (!iter->fptr) {
    SetLastError(0);
    HANDLE find_handler = FindFirstFileA(iter->pathbuf->data, &cur_file);
    if (find_handler == INVALID_HANDLE_VALUE) {
      // iter->pathbuf->data[iter->old_len] = '\0';
      c_error_set(GetLastError());
      return -1;
    }

    iter->fptr = find_handler;
  } else {
    SetLastError(0);
    bool status = FindNextFileA((HANDLE)iter->fptr, &cur_file);
    if (!status) {
      int err = GetLastError();
      if (err != ERROR_NO_MORE_FILES) {
        c_error_set(err);
        return -1;
      } else {
        return 1;
      }
    }
  }

  filename     = cur_file.cFileName;
  filename_len = strlen(filename);
#else
  errno                             = 0;
  struct dirent* cur_dir_properties = readdir(iter->fptr);
  if (!cur_dir_properties) {
    if (errno != 0) {
      c_error_set(errno);
      return -1;
    } else {
      return 1;
    }
  }

  filename     = cur_dir_properties->d_name;
  filename_len = strlen(filename);
#endif

  if (iter->pathbuf->capacity < iter->pathbuf->size + filename_len + 1) {
    c_error_set(C_ERROR_capacity_full);
    return -1;
  }

  iter->pathbuf->size = iter->old_len;

  iter->pathbuf->data[iter->pathbuf->size++] = C_FS_PATH_SEP[0];
  memcpy(iter->pathbuf->data + iter->pathbuf->size, filename, filename_len);
  iter->pathbuf->size += filename_len;
  iter->pathbuf->data[iter->pathbuf->size] = '\0';

  if (out_cur_path) *out_cur_path = c_fs_cpathbuf_to_cpathref(*iter->pathbuf);
  return 0;
}

int c_fs_iter_close(CFsIter* iter)
{
  int err = 0;
  if (!iter) return err;

  if (iter->fptr) {
#ifdef _WIN32
    SetLastError(0);
    if (!FindClose((HANDLE)iter->fptr)) {
      c_error_set(GetLastError());
      err = -1;
    }
#else
    errno = 0;
    if (closedir(iter->fptr) != 0) {
      c_error_set(errno);
      err = -1;
    }
#endif
  }

  iter->pathbuf->data[iter->old_len] = '\0';
  *iter                              = (CFsIter){0};
  return err;
}

// ------------------------- internal ------------------------- //

#ifdef _MSC_VER
#pragma warning(pop)
#endif
