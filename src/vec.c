#include "anylibs/vec.h"
#include "anylibs/error.h"
#include "internal/vec.h"

#include <assert.h>
#include <stdbool.h>
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
#define GET_CAPACITY(vec)                                      \
  (TO_IMPL(vec)->raw_capacity > 0 ? TO_IMPL(vec)->raw_capacity \
                                  : c_allocator_mem_size(TO_IMPL(vec)->data))
#define c_vec_should_shrink(vec) (TO_IMPL(vec)->len <= (GET_CAPACITY(vec) / 4))

CVec* c_vec_create(size_t element_size, CAllocator* allocator)
{
  return c_vec_create_with_capacity(element_size, 1U, false, allocator);
}

CVec* c_vec_create_with_capacity(size_t element_size, size_t capacity,
                                 bool set_mem_to_zero, CAllocator* allocator)
{
  assert(element_size > 0);

  if (!allocator) allocator = c_allocator_default();

  CVecImpl* impl     = c_allocator_alloc(allocator, c_allocator_alignas(CVecImpl, 1), set_mem_to_zero);
  void*     data_mem = c_allocator_alloc(allocator, capacity * element_size, element_size, set_mem_to_zero);
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

CVec* c_vec_create_from_raw(void* data, size_t data_len, size_t element_size, bool should_copy, CAllocator* allocator)
{
  assert(element_size > 0);
  assert(data && data_len > 0);

  if (!allocator) allocator = c_allocator_default();

  CVecImpl* impl;
  if (!should_copy) {
    impl = c_allocator_alloc(allocator, c_allocator_alignas(CVecImpl, 1), false);
    if (!impl) goto ERROR_ALLOC;

    impl->data         = data;
    impl->element_size = element_size;
    impl->len          = data_len;
    impl->allocator    = allocator;
    impl->raw_capacity = data_len;
  } else {
    impl = (CVecImpl*)c_vec_create_with_capacity(element_size, data_len, false, allocator);
    if (!impl) goto ERROR_ALLOC;

    impl->len = TO_BYTES(impl, data_len);
    memcpy(impl->data, data, impl->len);
  }

  return FROM_IMPL(impl);
ERROR_ALLOC:
  c_error_set(C_ERROR_mem_allocation);
  c_vec_destroy(FROM_IMPL(impl));
  return NULL;
}

CVec* c_vec_clone(CVec const* self, bool should_shrink_clone)
{
  assert(self && self->data);

  CVecImpl* cloned_vec = (CVecImpl*)c_vec_create_with_capacity(TO_IMPL(self)->element_size, should_shrink_clone ? TO_UNITS(self, TO_IMPL(self)->len) : TO_UNITS(self, GET_CAPACITY(self)), false, TO_IMPL(self)->allocator);
  if (!cloned_vec) return NULL;

  memcpy(cloned_vec->data, self->data, TO_IMPL(self)->len);
  cloned_vec->len = TO_IMPL(self)->len;

  return FROM_IMPL(cloned_vec);
}

bool c_vec_is_empty(CVec const* self)
{
  assert(self);
  return TO_IMPL(self)->len > 0;
}

size_t c_vec_len(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, TO_IMPL(self)->len);
}

bool c_vec_set_len(CVec* self, size_t new_len)
{
  assert(self && self->data);
  assert(new_len > 0);

  if (TO_BYTES(self, new_len) <= GET_CAPACITY(self)) {
    TO_IMPL(self)->len = TO_BYTES(self, new_len);
    return true;
  } else {
    c_error_set(C_ERROR_invalid_len);
    return false;
  }
}

size_t c_vec_capacity(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, GET_CAPACITY(self));
}

size_t c_vec_spare_capacity(CVec const* self)
{
  assert(self);
  return TO_UNITS(self, GET_CAPACITY(self) - TO_IMPL(self)->len);
}

bool c_vec_set_capacity(CVec* self, size_t new_capacity)
{
  assert(self && self->data);

  if (TO_IMPL(self)->raw_capacity > 0) {
    if (new_capacity <= TO_IMPL(self)->raw_capacity) {
      TO_IMPL(self)->raw_capacity = new_capacity;
      return true;
    } else {
      c_error_set(C_ERROR_raw_data);
      return false;
    }
  }

  size_t new_len = (TO_BYTES(self, new_capacity) < TO_IMPL(self)->len)
                       ? TO_BYTES(self, new_capacity)
                       : TO_IMPL(self)->len;

  self->data = c_allocator_resize(TO_IMPL(self)->allocator, self->data, TO_BYTES(self, new_capacity));

  if (self->data) {
    TO_IMPL(self)->len = new_len;
    return true;
  } else {
    return false;
  }
}

size_t c_vec_element_size(CVec* self)
{
  return TO_IMPL(self)->element_size;
}

bool c_vec_shrink_to_fit(CVec* self)
{
  return c_vec_set_capacity(self, TO_UNITS(self, TO_IMPL(self)->len));
}

bool c_vec_find(CVec const* self, void* element, CVecCompareFn cmp,
                void** out_data)
{
  assert(self && self->data);

  if (!cmp) {
    c_error_set(C_ERROR_invalid_compare_fn);
    return false;
  }

  for (size_t iii = 0; iii < TO_UNITS(self, TO_IMPL(self)->len); iii++) {
    if (cmp(element, (uint8_t*)self->data + TO_BYTES(self, iii)) == 0) {
      void* elm = (uint8_t*)self->data + TO_BYTES(self, iii);
      if (out_data) *out_data = elm;
      return true;
    }
  }

  c_error_set(C_ERROR_not_found);
  return false;
}

bool c_vec_binary_find(CVec const* self, void const* element, CVecCompareFn cmp, void** out_data)
{
  assert(self && self->data);

  if (!cmp) {
    c_error_set(C_ERROR_invalid_compare_fn);
    return false;
  }

  void* out_element = bsearch(element, self->data, TO_UNITS(self, TO_IMPL(self)->len), TO_IMPL(self)->element_size, cmp);
  if (!out_element) {
    c_error_set(C_ERROR_not_found);
    return false;
  }

  if (out_data) *out_data = out_element;
  return true;
}

int c_vec_starts_with(CVec const* self, void const* data, size_t data_len, CVecCompareFn cmp)
{
  assert(self && self->data);
  assert(data && data_len > 0);

  // validate
  if (!cmp) {
    c_error_set(C_ERROR_invalid_compare_fn);
    return -1;
  }
  if (TO_IMPL(self)->len < TO_BYTES(self, data_len)) {
    c_error_set(C_ERROR_invalid_len);
    return -1;
  }

  for (size_t iii = 0; iii < data_len; iii++) {
    if (cmp((uint8_t*)self->data + TO_BYTES(self, iii),
            (uint8_t*)data + TO_BYTES(self, iii)) != 0) {
      return 1;
    }
  }

  return 0;
}

int c_vec_ends_with(CVec const* self, void const* data, size_t data_len, CVecCompareFn cmp)
{
  assert(self && self->data);
  assert(data && data_len > 0);

  // validate
  if (!cmp) {
    c_error_set(C_ERROR_invalid_compare_fn);
    return -1;
  }
  if (TO_IMPL(self)->len < TO_BYTES(self, data_len)) {
    c_error_set(C_ERROR_invalid_len);
    return -1;
  }

  for (size_t iii = TO_UNITS(self, TO_IMPL(self)->len) - data_len, jjj = 0;
       iii < TO_UNITS(self, TO_IMPL(self)->len); iii++, jjj++) {
    if (cmp((uint8_t*)self->data + TO_BYTES(self, iii),
            (uint8_t*)data + TO_BYTES(self, jjj)) != 0) {
      return 1;
    }
  }

  return 0;
}

bool c_vec_sort(CVec* self, CVecCompareFn cmp)
{
  assert(self && self->data);

  if (!cmp) {
    c_error_set(C_ERROR_invalid_compare_fn);
    return false;
  }

  qsort(self->data, TO_UNITS(self, TO_IMPL(self)->len), TO_IMPL(self)->element_size, cmp);
  return true;
}

int c_vec_is_sorted(CVec* self, CVecCompareFn cmp)
{
  assert(self && self->data);

  if (!cmp) {
    c_error_set(C_ERROR_invalid_compare_fn);
    return -1;
  }

  for (size_t iii = 1; iii < TO_UNITS(self, TO_IMPL(self)->len); ++iii) {
    if (cmp((uint8_t*)self->data + TO_BYTES(self, iii),
            (uint8_t*)self->data + TO_BYTES(self, iii - 1)) < 0) {
      return 1;
    }
  }

  return 0;
}

bool c_vec_get(CVec const* self, size_t index, void** out_data)
{
  assert(self && self->data);

  if (TO_BYTES(self, index) > TO_IMPL(self)->len) {
    c_error_set(C_ERROR_invalid_index);
    return false;
  }

  if (out_data) *out_data = (uint8_t*)self->data + TO_BYTES(self, index);
  return true;
}

bool c_vec_push(CVec* self, void const* element)
{
  assert(self && self->data);
  assert(element);

  if (TO_IMPL(self)->len >= GET_CAPACITY(self)) {
    bool resized = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (!resized) return resized;
  }

  memcpy((uint8_t*)self->data + TO_IMPL(self)->len, element, TO_IMPL(self)->element_size);
  TO_IMPL(self)->len += TO_IMPL(self)->element_size;

  return true;
}

bool c_vec_push_range(CVec* self, void const* elements, size_t elements_len)
{
  return c_vec_insert_range(self, TO_UNITS(self, TO_IMPL(self)->len), elements, elements_len);
}

bool c_vec_pop(CVec* self, void* out_element)
{
  assert(self && self->data);

  if (TO_IMPL(self)->len == 0) {
    c_error_set(C_ERROR_empty);
    return false;
  }

  void* element_ptr = (uint8_t*)self->data + TO_IMPL(self)->len - TO_IMPL(self)->element_size;
  TO_IMPL(self)->len -= TO_IMPL(self)->element_size;

  if (out_element) memcpy(out_element, element_ptr, TO_IMPL(self)->element_size);

  if (c_vec_should_shrink(self)) {
    bool resized = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
    if (!resized) return resized;
  }

  return true;
}

bool c_vec_insert(CVec* self, size_t index, void const* element)
{
  assert(self && self->data);

  if (TO_IMPL(self)->len <= TO_BYTES(self, index)) {
    c_error_set(C_ERROR_invalid_index);
    return false;
  }

  if (TO_IMPL(self)->len == GET_CAPACITY(self)) {
    bool resized = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (!resized) return resized;
  }

  if (index < TO_UNITS(self, TO_IMPL(self)->len)) {
    memmove((uint8_t*)self->data + TO_BYTES(self, index + 1),
            (uint8_t*)self->data + TO_BYTES(self, index),
            TO_IMPL(self)->len - TO_BYTES(self, index));
  }

  memcpy((uint8_t*)self->data + TO_BYTES(self, index), element, TO_IMPL(self)->element_size);
  TO_IMPL(self)->len += TO_IMPL(self)->element_size;

  return true;
}

bool c_vec_insert_range(CVec* self, size_t index, void const* data,
                        size_t data_len)
{
  assert(self && self->data);
  assert(data);
  assert(data_len > 0);

  if (TO_IMPL(self)->len < TO_BYTES(self, index)) {
    c_error_set(C_ERROR_invalid_index);
    return false;
  }

  while ((TO_IMPL(self)->len + TO_BYTES(self, data_len)) > GET_CAPACITY(self)) {
    bool resized = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) * 2);
    if (!resized) return resized;
  }

  if (TO_BYTES(self, index) < TO_IMPL(self)->len) {
    memmove((uint8_t*)self->data + TO_BYTES(self, index + data_len),
            (uint8_t*)self->data + TO_BYTES(self, index),
            TO_IMPL(self)->len - TO_BYTES(self, index));
  }

  memcpy((uint8_t*)self->data + TO_BYTES(self, index), data, TO_BYTES(self, data_len));

  TO_IMPL(self)->len += TO_BYTES(self, data_len);

  return true;
}

void c_vec_fill(CVec* self, void* data)
{
  assert(self && self->data);
  if (!data) return;

  size_t vec_capacity = TO_UNITS(self, GET_CAPACITY(self));

  for (size_t iii = 0; iii < vec_capacity; iii++) {
    memcpy((uint8_t*)self->data + TO_BYTES(self, iii), data, TO_IMPL(self)->element_size);
  }
  TO_IMPL(self)->len = TO_BYTES(self, vec_capacity);
}

bool c_vec_concatenate(CVec* vec1, CVec const* vec2)
{
  assert(vec1 && vec1->data);
  assert(vec2 && vec2->data);

  if (TO_IMPL(vec1)->element_size != TO_IMPL(vec2)->element_size) {
    c_error_set(C_ERROR_invalid_element_size);
    return false;
  }

  if (GET_CAPACITY(vec1) < (TO_IMPL(vec1)->len + TO_IMPL(vec2)->len)) {
    bool resized = c_vec_set_capacity(vec1, TO_UNITS(vec1, TO_IMPL(vec1)->len + TO_IMPL(vec2)->len));
    if (!resized) return resized;
  }

  memcpy((uint8_t*)vec1->data + TO_IMPL(vec1)->len, vec2->data, TO_IMPL(vec2)->len);

  TO_IMPL(vec1)->len += TO_IMPL(vec2)->len;

  return true;
}

bool c_vec_fill_with_repeat(CVec* self, void* data, size_t data_len)
{
  assert(self && self->data);

  if (data_len > TO_UNITS(self, GET_CAPACITY(self))) {
    c_error_set(C_ERROR_invalid_len);
    return false;
  }

  size_t const number_of_repeats = TO_UNITS(self, GET_CAPACITY(self)) / data_len;
  size_t const repeat_size       = TO_BYTES(self, data_len);
  for (size_t iii = 0; iii < number_of_repeats; ++iii) {
    memcpy((uint8_t*)self->data + (iii * (repeat_size)), data, TO_BYTES(self, data_len));
  }

  TO_IMPL(self)->len = TO_BYTES(self, number_of_repeats * data_len);

  return true;
}

bool c_vec_replace(CVec* self, size_t index, size_t range_len, void* data,
                   size_t data_len)
{
  assert(self && self->data);

  if (range_len == 0) {
    c_error_set(C_ERROR_invalid_range);
    return false;
  }
  if (!data || data_len == 0) {
    c_error_set(C_ERROR_invalid_data);
    return false;
  }

  size_t len_as_units = TO_UNITS(self, TO_IMPL(self)->len);
  size_t cap_as_units = TO_UNITS(self, c_allocator_mem_size(self->data));

  if ((index + range_len) >= len_as_units) range_len = len_as_units - index;

  /// enlarge if needed
  if ((len_as_units - range_len + data_len) > cap_as_units) {
    cap_as_units -= range_len - data_len;
    bool resized = c_vec_set_capacity(self, cap_as_units);
    if (!resized) return resized;
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
      if (!resized) return resized;
    }
  }

  memcpy((uint8_t*)self->data + TO_BYTES(self, index), data, TO_BYTES(self, data_len));

  return true;
}

bool c_vec_rotate_right(CVec* self, size_t elements_count)
{
  assert(self && self->data);

  if (elements_count == 0) return true;
  if (TO_BYTES(self, elements_count) > TO_IMPL(self)->len) {
    c_error_set(C_ERROR_invalid_range);
    return false;
  }

  void* tmp_mem = c_allocator_alloc(TO_IMPL(self)->allocator,
                                    TO_BYTES(self, elements_count),
                                    TO_IMPL(self)->element_size, false);
  if (!tmp_mem) return false;

  memcpy(tmp_mem,
         (uint8_t*)self->data + TO_IMPL(self)->len -
             TO_BYTES(self, elements_count),
         TO_BYTES(self, elements_count));
  memmove((uint8_t*)self->data + TO_BYTES(self, elements_count), self->data, TO_BYTES(self, elements_count));
  memcpy(self->data, tmp_mem, TO_BYTES(self, elements_count));

  c_allocator_free(TO_IMPL(self)->allocator, tmp_mem);

  return true;
}

bool c_vec_rotate_left(CVec* self, size_t elements_count)
{
  assert(self && self->data);

  if (elements_count == 0) return true;
  if (TO_BYTES(self, elements_count) > TO_IMPL(self)->len) {
    c_error_set(C_ERROR_invalid_range);
    return false;
  }

  void* tmp_mem = c_allocator_alloc(TO_IMPL(self)->allocator,
                                    TO_BYTES(self, elements_count),
                                    TO_IMPL(self)->element_size, false);
  if (!tmp_mem) return false;

  memcpy(tmp_mem, self->data, TO_BYTES(self, elements_count));
  memmove(self->data, (uint8_t*)self->data + TO_BYTES(self, elements_count), TO_BYTES(self, elements_count));
  memcpy((uint8_t*)self->data + TO_IMPL(self)->len -
             TO_BYTES(self, elements_count),
         tmp_mem, TO_BYTES(self, elements_count));

  c_allocator_free(TO_IMPL(self)->allocator, tmp_mem);

  return true;
}

bool c_vec_remove(CVec* self, size_t index)
{
  assert(self && self->data);

  if (TO_BYTES(self, index) <= TO_IMPL(self)->len) {
    c_error_set(C_ERROR_invalid_index);
    return false;
  }

  uint8_t* element = (uint8_t*)self->data + TO_BYTES(self, index);

  memmove(element, element + TO_IMPL(self)->element_size, TO_IMPL(self)->len - TO_BYTES(self, index - 1));
  TO_IMPL(self)->len -= TO_IMPL(self)->element_size;

  if (c_vec_should_shrink(self)) {
    bool resized = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
    if (!resized) return resized;
  }

  return true;
}

bool c_vec_remove_range(CVec* self, size_t start_index, size_t range_len)
{
  assert(self && self->data);

  /// validation
  if (TO_IMPL(self)->len == 0U) {
    c_error_set(C_ERROR_empty);
    return false;
  }
  if (start_index > (TO_UNITS(self, TO_IMPL(self)->len) - 1U)) {
    c_error_set(C_ERROR_invalid_index);
    return false;
  }
  if (TO_BYTES(self, start_index + range_len) > TO_IMPL(self)->len) {
    c_error_set(C_ERROR_invalid_range);
    return false;
  }

  uint8_t* start_ptr       = (uint8_t*)self->data + TO_BYTES(self, start_index);
  uint8_t* end_ptr         = (uint8_t*)self->data + TO_BYTES(self, (start_index + range_len));
  size_t   right_range_len = TO_BYTES(self, TO_IMPL(self)->len - TO_BYTES(self, (start_index + range_len)));

  memmove(start_ptr, end_ptr, right_range_len);
  TO_IMPL(self)->len -= TO_BYTES(self, range_len);

  if (c_vec_should_shrink(self)) {
    bool resized = c_vec_set_capacity(self, TO_UNITS(self, GET_CAPACITY(self)) / 2);
    if (!resized) return resized;
  }

  return true;
}

bool c_vec_deduplicate(CVec* self, CVecCompareFn cmp)
{
  assert(self && self->data);

  if (!cmp) {
    c_error_set(C_ERROR_invalid_compare_fn);
    return false;
  }

  for (size_t iii = 0; iii < TO_UNITS(self, TO_IMPL(self)->len); ++iii) {
    for (size_t jjj = iii + 1; jjj < TO_UNITS(self, TO_IMPL(self)->len); ++jjj) {
      if (cmp((uint8_t*)self->data + (TO_IMPL(self)->element_size * iii),
              (uint8_t*)self->data + (TO_IMPL(self)->element_size * jjj)) ==
          0) {
        memmove((uint8_t*)self->data + TO_BYTES(self, jjj),
                (uint8_t*)self->data + TO_BYTES(self, jjj + 1),
                TO_IMPL(self)->len - TO_BYTES(self, jjj + 1));
        TO_IMPL(self)->len--;
        jjj--;
      }
    }
  }

  return true;
}

CVec* c_vec_slice(CVec const* self, size_t start_index, size_t range_len)
{
  assert(self && self->data);

  if (TO_BYTES(self, start_index) > TO_IMPL(self)->len) {
    c_error_set(C_ERROR_invalid_index);
    return NULL;
  }

  range_len = (TO_UNITS(self, TO_IMPL(self)->len) - start_index) < range_len
                  ? TO_UNITS(self, TO_IMPL(self)->len)
                  : range_len;

  CVec* vec = c_vec_create_from_raw(
      (uint8_t*)self->data + TO_BYTES(self, start_index), range_len,
      TO_IMPL(self)->element_size, false, TO_IMPL(self)->allocator);
  return vec;
}

CIter c_vec_iter(CVec* self)
{
  assert(self);

  CIter iter = c_iter(self->data, TO_IMPL(self)->len, TO_IMPL(self)->element_size);
  return iter;
}

bool c_vec_reverse(CVec* self)
{
  assert(self && self->data);

  uint8_t* start = self->data;
  uint8_t* end   = (uint8_t*)self->data + (TO_IMPL(self)->len - TO_IMPL(self)->element_size);

  void* tmp_mem = c_allocator_alloc(TO_IMPL(self)->allocator, TO_IMPL(self)->element_size, TO_IMPL(self)->element_size, false);
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

void c_vec_clear(CVec* self)
{
  assert(self && self->data);

  memset(self->data, 0, TO_UNITS(self, TO_IMPL(self)->len));
  TO_IMPL(self)->len = 0;
}

CStrBuf* c_cvec_to_cstrbuf(CVec* self)
{
  assert(self);
  return (CStrBuf*)self;
}

void c_vec_destroy(CVec* self)
{
  if (self && self->data) {
    CAllocator* allocator = TO_IMPL(self)->allocator;
    if (TO_IMPL(self)->raw_capacity == 0) c_allocator_free(allocator, self->data);
    *TO_IMPL(self) = (CVecImpl){0};
    c_allocator_free(allocator, self);
  }
}

#ifdef MSC_VER
#pragma warning(pop)
#endif
