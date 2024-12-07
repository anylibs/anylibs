/**
 * @file vec.h
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

#ifndef ANYLIBS_VEC_H
#define ANYLIBS_VEC_H
#include <stdbool.h>
#include <stddef.h>

#include "allocator.h"
#include "error.h"

typedef struct CVec {
  void* data; ///< heap allocated data
} CVec;

c_error_t
c_vec_create(size_t element_size, CAllocator* allocator, CVec** out_c_vec);

c_error_t c_vec_create_with_capacity(size_t      element_size,
                                     size_t      capacity,
                                     bool        zero_initialized,
                                     CAllocator* allocator,
                                     CVec**      out_c_vec);

/// FIXME: this is buggy right now
c_error_t c_vec_create_from_raw(void*       data,
                                size_t      data_len,
                                size_t      element_size,
                                CAllocator* allocator,
                                CVec**      out_c_vec);

c_error_t
c_vec_clone(CVec const* self, bool should_shrink_clone, CVec** out_c_vec);

bool c_vec_is_empty(CVec const* self);

size_t c_vec_len(CVec const* self);

c_error_t c_vec_set_len(CVec* self, size_t new_len);

size_t c_vec_capacity(CVec const* self);

size_t c_vec_spare_capacity(CVec const* self);

c_error_t c_vec_set_capacity(CVec* self, size_t new_capacity);

size_t c_vec_element_size(CVec* self);

c_error_t c_vec_shrink_to_fit(CVec* self);

c_error_t c_vec_get(CVec const* self, size_t index, void** out_element);

c_error_t c_vec_search(CVec const* self,
                       void*       element,
                       int         cmp(void const*, void const*),
                       size_t*     out_index);

c_error_t c_vec_binary_search(CVec const* self,
                              void const* element,
                              int         cmp(void const*, void const*),
                              size_t*     out_index);

bool c_vec_starts_with(CVec const* self,
                       void*       elements,
                       size_t      elements_len,
                       int         cmp(void const*, void const*));

bool c_vec_ends_with(CVec const* self,
                     void*       elements,
                     size_t      elements_len,
                     int         cmp(void const*, void const*));

void c_vec_sort(CVec* self, int cmp(void const*, void const*));

bool c_vec_is_sorted(CVec* self, int cmp(void const*, void const*));

bool c_vec_is_sorted_inv(CVec* self, int cmp(void const*, void const*));

c_error_t c_vec_push(CVec* self, void const* element);

c_error_t
c_vec_push_range(CVec* self, void const* elements, size_t elements_len);

c_error_t c_vec_pop(CVec* self, void* out_element);

c_error_t c_vec_insert(CVec* self, void const* element, size_t index);

c_error_t
c_vec_insert_range(CVec* self, size_t index, void const* data, size_t data_len);

void c_vec_fill(CVec* self, void* data);

c_error_t c_vec_fill_with_repeat(CVec* self, void* data, size_t data_len);

c_error_t c_vec_concatenate(CVec* vec1, CVec const* vec2);

c_error_t c_vec_rotate_right(CVec* self, size_t elements_count);

c_error_t c_vec_rotate_left(CVec* self, size_t elements_count);

c_error_t c_vec_remove(CVec* self, size_t index);

c_error_t c_vec_remove_range(CVec* self, size_t start_index, size_t range_size);

c_error_t c_vec_deduplicate(CVec* self, int cmp(void const*, void const*));

c_error_t c_vec_slice(CVec const* self,
                      size_t      start_index,
                      size_t      range,
                      CVec**      out_slice);

bool c_vec_iter(CVec* self, size_t* index, void** element);

c_error_t c_vec_reverse(CVec* self);

void c_vec_clear(CVec* self);

void c_vec_destroy(CVec** self);
#endif // ANYLIBS_VEC_H
