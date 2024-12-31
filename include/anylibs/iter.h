#ifndef ANYLIBS_ITER_H
#define ANYLIBS_ITER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct CIter {
  void*  data;
  size_t data_size;
  void*  ptr;
  size_t step_size;
} CIter;

CIter c_iter(void* data, size_t data_size, size_t step_size /* , bool reversed */); // create new iter, step_size usually the sizeof(T), reversed[false]: start from the end(this is only useful with c_iter_prev)
bool  c_iter_next(CIter* self, void** out_data); // get the next element, out_data could be NULL and this will advance the iterator only
bool  c_iter_prev(CIter* self, void** out_data); // get the previous element, out_data could be NULL and this will advance the iterator only
bool  c_iter_nth(CIter* self, size_t index, void** out_data); // get nth element at index, and advance the iterator to index, out_data could be NULL and this will advance the iterator only
bool  c_iter_peek(CIter const* self, void** out_data); // get the next element without advancing the iterator, out_data could be NULL but this make this function useless
void* c_iter_first(CIter* self); // get the first element
void* c_iter_last(CIter* self); // get the last element

#endif // ANYLIBS_ITER_H
