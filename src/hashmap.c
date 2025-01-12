#include "anylibs/hashmap.h"
#include "anylibs/error.h"

#include <assert.h>

#define CMAP_DEFAULT_CAPACITY 16U

typedef struct CHashMap {
  void*  buckets; // { [CHashMapBucket, key, value], ... }
  void*  spare_bucket1; // { [CHashMapBucket, key, value] }
  void*  spare_bucket2; // { [CHashMapBucket, key, value] }
  size_t capacity; ///< in units
  size_t len; ///< in units
  struct {
    size_t orig;
    size_t aligned;
  } key_size;
  struct {
    size_t orig;
    size_t aligned;
  } value_size;
  size_t      bucket_size;
  size_t      mask;
  CAllocator* allocator;
} CHashMap;

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

typedef struct CHashMapBucket CHashMapBucket;
struct CHashMapBucket {
  uint64_t distance_from_initial_bucket : 16;
  uint64_t hash : 48;
};

static const size_t spare_buckets_count = 2U;

static size_t                 c_internal_map_hash(const void* data, size_t data_len);
static size_t                 c_internal_map_hash_fnv(const void* data, size_t data_len);
static size_t                 c_internal_map_clip_hash(size_t hash);
static bool                   c_internal_map_resize(CHashMap* self, size_t new_capacity);
static inline void*           c_internal_map_get_key(CHashMap const* self, size_t index);
static inline void*           c_internal_map_get_value(CHashMap const* self, size_t index);
static inline CHashMapBucket* c_internal_map_get_bucket(CHashMap const* self, size_t index);

CHashMap* c_hashmap_create(size_t key_size, size_t value_size, CAllocator* allocator)
{
  return c_hashmap_create_with_capacity(key_size, value_size, CMAP_DEFAULT_CAPACITY, allocator);
}

CHashMap* c_hashmap_create_with_capacity(size_t key_size, size_t value_size, size_t capacity, CAllocator* allocator)
{
  if (!key_size || !value_size) {
    c_error_set(C_ERROR_invalid_size);
    return NULL;
  }
  if (!capacity) {
    c_error_set(C_ERROR_invalid_capacity);
    return NULL;
  }

  // make sure the capacity is multiplicate of 2
  if (capacity < CMAP_DEFAULT_CAPACITY) {
    capacity = CMAP_DEFAULT_CAPACITY;
  } else {
    size_t new_capacity = CMAP_DEFAULT_CAPACITY;
    while (new_capacity < capacity) {
      new_capacity *= 2;
    }
    capacity = new_capacity;
  }

  if (!allocator) allocator = c_allocator_default();

  CHashMap* map = c_allocator_alloc(allocator, c_allocator_alignas(CHashMap, 1), true);
  if (!map) return NULL;

  // make sure the key and value sizes are multiplicate of sizeof(uintptr_t)
  size_t aligned_key_size   = key_size;
  size_t aligned_value_size = value_size;
  while (aligned_key_size & (sizeof(uintptr_t) - 1)) {
    aligned_key_size++;
  }
  while (aligned_value_size & (sizeof(uintptr_t) - 1)) {
    aligned_value_size++;
  }
  size_t bucket_size = sizeof(CHashMapBucket) + aligned_key_size + aligned_value_size;

  map->spare_bucket1 = c_allocator_alloc(allocator, (capacity + spare_buckets_count) * bucket_size, sizeof(CHashMapBucket), true);
  if (!map->spare_bucket1) goto ON_ERROR;
  map->spare_bucket2 = (char*)(map->spare_bucket1) + bucket_size;
  map->buckets       = (char*)(map->spare_bucket2) + bucket_size;

  map->capacity           = capacity;
  map->key_size.orig      = key_size;
  map->key_size.aligned   = aligned_key_size;
  map->value_size.orig    = value_size;
  map->value_size.aligned = aligned_value_size;
  map->bucket_size        = bucket_size;
  map->mask               = map->capacity - 1;
  map->allocator          = allocator;

  return map;

ON_ERROR:
  c_allocator_free(allocator, map->spare_bucket1);
  c_allocator_free(allocator, map);
  return NULL;
}

size_t c_hashmap_len(CHashMap const* self)
{
  return self->len;
}

bool c_hashmap_is_empty(CHashMap const* self)
{
  return self->len == 0;
}

size_t c_hashmap_capacity(CHashMap const* self)
{
  return self->capacity;
}

bool c_hashmap_insert(CHashMap* self, void* key, void* value)
{
  assert(self && self->buckets);

  if (!key || !value) {
    c_error_set(C_ERROR_invalid_data);
    return false;
  }

  size_t hash  = c_internal_map_hash(key, self->key_size.orig);
  size_t index = hash & self->mask;

  if (self->capacity <= self->len) {
    bool status = c_internal_map_resize(self, self->capacity * 2);
    if (!status) return false;
  }

  CHashMapBucket* new_bucket               = self->spare_bucket1;
  new_bucket->distance_from_initial_bucket = 1;
  new_bucket->hash                         = hash;
  memcpy((char*)(&new_bucket[1]), key, self->key_size.orig);
  memcpy((char*)(&new_bucket[1]) + self->key_size.aligned, value, self->value_size.orig);

  for (;;) {
    CHashMapBucket* bucket     = c_internal_map_get_bucket(self, index);
    void*           bucket_key = c_internal_map_get_key(self, index);

    // [1] empty bucket
    if (bucket->distance_from_initial_bucket == 0) {
      memcpy(bucket, new_bucket, self->bucket_size);
      self->len++;
      return true;
    }

    // [2] found one, same hash, update it
    /// TODO: we need external compare method
    /// TODO: we need to return old data
    if ((new_bucket->hash == bucket->hash) && (memcmp(bucket_key, key, self->key_size.orig) == 0)) {
      memcpy(bucket, new_bucket, self->bucket_size);
      return true;
    }

    // [3] found one, different hash, collision
    if (bucket->distance_from_initial_bucket < new_bucket->distance_from_initial_bucket) {
      // swap
      CHashMapBucket* tmp = (CHashMapBucket*)(self->spare_bucket2);
      memcpy(tmp, bucket, self->bucket_size);
      memcpy(bucket, new_bucket, self->bucket_size);
      memcpy(new_bucket, tmp, self->bucket_size);
    }

    index = (index + 1) & self->mask;
    new_bucket->distance_from_initial_bucket++;
  }

  return false;
}

bool c_hashmap_get(CHashMap const* self, void* key, void** out_value)
{
  assert(self && self->buckets);

  if (!key || !out_value) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  size_t hash = c_internal_map_hash(key, self->key_size.orig);

  for (size_t index = hash & self->mask;; index = (index + 1) & self->mask) {
    CHashMapBucket* bucket       = c_internal_map_get_bucket(self, index);
    void*           bucket_key   = c_internal_map_get_key(self, index);
    void*           bucket_value = c_internal_map_get_value(self, index);

    if (!bucket->distance_from_initial_bucket) {
      *out_value = NULL;
      break;
    }

    if (bucket->hash == hash) {
      if (memcmp(bucket_key, key, self->key_size.orig) == 0) {
        *out_value = bucket_value;
        break;
      }
    }
  }

  return true;
}

bool c_hashmap_has_key(CHashMap const* self, void* key)
{
  void* out_value;
  bool  status = c_hashmap_get(self, key, &out_value);

  return status;
}

bool c_hashmap_remove(CHashMap* self, void* key, void** out_value)
{
  assert(self && self->buckets);

  if (!key || !out_value) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }

  size_t hash = c_internal_map_hash(key, self->key_size.orig);

  for (size_t index = hash & self->mask;; index = (index + 1) & self->mask) {
    CHashMapBucket* bucket = c_internal_map_get_bucket(self, index);
    if (!bucket->distance_from_initial_bucket) {
      c_error_set(C_ERROR_not_found);
      return false;
    }

    void* bucket_key = c_internal_map_get_key(self, index);
    if (bucket->hash == hash && (memcmp(bucket_key, key, self->key_size.orig) == 0)) {
      memcpy(self->spare_bucket1, bucket, self->bucket_size);
      bucket->distance_from_initial_bucket = 0;

      for (;;) {
        CHashMapBucket* prev   = bucket;
        index                  = (index + 1) & self->mask;
        CHashMapBucket* bucket = c_internal_map_get_bucket(self, index);
        if (bucket->distance_from_initial_bucket <= 1) {
          prev->distance_from_initial_bucket = 0;
          break;
        }

        memcpy(prev, bucket, self->bucket_size);
        prev->distance_from_initial_bucket--;
      }

      self->len--;

      if ((self->len <= (self->capacity / 4)) && (self->len > CMAP_DEFAULT_CAPACITY)) {
        c_internal_map_resize(self, self->capacity / 2);
      }

      *out_value = (char*)self->spare_bucket1 + sizeof(CHashMapBucket) + self->key_size.aligned;
      break;
    }
  }

  return true;
}

void c_hashmap_clear(CHashMap* self, CHashMapElementDestroyFn element_destroy_fn, void* user_data)
{
  if (element_destroy_fn) {
    void*        key;
    void*        value;
    CHashMapIter iter = c_hashmap_iter(self);
    while (c_hashmap_iter_next(&iter, &key, &value)) {
      element_destroy_fn(key, value, user_data);
    }
  }

  memset(self->spare_bucket1, 0, (self->capacity + 2) * self->bucket_size);
  self->len = 0;
}

CHashMapIter c_hashmap_iter(CHashMap* self)
{
  assert(self && self->spare_bucket1);

  CHashMapIter iter = {
      .map = self,
  };

  return iter;
}

bool c_hashmap_iter_next(CHashMapIter* iter, void** key, void** value)
{
  if (!iter) return false;

  if (!iter->index) iter->index = 0;

  for (; iter->index < iter->map->capacity; ++iter->index) {
    CHashMapBucket* bucket = c_internal_map_get_bucket(iter->map, iter->index);
    if (bucket->distance_from_initial_bucket) {
      if (key) { *key = c_internal_map_get_key(iter->map, iter->index); }
      if (value) { *value = c_internal_map_get_value(iter->map, iter->index); }
      iter->index++;
      break;
    }
  }

  if (iter->index >= iter->map->capacity) {
    return false;
  } else {
    return true;
  }
}

void c_hashmap_destroy(CHashMap* self, CHashMapElementDestroyFn element_destroy_fn, void* user_data)
{
  if (self && self->buckets) {
    if (element_destroy_fn) {
      c_hashmap_clear(self, element_destroy_fn, user_data);
    }
    CAllocator* allocator = self->allocator;
    c_allocator_free(allocator, self->spare_bucket1);
    *self = (CHashMap){0};
    c_allocator_free(allocator, self);
  }
}

// ------------------------- internal ------------------------- //

size_t c_internal_map_hash(const void* data, size_t data_len)
{
  /// TODO: make the hash algo changeable
  return c_internal_map_clip_hash(c_internal_map_hash_fnv(data, data_len));
}

size_t c_internal_map_hash_fnv(const void* data, size_t data_len)
{
  // Constants for the FNV-1a hash function
  const size_t fnv_prime = 1099511628211U;
  size_t       hash      = 14695981039346656037U; // FNV offset basis

  const uint8_t* bytes = (const uint8_t*)data;

  for (size_t i = 0; i < data_len; i++) {
    hash ^= bytes[i]; // XOR the byte with the hash
    hash *= fnv_prime; // Multiply by the FNV prime
  }

  return hash;
}

bool c_internal_map_resize(CHashMap* self, size_t new_capacity)
{
  CHashMap* new_map = c_hashmap_create_with_capacity(self->key_size.orig, self->value_size.orig, new_capacity, self->allocator);
  if (new_map) return false;

  for (size_t iii = 0; iii < self->capacity; iii++) {
    CHashMapBucket* bucket = c_internal_map_get_bucket(self, iii);
    if (!bucket->distance_from_initial_bucket) continue;

    bucket->distance_from_initial_bucket = 1;

    for (size_t new_index = bucket->hash & new_map->mask;; new_index = (new_index + 1) & new_map->mask) {
      CHashMapBucket* new_map_bucket = c_internal_map_get_bucket(new_map, new_index);

      if (new_map_bucket->distance_from_initial_bucket == 0) {
        memcpy(new_map_bucket, bucket, self->bucket_size);
        break;
      }

      if (new_map_bucket->distance_from_initial_bucket < bucket->distance_from_initial_bucket) {
        // swap
        memcpy(self->spare_bucket2, bucket, self->bucket_size);
        memcpy(bucket, new_map_bucket, self->bucket_size);
        memcpy(new_map_bucket, self->spare_bucket2, self->bucket_size);
      }

      bucket->distance_from_initial_bucket += 1;
    }
  }

  memcpy(new_map->spare_bucket1, self->spare_bucket1, self->bucket_size);
  memcpy(new_map->spare_bucket2, self->spare_bucket2, self->bucket_size);
  c_allocator_free(self->allocator, self->spare_bucket1);

  self->spare_bucket1 = new_map->spare_bucket1;
  self->spare_bucket2 = new_map->spare_bucket2;
  self->buckets       = new_map->buckets;
  self->capacity      = new_map->capacity;
  self->mask          = new_map->mask;

  return new_map;
}

void* c_internal_map_get_key(const CHashMap* self, size_t index)
{
  return (void*)(((char*)(c_internal_map_get_bucket(self, index))) + sizeof(CHashMapBucket));
}

void* c_internal_map_get_value(const CHashMap* self, size_t index)
{
  return (void*)(((char*)(c_internal_map_get_bucket(self, index))) + sizeof(CHashMapBucket) + self->key_size.aligned);
}

CHashMapBucket* c_internal_map_get_bucket(const CHashMap* self, size_t index)
{
  return (CHashMapBucket*)(((char*)(self->buckets)) + (self->bucket_size * index));
}

size_t c_internal_map_clip_hash(size_t hash)
{
  return hash & 0xFFFFFFFFFFFF;
}
