#ifndef ANYLIBS_ERROR_H
#define ANYLIBS_ERROR_H

#include "str.h"

/// @brief usually this the return for any function inside anylibs.
/// if any code return beside @ref C_ERROR_none, the return
/// values (that usually return also as a pointer through parameters)
/// will be in an invalid state, using these value is undefined behavior
typedef enum c_error_t {
  C_ERROR_none,
  C_ERROR_fs_invalid_open_mode = -300,
  C_ERROR_fs_invalid_path,
  C_ERROR_fs_close_failed,
  C_ERROR_fs_is_dir,
  C_ERROR_fs_not_dir,
  C_ERROR_mem_allocation = -255,
  C_ERROR_invalid_len,
  C_ERROR_invalid_size,
  C_ERROR_invalid_capacity,
  C_ERROR_invalid_index,
  C_ERROR_invalid_element_size,
  C_ERROR_invalid_data,
  C_ERROR_null_ptr,
  C_ERROR_capacity_full,
  C_ERROR_empty,
  C_ERROR_invalid_range,
  C_ERROR_not_found,
  C_ERROR_invalid_alignment,
  C_ERROR_raw_data,
  C_ERROR_invalid_iterator,
  C_ERROR_invalid_unicode,
  C_ERROR_none_terminated_raw_str,
  C_ERROR_invalid_compare_fn,
  C_ERROR_invalid_format,
  C_ERROR_dl_loader_failed,
  C_ERROR_dl_loader_invalid_symbol,
} c_error_t;

char const* c_error_to_str(c_error_t code);

#if ANYLIBS_ENABLE_ERROR_CALLBACK
typedef void (*CErrorCallback)(c_error_t err, CStr function_name, CStr file_name, size_t line_number, void* user_data);

#define c_error_set(err) __c_error_set(err, CSTR(__func__), CSTR("/" __FILE__), (size_t)__LINE__)
void      __c_error_set(c_error_t err, CStr function_name, CStr file_name, size_t line_numer);
c_error_t c_error_get(void);
/// FIXME: this is not secure, as someone could inject some callback before the actual registeration.
///        check: mprotect (Linux/Unix): Mark memory as read-only, Windows APIs: Use VirtualProtect.
void      c_error_register_once(CErrorCallback callback, void* user_data);
#else
#define c_error_set(err) ((void)0)
#define c_error_get() (C_ERROR_none)
#define c_error_register_once(callback, user_data) ((void)0)
#endif // ANYLIBS_ENABLE_ERROR_CALLBACK

#endif // ANYLIBS_ERROR_H
