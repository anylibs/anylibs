#ifndef ANYLIBS_HASHMAP_H
#define ANYLIBS_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "allocator.h"

typedef struct CHashMap CHashMap;
typedef struct CHashMapIter {
  CHashMap* map;
  size_t    index;
} CHashMapIter;
typedef void (*CHashMapElementDestroyFn)(void* key, void* value, void* user_data);

CHashMap*    c_hashmap_create(size_t key_size, size_t value_size, CAllocator* allocator);
CHashMap*    c_hashmap_create_with_capacity(size_t key_size, size_t value_size, size_t capacity, CAllocator* allocator);
size_t       c_hashmap_len(CHashMap const* self);
bool         c_hashmap_is_empty(CHashMap const* self);
size_t       c_hashmap_capacity(CHashMap const* self);
bool         c_hashmap_insert(CHashMap* self, void* key, void* value);
bool         c_hashmap_get(CHashMap const* self, void* key, void** out_value);
bool         c_hashmap_has_key(CHashMap const* self, void* key);
bool         c_hashmap_remove(CHashMap* self, void* key, void** out_value);
void         c_hashmap_clear(CHashMap* self, CHashMapElementDestroyFn element_destroy_fn, void* user_data);
CHashMapIter c_hashmap_iter(CHashMap* self);
bool         c_hashmap_iter_next(CHashMapIter* iter, void** key, void** value);
void         c_hashmap_destroy(CHashMap* self, CHashMapElementDestroyFn element_destroy_fn, void* user_data);

#endif // ANYLIBS_HASHMAP_H
