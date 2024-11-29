#include "cbuild.h"

CError
build(CBuild* self)
{
  CTarget target;
  CError err = cbuild_exe_create(self, "target", sizeof("target") - 1, &target);
  if (err.code != 0) { return err; }

  err = cbuild_target_add_source(self, &target, "module1/main.c",
                                 sizeof("module1/main.c") - 1);
  if (err.code != 0) { return err; }

  return CERROR_none;
}
