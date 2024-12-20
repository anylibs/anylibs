#ifndef ANYLIBS_DEF_H
#define ANYLIBS_DEF_H

#include <stdbool.h>
#include <stddef.h>

typedef struct CResultBool {
  bool is_true;
  bool is_ok;
} CResultBool;

typedef struct CResultSizeT {
  size_t s;
  bool   is_ok;
} CResultSizeT;

typedef struct CResultVoidPtr {
  void* vp;
  bool  is_ok;
} CResultVoidPtr;

/// ---------------------------------------------------------------------------
/// disable sanitizer for undefined
/// ---------------------------------------------------------------------------
#if defined(__has_attribute) && __has_attribute(no_sanitize)
#define ANYLIBS_C_DISABLE_UNDEFINED __attribute__((no_sanitize("undefined")))
#else
#define ANYLIBS_C_DISABLE_UNDEFINED
#endif

/// ---------------------------------------------------------------------------
/// this will solve the security issues related to vsnprintf
/// ---------------------------------------------------------------------------
#ifndef ANYLIBS_C_PRINTF
#if (defined(__GNUC__) || defined(__clang__) || defined(__IAR_SYSTEMS_ICC__))  \
    && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)              \
    && !defined(CURL_NO_FMT_CHECKS)
#if defined(__MINGW32__) && !defined(__clang__)
#if defined(__MINGW_PRINTF_FORMAT) /* mingw-w64 3.0.0+. Needs stdio.h. */
#define ANYLIBS_C_PRINTF(fmt, arg)                                             \
  __attribute__((format(__MINGW_PRINTF_FORMAT, fmt, arg)))
#else
#define ANYLIBS_C_PRINTF(fmt, arg)
#endif
#else
#define ANYLIBS_C_PRINTF(fmt, arg) __attribute__((format(printf, fmt, arg)))
#endif
#else
#define ANYLIBS_C_PRINTF(fmt, arg)
#endif
#endif

#endif // ANYLIBS_DEF_H
