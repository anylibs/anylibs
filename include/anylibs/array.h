/**
 * @copyright (C) 2024-2025 Mohamed A. Elmeligy
 * MIT License
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

#ifndef ANYLIBS_ARRAY_H
#define ANYLIBS_ARRAY_H
#include <stdbool.h>
#include <stddef.h>

#include "error.h"

typedef struct CArray {
  void*  data;
  size_t len;      ///< current length, note: this unit based not bytes based
  size_t capacity; ///< maximum data that can be hold, note: this unit based
                   ///  not bytes based
  size_t element_size; ///< size of the unit
} CArray;

c_error_t c_array_init(size_t element_size, CArray* out_c_array);

c_error_t c_array_create_with_capacity(size_t  element_size,
                                       size_t  capacity,
                                       CArray* out_c_array);

bool c_array_is_empty(CArray const* self);

size_t c_array_len(CArray const* self);

c_error_t c_array_set_len(CArray* self, size_t new_len);

size_t c_array_capacity(CArray const* self);

c_error_t c_array_set_capacity(CArray* self, size_t new_capacity);

size_t c_array_element_size(CArray* self);

c_error_t c_array_push(CArray* self, void const* element);

c_error_t c_array_pop(CArray* self, void* out_element);

c_error_t c_array_insert(CArray* self, void const* element, size_t index);

c_error_t c_array_insert_range(CArray*     self,
                               size_t      index,
                               void const* data,
                               size_t      data_len);

c_error_t c_array_remove(CArray* self, size_t index);

c_error_t
c_array_remove_range(CArray* self, size_t start_index, size_t range_size);

void c_array_deinit(CArray* self);
#endif // ANYLIBS_ARRAY_H
