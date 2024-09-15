#include <cbuild.h>
#include <cbuild_private.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define STR(s) s, (sizeof(s) - 1)

int main(void)
{
    CBuild cbuild;
    CError err = cbuild_create(CBUILD_TYPE_debug, ".", 1, &cbuild);
    assert(err.code == 0);

    CBuildTarget target;
    err = cbuild_exe_create(&cbuild, STR("t"), &target);
    assert(err.code == 0);

    err = cbuild_target_add_source(&cbuild, &target, STR("build/cbuild/main.c"));
    assert(err.code == 0);

    err = cbuild_build(&cbuild);
    assert(err.code == 0);

    cbuild_destroy(&cbuild);
    return 0;
}
