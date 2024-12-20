/**
 * @file anylibs/vec.h
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
#include "internal/vec.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if WIN32 && (!_MSC_VER || !(_MSC_VER >= 1900))
#error "You need MSVC must be higher that or equal to 1900"
#endif

#ifdef MSC_VER
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

/// @brief create a new vec object
/// @param element_size
/// @param allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @return CVec object created or NULL on error
CVec*
c_vec_create(size_t element_size, CAllocator* allocator)
{
  return c_vec_create_with_capacity(element_size, 1U, false, allocator);
}

/// @brief same as @ref c_vec_create but with allocating capacity
/// @param element_size
/// @param capacity maximum number of elements to be allocated, minimum
///                     capacity is 1
/// @param zero_initialized should zero the memory or not
/// @param allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @return CVec object created or NULL on error
CVec*
c_vec_create_with_capacity(size_t      element_size,
                           size_t      capacity,
                           bool        zero_initialized,
                           CAllocator* allocator)
{
  assert(element_size > 0);

  if (!allocator) allocator = c_allocator_default();

  CVecImpl* impl = c_allocator_alloc(
      allocator, c_allocator_alignas(CVecImpl, 1), zero_initialized);
  void* data_mem = c_allocator_alloc(allocator, capacity * element_size,
                                     element_size, zero_initialized);
  if (!impl || !data_mem) goto ERROR_ALLOC;

  impl->data         = data_mem;
  impl->element_size = element_size;
  impl->len          = 0;
  impl->allocator    = allocator;
  impl->raw_capacity = 0;

  return FROM_IMPL(impl);

ERROR_ALLOC:
  c_allocator_free(allocator, impl);
  c_allocator_free(allocator, data_mem);
  return NULL;
}

/// @brief create @ref CVecImpl from raw pointer
/// @param data
/// @param data_len this is in @ref CVecImpl::element_size not bytes
/// @param element_size
/// @param should_copy true: copy data, false: do not copy data (just ref)
/// @param allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @return CVec object created or NULL on error
CVec*
c_vec_create_from_raw(void*       data,
                      size_t      data_len,
                      size_t      element_size,
                      bool        should_copy,
                      CAllocator* allocator)
{
  assert(element_size > 0);
  assert(data && data_len > 0);

  if (!allocator) allocator = c_allocator_default();

  CVecImpl* impl;

  if (!should_copy) {
    impl
        = c_allocator_alloc(allocator, c_allocator_alignas(CVecImpl, 1), false);
    if (!impl) goto ERROR_ALLOC;

    impl->data         = data;
    impl->element_size = element_size;
    impl->len          = data_len;
    impl->allocator    = allocator;
    impl->raw_capacity = data_len;
  } else {
    impl = (CVecImpl*)c_vec_create_with_capacity(element_size, data_len, false,
                                                 allocator);
    if (!impl) return NULL;

    impl->len = TO_BYTES(impl, data_len);
    memcpy(impl->data, data, TO_IMPL(impl)->len);
  }

  return FROM_IMPL(impl);
ERROR_ALLOC:
  c_allocator_free(allocator, impl);
  return NULL;
}

/// @brief clone @ref CVecImpl
/// @note this will make a deep copy
/// @param self
/// @param should_shrink_clone
/// @return CVec object cloned or NULL on error
CVec*
c_vec_clone(CVec const* self, bool should_shrink_clone)
{
  assert(self && TO_IMPL(self)->data);

  CVec* cloned_vec = c_vec_create_with_capacity(
      TO_IMPL(self)->element_size,
      should_shrink_clone ? TO_UNITS(self, TO_IMPL(self)->len)
                          : TO_UNITS(self, GET_CAPACITY(self)),
      false, TO_IMPL(self)->allocator);
  if (!cloned_vec) return NULL;

  memcpy(TO_IMPL(cloned_vec)->data, TO_IMPL(self)->data, TO_IMPL(self)->len);
  TO_IMPL(cloned_vec)->len = TO_IMPL(self)->len;

  return cloned_vec;
}

/// @brief check wether the vec is empty
/// @param self
/// @return bool
bool
c_vec_is_empty(CVec const* self)
{
  assert(self);
  return TO_IMPL(self)->len > 0;
}

/// @brief get @ref CVecImpl::len
/// @param self
/// @return len
size_t
c_vec_len(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, TO_IMPL(self)->len);
}

/// @brief set vec length
///        this is useful if you want
///        to manipulate the data by yourself
/// @note this could reallocate @ref CVec::data
/// @param self
/// @param new_len
void
c_vec_set_len(CVec* self, size_t new_len)
{
  assert(self && TO_IMPL(self)->data);
  assert(new_len > 0);

  if (TO_BYTES(self, new_len) <= GET_CAPACITY(self)) {
    TO_IMPL(self)->len = TO_BYTES(self, new_len);
  }
}

/// @brief get @p self capacity
///        this will return the capacity in @ref CVecImpl::element_size wise
///        return 'capacity = 10' which means
///        we can have up to '10 * element_size' bytes
/// @param self
/// @return capacity
size_t
c_vec_capacity(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, GET_CAPACITY(self));
}

/// @brief return the remaining empty space
/// @param self
/// @return spare_capacity
size_t
c_vec_spare_capacity(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, GET_CAPACITY(self) - TO_IMPL(self)->len);
}

/// @brief set capacity
/// @note this could reallocate @ref CVec::data
/// @param self address of self
/// @param new_capacity
/// @return true: success, false: error happened
bool
c_vec_set_capacity(CVec* self, size_t new_capacity)
{
  assert(self && TO_IMPL(self)->data);
  assert(new_capacity > 0);

  if (TO_IMPL(self)->raw_capacity > 0) {
    if (new_capacity <= TO_IMPL(self)->raw_capacity) {
      TO_IMPL(self)->raw_capacity = new_capacity;
      return true;
    } else {
      return false;
    }
  }

  size_t new_len = (TO_BYTES(self, new_capacity) < TO_IMPL(self)->len)
                       ? TO_BYTES(self, new_capacity)
                       : TO_IMPL(self)->len;

  CAllocatorResizeResult result
      = c_allocator_resize(TO_IMPL(self)->allocator, TO_IMPL(self)->data,
                           TO_BYTES(self, new_capacity));

  if (result.is_ok) {
    TO_IMPL(self)->data = result.memory;
    TO_IMPL(self)->len  = new_len;
    return true;
  } else {
    return false;
  }
}

/// @brief get @ref CVecImpl::element_size in bytes
/// @param self
/// @return element_size
size_t
c_vec_element_size(CVec* self)
{
  return TO_IMPL(self)->element_size;
}

/// @brief make the capacity equals @ref CVecImpl::len
/// @param self
/// @return true: success, false: error happened
bool
c_vec_shrink_to_fit(CVec* self)
{
  return c_vec_set_capacity(self, TO_UNITS(self, TO_IMPL(self)->len));
}

/// @brief search for @p element
/// @param self
/// @param element
/// @param cmp this is similar to strcmp
/// @return is_ok[true]: return the found element
///         is_ok[false]: not found
CVecFindResult
c_vec_find(CVec const* self, void* element, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);

  CVecFindResult result = {0};
  if (!cmp) return result;

  for (size_t iii = 0; iii < TO_UNITS(self, TO_IMPL(self)->len); iii++) {
    if (cmp(element, (char*)TO_IMPL(self)->data + TO_BYTES(self, iii)) == 0) {
      result.element = (char*)TO_IMPL(self)->data + TO_BYTES(self, iii);
      result.is_ok   = true;
      return result;
    }
  }

  return result;
}

/// @brief search for @p element using binary search tree
/// @note  If @ref CVec::data is not sorted, the returned result is
///        unspecified and meaningless
/// @param self
/// @param element
/// @param cmp this is similar to strcmp
/// @return is_ok[true]: return the found element
///         is_ok[false]: not found
CVecFindResult
c_vec_binary_find(CVec const* self,
                  void const* element,
                  int         cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);

  CVecFindResult result = {0};
  if (!cmp) return result;

  void* out_element = bsearch(element, TO_IMPL(self)->data,
                              TO_UNITS(self, TO_IMPL(self)->len),
                              TO_IMPL(self)->element_size, cmp);

  if (out_element) {
    result.element = out_element;
    result.is_ok   = true;
  }

  return result;
}

/// @brief check if @p elements is the same as the start elements
///        of @ref CVec::data
/// @param self
/// @param elements
/// @param elements_len
/// @param cmp this is similar to strcmp
/// @return bool
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
///        of @ref CVec::data
/// @param self
/// @param elements
/// @param elements_len
/// @param cmp this is similar to strcmp
/// @return bool
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
/// @param self
/// @param cmp this is similar to strcmp
void
c_vec_sort(CVec* self, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);
  if (!cmp) return;

  qsort(TO_IMPL(self)->data, TO_UNITS(self, TO_IMPL(self)->len),
        TO_IMPL(self)->element_size, cmp);
}

/// @brief check if sorted or not (in ascending order)
/// @param self
/// @param cmp this is similar to strcmp
/// @return return if sorted or not
bool
c_vec_is_sorted(CVec* self, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);

  if (!cmp) return false;

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
/// @param self
/// @param cmp this is similar to strcmp
/// @return return if sorted or not
bool
c_vec_is_sorted_inv(CVec* self, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);

  if (!cmp) return false;

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
/// @param self
/// @param index
/// @return is_ok[true]: return the element
///         is_ok[false]: not found
CVecElementResult
c_vec_get(CVec const* self, size_t index)
{
  assert(self && TO_IMPL(self)->data);

  CVecElementResult result = {0};
  if (TO_BYTES(self, index) > TO_IMPL(self)->len) return result;

  result.element = (char*)TO_IMPL(self)->data + TO_BYTES(self, index);
  result.is_ok   = true;
  return result;
}

/// @brief push one element at the end
///        if you want to push literals (example: 3, 5 or 10 ...)
///        c_vec_push(vec, &(int){3});
/// @note this could reallocate @ref CVec::data
/// @note this will COPY @p element
/// @param self pointer to self
/// @param element a pointer the data of size @ref CVecImpl::element_size
///                    that you want to push back
/// @return true: success, false: error happened
bool
c_vec_push(CVec* self, void const* element)
{
  assert(self && TO_IMPL(self)->data);
  assert(element);

  if (TO_IMPL(self)->len >= GET_CAPACITY(self)) {
    bool resized
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (!resized) return false;
  }

  memcpy((uint8_t*)TO_IMPL(self)->data + TO_IMPL(self)->len, element,
         TO_IMPL(self)->element_size);
  TO_IMPL(self)->len += TO_IMPL(self)->element_size;

  return true;
}

/// @brief push elements at the end of @ref CVecImpl
/// @note this could reallocate @ref CVec::data
/// @note this will COPY @p element
/// @param self
/// @param elements
/// @param elements_len
/// @return true: success, false: error happened
bool
c_vec_push_range(CVec* self, void const* elements, size_t elements_len)
{
  return c_vec_insert_range(self, TO_UNITS(self, TO_IMPL(self)->len), elements,
                            elements_len);
}

/// @brief pop one element from the end
/// @note this could reallocate @ref CVec::data
/// @param self
/// @return is_ok[true]: return the element
///         is_ok[false]: not found
CVecElementResult
c_vec_pop(CVec* self)
{
  assert(self && TO_IMPL(self)->data);

  CVecElementResult result = {0};
  if (TO_IMPL(self)->len == 0) return result;

  // memcpy(out_element,
  //        (uint8_t*)TO_IMPL(self)->data + TO_IMPL(self)->len
  //            - TO_IMPL(self)->element_size,
  //        TO_IMPL(self)->element_size);
  result.element = (uint8_t*)TO_IMPL(self)->data + TO_IMPL(self)->len
                   - TO_IMPL(self)->element_size;
  TO_IMPL(self)->len -= TO_IMPL(self)->element_size;

  if (TO_IMPL(self)->len <= (GET_CAPACITY(self) / 4)) {
    bool resized
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
    if (!resized) return result;
  }

  result.is_ok = true;
  return result;
}

/// @brief insert 1 element at @p index
/// @note this could reallocate @ref CVec::data
/// @param self pointer to self
/// @param index
/// @param element a pointer the data of size @ref CVecImpl::element_size
///                    that you want to push back
/// @return true: success, false: error happened
bool
c_vec_insert(CVec* self, size_t index, void const* element)
{
  assert(self && TO_IMPL(self)->data);

  if (TO_IMPL(self)->len <= TO_BYTES(self, index)) return false;

  if (TO_IMPL(self)->len == GET_CAPACITY(self)) {
    bool resized
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (!resized) return false;
  }

  if (index < TO_UNITS(self, TO_IMPL(self)->len)) {
    memmove((uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index + 1),
            (uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index),
            TO_IMPL(self)->len - TO_BYTES(self, index));
  }

  memcpy((uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index), element,
         TO_IMPL(self)->element_size);
  TO_IMPL(self)->len += TO_IMPL(self)->element_size;

  return true;
}

/// @brief insert multiple elements at index
/// @note this could reallocate @ref CVec::data
/// @param self
/// @param index
/// @param data
/// @param data_len
/// @return true: success, false: error happened
bool
c_vec_insert_range(CVec* self, size_t index, void const* data, size_t data_len)
{
  assert(self && TO_IMPL(self)->data);
  assert(data);
  assert(data_len > 0);

  if (TO_IMPL(self)->len < TO_BYTES(self, index)) return false;

  while ((TO_IMPL(self)->len + TO_BYTES(self, data_len)) > GET_CAPACITY(self)) {
    bool resized
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (!resized) return false;
  }

  if (TO_BYTES(self, index) < TO_IMPL(self)->len) {
    memmove((uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index + data_len),
            (uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index),
            TO_IMPL(self)->len - TO_BYTES(self, index));
  }

  memcpy((uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index), data,
         TO_BYTES(self, data_len));

  TO_IMPL(self)->len += TO_BYTES(self, data_len);

  return true;
}

/// @brief fill the whole @ref CVecImpl::data with @p data
/// @param self
/// @param data
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

/// @brief concatenate vec2 @ref CVec::data to vec1 @ref CVec::data
/// @param vec1
/// @param vec2
/// @return true: success, false: error happened
bool
c_vec_concatenate(CVec* vec1, CVec const* vec2)
{
  assert(vec1 && vec1->data);
  assert(vec2 && vec2->data);

  if (TO_IMPL(vec1)->element_size != TO_IMPL(vec2)->element_size) return false;

  if (GET_CAPACITY(vec1) < (TO_IMPL(vec1)->len + TO_IMPL(vec2)->len)) {
    bool resized = c_vec_set_capacity(
        vec1, TO_UNITS(vec1, TO_IMPL(vec1)->len + TO_IMPL(vec2)->len));
    if (!resized) return false;
  }

  memcpy((char*)vec1->data + TO_IMPL(vec1)->len, TO_IMPL(vec2)->data,
         TO_IMPL(vec2)->len);

  TO_IMPL(vec1)->len += TO_IMPL(vec2)->len;

  return true;
}

/// @brief fill @ref CVec::data with repeated @p data
/// @param self
/// @param data
/// @param data_len this is in @ref CVecImpl::element_size not bytes
/// @return true: success, false: error happened
bool
c_vec_fill_with_repeat(CVec* self, void* data, size_t data_len)
{
  assert(self && TO_IMPL(self)->data);
  if (data_len > TO_UNITS(self, GET_CAPACITY(self))) return false;

  size_t const number_of_repeats
      = TO_UNITS(self, GET_CAPACITY(self)) / data_len;
  size_t const repeat_size = TO_BYTES(self, data_len);
  for (size_t iii = 0; iii < number_of_repeats; ++iii) {
    memcpy((char*)TO_IMPL(self)->data + (iii * (repeat_size)), data,
           TO_BYTES(self, data_len));
  }

  TO_IMPL(self)->len = TO_BYTES(self, number_of_repeats * data_len);

  return true;
}

/// @brief replace range start from @p index with @p data
/// @param self
/// @param index
/// @param range_len
/// @param data
/// @param data_len
/// @return true: success, false: error happened
bool
c_vec_replace(
    CVec* self, size_t index, size_t range_len, void* data, size_t data_len)
{
  assert(self && self->data);

  if (range_len == 0 || !data || data_len == 0) return false;

  size_t len_as_units = TO_UNITS(self, TO_IMPL(self)->len);
  size_t cap_as_units
      = TO_UNITS(self, c_allocator_mem_size(TO_IMPL(self)->data));

  if ((index + range_len) >= len_as_units) range_len = len_as_units - index;

  /// enlarge if needed
  if ((len_as_units - range_len + data_len + 1) > cap_as_units) {
    cap_as_units -= range_len - data_len;
    bool resized = c_vec_set_capacity(self, cap_as_units);
    if (!resized) return false;
  }

  if (data_len < range_len || data_len > range_len) {
    memmove((uint8_t*)(self->data) + TO_BYTES(self, index + data_len),
            (uint8_t*)(self->data) + TO_BYTES(self, index + range_len),
            TO_BYTES(self, len_as_units - (range_len + index)));
    len_as_units -= range_len;
    len_as_units += data_len;
    TO_IMPL(self)->len = TO_BYTES(self, len_as_units);

    /// shrink if needed
    if ((len_as_units > 0) && (len_as_units <= cap_as_units / 4)) {
      bool resized = c_vec_set_capacity(self, cap_as_units / 2);
      if (!resized) return false;
    }
  }

  memcpy((uint8_t*)self->data + TO_BYTES(self, index), data,
         TO_BYTES(self, data_len));

  return true;
}

/// @brief rotate @p elements_count to the right
/// @param self
/// @param elements_count
/// @return true: success, false: error happened
bool
c_vec_rotate_right(CVec* self, size_t elements_count)
{
  assert(self && TO_IMPL(self)->data);
  if ((elements_count == 0)
      || (TO_BYTES(self, elements_count) > TO_IMPL(self)->len))
    return false;

  void* tmp_mem = c_allocator_alloc(TO_IMPL(self)->allocator,
                                    TO_BYTES(self, elements_count),
                                    TO_IMPL(self)->element_size, false);
  if (!tmp_mem) return false;

  memcpy(tmp_mem,
         (char*)TO_IMPL(self)->data + TO_IMPL(self)->len
             - TO_BYTES(self, elements_count),
         TO_BYTES(self, elements_count));

  memmove((char*)TO_IMPL(self)->data + TO_BYTES(self, elements_count),
          TO_IMPL(self)->data, TO_BYTES(self, elements_count));
  memcpy(TO_IMPL(self)->data, tmp_mem, TO_BYTES(self, elements_count));

  c_allocator_free(TO_IMPL(self)->allocator, tmp_mem);

  return true;
}

/// @brief rotate @p elements_count to the left
/// @param self
/// @param elements_count
/// @return true: success, false: error happened
bool
c_vec_rotate_left(CVec* self, size_t elements_count)
{
  assert(self && TO_IMPL(self)->data);
  if ((elements_count == 0)
      || (TO_BYTES(self, elements_count) > TO_IMPL(self)->len))
    return false;

  void* tmp_mem = c_allocator_alloc(TO_IMPL(self)->allocator,
                                    TO_BYTES(self, elements_count),
                                    TO_IMPL(self)->element_size, false);
  if (!tmp_mem) return false;

  memcpy(tmp_mem, TO_IMPL(self)->data, TO_BYTES(self, elements_count));

  memmove(TO_IMPL(self)->data,
          (char*)TO_IMPL(self)->data + TO_BYTES(self, elements_count),
          TO_BYTES(self, elements_count));
  memcpy((char*)TO_IMPL(self)->data + TO_IMPL(self)->len
             - TO_BYTES(self, elements_count),
         tmp_mem, TO_BYTES(self, elements_count));

  c_allocator_free(TO_IMPL(self)->allocator, tmp_mem);

  return true;
}

/// @brief remove element from CVec
/// @note this could reallocate @ref CVec::data
/// @param self
/// @param index index to be removed
/// @return true: success, false: error happened
bool
c_vec_remove(CVec* self, size_t index)
{
  assert(self && TO_IMPL(self)->data);

  if (TO_BYTES(self, index) <= TO_IMPL(self)->len) return false;

  uint8_t* element = (uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, index);

  memmove(element, element + TO_IMPL(self)->element_size,
          TO_IMPL(self)->len - TO_BYTES(self, index - 1));
  TO_IMPL(self)->len -= TO_IMPL(self)->element_size;

  if (TO_IMPL(self)->len <= (GET_CAPACITY(self) / 4)) {
    bool resized
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
    if (!resized) return false;
  }

  return true;
}

/// @brief remove a range of elements from CVec starting from @p start_index
///        with size @p range_len
/// @note this could reset new reallocated @p CVec::data
/// @param self
/// @param start_index
/// @param range_len range length
/// @return true: success, false: error happened
bool
c_vec_remove_range(CVec* self, size_t start_index, size_t range_len)
{
  assert(self && TO_IMPL(self)->data);

  if (TO_IMPL(self)->len == 0U) return false;
  if (start_index > (TO_UNITS(self, TO_IMPL(self)->len) - 1U)) return false;
  if (TO_BYTES(self, start_index + range_len) > TO_IMPL(self)->len)
    return false;

  uint8_t* start_ptr
      = (uint8_t*)TO_IMPL(self)->data + TO_BYTES(self, start_index);
  uint8_t const* end_ptr = (uint8_t*)TO_IMPL(self)->data
                           + TO_BYTES(self, (start_index + range_len));
  size_t right_range_len = TO_BYTES(
      self, TO_IMPL(self)->len - TO_BYTES(self, (start_index + range_len)));

  memmove(start_ptr, end_ptr, right_range_len);
  TO_IMPL(self)->len -= TO_BYTES(self, range_len);

  if (TO_IMPL(self)->len <= (GET_CAPACITY(self) / 4)) {
    bool resized
        = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
    if (!resized) return false;
  }

  return true;
}

/// @brief remove duplicated elements in place
/// @note this is a very costy function
/// @param self
/// @param cmp this is similar to strcmp
/// @return true: success, false: error happened
bool
c_vec_deduplicate(CVec* self, int cmp(void const*, void const*))
{
  assert(self && TO_IMPL(self)->data);

  if (!cmp) return false;

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

  return true;
}

/// @brief get a slice from @ref CVecImpl
/// @note this is only reference to the data
///       so capacity will be zero
/// @param self
/// @param start_index
/// @param range_len if range is bigger than @ref CVecImpl::len,
///                  @ref CVecImpl::len will be the returned range
/// @return slice or NULL on error
CVec*
c_vec_slice(CVec const* self, size_t start_index, size_t range_len)
{
  assert(self && TO_IMPL(self)->data);

  CVec* slice = NULL;

  if (TO_BYTES(self, start_index) > TO_IMPL(self)->len) return slice;

  range_len = (TO_UNITS(self, TO_IMPL(self)->len) - start_index) < range_len
                  ? TO_UNITS(self, TO_IMPL(self)->len)
                  : range_len;

  slice = c_vec_create_from_raw(
      (char*)TO_IMPL(self)->data + TO_BYTES(self, start_index), range_len,
      TO_IMPL(self)->element_size, false, TO_IMPL(self)->allocator);

  return slice;
}

/// @brief create an iterator for @ref CVec, check @ref CIter for the definition
///        and @ref iter.h for other functions
/// @param self
/// @param step_callback callback needed internally for other Iter
///                          functions, this could be NULL and it will use
///                          the default step callback
/// @return c_iter
CIter
c_vec_iter(CVec* self, CIterStepCallback step_callback)
{
  assert(self);

  CIter iter = c_iter(TO_IMPL(self)->element_size, step_callback);
  return iter;
}

/// @brief reverse the @ref CVec::data in place
/// @param self
/// @return true: success, false: error happened
bool
c_vec_reverse(CVec* self)
{
  assert(self && TO_IMPL(self)->data);

  char* start = TO_IMPL(self)->data;
  char* end   = (char*)TO_IMPL(self)->data
              + (TO_IMPL(self)->len - TO_IMPL(self)->element_size);

  void* tmp_mem
      = c_allocator_alloc(TO_IMPL(self)->allocator, TO_IMPL(self)->element_size,
                          TO_IMPL(self)->element_size, false);
  if (!tmp_mem) return false;

  while (end > start) {
    memcpy(tmp_mem, start, TO_IMPL(self)->element_size);
    memcpy(start, end, TO_IMPL(self)->element_size);
    memcpy(end, tmp_mem, TO_IMPL(self)->element_size);

    start += TO_IMPL(self)->element_size;
    end -= TO_IMPL(self)->element_size;
  }

  c_allocator_free(TO_IMPL(self)->allocator, tmp_mem);

  return true;
}

/// @brief clear the @ref CVecImpl without changing the capacity
/// @param self
void
c_vec_clear(CVec* self)
{
  assert(self && TO_IMPL(self)->data);
  TO_IMPL(self)->len = 0;
}

/// @brief convert to string
/// @param self
/// @return @ref CString obj or NULL on error
CString*
c_cvec_to_cstring(CVec* self)
{
  assert(self);

  return (CString*)self;
}

/// @brief destroy an vec object
/// @note this could handle self as NULL
/// @param self
void
c_vec_destroy(CVec* self)
{
  if (self && TO_IMPL(self)->data) {
    CAllocator* allocator = TO_IMPL(self)->allocator;
    if (TO_IMPL(self)->raw_capacity == 0) {
      c_allocator_free(allocator, TO_IMPL(self)->data);
    }
    c_allocator_free(allocator, self);
  }
}

#ifdef MSC_VER
#pragma warning(pop)
#endif
