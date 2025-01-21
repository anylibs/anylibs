#ifndef ANYLIBS_FS_H
#define ANYLIBS_FS_H

#include <stdio.h>
#include <time.h>

typedef struct CFile CFile;

typedef enum CFsFileType {
  C_FS_FILE_TYPE_unknown,
  C_FS_FILE_TYPE_file,
  C_FS_FILE_TYPE_dir,
  C_FS_FILE_TYPE_symlink,
} CFsFileType;

typedef enum CFsFilePermission {
  C_FS_FILE_PERMISSION_unknown,
  C_FS_FILE_PERMISSION_read_only,
  C_FS_FILE_PERMISSION_read_write,
} CFsFilePermission;

typedef struct CFsMetadata {
  CFsFileType       ftype;
  size_t            fsize;
  CFsFilePermission fperm;
  time_t            last_modified;
  time_t            last_accessed;
  time_t            created_time;
} CFsMetadata;

typedef struct CPath    CPath;
typedef struct CPathBuf CPathBuf;
typedef struct CPathRef CPathRef;

struct CPath {
  char const* data;
  size_t      size;
};
static inline CPath c_fs_path_create(char const path[], size_t path_size)
{
  return (CPath){.data = path, .size = path_size};
}

struct CPathBuf {
  char*  data;
  size_t size;
  size_t capacity;
};
static inline CPathBuf c_fs_pathbuf_create(char pathbuf[], size_t pathbuf_size, size_t pathbuf_capacity)
{
  return (CPathBuf){.data = pathbuf, .size = pathbuf_size, .capacity = pathbuf_capacity};
}

struct CPathRef {
  char*  data;
  size_t size;
};
static inline CPath c_fs_cpathref_to_cpath(CPathRef path_ref)
{
  return c_fs_path_create(path_ref.data, path_ref.size);
}
static inline CPathRef c_fs_cpathbuf_to_cpathref(CPathBuf pathbuf)
{
  return (CPathRef){.data = pathbuf.data, .size = pathbuf.size};
}

typedef struct CFsIter {
  CPathBuf* pathbuf;
  size_t    old_len;
  void*     fptr;
} CFsIter;

CFile* c_fs_file_open(CPath path, char const mode[], size_t mode_size);
int    c_fs_file_size(CFile* self, size_t* out_file_size);
size_t c_fs_file_read(CFile* self, char buf[], size_t elements_count, size_t element_size);
size_t c_fs_file_write(CFile* self, char buf[], size_t elements_count, size_t element_size);
int    c_fs_file_flush(CFile* self);
int    c_fs_file_close(CFile* self);
int    c_fs_path_append(CPathBuf* base_path, CPath path);
int    c_fs_path_to_absolute(CPath path, CPathBuf* in_out_abs_path);
int    c_fs_path_is_absolute(CPath path);
int    c_fs_path_parent(CPath* path);
char   c_fs_path_separator(void);
// size_t c_fs_path_max_len(void);
int    c_fs_path_filename(CPath* path);
int    c_fs_path_filestem(CPath* path);
int    c_fs_path_file_extension(CPath* path);
// CIter  c_fs_path_iter(CPath path);
// int    c_fs_path_iter_component_next(CIter* iter, CStr* out_component);
int    c_fs_path_metadata(CPath path, CFsMetadata* out_metadata);
int    c_fs_dir_create(CPath dir_path);
int    c_fs_is_dir(CPath path);
int    c_fs_is_file(CPath path);
int    c_fs_is_symlink(CPath path);
int    c_fs_dir_current(CPathBuf* in_out_dir_path);
int    c_fs_dir_current_exe(CPathBuf* in_out_dir_path);
int    c_fs_dir_change_current(CPath new_path);
int    c_fs_dir_is_empty(CPath path);
int    c_fs_exists(CPath path);
int    c_fs_delete(CPath path);
int    c_fs_delete_recursively(CPathBuf* path);
int    c_fs_iter(CPathBuf* path, CFsIter* out_iter);
int    c_fs_iter_next(CFsIter* iter, CPathRef* out_cur_path);
int    c_fs_iter_close(CFsIter* iter);

#endif // ANYLIBS_FS_H
