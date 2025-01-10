#ifndef ANYLIBS_MAP_H
#define ANYLIBS_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "allocator.h"

typedef struct CMap CMap;
typedef struct CMapIter {
  CMap*  map;
  size_t index;
} CMapIter;
typedef void (*CMapElementDestroyFn)(void* key, void* value, void* user_data);

CMap*    c_map_create(size_t key_size, size_t value_size, CAllocator* allocator);
CMap*    c_map_create_with_capacity(size_t key_size, size_t value_size, size_t capacity, CAllocator* allocator);
size_t   c_map_len(CMap const* self);
bool     c_map_is_empty(CMap const* self);
size_t   c_map_capacity(CMap const* self);
bool     c_map_insert(CMap* self, void* key, void* value);
bool     c_map_get(CMap const* self, void* key, void** out_value);
bool     c_map_has_key(CMap const* self, void* key);
bool     c_map_remove(CMap* self, void* key, void** out_value);
void     c_map_clear(CMap* self, CMapElementDestroyFn element_destroy_fn, void* user_data);
CMapIter c_map_iter(CMap* self);
bool     c_map_iter_next(CMapIter* iter, void** key, void** value);
void     c_map_destroy(CMap* self, CMapElementDestroyFn element_destroy_fn, void* user_data);

#endif // ANYLIBS_MAP_H
