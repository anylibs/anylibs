#ifndef ANYLIBS_STR_H
#define ANYLIBS_STR_H

#include "allocator.h"
#include "def.h"
#include "iter.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/// @brief this is like std::string
typedef struct CStrBuf {
  char* data;
} CStrBuf;

/// @brief this is like std::string_view
typedef struct CStr {
  char*  data;
  size_t len; ///< in bytes
} CStr;
#define CSTR(str) ((CStr){(char*)(str), sizeof(str) - 1}) ///< create CStr object from char* string

typedef struct CChar {
  char   data[4]; ///< this is maximum size of a valid UTF-8 character
  size_t count;
} CChar;

typedef struct CVec CVec;

CStrBuf* c_str_create(CAllocator* allocator);
CStrBuf* c_str_create_from_raw(CStr cstr, bool should_copy, CAllocator* allocator);
CStrBuf* c_str_create_with_capacity(size_t capacity, CAllocator* allocator, bool zero_initialized);
CStrBuf* c_str_clone(CStrBuf const* self, bool should_shrink_clone);
bool     c_str_is_empty(CStrBuf const* self);
int      c_str_count(CStrBuf const* self);
size_t   c_str_len(CStrBuf const* self);
void     c_str_set_len(CStrBuf* self, size_t new_len);
size_t   c_str_capacity(CStrBuf const* self);
size_t   c_str_spare_capacity(CStrBuf const* self);
bool     c_str_set_capacity(CStrBuf* self, size_t new_capacity);
bool     c_str_shrink_to_fit(CStrBuf* self);
bool     c_str_get(CStrBuf const* self, size_t start_index, size_t range_size, CStr* out_str);
bool     c_str_find(CStrBuf const* self, CStr data, CStr* out_str);
bool     c_str_find_by_iter(CIter* iter, CStr data, CStr* out_str);
int      c_str_starts_with(CStrBuf const* self, CStr cstr);
int      c_str_ends_with(CStrBuf const* self, CStr cstr);
bool     c_str_push(CStrBuf* self, CStr cstr);
bool     c_str_pop(CStrBuf* self, CChar* out_ch);
bool     c_str_insert(CStrBuf* self, size_t byte_index, CStr cstr);
bool     c_str_fill(CStrBuf* self, CStr cstr);
bool     c_str_replace(CStrBuf* self, size_t index, size_t range_size, CStr cstr);
bool     c_str_concatenate(CStrBuf* str1, CStrBuf const* str2);
bool     c_str_remove(CStrBuf* self, size_t start_index, size_t range_size);
CStr     c_str_trim(CStrBuf const* self);
CStr     c_str_trim_start(CStrBuf const* self);
CStr     c_str_trim_end(CStrBuf const* self);
bool     c_str_iter_split(CIter* iter, CChar const delimeters[], size_t delimeters_len, CStr* out_str);
bool     c_str_iter_split_by_whitespace(CIter* iter, CStr* out_str);
bool     c_str_iter_split_by_line(CIter* iter, CStr* out_str);
CVec*    c_str_to_utf16(CStrBuf const* self);
CStrBuf* c_str_from_utf16(CVec const* vec_u16);
bool     c_str_is_ascii(CStrBuf const* self);
void     c_str_to_ascii_uppercase(CStrBuf* self);
void     c_str_to_ascii_lowercase(CStrBuf* self);
CIter    c_str_iter(CStrBuf* self);
bool     c_str_iter_next(CIter* iter, CStr* out_ch);
bool     c_str_iter_prev(CIter* iter, CStr* out_ch);
bool     c_str_iter_nth(CIter* iter, size_t index, CStr* out_ch);
bool     c_str_iter_peek(CIter const* iter, CStr* out_ch);
CStr     c_str_iter_first(CIter* iter);
CStr     c_str_iter_last(CIter* iter);
CStrBuf* c_str_reverse(CStrBuf const* self);
void     c_str_clear(CStrBuf* self);
bool     c_str_format(CStrBuf* self, size_t index, size_t format_len, char const* format, ...) ANYLIBS_C_PRINTF(4, 5);
bool     c_str_format_va(CStrBuf* self, size_t index, size_t format_len, char const* format, va_list va) ANYLIBS_C_PRINTF(4, 0);
int      c_str_compare(CStrBuf* str1, CStrBuf* str2);
bool     c_str_is_whitespace(CChar ch);
bool     c_str_is_ascii_whitespace(char ch);
CVec*    c_str_to_vec(CStrBuf* self);
CStr     c_cstrbuf_to_cstr(CStrBuf* self);
CStrBuf* c_cstr_to_cstrbuf(CStr cstr, CAllocator* allocator);
void     c_str_destroy(CStrBuf* self);

#endif // ANYLIBS_STR_H
