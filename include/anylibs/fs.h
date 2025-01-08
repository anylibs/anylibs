#ifndef ANYLIBS_FS_H
#define ANYLIBS_FS_H

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "allocator.h"
#include "str.h"

typedef struct CFile CFile;
typedef struct CFsIter {
  CStrBuf* pathbuf;
  size_t   old_len;
  void*    cur_dir;
} CFsIter;

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

CFile*   c_fs_file_open(CStr path, CStr mode);
bool     c_fs_file_size(CFile* self, size_t* out_file_size);
bool     c_fs_file_read(CFile* self, CStrBuf* buf, size_t* out_read_size);
bool     c_fs_file_write(CFile* self, CStr buf, size_t* out_write_size);
bool     c_fs_file_flush(CFile* self);
bool     c_fs_file_close(CFile* self);
bool     c_fs_path_append(CStrBuf* base_path, CStr path);
CStrBuf* c_fs_path_to_absolute(CStr path, CAllocator* allocator);
bool     c_fs_path_is_absolute(CStr path);
bool     c_fs_path_parent(CStr path, CStr* out_parent);
char     c_fs_path_separator(void);
size_t   c_fs_path_max_len(void);
bool     c_fs_path_filename(CStr path, CStr* out_filename);
bool     c_fs_path_filestem(CStr path, CStr* out_filestem);
bool     c_fs_path_file_extension(CStr path, CStr* out_file_extension);
CIter    c_fs_path_iter(CStr path);
bool     c_fs_path_iter_component_next(CIter* iter, CStr* out_component);
bool     c_fs_path_metadata(CStr path, CFsMetadata* out_metadata);
bool     c_fs_dir_create(CStr dir_path);
int      c_fs_is_dir(CStr const path);
int      c_fs_is_file(CStr const path);
int      c_fs_is_symlink(CStr const path);
CStrBuf* c_fs_dir_current(CAllocator* allocator);
CStrBuf* c_fs_dir_current_exe(CAllocator* allocator);
bool     c_fs_dir_change_current(CStr new_path);
int      c_fs_dir_is_empty(CStr path);
int      c_fs_exists(CStr const path);
bool     c_fs_delete(CStr const path);
bool     c_fs_delete_recursively(CStrBuf* path);
CFsIter  c_fs_iter(CStrBuf* path);
bool     c_fs_iter_next(CFsIter* iter, CStr* out_cur_path);
bool     c_fs_iter_close(CFsIter* iter);

#endif // ANYLIBS_FS_H
