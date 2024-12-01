/**
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

#include "anylibs/array.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32 && (!_MSC_VER || !(_MSC_VER >= 1900))
#error "You need MSVC must be higher that or equal to 1900"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // disable warning about unsafe functions
#endif

/**
 * @brief intialize a new array object
 * @param[in] element_size
 * @param[out] out_c_array initialize CArray object
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_init(size_t element_size, CArray* out_c_array)
{
  return c_array_create_with_capacity(element_size, 1U, out_c_array);
}

/**
 * @brief same as `c_array_create` but with allocating capacity
 *        (NOTE: malloc_fn is a malloc like function)
 * @param[in] element_size
 * @param[in] capacity maximum number of elements to be allocated, minimum
 *                 capacity is 1
 * @param[out] out_c_array the result CArray object created
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_create_with_capacity(size_t  element_size,
                             size_t  capacity,
                             CArray* out_c_array)
{
  assert(element_size > 0 || capacity > 0);

  if (!out_c_array) return C_ERROR_none;

  *out_c_array      = (CArray){0};
  out_c_array->data = malloc(capacity * element_size);
  if (!out_c_array->data) return C_ERROR_mem_allocation;

  out_c_array->capacity     = capacity;
  out_c_array->element_size = element_size;

  return C_ERROR_none;
}

/**
 * @brief check wether the array is empty
 * @param[in] self
 * @return bool
 */
bool
c_array_is_empty(CArray const* self)
{
  return self->len > 0;
}

/**
 * @brief get array length
 * @param[in] self
 * @return @ref CArray::len
 */
size_t
c_array_len(CArray const* self)
{
  return self->len;
}

/**
 * @brief set array length
 *        this is useful if you want
 *        to manipulate the data by yourself
 * @note this could reset new reallocated @ref CArray::data
 * @param[in] self
 * @param[in] new_len
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_set_len(CArray* self, size_t new_len)
{
  assert(self && self->data);
  assert(new_len > 0);

  if (new_len > self->capacity) {

    /// FIXME:
    c_error_t err = c_array_set_capacity(self, new_len);
    if (err != C_ERROR_none) return err;
  }

  self->len = new_len;
  return C_ERROR_none;
}

/**
 * @brief get array capacity
 *        this will return the capacity in @ref CArray::element_size wise
 *
 *        return 'capacity = 10' which means
 *        we can have up to '10 * element_size' bytes
 * @param[in] self
 * @return @ref CArray::capacity
 */
size_t
c_array_capacity(CArray const* self)
{
  return self->capacity;
}

/**
 * @brief set capacity
 * @note this could reset new reallocated @ref CArray::data
 * @param[in] self address of self
 * @param[in] new_capacity
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_set_capacity(CArray* self, size_t new_capacity)
{
  assert(self && self->data);
  assert(new_capacity > 0);

  void* reallocated_data
      = realloc(self->data, (new_capacity * self->element_size));
  if (!reallocated_data) return C_ERROR_mem_allocation;
  self->data     = reallocated_data;
  self->capacity = new_capacity;

  return C_ERROR_none;
}

/**
 * @brief get elemet_size in bytes
 * @param[in] self
 * @return @ref CArray::element_size
 */
size_t
c_array_element_size(CArray* self)
{
  return self->element_size;
}

/**
 * @brief push one element at the end
 *        if you want to push literals (example: 3, 5 or 10 ...)
 *        c_array_push(array, &(int){3});
 * @note this could reset new reallocated @ref CArray::data
 * @note this will COPY @p element
 * @param[in] self pointer to self
 * @param[in] element a pointer the data of size @ref CArray::element_size
 *                    that you want to push back
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_push(CArray* self, void const* element)
{
  assert(self && self->data);
  assert(element);

  if (self->len >= self->capacity) {
    c_error_t err = c_array_set_capacity(self, self->capacity * 2);
    if (err != C_ERROR_none) return err;
  }

  memcpy((uint8_t*)self->data + (self->len * self->element_size), element,
         self->element_size);
  self->len++;

  return C_ERROR_none;
}

/**
 * @brief pop one element from the end
 * @note this could reset new reallocated @ref CArray::data
 * @param[in] self
 * @param[out] out_element the returned result
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_pop(CArray* self, void* out_element)
{
  assert(self && self->data);

  if (self->len == 0) return C_ERROR_wrong_len;

  c_error_t err = C_ERROR_none;

  if (out_element) {
    memcpy(out_element,
           (uint8_t*)self->data + ((self->len - 1U) * self->element_size),
           self->element_size);
    self->len--;
  }

  if (self->len <= self->capacity / 4) {
    err = c_array_set_capacity(self, self->capacity / 2);
  }

  return err;
}

/**
 * @brief insert 1 element at @ref index
 * @note this could reset new reallocated @ref CArray::data
 * @param[in] self pointer to self
 * @param[in] element a pointer the data of size @ref CArray::element_size
 *                    that you want to push back
 * @param[in] index
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_insert(CArray* self, void const* element, size_t index)
{
  assert(self && self->data);

  if (self->len <= index) return C_ERROR_wrong_index;

  if (self->len == self->capacity) {
    c_error_t err = c_array_set_capacity(self, self->capacity * 2);
    if (err != C_ERROR_none) return err;
  }

  if (index < self->len) {
    memmove((uint8_t*)self->data + ((index + 1) * self->element_size),
            (uint8_t*)self->data + (index * self->element_size),
            (self->len - index) * self->element_size);
  }

  memcpy((uint8_t*)self->data + (index * self->element_size), element,
         self->element_size);
  self->len++;

  return C_ERROR_none;
}

/**
 * @brief insert multiple elements at index
 * @note this could reset new reallocated @ref CArray::data
 * @param[in] self
 * @param[in] index
 * @param[in] data
 * @param[in] data_len
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_insert_range(CArray*     self,
                     size_t      index,
                     void const* data,
                     size_t      data_len)
{
  assert(self && self->data);
  assert(data);
  assert(data_len > 0);

  if (self->len <= index) return C_ERROR_wrong_index;

  while ((self->len + data_len) > self->capacity) {
    c_error_t err = c_array_set_capacity(self, self->capacity * 2);
    if (err != C_ERROR_none) return err;
  }

  if (index < self->len) {
    memmove((uint8_t*)self->data + ((index + data_len) * self->element_size),
            (uint8_t*)self->data + (index * self->element_size),
            (self->len - index) * self->element_size);
  }

  memcpy((uint8_t*)self->data + (index * self->element_size), data,
         data_len * self->element_size);

  self->len += data_len;

  return C_ERROR_none;
}

/**
 * @brief remove element from CArray
 * @note this could reset new reallocated @ref CArray::data
 * @param[in] self
 * @param[in] index index to be removed
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_remove(CArray* self, size_t index)
{
  assert(self && self->data);

  if (index <= self->len) return C_ERROR_wrong_index;

  c_error_t err = C_ERROR_none;

  uint8_t* element = (uint8_t*)self->data + (index * self->element_size);

  memmove(element, element + self->element_size,
          (self->len - index - 1) * self->element_size);
  self->len--;

  if (self->len <= self->capacity / 4) {
    err = c_array_set_capacity(self, self->capacity / 2);
  }

  return err;
}

/**
 * @brief remove a range of elements from CArray starting from @p start_index
 *        with size @p range_size
 * @note this could reset new reallocated @p CArray::data
 * @param[in] self
 * @param[in] start_index
 * @param[in] range_size range length
 * @return return error (any value but zero is treated as an error)
 */
c_error_t
c_array_remove_range(CArray* self, size_t start_index, size_t range_size)
{
  assert(self && self->data);

  c_error_t err = C_ERROR_none;

  if (self->len == 0U) return C_ERROR_wrong_len;
  if (start_index > (self->len - 1U)) return C_ERROR_wrong_index;
  if ((start_index + range_size) > self->len) return C_ERROR_wrong_len;

  uint8_t* start_ptr
      = (uint8_t*)self->data + (start_index * self->element_size);
  uint8_t const* end_ptr = (uint8_t*)self->data
                           + ((start_index + range_size) * self->element_size);
  size_t right_range_size
      = (self->len - (start_index + range_size)) * self->element_size;

  memmove(start_ptr, end_ptr, right_range_size);
  self->len -= range_size;

  if (self->len <= self->capacity / 4) {
    err = c_array_set_capacity(self, self->capacity / 2);
  }

  return err;
}

/**
 * @brief deinit an array object
 *        this will free any allocated memory and set the object data to zero
 * @note this could handle self as NULL
 * @param[in] self
 */
void
c_array_deinit(CArray* self)
{
  if (self && self->data) {
    free(self->data);
    *self = (CArray){0};
  }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
