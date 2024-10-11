#include <cbuild.h>
#include <cbuild_private.h>

#include <utest.h>

#include <stdio.h>
#include <stdlib.h>

#define STR(s) s, (sizeof (s) - 1)

UTEST_F_SETUP (CBuild)
{
  CError err = cbuild_create (CBUILD_TYPE_debug, ".", 1, utest_fixture);
  ASSERT_EQ_MSG (err.code, 0, err.msg);
}

UTEST_F_TEARDOWN (CBuild)
{
  ASSERT_TRUE (utest_fixture);
  cbuild_destroy (utest_fixture);
}

UTEST_F (CBuild, general)
{
  CTarget target;
  CError err = cbuild_exe_create (utest_fixture, STR ("t"), &target);
  ASSERT_EQ_MSG (err.code, 0, err.msg);

  err = cbuild_target_add_source (
      utest_fixture, &target, STR ("build/cbuild/main.c")
  );
  ASSERT_EQ_MSG (err.code, 0, err.msg);

  CTarget target2;
  err = cbuild_shared_lib_create (utest_fixture, STR ("t2"), &target2);
  ASSERT_EQ_MSG (err.code, 0, err.msg);

  err = cbuild_target_add_source (
      utest_fixture, &target2, STR ("build/cbuild/main.c")
  );
  ASSERT_EQ_MSG (err.code, 0, err.msg);

  CTarget target3;
  err = cbuild_static_lib_create (utest_fixture, STR ("t3"), &target3);
  ASSERT_EQ_MSG (err.code, 0, err.msg);

  err = cbuild_target_add_source (
      utest_fixture, &target3, STR ("build/cbuild/main.c")
  );
  ASSERT_EQ_MSG (err.code, 0, err.msg);

  err = cbuild_build (utest_fixture);
  ASSERT_EQ_MSG (err.code, 0, err.msg);
}
