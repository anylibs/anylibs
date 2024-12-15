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
#include "error.h"
#include "iter.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

typedef struct CString {
  char* data;
} CString;

typedef struct CStr {
  char*  data;
  size_t len;
} CStr;

typedef struct CChar {
  char*  data;
  size_t size;
} CChar;

typedef struct CCharOwned {
  char   data[4];
  size_t size;
} CCharOwned;

typedef struct CStringIter {
  CIter  base;
  size_t byte_counter;
} CStringIter;

typedef bool (*CStringIterStepCallback)(CStringIter* iter,
                                        char*        data,
                                        size_t       data_len,
                                        CChar*       out_element);

typedef struct CVec CVec;

c_error_t c_str_create(CAllocator* allocator, CString** out_c_str);

c_error_t c_str_create_from_raw(char        data[],
                                size_t      data_len,
                                bool        should_copy,
                                CAllocator* allocator,
                                CString**   out_c_str);

c_error_t c_str_create_with_capacity(size_t      capacity,
                                     CAllocator* allocator,
                                     bool        zero_initialized,
                                     CString**   out_c_str);

c_error_t
c_str_clone(CString const* self, bool should_shrink_clone, CString** out_c_str);

bool c_str_is_empty(CString const* self);

c_error_t c_str_count(CString const* self, size_t* out_count);

void c_str_len(CString const* self, size_t* out_len);

c_error_t c_str_set_len(CString* self, size_t new_len);

void c_str_capacity(CString const* self, size_t* out_capacity);

void c_str_spare_capacity(CString const* self, size_t* out_spare_capacity);

c_error_t c_str_set_capacity(CString* self, size_t new_capacity);

c_error_t c_str_shrink_to_fit(CString* self);

c_error_t c_str_get(CString const* self, size_t byte_index, char* out_data);

c_error_t c_str_find(CString const* self, CChar ch, size_t* out_index);

c_error_t c_str_find_by_iter(CString const* self,
                             CChar          ch,
                             CStringIter*   iter,
                             size_t*        out_index);

bool c_str_starts_with(CString const* self, char const data[], size_t data_len);

bool c_str_ends_with(CString const* self, char const data[], size_t data_len);

void c_str_sort(CString* self);

bool c_str_is_sorted(CString* self);

bool c_str_is_sorted_inv(CString* self);

c_error_t c_str_push(CString* self, char const data[], size_t data_len);

c_error_t c_str_pop(CString* self, CCharOwned* out_ch);

c_error_t c_str_insert(CString*   self,
                       size_t     byte_index,
                       char const data[],
                       size_t     data_len);

c_error_t c_str_fill(CString* self, char data[], size_t data_len);

c_error_t c_str_replace(CString* self,
                        size_t   index,
                        size_t   range_size,
                        char     data[],
                        size_t   data_len);

c_error_t c_str_concatenate(CString* str1, CString const* str2);

c_error_t c_str_remove(CString* self, size_t start_index, size_t range_size);

c_error_t c_str_trim(CString const* self, CStr* out_slice);

c_error_t c_str_trim_start(CString const* self, CStr* out_slice);

c_error_t c_str_trim_end(CString const* self, CStr* out_slice);

// c_error_t c_str_split(CString const* self, size_t index, CVec** out_slices);
bool c_str_split(CString const* self,
                 CChar const    delimeters[],
                 size_t         delimeters_len,
                 CStringIter*   iter,
                 CStr*          out_slice);

bool c_str_split_by_whitespace(CString const* self,
                               CStringIter*   iter,
                               CStr*          out_slice);

bool
c_str_split_by_line(CString const* self, CStringIter* iter, CStr* out_slice);

c_error_t c_str_to_utf16(CString const* self, CVec* out_vec_u16);

c_error_t c_str_from_utf16(CVec const* vec_u16, CString* out_c_str);

bool c_str_is_ascii(CString const* self);

void c_str_to_ascii_uppercase(CString* self);

void c_str_to_ascii_lowercase(CString* self);

c_error_t c_str_slice(CString const* self,
                      size_t         start_index,
                      size_t         range_size,
                      CStr*          out_slice);

void c_str_iter(CStringIterStepCallback step_callback, CStringIter* out_c_iter);

/// suppress sanitizers error
ANYLIBS_C_DISABLE_UNDEFINED
bool c_str_iter_default_step_callback(CStringIter* iter,
                                      char*        data,
                                      size_t       data_len,
                                      CChar*       out_char);

void c_str_iter_rev(CString const* self, CStringIter* iter);

bool c_str_iter_next(CString const* self, CStringIter* iter, CChar* out_char);

c_error_t c_str_iter_nth(CString const* self,
                         CStringIter*   iter,
                         size_t         index,
                         CChar*         out_char);

c_error_t
c_str_iter_peek(CString const* self, CStringIter const* iter, CChar* out_char);

void c_str_iter_first(CString const* self, CStringIter* iter, CChar* out_char);

void c_str_iter_last(CString const* self, CStringIter* iter, CChar* out_char);

c_error_t c_str_reverse(CString const* self, CString** reversed_str);

void c_str_clear(CString* self);

c_error_t c_str_format(CString*    self,
                       size_t      index,
                       size_t      format_len,
                       char const* format,
                       ...) ANYLIBS_C_PRINTF(4, 5);

c_error_t c_str_format_va(CString*    self,
                          size_t      index,
                          size_t      format_len,
                          char const* format,
                          va_list     va) ANYLIBS_C_PRINTF(4, 0);

/// TODO: create Ordering like rust
int c_str_compare(CString* str1, CString* str2);

bool c_str_is_whitespace(char const data[], size_t data_size);

bool c_str_is_ascii_whitespace(unsigned char data);

void c_str_to_vec(CString* self, CVec** out_c_vec);

void c_str_destroy(CString** self);

#endif // ANYLIBS_STR_H
