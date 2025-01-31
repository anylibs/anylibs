#include "anylibs/fs.h"
#include "anylibs/error.h"

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
#else
#define C_FS_PATH_SEP "/"
#endif

#ifdef _WIN32
#define MAX_PATH_SIZE INT16_MAX
#elif defined(FILENAME_MAX)
#define MAX_PATH_SIZE FILENAME_MAX
#else
#define MAX_PATH_SIZE 4096
#endif

#if _WIN32 && (!_MSC_VER || !(_MSC_VER >= 1900))
#error "You need MSVC must be higher that or equal to 1900"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // disable warning about unsafe functions
#pragma comment(lib, "Shlwapi.lib")
#endif

#define anylibs_fs_path_validate(path, path_len)    \
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

static inline bool anylibs_fs_is_separator(char ch)
{
#ifdef WIN32
  return ch == '\\' || ch == '/';
#else
  return ch == '/';
#endif
}

anylibs_fs_path_t anylibs_fs_path_create(char const path[], size_t path_size)
{
  return (anylibs_fs_path_t){.data = path, .size = path_size};
}

anylibs_fs_pathbuf_t anylibs_fs_pathbuf_create(char pathbuf[], size_t pathbuf_size, size_t pathbuf_capacity)
{
  return (anylibs_fs_pathbuf_t){.data = pathbuf, .size = pathbuf_size, .capacity = pathbuf_capacity};
}

// void invalid_parameter_handler(
//     wchar_t const* a,
//     wchar_t const* b,
//     wchar_t const* c,
//     unsigned int   d,
//     uintptr_t      e)
// {
//   (void)a;
//   (void)b;
//   (void)c;
//   (void)d;
//   (void)e;
// }

anylibs_fs_file_t* anylibs_fs_file_open(anylibs_fs_path_t path, char const mode[], size_t mode_size)
{
  // validation
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

  // if (anylibs_fs_is_dir(path) == 0) {
  //   c_error_set(C_ERROR_fs_is_dir);
  //   return NULL;
  // }

#if defined(_WIN32)
  char mode_[MAX_FINAL_MODE_LEN] = {0};
  memcpy(mode_, mode, MAX_MODE_LEN);
  memcpy(mode_ + mode_size, "b, ccs=UTF-8", MAX_FINAL_MODE_LEN - MAX_MODE_LEN);

  // _set_invalid_parameter_handler(invalid_parameter_handler);
  // _set_thread_local_invalid_parameter_handler(NULL);
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

  // on unix you could open directories as a file
  // check and fail if the path is directory
  int fno = fileno(f);
  errno   = 0;
  struct stat statbuf;
  int status = fstat(fno, &statbuf);
  if (status != 0) {
    c_error_set(errno);
    return NULL;
  }
  if (S_ISDIR(statbuf.st_mode)) {
    c_error_set(C_ERROR_fs_is_dir);
    return NULL;
  }
#endif

  return (anylibs_fs_file_t*)f;
}

int anylibs_fs_file_is_open(anylibs_fs_file_t* self)
{
  return self ? 0 : 1;
}

int anylibs_fs_file_size(anylibs_fs_file_t* self, size_t* out_file_size)
{
  if (!self || !out_file_size) {
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

size_t anylibs_fs_file_read(anylibs_fs_file_t* self, char buf[], size_t elements_count, size_t element_size)
{
  if (!self || !buf) {
    c_error_set(C_ERROR_null_ptr);
    return 0;
  }

  clearerr((FILE*)self);
  size_t read_size = fread(buf, element_size, elements_count, (FILE*)self);

  if (read_size > 0) {
    return read_size;
  } else {
    if (errno != 0) {
      c_error_set(ferror((FILE*)self));
    } else {
      c_error_set(C_ERROR_invalid_size);
    }
    return 0;
  }
}

size_t anylibs_fs_file_write(anylibs_fs_file_t* self, char buf[], size_t elements_count, size_t element_size)
{
  if (!self || !buf) {
    c_error_set(C_ERROR_null_ptr);
    return 0;
  }

  clearerr((FILE*)self);
  size_t write_size = fwrite(buf, element_size, elements_count, (FILE*)self);

  if (write_size > 0) {
    return write_size;
  } else {
    c_error_set(ferror((FILE*)self));
    return 0;
  }
}

int anylibs_fs_file_flush(anylibs_fs_file_t* self)
{
  if (!self) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }

  errno      = 0;
  int status = fflush((FILE*)self);
  if (status) {
    c_error_set(errno);
    return -1;
  }

  return 0;
}

int anylibs_fs_file_close(anylibs_fs_file_t** self)
{
  if (anylibs_fs_file_is_open(*self) == 0) {
    clearerr((FILE*)*self);
    errno           = 0;
    int close_state = fclose((FILE*)*self);

    if (close_state != 0) {
      c_error_set(errno);
      return -1;
    }

    *self = NULL;
  }

  return 0;
}

size_t anylibs_fs_path_max_size(void)
{
  return MAX_PATH_SIZE;
}

int anylibs_fs_path_append(anylibs_fs_pathbuf_t* base_path, anylibs_fs_path_t path)
{
  if (!base_path || !base_path->data || !path.data) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }
  if (((base_path->capacity - base_path->size) < path.size + 1)) {
    c_error_set(C_ERROR_capacity_full);
    return -1;
  }

  base_path->data[base_path->size++] = C_FS_PATH_SEP[0];
  memcpy(base_path->data + base_path->size, path.data, path.size);
  base_path->size += path.size;
  base_path->data[base_path->size] = '\0';

  return 0;
}

int anylibs_fs_path_to_absolute(anylibs_fs_path_t path, anylibs_fs_pathbuf_t* in_out_abs_path)
{
  anylibs_fs_path_validate(path.data, path.size);
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
  DWORD abs_path_len = GetFullPathNameA(path.data, in_out_abs_path->capacity, in_out_abs_path->data, NULL);
  if (abs_path_len == 0) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  } else if (abs_path_len >= in_out_abs_path->capacity) {
    c_error_set(C_ERROR_capacity_full);
    return -1;
  } else {
    int exists = anylibs_fs_exists(path);
    if (exists != 0) return -1;

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

int anylibs_fs_path_is_absolute(anylibs_fs_path_t path)
{
  anylibs_fs_path_validate(path.data, path.size);

#ifdef _WIN32
  return PathIsRelativeA(path.data) ? 1 : 0;
#else
  return path.data[0] == C_FS_PATH_SEP[0] ? 0 : 1;
#endif
}

int anylibs_fs_path_parent(anylibs_fs_path_t* path)
{
  if (!path || !path->data) {
    c_error_set(C_ERROR_fs_invalid_path);
    return -1;
  }

  char const* ptr  = path->data + path->size - 1;

  // skip multiple trailing slashes
  bool had_slashes = false;
  for (; (ptr >= path->data) && anylibs_fs_is_separator(*ptr); ptr--) {
    had_slashes = true;
  }

  if (ptr == path->data) {
    path->size = 0;
    return 1;
  } else if (had_slashes) {
    ptr--;
  }

  // find the previous slash and skip any /./
  for (; (ptr >= path->data); ptr--) {
    if (anylibs_fs_is_separator(*ptr)) {
      if (*(ptr + 1) == '.') {
        if (((ptr + 2) == path->data + path->size) || anylibs_fs_is_separator(*(ptr + 2))) {
          continue;
        }
      }
      break;
    }
  }

  if (ptr == path->data) {
    path->size = 1;
    return 0;
  } else if (ptr < path->data) {
    path->size = 0;
    return 1;
  }

  // skip multiple trailing slashes
  for (; ((ptr - 1) >= path->data) && anylibs_fs_is_separator(*(ptr - 1)); ptr--) {
  }

  if (ptr == path->data) {
    path->size = 1;
  } else {
#ifdef _WIN32
    if ((ptr == path->data + 2) && (*(ptr - 1) == ':')) {
      ptr++;
    }
#endif
    path->size = (intptr_t)ptr - (intptr_t)path->data;
  }

  return 0;
}

char anylibs_fs_path_separator(void)
{
  return C_FS_PATH_SEP[0];
}

int anylibs_fs_path_filename(anylibs_fs_path_t* path)
{
  if (!path || !path->data) {
    c_error_set(C_ERROR_fs_invalid_path);
    return -1;
  }

  char const* ptr  = path->data + path->size - 1;
  char const* eptr = ptr;

  // skip trailing slashes
  for (; (ptr >= path->data) && anylibs_fs_is_separator(*ptr); ptr--, eptr--) {
  }

  // skip_to_separator
  for (--ptr; ptr >= path->data; ptr--) {
    if (anylibs_fs_is_separator(*ptr)) { /* this will skip `/./` */
      if (*(ptr + 1) == '.') {
        if (((ptr + 2) == path->data + path->size) || anylibs_fs_is_separator(*(ptr + 2))) {
          eptr = ptr;
          continue;
        }
      }
      break;
    }
  }

  if (ptr < path->data) {
    path->size = eptr - ptr - 1;
    ptr        = path->data - 1;

    return path->size == 0 ? 1 : 0;
  } else if (strncmp(ptr + 1, "..", strlen("..")) == 0) {
    path->size = 0;
    return 1;
  }

  if (anylibs_fs_is_separator(*eptr)) eptr--;

  path->size = eptr - ptr;
  path->data = ptr + 1;
  return 0;
}

int anylibs_fs_path_filestem(anylibs_fs_path_t* path)
{
  int status = anylibs_fs_path_filename(path);
  if (status) return status;

  size_t counter  = 0;
  char const* ptr = path->data + path->size;
  while (--ptr >= path->data) {
    if (*ptr == '.') {
      if (ptr == path->data) {
        path->size = counter + 1;
      } else {
        path->size -= counter + 1;
      }
      break;
    }
    counter++;
  }

  return 0;
}

int anylibs_fs_path_file_extension(anylibs_fs_path_t* path)
{
  int status = anylibs_fs_path_filename(path);
  if (status) return status;

  size_t counter  = 0;
  char const* ptr = path->data + path->size;
  while (--ptr >= path->data) {
    if (*ptr == '.') {
      if (ptr == path->data) {
        path->data = ptr + 1;
        path->size -= counter + 1;
      } else {
        path->data = ptr + 1;
        path->size = counter;
      }
      return 0;
    }
    counter++;
  }

  path->size = 0;
  return 1;
}

int anylibs_fs_path_metadata(anylibs_fs_path_t path, anylibs_fs_metadata_t* out_metadata)
{
  anylibs_fs_path_validate(path.data, path.size);
  if (!out_metadata) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }

  anylibs_fs_metadata_t metadata = {0};

#ifdef _WIN32
  errno = 0;
  struct _stat s;
  int status = _stat(path.data, &s);
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
    HANDLE hfind = FindFirstFile(path.data, &findFileData);
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
  metadata.fsize         = s.st_size;
  // permissions
  metadata.fperm         = (s.st_mode & _S_IWRITE) ? C_FS_FILE_PERMISSION_read_write : C_FS_FILE_PERMISSION_read_only;
  // times
  metadata.created_time  = s.st_ctime;
  metadata.last_accessed = s.st_atime;
  metadata.last_modified = s.st_mtime;

  *out_metadata          = metadata;
#else
  errno = 0;
  struct stat s;
  int status = stat(path.data, &s);
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

int anylibs_fs_dir_create(anylibs_fs_path_t dir_path)
{
#if defined(_WIN32)
  SetLastError(0);
  BOOL dir_created = CreateDirectoryA(dir_path.data, NULL);
  if (!dir_created) {
    c_error_set((c_error_t)GetLastError());
    return -1;
  }
#else
  errno = 0;
  enum { default_mask = 0777 };
  int dir_status = mkdir(dir_path.data, default_mask);
  if (dir_status != 0) {
    c_error_set((c_error_t)errno);
    return -1;
  }
#endif

  return 0;
}

int anylibs_fs_is_dir(anylibs_fs_path_t dir_path)
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
  } else if (S_ISDIR(sb.st_mode)) {
    return 0;
  } else {
    return 1; // not a directory
  }
#endif
}

int anylibs_fs_is_file(anylibs_fs_path_t path)
{
  int is_dir = anylibs_fs_is_dir(path);
  return is_dir != -1 ? !is_dir : -1;
}

int anylibs_fs_is_symlink(anylibs_fs_path_t path)
{
#if defined(_WIN32)
  SetLastError(0);
  WIN32_FIND_DATA findFileData;
  HANDLE hfind = FindFirstFile(path.data, &findFileData);
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
  int path_attributes = lstat(path.data, &sb);
  if (path_attributes != 0) {
    c_error_set((c_error_t)errno);
    return -1;
  } else if (S_ISLNK(sb.st_mode)) {
    return 0;
  } else {
    return 1; // not a symlink
  }
#endif
}

int anylibs_fs_dir_current(anylibs_fs_pathbuf_t* in_out_dir_path)
{
  if (!in_out_dir_path || !in_out_dir_path->data) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }
  if (in_out_dir_path->capacity == 0) {
    c_error_set(C_ERROR_invalid_capacity);
    return -1;
  }

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

int anylibs_fs_dir_current_exe(anylibs_fs_pathbuf_t* in_out_dir_path)
{
  if (!in_out_dir_path || !in_out_dir_path->data) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }
  if (in_out_dir_path->capacity == 0) {
    c_error_set(C_ERROR_invalid_capacity);
    return -1;
  }

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
  pid_t pid             = getpid(); // Get the current process ID

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

int anylibs_fs_dir_change_current(anylibs_fs_path_t new_path)
{
  anylibs_fs_path_validate(new_path.data, new_path.size);

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

int anylibs_fs_dir_is_empty(anylibs_fs_path_t path)
{
  int result = 0;

#ifdef _WIN32
  char pbuf[anylibs_fs_path_max_size()];
  memcpy(pbuf, path.data, path.size);
  snprintf(pbuf, anylibs_fs_path_max_size(), "%.*s" C_FS_PATH_SEP "*", (int)path.size, path.data);

  SetLastError(0);
  WIN32_FIND_DATAA cur_file;
  HANDLE find_handler = FindFirstFileA(pbuf, &cur_file);

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
  DIR* dir = opendir(path.data);

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

int anylibs_fs_exists(anylibs_fs_path_t path)
{
  if (!path.data) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }

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
  // struct stat path_stats = {0};
  errno      = 0;
  // int status        = stat(path.data, &path_stats);
  int status = access(path.data, F_OK);
  if (status == 0) {
    return 0;
  } else if (errno == ENOENT) {
    return 1;
  } else {
    c_error_set((c_error_t)errno);
    return -1;
  }
#endif
}

int anylibs_fs_delete(anylibs_fs_path_t path)
{
  anylibs_fs_path_validate(path.data, path.size);

#if defined(_WIN32)
  if (anylibs_fs_is_dir(path) == 0) {
    BOOL remove_dir_status = RemoveDirectoryA(path.data);
    if (!remove_dir_status) {
      c_error_set((c_error_t)GetLastError());
      return -1;
    }
  } else {
    anylibs_fs_path_validate(path.data, path.size);

    _doserrno         = 0;
    int remove_status = remove(path.data);
    if (remove_status) {
      c_error_set(_doserrno);
      return -1;
    }
  }
#else

  anylibs_fs_path_validate(path.data, path.size);

  errno             = 0;
  int remove_status = remove(path.data);
  if (remove_status) {
    c_error_set(errno);
    return -1;
  }
#endif

  return 0;
}

int anylibs_fs_delete_recursively(anylibs_fs_pathbuf_t* path)
{
  if (!path || !path->data) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }
  if (path->capacity == 0) {
    c_error_set(C_ERROR_invalid_capacity);
    return -1;
  }

  anylibs_fs_iter_t iter;
  if (anylibs_fs_iter_create(path, &iter) != 0) return -1;

  anylibs_fs_pathref_t cur_path_ref = {0};
  anylibs_fs_path_t cur_path        = {0};
  int next_status                   = 0;
  while ((next_status = anylibs_fs_iter_next(&iter, &cur_path_ref)) == 0) {
    cur_path = anylibs_fs_pathref_to_path(cur_path_ref);
    // skip . and ..
    if (cur_path_ref.data[cur_path_ref.size - 1] == '.') {
      if (anylibs_fs_is_separator(cur_path_ref.data[cur_path_ref.size - 2])) {
        continue;
      } else if (cur_path_ref.data[cur_path_ref.size - 2] == '.' && anylibs_fs_is_separator(cur_path_ref.data[cur_path_ref.size - 3])) {
        continue;
      }
    }

    if (anylibs_fs_is_dir(cur_path) == 0) {
      int err = anylibs_fs_delete_recursively(iter.pathbuf);
      if (err == -1) goto ON_ERROR;
    } else {
      int err = anylibs_fs_delete(cur_path);
      if (err == -1) goto ON_ERROR;
    }
  }
  if (next_status == -1) goto ON_ERROR;

  int err = anylibs_fs_iter_destroy(&iter);
  if (err) return -1;

  return anylibs_fs_delete(cur_path);
ON_ERROR:
  anylibs_fs_iter_destroy(&iter);
  return -1;
}

int anylibs_fs_iter_create(anylibs_fs_pathbuf_t* path, anylibs_fs_iter_t* out_iter)
{
  anylibs_fs_path_validate(path->data, path->size);
  if (!out_iter) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }

  *out_iter         = (anylibs_fs_iter_t){0};
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

int anylibs_fs_iter_next(anylibs_fs_iter_t* iter, anylibs_fs_pathref_t* out_cur_path)
{
  if (!iter || !iter->pathbuf) {
    c_error_set(C_ERROR_invalid_iterator);
    return -1;
  }

  size_t filename_len = 0;
  char* filename;
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

  iter->pathbuf->size                        = iter->old_len;

  iter->pathbuf->data[iter->pathbuf->size++] = C_FS_PATH_SEP[0];
  memcpy(iter->pathbuf->data + iter->pathbuf->size, filename, filename_len);
  iter->pathbuf->size += filename_len;
  iter->pathbuf->data[iter->pathbuf->size] = '\0';

  if (out_cur_path) *out_cur_path = anylibs_fs_pathbuf_to_pathref(*iter->pathbuf);
  return 0;
}

int anylibs_fs_iter_destroy(anylibs_fs_iter_t* iter)
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
  iter->pathbuf->size                = iter->old_len;
  *iter                              = (anylibs_fs_iter_t){0};
  return err;
}

int anylibs_fs_path_get_last_component(anylibs_fs_path_t path, anylibs_fs_path_t* out_last_component)
{
  if (!out_last_component || !path.data) {
    c_error_set(C_ERROR_null_ptr);
    return -1;
  }

  char const* ptr  = path.data + path.size - 1;
  char const* eptr = ptr;

  // skip multiple trailing slashes
  for (; (ptr > path.data) && anylibs_fs_is_separator(*ptr); ptr--, eptr--) {}

  // skip_to_separator
  for (; ptr >= path.data && !anylibs_fs_is_separator(*ptr); ptr--) {}

  if (eptr == ptr) {
    out_last_component->data = ptr;
    out_last_component->size = 1;
  } else {
    out_last_component->data = ptr + 1;
    out_last_component->size = eptr - ptr;
  }

  return 0;
}

anylibs_fs_path_t anylibs_fs_pathref_to_path(anylibs_fs_pathref_t path_ref)
{
  return (anylibs_fs_path_t){.data = path_ref.data, .size = path_ref.size};
}

anylibs_fs_pathref_t anylibs_fs_pathbuf_to_pathref(anylibs_fs_pathbuf_t pathbuf)
{
  return (anylibs_fs_pathref_t){.data = pathbuf.data, .size = pathbuf.size};
}

anylibs_fs_path_t anylibs_fs_pathbuf_to_path(anylibs_fs_pathbuf_t pathbuf)
{
  return (anylibs_fs_path_t){.data = pathbuf.data, .size = pathbuf.size};
}

// ------------------------- internal ------------------------- //

#ifdef _MSC_VER
#pragma warning(pop)
#endif
