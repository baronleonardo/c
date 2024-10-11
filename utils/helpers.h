#ifndef HELPERS_H
#define HELPERS_H

#include <ctype.h>

char*
c_skip_whitespaces (char* needle)
{
  while (*needle && isspace (*needle))
    {
      needle++;
    }

  return needle;
}

#endif // HELPERS_H
