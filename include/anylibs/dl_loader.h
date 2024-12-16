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
 */

#ifndef ANYLIBS_DL_LOADER_H
#define ANYLIBS_DL_LOADER_H

#include "error.h"

#include <stddef.h>

typedef struct CDLLoader CDLLoader;

c_error_t c_dl_loader_create(char const  file_path[],
                             size_t      file_path_len,
                             CDLLoader** out_dl_loader);

c_error_t c_dl_loader_get(CDLLoader* self,
                          char const symbol_name[],
                          size_t     symbol_name_len,
                          void**     out_result);

void c_dl_loader_destroy(CDLLoader** self);

#endif // ANYLIBS_DL_LOADER_H
