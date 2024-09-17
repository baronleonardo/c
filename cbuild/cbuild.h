#ifndef CBUILD_H
#define CBUILD_H

#include <stddef.h>

#include "cerror.h"

#include <str.h>

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

typedef struct CTargetSource
{
    char *path;
    size_t path_len;
} CTargetSource;

typedef struct CBuildTargetImpl CBuildTargetImpl;

typedef struct CBuildTarget
{
    CBuildTargetImpl *impl;
} CBuildTarget;

typedef struct CBuild
{
    CBuildType btype;
    CStr base_path;
    CStr compiler;
    CStr linker;
    CStr cflags;
    CStr lflags;
    CBuildTargetImpl **targets;
} CBuild;

// target create
CError cbuild_static_lib_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target);
CError cbuild_shared_lib_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target);
CError cbuild_exe_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target);

CError cbuild_target_add_source(CBuild *self, CBuildTarget *target, const char source_path[], size_t source_path_len);

void cbuild_target_destroy(CBuild *self, CBuildTarget *target);

#endif // CBUILD_H
