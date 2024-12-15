/**
 * @file defer.h
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

#ifndef ANYLIBS_DEFER_H
#define ANYLIBS_DEFER_H

#include "def.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct CDeferNode CDeferNode;
typedef struct CDefer {
  size_t capacity;
  size_t len;
  struct CDeferNode {
    void (*destructor)(void*);
    void* param;
  }* nodes;
} CDefer;

/// @brief between this and `c_defer_deinit` you can call any `c_defer` method
/// @param defer_stack_capacity the capacity of the stack that will hold the
///                             destructors
#define c_defer_init(defer_stack_capacity)                                     \
  CDefer c_defer_var__ = {.nodes    = (CDeferNode[defer_stack_capacity]){{0}}, \
                          .capacity = defer_stack_capacity}

/// @brief this pushes a destructor and it parameter to stack
///        and call each one in LIFO style at when c_defer_deinit get called
/// @param destructor a function with void(*)(void*) signature (could be NULL)
/// @param destructor_param the destructor parameter
#define c_defer(destructor, destructor_param)                                  \
  if (c_defer_var__.len < c_defer_var__.capacity) {                            \
    c_defer_var__.nodes[c_defer_var__.len++]                                   \
        = (CDeferNode){(void (*)(void*))destructor, destructor_param};         \
  }

/// @brief same as c_defer_err but in `cond` failure, it fails immediately
/// @param cond a test condition
/// @param destructor a function with void(*)(void*) signature (could be NULL)
/// @param destructor_param the destructor parameter
/// @param on_error a code that will be called on failed `cond`, this will be
///                 called before the destructor in failure of `cond`
#define c_defer_err(cond, destructor, destructor_param, on_error)              \
  do {                                                                         \
    if (c_defer_var__.len < c_defer_var__.capacity) {                          \
      c_defer_var__.nodes[c_defer_var__.len++]                                 \
          = (CDeferNode){(void (*)(void*))destructor, destructor_param};       \
      if (!(cond)) {                                                           \
        do {                                                                   \
          on_error;                                                            \
        } while (0);                                                           \
        goto c_defer_deinit_label;                                             \
      }                                                                        \
    }                                                                          \
  } while (0)

/// @brief same as c_defer_err but if `cond` is false, it fails immediately
/// @param cond a test condition
/// @param on_error a code that will be called on failed `cond`, this will be
///                 called before the destructor in failure of `cond`
#define c_defer_check(cond, on_error)                                          \
  do {                                                                         \
    if (!(cond)) {                                                             \
      do {                                                                     \
        on_error;                                                              \
      } while (0);                                                             \
      goto c_defer_deinit_label;                                               \
    }                                                                          \
  } while (0)

/// @brief this will deinitiate the c_defer and will call all destructors
#define c_defer_deinit()                                                       \
  do {                                                                         \
  c_defer_deinit_label:                                                        \
    for (size_t i = c_defer_var__.len - 1; i < c_defer_var__.len; --i) {       \
      if (c_defer_var__.nodes[i].destructor) {                                 \
        c_defer_var__.nodes[i].destructor(c_defer_var__.nodes[i].param);       \
      }                                                                        \
    }                                                                          \
    c_defer_var__ = (CDefer){0};                                               \
  } while (0)

#endif // ANYLIBS_DEFER_H
