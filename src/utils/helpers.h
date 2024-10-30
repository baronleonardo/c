#ifndef HELPERS_H
#define HELPERS_H

#include <ctype.h>

static inline char*
c_skip_whitespaces (char* needle)
{
  while (*needle && isspace (*needle))
    {
      needle++;
    }

  return needle;
}

#define C_STR(s) (s), sizeof (s) - 1
#define C_STR2(s) (s), strlen (s)
#define C_STR_INV(s) sizeof (s) - 1, (s)
#define C_CONCATENATE(a, b) a##b
#define C_STRINGIFY(s) #s

#endif // HELPERS_H
