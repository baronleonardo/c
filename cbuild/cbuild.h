#ifndef CBUILD_H
#define CBUILD_H

#include <stddef.h>

#include "cerror.h"

#include <array.h>
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
  CBUILD_TARGET_TYPE_object,
  CBUILD_TARGET_TYPE_executable,
  CBUILD_TARGET_TYPE_shared,
  CBUILD_TARGET_TYPE_static,
} CBuildTargetType;

typedef struct CTargetSource
{
  char* path;
  size_t path_len;
} CTargetSource;

typedef struct CBuildTargetImpl CBuildTargetImpl;
typedef struct CBuildTarget
{
  CBuildTargetImpl* impl;
} CBuildTarget;

typedef struct CBuild
{
  CBuildType btype;
  CStr base_path;
  struct
  {
    CStr compiler;
    CStr linker;
    CStr static_lib_creator;
    CStr shared_lib_creator;
  } cmds;
  CStr cflags;
  CStr lflags;
  CArray targets; // CArray<CBuildTargetImpl*>
} CBuild;

// target create
CError cbuild_object_create (
    CBuild* self, char const* name, size_t name_len, CBuildTarget* out_target
);
CError cbuild_static_lib_create (
    CBuild* self, char const* name, size_t name_len, CBuildTarget* out_target
);
CError cbuild_shared_lib_create (
    CBuild* self, char const* name, size_t name_len, CBuildTarget* out_target
);
CError cbuild_exe_create (
    CBuild* self, char const* name, size_t name_len, CBuildTarget* out_target
);

CError cbuild_target_add_source (
    CBuild* self,
    CBuildTarget* target,
    char const source_path[],
    size_t source_path_len
);

CError cbuild_target_add_compile_flag (
    CBuild* self, CBuildTarget* target, char const flag[], size_t flag_len
);

CError cbuild_target_add_link_flag (
    CBuild* self, CBuildTarget* target, char const flag[], size_t flag_len
);

void cbuild_target_destroy (CBuild* self, CBuildTarget* target);

#endif // CBUILD_H
