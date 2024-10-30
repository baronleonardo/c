#include "cbuild.h"

#define STR(s) (s), sizeof(s) - 1
#define ON_ERR(err)                                                            \
  if (err.code != 0) {                                                         \
    return err;                                                                \
  }

CError
project2(CBuild* cbuild)
{
  CTarget target;
  CError err = CERROR_none;

  CBuild project1_cbuild;
  err = cbuild_depends_on(cbuild, STR("../project1"), &project1_cbuild);
  ON_ERR(err);

  err = cbuild_exe_create(cbuild, STR("main"), STR("."), &target);
  ON_ERR(err);
  err = cbuild_target_add_source(cbuild, &target, STR("main.c"));
  ON_ERR(err);

  CTarget calc_target;
  err = cbuild_target_get(&project1_cbuild, STR("calc"), &calc_target);
  ON_ERR(err);
  err = cbuild_target_depends_on(cbuild,
                                 &target,
                                 &calc_target,
                                 CTARGET_PROPERTY_include_path |
                                   CTARGET_PROPERTY_library_with_rpath);
  ON_ERR(err);

  return CERROR_none;
}
