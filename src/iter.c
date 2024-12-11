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

#include "anylibs/iter.h"

#include <assert.h>

static bool c_internal_iter_next(CIter* self,
                                 void*  data,
                                 size_t data_len,
                                 void** out_element);

/// @brief create an iterator
/// @param[in] step_size this is usually the type size
/// @param[in] is_reversed the iterator direction
/// @param[in] step_callback
/// @param[out] out_c_iter
/// @return error (any value but zero is treated as an error)
c_error_t
c_iter_create(size_t            step_size,
              bool              is_reversed,
              CIterStepCallback step_callback,
              CIter*            out_c_iter)
{
  if (!out_c_iter) return C_ERROR_none;

  *out_c_iter = (CIter){
      .current_pos   = 0,
      .is_reversed   = is_reversed,
      .step_size     = step_size,
      .is_done       = false,
      .step_callback = step_callback ? step_callback : c_internal_iter_next,
  };

  return C_ERROR_none;
}

/// @brief reverse the direction starting from the last element
/// @param[in] self
/// @param[in] data
/// @param data_len
void
c_iter_rev(CIter* self, void* data, size_t data_len)
{
  assert(self);
  (void)data;

  self->current_pos = data_len - 1;
}

/// @brief get the next element
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
/// @param[out] out_element pointer to the next element
/// @return error (any value but zero is treated as an error)
bool
c_iter_next(CIter* self, void* data, size_t data_len, void** out_element)
{
  assert(self);

  return self->step_callback(self, data, data_len, out_element);
}

#include <stdio.h>
/// @brief move the iterator to the nth element and return its element
/// @param[in] self
/// @param[in] index
/// @param[in] data
/// @param[in] data_len
/// @param[out] out_element pointer to the nth element
/// @return error (any value but zero is treated as an error)
c_error_t
c_iter_nth(
    CIter* self, size_t index, void* data, size_t data_len, void** out_element)
{
  assert(self);

  if (index >= data_len) return C_ERROR_wrong_index;

  size_t counter = 0;
  while (c_iter_next(self, data, data_len, out_element)) {
    if (counter++ == index) break;
  }

  return C_ERROR_none;
}

/// @brief get the current data without changing the iterator
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
/// @param[out] out_element pointer to the current element
/// @return error (any value but zero is treated as an error)
c_error_t
c_iter_peek(CIter const* self, void* data, size_t data_len, void** out_element)
{
  assert(self);
  (void)data_len;

  if (self->is_done || (self->current_pos >= data_len))
    return C_ERROR_wrong_index;

  if (out_element)
    *out_element = (uint8_t*)data + (self->current_pos * self->step_size);

  return C_ERROR_none;
}

/// @brief move the iterator to the first element and return its element
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
/// @param[out] out_element pointer to the first element
void
c_iter_first(CIter* self, void* data, size_t data_len, void** out_element)
{
  c_iter_nth(self, 0, data, data_len, out_element);
}

/// @brief move the iterator to the last element and return its element
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
/// @param[out] out_element pointer to the last element
void
c_iter_last(CIter* self, void* data, size_t data_len, void** out_element)
{
  c_iter_nth(self, data_len - 1, data, data_len, out_element);
}

/******************************************************************************/
/*                                  Internal                                  */
/******************************************************************************/

bool
c_internal_iter_next(CIter* self,
                     void*  data,
                     size_t data_len,
                     void** out_element)
{
  assert(self);

  size_t pos_inc_direction = 1;

  if (self->is_reversed) {
    if (self->is_done) return false;

    if (!self->current_pos) self->is_done = true;
    pos_inc_direction = -1;
  } else {
    if (self->current_pos >= data_len) return false;

    self->is_done = false;
  }

  if (out_element)
    *out_element = (uint8_t*)data + (self->current_pos * self->step_size);
  self->current_pos += pos_inc_direction;

  return true;
}
