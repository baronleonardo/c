#include "cbuild.h"

#define STR(s) (s), sizeof (s) - 1
#define ON_ERR(err)                                                            \
  if (err.code != 0)                                                           \
    {                                                                          \
      return err;                                                              \
    }

CError
project1 (CBuild* cbuild)
{
  CTarget target;
  CError err = cbuild_exe_create (cbuild, STR ("target"), &target);
  ON_ERR (err);

  err = cbuild_target_add_source (cbuild, &target, STR ("module1/main.c"));
  ON_ERR (err);

  err = cbuild_object_create (cbuild, STR ("target_o"), &target);
  ON_ERR (err);

  err = cbuild_target_add_source (cbuild, &target, STR ("module1/main.c"));
  ON_ERR (err);

  ///////

  CTarget calc;
  err = cbuild_static_lib_create (cbuild, STR ("calc"), &calc);
  ON_ERR (err);
  err = cbuild_target_add_source (cbuild, &calc, STR ("calc.c"));
  ON_ERR (err);

  err = cbuild_exe_create (cbuild, STR ("main"), &target);
  ON_ERR (err);
  err = cbuild_target_add_source (cbuild, &target, STR ("main.c"));
  ON_ERR (err);
  err = cbuild_target_depends_on (
      cbuild, &target, &calc, CTARGET_PROPERTY_library_with_rpath
  );
  ON_ERR (err);
  err = cbuild_target_add_include_dir (cbuild, &target, STR ("/usr/include"));
  ON_ERR (err);

  return CERROR_none;
}