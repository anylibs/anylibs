#include "anylibs/str.h"
#include "anylibs/error.h"
#include "anylibs/iter.h"
#include "anylibs/vec.h"
#include "internal/vec.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#if _WIN32 && (!_MSC_VER || !(_MSC_VER >= 1900))
#error "You need MSVC must be higher that or equal to 1900"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // disable warning about unsafe functions
#endif

#define TO_VEC(str) ((CVec*)(str))
#define TO_IMPL(str) ((CVecImpl*)(str))
#define FROM_IMPL(impl) ((CStrBuf*)(impl))
#define TO_BYTES(str, units) ((units) * TO_IMPL(str)->element_size)
#define TO_UNITS(str, bytes) ((bytes) / TO_IMPL(str)->element_size)
#define GET_CAPACITY(str) (TO_IMPL(str)->raw_capacity > 0 ? TO_IMPL(str)->raw_capacity \
                                                          : c_allocator_mem_size(TO_IMPL(str)->data))

/// @brief ASCII whitespaces
static unsigned char const c_ascii_whitespaces[] = {
    ' ', // space
    '\t', // horizontal tab
    '\n', // newline
    '\v', // vertical tab
    '\f', // form feed
    '\r' // carriage return
};

/// @brief UTF-8 whitespaces
#define CHAR(s) {s, sizeof(s) - 1}
static CChar c_whitespaces[] = {
    CHAR("\t"), // character tabulation
    CHAR("\n"), // line feed
    CHAR("\v"), // line tabulation
    CHAR("\f"), // form feed
    CHAR("\r"), // carriage return
    CHAR(" "), // space
    CHAR(""), // next line
    CHAR("\u00A0"), // no-break space
    CHAR("\u1680"), // ogham space mark
    CHAR("\u2000"), // en quad
    CHAR("\u2001"), // em quad
    CHAR("\u2002"), // en space
    CHAR("\u2003"), // em space
    CHAR("\u2004"), // three-per-em space
    CHAR("\u2005"), // four-per-em space
    CHAR("\u2006"), // six-per-em space
    CHAR("\u2007"), // figure space
    CHAR("\u2008"), // punctuation space
    CHAR("\u2009"), // thin space
    CHAR("\u200A"), // hair space
    CHAR("\u2028"), // line separator
    CHAR("\u2029"), // paragraph separator
    CHAR("\u202F"), // narrow no-break space
    CHAR("\u205F"), // medium mathematical space
    CHAR("\u3000"), // ideographic space
    CHAR("\u180E"), // mongolian vowel separator
    CHAR("\u200B"), // zero width space
    CHAR("\u200C"), // zero width non-joiner
    CHAR("\u200D"), // zero width joiner
    CHAR("\u2060"), // word joiner
    CHAR("\uFEFF") // zero width non-breaking space
};

typedef struct CChar16 {
  uint16_t data[2];
  size_t   count;
} CChar16;

typedef struct CChar32 {
  uint32_t data;
  size_t   count;
} CChar32;

static bool c_internal_str_utf8_to_utf32(const char* utf8, CChar32* out_ch);
static bool c_internal_str_utf32_to_utf16(CChar32 utf32, CChar16* out_ch);

/// @brief create @ref CStrBuf object
/// @param allocator the allocator (if NULL the Default Allocator will be used)
/// @return new allocated @ref CStrBuf or NULL on error
CStrBuf* c_str_create(CAllocator* allocator)
{
  CStrBuf* str = c_str_create_with_capacity(1, allocator, false);
  return str;
}

/// @brief create @ref CStrBuf object from array of chars
/// @param cstr the input array of chars
/// @param should_copy true: will copy the data, false: will just reference the
/// data
/// @param allocator the allocator (if NULL the Default Allocator will be used)
/// @return new allocated @ref CStrBuf or NULL on error
CStrBuf* c_str_create_from_raw(CStr cstr, bool should_copy,
                               CAllocator* allocator)
{
  if (!allocator) allocator = c_allocator_default();

  if (!should_copy) {
    CVecImpl* impl = c_allocator_alloc(allocator, c_allocator_alignas(CVecImpl, 1), false);
    if (!impl) return NULL;

    impl->data         = cstr.data;
    impl->raw_capacity = cstr.len;
    impl->len          = cstr.len;
    impl->allocator    = allocator;
    impl->element_size = sizeof(char);

    return FROM_IMPL(impl);
  } else {
    CStrBuf* str = c_str_create_with_capacity(cstr.len + 1, allocator, false);
    if (!str) return NULL;

    memcpy((str)->data, cstr.data, cstr.len);
    str->data[cstr.len] = '\0';

    TO_IMPL(str)->len = cstr.len;

    return str;
  }
}

/// @brief create new empty @ref CStrBuf object with capacity
/// @param capacity
/// @param allocator the allocator (if NULL the Default Allocator will be used)
/// @param zero_initialized
/// @return new allocated @ref CStrBuf or NULL on error
CStrBuf* c_str_create_with_capacity(size_t capacity, CAllocator* allocator,
                                    bool zero_initialized)
{
  CStrBuf* str = (CStrBuf*)c_vec_create_with_capacity(
      sizeof(char), capacity, zero_initialized, allocator);
  return str;
}

/// @brief clone another @ref CStrBuf object
/// @param self
/// @param should_shrink_clone true: shrink the clone capacity to @ref CStrBuf
/// len
/// @return new allocated @ref CStrBuf or NULL on error
CStrBuf* c_str_clone(CStrBuf const* self, bool should_shrink_clone)
{
  CStrBuf* str = (CStrBuf*)c_vec_clone((CVec const*)self, should_shrink_clone);
  return str;
}

/// @brief return if @ref CStrBuf is empty
/// @note this will also return false on failure if any
/// @param self
/// @return
bool c_str_is_empty(CStrBuf const* self)
{
  return c_vec_is_empty((CVec const*)self);
}

/// @brief return the number of utf8 characters
/// @param self
/// @return is_ok[true]: utf-8 characters count
///         is_ok[false]: invalid utf-8 or other errors
int c_str_count(CStrBuf const* self)
{
  assert(self);

  CIter  iter = c_str_iter((CStrBuf*)self);
  CStr   ch;
  size_t counter = 0;
  while (c_str_iter_next(&iter, &ch)) {
    counter++;
  }

  if ((uint8_t*)iter.ptr < (uint8_t*)iter.data + iter.data_size - 1) {
    c_error_set(C_ERROR_invalid_unicode);
    return -1;
  }

  return (int)counter;
}

/// @brief return @ref CStrBuf length in bytes
/// @param self
/// @return length in bytes
size_t c_str_len(CStrBuf const* self)
{
  return c_vec_len((CVec const*)self);
}

/// @brief set new length
/// @note this could reallocate @ref CStrBuf::data
/// @param self
/// @param new_len
void c_str_set_len(CStrBuf* self, size_t new_len)
{
  // 1 is for the trailing 0
  c_vec_set_len((CVec*)self, new_len + 1);
  self->data[TO_IMPL(self)->len - 1] = '\0';
  TO_IMPL(self)->len -= 1;
}

/// @brief get capacity in bytes
/// @param self
/// @return capacity
size_t c_str_capacity(CStrBuf const* self)
{
  return c_vec_capacity((CVec const*)self);
}

/// @brief return the empty remaining capacity in bytes
/// @param self
/// @return spare_capacity
size_t c_str_spare_capacity(CStrBuf const* self)
{
  return c_vec_spare_capacity((CVec const*)self);
}

/// @brief set new capacity
/// @note this could reallocate @ref CStrBuf::data
/// @param self
/// @param new_capacity
/// @return true: success, false: failed (error)
bool c_str_set_capacity(CStrBuf* self, size_t new_capacity)
{
  bool is_shrinking = GET_CAPACITY(self) > new_capacity;
  // 1 is for the trailing 0
  bool status = c_vec_set_capacity((CVec*)self, new_capacity + 1);
  if (status && is_shrinking) {
    self->data[TO_IMPL(self)->len - 1] = '\0';
  }

  return status;
}

/// @brief set capacity to the current length
/// @note this could reallocate @ref CStrBuf::data
/// @param self
/// @return true: success, false: failed (error)
bool c_str_shrink_to_fit(CStrBuf* self)
{
  assert(self);

  TO_IMPL(self)->len += 1;
  bool status = c_vec_shrink_to_fit((CVec*)self);
  TO_IMPL(self)->len -= 1;

  return status;
}

/// @brief get a byte (char) from @p index
/// @param self
/// @param start_index
/// @param range_size
/// @return is_ok[true]: return @ref CStr that has a pointer to character at @p
///         start_index, is_ok[false]: not found (or error happened)
bool c_str_get(CStrBuf const* self, size_t start_index, size_t range_size,
               CStr* out_str)
{
  assert(self);

  if (!out_str) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }
  if (start_index >= TO_IMPL(self)->len) {
    c_error_set(C_ERROR_invalid_index);
    return false;
  }

  out_str->data = self->data + start_index;
  out_str->len  = range_size;

  return true;
}

/// @brief find utf8 char
/// @param self
/// @param ch a struct that you need to fill it manually
/// @return is_ok[true]: return @ref CStr that has a pointer to character at @p
///         start_index, is_ok[false]: not found (or error happened)
bool c_str_find(CStrBuf const* self, CStr data, CStr* out_str)
{
  CIter iter   = c_str_iter((CStrBuf*)self);
  bool  status = c_str_find_by_iter(&iter, data, out_str);

  return status;
}

/// @brief same like @ref c_str_find by it will use external iter
///        so you can for example use @ref c_str_iter_nth to start from some
///        index
/// @param self
/// @param ch a struct that you need to fill it manually
/// @param iter
/// @return is_ok[true]: return @ref CStr that has a pointer to character at @p
///         start_index, is_ok[false]: not found (or error happened)
bool c_str_find_by_iter(CIter* iter, CStr data, CStr* out_str)
{
  if (!out_str) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }
  if (!iter) {
    c_error_set(C_ERROR_invalid_iterator);
    return false;
  }
  if ((uint8_t*)iter->ptr >= (uint8_t*)iter->data + iter->data_size) {
    c_error_set(C_ERROR_invalid_iterator);
    return false;
  }

  while (c_str_iter_next(iter, out_str)) {
    if (memcmp(data.data, out_str->data, out_str->len) == 0) {
      return true;
    }
  }

  return false;
}

/// @brief check if @ref CStrBuf starts with @p data
/// @param self
/// @param cstr the input array of chars
/// @return true/false
int c_str_starts_with(CStrBuf const* self, CStr cstr)
{
  return c_vec_starts_with((CVec const*)self, cstr.data, cstr.len,
                           (CVecCompareFn)strcmp);
}

/// @brief check if @ref CStrBuf ends with @p data
/// @param self
/// @param cstr the input array of chars
/// @return true/false
int c_str_ends_with(CStrBuf const* self, CStr cstr)
{
  return c_vec_ends_with((CVec const*)self, cstr.data, cstr.len,
                         (CVecCompareFn)strcmp);
}

/// @brief push some data to the end
/// @param self
/// @param cstr the input array of chars
/// @return true: success, false: failed (error)
bool c_str_push(CStrBuf* self, CStr cstr)
{
  TO_IMPL(self)->len += 1;
  bool status = c_vec_insert_range((CVec*)self, TO_IMPL(self)->len - 1,
                                   cstr.data, cstr.len);
  TO_IMPL(self)->len -= 1;

  return status;
}

/// @brief pop last character from the end
/// @note this could reallocate @ref CStrBuf::data
/// @param self
/// @return is_ok[true]: return @ref CCharOwned
///         is_ok[false]: failed (or error happened)
bool c_str_pop(CStrBuf* self, CChar* out_ch)
{
  assert(self);

  if (TO_IMPL(self)->len == 0) {
    c_error_set(C_ERROR_empty);
    return false;
  }

  CIter iter    = c_str_iter(self);
  CStr  last_ch = c_str_iter_last(&iter);

  if (out_ch) {
    memcpy(out_ch->data, last_ch.data, last_ch.len);
    out_ch->count = last_ch.len;
  }

  c_str_set_len(self, TO_IMPL(self)->len - last_ch.len);
  return true;
}

/// @brief insert @p data at @p index
/// @note @p index is in bytes not utf8 characters
/// @note this could reallocate @ref CStrBuf::data
/// @param self
/// @param byte_index
/// @param cstr the input array of chars
/// @return true: success, false: failed (error)
bool c_str_insert(CStrBuf* self, size_t byte_index, CStr cstr)
{
  bool status = c_vec_insert_range((CVec*)self, byte_index, cstr.data, cstr.len);
  return status;
}

/// @brief fill the whole string with some data, @p data will be repeatedly
///        inserted for the whole capacity
/// @param self
/// @param cstr the input array of chars
/// @return true: success, false: failed (error)
bool c_str_fill(CStrBuf* self, CStr cstr)
{
  bool status = c_vec_fill_with_repeat((CVec*)self, cstr.data, cstr.len);
  return status;
}

/// @brief replace substring starting from @p index with @p range_size with @p
///        data with @p data_len
/// @note this could reallocate @ref CStrBuf::data
/// @param self
/// @param index in bytes not utf-8 characters
/// @param range_size in bytes not utf-8 characters
/// @param cstr the input array of chars
/// @return true: success, false: failed (error)
bool c_str_replace(CStrBuf* self, size_t index, size_t range_size, CStr cstr)
{
  bool status = c_vec_replace((CVec*)self, index, range_size, cstr.data, cstr.len);
  return status;
}

/// @brief concatenate @p str2 into @p str1
/// @note this could reallocate @ref CStrBuf::data of @p str1
/// @param str1
/// @param str2
/// @return true: success, false: failed (error)
bool c_str_concatenate(CStrBuf* str1, CStrBuf const* str2)
{
  TO_IMPL(str2)->len += 1;
  bool status = c_vec_concatenate((CVec*)str1, (CVec const*)str2);
  TO_IMPL(str2)->len -= 1;
  str1->data[TO_IMPL(str1)->len - 1] = '\0';

  return status;
}

/// @brief remove substring start from @p start_index with @p range_size
/// @note this could reallocate @ref CStrBuf::data
/// @param self
/// @param start_index
/// @param range_size
/// @return true: success, false: failed (error)
bool c_str_remove(CStrBuf* self, size_t start_index, size_t range_size)
{
  bool status = c_vec_remove_range((CVec*)self, start_index, range_size);
  return status;
}

/// @brief trim utf-8 whitespaces from start and end into @p out_slice
/// @param self
/// @return slice @ref CStr::data is not zero terminated
CStr c_str_trim(CStrBuf const* self)
{
  assert(self);

  /// from start
  size_t index = 0;
  {
    CIter  iter = c_str_iter((CStrBuf*)self);
    CStr   next_ch;
    size_t counter = 0;
    while ((c_str_iter_next(&iter, &next_ch))) {
      CChar ch = {.count = next_ch.len};
      memcpy(ch.data, next_ch.data, next_ch.len);
      if (c_str_is_whitespace(ch)) {
        index += ch.count;
      } else {
        break;
      }
      counter++;
    }

    // if the input was just whitespace(s)
    if (counter == TO_IMPL(self)->len) {
      return (CStr){.data = self->data + TO_IMPL(self)->len, .len = 0};
    }
  }

  /// from end
  size_t index_end = TO_IMPL(self)->len - 1;
  {
    CIter iter_end = c_str_iter((CStrBuf*)self);
    CStr  next_ch;
    while (c_str_iter_prev(&iter_end, &next_ch)) {
      CChar ch = {.count = next_ch.len};
      memcpy(ch.data, next_ch.data, next_ch.len);
      if (c_str_is_whitespace(ch)) {
        index_end -= ch.count;
      } else {
        break;
      }
    }
  }

  CStr str;
  c_str_get(self, index, index_end + 1 - index, &str);
  return str;
}

/// @brief trim utf-8 whitespaces from the start into @p out_slice
/// @param self
/// @return slice @ref CStr::data is not zero terminated
CStr c_str_trim_start(CStrBuf const* self)
{
  assert(self);

  CIter iter = c_str_iter((CStrBuf*)self);

  CStr   next_ch;
  size_t counter = 0;
  size_t index   = 0;
  while ((c_str_iter_next(&iter, &next_ch))) {
    CChar ch = {.count = next_ch.len};
    memcpy(ch.data, next_ch.data, next_ch.len);
    if (c_str_is_whitespace(ch)) {
      index += ch.count;
    } else {
      break;
    }
    counter++;
  }

  // if the input was just whitespace(s)
  if (counter == TO_IMPL(self)->len) {
    return (CStr){.data = self->data + TO_IMPL(self)->len, .len = 0};
  }

  CStr str;
  c_str_get(self, index, TO_IMPL(self)->len - index, &str);
  return str;
}

/// @brief trim utf-8 whitespaces from the end into @p out_slice
/// @param self
/// @return slice @ref CStr::data is not zero terminated
CStr c_str_trim_end(CStrBuf const* self)
{
  assert(self);

  CIter iter_end = c_str_iter((CStrBuf*)self);

  CStr   next_ch;
  size_t counter = 0;
  size_t index   = TO_IMPL(self)->len - 1;
  while ((c_str_iter_prev(&iter_end, &next_ch))) {
    CChar ch = {.count = next_ch.len};
    memcpy(ch.data, next_ch.data, next_ch.len);
    if (c_str_is_whitespace(ch)) {
      index -= ch.count;
    } else {
      break;
    }
    counter++;
  }

  // if the input was just whitespace(s)
  if (counter == TO_IMPL(self)->len) {
    return (CStr){.data = self->data + TO_IMPL(self)->len, .len = 0};
  }

  CStr str;
  c_str_get(self, 0, index + 1, &str);
  return str;
}

/// @brief this is an iter like function that you call multiple times until it
///        return false
/// @param self
/// @param delimeters this is a list of utf-8 characters
/// @param delimeters_len the length of this list
/// @param iter
/// @return slice
/// @return is_ok[true]: return none zero terminated substring
///         is_ok[false]: end of @ref CStrBuf::data, invalid iter or other
///         errors
bool c_str_iter_split(CIter* iter, CChar const delimeters[], size_t delimeters_len, CStr* out_str)
{
  assert(iter);

  if (!out_str) {
    c_error_set(C_ERROR_null_ptr);
    return false;
  }
  if (!iter) {
    c_error_set(C_ERROR_invalid_iterator);
    return false;
  }

  CStr   next_ch;
  bool   found          = false;
  size_t delimeter_size = 0;
  size_t total_len      = 0;
  while (!found && c_str_iter_next(iter, &next_ch)) {
    delimeter_size = 0;
    for (size_t iii = 0; iii < delimeters_len; ++iii) {
      if ((next_ch.len == delimeters[iii].count) &&
          (memcmp(next_ch.data, delimeters[iii].data, delimeters[iii].count) ==
           0)) {
        found          = true;
        delimeter_size = delimeters[iii].count;
        break;
      }
    }
    total_len += next_ch.len - delimeter_size;
  }

  out_str->data = (char*)iter->ptr - total_len;
  out_str->len  = total_len;
  return total_len ? true : false;
}

/// @brief same like @ref c_str_split, but the delimeters are whitespaces
/// @param self
/// @param iter
/// @return slice
/// @return is_ok[true]: return none zero terminated substring
///         is_ok[false]: end of @ref CStrBuf::data, invalid iter or other
///         errors
bool c_str_iter_split_by_whitespace(CIter* iter, CStr* out_str)
{
  bool status = c_str_iter_split(iter, c_whitespaces, sizeof(c_whitespaces) / sizeof(*c_whitespaces), out_str);
  return status;
}

/// @brief same like @ref c_str_split, but the delimeters are line endings
///        '\\n', '\\r' or '\\r\\n'
/// @param self
/// @param iter
/// @return slice
/// @return is_ok[true]: return none zero terminated substring
///         is_ok[false]: end of @ref CStrBuf::data, invalid iter or other
///         errors
bool c_str_iter_split_by_line(CIter* iter, CStr* out_str)
{
  CChar const line_separators[] = {
      {.data = {[0] = '\n'}, 1},
      {.data = {[0] = '\r'}, 1},
  };

  bool status = c_str_iter_split(
      iter, line_separators, sizeof(line_separators) / sizeof(*line_separators),
      out_str);

  /// handle the case of '\\r\\n'
  if (status) {
    if ((*((char*)iter->ptr) == '\r') && (*((char*)iter->ptr + 1) == '\n')) {
      iter->ptr = (uint8_t*)iter->ptr + 1;
    }
  }

  return status;
}

/// @brief convert utf-8 string to utf-16 vector
/// @note this works indepent of the current locale
/// @param self
/// @return vec_u16 this is a @ref CVec of uint16_t or NULL on error
CVec* c_str_to_utf16(CStrBuf const* self)
{
  assert(self);

  char* utf8 = self->data;

  CVec* u16vec = c_vec_create(sizeof(uint16_t), TO_IMPL(self)->allocator);
  if (!u16vec) goto ON_ERROR;

  while (*utf8) {
    CChar32 char32;
    bool    status = c_internal_str_utf8_to_utf32(utf8, &char32);
    if (!status) {
      c_error_set(C_ERROR_invalid_unicode);
      goto ON_ERROR;
    }

    CChar16 char16;
    status = c_internal_str_utf32_to_utf16(char32, &char16);
    if (!status) {
      c_error_set(C_ERROR_invalid_unicode);
      goto ON_ERROR;
    }

    for (size_t iii = 0; iii < char16.count; iii++) {
      bool status = c_vec_push(u16vec, &char16.data[iii]);
      if (!status) goto ON_ERROR;
    }

    utf8 += char32.count;
  }

  return u16vec;
ON_ERROR:
  c_vec_destroy(u16vec);
  return NULL;
}

/// @brief this is the opposite of @ref c_str_to_utf16
/// @note this works indepent of the current locale
/// @param vec_u16 this is a @ref CVec of uint16_t
/// @return new created @ref CStrBuf or NULL on error
CStrBuf* c_str_from_utf16(CVec const* vec_u16)
{
  assert(vec_u16);

  uint16_t* utf16_data = ((uint16_t*)(vec_u16->data));
  enum { UTF8_MAX_COUNT = 4 };
  size_t vec_len = c_vec_len(vec_u16);

  CStrBuf* str = c_str_create(TO_IMPL(vec_u16)->allocator);
  if (!str) return NULL;

  for (size_t i = 0; i < vec_len; i++) {
    uint16_t unit                = utf16_data[i];
    char     buf[UTF8_MAX_COUNT] = {0};

    if (unit >= 0xD800 && unit <= 0xDBFF) { // High surrogate
      if (i + 1 < vec_len && utf16_data[i + 1] >= 0xDC00 && utf16_data[i + 1] <= 0xDFFF) { // Valid surrogate pair
        uint16_t high      = unit;
        uint16_t low       = utf16_data[++i];
        uint32_t codepoint = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);

        // UTF-8 encoding for a supplementary character (4 bytes)
        buf[0]      = (uint8_t)((codepoint >> 18) | 0xF0);
        buf[1]      = (uint8_t)(((codepoint >> 12) & 0x3F) | 0x80);
        buf[2]      = (uint8_t)(((codepoint >> 6) & 0x3F) | 0x80);
        buf[3]      = (uint8_t)((codepoint & 0x3F) | 0x80);
        bool status = c_str_push(str, (CStr){buf, 4});
        if (!status) goto ON_ERROR;
      } else {
        c_error_set(C_ERROR_invalid_unicode);
        goto ON_ERROR; // Error: Unmatched high surrogate
      }
    } else if (unit >= 0xDC00 && unit <= 0xDFFF) { // Low surrogate without a preceding high surrogate
      c_error_set(C_ERROR_invalid_unicode);
      goto ON_ERROR; // Error: Invalid low surrogate
    } else { // BMP character (U+0000 to U+FFFF) UTF-8 encoding for a basic character (1 to 3 bytes)
      if (unit <= 0x7F) { // 1 byte (ASCII range)
        bool status = c_str_push(str, (CStr){(char*)&unit, 1});
        if (!status) goto ON_ERROR;
      } else if (unit <= 0x7FF) { // 2 bytes
        buf[0]      = (uint8_t)((unit >> 6) | 0xC0);
        buf[1]      = (uint8_t)((unit & 0x3F) | 0x80);
        bool status = c_str_push(str, (CStr){buf, 2});
        if (!status) goto ON_ERROR;
      } else { // 3 bytes
        buf[0]      = (uint8_t)((unit >> 12) | 0xE0);
        buf[1]      = (uint8_t)(((unit >> 6) & 0x3F) | 0x80);
        buf[2]      = (uint8_t)((unit & 0x3F) | 0x80);
        bool status = c_str_push(str, (CStr){buf, 3});
        if (!status) goto ON_ERROR;
      }
    }
  }

  return str;

ON_ERROR:
  c_str_destroy(str);
  return NULL;
}

/// @brief check if @ref CStrBuf::data is ascii
/// @param self
/// @return true/false
bool c_str_is_ascii(CStrBuf const* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    if (!isascii(self->data[iii])) return false;
  }

  return true;
}

/// @brief convert the @ref CStrBuf::data to ascii uppercase
/// @param self
void c_str_to_ascii_uppercase(CStrBuf* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    self->data[iii] = toupper(self->data[iii]);
  }
}

/// @brief convert the @ref CStrBuf::data to ascii lowercase
/// @param self
void c_str_to_ascii_lowercase(CStrBuf* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    self->data[iii] = tolower(self->data[iii]);
  }
}

/// @brief create a new iterator, this is a must call function if you want to
///        deal with @ref CStrBuf iterators
/// @param step_callback this could be NULL, if so, it will use
///                          @ref c_str_iter_default_step_callback
/// @return newly create @ref CIter
CIter c_str_iter(CStrBuf* self)
{
  CIter iter = c_iter(self->data, TO_IMPL(self)->len, sizeof(char));
  return iter;
}

bool c_str_iter_next(CIter* iter, CStr* out_ch)
{
  char* ptr;
  bool  status = c_iter_next(iter, (void**)&ptr);
  if (!status) return status;

  // Determine the number of bytes in the current character
  size_t        codepoint_size = 0;
  unsigned char ch             = *ptr;
  if ((ch & 0x80) == 0) {
    // Single byte character
    codepoint_size = 1;
  } else if ((ch & 0xE0) == 0xC0) {
    // Two byte character
    codepoint_size = 2;
  } else if ((ch & 0xF0) == 0xE0) {
    // Three byte character
    codepoint_size = 3;
  } else if ((ch & 0xF8) == 0xF0) {
    // Four byte character
    codepoint_size = 4;
  }
  // else if ((ch & 0xFC) == 0xF8)
  //   {
  //     // Five byte character (should not occur in valid UTF-8)
  //     codepoint_size = 5;
  //   }
  // else if ((ch & 0xFE) == 0xFC)
  //   {
  //     // Six byte character (should not occur in valid UTF-8)
  //     codepoint_size = 6;
  //   }
  else {
    c_error_set(C_ERROR_invalid_unicode);
    return false;
  }

  // Extract the remaining bytes of the character
  for (size_t iii = 1; iii < codepoint_size; ++iii) {
    if (((unsigned char)(((char*)iter->ptr)[iii]) & 0xC0) != 0x80) {
      c_error_set(C_ERROR_invalid_unicode);
      return false;
    }
  }

  if (out_ch) *out_ch = (CStr){.data = iter->ptr, .len = codepoint_size};
  iter->ptr = (uint8_t*)iter->ptr + codepoint_size - sizeof(char);
  return true;
}

bool c_str_iter_prev(CIter* iter, CStr* out_ch)
{
  // get the first valid codepoint
  char* ptr = NULL;
#define is_utf8_start_byte(ch) (((ch) & 0xC0) != 0x80)
  while (c_iter_prev(iter, (void**)&ptr) && !is_utf8_start_byte(*ptr)) {}
  if (!ptr) return false;

  // Determine the number of bytes in the current character
  size_t        codepoint_size = 0;
  unsigned char ch             = *(char*)iter->ptr;
  if ((ch & 0x80) == 0) {
    // Single byte character
    codepoint_size = 1;
  } else if ((ch & 0xE0) == 0xC0) {
    // Two byte character
    codepoint_size = 2;
  } else if ((ch & 0xF0) == 0xE0) {
    // Three byte character
    codepoint_size = 3;
  } else if ((ch & 0xF8) == 0xF0) {
    // Four byte character
    codepoint_size = 4;
  }
  // else if ((ch & 0xFC) == 0xF8)
  //   {
  //     // Five byte character (should not occur in valid UTF-8)
  //     codepoint_size = 5;
  //   }
  // else if ((ch & 0xFE) == 0xFC)
  //   {
  //     // Six byte character (should not occur in valid UTF-8)
  //     codepoint_size = 6;
  //   }
  else {
    c_error_set(C_ERROR_invalid_unicode);
    return false;
  }

  // Extract the remaining bytes of the character
  for (size_t iii = 1; iii < codepoint_size; ++iii) {
    if (((unsigned char)(((char*)iter->ptr)[iii]) & 0xC0) != 0x80) {
      c_error_set(C_ERROR_invalid_unicode);
      return false;
    }
  }

  if (out_ch) *out_ch = (CStr){.data = iter->ptr, .len = codepoint_size};
  return true;
}

bool c_str_iter_nth(CIter* iter, size_t index, CStr* out_ch)
{
  size_t counter = 0;
  while (c_str_iter_next(iter, out_ch)) {
    if (counter == index) return true;
  }

  return false;
}

bool c_str_iter_peek(CIter const* iter, CStr* out_ch)
{
  CIter tmp    = *iter;
  bool  status = c_str_iter_next(&tmp, out_ch);

  return status;
}

CStr c_str_iter_first(CIter* iter)
{
  iter->ptr = (uint8_t*)iter->data - 1;

  CStr out_ch;
  c_str_iter_next(iter, &out_ch);
  return out_ch;
}

CStr c_str_iter_last(CIter* iter)
{
  iter->ptr = (uint8_t*)iter->data + iter->data_size;

  CStr out_ch;
  c_str_iter_prev(iter, &out_ch);
  return out_ch;
}

/// @brief create new @ref CStrBuf containes reversed utf-8 characters ()
///        orignal[ch1, ch2, ch3, ... chN] => reversed[chN, ... ch3, ch2, ch1]
/// @param self
/// @return new allocated @ref CStrBuf or NULL on error
CStrBuf* c_str_reverse(CStrBuf const* self)
{
  assert(self);

  CStrBuf* rev_str = c_str_create_with_capacity(
      TO_IMPL(self)->len + 1, TO_IMPL(self)->allocator, false);
  if (!rev_str) return NULL;

  CIter iter_end = c_str_iter((CStrBuf*)self);

  CStr ch;
  while (c_str_iter_prev(&iter_end, &ch)) {
    c_str_push(rev_str, ch);
  }

  return rev_str;
}

/// @brief set length to zero
/// @param self
void c_str_clear(CStrBuf* self)
{
  c_vec_clear((CVec*)self);
}

/// @brief this is a safer wrapper around sprintf
/// @param self
/// @param index this is byte wise not utf-8 characters
/// @param format_len
/// @param format
/// @return true: success, false: error
bool c_str_format(CStrBuf* self, size_t index, size_t format_len,
                  char const* format, ...)
{
  va_list va;
  va_start(va, format);
  bool status = c_str_format_va(self, index, format_len, format, va);
  va_end(va);

  return status;
}

/// @brief same like @ref c_str_format
/// @param self
/// @param index this is byte wise not utf-8 characters
/// @param format_len
/// @param format
/// @param va vadriac list
/// @return true: success, false: error
bool c_str_format_va(CStrBuf* self, size_t index, size_t format_len,
                     char const* format, va_list va)
{
  assert(self);

  if (index > TO_IMPL(self)->len) {
    c_error_set(C_ERROR_invalid_index);
    return false;
  }
  if (format[format_len] != '\0') {
    c_error_set(C_ERROR_none_terminated_raw_str);
    return false;
  }

  va_list va_tmp;
  va_copy(va_tmp, va);
  int needed_len = vsnprintf(NULL, 0, format, va_tmp);
  if (needed_len < 0) {
    c_error_set(C_ERROR_invalid_format);
    return false;
  }

  if ((size_t)needed_len >= GET_CAPACITY(self) - index) {
    bool status = c_str_set_capacity(self, GET_CAPACITY(self) + needed_len + 1);
    if (!status) return status;
  }

  needed_len = vsnprintf(self->data + index, GET_CAPACITY(self) - index, format, va);
  if (needed_len < 0) {
    c_error_set(C_ERROR_invalid_format);
    return false;
  }

  TO_IMPL(self)->len += needed_len;
  self->data[TO_IMPL(self)->len] = '\0';

  return true;
}

/// @brief compare @p str2 with @p str2
/// @param str1
/// @param str2
/// @return same like strcmp
int c_str_compare(CStrBuf* str1, CStrBuf* str2)
{
  assert(str1);
  assert(str2);

  size_t min_size = (TO_IMPL(str1)->len <= TO_IMPL(str2)->len)
                        ? TO_IMPL(str1)->len
                        : TO_IMPL(str2)->len;
  return strncmp(str1->data, str2->data, min_size);
}

/// @brief check if @p data is utf-8 whitespace
/// @param ch utf8 char
/// @return true/false
bool c_str_is_whitespace(CChar ch)
{
  for (size_t iii = 0; iii < sizeof(c_whitespaces) / sizeof(*c_whitespaces);
       ++iii) {
    if (memcmp(c_whitespaces[iii].data, ch.data, ch.count) == 0)
      return true;
  }

  return false;
}

/// @brief check if @p data is ascii whitespace
/// @param ch
/// @return true/false
bool c_str_is_ascii_whitespace(char ch)
{
  return memchr(c_ascii_whitespaces, ch, sizeof(c_ascii_whitespaces) - 1)
             ? true
             : false;
}

/// @brief convert this to @ref CVec
/// @note this will not create new memory, so the new created @ref CVec will use
///       the same memory as @ref CStrBuf
/// @param self
/// @return @ref CVec object
CVec* c_str_to_vec(CStrBuf* self)
{
  assert(self);

  return (CVec*)self;
}

/// @brief convert @ref CStrBuf to @ref CStr
/// @note this will not create a new library, and the resulted @ref CStr will
///       act like a reference to @p self
/// @param self
/// @return @ref CStr object
CStr c_cstrbuf_to_cstr(CStrBuf* self)
{
  assert(self);

  CStr str = {.data = self->data, .len = c_str_len(self)};
  return str;
}

/// @brief convert @ref CStr to @ref CStrBuf
/// @param cstr
/// @param allocator
/// @return new allocated CStrBuf object or NULL on error
CStrBuf* c_cstr_to_cstrbuf(CStr cstr, CAllocator* allocator)
{
  CStrBuf* str = c_str_create_from_raw(cstr, true, allocator);
  return str;
}

/// @brief free the memory of the allocated @ref CStrBuf
/// @param self
void c_str_destroy(CStrBuf* self)
{
  c_vec_destroy((CVec*)self);
}

/******************************************************************************/
/*                                  Internal                                  */
/******************************************************************************/

bool c_internal_str_utf8_to_utf32(const char* utf8, CChar32* out_ch)
{
  const unsigned char* bytes = (const unsigned char*)utf8;
  if (bytes[0] < 0x80) {
    out_ch->data  = bytes[0];
    out_ch->count = 1; // 1-byte UTF-8 sequence
  } else if ((bytes[0] & 0xE0) == 0xC0) {
    out_ch->data  = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
    out_ch->count = 2; // 2-byte UTF-8 sequence
  } else if ((bytes[0] & 0xF0) == 0xE0) {
    out_ch->data = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) |
                   (bytes[2] & 0x3F);
    out_ch->count = 3; // 3-byte UTF-8 sequence
  } else if ((bytes[0] & 0xF8) == 0xF0) {
    out_ch->data = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) |
                   ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
    out_ch->count = 4; // 4-byte UTF-8 sequence
  } else {
    return false; // Invalid UTF-8 sequence
  }

  return true;
}

// Function to encode a Unicode code point into UTF-16
bool c_internal_str_utf32_to_utf16(CChar32 utf32, CChar16* out_ch)
{
  if (utf32.data < 0x10000) {
    out_ch->data[0] = (uint16_t)utf32.data;
    out_ch->count   = 1; // 1 UTF-16 code unit
  } else if (utf32.data <= 0x10FFFF) {
    utf32.data -= 0x10000;
    out_ch->data[0] = (uint16_t)((utf32.data >> 10) + 0xD800); // High surrogate
    out_ch->data[1] = (uint16_t)((utf32.data & 0x3FF) + 0xDC00); // Low surrogate
    out_ch->count   = 2; // 2 UTF-16 code units
  } else {
    return false; // Invalid code point
  }

  return true;
}

// void
// c_internal_str_unicode_to_utf8(int unicode, Utf8Char* utf8)
// {
//   if (unicode <= 0x7F) {
//     // 1-byte UTF-8
//     utf8->data[0] = (char)unicode;
//     utf8->size    = 1;
//   } else if (unicode <= 0x7FF) {
//     // 2-byte UTF-8
//     utf8->data[0] = (char)(0xC0 | ((unicode >> 6) & 0x1F));
//     utf8->data[1] = (char)(0x80 | (unicode & 0x3F));
//     utf8->size    = 2;
//   } else if (unicode <= 0xFFFF) {
//     // 3-byte UTF-8
//     utf8->data[0] = (char)(0xE0 | ((unicode >> 12) & 0x0F));
//     utf8->data[1] = (char)(0x80 | ((unicode >> 6) & 0x3F));
//     utf8->data[2] = (char)(0x80 | (unicode & 0x3F));
//     utf8->size    = 3;
//   } else if (unicode <= 0x10FFFF) {
//     // 4-byte UTF-8
//     utf8->data[0] = (char)(0xF0 | ((unicode >> 18) & 0x07));
//     utf8->data[1] = (char)(0x80 | ((unicode >> 12) & 0x3F));
//     utf8->data[2] = (char)(0x80 | ((unicode >> 6) & 0x3F));
//     utf8->data[3] = (char)(0x80 | (unicode & 0x3F));
//     utf8->size    = 4;
//   } else {
//     // Invalid Unicode code point
//     utf8->size = 0;
//   }
// }

// size_t
// c_utf16_to_codepoints(const uint16_t* utf16,
//                     size_t          utf16_len,
//                     uint32_t*       codepoints,
//                     size_t          codepoints_capacity)
// {
//   size_t codepoint_count = 0;

//   for (size_t i = 0; i < utf16_len; i++) {
//     uint16_t unit = utf16[i];

//     if (unit >= 0xD800 && unit <= 0xDBFF) { // High surrogate
//       if (i + 1 < utf16_len && utf16[i + 1] >= 0xDC00
//           && utf16[i + 1] <= 0xDFFF) { // Valid surrogate pair
//         uint16_t high      = unit;
//         uint16_t low       = utf16[++i];
//         uint32_t codepoint = 0x10000 + ((high - 0xD800) << 10) + (low -
//         0xDC00);

//         if (codepoint_count < codepoints_capacity) {
//           codepoints[codepoint_count++] = codepoint;
//         } else {
//           fprintf(stderr, "Error: Codepoints buffer overflow\n");
//           return 0;
//         }
//       } else {
//         fprintf(stderr, "Error: Unmatched high surrogate at index %zu\n", i);
//         return 0;
//       }
//     } else if (unit >= 0xDC00 && unit <= 0xDFFF) { // Low surrogate without a
//                                                    // preceding high
//                                                    surrogate
//       fprintf(stderr, "Error: Unmatched low surrogate at index %zu\n", i);
//       return 0;
//     } else { // BMP character
//       if (codepoint_count < codepoints_capacity) {
//         codepoints[codepoint_count++] = unit;
//       } else {
//         fprintf(stderr, "Error: Codepoints buffer overflow\n");
//         return 0;
//       }
//     }
//   }

//   return codepoint_count;
// }
