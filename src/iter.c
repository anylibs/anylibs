#include "anylibs/iter.h"
#include "anylibs/error.h"

#include <stdint.h>

CIter c_iter(void* data, size_t data_size, size_t step_size)
{
  CIter iter = {
      .data      = data,
      .data_size = data_size,
      .step_size = step_size,
  };

  return iter;
}

bool c_iter_next(CIter* self, void** out_data)
{
  if (!self->ptr) {
    self->ptr = self->data;
  } else {
    self->ptr = (uint8_t*)self->ptr + self->step_size;
    if ((uint8_t*)self->ptr >= (uint8_t*)self->data + self->data_size) return false;
  }

  if (out_data) *out_data = self->ptr;
  return true;
}

bool c_iter_prev(CIter* self, void** out_data)
{
  if (!self->ptr) {
    self->ptr = (uint8_t*)self->data + self->data_size - self->step_size;
  } else {
    self->ptr = (uint8_t*)self->ptr - self->step_size;
    if ((uint8_t*)self->ptr < (uint8_t*)self->data) return false;
  }

  if (out_data) *out_data = self->ptr;
  return true;
}

bool c_iter_nth(CIter* self, size_t index, void** out_data)
{
  if (index >= self->data_size / self->step_size) {
    c_error_set(C_ERROR_invalid_index);
    return false;
  }

  self->ptr = (uint8_t*)self->data + (index * self->step_size);
  if (out_data) *out_data = self->ptr;
  return true;
}

bool c_iter_peek(CIter const* self, void** out_data)
{
  CIter tmp = *self;

  bool status = c_iter_next(&tmp, out_data);
  return status;
}

void* c_iter_first(CIter* self)
{
  self->ptr = NULL;

  void* out_data;
  c_iter_next(self, &out_data);
  return out_data;
}

void* c_iter_last(CIter* self)
{
  self->ptr = NULL;

  void* out_data;
  c_iter_prev(self, &out_data);
  return out_data;
}
