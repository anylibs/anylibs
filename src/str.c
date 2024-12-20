/**
 * @file str.h
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

#include "anylibs/str.h"
#include "anylibs/vec.h"
#include "internal/vec.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
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
#define FROM_IMPL(impl) ((CString*)(impl))
#define TO_BYTES(str, units) ((units) * TO_IMPL(str)->element_size)
#define TO_UNITS(str, bytes) ((bytes) / TO_IMPL(str)->element_size)
#define GET_CAPACITY(str)                                                      \
  (TO_IMPL(str)->raw_capacity > 0 ? TO_IMPL(str)->raw_capacity                 \
                                  : c_allocator_mem_size(TO_IMPL(str)->data))

/// @brief ASCII whitespaces
static unsigned char const c_ascii_whitespaces[] = {
    ' ',  // space
    '\t', // horizontal tab
    '\n', // newline
    '\v', // vertical tab
    '\f', // form feed
    '\r'  // carriage return
};

/// @brief UTF-8 whitespaces
static CChar c_whitespaces[] = {
    CCHAR("\t"),     // character tabulation
    CCHAR("\n"),     // line feed
    CCHAR("\v"),     // line tabulation
    CCHAR("\f"),     // form feed
    CCHAR("\r"),     // carriage return
    CCHAR(" "),      // space
    CCHAR(""),     // next line
    CCHAR("\u00A0"), // no-break space
    CCHAR("\u1680"), // ogham space mark
    CCHAR("\u2000"), // en quad
    CCHAR("\u2001"), // em quad
    CCHAR("\u2002"), // en space
    CCHAR("\u2003"), // em space
    CCHAR("\u2004"), // three-per-em space
    CCHAR("\u2005"), // four-per-em space
    CCHAR("\u2006"), // six-per-em space
    CCHAR("\u2007"), // figure space
    CCHAR("\u2008"), // punctuation space
    CCHAR("\u2009"), // thin space
    CCHAR("\u200A"), // hair space
    CCHAR("\u2028"), // line separator
    CCHAR("\u2029"), // paragraph separator
    CCHAR("\u202F"), // narrow no-break space
    CCHAR("\u205F"), // medium mathematical space
    CCHAR("\u3000"), // ideographic space
    CCHAR("\u180E"), // mongolian vowel separator
    CCHAR("\u200B"), // zero width space
    CCHAR("\u200C"), // zero width non-joiner
    CCHAR("\u200D"), // zero width joiner
    CCHAR("\u2060"), // word joiner
    CCHAR("\uFEFF")  // zero width non-breaking space
};

typedef struct CChar16 {
  uint16_t data[2];
  size_t   count;
} CChar16;

typedef struct CChar32 {
  uint32_t data;
  size_t   count;
} CChar32;

typedef struct CChar16Result {
  CChar16 ch;
  bool    is_ok;
} CChar16Result;

typedef struct CChar32Result {
  CChar32 ch;
  bool    is_ok;
} CChar32Result;

static CChar32Result c_internal_str_utf8_to_utf32(const char* utf8);
static CChar16Result c_internal_str_utf32_to_utf16(uint32_t codepoint);

/// @brief create @ref CString object
/// @param allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @return new allocated @ref CString or NULL on error
CString*
c_str_create(CAllocator* allocator)
{
  CString* str = c_str_create_with_capacity(1, allocator, false);
  return str;
}

/// @brief create @ref CString object from array of chars
/// @param cstr the input array of chars
/// @param should_copy true: will copy the data, false: will just reference
///                        the data
/// @param allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @return new allocated @ref CString or NULL on error
CString*
c_str_create_from_raw(CStr cstr, bool should_copy, CAllocator* allocator)
{
  if (!allocator) allocator = c_allocator_default();

  if (!should_copy) {
    CVecImpl* impl
        = c_allocator_alloc(allocator, c_allocator_alignas(CVecImpl, 1), false);
    if (!impl) return NULL;

    impl->data         = cstr.data;
    impl->raw_capacity = cstr.len;
    impl->len          = cstr.len;
    impl->allocator    = allocator;
    impl->element_size = sizeof(char);

    return FROM_IMPL(impl);
  } else {
    CString* str = c_str_create_with_capacity(cstr.len + 1, allocator, false);
    if (!str) return NULL;
    memcpy((str)->data, cstr.data, cstr.len);
    str->data[cstr.len] = '\0';

    TO_IMPL(str)->len = cstr.len;

    return str;
  }
}

/// @brief create new empty @ref CString object with capacity
/// @param capacity
/// @param allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @param zero_initialized
/// @return new allocated @ref CString or NULL on error
CString*
c_str_create_with_capacity(size_t      capacity,
                           CAllocator* allocator,
                           bool        zero_initialized)
{
  CString* str = (CString*)c_vec_create_with_capacity(
      sizeof(char), capacity, zero_initialized, allocator);

  return str;
}

/// @brief clone another @ref CString object
/// @param self
/// @param should_shrink_clone true: shrink the clone capacity to @ref
///                                CString len
/// @return new allocated @ref CString or NULL on error
CString*
c_str_clone(CString const* self, bool should_shrink_clone)
{
  CString* str = (CString*)c_vec_clone((CVec const*)self, should_shrink_clone);
  return str;
}

/// @brief return if @ref CString is empty
/// @note this will also return false on failure if any
/// @param self
/// @return
bool
c_str_is_empty(CString const* self)
{
  return c_vec_is_empty((CVec const*)self);
}

/// @brief return the number of utf8 characters
/// @param self
/// @return utf-8 characters count
size_t
c_str_count(CString const* self)
{
  assert(self);

  CStringIter iter = c_str_iter(NULL);
  while (c_str_iter_next(self, &iter).is_ok) {}
  if (iter.base.counter < TO_IMPL(self)->len) return C_ERROR_invalid_unicode;

  return iter.ch_counter;
}

/// @brief return @ref CString length in bytes
/// @param self
/// @return length in bytes
size_t
c_str_len(CString const* self)
{
  return c_vec_len((CVec const*)self);
}

/// @brief set new length
/// @note this could reallocate @ref CString::data
/// @param self
/// @param new_len
void
c_str_set_len(CString* self, size_t new_len)
{
  // 1 is for the trailing 0
  c_vec_set_len((CVec*)self, new_len + 1);
  ((char*)TO_IMPL(self)->data)[TO_IMPL(self)->len - 1] = '\0';
  TO_IMPL(self)->len -= 1;
}

/// @brief get capacity in bytes
/// @param self
/// @return capacity
size_t
c_str_capacity(CString const* self)
{
  return c_vec_capacity((CVec const*)self);
}

/// @brief return the empty remaining capacity in bytes
/// @param self
/// @return spare_capacity
size_t
c_str_spare_capacity(CString const* self)
{
  return c_vec_spare_capacity((CVec const*)self);
}

/// @brief set new capacity
/// @note this could reallocate @ref CString::data
/// @param self
/// @param new_capacity
/// @return true: success, false: failed (error)
bool
c_str_set_capacity(CString* self, size_t new_capacity)
{
  bool is_shrinking = GET_CAPACITY(self) > new_capacity;
  // 1 is for the trailing 0
  bool status = c_vec_set_capacity((CVec*)self, new_capacity + 1);
  if (status && is_shrinking) {
    ((char*)TO_IMPL(self)->data)[TO_IMPL(self)->len - 1] = '\0';
  }

  return status;
}

/// @brief set capacity to the current length
/// @note this could reallocate @ref CString::data
/// @param self
/// @return true: success, false: failed (error)
bool
c_str_shrink_to_fit(CString* self)
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
CStrResult
c_str_get(CString const* self, size_t start_index, size_t range_size)
{
  assert(self);

  CStrResult result = {0};

  if (start_index >= TO_IMPL(self)->len) return result;
  result.str.data = self->data + start_index;
  result.str.len  = range_size;
  result.is_ok    = true;

  return result;
}

/// @brief find utf8 char
/// @param self
/// @param ch a struct that you need to fill it manually
/// @return is_ok[true]: return @ref CStr that has a pointer to character at @p
///         start_index, is_ok[false]: not found (or error happened)
CStrResult
c_str_find(CString const* self, CChar ch)
{
  CStringIter iter   = c_str_iter(NULL);
  CStrResult  result = c_str_find_by_iter(self, ch, &iter);

  return result;
}

/// @brief same like @ref c_str_find by it will use external iter
///        so you can for example use @ref c_str_iter_nth to start from some
///        index
/// @param self
/// @param ch a struct that you need to fill it manually
/// @param iter
/// @return is_ok[true]: return @ref CStr that has a pointer to character at @p
///         start_index, is_ok[false]: not found (or error happened)
CStrResult
c_str_find_by_iter(CString const* self, CChar ch, CStringIter* iter)
{
  assert(self);

  CStrResult result = {0};
  if (!iter) return result;
  if (iter->base.counter >= TO_IMPL(self)->len) return result;

  CCharResult ch_res;
  while ((ch_res = c_str_iter_next(self, iter)).is_ok) {
    if (memcmp(ch_res.ch.data, ch.data, ch.size) == 0) {
      result.str.data = ch_res.ch.data;
      result.str.len  = TO_IMPL(self)->len - (ch_res.ch.data - self->data);
      result.is_ok    = true;
      return result;
    }
  }

  return result;
}

/// @brief check if @ref CString starts with @p data
/// @param self
/// @param cstr the input array of chars
/// @return true/false
bool
c_str_starts_with(CString const* self, CStr cstr)
{
  return c_vec_starts_with((CVec const*)self, cstr.data, cstr.len,
                           (int (*)(void const*, void const*))strcmp);
}

/// @brief check if @ref CString ends with @p data
/// @param self
/// @param cstr the input array of chars
/// @return true/false
bool
c_str_ends_with(CString const* self, CStr cstr)
{
  return c_vec_ends_with((CVec const*)self, cstr.data, cstr.len,
                         (int (*)(void const*, void const*))strcmp);
}

/// @brief push some data to the end
/// @param self
/// @param cstr the input array of chars
/// @return true: success, false: failed (error)
bool
c_str_push(CString* self, CStr cstr)
{
  return c_vec_push_range((CVec*)self, cstr.data, cstr.len);
}

/// @brief pop last character from the end
/// @note this could reallocate @ref CString::data
/// @param self
/// @return is_ok[true]: return @ref CCharOwned
///         is_ok[false]: failed (or error happened)
CCharOwnedResult
c_str_pop(CString* self)
{
  assert(self);

  CCharOwnedResult result = {0};
  if (TO_IMPL(self)->len == 0) return result;

  CStringIter iter = c_str_iter(NULL);
  c_str_iter_rev(self, &iter);
  CCharResult ch_result = c_str_iter_next(self, &iter);
  if (!ch_result.is_ok) return result;

  memcpy(result.ch.data, ch_result.ch.data, ch_result.ch.size);
  result.is_ok = true;

  TO_IMPL(self)->len -= ch_result.ch.size;
  self->data[TO_IMPL(self)->len] = '\0';

  return result;
}

/// @brief insert @p data at @p index
/// @note @p index is in bytes not utf8 characters
/// @note this could reallocate @ref CString::data
/// @param self
/// @param byte_index
/// @param cstr the input array of chars
/// @return true: success, false: failed (error)
bool
c_str_insert(CString* self, size_t byte_index, CStr cstr)
{
  bool status
      = c_vec_insert_range((CVec*)self, byte_index, cstr.data, cstr.len);
  return status;
}

/// @brief fill the whole string with some data, @p data will be repeatedly
///        inserted for the whole capacity
/// @param self
/// @param cstr the input array of chars
/// @return true: success, false: failed (error)
bool
c_str_fill(CString* self, CStr cstr)
{
  bool status = c_vec_fill_with_repeat((CVec*)self, cstr.data, cstr.len);
  return status;
}

/// @brief replace substring starting from @p index with @p range_size with @p
///        data with @p data_len
/// @note this could reallocate @ref CString::data
/// @param self
/// @param index in bytes not utf-8 characters
/// @param range_size in bytes not utf-8 characters
/// @param cstr the input array of chars
/// @return true: success, false: failed (error)
bool
c_str_replace(CString* self, size_t index, size_t range_size, CStr cstr)
{
  bool status
      = c_vec_replace((CVec*)self, index, range_size, cstr.data, cstr.len);
  return status;
}

/// @brief concatenate @p str2 into @p str1
/// @note this could reallocate @ref CString::data of @p str1
/// @param str1
/// @param str2
/// @return true: success, false: failed (error)
bool
c_str_concatenate(CString* str1, CString const* str2)
{
  TO_IMPL(str2)->len += 1;
  bool status = c_vec_concatenate((CVec*)str1, (CVec const*)str2);
  TO_IMPL(str2)->len -= 1;
  str1->data[TO_IMPL(str1)->len - 1] = '\0';

  return status;
}

/// @brief remove substring start from @p start_index with @p range_size
/// @note this could reallocate @ref CString::data
/// @param self
/// @param start_index
/// @param range_size
/// @return true: success, false: failed (error)
bool
c_str_remove(CString* self, size_t start_index, size_t range_size)
{
  bool status = c_vec_remove_range((CVec*)self, start_index, range_size);
  return status;
}

/// @brief trim utf-8 whitespaces from start and end into @p out_slice
/// @param self
/// @return slice @ref CStr::data is not zero terminated
CStr
c_str_trim(CString const* self)
{
  assert(self);

  /// from start
  size_t index = 0;
  {
    CStringIter iter   = c_str_iter(NULL);
    CCharResult result = {0};
    while ((result = c_str_iter_next(self, &iter)).is_ok) {
      if (c_str_is_whitespace(result.ch)) {
        index += result.ch.size;
      } else {
        break;
      }
    }
  }

  /// from end
  size_t index_end = TO_IMPL(self)->len - 1;
  {
    CStringIter iter_end = c_str_iter(NULL);
    c_str_iter_rev(self, &iter_end);

    CCharResult result = {0};
    while ((result = c_str_iter_next(self, &iter_end)).is_ok) {
      if (c_str_is_whitespace(result.ch)) {
        index_end -= result.ch.size;
      } else {
        break;
      }
    }
  }

  CStr str = c_str_get(self, index, index_end + 1 - index).str;
  return str;
}

/// @brief trim utf-8 whitespaces from the start into @p out_slice
/// @param self
/// @return slice @ref CStr::data is not zero terminated
CStr
c_str_trim_start(CString const* self)
{
  assert(self);

  CStringIter iter = c_str_iter(NULL);

  CCharResult result = {0};
  size_t      index  = 0;
  while ((result = c_str_iter_next(self, &iter)).is_ok) {
    if (c_str_is_whitespace(result.ch)) {
      index += result.ch.size;
    } else {
      break;
    }
  }

  CStr str = c_str_get(self, index, TO_IMPL(self)->len - index).str;
  return str;
}

/// @brief trim utf-8 whitespaces from the end into @p out_slice
/// @param self
/// @return slice @ref CStr::data is not zero terminated
CStr
c_str_trim_end(CString const* self)
{
  assert(self);

  CStringIter iter_end = c_str_iter(NULL);
  c_str_iter_rev(self, &iter_end);

  CCharResult result = {0};
  size_t      index  = TO_IMPL(self)->len - 1;
  while ((result = c_str_iter_next(self, &iter_end)).is_ok) {
    if (c_str_is_whitespace(result.ch)) {
      index -= result.ch.size;
    } else {
      break;
    }
  }

  CStr str = c_str_get(self, 0, index + 1).str;
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
///         is_ok[false]: end of @ref CString::data, invalid iter or other
///         errors
CStrResult
c_str_split(CString const* self,
            CChar const    delimeters[],
            size_t         delimeters_len,
            CStringIter*   iter)
{
  assert(self);

  CStrResult result = {0};

  if (!iter) return result;
  if (iter->base.counter >= TO_IMPL(self)->len) return result;

  result.str.data = self->data + iter->base.counter;

  size_t      old_base_counter = iter->base.counter;
  CCharResult ch_res;
  bool        found          = false;
  size_t      delimeter_size = 0;
  while (!found && (ch_res = c_str_iter_next(self, iter)).is_ok) {
    delimeter_size = 0;
    for (size_t iii = 0; iii < delimeters_len; ++iii) {
      if ((ch_res.ch.size == delimeters[iii].size)
          && (memcmp(ch_res.ch.data, delimeters[iii].data, delimeters[iii].size)
              == 0)) {
        found          = true;
        delimeter_size = delimeters[iii].size;
        break;
      }
    }
  }

  result.str.len = iter->base.counter - old_base_counter - delimeter_size;
  result.is_ok   = true;

  return result;
}

/// @brief same like @ref c_str_split, but the delimeters are whitespaces
/// @param self
/// @param iter
/// @return slice
/// @return is_ok[true]: return none zero terminated substring
///         is_ok[false]: end of @ref CString::data, invalid iter or other
///         errors
CStrResult
c_str_split_by_whitespace(CString const* self, CStringIter* iter)
{
  CStrResult result
      = c_str_split(self, c_whitespaces,
                    sizeof(c_whitespaces) / sizeof(*c_whitespaces), iter);
  return result;
}

/// @brief same like @ref c_str_split, but the delimeters are line endings
///        '\\n', '\\r' or '\\r\\n'
/// @param self
/// @param iter
/// @return slice
/// @return is_ok[true]: return none zero terminated substring
///         is_ok[false]: end of @ref CString::data, invalid iter or other
///         errors
CStrResult
c_str_split_by_line(CString const* self, CStringIter* iter)
{
  CChar const line_separators[] = {
      CCHAR("\n"),
      CCHAR("\r"),
  };

  CStrResult result
      = c_str_split(self, line_separators,
                    sizeof(line_separators) / sizeof(*line_separators), iter);

  /// handle the case of '\\r\\n'
  if (result.is_ok) {
    if ((self->data[iter->base.counter - 1] == '\r')
        && (self->data[iter->base.counter] == '\n')) {
      iter->base.counter += 1;
      iter->ch_counter += 1;
    }
  }

  return result;
}

/// @brief convert utf-8 string to utf-16 vector
/// @note this works indepent of the current locale
/// @param self
/// @return vec_u16 this is a @ref CVec of uint16_t or NULL on error
CVec*
c_str_to_utf16(CString const* self)
{
  assert(self);

  char* utf8 = self->data;

  CVec* u16vec = c_vec_create(sizeof(uint16_t), TO_IMPL(self)->allocator);
  if (!u16vec) goto ON_ERROR;

  while (*utf8) {
    CChar32Result char32_result = c_internal_str_utf8_to_utf32(utf8);
    if (!char32_result.is_ok) goto ON_ERROR;

    CChar16Result char16_result
        = c_internal_str_utf32_to_utf16(char32_result.ch.data);
    if (!char16_result.is_ok) goto ON_ERROR;

    for (size_t iii = 0; iii < char16_result.ch.count; iii++) {
      bool status = c_vec_push(u16vec, &char16_result.ch.data[iii]);
      if (!status) goto ON_ERROR;
    }

    utf8 += char32_result.ch.count;
  }

  return u16vec;
ON_ERROR:
  c_vec_destroy(u16vec);
  return NULL;
}

/// @brief this is the opposite of @ref c_str_to_utf16
/// @note this works indepent of the current locale
/// @param vec_u16 this is a @ref CVec of uint16_t
/// @return new created @ref CString or NULL on error
CString*
c_str_from_utf16(CVec const* vec_u16)
{
  assert(vec_u16);

  uint16_t* utf16_data = ((uint16_t*)(vec_u16->data));
  enum { UTF8_MAX_COUNT = 4 };
  size_t vec_len = c_vec_len(vec_u16);

  CString* str = c_str_create(TO_IMPL(vec_u16)->allocator);
  if (!str) return NULL;

  for (size_t i = 0; i < vec_len; i++) {
    uint16_t unit                = utf16_data[i];
    char     buf[UTF8_MAX_COUNT] = {0};

    if (unit >= 0xD800 && unit <= 0xDBFF) { // High surrogate
      if (i + 1 < vec_len && utf16_data[i + 1] >= 0xDC00
          && utf16_data[i + 1] <= 0xDFFF) { // Valid surrogate pair
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
        goto ON_ERROR; // Error: Unmatched high surrogate
      }
    } else if (unit >= 0xDC00 && unit <= 0xDFFF) { // Low surrogate without a
                                                   // preceding high surrogate
      goto ON_ERROR; // Error: Invalid low surrogate
    } else {         // BMP character (U+0000 to U+FFFF)
      // UTF-8 encoding for a basic character (1 to 3 bytes)
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

/// @brief check if @ref CString::data is ascii
/// @param self
/// @return true/false
bool
c_str_is_ascii(CString const* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    if (!isascii(self->data[iii])) return false;
  }

  return true;
}

/// @brief convert the @ref CString::data to ascii uppercase
/// @param self
void
c_str_to_ascii_uppercase(CString* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    self->data[iii] = toupper(self->data[iii]);
  }
}

/// @brief convert the @ref CString::data to ascii lowercase
/// @param self
void
c_str_to_ascii_lowercase(CString* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    self->data[iii] = tolower(self->data[iii]);
  }
}

/// @brief create a new iterator, this is a must call function if you want to
///        deal with @ref CString iterators
/// @param step_callback this could be NULL, if so, it will use
///                          @ref c_str_iter_default_step_callback
/// @return newly create @ref CStringIter
CStringIter
c_str_iter(CStringIterStepCallback step_callback)
{
  CStringIter iter  = {0};
  iter.base         = c_iter(sizeof(char),
                     step_callback
                                 ? (CIterStepCallback)step_callback
                                 : (CIterStepCallback)c_str_iter_default_step_callback);
  iter.base.counter = 0;

  return iter;
}

/// @brief this is the default step callback for @ref c_str_iter if @p
///        step_callback was NULL.
///        this public for custom step function (filter like) that could also
///        use this default step function
/// @param iter
/// @param data
/// @param data_len in bytes
/// @return is_ok[true]: char pointer of none zero terminated utf-8 character
///         is_ok[false]: could not step more (reach the end) or error happened
CIterElementResult
c_str_iter_default_step_callback(CStringIter* iter, char* data, size_t data_len)
{
  CIterElementResult result = {0};

  if (iter->base.counter > data_len) return result;
  if (!iter->base.is_reversed && (iter->base.counter == data_len))
    return result;

  if (iter->base.is_reversed) {
    char* ptr = data + iter->base.counter;

    // get the beginning of a utf8 char
#define is_utf8_start_byte(ch) (((ch) & 0xC0) != 0x80)
    while ((--ptr >= data) && !is_utf8_start_byte(*ptr)) {}

    if (ptr < data) return result;
    iter->base.counter = ptr - data;
  }

  // Determine the number of bytes in the current character
  size_t        codepoint_size = 0;
  size_t        byte_count     = iter->base.counter;
  unsigned char ch             = ((char*)data)[byte_count];
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
    return result;
  }

  byte_count++;

  // Extract the remaining bytes of the character
  for (size_t iii = 1; iii < codepoint_size; ++iii) {
    if (((unsigned char)(((char*)data)[byte_count]) & 0xC0) != 0x80) {
      return result;
    }
    byte_count++;
  }

  iter->ch_counter++;

  result.element        = data + iter->base.counter;
  iter->current_ch_size = codepoint_size;

  if (!iter->base.is_reversed) iter->base.counter += codepoint_size;

  result.is_ok = true;
  return result;
}

/// @brief reverse the @p iter direction
/// @param self
/// @param iter a valid iter created using @ref c_str_iter
void
c_str_iter_rev(CString const* self, CStringIter* iter)
{
  c_iter_rev(&iter->base, self->data, TO_IMPL(self)->len);
  iter->ch_counter   = 0;
  iter->base.counter = TO_IMPL(self)->len;
}

/// @brief this function could be called multiple times to traverse utf-8
///        characters
/// @param self
/// @param iter a valid iter created using @ref c_str_iter
/// @return char pointer of none zero terminated utf-8 character
/// @return is_ok[true]: return none zero terminated @ref CChar
///         is_ok[false]: end of @ref CString::data, invalid iter or other
///         errors
CCharResult
c_str_iter_next(CString const* self, CStringIter* iter)
{
  CIterElementResult result
      = ((CStringIterStepCallback)iter->base.step_callback)(iter, self->data,
                                                            TO_IMPL(self)->len);

  return (CCharResult){
      .ch    = (CChar){.data = result.element, .size = iter->current_ch_size},
      .is_ok = result.is_ok};
}

/// @brief traverse to the nth utf-8 character
/// @note this will start from the beginning to the nth character
/// @param self
/// @param iter a valid iter created using @ref c_str_iter
/// @param index index here is utf-8 character wise, not bytes
/// @return is_ok[true]: return none zero terminated @ref CChar
///         is_ok[false]: end of @ref CString::data, invalid iter or other
///         errors
CCharResult
c_str_iter_nth(CString const* self, CStringIter* iter, size_t index)
{
  CIterElementResult result
      = c_iter_nth((CIter*)iter, index, self->data, TO_IMPL(self)->len);

  return (CCharResult){
      .ch    = (CChar){.data = result.element, .size = iter->current_ch_size},
      .is_ok = result.is_ok};
}

/// @brief get the next character without changing the iterator
/// @param self
/// @param iter a valid iter created using @ref c_str_iter
/// @return is_ok[true]: return none zero terminated @ref CChar
///         is_ok[false]: end of @ref CString::data, invalid iter or other
///         errors
CCharResult
c_str_iter_peek(CString const* self, CStringIter const* iter)
{
  CIterElementResult result
      = c_iter_peek((CIter*)iter, self->data, TO_IMPL(self)->len);

  return (CCharResult){
      .ch    = (CChar){.data = result.element, .size = iter->current_ch_size},
      .is_ok = result.is_ok};
}

/// @brief move @p iter to the first character and return it
/// @param self
/// @param iter a valid iter created using @ref c_str_iter
/// @return is_ok[true]: return none zero terminated @ref CChar
///         is_ok[false]: end of @ref CString::data, invalid iter or other
///         errors
CCharResult
c_str_iter_first(CString const* self, CStringIter* iter)
{
  CIterElementResult result
      = c_iter_first((CIter*)iter, self->data, TO_IMPL(self)->len);

  return (CCharResult){
      .ch    = (CChar){.data = result.element, .size = iter->current_ch_size},
      .is_ok = result.is_ok};
}

/// @brief move @p iter to the last character and return it
/// @param self
/// @param iter a valid iter created using @ref c_str_iter
/// @return is_ok[true]: return none zero terminated @ref CChar
///         is_ok[false]: end of @ref CString::data, invalid iter or other
///         errors
CCharResult
c_str_iter_last(CString const* self, CStringIter* iter)
{
  CIterElementResult result
      = c_iter_last((CIter*)iter, self->data, TO_IMPL(self)->len);

  return (CCharResult){
      .ch    = (CChar){.data = result.element, .size = iter->current_ch_size},
      .is_ok = result.is_ok};
}

/// @brief create new @ref CString containes reversed utf-8 characters ()
///        orignal[ch1, ch2, ch3, ... chN] => reversed[chN, ... ch3, ch2, ch1]
/// @param self
/// @return new allocated @ref CString or NULL on error
CString*
c_str_reverse(CString const* self)
{
  assert(self);

  CString* rev_str = c_str_create_with_capacity(
      TO_IMPL(self)->len + 1, TO_IMPL(self)->allocator, false);
  if (!rev_str) return NULL;

  CStringIter iter_end = c_str_iter(NULL);
  c_str_iter_rev(self, &iter_end);

  CCharResult result;
  while ((result = c_str_iter_next(self, &iter_end)).is_ok) {
    c_str_push(rev_str, (CStr){result.ch.data, result.ch.size});
  }

  return rev_str;
}

/// @brief set length to zero
/// @param self
void
c_str_clear(CString* self)
{
  c_vec_clear((CVec*)self);
}

/// @brief this is a safer wrapper around sprintf
/// @param self
/// @param index this is byte wise not utf-8 characters
/// @param format_len
/// @param format
/// @return true: success, false: error
bool
c_str_format(
    CString* self, size_t index, size_t format_len, char const* format, ...)
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
bool
c_str_format_va(CString*    self,
                size_t      index,
                size_t      format_len,
                char const* format,
                va_list     va)
{
  assert(self);

  if (index > TO_IMPL(self)->len) return false;
  if (format[format_len] != '\0') return false;

  errno = 0;
  va_list va_tmp;
  va_copy(va_tmp, va);
  int needed_len = vsnprintf(NULL, 0, format, va_tmp);
  if (needed_len < 0) { return errno; }

  if ((size_t)needed_len >= GET_CAPACITY(self) - index) {
    bool status = c_str_set_capacity(self, GET_CAPACITY(self) + needed_len + 1);
    if (!status) return status;
  }

  errno = 0;
  needed_len
      = vsnprintf(self->data + index, GET_CAPACITY(self) - index, format, va);
  if (needed_len < 0) { return errno; }

  TO_IMPL(self)->len += needed_len;
  self->data[TO_IMPL(self)->len] = '\0';

  return true;
}

/// @brief compare @p str2 with @p str2
/// @param str1
/// @param str2
/// @return same like strcmp
int
c_str_compare(CString* str1, CString* str2)
{
  assert(str1);
  assert(str2);

  size_t min_size = TO_IMPL(str1)->len <= TO_IMPL(str2)->len
                        ? TO_IMPL(str1)->len
                        : TO_IMPL(str2)->len;
  return strncmp(str1->data, str2->data, min_size);
}

/// @brief check if @p data is utf-8 whitespace
/// @param ch utf8 char
/// @return true/false
bool
c_str_is_whitespace(CChar ch)
{
  for (size_t iii = 0; iii < sizeof(c_whitespaces) / sizeof(*c_whitespaces);
       ++iii) {
    if (memcmp(c_whitespaces[iii].data, ch.data, ch.size) == 0) return true;
  }

  return false;
}

/// @brief check if @p data is ascii whitespace
/// @param ch
/// @return true/false
bool
c_str_is_ascii_whitespace(char ch)
{
  return memchr(c_ascii_whitespaces, ch, sizeof(c_ascii_whitespaces) - 1)
             ? true
             : false;
}

/// @brief convert this to @ref CVec
/// @note this will not create new memory, so the new created @ref CVec will use
///       the same memory as @ref CString
/// @param self
/// @return @ref CVec object
CVec*
c_str_to_vec(CString* self)
{
  assert(self);

  return (CVec*)self;
}

/// @brief convert @ref CString to @ref CStr
/// @note this will not create a new library, and the resulted @ref CStr will
///       act like a reference to @p self
/// @param self
/// @return @ref CStr object
CStr*
c_cstring_to_cstr(CString* self)
{
  assert(self);

  return (CStr*)self;
}

/// @brief convert @ref CStr to @ref CString
/// @param cstr
/// @param allocator
/// @return new allocated CString object or NULL on error
CString*
c_cstr_to_cstring(CStr cstr, CAllocator* allocator)
{
  CString* str = c_str_create_from_raw(cstr, true, allocator);
  return str;
}

/// @brief free the memory of the allocated @ref CString
/// @param self
void
c_str_destroy(CString* self)
{
  c_vec_destroy((CVec*)self);
}

/******************************************************************************/
/*                                  Internal                                  */
/******************************************************************************/

CChar32Result
c_internal_str_utf8_to_utf32(const char* utf8)
{
  CChar32Result        result = {0};
  const unsigned char* bytes  = (const unsigned char*)utf8;
  if (bytes[0] < 0x80) {
    result.ch.data  = bytes[0];
    result.ch.count = 1; // 1-byte UTF-8 sequence
  } else if ((bytes[0] & 0xE0) == 0xC0) {
    result.ch.data  = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
    result.ch.count = 2; // 2-byte UTF-8 sequence
  } else if ((bytes[0] & 0xF0) == 0xE0) {
    result.ch.data = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6)
                     | (bytes[2] & 0x3F);
    result.ch.count = 3; // 3-byte UTF-8 sequence
  } else if ((bytes[0] & 0xF8) == 0xF0) {
    result.ch.data = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12)
                     | ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
    result.ch.count = 4; // 4-byte UTF-8 sequence
  } else {
    return result; // Invalid UTF-8 sequence
  }

  result.is_ok = true;
  return result;
}

// Function to encode a Unicode code point into UTF-16
CChar16Result
c_internal_str_utf32_to_utf16(uint32_t codepoint)
{
  CChar16Result result = {0};

  if (codepoint < 0x10000) {
    result.ch.data[0] = (uint16_t)codepoint;
    result.ch.count   = 1; // 1 UTF-16 code unit
  } else if (codepoint <= 0x10FFFF) {
    codepoint -= 0x10000;
    result.ch.data[0]
        = (uint16_t)((codepoint >> 10) + 0xD800); // High surrogate
    result.ch.data[1]
        = (uint16_t)((codepoint & 0x3FF) + 0xDC00); // Low surrogate
    result.ch.count = 2;                            // 2 UTF-16 code units
  } else {
    return result; // Invalid code point
  }

  result.is_ok = true;
  return result;
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
