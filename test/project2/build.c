#include "cbuild.h"

#define STR(s) (s), sizeof (s) - 1
#define ON_ERR(err)                                                            \
  if (err.code != 0)                                                           \
    {                                                                          \
      return err;                                                              \
    }

CError
project2 (CBuild* cbuild)
{
  CTarget target;
  CError err = CERROR_none;

  err = cbuild_depends_on (cbuild, STR ("../project1"));
  ON_ERR (err);

  err = cbuild_exe_create (cbuild, STR ("main"), &target);
  ON_ERR (err);
  err = cbuild_target_add_source (cbuild, &target, STR ("main.c"));
  ON_ERR (err);

  return CERROR_none;
}
