#ifndef ANYLIBS_DL_LOADER_H
#define ANYLIBS_DL_LOADER_H

#include "str.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct CDLLoader CDLLoader;

/// FIXME: this should be CPath
CDLLoader* c_dl_loader_create(CStr file_path);
void*      c_dl_loader_load(CDLLoader* self, CStr symbol_name);
bool       c_dl_loader_destroy(CDLLoader* self);

#endif // ANYLIBS_DL_LOADER_H
