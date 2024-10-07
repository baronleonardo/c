#ifndef CBUILD_PRIVATE_H
#define CBUILD_PRIVATE_H

#include "cbuild.h"

struct CTargetImpl
{
  CTargetType ttype;
  CStr name;
  CStr build_path;
  CStr install_path;
  CStr cflags;
  CStr lflags;
  CArray sources;      // CArray< CStr >
  CArray dependencies; // CArray< CTargetImpl* >
};

__C_DLL__ CError cbuild_create (
    CBuildType btype,
    char const base_path[],
    size_t base_path_len,
    CBuild* out_cbuild
);

__C_DLL__ CError cbuild_build (CBuild* self);

__C_DLL__ void cbuild_destroy (CBuild* self);

#endif // CBUILD_PRIVATE_H
