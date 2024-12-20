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
#include "iter.h"

typedef struct CVec {
  void* data; ///< heap allocated data
} CVec;
typedef struct CString CString;

typedef struct CVecFindResult {
  void* element;
  bool  is_ok;
} CVecFindResult;

typedef struct CVecElementResult {
  void* element;
  bool  is_ok;
} CVecElementResult;

CVec* c_vec_create(size_t element_size, CAllocator* allocator);

CVec* c_vec_create_with_capacity(size_t      element_size,
                                 size_t      capacity,
                                 bool        zero_initialized,
                                 CAllocator* allocator);

CVec* c_vec_create_from_raw(void*       data,
                            size_t      data_len,
                            size_t      element_size,
                            bool        should_copy,
                            CAllocator* allocator);

CVec* c_vec_clone(CVec const* self, bool should_shrink_clone);

bool c_vec_is_empty(CVec const* self);

size_t c_vec_len(CVec const* self);

void c_vec_set_len(CVec* self, size_t new_len);

size_t c_vec_capacity(CVec const* self);

size_t c_vec_spare_capacity(CVec const* self);

bool c_vec_set_capacity(CVec* self, size_t new_capacity);

size_t c_vec_element_size(CVec* self);

bool c_vec_shrink_to_fit(CVec* self);

CVecElementResult c_vec_get(CVec const* self, size_t index);

CVecFindResult
c_vec_find(CVec const* self, void* element, int cmp(void const*, void const*));

CVecFindResult c_vec_binary_find(CVec const* self,
                                 void const* element,
                                 int         cmp(void const*, void const*));

bool c_vec_starts_with(CVec const* self,
                       void const* elements,
                       size_t      elements_len,
                       int         cmp(void const*, void const*));

bool c_vec_ends_with(CVec const* self,
                     void const* elements,
                     size_t      elements_len,
                     int         cmp(void const*, void const*));

void c_vec_sort(CVec* self, int cmp(void const*, void const*));

bool c_vec_is_sorted(CVec* self, int cmp(void const*, void const*));

bool c_vec_is_sorted_inv(CVec* self, int cmp(void const*, void const*));

bool c_vec_push(CVec* self, void const* element);

bool c_vec_push_range(CVec* self, void const* elements, size_t elements_len);

CVecElementResult c_vec_pop(CVec* self);

bool c_vec_insert(CVec* self, size_t index, void const* element);

bool
c_vec_insert_range(CVec* self, size_t index, void const* data, size_t data_len);

void c_vec_fill(CVec* self, void* data);

bool c_vec_fill_with_repeat(CVec* self, void* data, size_t data_len);

bool c_vec_replace(
    CVec* self, size_t index, size_t range_len, void* data, size_t data_len);

bool c_vec_concatenate(CVec* vec1, CVec const* vec2);

bool c_vec_rotate_right(CVec* self, size_t elements_count);

bool c_vec_rotate_left(CVec* self, size_t elements_count);

bool c_vec_remove(CVec* self, size_t index);

bool c_vec_remove_range(CVec* self, size_t start_index, size_t range_len);

bool c_vec_deduplicate(CVec* self, int cmp(void const*, void const*));

CVec* c_vec_slice(CVec const* self, size_t start_index, size_t range_len);

CIter c_vec_iter(CVec* self, CIterStepCallback step_callback);

bool c_vec_reverse(CVec* self);

void c_vec_clear(CVec* self);

CString* c_cvec_to_cstring(CVec* self);

void c_vec_destroy(CVec* self);
#endif // ANYLIBS_VEC_H
