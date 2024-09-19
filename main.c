#include "ccmd.h"

int
main (int argc, char* argv[])
{
  CCmd ccmd = ccmd_create (argc, argv);

  ccmd_destroy (&ccmd);

  return 0;
}
