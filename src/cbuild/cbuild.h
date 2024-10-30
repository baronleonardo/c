#ifndef CBUILD_H
#define CBUILD_H

#include <stddef.h>

#include "cerror.h"

#include <array.h>
#include <str.h>

#ifdef _WIN32
#ifdef __C_BUILD_DLL__
#define __C_DLL__ __declspec(dllexport)
#else
#define __C_DLL__ __declspec(dllimport)
#endif
#else
#define __C_DLL__
#endif

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
  CTARGET_PROPERTY_objects = 1 << 0,
  CTARGET_PROPERTY_library = 1 << 1,
  CTARGET_PROPERTY_library_with_rpath = 1 << 2,
  CTARGET_PROPERTY_cflags = 1 << 3,
  CTARGET_PROPERTY_lflags = 1 << 4,
  CTARGET_PROPERTY_include_path = 1 << 5,
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

typedef struct CBuildImpl CBuildImpl;
typedef struct CBuild
{
  CBuildImpl* impl;
} CBuild;

// target create
__C_DLL__ CError
cbuild_object_create(CBuild* self,
                     char const name[],
                     size_t name_len,
                     char const base_path[],
                     size_t base_path_len,
                     CTarget* out_target);
__C_DLL__ CError
cbuild_static_lib_create(CBuild* self,
                         char const name[],
                         size_t name_len,
                         char const base_path[],
                         size_t base_path_len,
                         CTarget* out_target);
__C_DLL__ CError
cbuild_shared_lib_create(CBuild* self,
                         char const name[],
                         size_t name_len,
                         char const base_path[],
                         size_t base_path_len,
                         CTarget* out_target);
__C_DLL__ CError
cbuild_exe_create(CBuild* self,
                  char const name[],
                  size_t name_len,
                  char const base_path[],
                  size_t base_path_len,
                  CTarget* out_target);

__C_DLL__ CError
cbuild_target_add_source(CBuild* self,
                         CTarget* target,
                         char const source_path[],
                         size_t source_path_len);

__C_DLL__ CError
cbuild_target_set_include_path(CBuild* self,
                               CTarget* target,
                               char const include_path[],
                               size_t include_path_len);

__C_DLL__ CError
cbuild_target_add_include_path_flag(CBuild* self,
                                    CTarget* target,
                                    char const include_path[],
                                    size_t include_path_len);

__C_DLL__ CError
cbuild_target_depends_on(CBuild* self,
                         CTarget* target,
                         CTarget* depend_on,
                         CTargetProperty resource);

__C_DLL__ CError
cbuild_target_add_compile_flag(CBuild* self,
                               CTarget* target,
                               char const flag[],
                               size_t flag_len);

__C_DLL__ CError
cbuild_target_add_link_flag(CBuild* self,
                            CTarget* target,
                            char const flag[],
                            size_t flag_len);

__C_DLL__ CError
cbuild_target_get(CBuild* self,
                  char const target_name[],
                  size_t target_name_len,
                  CTarget* out_target);

__C_DLL__ CError
cbuild_depends_on(CBuild* self,
                  char const other_cbuild_path[],
                  size_t other_cbuild_path_len,
                  CBuild* out_other_cbuild);

__C_DLL__ void
cbuild_target_destroy(CBuild* self, CTarget* target);

#endif // CBUILD_H
