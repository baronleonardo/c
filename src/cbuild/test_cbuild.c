#include <cbuild.h>
#include <cbuild_private.h>
#include <helpers.h>

#include <utest.h>

#include <stdio.h>
#include <stdlib.h>

UTEST_F_SETUP(CBuild)
{
  CError err = cbuild_create(CBUILD_TYPE_debug, ".", 1, utest_fixture);
  ASSERT_EQ_MSG(err.code, 0, err.desc);
}

UTEST_F_TEARDOWN(CBuild)
{
  ASSERT_TRUE(utest_fixture);
  cbuild_destroy(utest_fixture);
}

UTEST_F(CBuild, general)
{
  CTarget target;
  CError err =
    cbuild_exe_create(utest_fixture, C_STR("t"), C_STR("."), &target);
  ASSERT_EQ_MSG(err.code, 0, err.desc);

  err = cbuild_target_add_source(
    utest_fixture, &target, C_STR("build/cbuild/main.c"));
  ASSERT_EQ_MSG(err.code, 0, err.desc);

  CTarget target2;
  err =
    cbuild_shared_lib_create(utest_fixture, C_STR("t2"), C_STR("."), &target2);
  ASSERT_EQ_MSG(err.code, 0, err.desc);

  err = cbuild_target_add_source(
    utest_fixture, &target2, C_STR("build/cbuild/main.c"));
  ASSERT_EQ_MSG(err.code, 0, err.desc);

  CTarget target3;
  err =
    cbuild_static_lib_create(utest_fixture, C_STR("t3"), C_STR("."), &target3);
  ASSERT_EQ_MSG(err.code, 0, err.desc);

  err = cbuild_target_add_source(
    utest_fixture, &target3, C_STR("build/cbuild/main.c"));
  ASSERT_EQ_MSG(err.code, 0, err.desc);

  err = cbuild_build(utest_fixture);
  ASSERT_EQ_MSG(err.code, 0, err.desc);
}
