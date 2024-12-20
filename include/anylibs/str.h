/**
 * @file str.h
 * @author Mohamed A. Elmeligy
 * @date 2024-2025
 * @copyright MIT License
 *
 * Permission is hereby granted, free of charge, to use, copy, modify, and
 * distribute this software, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO MERCHANTABILITY OR FITNESS FOR
 * A PARTICULAR PURPOSE. See the License for details.
 */

#ifndef ANYLIBS_STR_H
#define ANYLIBS_STR_H

#include "allocator.h"
#include "def.h"
#include "iter.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/// @brief create CStr object from char* string
#define CSTR(str) ((CStr){(char*)(str), sizeof(str) - 1})
/// @brief create CChar object from char* string ()
/// @note size of ch should be maximum of 4 as a valid UTF-8 char
#define CCHAR(ch) ((CChar){(char*)(ch), sizeof(ch) - 1})

typedef struct CString {
  char* data;
} CString;

typedef struct CStr {
  char*  data;
  size_t len; ///< in bytes
} CStr;

typedef struct CChar {
  char*  data;
  size_t size; ///< in bytes
} CChar;

typedef struct CCharOwned {
  char   data[4]; ///< this is maximum size of a valid UTF-8 character
  size_t count;
} CCharOwned;

typedef struct CStringIter {
  CIter  base;
  size_t ch_counter;
  size_t current_ch_size;
} CStringIter;

typedef struct CResultCharOwned {
  CCharOwned ch;
  bool       is_ok;
} CResultCharOwned;

typedef struct CResultChar {
  CChar ch;
  bool  is_ok;
} CResultChar;

typedef struct CResultStr {
  CStr str;
  bool is_ok;
} CResultStr;

typedef CResultVoidPtr (*CStringIterStepCallback)(CStringIter* self,
                                                  char*        data,
                                                  size_t       data_size);
typedef struct CVec CVec;

CString* c_str_create(CAllocator* allocator);

CString*
c_str_create_from_raw(CStr cstr, bool should_copy, CAllocator* allocator);

CString* c_str_create_with_capacity(size_t      capacity,
                                    CAllocator* allocator,
                                    bool        zero_initialized);

CString* c_str_clone(CString const* self, bool should_shrink_clone);

bool c_str_is_empty(CString const* self);

CResultSizeT c_str_count(CString const* self);

size_t c_str_len(CString const* self);

void c_str_set_len(CString* self, size_t new_len);

size_t c_str_capacity(CString const* self);

size_t c_str_spare_capacity(CString const* self);

bool c_str_set_capacity(CString* self, size_t new_capacity);

bool c_str_shrink_to_fit(CString* self);

CResultStr
c_str_get(CString const* self, size_t start_index, size_t range_size);

CResultStr c_str_find(CString const* self, CChar ch);

CResultStr c_str_find_by_iter(CString const* self, CChar ch, CStringIter* iter);

CResultBool c_str_starts_with(CString const* self, CStr cstr);

CResultBool c_str_ends_with(CString const* self, CStr cstr);

bool c_str_push(CString* self, CStr cstr);

CResultCharOwned c_str_pop(CString* self);

bool c_str_insert(CString* self, size_t byte_index, CStr cstr);

bool c_str_fill(CString* self, CStr cstr);

bool c_str_replace(CString* self, size_t index, size_t range_size, CStr cstr);

bool c_str_concatenate(CString* str1, CString const* str2);

bool c_str_remove(CString* self, size_t start_index, size_t range_size);

CStr c_str_trim(CString const* self);

CStr c_str_trim_start(CString const* self);

CStr c_str_trim_end(CString const* self);

// c_error_t c_str_split(CString const* self, size_t index, CVec** out_slices);
CResultStr c_str_split(CString const* self,
                       CChar const    delimeters[],
                       size_t         delimeters_len,
                       CStringIter*   iter);

CResultStr c_str_split_by_whitespace(CString const* self, CStringIter* iter);

CResultStr c_str_split_by_line(CString const* self, CStringIter* iter);

CVec* c_str_to_utf16(CString const* self);

CString* c_str_from_utf16(CVec const* vec_u16);

bool c_str_is_ascii(CString const* self);

void c_str_to_ascii_uppercase(CString* self);

void c_str_to_ascii_lowercase(CString* self);

CStringIter c_str_iter(CStringIterStepCallback step_callback);

ANYLIBS_C_DISABLE_UNDEFINED
CResultVoidPtr c_str_iter_default_step_callback(CStringIter* iter,
                                                char*        data,
                                                size_t       data_len);

void c_str_iter_rev(CString const* self, CStringIter* iter);

CResultChar c_str_iter_next(CString const* self, CStringIter* iter);

CResultChar
c_str_iter_nth(CString const* self, CStringIter* iter, size_t index);

CResultChar c_str_iter_peek(CString const* self, CStringIter const* iter);

CResultChar c_str_iter_first(CString const* self, CStringIter* iter);

CResultChar c_str_iter_last(CString const* self, CStringIter* iter);

CString* c_str_reverse(CString const* self);

void c_str_clear(CString* self);

bool c_str_format(CString*    self,
                  size_t      index,
                  size_t      format_len,
                  char const* format,
                  ...) ANYLIBS_C_PRINTF(4, 5);

bool c_str_format_va(CString*    self,
                     size_t      index,
                     size_t      format_len,
                     char const* format,
                     va_list     va) ANYLIBS_C_PRINTF(4, 0);

/// TODO: create Ordering like rust
int c_str_compare(CString* str1, CString* str2);

bool c_str_is_whitespace(CChar ch);

bool c_str_is_ascii_whitespace(char ch);

CVec* c_str_to_vec(CString* self);

CStr* c_cstring_to_cstr(CString* self);

CString* c_cstr_to_cstring(CStr cstr, CAllocator* allocator);

void c_str_destroy(CString* self);

#endif // ANYLIBS_STR_H
