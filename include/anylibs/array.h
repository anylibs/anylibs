/**
 * @file array.c
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
  void* data; ///< heap allocated data
} CArray;

c_error_t c_array_create(size_t element_size, CArray** out_c_array);

c_error_t c_array_create_with_capacity(size_t   element_size,
                                       size_t   capacity,
                                       bool     zeroed_out,
                                       CArray** out_c_array);

c_error_t c_array_clone(CArray const* self,
                        bool          should_shrink_clone,
                        CArray**      out_c_array);

bool c_array_is_empty(CArray const* self);

size_t c_array_len(CArray const* self);

c_error_t c_array_set_len(CArray* self, size_t new_len);

size_t c_array_capacity(CArray const* self);

size_t c_array_spare_capacity(CArray const* self);

c_error_t c_array_set_capacity(CArray* self, size_t new_capacity);

size_t c_array_element_size(CArray* self);

c_error_t c_array_shrink_to_fit(CArray* self);

c_error_t c_array_get(CArray const* self, size_t index, void** out_element);

c_error_t c_array_search(CArray const* self,
                         void*         element,
                         int           cmp(void const*, void const*),
                         size_t*       out_index);

c_error_t c_array_binary_search(CArray const* self,
                                void const*   element,
                                int           cmp(void const*, void const*),
                                size_t*       out_index);

bool c_array_starts_with(CArray const* self,
                         void*         elements,
                         size_t        elements_len,
                         int           cmp(void const*, void const*));

bool c_array_ends_with(CArray const* self,
                       void*         elements,
                       size_t        elements_len,
                       int           cmp(void const*, void const*));

void c_array_sort(CArray* self, int cmp(void const*, void const*));

bool c_array_is_sorted(CArray* self, int cmp(void const*, void const*));

bool c_array_is_sorted_inv(CArray* self, int cmp(void const*, void const*));

c_error_t c_array_push(CArray* self, void const* element);

c_error_t
c_array_push_range(CArray* self, void const* elements, size_t elements_len);

c_error_t c_array_pop(CArray* self, void* out_element);

c_error_t c_array_insert(CArray* self, void const* element, size_t index);

c_error_t c_array_insert_range(CArray*     self,
                               size_t      index,
                               void const* data,
                               size_t      data_len);

void c_array_fill(CArray* self, void* data);

c_error_t c_array_fill_with_repeat(CArray* self, void* data, size_t data_len);

c_error_t c_array_concatenate(CArray* arr1, CArray const* arr2);

c_error_t c_array_rotate_right(CArray* self, size_t elements_count);

c_error_t c_array_rotate_left(CArray* self, size_t elements_count);

c_error_t c_array_remove(CArray* self, size_t index);

c_error_t
c_array_remove_range(CArray* self, size_t start_index, size_t range_size);

c_error_t c_array_deduplicate(CArray* self, int cmp(void const*, void const*));

c_error_t c_array_slice(CArray const* self,
                        size_t        start_index,
                        size_t        range,
                        CArray**      out_slice);

bool c_array_iter(CArray* self, size_t* index, void** element);

c_error_t c_array_reverse(CArray* self);

void c_array_clear(CArray* self);

void c_array_destroy(CArray** self);
#endif // ANYLIBS_ARRAY_H
