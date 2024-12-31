#ifndef ANYLIBS_VEC_H
#define ANYLIBS_VEC_H
#include <stdbool.h>
#include <stddef.h>

#include "allocator.h"
#include "iter.h"

typedef struct CVec {
  void* data; ///< heap allocated data
} CVec;
typedef struct CStrBuf CStrBuf;
typedef int (*CVecCompareFn)(void const*, void const*); ///< this is similar to strcmp

CVec*    c_vec_create(size_t element_size, CAllocator* allocator); ///< create a new CVec object, allocator could be NULL, in that case c_allocator_default will be used
CVec*    c_vec_create_with_capacity(size_t element_size, size_t capacity, bool set_mem_to_zero, CAllocator* allocator); ///< same like c_vec_create
CVec*    c_vec_create_from_raw(void* data, size_t data_len, size_t element_size, bool should_copy, CAllocator* allocator); ///< same like c_vec_create, should_copy[false]: it will not allocate new memory
CVec*    c_vec_clone(CVec const* self, bool should_shrink_clone); ///< deep copy
bool     c_vec_is_empty(CVec const* self); ///< when length is zero
size_t   c_vec_len(CVec const* self); ///< length in units
bool     c_vec_set_len(CVec* self, size_t new_len); ///< set new length, which should be less than the vector length
size_t   c_vec_capacity(CVec const* self); ///< capacity in units
size_t   c_vec_spare_capacity(CVec const* self); ///< return the remaining capacity in units
bool     c_vec_set_capacity(CVec* self, size_t new_capacity); ///< set capacity, this could change the internal data address
size_t   c_vec_element_size(CVec* self); ///< return element size in bytes
bool     c_vec_shrink_to_fit(CVec* self); ///< make capacity equals to length
bool     c_vec_get(CVec const* self, size_t index, void** out_data); ///< get an element at index through out_data
bool     c_vec_find(CVec const* self, void* element, CVecCompareFn cmp, void** out_data); ///< find an element and return pointer to it if exists
bool     c_vec_binary_find(CVec const* self, void const* element, CVecCompareFn cmp, void** out_data); ///< same like c_vec_find, but will use binary search tree, If data is not sorted, the returned result is unspecified and meaningless
int      c_vec_starts_with(CVec const* self, void const* data, size_t data_len, CVecCompareFn cmp); ///< check if the vector starts with data using cmp, 0 => success, 1 => failed, -1 => failed with error
int      c_vec_ends_with(CVec const* self, void const* data, size_t data_len, CVecCompareFn cmp); ///< check if the vector ends with data using cmp, 0 => success, 1 => failed, -1 => failed with error
bool     c_vec_sort(CVec* self, CVecCompareFn cmp); ///< sort using cmp
int      c_vec_is_sorted(CVec* self, CVecCompareFn cmp); ///< check if sorted using cmp, 0 => success, 1 => failed, -1 => failed with error
bool     c_vec_push(CVec* self, void const* element); ///< push a new element to the end, this will copy the element data, this could resize the data
bool     c_vec_push_range(CVec* self, void const* elements, size_t elements_len); ///< same like c_vec_push, but push multiple elements, this could resize the data
bool     c_vec_pop(CVec* self, void* out_element); ///< pop last element, and if out_element is not NULL, copy the element to out_element, this could resize the data
bool     c_vec_insert(CVec* self, size_t index, void const* element); ///< insert one element at index, this could resize the data
bool     c_vec_insert_range(CVec* self, size_t index, void const* data, size_t data_len); ///< same like c_vec_insert, but insert multiple elements, this could resize the data
void     c_vec_fill(CVec* self, void* data); ///< this is similar to memset
bool     c_vec_fill_with_repeat(CVec* self, void* data, size_t data_len); ///< similar to c_vec_fill, but the data here is more than one element
bool     c_vec_replace(CVec* self, size_t index, size_t range_len, void* data, size_t data_len); ///<  replace at index with range_len with data
bool     c_vec_concatenate(CVec* vec1, CVec const* vec2); ///< concatenate vec2 to vec1
bool     c_vec_rotate_right(CVec* self, size_t elements_count); ///< rotate right with elements_count times
bool     c_vec_rotate_left(CVec* self, size_t elements_count); ///< rotate left with elements_count times
bool     c_vec_remove(CVec* self, size_t index); ///< remove one element at index
bool     c_vec_remove_range(CVec* self, size_t start_index, size_t range_len); ///< samelike c_vec_remove but this will remove a range
bool     c_vec_deduplicate(CVec* self, CVecCompareFn cmp); ///< remove any duplicated elements in place
CVec*    c_vec_slice(CVec const* self, size_t start_index, size_t range_len); ///< return a sub vector starting from start_index, this internally will reference the original data, if range_len is bigger than the vector length, the vector length will be used instead
CIter    c_vec_iter(CVec* self); ///< create an iterator, for other functionality check iter.h
bool     c_vec_reverse(CVec* self); ///< reverse the vector, this will allocate a tmp memory
void     c_vec_clear(CVec* self); ///< set the length to zero, set the data to zero, keep the capacity as it is
CStrBuf* c_cvec_to_cstrbuf(CVec* self); ///< convert a vector to string in place, this is so cheap and it will not allocate new memory
void     c_vec_destroy(CVec* self); ///< destroy a vector object created, if self is NULL, nothing will happen

#endif // ANYLIBS_VEC_H
