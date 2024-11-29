#include "ccmd.h"

#include <assert.h>

#ifdef _WIN32
#include <Windows.h>
#endif

int
main(int argc, char* argv[])
{
#ifdef _WIN32
  SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);
#endif

  CCmd   ccmd;
  CError err = ccmd_create(argc, argv, &ccmd);
  assert(err.code == 0);

  ccmd_destroy(&ccmd);

  return 0;
}
