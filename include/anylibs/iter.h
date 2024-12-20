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

typedef struct CIterElementResult {
  void* element;
  bool  is_ok;
} CIterElementResult;

typedef struct CIter CIter;
typedef CIterElementResult (*CIterStepCallback)(CIter* self,
                                                void*  data,
                                                size_t data_len);

struct CIter {
  size_t            counter;
  size_t            step_size;     ///< this is usually the element size
  CIterStepCallback step_callback; ///< this is mainly act like c_iter_next for
                                   ///< whatever the direction
  bool is_reversed;                ///< thiis is true when go backward
  bool is_done; ///< this is used only if @ref CIter::is_reversed is true
};

CIter c_iter(size_t step_size, CIterStepCallback step_callback);

CIterElementResult
c_iter_default_step_callback(CIter* self, void* data, size_t data_len);

void c_iter_rev(CIter* self, void* data, size_t data_len);

CIterElementResult c_iter_next(CIter* self, void* data, size_t data_len);

CIterElementResult
c_iter_nth(CIter* self, size_t index, void* data, size_t data_len);

CIterElementResult c_iter_peek(CIter const* self, void* data, size_t data_len);

CIterElementResult c_iter_first(CIter* self, void* data, size_t data_len);

CIterElementResult c_iter_last(CIter* self, void* data, size_t data_len);

#endif // ANYLIBS_ITER_H
