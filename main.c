#include "ccmd.h"

#include <assert.h>

int
main (int argc, char* argv[])
{
  CCmd ccmd;
  CError err = ccmd_create (argc, argv, &ccmd);
  assert (err.code == 0);

  ccmd_destroy (&ccmd);

  return 0;
}
