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

#include "anylibs/vec.h"

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

#define TO_IMPL(vec) ((CVecImpl*)(vec))
#define FROM_IMPL(impl) ((CVec*)(impl))
#define TO_BYTES(vec, units) ((units) * TO_IMPL(vec)->element_size)
#define TO_UNITS(vec, bytes) ((bytes) / TO_IMPL(vec)->element_size)
#define GET_CAPACITY(vec)                                                      \
  (TO_IMPL(vec)->raw_capacity > 0 ? TO_IMPL(vec)->raw_capacity                 \
                                  : c_allocator_mem_size(TO_IMPL(vec)->data))

typedef struct CVecImpl {
  void*       data;         ///< heap allocated data
  size_t      len;          ///< current length in bytes
  size_t      element_size; ///< size of the element unit
  CAllocator* allocator;    ///< memory allocator/deallocator
  size_t      raw_capacity; ///< this is only useful when used with
                            ///< @ref c_vec_create_from_raw
} CVecImpl;

/// @brief intialize a new vec object
/// @param[in] element_size
/// @param[in] allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @param[out] out_c_vec the result CVec object created
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_create(size_t element_size, CAllocator* allocator, CVec** out_c_vec)
{
  return c_vec_create_with_capacity(element_size, 1U, false, allocator,
                                    out_c_vec);
}

/// @brief same as @ref c_vec_create but with allocating capacity
/// @param[in] element_size
/// @param[in] capacity maximum number of elements to be allocated, minimum
///                     capacity is 1
/// @param[in] zero_initialized should zero the memory or not
/// @param[in] allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @param[out] out_c_vec the result CVec object created
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_create_with_capacity(size_t      element_size,
                           size_t      capacity,
                           bool        zero_initialized,
                           CAllocator* allocator,
                           CVec**      out_c_vec)
{
  assert(element_size > 0 || capacity > 0);

  if (!out_c_vec) return C_ERROR_none;
  if (!allocator) c_allocator_default(&allocator);

  CVecImpl* impl;
  void*     data_mem;
  c_error_t err = c_allocator_alloc(allocator, c_allocator_alignas(CVecImpl, 1),
                                    zero_initialized, (void**)&impl);
  if (err) goto ERROR_ALLOC;
  err = c_allocator_alloc(allocator, capacity * element_size, element_size,
                          zero_initialized, &data_mem);
  if (err) goto ERROR_ALLOC;

  impl->data         = data_mem;
  impl->element_size = element_size;
  impl->len          = 0;
  impl->allocator    = allocator;
  impl->raw_capacity = 0;

  *out_c_vec = FROM_IMPL(impl);

  return C_ERROR_none;

ERROR_ALLOC:
  c_allocator_free(allocator, (void**)&impl);
  c_allocator_free(allocator, &data_mem);
  return C_ERROR_mem_allocation;
}

/// @brief create @ref CVecImpl from raw pointer
/// @note this will not allocate memory for the data
/// @param[in] data
/// @param[in] data_len this is in @ref CVecImpl::element_size not bytes
/// @param[in] element_size
/// @param[in] allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @param[out] out_c_vec the result CVec object created
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_create_from_raw(void*       data,
                      size_t      data_len,
                      size_t      element_size,
                      CAllocator* allocator,
                      CVec**      out_c_vec)
{
  assert(element_size > 0);
  assert(data && data_len > 0);

  if (!out_c_vec) return C_ERROR_none;
  if (!allocator) c_allocator_default(&allocator);

  CVecImpl* impl;
  c_error_t err = c_allocator_alloc(allocator, c_allocator_alignas(CVecImpl, 1),
                                    false, (void**)&impl);
  if (err) goto ERROR_ALLOC;

  impl->data         = data;
  impl->element_size = element_size;
  impl->len          = data_len;
  impl->allocator    = allocator;
  impl->raw_capacity = data_len;

  *out_c_vec = FROM_IMPL(impl);

  return C_ERROR_none;

ERROR_ALLOC:
  c_allocator_free(allocator, (void**)&impl);
  return C_ERROR_mem_allocation;
}

/// @brief clone @ref CVecImpl
/// @note this will make a deep copy
/// @param[in] self
/// @param[in] should_shrink_clone
/// @param[out] out_c_vec
/// @return
c_error_t
c_vec_clone(CVec const* self, bool should_shrink_clone, CVec** out_c_vec)
{
  assert(self && TO_IMPL(self)->data);
  if (!out_c_vec) return C_ERROR_none;

  c_error_t err = c_vec_create_with_capacity(
      TO_IMPL(self)->element_size,
      should_shrink_clone ? TO_UNITS(self, TO_IMPL(self)->len)
                          : TO_UNITS(self, GET_CAPACITY(self)),
      false, TO_IMPL(self)->allocator, out_c_vec);
  if (err) return err;

  memcpy(TO_IMPL(*out_c_vec)->data, TO_IMPL(self)->data, TO_IMPL(self)->len);
  TO_IMPL(*out_c_vec)->len = TO_IMPL(self)->len;

  return C_ERROR_none;
}

/// @brief check wether the vec is empty
/// @param[in] self
/// @return bool
bool
c_vec_is_empty(CVec const* self)
{
  assert(self);
  return TO_IMPL(self)->len > 0;
}

/// @brief get vec length
/// @param[in] self
/// @return @ref CVecImpl::len
size_t
c_vec_len(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, TO_IMPL(self)->len);
}

/// @brief set vec length
///        this is useful if you want
///        to manipulate the data by yourself
/// @note this could reset new reallocated @ref CVecImpl::data
/// @param[in] self
/// @param[in] new_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_set_len(CVec* self, size_t new_len)
{
  assert(self && TO_IMPL(self)->data);
  assert(new_len > 0);

  if (TO_BYTES(self, new_len) <= GET_CAPACITY(self)) {
  TO_IMPL(self)->len = TO_BYTES(self, new_len);
  }

  return C_ERROR_none;
}

/// @brief get vec capacity
///        this will return the capacity in @ref CVecImpl::element_size wise
///        return 'capacity = 10' which means
///        we can have up to '10 * element_size' bytes
/// @param[in] self
/// @return @ref CVecImpl::capacity
size_t
c_vec_capacity(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, GET_CAPACITY(self));
}

/// @brief return the remaining empty space
/// @param[in] self
/// @return the remaining space
size_t
c_vec_spare_capacity(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, GET_CAPACITY(self) - TO_IMPL(self)->len);
}

/// @brief set capacity
/// @note this could reset new reallocated @ref CVecImpl::data
/// @param[in] self address of self
/// @param[in] new_capacity
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_set_capacity(CVec* self, size_t new_capacity)
{
  assert(self && TO_IMPL(self)->data);
  assert(new_capacity > 0);

  if (TO_IMPL(self)->raw_capacity > 0) {
    if (new_capacity <= TO_IMPL(self)->raw_capacity) {
      TO_IMPL(self)->raw_capacity = new_capacity;
      return C_ERROR_none;
    } else {
      return C_ERROR_capacity_full;
    }
  }

  size_t new_len = (TO_BYTES(self, new_capacity) < TO_IMPL(self)->len)
                       ? TO_BYTES(self, new_capacity)
                       : TO_IMPL(self)->len;

  c_error_t err
      = c_allocator_resize(TO_IMPL(self)->allocator, &TO_IMPL(self)->data,
                           TO_BYTES(self, new_capacity));
  if (!err) TO_IMPL(self)->len = new_len;
  return err;
}

/// @brief get elemet_size in bytes
/// @param[in] self
/// @return @ref CVecImpl::element_size
size_t
c_vec_element_size(CVec* self)
{
  return TO_IMPL(self)->element_size;
}

/// @brief make the @ref CVecImpl::capacity equals @ref CVecImpl::len
/// @param[in] self
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_shrink_to_fit(CVec* self)
{
  return c_vec_set_capacity(self, TO_UNITS(self, TO_IMPL(self)->len));
}

/// @brief search for @p element
/// @param[in] self
/// @param[in] element
/// @param[in] cmp this is similar to strcmp
/// @param[out] out_index
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_search(CVec const* self,
             void*       element,
             int         cmp(void const*, void const*),
             size_t*     out_index)
{
  assert(self && TO_IMPL(self)->data);
  if (!out_index || !cmp) return C_ERROR_none;

  for (size_t iii = 0; iii < TO_UNITS(self, TO_IMPL(self)->len); iii++) {
    if (cmp(element, (char*)TO_IMPL(self)->data + TO_BYTES(self, iii)) == 0) {
      *out_index = iii;
      return C_ERROR_none;
    }
  }

  return C_ERROR_not_found;
}

/// @brief search for @p element using binary search tree
/// @note  If @ref CVecImpl::data is not sorted, the returned result is
///        unspecified and meaningless
/// @param[in] self
/// @param[in] element
/// @param[in] cmp this is similar to strcmp
/// @param[out] out_index
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_binary_search(CVec const* self,
                    void const* element,
                    int         cmp(void const*, void const*),
                    size_t*     out_index)
{
  assert(self && TO_IMPL(self)->data);
  if (!out_index || !cmp) return C_ERROR_none;

  void* out_element = bsearch(element, TO_IMPL(self)->data,
                              TO_UNITS(self, TO_IMPL(self)->len),
                              TO_IMPL(self)->element_size, cmp);

  if (out_element) {
    *out_index
        = TO_UNITS(self, (char*)out_element - (char*)TO_IMPL(self)->data);
    return C_ERROR_none;
  } else {
    return C_ERROR_not_found;
  }
}

/// @brief check if @p elements is the same as the start elements
///        of @ref CVecImpl::data
/// @param[in] self
/// @param[in] elements
/// @param[in] elements_len
/// @param[in] cmp this is similar to strcmp
/// @return
bool
c_vec_starts_with(CVec const* self,
                  void const* elements,
                  size_t      elements_len,
                  int         cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);
  assert(elements && elements_len > 0);

  if (!cmp) return false;
  if (TO_IMPL(self)->len < TO_BYTES(self, elements_len)) return false;

  for (size_t iii = 0; iii < elements_len; iii++) {
    if (cmp((char*)TO_IMPL(self)->data + TO_BYTES(self, iii),
            (char*)elements + TO_BYTES(self, iii))
        != 0) {
      return false;
    }
  }

  return true;
}

/// @brief check if @p elements is the same as the end elements
///        of @ref CVecImpl::data
/// @param[in] self
/// @param[in] elements
/// @param[in] elements_len
/// @param[in] cmp this is similar to strcmp
/// @return
bool
c_vec_ends_with(CVec const* self,
                void const* elements,
                size_t      elements_len,
                int         cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);
  assert(elements && elements_len > 0);

  if (!cmp) return false;
  if (TO_IMPL(self)->len < TO_BYTES(self, elements_len)) return false;

  for (size_t iii = TO_UNITS(self, TO_IMPL(self)->len) - elements_len, jjj = 0;
       iii < TO_UNITS(self, TO_IMPL(self)->len); iii++, jjj++) {
    if (cmp((char*)TO_IMPL(self)->data + TO_BYTES(self, iii),
            (char*)elements + TO_BYTES(self, jjj))
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
c_vec_sort(CVec* self, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);
  if (!cmp) return;

  qsort(TO_IMPL(self)->data, TO_UNITS(self, TO_IMPL(self)->len),
        TO_IMPL(self)->element_size, cmp);
}

/// @brief check if sorted or not (in ascending order)
/// @param[in] self
/// @param[in] cmp this is similar to strcmp
/// @return return if sorted or not
bool
c_vec_is_sorted(CVec* self, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);

  for (size_t iii = 1; iii < TO_UNITS(self, TO_IMPL(self)->len); ++iii) {
    if (cmp((char*)TO_IMPL(self)->data + TO_BYTES(self, iii),
            (char*)TO_IMPL(self)->data + TO_BYTES(self, iii - 1))
        < 0) {
      return false;
    }
  }

  return true;
}

/// @brief check if sorted or not (in descending order)
/// @param[in] self
/// @param[in] cmp this is similar to strcmp
/// @return return if sorted or not
bool
c_vec_is_sorted_inv(CVec* self, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);

  for (size_t iii = 1; iii < TO_UNITS(self, TO_IMPL(self)->len); ++iii) {
    if (cmp((char*)TO_IMPL(self)->data + TO_BYTES(self, iii),
            (char*)TO_IMPL(self)->data + TO_BYTES(self, iii - 1))
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
c_vec_get(CVec const* self, size_t index, void** out_element)
{
  assert(self && TO_IMPL(self)->data);
  if (!out_element) return C_ERROR_none;
  if (TO_BYTES(self, index) > TO_IMPL(self)->len) return C_ERROR_wrong_index;

  *out_element = (char*)TO_IMPL(self)->data + TO_BYTES(self, index);
  return C_ERROR_none;
}

/// @brief push one element at the end
///        if you want to push literals (example: 3, 5 or 10 ...)
///        c_vec_push(vec, &(int){3});
/// @note this could reset new reallocated @ref CVecImpl::data
/// @note this will COPY @p element
/// @param[in] self pointer to self
/// @param[in] element a pointer the data of size @ref CVecImpl::element_size
///                    that you want to push back
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_push(CVec* self, void const* element)
{
  assert(self && TO_IMPL(self)->data);
  assert(element);

  if (TO_IMPL(self)->len >= GET_CAPACITY(self)) {
    c_error_t err
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (err != C_ERROR_none) return err;
  }

  memcpy((uint8_t*)TO_IMPL(self)->data + TO_IMPL(self)->len, element,
         TO_IMPL(self)->element_size);
  TO_IMPL(self)->len += TO_IMPL(self)->element_size;

  return C_ERROR_none;
}

/// @brief push elements at the end of @ref CVecImpl
/// @note this could reset new reallocated @ref CVecImpl::data
/// @note this will COPY @p element
/// @param[in] self
/// @param[in] elements
/// @param[in] elements_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_push_range(CVec* self, void const* elements, size_t elements_len)
{
  return c_vec_insert_range(self, TO_UNITS(self, TO_IMPL(self)->len), elements,
                            elements_len);
}

/// @brief pop one element from the end
/// @note this could reset new reallocated @ref CVecImpl::data
/// @param[in] self
/// @param[out] out_element the returned result
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_pop(CVec* self, void* out_element)
{
  assert(self && TO_IMPL(self)->data);

  if (TO_IMPL(self)->len == 0) return C_ERROR_wrong_len;

  c_error_t err = C_ERROR_none;

  if (out_element) {
    memcpy(out_element,
           (uint8_t*)TO_IMPL(self)->data + TO_IMPL(self)->len
               - TO_IMPL(self)->element_size,
           TO_IMPL(self)->element_size);
    TO_IMPL(self)->len -= TO_IMPL(self)->element_size;
  }

  if (TO_IMPL(self)->len <= (GET_CAPACITY(self) / 4)) {
    err = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
  }

  return err;
}

/// @brief insert 1 element at @p index
/// @note this could reset new reallocated @ref CVecImpl::data
/// @param[in] self pointer to self
/// @param[in] index
/// @param[in] element a pointer the data of size @ref CVecImpl::element_size
///                    that you want to push back
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_insert(CVec* self, size_t index, void const* element)
{
  assert(self && TO_IMPL(self)->data);

  if (TO_IMPL(self)->len <= TO_BYTES(self, index)) return C_ERROR_wrong_index;

  if (TO_IMPL(self)->len == GET_CAPACITY(self)) {
    c_error_t err
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (err != C_ERROR_none) return err;
  }

  if (index < TO_UNITS(self, TO_IMPL(self)->len)) {
    memmove((uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index + 1),
            (uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index),
            TO_IMPL(self)->len - TO_BYTES(self, index));
  }

  memcpy((uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index), element,
         TO_IMPL(self)->element_size);
  TO_IMPL(self)->len += TO_IMPL(self)->element_size;

  return C_ERROR_none;
}

/// @brief insert multiple elements at index
/// @note this could reset new reallocated @ref CVecImpl::data
/// @param[in] self
/// @param[in] index
/// @param[in] data
/// @param[in] data_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_insert_range(CVec* self, size_t index, void const* data, size_t data_len)
{
  assert(self && TO_IMPL(self)->data);
  assert(data);
  assert(data_len > 0);

  if (TO_IMPL(self)->len < TO_BYTES(self, index)) return C_ERROR_wrong_index;

  while ((TO_IMPL(self)->len + TO_BYTES(self, data_len)) > GET_CAPACITY(self)) {
    c_error_t err
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (err != C_ERROR_none) return err;
  }

  if (TO_BYTES(self, index) < TO_IMPL(self)->len) {
    memmove((uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index + data_len),
            (uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index),
            TO_IMPL(self)->len - TO_BYTES(self, index));
  }

  memcpy((uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index), data,
         TO_BYTES(self, data_len));

  TO_IMPL(self)->len += TO_BYTES(self, data_len);

  return C_ERROR_none;
}

/// @brief fill the whole @ref CVecImpl::capacity with @p data
/// @param[in] self
/// @param[in] data
void
c_vec_fill(CVec* self, void* data)
{
  assert(self && TO_IMPL(self)->data);
  if (!data) return;

  size_t vec_capacity = TO_UNITS(self, GET_CAPACITY(self));

  for (size_t iii = 0; iii < vec_capacity; iii++) {
    memcpy((char*)TO_IMPL(self)->data + TO_BYTES(self, iii), data,
           TO_IMPL(self)->element_size);
  }
  TO_IMPL(self)->len = TO_BYTES(self, vec_capacity);
}

/// @brief concatenate vec2 @ref CVecImpl::data to vec1 @ref CVecImpl::data
/// @param[in] vec1
/// @param[in] vec2
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_concatenate(CVec* vec1, CVec const* vec2)
{
  assert(vec1 && vec1->data);
  assert(vec2 && vec2->data);

  c_error_t err = C_ERROR_none;

  if (TO_IMPL(vec1)->element_size != TO_IMPL(vec2)->element_size)
    return C_ERROR_wrong_element_size;

  if (GET_CAPACITY(vec1) < (TO_IMPL(vec1)->len + TO_IMPL(vec2)->len)) {
    err = c_vec_set_capacity(
        vec1, TO_UNITS(vec1, TO_IMPL(vec1)->len + TO_IMPL(vec2)->len));
    if (err) return err;
  }

  memcpy((char*)vec1->data + TO_IMPL(vec1)->len, TO_IMPL(vec2)->data,
         TO_IMPL(vec2)->len);

  TO_IMPL(vec1)->len += TO_IMPL(vec2)->len;

  return err;
}

/// @brief fill @ref CVecImpl::data with repeated @p data
/// @param[in] self
/// @param[in] data
/// @param[in] data_len this is in @ref CVecImpl::element_size not bytes
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_fill_with_repeat(CVec* self, void* data, size_t data_len)
{
  assert(self && TO_IMPL(self)->data);
  if (data_len > TO_UNITS(self, GET_CAPACITY(self))) return C_ERROR_wrong_len;

  size_t const number_of_repeats
      = TO_UNITS(self, GET_CAPACITY(self)) / data_len;
  size_t const repeat_size = TO_BYTES(self, data_len);
  for (size_t iii = 0; iii < number_of_repeats; ++iii) {
    memcpy((char*)TO_IMPL(self)->data + (iii * (repeat_size)), data,
           TO_BYTES(self, data_len));
  }

  TO_IMPL(self)->len = TO_BYTES(self, number_of_repeats * data_len);

  return C_ERROR_none;
}

/// @brief replace range start from @p index with @p data
/// @param[in] self
/// @param[in] index
/// @param[in] range_len
/// @param[in] data
/// @param[in] data_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_replace(
    CVec* self, size_t index, size_t range_len, void* data, size_t data_len)
{
  /// TODO:
  assert(false);
  (void)self;
  (void)index;
  (void)range_len;
  (void)data;
  (void)data_len;
}

/// @brief rotate @p elements_count to the right
/// @param[in] self
/// @param[in] elements_count
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_rotate_right(CVec* self, size_t elements_count)
{
  assert(self && TO_IMPL(self)->data);
  if ((elements_count == 0)
      || (TO_BYTES(self, elements_count) > TO_IMPL(self)->len))
    return C_ERROR_none;

  void*     tmp_mem;
  c_error_t err = c_allocator_alloc(
      TO_IMPL(self)->allocator, TO_BYTES(self, elements_count),
      TO_IMPL(self)->element_size, false, &tmp_mem);
  if (err) return err;

  memcpy(tmp_mem,
         (char*)TO_IMPL(self)->data + TO_IMPL(self)->len
             - TO_BYTES(self, elements_count),
         TO_BYTES(self, elements_count));

  memmove((char*)TO_IMPL(self)->data + TO_BYTES(self, elements_count),
          TO_IMPL(self)->data, TO_BYTES(self, elements_count));
  memcpy(TO_IMPL(self)->data, tmp_mem, TO_BYTES(self, elements_count));

  c_allocator_free(TO_IMPL(self)->allocator, &tmp_mem);

  return C_ERROR_none;
}

/// @brief rotate @p elements_count to the left
/// @param[in] self
/// @param[in] elements_count
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_rotate_left(CVec* self, size_t elements_count)
{
  assert(self && TO_IMPL(self)->data);
  if ((elements_count == 0)
      || (TO_BYTES(self, elements_count) > TO_IMPL(self)->len))
    return C_ERROR_none;

  void*     tmp_mem;
  c_error_t err = c_allocator_alloc(
      TO_IMPL(self)->allocator, TO_BYTES(self, elements_count),
      TO_IMPL(self)->element_size, false, &tmp_mem);
  if (err) return err;

  memcpy(tmp_mem, TO_IMPL(self)->data, TO_BYTES(self, elements_count));

  memmove(TO_IMPL(self)->data,
          (char*)TO_IMPL(self)->data + TO_BYTES(self, elements_count),
          TO_BYTES(self, elements_count));
  memcpy((char*)TO_IMPL(self)->data + TO_IMPL(self)->len
             - TO_BYTES(self, elements_count),
         tmp_mem, TO_BYTES(self, elements_count));

  c_allocator_free(TO_IMPL(self)->allocator, &tmp_mem);

  return C_ERROR_none;
}

/// @brief remove element from CVec
/// @note this could reset new reallocated @ref CVecImpl::data
/// @param[in] self
/// @param[in] index index to be removed
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_remove(CVec* self, size_t index)
{
  assert(self && TO_IMPL(self)->data);

  if (TO_BYTES(self, index) <= TO_IMPL(self)->len) return C_ERROR_wrong_index;

  c_error_t err = C_ERROR_none;

  uint8_t* element = (uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index);

  memmove(element, element + TO_IMPL(self)->element_size,
          TO_IMPL(self)->len - TO_BYTES(self, index - 1));
  TO_IMPL(self)->len -= TO_IMPL(self)->element_size;

  if (TO_IMPL(self)->len <= (GET_CAPACITY(self) / 4)) {
    err = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
  }

  return err;
}

/// @brief remove a range of elements from CVec starting from @p start_index
///        with size @p range_len
/// @note this could reset new reallocated @p CVec::data
/// @param[in] self
/// @param[in] start_index
/// @param[in] range_len range length
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_remove_range(CVec* self, size_t start_index, size_t range_len)
{
  assert(self && TO_IMPL(self)->data);

  c_error_t err = C_ERROR_none;

  if (TO_IMPL(self)->len == 0U) return C_ERROR_wrong_len;
  if (start_index > (TO_UNITS(self, TO_IMPL(self)->len) - 1U))
    return C_ERROR_wrong_index;
  if (TO_BYTES(self, start_index + range_len) > TO_IMPL(self)->len)
    return C_ERROR_wrong_len;

  uint8_t* start_ptr
      = (uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, start_index);
  uint8_t const* end_ptr = (uint8_t*)TO_IMPL(self)->data
                           + TO_BYTES(self, (start_index + range_len));
  size_t right_range_len = TO_BYTES(
      self, TO_IMPL(self)->len - TO_BYTES(self, (start_index + range_len)));

  memmove(start_ptr, end_ptr, right_range_len);
  TO_IMPL(self)->len -= TO_BYTES(self, range_len);

  if (TO_IMPL(self)->len <= (GET_CAPACITY(self) / 4)) {
    err = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
  }

  return err;
}

/// @brief remove duplicated elements in place
/// @note this is a very costy function
/// @param[in] self
/// @param[in] cmp this is similar to strcmp
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_deduplicate(CVec* self, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);

  if (!cmp) return C_ERROR_none;

  for (size_t iii = 0; iii < TO_UNITS(self, TO_IMPL(self)->len); ++iii) {
    for (size_t jjj = iii + 1; jjj < TO_UNITS(self, TO_IMPL(self)->len);
         ++jjj) {
      if (cmp((char*)TO_IMPL(self)->data + (TO_IMPL(self)->element_size * iii),
              (char*)TO_IMPL(self)->data + (TO_IMPL(self)->element_size * jjj))
          == 0) {
        memmove((char*)TO_IMPL(self)->data + TO_BYTES(self, jjj),
                (char*)TO_IMPL(self)->data + TO_BYTES(self, jjj + 1),
                TO_IMPL(self)->len - TO_BYTES(self, jjj + 1));
        TO_IMPL(self)->len--;
        jjj--;
      }
    }
  }

  return C_ERROR_none;
}

/// @brief get a slice from @ref CVecImpl
/// @note this is only reference to the data
///       so @ref CVecImpl::capacity will be zero
/// @param[in] self
/// @param[in] start_index
/// @param[in] range if range is bigger than @ref CVecImpl::len,
///                  @ref CVecImpl::len will be the returned range
/// @param[out] out_slice
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_slice(CVec const* self,
            size_t      start_index,
            size_t      range_len,
            CVec**      out_slice)
{
  assert(self && TO_IMPL(self)->data);

  if (TO_BYTES(self, start_index) > TO_IMPL(self)->len)
    return C_ERROR_wrong_index;

  range_len = (TO_UNITS(self, TO_IMPL(self)->len) - start_index) < range_len
                  ? TO_UNITS(self, TO_IMPL(self)->len)
                  : range_len;

  c_error_t err = c_vec_create_from_raw(
      (char*)TO_IMPL(self)->data + TO_BYTES(self, start_index), range_len,
      TO_IMPL(self)->element_size, TO_IMPL(self)->allocator, out_slice);

  return err;
}

/// @brief create an iterator for @ref CVec, check @ref CIter for the definition
///        and @ref iter.h for other functions
/// @param[in] self
/// @param[in] step_callback callback needed internally for other Iter
///                          functions, this could be NULL and it will use
///                          the default step callback
/// @param[out] out_c_iter
void
c_vec_iter_create(CVec*             self,
                  CIterStepCallback step_callback,
                  CIter*            out_c_iter)
{
  assert(self);

  c_iter_create(TO_IMPL(self)->element_size, step_callback, out_c_iter);
}

/// @brief reverse the @ref CVecImpl::data in place
/// @param[in] self
/// @return error (any value but zero is treated as an error)
c_error_t
c_vec_reverse(CVec* self)
{
  assert(self && TO_IMPL(self)->data);

  char* start = TO_IMPL(self)->data;
  char* end   = (char*)TO_IMPL(self)->data
              + (TO_IMPL(self)->len - TO_IMPL(self)->element_size);

  void*     tmp_mem;
  c_error_t err
      = c_allocator_alloc(TO_IMPL(self)->allocator, TO_IMPL(self)->element_size,
                          TO_IMPL(self)->element_size, false, &tmp_mem);
  if (err) return err;

  while (end > start) {
    memcpy(tmp_mem, start, TO_IMPL(self)->element_size);
    memcpy(start, end, TO_IMPL(self)->element_size);
    memcpy(end, tmp_mem, TO_IMPL(self)->element_size);

    start += TO_IMPL(self)->element_size;
    end -= TO_IMPL(self)->element_size;
  }

  c_allocator_free(TO_IMPL(self)->allocator, &tmp_mem);

  return C_ERROR_none;
}

/// @brief clear the @ref CVecImpl without changing the capacity
/// @param[in] self
void
c_vec_clear(CVec* self)
{
  assert(self && TO_IMPL(self)->data);
  TO_IMPL(self)->len = 0;
}

/// @brief destroy an vec object
/// @note this could handle self as NULL
/// @param[in] self
void
c_vec_destroy(CVec** self)
{
  if (self && TO_IMPL(*self)->data) {
    CAllocator* allocator = TO_IMPL(*self)->allocator;
    if (TO_IMPL(*self)->raw_capacity == 0) {
      c_allocator_free(TO_IMPL(*self)->allocator, &TO_IMPL(*self)->data);
    }
    c_allocator_free(allocator, (void**)self);
    *self = NULL;
  }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
