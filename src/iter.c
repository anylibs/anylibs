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
/// @param step_size this is usually the type size
/// @param step_callback
/// @return new object of CIter
CIter
c_iter(size_t step_size, CIterStepCallback step_callback)
{
  CIter iter = {
      .counter     = 0,
      .is_reversed = false,
      .step_size   = step_size,
      .is_done     = false,
      .step_callback
      = step_callback ? step_callback : c_iter_default_step_callback,
  };

  return iter;
}

/// @brief this is the default step callback for @ref c_iter if @p
///        step_callback was NULL.
///        this public for custom step function (filter like) that could also
///        use this default step function
/// @param self
/// @param data
/// @param data_len
/// @return element
/// @return the state of the step, is_ok: false here means could not continue
///         stepping, true: return the next element
CIterElementResult
c_iter_default_step_callback(CIter* self, void* data, size_t data_len)
{
  assert(self);

  CIterElementResult result            = {0};
  size_t             pos_inc_direction = 1;

  if (self->is_reversed) {
    if (self->is_done) return result;

    if (!self->counter) self->is_done = true;
    pos_inc_direction = -1;
  } else {
    if (self->counter >= data_len) return result;

    self->is_done = false;
  }

  result.element = (uint8_t*)data + (self->counter * self->step_size);
  result.is_ok   = true;
  self->counter += pos_inc_direction;

  return result;
}

/// @brief reverse the direction starting from the last element
/// @param self
/// @param data
/// @param data_len
void
c_iter_rev(CIter* self, void* data, size_t data_len)
{
  assert(self);
  (void)data;

  self->is_reversed = true;
  self->counter     = data_len - 1;
}

/// @brief get the next element
/// @param self
/// @param data
/// @param data_len
/// @return element pointer to the next element
/// @return the state of the step, is_ok: false here means could not continue
///         stepping, true: return the next element
CIterElementResult
c_iter_next(CIter* self, void* data, size_t data_len)
{
  assert(self);

  CIterElementResult result = self->step_callback(self, data, data_len);
  return result;
}

/// @brief move the iterator to the nth element and return its element
/// @param self
/// @param index
/// @param data
/// @param data_len
/// @return is_ok: true -> nth element, false -> not found (or error happened)
CIterElementResult
c_iter_nth(CIter* self, size_t index, void* data, size_t data_len)
{
  assert(self);

  CIterElementResult result = {0};
  if (index >= data_len) return result;

  while ((result = c_iter_next(self, data, data_len)).is_ok) {
    if (self->counter - 1 == index) {
      result.element = (uint8_t*)data + (index * self->step_size);
      result.is_ok   = true;
      break;
    }
  }

  return result;
}

/// @brief get the current data without changing the iterator
/// @param self
/// @param data
/// @param data_len
/// @return is_ok: true -> peek next element, false -> not found (or error
///         happened)
CIterElementResult
c_iter_peek(CIter const* self, void* data, size_t data_len)
{
  assert(self);

  CIter              tmp    = *self;
  CIterElementResult result = c_iter_next(&tmp, data, data_len);

  return result;
}

/// @brief move the iterator to the first element and return its element
/// @param self
/// @param data
/// @param data_len
/// @return is_ok: true -> first element, false -> not found (or error happened)
CIterElementResult
c_iter_first(CIter* self, void* data, size_t data_len)
{
  assert(self);

  CIterElementResult result = c_iter_nth(self, 0, data, data_len);
  return result;
}

/// @brief move the iterator to the last element and return its element
/// @param self
/// @param data
/// @param data_len
/// @return is_ok: true -> last element, false -> not found (or error happened)
CIterElementResult
c_iter_last(CIter* self, void* data, size_t data_len)
{
  assert(self);

  CIterElementResult result = c_iter_nth(self, data_len - 1, data, data_len);
  return result;
}
