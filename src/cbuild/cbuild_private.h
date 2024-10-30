#ifndef CBUILD_PRIVATE_H
#define CBUILD_PRIVATE_H

#include "cbuild.h"

struct CTargetImpl
{
  CTargetType ttype;
  CStr name;
  CStr cbuild_base_dir;
  CStr base_dir;
  CStr build_path;
  CStr install_path;
  CStr cflags;
  CStr lflags;
  CStr link_with;
  CArray sources;      // CArray< CStr >
  CArray dependencies; // CArray< CTargetImpl* >
};

struct CBuildImpl
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
  CStr link_with;
  CArray targets;        // CArray< CTargetImpl* >
  CArray other_projects; // CArray< CBuildImpl* >
};

__C_DLL__ CError
cbuild_create(CBuildType btype,
              char const base_path[],
              size_t base_path_len,
              CBuild* out_cbuild);

__C_DLL__ CError
cbuild_configure(CBuild* self);
__C_DLL__ CError
cbuild_build(CBuild* self);

__C_DLL__ CError
cbuild_target_create(CBuild* self,
                     char const name[],
                     size_t name_len,
                     char const base_path[],
                     size_t base_path_len,
                     char const lflags[],
                     size_t lflags_len,
                     CTargetType ttype,
                     CTarget* out_target);

__C_DLL__ CError
cbuild_target_build(CBuild* self, CTargetImpl* target);

__C_DLL__ CError
cbuild_target_compile(CBuild* self, CTargetImpl* target);

__C_DLL__ CError
cbuild_target_link(CBuild* self, CTargetImpl* target);

__C_DLL__ void
cbuild_destroy(CBuild* self);

#endif // CBUILD_PRIVATE_H
