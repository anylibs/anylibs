/**
 * @file array.c
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

#define TO_IMPL(arr) ((CArrayImpl*)(arr))

typedef struct CArrayImpl {
  void*  data;     ///< heap allocated data
  size_t len;      ///< current length, note: this unit based not bytes based
  size_t capacity; ///< maximum data that can be hold, note: this unit based
                   ///  not bytes based
  size_t element_size; ///< size of the unit
} CArrayImpl;

/// @brief intialize a new array object
/// @param[in] element_size
/// @param[out] out_c_array the result CArray object initiated
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_init(size_t element_size, CArray* out_c_array)
{
  return c_array_init_with_capacity(element_size, 1U, false, out_c_array);
}

/// @brief same as @ref c_array_init but with allocating capacity
/// @param[in] element_size
/// @param[in] capacity maximum number of elements to be allocated, minimum
///                     capacity is 1
/// @param[in] zeroed_out should zero the memory or not
/// @param[out] out_c_array the result CArray object initiated
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_init_with_capacity(size_t  element_size,
                           size_t  capacity,
                           bool    zeroed_out,
                           CArray* out_c_array)
{
  assert(element_size > 0 || capacity > 0);

  if (!out_c_array) return C_ERROR_none;

  *out_c_array      = (CArray){0};
  out_c_array->data = malloc(capacity * element_size);
  if (!out_c_array->data) return C_ERROR_mem_allocation;

  TO_IMPL(out_c_array)->capacity     = capacity;
  TO_IMPL(out_c_array)->element_size = element_size;

  if (zeroed_out)
    memset(out_c_array->data, 0,
           TO_IMPL(out_c_array)->capacity * TO_IMPL(out_c_array)->element_size);

  return C_ERROR_none;
}

/// @brief initialize @ref CArray from raw pointer
/// @note this will not allocate memory for the data
/// @param[in] data
/// @param[in] data_len this is in @ref CArray element_size not bytes
/// @param[in] element_size
/// @param[out] out_c_array the result CArray object initiated
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_init_from_raw(void*   data,
                      size_t  data_len,
                      size_t  element_size,
                      CArray* out_c_array)
{
  assert(element_size > 0);
  assert(data && data_len > 0);

  if (!out_c_array) return C_ERROR_none;

  *out_c_array                       = (CArray){0};
  out_c_array->data                  = data;
  TO_IMPL(out_c_array)->len          = data_len;
  TO_IMPL(out_c_array)->element_size = element_size;

  return C_ERROR_none;
}

/// @brief clone @ref CArray
/// @note this will make a deep copy
/// @param[in] self
/// @param[in] should_shrink_clone
/// @param[out] out_c_array
/// @return
c_error_t
c_array_clone(CArray const* self, bool should_shrink_clone, CArray* out_c_array)
{
  assert(self && self->data);
  if (!out_c_array) return C_ERROR_none;

  c_error_t err = c_array_init_with_capacity(
      TO_IMPL(self)->element_size,
      should_shrink_clone ? TO_IMPL(self)->len : TO_IMPL(self)->capacity, false,
      out_c_array);
  if (err) return err;

  memcpy(out_c_array->data, self->data, TO_IMPL(self)->len);
  TO_IMPL(out_c_array)->len = TO_IMPL(self)->len;

  return C_ERROR_none;
}

/// @brief check wether the array is empty
/// @param[in] self
/// @return bool
bool
c_array_is_empty(CArray const* self)
{
  assert(self);
  return TO_IMPL(self)->len > 0;
}

/// @brief get array length
/// @param[in] self
/// @return @ref CArray length
size_t
c_array_len(CArray const* self)
{
  assert(self);
  return TO_IMPL(self)->len;
}

/// @brief set array length
///        this is useful if you want
///        to manipulate the data by yourself
/// @note this could reset new reallocated @ref CArray::data
/// @param[in] self
/// @param[in] new_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_set_len(CArray* self, size_t new_len)
{
  assert(self && self->data);
  assert(new_len > 0);

  if (new_len > TO_IMPL(self)->capacity) {
    c_error_t err = c_array_set_capacity(self, new_len);
    if (err != C_ERROR_none) return err;
  }

  TO_IMPL(self)->len = new_len;
  return C_ERROR_none;
}

/// @brief get array capacity
///        this will return the capacity in @ref CArray element_size wise
///        return 'capacity = 10' which means
///        we can have up to '10 * element_size' bytes
/// @param[in] self
/// @return @ref CArray capacity
size_t
c_array_capacity(CArray const* self)
{
  assert(self);
  return TO_IMPL(self)->capacity;
}

/// @brief return the remaining empty space
/// @param[in] self
/// @return the remaining space
size_t
c_array_spare_capacity(CArray const* self)
{
  assert(self);
  return TO_IMPL(self)->capacity - TO_IMPL(self)->len;
}

/// @brief set capacity
/// @note this could reset new reallocated @ref CArray::data
/// @param[in] self address of self
/// @param[in] new_capacity
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_set_capacity(CArray* self, size_t new_capacity)
{
  assert(self && self->data);
  assert(new_capacity > 0);

  void* reallocated_data
      = realloc(self->data, (new_capacity * TO_IMPL(self)->element_size));
  if (!reallocated_data) return C_ERROR_mem_allocation;
  self->data              = reallocated_data;
  TO_IMPL(self)->capacity = new_capacity;

  return C_ERROR_none;
}

/// @brief get elemet_size in bytes
/// @param[in] self
/// @return @ref CArray element_size
size_t
c_array_element_size(CArray* self)
{
  return TO_IMPL(self)->element_size;
}

/// @brief make the @ref CArray capacity equals @ref CArray length
/// @param[in] self
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_shrink_to_fit(CArray* self)
{
  return c_array_set_capacity(self, TO_IMPL(self)->len);
}

/// @brief search for @p element
/// @param[in] self
/// @param[in] element
/// @param[in] cmp this is similar to strcmp
/// @param[out] out_index
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_search(CArray const* self,
               void*         element,
               int           cmp(void const*, void const*),
               size_t*       out_index)
{
  assert(self && self->data);
  if (!out_index || !cmp) return C_ERROR_none;

  for (size_t iii = 0; iii < TO_IMPL(self)->len; iii++) {
    if (cmp(element, (char*)self->data + (iii * TO_IMPL(self)->element_size))
        == 0) {
      *out_index = iii;
      return C_ERROR_none;
    }
  }

  return C_ERROR_not_found;
}

/// @brief search for @p element using binary search tree
/// @note  If @ref CArray::data is not sorted, the returned result is
///        unspecified and meaningless
/// @param[in] self
/// @param[in] element
/// @param[in] cmp this is similar to strcmp
/// @param[out] out_index
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_binary_search(CArray const* self,
                      void const*   element,
                      int           cmp(void const*, void const*),
                      size_t*       out_index)
{
  assert(self && self->data);
  if (!out_index || !cmp) return C_ERROR_none;

  void* out_element = bsearch(element, self->data, TO_IMPL(self)->len,
                              TO_IMPL(self)->element_size, cmp);

  if (out_element) {
    *out_index = ((char*)out_element - (char*)self->data)
                 / TO_IMPL(self)->element_size;
    return C_ERROR_none;
  } else {
    return C_ERROR_not_found;
  }
}

/// @brief check if @p elements is the same as the start elements
///        of @ref CArray::data
/// @param[in] self
/// @param[in] elements
/// @param[in] elements_len
/// @param[in] cmp this is similar to strcmp
/// @return
bool
c_array_starts_with(CArray const* self,
                    void*         elements,
                    size_t        elements_len,
                    int           cmp(void const*, void const*))
{
  assert(self && self->data);
  assert(elements && elements_len > 0);

  if (!cmp) return false;
  if (TO_IMPL(self)->len < elements_len) return false;

  for (size_t iii = 0; iii < elements_len; iii++) {
    if (cmp((char*)self->data + (iii * TO_IMPL(self)->element_size),
            (char*)elements + (iii * TO_IMPL(self)->element_size))
        != 0) {
      return false;
    }
  }

  return true;
}

/// @brief check if @p elements is the same as the end elements
///        of @ref CArray::data
/// @param[in] self
/// @param[in] elements
/// @param[in] elements_len
/// @param[in] cmp this is similar to strcmp
/// @return
bool
c_array_ends_with(CArray const* self,
                  void*         elements,
                  size_t        elements_len,
                  int           cmp(void const*, void const*))
{
  assert(self && self->data);
  assert(elements && elements_len > 0);

  if (!cmp) return false;
  if (TO_IMPL(self)->len < elements_len) return false;

  for (size_t iii = TO_IMPL(self)->len - elements_len, jjj = 0;
       iii < TO_IMPL(self)->len; iii++, jjj++) {
    if (cmp((char*)self->data + (iii * TO_IMPL(self)->element_size),
            (char*)elements + (jjj * TO_IMPL(self)->element_size))
        != 0) {
      return false;
    }
  }

  return true;
}

/// @brief sort
/// @param[in] self
/// @param[in] cmp this is similar to strcmp
void
c_array_sort(CArray* self, int cmp(void const*, void const*))
{
  assert(self && self->data);
  if (!cmp) return;

  qsort(self->data, TO_IMPL(self)->len, TO_IMPL(self)->element_size, cmp);
}

/// @brief check if sorted or not (in ascending order)
/// @param[in] self
/// @return return if sorted or not
bool
c_array_is_sorted(CArray* self, int cmp(void const*, void const*))
{
  assert(self && self->data);

  for (size_t iii = 1; iii < TO_IMPL(self)->len; ++iii) {
    if (cmp((char*)self->data + (iii * TO_IMPL(self)->element_size),
            (char*)self->data + ((iii - 1) * TO_IMPL(self)->element_size))
        < 0) {
      return false;
    }
  }

  return true;
}

/// @brief check if sorted or not (in descending order)
/// @param[in] self
/// @return return if sorted or not
bool
c_array_is_sorted_inv(CArray* self, int cmp(void const*, void const*))
{
  assert(self && self->data);

  for (size_t iii = 1; iii < TO_IMPL(self)->len; ++iii) {
    if (cmp((char*)self->data + (iii * TO_IMPL(self)->element_size),
            (char*)self->data + ((iii - 1) * TO_IMPL(self)->element_size))
        > 0) {
      return false;
    }
  }

  return true;
}

/// @brief get an @p element at @p index
/// @param[in] self
/// @param[in] index
/// @param[out] out_element
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_get(CArray const* self, size_t index, void** out_element)
{
  assert(self && self->data);
  if (!out_element) return C_ERROR_none;
  if (index > TO_IMPL(self)->len) return C_ERROR_wrong_index;

  *out_element = (char*)self->data + (index * TO_IMPL(self)->element_size);
  return C_ERROR_none;
}

/// @brief push one element at the end
///        if you want to push literals (example: 3, 5 or 10 ...)
///        c_array_push(array, &(int){3});
/// @note this could reset new reallocated @ref CArray::data
/// @note this will COPY @p element
/// @param[in] self pointer to self
/// @param[in] element a pointer the data of size @ref CArray element_size
///                    that you want to push back
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_push(CArray* self, void const* element)
{
  assert(self && self->data);
  assert(element);

  if (TO_IMPL(self)->len >= TO_IMPL(self)->capacity) {
    c_error_t err = c_array_set_capacity(self, TO_IMPL(self)->capacity * 2);
    if (err != C_ERROR_none) return err;
  }

  memcpy((uint8_t*)self->data
             + (TO_IMPL(self)->len * TO_IMPL(self)->element_size),
         element, TO_IMPL(self)->element_size);
  TO_IMPL(self)->len++;

  return C_ERROR_none;
}

/// @brief push elements at the end of @ref CArray
/// @note this could reset new reallocated @ref CArray::data
/// @note this will COPY @p element
/// @param[in] self
/// @param[in] elements
/// @param[in] elements_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_push_range(CArray* self, void const* elements, size_t elements_len)
{
  return c_array_insert_range(self, TO_IMPL(self)->len, elements, elements_len);
}

/// @brief pop one element from the end
/// @note this could reset new reallocated @ref CArray::data
/// @param[in] self
/// @param[out] out_element the returned result
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_pop(CArray* self, void* out_element)
{
  assert(self && self->data);

  if (TO_IMPL(self)->len == 0) return C_ERROR_wrong_len;

  c_error_t err = C_ERROR_none;

  if (out_element) {
    memcpy(out_element,
           (uint8_t*)self->data
               + ((TO_IMPL(self)->len - 1U) * TO_IMPL(self)->element_size),
           TO_IMPL(self)->element_size);
    TO_IMPL(self)->len--;
  }

  if (TO_IMPL(self)->len <= TO_IMPL(self)->capacity / 4) {
    err = c_array_set_capacity(self, TO_IMPL(self)->capacity / 2);
  }

  return err;
}

/// @brief insert 1 element at @p index
/// @note this could reset new reallocated @ref CArray::data
/// @param[in] self pointer to self
/// @param[in] element a pointer the data of size @ref CArray element_size
///                    that you want to push back
/// @param[in] index
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_insert(CArray* self, void const* element, size_t index)
{
  assert(self && self->data);

  if (TO_IMPL(self)->len <= index) return C_ERROR_wrong_index;

  if (TO_IMPL(self)->len == TO_IMPL(self)->capacity) {
    c_error_t err = c_array_set_capacity(self, TO_IMPL(self)->capacity * 2);
    if (err != C_ERROR_none) return err;
  }

  if (index < TO_IMPL(self)->len) {
    memmove((uint8_t*)self->data + ((index + 1) * TO_IMPL(self)->element_size),
            (uint8_t*)self->data + (index * TO_IMPL(self)->element_size),
            (TO_IMPL(self)->len - index) * TO_IMPL(self)->element_size);
  }

  memcpy((uint8_t*)self->data + (index * TO_IMPL(self)->element_size), element,
         TO_IMPL(self)->element_size);
  TO_IMPL(self)->len++;

  return C_ERROR_none;
}

/// @brief insert multiple elements at index
/// @note this could reset new reallocated @ref CArray::data
/// @param[in] self
/// @param[in] index
/// @param[in] data
/// @param[in] data_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_insert_range(CArray*     self,
                     size_t      index,
                     void const* data,
                     size_t      data_len)
{
  assert(self && self->data);
  assert(data);
  assert(data_len > 0);

  if (TO_IMPL(self)->len < index) return C_ERROR_wrong_index;

  while ((TO_IMPL(self)->len + data_len) > TO_IMPL(self)->capacity) {
    c_error_t err = c_array_set_capacity(self, TO_IMPL(self)->capacity * 2);
    if (err != C_ERROR_none) return err;
  }

  if (index < TO_IMPL(self)->len) {
    memmove((uint8_t*)self->data
                + ((index + data_len) * TO_IMPL(self)->element_size),
            (uint8_t*)self->data + (index * TO_IMPL(self)->element_size),
            (TO_IMPL(self)->len - index) * TO_IMPL(self)->element_size);
  }

  memcpy((uint8_t*)self->data + (index * TO_IMPL(self)->element_size), data,
         data_len * TO_IMPL(self)->element_size);

  TO_IMPL(self)->len += data_len;

  return C_ERROR_none;
}

/// @brief fill the whole @ref CArray capacity with @p data
/// @param[in] self
/// @param[in] data
void
c_array_fill(CArray* self, void* data)
{
  assert(self && self->data);
  if (!data) return;

  for (size_t iii = 0; iii < TO_IMPL(self)->capacity; iii++) {
    memcpy((char*)self->data + (iii * TO_IMPL(self)->element_size), data,
           TO_IMPL(self)->element_size);
  }
  TO_IMPL(self)->len = TO_IMPL(self)->capacity;
}

/// @brief concatenate arr2 @ref CArray::data to arr1 @ref CArray::data
/// @param[in] arr1
/// @param[in] arr2
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_concatenate(CArray* arr1, CArray const* arr2)
{
  assert(arr1 && arr1->data);
  assert(arr2 && arr2->data);

  c_error_t err = C_ERROR_none;

  if (TO_IMPL(arr1)->element_size != TO_IMPL(arr2)->element_size)
    return C_ERROR_wrong_element_size;

  if (TO_IMPL(arr1)->capacity < (TO_IMPL(arr1)->len + TO_IMPL(arr2)->len)) {
    err = c_array_set_capacity(arr1, TO_IMPL(arr1)->len + TO_IMPL(arr2)->len);
    if (err) return err;
  }

  memcpy((char*)arr1->data + (TO_IMPL(arr1)->len * TO_IMPL(arr1)->element_size),
         arr2->data, TO_IMPL(arr2)->len * TO_IMPL(arr2)->element_size);

  TO_IMPL(arr1)->len += TO_IMPL(arr2)->len;

  return err;
}

/// @brief fill @ref CArray::data with repeated @p data
/// @param[in] self
/// @param[in] data
/// @param[in] data_len this is in @ref CArray element_size not bytes
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_fill_with_repeat(CArray* self, void* data, size_t data_len)
{
  assert(self && self->data);
  if (data_len > TO_IMPL(self)->capacity) return C_ERROR_wrong_len;

  size_t const number_of_repeats = TO_IMPL(self)->capacity / data_len;
  size_t const repeat_size       = data_len * TO_IMPL(self)->element_size;
  for (size_t iii = 0; iii < number_of_repeats; ++iii) {
    memcpy((char*)self->data + (iii * (repeat_size)), data,
           data_len * TO_IMPL(self)->element_size);
  }

  TO_IMPL(self)->len = number_of_repeats * data_len;

  return C_ERROR_none;
}

/// @brief rotate @p elements_count to the right
/// @param[in] self
/// @param[in] elements_count
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_rotate_right(CArray* self, size_t elements_count)
{
  assert(self && self->data);
  if ((elements_count == 0) || (elements_count > TO_IMPL(self)->len))
    return C_ERROR_none;

  void* tmp = malloc(elements_count * TO_IMPL(self)->element_size);
  if (!tmp) return C_ERROR_mem_allocation;

  memcpy(tmp,
         (char*)self->data
             + ((TO_IMPL(self)->len - elements_count)
                * TO_IMPL(self)->element_size),
         elements_count * TO_IMPL(self)->element_size);

  memmove((char*)self->data + (elements_count * TO_IMPL(self)->element_size),
          self->data, elements_count * TO_IMPL(self)->element_size);
  memcpy(self->data, tmp, elements_count * TO_IMPL(self)->element_size);

  free(tmp);

  return C_ERROR_none;
}

/// @brief rotate @p elements_count to the left
/// @param[in] self
/// @param[in] elements_count
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_rotate_left(CArray* self, size_t elements_count)
{
  assert(self && self->data);
  if ((elements_count == 0) || (elements_count > TO_IMPL(self)->len))
    return C_ERROR_none;

  void* tmp = malloc(elements_count * TO_IMPL(self)->element_size);
  if (!tmp) return C_ERROR_mem_allocation;

  memcpy(tmp, self->data, elements_count * TO_IMPL(self)->element_size);

  memmove(self->data,
          (char*)self->data + (elements_count * TO_IMPL(self)->element_size),
          elements_count * TO_IMPL(self)->element_size);
  memcpy((char*)self->data
             + ((TO_IMPL(self)->len - elements_count)
                * TO_IMPL(self)->element_size),
         tmp, elements_count * TO_IMPL(self)->element_size);

  free(tmp);

  return C_ERROR_none;
}

/// @brief remove element from CArray
/// @note this could reset new reallocated @ref CArray::data
/// @param[in] self
/// @param[in] index index to be removed
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_remove(CArray* self, size_t index)
{
  assert(self && self->data);

  if (index <= TO_IMPL(self)->len) return C_ERROR_wrong_index;

  c_error_t err = C_ERROR_none;

  uint8_t* element
      = (uint8_t*)self->data + (index * TO_IMPL(self)->element_size);

  memmove(element, element + TO_IMPL(self)->element_size,
          (TO_IMPL(self)->len - index - 1) * TO_IMPL(self)->element_size);
  TO_IMPL(self)->len--;

  if (TO_IMPL(self)->len <= TO_IMPL(self)->capacity / 4) {
    err = c_array_set_capacity(self, TO_IMPL(self)->capacity / 2);
  }

  return err;
}

/// @brief remove a range of elements from CArray starting from @p start_index
///        with size @p range_size
/// @note this could reset new reallocated @p CArray::data
/// @param[in] self
/// @param[in] start_index
/// @param[in] range_size range length
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_remove_range(CArray* self, size_t start_index, size_t range_size)
{
  assert(self && self->data);

  c_error_t err = C_ERROR_none;

  if (TO_IMPL(self)->len == 0U) return C_ERROR_wrong_len;
  if (start_index > (TO_IMPL(self)->len - 1U)) return C_ERROR_wrong_index;
  if ((start_index + range_size) > TO_IMPL(self)->len) return C_ERROR_wrong_len;

  uint8_t* start_ptr
      = (uint8_t*)self->data + (start_index * TO_IMPL(self)->element_size);
  uint8_t const* end_ptr
      = (uint8_t*)self->data
        + ((start_index + range_size) * TO_IMPL(self)->element_size);
  size_t right_range_size = (TO_IMPL(self)->len - (start_index + range_size))
                            * TO_IMPL(self)->element_size;

  memmove(start_ptr, end_ptr, right_range_size);
  TO_IMPL(self)->len -= range_size;

  if (TO_IMPL(self)->len <= TO_IMPL(self)->capacity / 4) {
    err = c_array_set_capacity(self, TO_IMPL(self)->capacity / 2);
  }

  return err;
}

/// @brief remove duplicated elements in place
/// @note this is a very costy function
/// @param[in] self
/// @param[in] cmp this is similar to strcmp
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_deduplicate(CArray* self, int cmp(void const*, void const*))
{
  assert(self && self->data);

  if (!cmp) return C_ERROR_none;

  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    for (size_t jjj = iii + 1; jjj < TO_IMPL(self)->len; ++jjj) {
      if (cmp((char*)self->data + (TO_IMPL(self)->element_size * iii),
              (char*)self->data + (TO_IMPL(self)->element_size * jjj))
          == 0) {
        memmove((char*)self->data + (jjj * TO_IMPL(self)->element_size),
                (char*)self->data + ((jjj + 1) * TO_IMPL(self)->element_size),
                (TO_IMPL(self)->len - jjj + 1) * TO_IMPL(self)->element_size);
        TO_IMPL(self)->len--;
        jjj--;
      }
    }
  }

  return C_ERROR_none;
}

/// @brief get a slice from @ref CArray
/// @note this is only reference to the data
///       so @ref CArray capacity will be zero
/// @param[in] self
/// @param[in] start_index
/// @param[in] range if range is bigger than @ref CArray length,
///                  @ref CArray length will be the returned range
/// @param[out] out_slice
/// @return error (any value but zero is treated as an error)
c_error_t
c_array_slice(CArray const* self,
              size_t        start_index,
              size_t        range,
              CArray*       out_slice)
{
  assert(self && self->data);

  if (start_index > TO_IMPL(self)->len) return C_ERROR_wrong_index;
  range
      = (TO_IMPL(self)->len - start_index) < range ? TO_IMPL(self)->len : range;

  *out_slice = *(CArray*)&((CArrayImpl){
      .data = (char*)self->data + (start_index * TO_IMPL(self)->element_size),
      .len  = range,
      .element_size = TO_IMPL(self)->element_size});

  return C_ERROR_none;
}

/// @brief an iterator to loop over the @ref CArray::data
/// @param[in] self
/// @param[in] index pointer to a counter index, it will keep count till @ref
///              CArray::len
/// @param[in] element a pointer to the current element
/// @return bool indicating if we reached the end of the array
bool
c_array_iter(CArray* self, size_t* index, void** element)
{
  assert(self && self->data);
  if (!index || !element) return false;

  if (*index < TO_IMPL(self)->len) {
    *element = (char*)self->data + (*index * TO_IMPL(self)->element_size);
    *index += 1;
    return true;
  }

  return false;
}

/// @brief reverse the @ref CArray::data in place
/// @param[in] self
c_error_t
c_array_reverse(CArray* self)
{
  assert(self && self->data);

  char* start = self->data;
  char* end   = (char*)self->data
              + ((TO_IMPL(self)->len - 1) * TO_IMPL(self)->element_size);

  void* tmp = malloc(TO_IMPL(self)->element_size);
  if (!tmp) return C_ERROR_mem_allocation;

  while (end > start) {
    memcpy(tmp, start, TO_IMPL(self)->element_size);
    memcpy(start, end, TO_IMPL(self)->element_size);
    memcpy(end, tmp, TO_IMPL(self)->element_size);

    start += TO_IMPL(self)->element_size;
    end -= TO_IMPL(self)->element_size;
  }

  free(tmp);

  return C_ERROR_none;
}

/// @brief clear the @ref CArray without changing the capacity
/// @param[in] self
void
c_array_clear(CArray* self)
{
  assert(self && self->data);
  TO_IMPL(self)->len = 0;
}

/// @brief deinit an array object
///        this will free any allocated memory and set the object data to zero
/// @note this could handle self as NULL
/// @param[in] self
void
c_array_deinit(CArray* self)
{
  if (self && self->data) {
    if (TO_IMPL(self)->capacity) free(self->data);
    *self = (CArray){0};
  }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
