#include "cbuild.h"

#define STR(s) (s), sizeof(s) - 1
#define ON_ERR(err)                                                            \
  if (err.code != 0) { return err; }

CError
project1(CBuild* cbuild)
{
  CTarget target;
  CError  err = cbuild_exe_create(cbuild, STR("target"), STR("."), &target);
  ON_ERR(err);

  err = cbuild_target_add_source(cbuild, &target, STR("module1/main.c"));
  ON_ERR(err);

  err = cbuild_object_create(cbuild, STR("target_o"), STR("."), &target);
  ON_ERR(err);

  err = cbuild_target_add_source(cbuild, &target, STR("module1/main.c"));
  ON_ERR(err);

  ///////

  CTarget calc;
#ifdef _WIN32
  err = cbuild_static_lib_create(cbuild, STR("calc"), STR("."), &calc);
#else
  err = cbuild_shared_lib_create(cbuild, STR("calc"), STR("."), &calc);
#endif
  ON_ERR(err);
  err = cbuild_target_add_source(cbuild, &calc, STR("calc.c"));
  ON_ERR(err);

  err = cbuild_exe_create(cbuild, STR("main"), STR("."), &target);
  ON_ERR(err);
  err = cbuild_target_add_source(cbuild, &target, STR("main.c"));
  ON_ERR(err);
  err = cbuild_target_depends_on(cbuild, &target, &calc,
                                 CTARGET_PROPERTY_library_with_rpath);
  ON_ERR(err);
  err = cbuild_target_add_include_path_flag(cbuild, &target,
                                            STR("/usr/include"));
  ON_ERR(err);

  return CERROR_none;
}
