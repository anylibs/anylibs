#include "anylibs/error.h"

#include <stdlib.h>
#include <string.h>
#include <threads.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#define __FILENAME__ strrchr(f, '/') + 1

char const* c_error_to_str(c_error_t code)
{
  switch (code) {
    case C_ERROR_none:                     return "";
    case C_ERROR_fs_invalid_open_mode:     return "filesystem invalid open mode";
    case C_ERROR_fs_invalid_path:          return "filesystem invalid path";
    case C_ERROR_fs_close_failed:          return "closing file/dir failed";
    case C_ERROR_fs_is_dir:                return "is a directory";
    case C_ERROR_fs_not_dir:               return "is not a directory";
    case C_ERROR_mem_allocation:           return "memory allocation";
    case C_ERROR_invalid_len:              return "invalid len";
    case C_ERROR_invalid_size:             return "invalid size";
    case C_ERROR_invalid_capacity:         return "invalid capacity";
    case C_ERROR_invalid_index:            return "invalid index";
    case C_ERROR_invalid_element_size:     return "invalid element size";
    case C_ERROR_invalid_data:             return "invalid data";
    case C_ERROR_null_ptr:                 return "null pointer";
    case C_ERROR_capacity_full:            return "capacity is full";
    case C_ERROR_empty:                    return "empty";
    case C_ERROR_invalid_range:            return "invalid range";
    case C_ERROR_not_found:                return "not found";
    case C_ERROR_invalid_alignment:        return "invalid alignment";
    case C_ERROR_raw_data:                 return "raw data";
    case C_ERROR_invalid_iterator:         return "invalid iterator";
    case C_ERROR_invalid_unicode:          return "invalid unicode";
    case C_ERROR_none_terminated_raw_str:  return "none-terminated raw string";
    case C_ERROR_invalid_compare_fn:       return "invalid compare function";
    case C_ERROR_invalid_format:           return "invalid format";
    case C_ERROR_dl_loader_failed:         return "dl_loader failed";
    case C_ERROR_dl_loader_invalid_symbol: return "dl_loader invalid symbol";
    default:                               {
#ifdef _WIN32
      static thread_local char buffer[1024];
      DWORD                    msg_len = FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                       NULL, /* (not used with FORMAT_MESSAGE_FROM_SYSTEM) */
                                                       code,
                                                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                                       buffer,
                                                       sizeof(buffer),
                                                       NULL);

      return msg_len ? buffer : "unkown error";
#else
      return strerror(code);
#endif
    }
  }
}

#if ANYLIBS_ENABLE_ERROR_CALLBACK
#include "anylibs/str.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static void c_error_callback_internal_fn(c_error_t err, CStr function_name,
                                         CStr file_name, size_t line_number,
                                         void* user_data);

#ifdef _WIN32
/// TODO: it solves an issue with clang and qemu I have to use int instead of bool
static int c_error_initiated = false;
static c_error_t c_error           = C_ERROR_none;
#else
static _Atomic(bool)      c_error_initiated = false;
static _Atomic(c_error_t) c_error           = C_ERROR_none;
#endif
static CErrorCallback c_error_callback_internal  = c_error_callback_internal_fn;
static void*          c_internal_error_user_data = NULL;

void c_error_register_once(CErrorCallback callback, void* user_data)
{

#ifdef _WIN32
  bool old_state = InterlockedCompareExchange((volatile long*)&c_error_initiated, true, false);
  if (old_state) return;
#else
  if (!c_error_initiated) {
    c_error_initiated = true;
  } else {
    return;
  }
#endif

  c_error_callback_internal  = callback;
  c_internal_error_user_data = user_data;
}

void c_error_callback_internal_fn(c_error_t err, CStr function_name,
                                  CStr file_name, size_t line_number,
                                  void* user_data)
{
  (void)user_data;
  char* filename = strrchr(file_name.data, '/') + 1;
  fprintf(stderr, "E%d: [%.*s] %s:%zu, %s\n",
          err,
          (int)function_name.len,
          function_name.data,
          filename,
          line_number,
          c_error_to_str(err));
}

void __c_error_set(c_error_t err, CStr function_name, CStr file_name,
                   size_t line_number)
{
#ifdef _WIN32
  InterlockedExchange((volatile long*)&c_error, err);
#else
  c_error = err;
#endif

  c_error_callback_internal(err, function_name, file_name, line_number,
                            c_internal_error_user_data);
}

c_error_t c_error_get(void)
{
  return c_error;
}
#endif
