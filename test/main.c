#ifdef _WIN32
#include <Windows.h>
#endif
#include "utest.h"

#ifdef _WIN32
void invalid_parameter_handler(
    wchar_t const* a,
    wchar_t const* b,
    wchar_t const* c,
    unsigned int   d,
    uintptr_t      e)
{
  (void)a;
  (void)b;
  (void)c;
  (void)d;
  (void)e;
}
#endif

UTEST_STATE();

int main(int argc, const char* const argv[])
{
#ifdef _WIN32
  // don't show the debug dialog on windows
  _set_invalid_parameter_handler(invalid_parameter_handler);
#endif

  return utest_main(argc, argv);
}
