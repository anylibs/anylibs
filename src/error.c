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

#include "anylibs/error.h"

char const*
c_error_to_str(c_error_t code)
{
  switch (code) {
  case C_ERROR_none:
    return "";
  case C_ERROR_mem_allocation:
    return "memory allocation";
  case C_ERROR_wrong_len:
    return "wrong len";
  case C_ERROR_wrong_capacity:
    return "wrong capacity";
  case C_ERROR_wrong_index:
    return "wrong index";
  case C_ERROR_wrong_element_size:
    return "wrong element size";
  case C_ERROR_capacity_full:
    return "capacity is full";
  case C_ERROR_empty:
    return "empty";
  case C_ERROR_wrong_range:
    return "wrong range";
  case C_ERROR_not_found:
    return "not found";
  case C_ERROR_wrong_alignment:
    return "wrong alignment";
  case C_ERROR_raw_data:
    return "raw data";
  default:
    return "";
  }
}
