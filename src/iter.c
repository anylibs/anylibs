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

/// @brief create an iterator
/// @param[in] step_size this is usually the type size
/// @param[in] step_callback
/// @param[out] out_c_iter
void
c_iter_create(size_t            step_size,
              CIterStepCallback step_callback,
              CIter*            out_c_iter)
{
  if (!out_c_iter) return;

  *out_c_iter = (CIter){
      .counter     = 0,
      .is_reversed = false,
      .step_size   = step_size,
      .is_done     = false,
      .step_callback
      = step_callback ? step_callback : c_iter_default_step_callback,
  };
}

/// @brief this is the default step callback for @ref c_iter_create if @p
///        step_callback was NULL.
///        this public for custom step function (filter like) that could also
///        use this default step function
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
/// @param[out] out_element
/// @return the state of the step, false here means could not continue stepping
bool
c_iter_default_step_callback(CIter* self,
                             void*  data,
                             size_t data_len,
                             void** out_element)
{
  assert(self);

  size_t pos_inc_direction = 1;

  if (self->is_reversed) {
    if (self->is_done) return false;

    if (!self->counter) self->is_done = true;
    pos_inc_direction = -1;
  } else {
    if (self->counter >= data_len) return false;

    self->is_done = false;
  }

  if (out_element)
    *out_element = (uint8_t*)data + (self->counter * self->step_size);
  self->counter += pos_inc_direction;

  return true;
}

/// @brief reverse the direction starting from the last element
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
void
c_iter_rev(CIter* self, void* data, size_t data_len)
{
  assert(self);
  (void)data;

  self->is_reversed = true;
  self->counter     = data_len - 1;
}

/// @brief get the next element
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
/// @param[out] out_element pointer to the next element
/// @return the state of the step, false here means could not continue stepping
bool
c_iter_next(CIter* self, void* data, size_t data_len, void** out_element)
{
  assert(self);

  return self->step_callback(self, data, data_len, out_element);
}

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
  CIter tmp    = *self;
  bool  status = c_iter_next(&tmp, data, data_len, out_element);
  if (!status) return C_ERROR_wrong_index;

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
