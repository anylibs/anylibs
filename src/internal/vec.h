#ifndef ANYLIBS_INTERNAL_VEC_H
#define ANYLIBS_INTERNAL_VEC_H

#include <stddef.h>

#include "anylibs/allocator.h"

typedef struct CVecImpl {
  void*       data;         ///< heap allocated data
  size_t      len;          ///< current length in bytes
  CAllocator* allocator;    ///< memory allocator/deallocator
  size_t      raw_capacity; ///< this is only useful when used with
                            ///< @ref c_vec_create_from_raw
  size_t element_size;      ///< size of the element unit
} CVecImpl;

#endif // ANYLIBS_INTERNAL_VEC_H
