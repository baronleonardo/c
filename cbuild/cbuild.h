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

typedef enum CTargetType
{
  CTARGET_TYPE_object,
  CTARGET_TYPE_executable,
  CTARGET_TYPE_shared,
  CTARGET_TYPE_static,
} CTargetType;

typedef enum CTargetProperty
{
  CTARGET_PROPERTY_objects,
  CTARGET_PROPERTY_library,
  CTARGET_PROPERTY_library_with_rpath,
  CTARGET_PROPERTY_cflags,
  CTARGET_PROPERTY_lflags,
} CTargetProperty;

typedef struct CTargetSource
{
  char* path;
  size_t path_len;
} CTargetSource;

typedef struct CTargetImpl CTargetImpl;
typedef struct CTarget
{
  CTargetImpl* impl;
} CTarget;

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
  CArray targets; // CArray<CTargetImpl*>
} CBuild;

// target create
CError cbuild_object_create (
    CBuild* self, char const* name, size_t name_len, CTarget* out_target
);
CError cbuild_static_lib_create (
    CBuild* self, char const* name, size_t name_len, CTarget* out_target
);
CError cbuild_shared_lib_create (
    CBuild* self, char const* name, size_t name_len, CTarget* out_target
);
CError cbuild_exe_create (
    CBuild* self, char const* name, size_t name_len, CTarget* out_target
);

CError cbuild_target_add_source (
    CBuild* self,
    CTarget* target,
    char const source_path[],
    size_t source_path_len
);

CError cbuild_target_add_include_dir (
    CBuild* self,
    CTarget* target,
    char const include_path[],
    size_t include_path_len
);

CError cbuild_target_depends_on (
    CBuild* self, CTarget* target, CTarget* depend_on, CTargetProperty resource
);

CError cbuild_target_add_compile_flag (
    CBuild* self, CTarget* target, char const flag[], size_t flag_len
);

CError cbuild_target_add_link_flag (
    CBuild* self, CTarget* target, char const flag[], size_t flag_len
);

void cbuild_target_destroy (CBuild* self, CTarget* target);

#endif // CBUILD_H
