#include <cbuild.h>
#include <cbuild_private.h>

#include <stdio.h>
#include <assert.h>

#define STR(s) s, (sizeof(s) - 1)

int main(void)
{
    CBuild cbuild;
    CError err = cbuild_create(CBUILD_TYPE_debug, &cbuild);
    assert(err.code == 0);

    CBuildTarget target;
    err = cbuild_exe_create(&cbuild, STR("t"), &target);
    assert(err.code == 0);
    err = cbuild_target_push(&cbuild, &target);
    assert(err.code == 0);

    target.sources = &(char *){"build/cbuild/main.c"};

    err = cbuild_build(&cbuild);
    assert(err.code == 0);

    cbuild_destroy(&cbuild);
    return 0;
}
