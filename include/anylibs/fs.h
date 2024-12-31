#ifndef ANYLIBS_FS_H
#define ANYLIBS_FS_H

#include <stdbool.h>
#include <stdio.h>

#include "allocator.h"
#include "str.h"

// typedef struct CPath    CPath;
// typedef struct CPathBuf CPathBuf;
typedef struct CFile CFile;
typedef struct CFsIter {
  CStrBuf* pathbuf;
  size_t   old_len;
  void*    cur_dir;
} CFsIter;

// CPath*    c_fs_path_create(CStr path_raw, CAllocator* allocator);
// void      c_fs_path_destroy(CStr* self);
// CPathBuf* c_fs_pathbuf_create(CStr path_raw, CAllocator* allocator);
// void      c_fs_pathbuf_destroy(CStrBuf* self);
CFile*   c_fs_file_open(CStr path, CStr mode);
bool     c_fs_file_size(CFile* self, size_t* out_file_size);
bool     c_fs_file_read(CFile* self, CStrBuf* buf, size_t* out_read_size);
bool     c_fs_file_write(CFile* self, CStr buf, size_t* out_write_size);
bool     c_fs_file_flush(CFile* self);
bool     c_fs_file_close(CFile* self);
bool     c_fs_path_append(CStrBuf* base_path, CStr path);
CStrBuf* c_fs_path_to_absolute(CStr path, CAllocator* allocator);
bool     c_fs_path_is_absolute(CStr path);
// c_error_t c_fs_path_to_parent(char path[], size_t path_len, size_t* out_new_path_len);
char     c_fs_path_separator(void);
size_t   c_fs_path_max_len(void);
bool     c_fs_dir_create(CStr dir_path);
int      c_fs_is_dir(CStr const path);
// int      c_fs_dir_exists(CStr const dir_path);
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

// ///////////////////////////////
// typedef struct CFsIter_v2 CFsIter_v2;

// CFsIter_v2* c_fs_iter_create_v2(CStrBuf* path, CAllocator* allocator);
// void        c_fs_iter_destroy_v2(CFsIter_v2* self);
// // bool c_fs_iter_default_step_callback_v2(CFsIter_v2* iter, CStr* path) ANYLIBS_C_DISABLE_UNDEFINED;
// bool        c_fs_iter_next_v2(CFsIter_v2* iter, CStr** out_cur_path);
// ///////////////////////////////

// CStr*    c_cpathbuf_to_cpath(CStrBuf* path_buf);
// CStrBuf* c_cpath_to_cpathbuf(CStr path);

#endif // ANYLIBS_FS_H
