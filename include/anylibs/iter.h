/**
 * @file iter.h
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

#ifndef ANYLIBS_ITER_H
#define ANYLIBS_ITER_H

#include "allocator.h"
#include "error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct CIter CIter;
typedef bool (*CIterStepCallback)(CIter* self,
                                  void*  data,
                                  size_t data_len,
                                  void** out_element);

struct CIter {
  size_t current_pos;          ///< this is in units not bytes
                               /// <(this is according to @ref CIter::step_size)
  size_t            step_size; ///< this is usually the element size
  CIterStepCallback step_callback; ///< this is mainly act like c_iter_next for
                                   ///< whatever the direction
  bool is_reversed;                ///< thiis is true when go backward
  bool is_done; ///< this is used only if @ref CIter::is_reversed is true
};

c_error_t c_iter_create(size_t            step_size,
                        bool              is_reversed,
                        CIterStepCallback step_callback,
                        CIter*            out_c_iter);

void c_iter_rev(CIter* self, void* data, size_t data_len);

bool c_iter_next(CIter* self, void* data, size_t data_len, void** out_element);

c_error_t c_iter_nth(
    CIter* self, size_t index, void* data, size_t data_len, void** out_element);

c_error_t
c_iter_peek(CIter const* self, void* data, size_t data_len, void** out_element);

void c_iter_first(CIter* self, void* data, size_t data_len, void** out_element);

void c_iter_last(CIter* self, void* data, size_t data_len, void** out_element);

#endif // ANYLIBS_ITER_H
