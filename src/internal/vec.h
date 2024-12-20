/**
 * @file internal/vec.h
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

#ifndef ANYLIBS_INTERNAL_VEC_H
#define ANYLIBS_INTERNAL_VEC_H

#include <stddef.h>

#include "anylibs/allocator.h"

typedef struct CVecImpl {
  void*       data;         ///< heap allocated data
  size_t      len;          ///< current length in bytes
  CAllocator* allocator;    ///< memory allocator/deallocator
  size_t      raw_capacity; ///< this is only useful when used with
                            ///< @ref c_vec_create_from_raw
  size_t element_size;      ///< size of the element unit
} CVecImpl;

#endif // ANYLIBS_INTERNAL_VEC_H
