#include "anylibs/dl_loader.h"
#include "anylibs/error.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

/// @brief load a shared library
/// @param file_path
/// @return an object of CDLLoader or NULL on error
CDLLoader *c_dl_loader_create(CStr file_path) {
  if (!file_path.data || file_path.len == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return NULL;
  }
  if (file_path.data[file_path.len] != '\0') {
    c_error_set(C_ERROR_none_terminated_raw_str);
    return NULL;
  }

#ifdef _WIN32
  void *result = (void *)LoadLibraryA(file_path.data);
#else
  void *result = dlopen(file_path.data, RTLD_LAZY);
#endif

  return result;
}

/// @brief get a function/variable from the loaded shared library
/// @param self
/// @param symbol_name
/// @return the function/variable, NULL on error
void *c_dl_loader_load(CDLLoader *self, CStr symbol_name) {
  assert(self);

  if (!symbol_name.data || symbol_name.len == 0) {
    c_error_set(C_ERROR_fs_invalid_path);
    return NULL;
  }
  if (symbol_name.data[symbol_name.len] != '\0') {
    c_error_set(C_ERROR_none_terminated_raw_str);
    return NULL;
  }

#ifdef _WIN32
  void *result = (void*)GetProcAddress((HMODULE)self, symbol_name.data);
#else
  void *result = dlsym(self, symbol_name.data);
#endif

  return result;
}

/// @brief destroy a loaded shared library
/// @param self
bool c_dl_loader_destroy(CDLLoader *self) {
  bool close_status = true;

  if (self) {
#ifdef _WIN32
    close_status = (bool)FreeLibrary((HMODULE)self);
#else
    close_status = dlclose(self) == 0;
#endif
  }

  return close_status;
}
