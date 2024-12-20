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

#include <string.h>

#define __FILENAME__ strrchr(f, '/') + 1

char const*
c_error_to_str(c_error_t code)
{
  switch (code) {
  case C_ERROR_none:
    return "";
  case C_ERROR_mem_allocation:
    return "memory allocation";
  case C_ERROR_invalid_len:
    return "invalid len";
  case C_ERROR_invalid_size:
    return "invalid size";
  case C_ERROR_invalid_capacity:
    return "invalid capacity";
  case C_ERROR_invalid_index:
    return "invalid index";
  case C_ERROR_invalid_element_size:
    return "invalid element size";
  case C_ERROR_capacity_full:
    return "capacity is full";
  case C_ERROR_empty:
    return "empty";
  case C_ERROR_invalid_range:
    return "invalid range";
  case C_ERROR_not_found:
    return "not found";
  case C_ERROR_invalid_alignment:
    return "invalid alignment";
  case C_ERROR_raw_data:
    return "raw data";
  case C_ERROR_invalid_iterator:
    return "invalid iterator";
  case C_ERROR_invalid_unicode:
    return "invalid unicode";
  case C_ERROR_none_terminated_raw_str:
    return "none-terminated raw string";
  case C_ERROR_invalid_compare_fn:
    return "invalid compare function";
  case C_ERROR_invalid_format:
    return "invalid format";
  case C_ERROR_dl_loader_failed:
    return "dl_loader failed";
  case C_ERROR_dl_loader_invalid_symbol:
    return "dl_loader invalid symbol";
  case C_ERROR_fs_invalid_open_mode:
    return "filesystem invalid open mode";
  case C_ERROR_fs_invalid_path:
    return "filesystem invalid path";
  default:
    return "";
  }
}

#if ENABLE_ERROR_CALLBACK
#include "anylibs/str.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static void c_internal_error_callback_fn(c_error_t err,
                                         CStr      function_name,
                                         CStr      file_name,
                                         size_t    line_number,
                                         void*     user_data);

#ifdef _WIN32
#include <Winnt.h>
static bool c_internal_error_initiated = false;
#else
static _Atomic(bool) c_internal_error_initiated = false;
#endif
static CErrorCallback c_internal_error_callback  = c_internal_error_callback_fn;
static void*          c_internal_error_user_data = NULL;

void
c_error_register_once(CErrorCallback callback, void* user_data)
{

#ifdef _WIN32
  bool old_state
      = InterlockedCompareExchange(&c_internal_error_initiated, true, false);
  if (old_state) return;
#else
  if (!c_internal_error_initiated) {
    c_internal_error_initiated = true;
  } else {
    return;
  }
#endif

  c_internal_error_callback  = callback;
  c_internal_error_user_data = user_data;
}

void
c_internal_error_callback_fn(c_error_t err,
                             CStr      function_name,
                             CStr      file_name,
                             size_t    line_number,
                             void*     user_data)
{
  (void)user_data;
  char* filename = strrchr(file_name.data, '/') + 1;

  fprintf(stderr, "E%d: [%.*s] %s:%zu, %s\n", err, (int)function_name.len,
          function_name.data, filename, line_number, c_error_to_str(err));
}

void
__c_error_set(c_error_t err,
              CStr      function_name,
              CStr      file_name,
              size_t    line_number)
{
  c_internal_error_callback(err, function_name, file_name, line_number,
                            c_internal_error_user_data);
}
#endif
