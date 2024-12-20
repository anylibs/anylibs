/**
 * @file dl_loader.h
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
 *
 * @brief this module help in loading function(s)/symbol(s) from a shared
 * library
 */

#ifndef ANYLIBS_DL_LOADER_H
#define ANYLIBS_DL_LOADER_H

#include "str.h"

#include <stddef.h>

typedef struct CDLLoader CDLLoader;

/// FIXME: this should be CPath
CDLLoader* c_dl_loader_create(CStr file_path);

void* c_dl_loader_get(CDLLoader* self, CStr symbol_name);

void c_dl_loader_destroy(CDLLoader* self);

#endif // ANYLIBS_DL_LOADER_H
