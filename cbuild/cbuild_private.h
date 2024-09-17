#ifndef CBUILD_PRIVATE_H
#define CBUILD_PRIVATE_H

#include "cbuild.h"

/// FIXME: these chars need to be malloced
struct CBuildTargetImpl
{
    CBuildTargetType ttype;
    CStr name;
    CStr build_path;
    CStr install_path;
    CStr cflags;
    CStr lflags;
    CStr *sources;
    struct CBuildTargetImpl **dependencies;
};

CError cbuild_create(CBuildType btype, char const base_path[], size_t base_path_len, CBuild *out_cbuild);

CError cbuild_build(CBuild *self);

void cbuild_destroy(CBuild *self);

#endif // CBUILD_PRIVATE_H
