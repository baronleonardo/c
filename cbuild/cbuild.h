#ifndef CBUILD_H
#define CBUILD_H

#include <stddef.h>

#include "cerror.h"

typedef enum CBuildType
{
    CBUILD_TYPE_none,
    CBUILD_TYPE_debug,
    CBUILD_TYPE_release,
    CBUILD_TYPE_release_with_debug_info,
    CBUILD_TYPE_release_with_minimum_size,
} CBuildType;

typedef enum CBuildTargetType
{
    CBUILD_TARGET_TYPE_executable,
    CBUILD_TARGET_TYPE_shared,
    CBUILD_TARGET_TYPE_static,
} CBuildTargetType;

typedef struct CBuildTarget
{
    CBuildTargetType ttype;
    char *name;
    char **sources;
    char *build_path;
    char *install_path;
    char *cflags;
    char *lflags;
    struct CBuildTarget **dependencies;
} CBuildTarget;

typedef struct CBuild
{
    CBuildType btype;
    char *compiler;
    char *linker;
    char *cflags;
    char *lflags;
    CBuildTarget **targets;
} CBuild;

// target create
CError cbuild_static_lib_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target);
CError cbuild_shared_lib_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target);
CError cbuild_exe_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target);

CError cbuild_target_push(CBuild *self, CBuildTarget *target);

void cbuild_target_destroy(CBuild *self, CBuildTarget *target);

// void cbuild_build(CBuild *self);

// void cbuild_destroy(CBuild *self);

#endif // CBUILD_H
