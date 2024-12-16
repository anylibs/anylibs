#include "anylibs/dl_loader.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

c_error_t
c_dl_loader_create(char const  file_path[],
                   size_t      file_path_len,
                   CDLLoader** out_dl_loader)
{
  if (!out_dl_loader || !file_path || file_path_len == 0) return C_ERROR_none;
  if (file_path[file_path_len] != '\0') return C_ERROR_none_terminated_raw_str;

#ifdef _WIN32
  out_dl_loader->raw = (void*)LoadLibraryA(file_path);
#else
  *out_dl_loader = dlopen(file_path, RTLD_LAZY);
#endif

  return *out_dl_loader ? C_ERROR_none : C_ERROR_dl_loader_failed;
}

c_error_t
c_dl_loader_get(CDLLoader* self,
                char const symbol_name[],
                size_t     symbol_name_len,
                void**     out_result)
{
  assert(self);

  if (!out_result || !symbol_name || symbol_name_len == 0) return C_ERROR_none;
  if (symbol_name[symbol_name_len] != '\0')
    return C_ERROR_none_terminated_raw_str;

#ifdef _WIN32
  *out_result = GetProcAddress(self, symbol_name);
#else
  *out_result = dlsym(self, symbol_name);
#endif

  return *out_result ? C_ERROR_none : C_ERROR_dl_loader_invalid_symbol;
}

void
c_dl_loader_destroy(CDLLoader** self)
{
  if (self) {
#ifdef _WIN32
    BOOL close_status = FreeLibrary(self->raw);
    (void)close_status;
#else
    int close_status = dlclose(self);
    /// FIXME:
    (void)close_status;
#endif

    *self = NULL;
  }
}
