/**
 * @file vec.h
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
#define TO_STR_ITER(iter) ((CStringIter*)(iter))

static unsigned char const c_ascii_whitespaces[] = {
    ' ',  // space
    '\t', // horizontal tab
    '\n', // newline
    '\v', // vertical tab
    '\f', // form feed
    '\r'  // carriage return
};

#define STR(s) (s), sizeof(s) - 1
static CChar c_whitespaces[] = {
    {STR("\t")},     // character tabulation
    {STR("\n")},     // line feed
    {STR("\v")},     // line tabulation
    {STR("\f")},     // form feed
    {STR("\r")},     // carriage return
    {STR(" ")},      // space
    {STR("")},     // next line
    {STR("\u00A0")}, // no-break space
    {STR("\u1680")}, // ogham space mark
    {STR("\u2000")}, // en quad
    {STR("\u2001")}, // em quad
    {STR("\u2002")}, // en space
    {STR("\u2003")}, // em space
    {STR("\u2004")}, // three-per-em space
    {STR("\u2005")}, // four-per-em space
    {STR("\u2006")}, // six-per-em space
    {STR("\u2007")}, // figure space
    {STR("\u2008")}, // punctuation space
    {STR("\u2009")}, // thin space
    {STR("\u200A")}, // hair space
    {STR("\u2028")}, // line separator
    {STR("\u2029")}, // paragraph separator
    {STR("\u202F")}, // narrow no-break space
    {STR("\u205F")}, // medium mathematical space
    {STR("\u3000")}, // ideographic space
    {STR("\u180E")}, // mongolian vowel separator
    {STR("\u200B")}, // zero width space
    {STR("\u200C")}, // zero width non-joiner
    {STR("\u200D")}, // zero width joiner
    {STR("\u2060")}, // word joiner
    {STR("\uFEFF")}  // zero width non-breaking space
};

static c_error_t c_internal_str_utf8_to_utf32(const char* utf8,
                                              uint32_t*   codepoint,
                                              size_t*     out_codepoint_size);
static c_error_t c_internal_str_utf32_to_utf16(uint32_t  codepoint,
                                               uint16_t* utf16,
                                               size_t*   out_utf16_size);

/// @brief create @ref CString object
/// @param[in] allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @param[out] out_c_str
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_create(CAllocator* allocator, CString** out_c_str)
{
  c_error_t err = c_str_create_with_capacity(1, allocator, false, out_c_str);
  return err;
}

/// @brief create @ref CString object from array of chars
/// @param[in] data the input array of chars
/// @param[in] data_len the length of the array of chars
/// @param[in] should_copy true: will copy the data, false: will just reference
///                        the data
/// @param[in] allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @param[out] out_c_str
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_create_from_raw(char        data[],
                      size_t      data_len,
                      bool        should_copy,
                      CAllocator* allocator,
                      CString**   out_c_str)
{
  if (!should_copy) {
    c_error_t err = c_str_create_with_capacity(0, allocator, false, out_c_str);
    if (err) return err;
    TO_IMPL(*out_c_str)->data         = data;
    TO_IMPL(*out_c_str)->raw_capacity = data_len;
    TO_IMPL(*out_c_str)->len          = data_len;

    return err;
  } else {
    c_error_t err
        = c_str_create_with_capacity(data_len + 1, allocator, false, out_c_str);
    if (err) return err;
    memcpy((*out_c_str)->data, data, data_len);
    (*out_c_str)->data[data_len] = '\0';

    TO_IMPL(*out_c_str)->len = data_len;

    return C_ERROR_none;
  }
}

/// @brief create new empty @ref CString object with capacity
/// @param[in] capacity
/// @param[in] allocator the allocator (if NULL the Default Allocator will be
///                      used)
/// @param[in] zero_initialized
/// @param[out] out_c_str
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_create_with_capacity(size_t      capacity,
                           CAllocator* allocator,
                           bool        zero_initialized,
                           CString**   out_c_str)
{
  return c_vec_create_with_capacity(sizeof(char), capacity, zero_initialized,
                                    allocator, (CVec**)out_c_str);
}

/// @brief clone another @ref CString object
/// @param[in] self
/// @param[in] should_shrink_clone true: shrink the clone capacity to @ref
///                                CString len
/// @param[out] out_c_str
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_clone(CString const* self, bool should_shrink_clone, CString** out_c_str)
{
  return c_vec_clone((CVec const*)self, should_shrink_clone, (CVec**)out_c_str);
}

/// @brief return if @ref CString is empty
/// @note this will also return false on failure if any
/// @param[in] self
/// @return error (any value but zero is treated as an error)
bool
c_str_is_empty(CString const* self)
{
  return c_vec_is_empty((CVec const*)self);
}

/// @brief return the number of utf8 characters
/// @param[in] self
/// @param[out] out_count
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_count(CString const* self, size_t* out_count)
{
  assert(self);

  CStringIter iter;
  c_str_iter(NULL, &iter);
  while (c_str_iter_next(self, &iter, NULL)) {}
  if (iter.byte_counter < TO_IMPL(self)->len) return C_ERROR_invalid_unicode;

  if (out_count) *out_count = iter.base.counter;
  return C_ERROR_none;
}

/// @brief return @ref CString length in bytes
/// @param[in] self
/// @param[out] out_len
void
c_str_len(CString const* self, size_t* out_len)
{
  c_vec_len((CVec const*)self, out_len);
}

/// @brief set new length
/// @note this could reallocate @ref CString::data
/// @param[in] self
/// @param[in] new_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_set_len(CString* self, size_t new_len)
{
  // 1 is for the trailing 0
  c_error_t err = c_vec_set_len((CVec*)self, new_len + 1);
  if (!err) {
    ((char*)TO_IMPL(self)->data)[TO_IMPL(self)->len - 1] = '\0';
    TO_IMPL(self)->len -= 1;
  }

  return err;
}

/// @brief get capacity in bytes
/// @param[in] self
/// @param[out] out_capacity
void
c_str_capacity(CString const* self, size_t* out_capacity)
{
  c_vec_capacity((CVec const*)self, out_capacity);
}

/// @brief return the empty remaining capacity in bytes
/// @param[in] self
/// @param[out] out_spare_capacity
void
c_str_spare_capacity(CString const* self, size_t* out_spare_capacity)
{
  c_vec_spare_capacity((CVec const*)self, out_spare_capacity);
}

/// @brief set new capacity
/// @note this could reallocate @ref CString::data
/// @param[in] self
/// @param[in] new_capacity
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_set_capacity(CString* self, size_t new_capacity)
{
  bool is_shrinking = GET_CAPACITY(self) > new_capacity;
  // 1 is for the trailing 0
  c_error_t err = c_vec_set_capacity((CVec*)self, new_capacity + 1);
  if (is_shrinking) ((char*)TO_IMPL(self)->data)[TO_IMPL(self)->len - 1] = '\0';

  return err;
}

/// @brief set capacity to the current length
/// @note this could reallocate @ref CString::data
/// @param[in] self
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_shrink_to_fit(CString* self)
{
  assert(self);

  TO_IMPL(self)->len += 1;
  c_error_t err = c_vec_shrink_to_fit((CVec*)self);
  TO_IMPL(self)->len -= 1;

  return err;
}

/// @brief get a byte (char) from @p index
/// @param[in] self
/// @param[in] byte_index
/// @param[out] out_data
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_get(CString const* self, size_t byte_index, char* out_data)
{
  assert(self);
  if (byte_index >= TO_IMPL(self)->len) return C_ERROR_wrong_index;
  if (out_data) *out_data = self->data[byte_index];

  return C_ERROR_none;
}

/// @brief find utf8 char
/// @param[in] self
/// @param[in] ch a struct that you need to fill it manually
/// @param[out] out_index
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_find(CString const* self, CChar ch, size_t* out_index)
{
  assert(self);
  if (!out_index) return C_ERROR_none;

  CStringIter iter;
  CChar       cur_ch;
  c_str_iter(NULL, &iter);

  while (c_str_iter_next(self, &iter, &cur_ch)) {
    if (memcmp(cur_ch.data, ch.data, ch.size) == 0) {
      *out_index = iter.base.counter - 1;
      return C_ERROR_none;
    }
  }

  return C_ERROR_not_found;
}

/// @brief same like @ref c_str_find by it will use external iter
///        so you can for example use @ref c_str_iter_nth to start from some
///        index
/// @param[in] self
/// @param[in] ch a struct that you need to fill it manually
/// @param[in] iter
/// @param[out] out_index
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_find_by_iter(CString const* self,
                   CChar          ch,
                   CStringIter*   iter,
                   size_t*        out_index)
{
  assert(self);
  if (!out_index || !iter) return C_ERROR_none;
  if (iter->byte_counter >= TO_IMPL(self)->len) return C_ERROR_invalid_iterator;

  CChar cur_ch;
  while (c_str_iter_next(self, iter, &cur_ch)) {
    if (memcmp(cur_ch.data, ch.data, ch.size) == 0) {
      *out_index = iter->base.counter - 1;
      return C_ERROR_none;
    }
  }

  return C_ERROR_not_found;
}

/// @brief check if @ref CString starts with @p data
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
/// @return true/false
bool
c_str_starts_with(CString const* self, char const data[], size_t data_len)
{
  return c_vec_starts_with((CVec const*)self, data, data_len,
                           (int (*)(void const*, void const*))strcmp);
}

/// @brief check if @ref CString ends with @p data
/// @param[in] self
/// @param[in] data
/// @param[in] data_len
/// @return true/false
bool
c_str_ends_with(CString const* self, char const data[], size_t data_len)
{
  return c_vec_ends_with((CVec const*)self, data, data_len,
                         (int (*)(void const*, void const*))strcmp);
}

/// @brief push some data to the end
/// @param[in] self
/// @param[in] data
/// @param[in] data_len in bytes
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_push(CString* self, char const data[], size_t data_len)
{
  return c_vec_push_range((CVec*)self, data, data_len);
}

/// @brief pop last character from the end
/// @note this could reallocate @ref CString::data
/// @param[in] self
/// @param[out] out_ch
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_pop(CString* self, CCharOwned* out_ch)
{
  assert(self);

  if (TO_IMPL(self)->len == 0) return C_ERROR_wrong_len;

  CChar ch;
  {
    CStringIter iter;
    c_str_iter(NULL, &iter);
    c_str_iter_rev(self, &iter);
    c_str_iter_next(self, &iter, &ch);
  }

  memcpy(out_ch, ch.data, ch.size);

  TO_IMPL(self)->len -= ch.size;
  self->data[TO_IMPL(self)->len] = '\0';

  return C_ERROR_none;
}

/// @brief insert @p data at @p index
/// @note @p index is in bytes not utf8 characters
/// @note this could reallocate @ref CString::data
/// @param[in] self
/// @param[in] byte_index
/// @param[in] data in bytes not utf-8 characters
/// @param[in] data_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_insert(CString*   self,
             size_t     byte_index,
             char const data[],
             size_t     data_len)
{
  return c_vec_insert_range((CVec*)self, byte_index, data, data_len);
}

/// @brief fill the whole string with some data, @p data will be repeatedly
///        inserted for the whole capacity
/// @param[in] self
/// @param[in] data in bytes not utf-8 characters
/// @param[in] data_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_fill(CString* self, char data[], size_t data_len)
{
  return c_vec_fill_with_repeat((CVec*)self, data, data_len);
}

/// @brief replace substring starting from @p index with @p range_size with @p
///        data with @p data_len
/// @note this could reallocate @ref CString::data
/// @param[in] self
/// @param[in] index in bytes not utf-8 characters
/// @param[in] range_size in bytes not utf-8 characters
/// @param[in] data in bytes not utf-8 characters
/// @param[in] data_len
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_replace(CString* self,
              size_t   index,
              size_t   range_size,
              char     data[],
              size_t   data_len)
{
  return c_vec_replace((CVec*)self, index, range_size, data, data_len);
}

/// @brief concatenate @p str2 into @p str1
/// @note this could reallocate @ref CString::data of @p str1
/// @param[in] str1
/// @param[in] str2
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_concatenate(CString* str1, CString const* str2)
{
  TO_IMPL(str2)->len += 1;
  c_error_t err = c_vec_concatenate((CVec*)str1, (CVec const*)str2);
  TO_IMPL(str2)->len -= 1;
  if (!err) str1->data[TO_IMPL(str1)->len - 1] = '\0';

  return err;
}

/// @brief remove substring start from @p start_index with @p range_size
/// @note this could reallocate @ref CString::data
/// @param[in] self
/// @param[in] start_index
/// @param[in] range_size
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_remove(CString* self, size_t start_index, size_t range_size)
{
  return c_vec_remove_range((CVec*)self, start_index, range_size);
}

/// @brief trim utf-8 whitespaces from start and end into @p out_slice
/// @param[in] self
/// @param[out] out_slice @ref CStr::data is not zero terminated
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_trim(CString const* self, CStr* out_slice)
{
  assert(self);

  if (!out_slice) return C_ERROR_none;

  /// from start
  size_t index = 0;
  {
    CStringIter iter;
    CChar       ch;

    c_str_iter(NULL, &iter);

    while (c_str_iter_next(self, &iter, &ch)) {
      if (c_str_is_whitespace(ch.data, ch.size)) {
        index += ch.size;
      } else {
        break;
      }
    }
  }

  /// from end
  size_t index_end = TO_IMPL(self)->len - 1;
  {
    CStringIter iter_end;
    c_str_iter(NULL, &iter_end);
    c_str_iter_rev(self, &iter_end);

    CChar ch;
    while (c_str_iter_next(self, &iter_end, &ch)) {
      if (c_str_is_whitespace(ch.data, ch.size)) {
        index_end -= ch.size;
      } else {
        break;
      }
    }
  }

  c_str_slice(self, index, index_end + 1 - index, out_slice);

  return C_ERROR_none;
}

/// @brief trim utf-8 whitespaces from the start into @p out_slice
/// @param[in] self
/// @param[out] out_slice @ref CStr::data is not zero terminated
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_trim_start(CString const* self, CStr* out_slice)
{
  assert(self);

  if (!out_slice) return C_ERROR_none;

  CStringIter iter = {0};
  c_str_iter(NULL, &iter);

  CChar  ch;
  size_t index = 0;
  while (c_str_iter_next(self, &iter, &ch)) {
    if (c_str_is_whitespace(ch.data, ch.size)) {
      index += ch.size;
    } else {
      break;
    }
  }

  c_str_slice(self, index, TO_IMPL(self)->len - index, out_slice);

  return C_ERROR_none;
}

/// @brief trim utf-8 whitespaces from the end into @p out_slice
/// @param[in] self
/// @param[out] out_slice @ref CStr::data is not zero terminated
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_trim_end(CString const* self, CStr* out_slice)
{
  assert(self);

  if (!out_slice) return C_ERROR_none;

  CStringIter iter_end;
  c_str_iter(NULL, &iter_end);
  c_str_iter_rev(self, &iter_end);

  CChar  ch;
  size_t index = TO_IMPL(self)->len - 1;
  while (c_str_iter_next(self, &iter_end, &ch)) {
    if (c_str_is_whitespace(ch.data, ch.size)) {
      index -= ch.size;
    } else {
      break;
    }
  }

  c_str_slice(self, 0, index + 1, out_slice);

  return C_ERROR_none;
}

/// @brief this is an iter like function that you call multiple times until it
///        return false
/// @param[in] self
/// @param[in] delimeters this is a list of utf-8 characters
/// @param[in] delimeters_len the length of this list
/// @param[in] iter
/// @param[out] out_slice
/// @return [true]: return none zero terminated substring
///         [false]: end of @ref CString::data, invalid iter or other errors
bool
c_str_split(CString const* self,
            CChar const    delimeters[],
            size_t         delimeters_len,
            CStringIter*   iter,
            CStr*          out_slice)
{
  assert(self);
  if (!iter || !out_slice) return false;
  if (iter->byte_counter >= TO_IMPL(self)->len) return false;

  out_slice->data = self->data + iter->byte_counter;

  size_t old_byte_counter = iter->byte_counter;
  CChar  ch;
  bool   found          = false;
  size_t delimeter_size = 0;
  while (!found && c_str_iter_next(self, iter, &ch)) {
    delimeter_size = 0;
    for (size_t iii = 0; iii < delimeters_len; ++iii) {
      if ((ch.size == delimeters[iii].size)
          && (memcmp(ch.data, delimeters[iii].data, delimeters[iii].size)
              == 0)) {
        found          = true;
        delimeter_size = delimeters[iii].size;
        break;
      }
    }
  }

  out_slice->len = iter->byte_counter - old_byte_counter - delimeter_size;

  return true;
}

/// @brief same like @ref c_str_split, but the delimeters are whitespaces
/// @param[in] self
/// @param[in] iter
/// @param[out] out_slice
/// @return [true]: return none zero terminated substring
///         [false]: end of @ref CString::data, invalid iter or other errors
bool
c_str_split_by_whitespace(CString const* self,
                          CStringIter*   iter,
                          CStr*          out_slice)
{
  return c_str_split(self, c_whitespaces,
                     sizeof(c_whitespaces) / sizeof(*c_whitespaces), iter,
                     out_slice);
}

/// @brief same like @ref c_str_split, but the delimeters are line endings
///        '\n', '\r' or '\r\n'
/// @param[in] self
/// @param[in] iter
/// @param[out] out_slice
/// @return [true]: return none zero terminated substring
///         [false]: end of @ref CString::data, invalid iter or other errors
bool
c_str_split_by_line(CString const* self, CStringIter* iter, CStr* out_slice)
{
  CChar const line_separators[] = {
      (CChar){STR("\n")},
      (CChar){STR("\r")},
  };

  bool status = c_str_split(self, line_separators,
                            sizeof(line_separators) / sizeof(*line_separators),
                            iter, out_slice);

  /// handle the case of '\r\n'
  if (status) {
    if ((self->data[iter->byte_counter - 1] == '\r')
        && (self->data[iter->byte_counter] == '\n')) {
      iter->byte_counter += 1;
      iter->base.counter += 1;
    }
  }

  return status;
}

/// @brief convert utf-8 string to utf-16 vector
/// @note this works indepent of the current locale
/// @param[in] self
/// @param[out] out_vec_u16 this is a @ref CVec of uint16_t
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_to_utf16(CString const* self, CVec* out_vec_u16)
{
  assert(self);

  if (!out_vec_u16) return C_ERROR_none;

  char*     utf8 = self->data;
  uint16_t  utf16;
  uint32_t  codepoint;
  c_error_t err = C_ERROR_none;

  while (*utf8) {
    size_t utf8_bytes;
    err = c_internal_str_utf8_to_utf32(utf8, &codepoint, &utf8_bytes);
    if (err) return err;

    size_t utf16_units;
    err = c_internal_str_utf32_to_utf16(codepoint, &utf16, &utf16_units);
    if (err) return err;

    err = c_vec_push(out_vec_u16, &utf16);
    if (err) return err;

    utf8 += utf8_bytes;
  }

  return C_ERROR_none;
}

/// @brief this is the opposite of @ref c_str_to_utf16
/// @note this works indepent of the current locale
/// @param[in] vec_u16 this is a @ref CVec of uint16_t
/// @param[out] out_c_str this is should be a valid pre created @ref CString
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_from_utf16(CVec const* vec_u16, CString* out_c_str)
{
  assert(vec_u16);
  if (!out_c_str) return C_ERROR_none;

  c_error_t err        = C_ERROR_none;
  uint16_t* utf16_data = ((uint16_t*)(vec_u16->data));
  enum { UTF8_MAX_COUNT = 4 };
  size_t vec_len;

  c_vec_len(vec_u16, &vec_len);
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
        buf[0] = (uint8_t)((codepoint >> 18) | 0xF0);
        buf[1] = (uint8_t)(((codepoint >> 12) & 0x3F) | 0x80);
        buf[2] = (uint8_t)(((codepoint >> 6) & 0x3F) | 0x80);
        buf[3] = (uint8_t)((codepoint & 0x3F) | 0x80);
        err    = c_str_push(out_c_str, buf, 4);
        if (err) return err;
      } else {
        return C_ERROR_invalid_unicode; // Error: Unmatched high surrogate
      }
    } else if (unit >= 0xDC00 && unit <= 0xDFFF) { // Low surrogate without a
                                                   // preceding high surrogate
      return C_ERROR_invalid_unicode; // Error: Invalid low surrogate
    } else {                          // BMP character (U+0000 to U+FFFF)
      // UTF-8 encoding for a basic character (1 to 3 bytes)
      if (unit <= 0x7F) { // 1 byte (ASCII range)
        err = c_str_push(out_c_str, (char*)&unit, 1);
        if (err) return err;
      } else if (unit <= 0x7FF) { // 2 bytes
        buf[0] = (uint8_t)((unit >> 6) | 0xC0);
        buf[1] = (uint8_t)((unit & 0x3F) | 0x80);
        err    = c_str_push(out_c_str, buf, 2);
        if (err) return err;
      } else { // 3 bytes
        buf[0] = (uint8_t)((unit >> 12) | 0xE0);
        buf[1] = (uint8_t)(((unit >> 6) & 0x3F) | 0x80);
        buf[2] = (uint8_t)((unit & 0x3F) | 0x80);
        err    = c_str_push(out_c_str, buf, 3);
        if (err) return err;
      }
    }
  }

  return C_ERROR_none;
}

/// @brief check if @ref CString::data is ascii
/// @param[in] self
/// @return
bool
c_str_is_ascii(CString const* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    if (!isascii(self->data[iii])) return false;
  }

  return true;
}

/// @brief convert the @ref CString::data to ascii uppercase
/// @param[in] self
void
c_str_to_ascii_uppercase(CString* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    self->data[iii] = toupper(self->data[iii]);
  }
}

/// @brief convert the @ref CString::data to ascii lowercase
/// @param[in] self
void
c_str_to_ascii_lowercase(CString* self)
{
  for (size_t iii = 0; iii < TO_IMPL(self)->len; ++iii) {
    self->data[iii] = tolower(self->data[iii]);
  }
}

/// @brief return a substring starting from @p start_index with @p range_size
/// @note @ref CStr::data is not zero terminated
/// @param[in] self
/// @param[in] start_index is in bytes not utf8 characters
/// @param[in] range_size is in bytes not utf8 characters
/// @param[out] out_slice
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_slice(CString const* self,
            size_t         start_index,
            size_t         range_size,
            CStr*          out_slice)
{
  assert(self);
  if (!out_slice) return C_ERROR_none;

  *out_slice = (CStr){
      .data = self->data + start_index,
      .len  = range_size,
  };

  return C_ERROR_none;
}

/// @brief create a new iterator, this is a must call function if you want to
///        deal with @ref CString iterators
/// @param[in] step_callback this could be NULL, if so, it will use
///                          @ref c_str_iter_default_step_callback
/// @param[out] out_c_iter
void
c_str_iter(CStringIterStepCallback step_callback, CStringIter* out_c_iter)
{
  if (!out_c_iter) return;

  c_iter(sizeof(char),
         step_callback ? (CIterStepCallback)step_callback
                       : (CIterStepCallback)c_str_iter_default_step_callback,
         (CIter*)out_c_iter);
  out_c_iter->byte_counter = 0;
}

/// @brief this is the default step callback for @ref c_str_iter if @p
///        step_callback was NULL.
///        this public for custom step function (filter like) that could also
///        use this default step function
/// @param[in] iter
/// @param[in] data
/// @param[in] data_len
/// @param[out] out_char pointer of none zero terminated utf-8 character
/// @return the state of the step, false here means could not continue stepping
bool
c_str_iter_default_step_callback(CStringIter* iter,
                                 char*        data,
                                 size_t       data_len,
                                 CChar*       out_char)
{
  if (iter->byte_counter > data_len) return false;
  if (!iter->base.is_reversed && (iter->byte_counter == data_len)) return false;

  if (iter->base.is_reversed) {
    char* ptr = data + iter->byte_counter;

    // get the beginning of a utf8 char
#define is_utf8_start_byte(ch) (((ch) & 0xC0) != 0x80)
    while ((--ptr >= data) && !is_utf8_start_byte(*ptr)) {}

    if (ptr < data) return false;
    iter->byte_counter = ptr - data;
  }

  // Determine the number of bytes in the current character
  size_t        codepoint_size = 0;
  size_t        byte_count     = iter->byte_counter;
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
    return false;
  }

  byte_count++;

  // Extract the remaining bytes of the character
  for (size_t iii = 1; iii < codepoint_size; ++iii) {
    if (((unsigned char)(((char*)data)[byte_count]) & 0xC0) != 0x80) {
      return false;
    }
    byte_count++;
  }

  iter->base.counter++;

  if (out_char)
    *out_char
        = (CChar){.data = data + iter->byte_counter, .size = codepoint_size};

  if (!iter->base.is_reversed) iter->byte_counter += codepoint_size;

  return true;
}

/// @brief reverse the @p iter direction
/// @param[in] self
/// @param[in] iter a valid iter created using @ref c_str_iter
void
c_str_iter_rev(CString const* self, CStringIter* iter)
{
  c_iter_rev(&iter->base, self->data, TO_IMPL(self)->len);
  iter->base.counter = 0;
  iter->byte_counter = TO_IMPL(self)->len;
}

/// @brief this function could be called multiple times to traverse utf-8
///        characters
/// @param[in] self
/// @param[in] iter a valid iter created using @ref c_str_iter
/// @param[out] out_char pointer of none zero terminated utf-8 character
/// @return [true]: return none zero terminated @ref CChar
///         [false]: end of @ref CString::data, invalid iter or other errors
bool
c_str_iter_next(CString const* self, CStringIter* iter, CChar* out_char)
{
  return ((CStringIterStepCallback)iter->base.step_callback)(
      iter, self->data, TO_IMPL(self)->len, out_char);
}

/// @brief traverse to the nth utf-8 character
/// @note this will start from the beginning to the nth character
/// @param[in] self
/// @param[in] iter a valid iter created using @ref c_str_iter
/// @param[in] index index here is utf-8 character wise, not bytes
/// @param[out] out_char pointer of none zero terminated utf-8 character
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_iter_nth(CString const* self,
               CStringIter*   iter,
               size_t         index,
               CChar*         out_char)
{
  return c_iter_nth((CIter*)iter, index, self->data, TO_IMPL(self)->len,
                    (void**)out_char);
}

/// @brief get the next character without changing the iterator
/// @param[in] self
/// @param[in] iter a valid iter created using @ref c_str_iter
/// @param[out] out_char pointer of none zero terminated utf-8 character
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_iter_peek(CString const* self, CStringIter const* iter, CChar* out_char)
{
  return c_iter_peek((CIter*)iter, self->data, TO_IMPL(self)->len,
                     (void**)out_char);
}

/// @brief move @p iter to the first character and return it
/// @param[in] self
/// @param[in] iter a valid iter created using @ref c_str_iter
/// @param[out] out_char pointer of none zero terminated utf-8 character
void
c_str_iter_first(CString const* self, CStringIter* iter, CChar* out_char)
{
  c_iter_first((CIter*)iter, self->data, TO_IMPL(self)->len, (void**)out_char);
}

/// @brief move @p iter to the last character and return it
/// @param[in] self
/// @param[in] iter a valid iter created using @ref c_str_iter
/// @param[out] out_char pointer of none zero terminated utf-8 character
void
c_str_iter_last(CString const* self, CStringIter* iter, CChar* out_char)
{
  c_iter_last((CIter*)iter, self->data, TO_IMPL(self)->len, (void**)out_char);
}

/// @brief create new @ref CString containes reversed utf-8 characters ()
///        orignal[ch1, ch2, ch3, ... chN] => reversed[chN, ... ch3, ch2, ch1]
/// @param[in] self
/// @param[out] reversed_str
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_reverse(CString const* self, CString** reversed_str)
{
  assert(self);

  if (!reversed_str) return C_ERROR_none;

  c_error_t err = c_str_create_with_capacity(
      TO_IMPL(self)->len + 1, TO_IMPL(self)->allocator, false, reversed_str);
  if (err) return err;

  CStringIter iter_end;
  c_str_iter(NULL, &iter_end);
  c_str_iter_rev(self, &iter_end);

  CChar ch;
  while (c_str_iter_next(self, &iter_end, &ch)) {
    c_str_push(*reversed_str, ch.data, ch.size);
  }

  return C_ERROR_none;
}

/// @brief set length to zero
/// @param[in] self
void
c_str_clear(CString* self)
{
  c_vec_clear((CVec*)self);
}

/// @brief this is a safer wrapper around sprintf
/// @param[in] self
/// @param[in] index this is byte wise not utf-8 characters
/// @param[in] format_len
/// @param[in] format
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_format(
    CString* self, size_t index, size_t format_len, char const* format, ...)
{
  va_list va;
  va_start(va, format);
  c_error_t err = c_str_format_va(self, index, format_len, format, va);
  va_end(va);

  return err;
}

/// @brief same like @ref c_str_format
/// @param[in] self
/// @param[in] index this is byte wise not utf-8 characters
/// @param[in] format_len
/// @param[in] format
/// @param[in] va vadriac list
/// @return error (any value but zero is treated as an error)
c_error_t
c_str_format_va(CString*    self,
                size_t      index,
                size_t      format_len,
                char const* format,
                va_list     va)
{
  assert(self);

  if (index > TO_IMPL(self)->len) return C_ERROR_wrong_index;
  if (format[format_len] != '\0') return C_ERROR_none_terminated_raw_str;

  errno = 0;
  va_list va_tmp;
  va_copy(va_tmp, va);
  int needed_len = vsnprintf(NULL, 0, format, va_tmp);
  if (needed_len < 0) { return errno; }

  if ((size_t)needed_len >= GET_CAPACITY(self) - index) {
    c_error_t err
        = c_str_set_capacity(self, GET_CAPACITY(self) + needed_len + 1);
    if (err) return err;
  }

  errno = 0;
  needed_len
      = vsnprintf(self->data + index, GET_CAPACITY(self) - index, format, va);
  if (needed_len < 0) { return errno; }

  TO_IMPL(self)->len += needed_len;
  self->data[TO_IMPL(self)->len] = '\0';

  return C_ERROR_none;
}

/// @brief compare @p str2 with @p str2
/// @param[in] str1
/// @param[in] str2
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
/// @param[in] data
/// @param[in] data_size in bytes not utf-8 characters
/// @return
bool
c_str_is_whitespace(char const data[], size_t data_size)
{
  for (size_t iii = 0; iii < sizeof(c_whitespaces) / sizeof(*c_whitespaces);
       ++iii) {
    if (memcmp(c_whitespaces[iii].data, data, data_size) == 0) return true;
  }

  return false;
}

/// @brief check if @p data is ascii whitespace
/// @param[in] data
/// @param[in] data_size
/// @return
bool
c_str_is_ascii_whitespace(unsigned char data)
{
  return memchr(c_ascii_whitespaces, data, sizeof(c_ascii_whitespaces) - 1)
             ? true
             : false;
}

/// @brief convert this to @ref CVec
/// @note this will not create new memory, so the new created @ref CVec will use
///       the same memory as @CString
/// @param[in] self
/// @param[out] out_c_vec
void
c_str_to_vec(CString* self, CVec** out_c_vec)
{
  assert(self);

  if (out_c_vec) *out_c_vec = (CVec*)self;
}

/// @brief free the memory of the allocated @ref CString
/// @param self
void
c_str_destroy(CString** self)
{
  c_vec_destroy((CVec**)self);
}

/******************************************************************************/
/*                                  Internal                                  */
/******************************************************************************/

c_error_t
c_internal_str_utf8_to_utf32(const char* utf8,
                             uint32_t*   codepoint,
                             size_t*     out_codepoint_size)
{
  const unsigned char* bytes = (const unsigned char*)utf8;
  if (bytes[0] < 0x80) {
    *codepoint = bytes[0];
    if (out_codepoint_size) *out_codepoint_size = 1; // 1-byte UTF-8 sequence
  } else if ((bytes[0] & 0xE0) == 0xC0) {
    *codepoint = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
    if (out_codepoint_size) *out_codepoint_size = 2; // 2-byte UTF-8 sequence
  } else if ((bytes[0] & 0xF0) == 0xE0) {
    *codepoint = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6)
                 | (bytes[2] & 0x3F);
    if (out_codepoint_size) *out_codepoint_size = 3; // 3-byte UTF-8 sequence
  } else if ((bytes[0] & 0xF8) == 0xF0) {
    *codepoint = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12)
                 | ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
    if (out_codepoint_size) *out_codepoint_size = 4; // 4-byte UTF-8 sequence
  } else {
    return C_ERROR_invalid_unicode; // Invalid UTF-8 sequence
  }

  return C_ERROR_none;
}

// Function to encode a Unicode code point into UTF-16
c_error_t
c_internal_str_utf32_to_utf16(uint32_t  codepoint,
                              uint16_t* utf16,
                              size_t*   out_utf16_size)
{
  if (codepoint < 0x10000) {
    utf16[0] = (uint16_t)codepoint;
    if (out_utf16_size) *out_utf16_size = 1; // 1 UTF-16 code unit
  } else if (codepoint <= 0x10FFFF) {
    codepoint -= 0x10000;
    utf16[0] = (uint16_t)((codepoint >> 10) + 0xD800);   // High surrogate
    utf16[1] = (uint16_t)((codepoint & 0x3FF) + 0xDC00); // Low surrogate
    if (out_utf16_size) *out_utf16_size = 2;             // 2 UTF-16 code units
  } else {
    return C_ERROR_invalid_unicode; // Invalid code point
  }

  return C_ERROR_none;
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
