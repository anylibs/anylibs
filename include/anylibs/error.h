/**
 * @file error.h
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

#ifndef ANYLIBS_ERROR_H
#define ANYLIBS_ERROR_H

/// @brief usually this the return for any function inside anylibs.
///        if any code return beside @ref c_error_t::C_ERROR_none, the return
///        values (that usually return also as a pointer through parameters)
///        will be in an invalid state, using these value is undefined behavior
typedef enum c_error_t {
  C_ERROR_none,
  C_ERROR_mem_allocation = 256,
  C_ERROR_wrong_len,
  C_ERROR_wrong_capacity,
  C_ERROR_wrong_index,
  C_ERROR_wrong_element_size,
  C_ERROR_capacity_full,
  C_ERROR_empty,
  C_ERROR_wrong_range,
  C_ERROR_not_found,
  C_ERROR_wrong_alignment,
  C_ERROR_raw_data,
  C_ERROR_invalid_iterator,
  C_ERROR_invalid_unicode,
  C_ERROR_none_terminated_raw_str,
  C_ERROR_dl_loader_failed,
  C_ERROR_dl_loader_invalid_symbol,
} c_error_t;

char const* c_error_to_str(c_error_t code);

#endif // ANYLIBS_ERROR_H
