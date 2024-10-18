#include "cbuild.h"
#include "cbuild_private.h"
#include "cbuilder_private.h"
#include "cerror.h"
#include "cprocess.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dl_loader.h>
#include <fs.h>

#if _WIN32 && (!_MSC_VER || !(_MSC_VER >= 1900))
#error "You need MSVC must be higher that or equal to 1900"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // disable warning about unsafe functions
#endif

static char const default_builder_path[] = ".c_build";
static char const default_install_path[] = "c_out";
#define default_build_c_target_name "_"
#define MAX_BUILD_FUNCTION_NAME_LEN 1000
#ifdef _WIN32
static char const default_pic_flag[] = "";
#else
static char const default_pic_flag[] = "-fPIC";
#endif

#ifdef _WIN32
static CBuilder* default_builder = &builders[CBUILDER_TYPE_msvc];
static char const lib_prefix[] = "";
#else
static CBuilder* default_builder = &builders[CBUILDER_TYPE_gcc];
static char const lib_prefix[] = "lib";
#endif

#define STR(s) (s), sizeof (s) - 1
#define STR2(s) (s), strlen (s)
#define STR_INV(s) sizeof (s) - 1, (s)
#define create_flags(type)                                                     \
  /* cflags*/                                                                  \
  str_err = c_str_create (STR2 (default_builder->cflags.type), &impl->cflags); \
  assert (str_err.code == 0);                                                  \
  /* lflags*/                                                                  \
  str_err = c_str_create (STR2 (default_builder->lflags.type), &impl->lflags); \
  assert (str_err.code == 0);

static CError internal_cbuild_get_path (
    CTargetImpl* target, char const build_install_dir_name[], CStr* out_path
);
static c_fs_error_t internal_find_and_push_all_compiled_objects_handler (
    char path[], size_t path_len, void* extra_data
);
static CError internal_compile_install_build_c (
    CBuild* self, CStr* out_cbuild_dll_dir, CStr* build_fn_name
);
static c_fs_error_t internal_find_and_push_all_compiled_objects_cstr_handler (
    char* path, size_t path_len, void* extra_data
);

CError
cbuild_create (
    CBuildType btype,
    char const base_path[],
    size_t base_path_len,
    CBuild* out_cbuild
)
{
  assert (base_path && base_path_len > 0);

  if (!out_cbuild)
    {
      return CERROR_none;
    }

  CBuildImpl* impl = calloc (1, sizeof (CBuildImpl));
  if (!impl)
    {
      return CERROR_memory_allocation;
    }

  /// btype
  *out_cbuild = (CBuild){ 0 };

  // type
  impl->btype = btype;

  // base path to absolute
  c_str_error_t str_err =
      c_str_create (base_path, base_path_len, &impl->base_path);
  assert (str_err.code == 0);
  str_err = c_str_set_capacity (&impl->base_path, c_fs_path_get_max_len ());
  assert (str_err.code == 0);
  c_fs_error_t fs_err = c_fs_path_to_absolute (
      base_path,
      base_path_len,
      impl->base_path.data,
      impl->base_path.capacity,
      &impl->base_path.len
  );
  assert (fs_err.code == 0);

  /// compiler
  str_err =
      c_str_create (STR2 (default_builder->compiler), &impl->cmds.compiler);
  assert (str_err.code == 0);

  /// linker
  str_err = c_str_create (STR2 (default_builder->linker), &impl->cmds.linker);
  assert (str_err.code == 0);

  /// static library creator
  str_err = c_str_create (
      STR2 (default_builder->archiver_static), &impl->cmds.static_lib_creator
  );
  assert (str_err.code == 0);

  /// shared library creator
  str_err = c_str_create (
      STR2 (default_builder->archiver_shared), &impl->cmds.shared_lib_creator
  );
  assert (str_err.code == 0);

  switch (btype)
    {
    case CBUILD_TYPE_debug:
      create_flags (debug);
      break;

    case CBUILD_TYPE_release:
      create_flags (release);
      break;

    case CBUILD_TYPE_release_with_debug_info:
      create_flags (release_with_debug_info);
      break;

    case CBUILD_TYPE_release_with_minimum_size:
      create_flags (release_with_minimum_size);
      break;

    default:
      create_flags (none);
    }

  /// link_with
  str_err = c_str_create (STR (""), &impl->link_with);
  assert (str_err.code == 0);

  /// targets
  c_array_error_t arr_err =
      c_array_create (sizeof (CTargetImpl*), &impl->targets);
  assert (arr_err.code == 0);

  /// other projects
  arr_err = c_array_create (sizeof (CBuildImpl*), &impl->other_projects);
  assert (arr_err.code == 0);

  *out_cbuild = (CBuild){ impl };

  return CERROR_none;
}

CError
cbuild_object_create (
    CBuild* self,
    char const name[],
    size_t name_len,
    char const base_path[],
    size_t base_path_len,
    CTarget* out_target
)
{
  return cbuild_target_create (
      self,
      name,
      name_len,
      base_path,
      base_path_len,
      STR (""),
      CTARGET_TYPE_object,
      out_target
  );
}

CError
cbuild_static_lib_create (
    CBuild* self,
    char const name[],
    size_t name_len,
    char const base_path[],
    size_t base_path_len,
    CTarget* out_target
)
{
  return cbuild_target_create (
      self,
      name,
      name_len,
      base_path,
      base_path_len,
      STR2 (default_builder->flags.static_library),
      CTARGET_TYPE_static,
      out_target
  );
}

CError
cbuild_shared_lib_create (
    CBuild* self,
    char const name[],
    size_t name_len,
    char const base_path[],
    size_t base_path_len,
    CTarget* out_target
)
{
  return cbuild_target_create (
      self,
      name,
      name_len,
      base_path,
      base_path_len,
      STR2 (default_builder->flags.shared_library),
      CTARGET_TYPE_shared,
      out_target
  );
}

CError
cbuild_exe_create (
    CBuild* self,
    char const name[],
    size_t name_len,
    char const base_path[],
    size_t base_path_len,
    CTarget* out_target
)
{
  return cbuild_target_create (
      self,
      name,
      name_len,
      base_path,
      base_path_len,
      STR (""),
      CTARGET_TYPE_executable,
      out_target
  );
}

CError
cbuild_target_add_source (
    CBuild* self,
    CTarget* target,
    char const source_path[],
    size_t source_path_len
)
{
  assert (self && self->impl);
  assert (target && target->impl && target->impl->name.data);
  assert (source_path && source_path_len > 0);

  // make sure the source has absolute path
  bool is_absolute;
  CStr target_path;
  c_fs_error_t fs_err =
      c_fs_path_is_absolute (source_path, source_path_len, &is_absolute);
  if (!is_absolute)
    {
      // <base_path>/
      c_str_error_t str_err =
          c_str_clone (&self->impl->base_path, &target_path);
      assert (str_err.code == 0);
      str_err = c_str_set_capacity (&target_path, c_fs_path_get_max_len ());
      assert (str_err.code == 0);

      // <base_path>/source_path
      fs_err = c_fs_path_append (
          target_path.data,
          target_path.len,
          target_path.capacity,
          source_path,
          source_path_len,
          &target_path.len
      );
      assert (fs_err.code == 0);
    }
  else
    {
      c_str_error_t str_err =
          c_str_create (source_path, source_path_len, &target_path);
      assert (str_err.code == 0);
    }

  bool exists;
  c_fs_exists (target_path.data, target_path.len, &exists);
  if (!exists)
    {
      return CERROR_no_such_source;
    }

  c_array_error_t arr_err = c_array_push (&target->impl->sources, &target_path);
  assert (arr_err.code == 0);

  return CERROR_none;
}

CError
cbuild_target_set_include_path (
    CBuild* self,
    CTarget* target,
    char const include_path[],
    size_t include_path_len
)
{
  assert (self && self->impl);
  assert (target && target->impl);
  assert (include_path && include_path_len > 0);

  if (include_path[include_path_len] != '\0')
    {
      return CERROR_invalid_string;
    }

  c_str_error_t str_err = c_str_replace_at (
      &target->impl->base_dir,
      0,
      target->impl->base_dir.len,
      include_path,
      include_path_len
  );
  assert (str_err.code == 0);

  return CERROR_none;
}

CError
cbuild_target_add_include_path_flag (
    CBuild* self,
    CTarget* target,
    char const include_path[],
    size_t include_path_len
)
{
  assert (self && self->impl);
  assert (target && target->impl);
  assert (include_path && include_path_len > 0);

  if (include_path[include_path_len] != '\0')
    {
      return CERROR_invalid_string;
    }

  c_str_error_t str_err = c_str_format (
      &target->impl->cflags,
      target->impl->cflags.len,
      STR_INV (" %s%s"),
      default_builder->cflags.include_path,
      include_path
  );
  assert (str_err.code == 0);

  return CERROR_none;
}

CError
cbuild_target_depends_on (
    CBuild* self, CTarget* target, CTarget* depend_on, CTargetProperty property
)
{
  assert (self && self->impl);
  assert (target && target->impl);
  assert (depend_on && depend_on->impl);

  c_str_error_t str_err = C_STR_ERROR_none;
  c_fs_error_t fs_err = C_FS_ERROR_none;

  if ((property & CTARGET_PROPERTY_objects) == CTARGET_PROPERTY_objects)
    {
      CStr build_path;

      str_err = c_str_create_empty (c_fs_path_get_max_len (), &build_path);
      assert (str_err.code == 0);

      if (strcmp (
              target->impl->base_dir.data, depend_on->impl->base_dir.data
          ) != 0)
        {
          char separator = c_fs_path_get_separator ();

          str_err = c_str_format (
              &build_path,
              0,
              STR_INV ("%s%c%s"),
              depend_on->impl->base_dir.data,
              separator,
              depend_on->impl->build_path.data
          );
          assert (str_err.code == 0);
        }
      else
        {
          str_err = c_str_append (&build_path, &depend_on->impl->build_path);
          assert (str_err.code == 0);
        }

      fs_err = c_fs_foreach (
          build_path.data,
          build_path.len,
          build_path.capacity,
          internal_find_and_push_all_compiled_objects_cstr_handler,
          &target->impl->sources
      );
      assert (fs_err.code == 0);

      c_str_destroy (&build_path);
    }

  if (((property & CTARGET_PROPERTY_library) == CTARGET_PROPERTY_library) ||
      ((property & CTARGET_PROPERTY_library_with_rpath) ==
       CTARGET_PROPERTY_library_with_rpath))
    {
      CStr install_path = { 0 };

      str_err = c_str_create_empty (c_fs_path_get_max_len (), &install_path);
      assert (str_err.code == 0);

      // if (strcmp (
      //         target->impl->base_dir.data, depend_on->impl->base_dir.data
      //     ) != 0)
      //   {
      char separator = c_fs_path_get_separator ();

      str_err = c_str_format (
          &install_path,
          0,
          STR_INV ("%s%c%s"),
          depend_on->impl->base_dir.data,
          separator,
          depend_on->impl->install_path.data
      );
      assert (str_err.code == 0);
      // }
      // else
      //   {
      //     str_err =
      //         c_str_append (&install_path, &depend_on->impl->install_path);
      //     assert (str_err.code == 0);
      //   }

      // -L<library path>
      str_err = c_str_format (
          &target->impl->lflags,
          target->impl->lflags.len,
          STR_INV (" %s%s"),
          default_builder->lflags.library_path,
          install_path.data
      );
      assert (str_err.code == 0);
      // -l<library>
      str_err = c_str_format (
          &target->impl->link_with,
          target->impl->link_with.len,
          STR_INV (" %s%s"),
          default_builder->lflags.library,
          depend_on->impl->name.data

      );
      assert (str_err.code == 0);
#ifndef _WIN32
      if ((property & CTARGET_PROPERTY_library_with_rpath) ==
          CTARGET_PROPERTY_library_with_rpath)
        {
          // -Wl,-rpath,<library path>
          str_err = c_str_format (
              &target->impl->lflags,
              target->impl->lflags.len,
              STR_INV (" -Wl,-rpath,%s"),
              install_path.data
          );
          assert (str_err.code == 0);
        }
#endif
#ifdef _WIN32
      str_err = c_str_append_with_cstr (
          &target->impl->link_with, STR2 (default_builder->extension.lib_static)
      );
      assert (str_err.code == 0);
#endif

      c_str_destroy (&install_path);
    }

  if ((property & CTARGET_PROPERTY_cflags) == CTARGET_PROPERTY_cflags)
    {
      str_err = c_str_format (
          &target->impl->cflags,
          target->impl->cflags.len,
          STR_INV (" %s"),
          depend_on->impl->cflags.data
      );
      assert (str_err.code == 0);
    }

  if ((property & CTARGET_PROPERTY_lflags) == CTARGET_PROPERTY_lflags)
    {
      str_err = c_str_format (
          &target->impl->lflags,
          target->impl->lflags.len,
          STR_INV (" %s"),
          depend_on->impl->lflags.data
      );
      assert (str_err.code == 0);
    }

  if ((property & CTARGET_PROPERTY_include_path) ==
      CTARGET_PROPERTY_include_path)
    {
      CError err = cbuild_target_add_include_path_flag (
          self,
          target,
          depend_on->impl->base_dir.data,
          depend_on->impl->base_dir.len
      );
      assert (err.code == 0);
    }

  return CERROR_none;
}

CError
cbuild_target_add_compile_flag (
    CBuild* self, CTarget* target, char const flag[], size_t flag_len
)
{
  assert (self && self->impl);
  assert (target && target->impl);

  if (flag[flag_len] != '\0')
    {
      return CERROR_invalid_string;
    }

  c_str_error_t str_err = c_str_format (
      &target->impl->cflags, target->impl->cflags.len, STR_INV (" %s"), flag
  );
  assert (str_err.code == 0);

  return CERROR_none;
}

CError
cbuild_target_add_link_flag (
    CBuild* self, CTarget* target, char const flag[], size_t flag_len
)
{
  assert (self && self->impl);
  assert (target && target->impl);

  if (flag[flag_len] != '\0')
    {
      return CERROR_invalid_string;
    }

  c_str_error_t str_err = c_str_format (
      &target->impl->lflags, target->impl->lflags.len, STR_INV (" %s"), flag
  );
  assert (str_err.code == 0);

  return CERROR_none;
}

CError
cbuild_target_get (
    CBuild* self,
    char const target_name[],
    size_t target_name_len,
    CTarget* out_target
)
{
  assert (self && self->impl);
  assert (target_name && target_name_len > 0);

  if (!out_target)
    {
      return CERROR_none;
    }

  /// FIXME: need better algorithm
  *out_target = (CTarget){ 0 };
  for (size_t i = 0; i < self->impl->targets.len; ++i)
    {
      CTargetImpl* target_impl = ((CTargetImpl**) self->impl->targets.data)[i];
      if (strncmp (target_impl->name.data, target_name, target_name_len) == 0)
        {
          *out_target = (CTarget){ .impl = target_impl };
          break;
        }
    }

  return CERROR_none;
}

CError
cbuild_depends_on (
    CBuild* self,
    char const other_cbuild_path[],
    size_t other_cbuild_path_len,
    CBuild* out_other_cbuild
)
{
  assert (self && self->impl);
  assert (other_cbuild_path && other_cbuild_path_len > 0);

  if (!out_other_cbuild)
    {
      return CERROR_none;
    }

  CError err = cbuild_create (
      self->impl->btype,
      other_cbuild_path,
      other_cbuild_path_len,
      out_other_cbuild
  );
  assert (err.code == 0);

  c_array_error_t arr_err =
      c_array_push (&self->impl->other_projects, &out_other_cbuild->impl);
  assert (arr_err.code == 0);

  cbuild_configure (out_other_cbuild);

  return CERROR_none;
}

void
cbuild_target_destroy (CBuild* self, CTarget* target)
{
  assert (self && self->impl);

  /// FIXME: find a better way to do this
  // remove it from target list from `self`
  for (size_t i = 0; i < self->impl->targets.len; i++)
    {
      // if (strcmp (
      //         target->impl->name.data,
      //         ((CTargetImpl**) self->impl->targets.data)[i]->name.data
      //     ) == 0)
      if (target->impl == ((CTargetImpl**) self->impl->targets.data)[i])
        {
          c_array_error_t arr_err = c_array_remove (&self->impl->targets, i);
          assert (arr_err.code == 0);
          break;
        }
    }

  c_str_destroy (&target->impl->name);
  c_str_destroy (&target->impl->cbuild_base_dir);
  c_str_destroy (&target->impl->base_dir);
  c_str_destroy (&target->impl->build_path);
  c_str_destroy (&target->impl->install_path);
  c_str_destroy (&target->impl->cflags);
  c_str_destroy (&target->impl->lflags);
  c_str_destroy (&target->impl->link_with);
  for (size_t i = 0; i < target->impl->sources.len; ++i)
    {
      c_str_destroy (&((CStr*) target->impl->sources.data)[i]);
    }
  c_array_destroy (&target->impl->sources);

  c_array_destroy (&target->impl->dependencies);

  *target->impl = (CTargetImpl){ 0 };
  free (target->impl);

  *target = (CTarget){ 0 };
}

CError
cbuild_configure (CBuild* self)
{
  assert (self && self->impl);

  CError err = CERROR_none;

  /// configure
  // save current path
  CStr cur_dir_path;
  c_str_error_t str_err =
      c_str_create_empty (c_fs_path_get_max_len (), &cur_dir_path);
  assert (str_err.code == 0);
  c_fs_error_t fs_err = c_fs_dir_get_current (
      cur_dir_path.data, cur_dir_path.capacity, &cur_dir_path.len
  );
  assert (fs_err.code == 0);

  // change to the base path
  fs_err = c_fs_dir_change_current (
      self->impl->base_path.data, self->impl->base_path.len
  );
  assert (fs_err.code == 0);

  // create build path if not existing
  CStr build_path;
  str_err = c_str_create (
      default_builder_path, sizeof (default_builder_path) - 1, &build_path
  );
  assert (str_err.code == 0);

  bool exists = false;
  c_fs_dir_exists (build_path.data, build_path.len, &exists);
  if (!exists)
    {
      fs_err = c_fs_dir_create (build_path.data, build_path.len);
      assert (fs_err.code == 0);
    }

  /// create install path if not existing
  CStr install_path;
  str_err = c_str_create (
      default_install_path, sizeof (default_install_path) - 1, &install_path
  );
  assert (str_err.code == 0);

  c_fs_dir_exists (install_path.data, install_path.len, &exists);
  if (!exists)
    {
      fs_err = c_fs_dir_create (install_path.data, install_path.len);
      assert (fs_err.code == 0);
    }

  // this will build and install build.c
  CStr cbuild_dll_dir;
  CStr build_function_name;
  err = internal_compile_install_build_c (
      self, &cbuild_dll_dir, &build_function_name
  );
  assert (err.code == 0);

  /// FIXME: find a better way
#ifdef _WIN32
  char const cbuild_dll_name[] = default_build_c_target_name ".dll";
#else
  char const cbuild_dll_name[] = "lib" default_build_c_target_name ".so";
#endif

  CStr cbuild_dll_path;
  str_err = c_str_clone (&cbuild_dll_dir, &cbuild_dll_path);
  assert (str_err.code == 0);
  str_err = c_str_set_capacity (&cbuild_dll_path, c_fs_path_get_max_len ());
  assert (str_err.code == 0);

  fs_err = c_fs_path_append (
      cbuild_dll_path.data,
      cbuild_dll_path.len,
      cbuild_dll_path.capacity,
      cbuild_dll_name,
      sizeof (cbuild_dll_name) - 1,
      &cbuild_dll_path.len
  );
  assert (fs_err.code == 0);

  fs_err = c_fs_path_to_absolute (
      cbuild_dll_path.data,
      cbuild_dll_path.len,
      cbuild_dll_path.data,
      cbuild_dll_path.capacity,
      &cbuild_dll_path.len
  );
  assert (fs_err.code == 0);

  CDLLoader dll_loader;
  c_dl_error_t dl_err = c_dl_loader_create (
      cbuild_dll_path.data, cbuild_dll_path.len, &dll_loader
  );
  assert (dl_err.code == 0);

  CError (*build_fn) (CBuild*);
  dl_err = c_dl_loader_get (
      &dll_loader,
      build_function_name.data,
      build_function_name.len,
      (void*) &build_fn
  );
  assert (dl_err.code == 0);

  err = build_fn (self);
  assert (err.code == 0);

  // restore old path
  fs_err = c_fs_dir_change_current (cur_dir_path.data, cur_dir_path.len);
  assert (fs_err.code == 0);

  // free
  c_dl_loader_destroy (&dll_loader);
  c_str_destroy (&cbuild_dll_path);
  c_str_destroy (&cbuild_dll_dir);
  c_str_destroy (&build_function_name);
  c_str_destroy (&cur_dir_path);
  c_str_destroy (&build_path);
  c_str_destroy (&install_path);

  return err;
}

CError
cbuild_build (CBuild* self)
{
  assert (self && self->impl);

  CError err = CERROR_none;

  // save current path
  CStr cur_dir_path;
  c_str_error_t str_err =
      c_str_create_empty (c_fs_path_get_max_len (), &cur_dir_path);
  assert (str_err.code == 0);
  c_fs_error_t fs_err = c_fs_dir_get_current (
      cur_dir_path.data, cur_dir_path.capacity, &cur_dir_path.len
  );
  assert (fs_err.code == 0);

  // change to the base path
  fs_err = c_fs_dir_change_current (
      self->impl->base_path.data, self->impl->base_path.len
  );
  assert (fs_err.code == 0);

  /// build
  for (size_t i = 0; i < self->impl->other_projects.len; i++)
    {
      err = cbuild_build (&(CBuild
      ){ ((CBuildImpl**) self->impl->other_projects.data)[i] });
      if (err.code != 0)
        {
          return err;
        }
    }

  /// build all targets
  for (size_t i = 0; i < self->impl->targets.len; ++i)
    {
      err = cbuild_target_build (
          self, ((CTargetImpl**) self->impl->targets.data)[i]
      );
    }

  // restore old path
  fs_err = c_fs_dir_change_current (cur_dir_path.data, cur_dir_path.len);
  assert (fs_err.code == 0);

  // free
  c_str_destroy (&cur_dir_path);

  return err;
}

void
cbuild_destroy (CBuild* self)
{
  assert (self && self->impl);

  for (size_t i = 0; i < self->impl->other_projects.len; i++)
    {
      cbuild_destroy (&((CBuild*) self->impl->other_projects.data)[i]);
    }

  c_array_destroy (&self->impl->other_projects);

  c_str_destroy (&self->impl->base_path);
  c_str_destroy (&self->impl->cflags);
  c_str_destroy (&self->impl->lflags);
  c_str_destroy (&self->impl->link_with);
  c_str_destroy (&self->impl->cmds.compiler);
  c_str_destroy (&self->impl->cmds.linker);
  c_str_destroy (&self->impl->cmds.shared_lib_creator);
  c_str_destroy (&self->impl->cmds.static_lib_creator);

  // targets
  for (size_t i = 0; i < self->impl->targets.len;)
    {
      cbuild_target_destroy (
          self, &(CTarget){ ((CTargetImpl**) self->impl->targets.data)[i] }
      );
    }
  c_array_destroy (&self->impl->targets);

  *self->impl = (CBuildImpl){ 0 };
  free (self->impl);

  *self = (CBuild){ 0 };
}

CError
cbuild_target_create (
    CBuild* self,
    char const name[],
    size_t name_len,
    char const base_path[],
    size_t base_path_len,
    char const lflags[],
    size_t lflags_len,
    CTargetType ttype,
    CTarget* out_target
)
{
  assert (self && self->impl);

  if (!out_target)
    {
      return CERROR_none;
    }

  *out_target = (CTarget){ 0 };
  out_target->impl = calloc (1, sizeof (CTargetImpl));
  if (!out_target->impl)
    {
      return CERROR_memory_allocation;
    }

  // ttype
  out_target->impl->ttype = ttype;

  // target name
  c_str_error_t str_err =
      c_str_create (name, name_len, &out_target->impl->name);
  assert (str_err.code == 0);

  // cbuild base dir
  str_err =
      c_str_clone (&self->impl->base_path, &out_target->impl->cbuild_base_dir);
  assert (str_err.code == 0);

  // base dir
  str_err = c_str_set_capacity (
      &out_target->impl->base_dir, c_fs_path_get_max_len ()
  );
  assert (str_err.code == 0);
  bool is_absolute = false;
  c_fs_error_t fs_err =
      c_fs_path_is_absolute (base_path, base_path_len, &is_absolute);
  if (!is_absolute)
    {
      fs_err = c_fs_path_to_absolute (
          base_path,
          base_path_len,
          out_target->impl->base_dir.data,
          out_target->impl->base_dir.capacity,
          &out_target->impl->base_dir.len
      );
      assert (fs_err.code == 0);
    }
  else
    {
      str_err = c_str_replace_at (
          &out_target->impl->base_dir,
          0,
          out_target->impl->base_dir.len,
          base_path,
          base_path_len
      );
      assert (str_err.code == 0);
    }

  // build path
  CError err = internal_cbuild_get_path (
      out_target->impl, default_builder_path, &out_target->impl->build_path
  );
  if (err.code != C_FS_ERROR_none.code)
    {
      return err;
    }

  // install path
  err = internal_cbuild_get_path (
      out_target->impl, default_install_path, &out_target->impl->install_path
  );
  if (err.code != C_FS_ERROR_none.code)
    {
      return err;
    }

  // cflags
  str_err = c_str_clone (&self->impl->cflags, &out_target->impl->cflags);
  assert (str_err.code == 0);
  // lflags
  str_err = c_str_create (lflags, lflags_len, &out_target->impl->lflags);
  assert (str_err.code == 0);
  // link_with
  str_err = c_str_create (STR (""), &out_target->impl->link_with);
  assert (str_err.code == 0);

  // sources
  c_array_error_t arr_err =
      c_array_create (sizeof (CStr), &out_target->impl->sources);
  assert (arr_err.code == 0);

  // dependencies
  arr_err =
      c_array_create (sizeof (CTargetImpl*), &out_target->impl->dependencies);
  assert (arr_err.code == 0);

  arr_err = c_array_push (&self->impl->targets, &out_target->impl);
  assert (arr_err.code == 0);

  return CERROR_none;
}

CError
cbuild_target_build (CBuild* self, CTargetImpl* target)
{
  if (!target)
    return CERROR_none;

  // create the target build path if not existing
  bool obj_out_path_exists = false;
  c_fs_dir_exists (
      target->build_path.data, target->build_path.len, &obj_out_path_exists
  );
  if (!obj_out_path_exists)
    {
      c_fs_error_t fs_err =
          c_fs_dir_create (target->build_path.data, target->build_path.len);
      assert (fs_err.code == 0);
    }

  for (size_t iii = 0; iii < target->dependencies.len; iii++)
    {
      /// FIXME: this will introduce an issue if one of deps destructed
      cbuild_target_build (
          self, ((CTargetImpl**) target->dependencies.data)[iii]
      );
    }

  CError err = CERROR_none;

  // compile
  err = cbuild_target_compile (self, target);

  cbuild_target_link (self, target);

  return err;
}

CError
cbuild_target_compile (CBuild* self, CTargetImpl* target)
{
  CError err = CERROR_none;

  CArray cmd; // CArray < char* >
  c_array_error_t arr_err = c_array_create (sizeof (char*), &cmd);
  assert (arr_err.code == 0);

  // $ <compiler>
  arr_err = c_array_push (&cmd, &self->impl->cmds.compiler.data);
  assert (arr_err.code == 0);

  // $ <compiler> <cflags>
  CStr cflags;
  c_str_error_t str_err = c_str_clone (&target->cflags, &cflags);
  assert (str_err.code == 0);
  char* subtoken = strtok (cflags.data, C_STR_WHITESPACES);
  if (subtoken)
    {
      do
        {
          arr_err = c_array_push (&cmd, &subtoken);
          assert (arr_err.code == 0);
        }
      while ((subtoken = strtok (NULL, C_STR_WHITESPACES)));
    }

  // $ <compiler> <cflags> -c
  arr_err =
      c_array_push (&cmd, &(char const*){ default_builder->cflags.compile });
  assert (arr_err.code == 0);

  char separator = c_fs_path_get_separator ();

#ifndef _WIN32
  // $ <compiler> <cflags> -c -o<c_out>/<target name>/<target name>.o
  CStr output;
  str_err = c_str_create_empty (1, &output);
  assert (str_err.code == 0);
  str_err = c_str_format (
      &output,
      0,
      STR_INV ("%s%s%c%s%s"),
      default_builder->flags.output,
      target->build_path.data,
      separator,
      target->name.data,
      default_builder->extension.object
  );
  assert (str_err.code == 0);
  arr_err = c_array_push (&cmd, &output.data);
  assert (arr_err.code == 0);
#else
  // $ <compiler> <cflags> -c /Foc:<c_out>/<target name>
  CStr obj_output;
  str_err = c_str_create_empty (1, &obj_output);
  assert (str_err.code == 0);
  str_err = c_str_format (
      &obj_output,
      0,
      STR_INV ("%s%s%c%s"),
      builder_windows_compile_flag_obj_output_path,
      target->build_path.data,
      separator,
      target->name.data
  );
  assert (str_err.code == 0);
  arr_err = c_array_push (&cmd, &obj_output.data);
  assert (arr_err.code == 0);

  // $ <compiler> <cflags> -c /Foc:<c_out>/<target name>
  // /Fdc:<c_out>/<target name>
  CStr pdb_output;
  str_err = c_str_create_empty (1, &pdb_output);
  assert (str_err.code == 0);
  str_err = c_str_format (
      &pdb_output,
      0,
      STR_INV ("%s%s%c%s"),
      builder_windows_compile_flag_pdb_output_path,
      target->build_path.data,
      separator,
      target->name.data
  );
  assert (str_err.code == 0);
  arr_err = c_array_push (&cmd, &pdb_output.data);
  assert (arr_err.code == 0);
#endif

  for (size_t iii = 0; iii < target->sources.len; ++iii)
    {
      // $ <compiler> <cflags> -c <source>
      arr_err = c_array_push (&cmd, &((CStr*) target->sources.data)[iii].data);
      assert (arr_err.code == 0);

      // $ <compiler> <cflags> -c <source> <NULL>
      arr_err = c_array_push (&cmd, &(void*){ NULL });
      assert (arr_err.code == 0);

      int process_status =
          cprocess_exec ((char const* const*) cmd.data, cmd.len);
      assert (process_status == 0);

      // $ <compiler> <cflags> -c <source>
      arr_err = c_array_pop (&cmd, NULL);
      assert (arr_err.code == 0);
      // $ <compiler> <cflags> -c
      arr_err = c_array_pop (&cmd, NULL);
      assert (arr_err.code == 0);
    }

  c_array_destroy (&cmd);
  c_str_destroy (&cflags);
#ifndef _WIN32
  c_str_destroy (&output);
#else
  c_str_destroy (&obj_output);
  c_str_destroy (&pdb_output);
#endif

  return err;
}

CError
cbuild_target_link (CBuild* self, CTargetImpl* target)
{
  CError err = CERROR_none;

  CArray cmd; // CArray < char* >
  c_array_error_t arr_err = c_array_create (sizeof (char*), &cmd);
  assert (arr_err.code == 0);

  // create the install path if not exists
  bool install_path_exists = false;
  c_fs_dir_exists (
      target->install_path.data, target->install_path.len, &install_path_exists
  );
  if (!install_path_exists)
    {
      c_fs_error_t fs_err =
          c_fs_dir_create (target->install_path.data, target->install_path.len);
      assert (fs_err.code == 0);
    }

  char const* lib_extenstion = NULL;
  char const* flag_output = default_builder->flags.output;

  // $ <link creator>
  if (target->ttype == CTARGET_TYPE_static)
    {
      arr_err = c_array_push (&cmd, &self->impl->cmds.static_lib_creator.data);
      assert (arr_err.code == 0);
      lib_extenstion = default_builder->extension.lib_static;
      /// FIXME: very ugly hack
      if (strcmp (
              "ar",
              &self->impl->cmds.static_lib_creator
                   .data[self->impl->cmds.static_lib_creator.len - 2]
          ) == 0)
        {
          flag_output = "";
        }
    }
  else if (target->ttype == CTARGET_TYPE_shared)
    {
      arr_err = c_array_push (&cmd, &self->impl->cmds.shared_lib_creator.data);
      assert (arr_err.code == 0);
      lib_extenstion = default_builder->extension.lib_shared;
    }
  else if (target->ttype == CTARGET_TYPE_executable)
    {
      arr_err = c_array_push (&cmd, &self->impl->cmds.linker.data);
      assert (arr_err.code == 0);
      lib_extenstion = default_builder->extension.exe;
    }
  else
    {
      err = CERROR_invalid_target_type;
      goto Error_cmd;
    }

  // $ <lib creator> <lflags>
  CStr lflags;
  c_str_error_t str_err = c_str_clone (&target->lflags, &lflags);
  assert (str_err.code == 0);
  char* subtoken = strtok (lflags.data, C_STR_WHITESPACES);
  if (subtoken)
    {
      do
        {
          arr_err = c_array_push (&cmd, &subtoken);
          assert (arr_err.code == 0);
        }
      while ((subtoken = strtok (NULL, C_STR_WHITESPACES)));
    }

  CStr output;
  str_err = c_str_create (STR (""), &output);
  assert (str_err.code == 0);

  char path_separator = c_fs_path_get_separator ();

  // -o<install_path>/lib<name>.so
  // -o<install path>/<name>
  // <install path>/<name>.a
  str_err = c_str_format (
      &output,
      0,
      STR_INV ("%s%s%c%s%s%s"),
      flag_output,
      target->install_path.data,
      path_separator,
      target->ttype != CTARGET_TYPE_executable ? lib_prefix : "",
      target->name.data,
      lib_extenstion
  );
  assert (str_err.code == 0);

  arr_err = c_array_push (&cmd, &output.data);
  assert (arr_err.code == 0);

  // $ <lib creator> <lflags> <object files>
  // object files
  size_t start_cmd_len = cmd.len;
  c_fs_foreach (
      target->build_path.data,
      target->build_path.len,
      target->build_path.capacity,
      &internal_find_and_push_all_compiled_objects_handler,
      (void*) &cmd
  );
  size_t end_cmd_len = cmd.len;

  // $ <lib creator> <lflags> <object files> <link with>
  CStr link_with;
  str_err = c_str_clone (&target->link_with, &link_with);
  assert (str_err.code == 0);
  subtoken = strtok (link_with.data, C_STR_WHITESPACES);
  if (subtoken)
    {
      do
        {
          arr_err = c_array_push (&cmd, &subtoken);
          assert (arr_err.code == 0);
        }
      while ((subtoken = strtok (NULL, C_STR_WHITESPACES)));
    }

  arr_err = c_array_push (&cmd, &(void*){ NULL });
  assert (arr_err.code == 0);

  int process_status = cprocess_exec ((char const* const*) cmd.data, cmd.len);
  assert (process_status == 0);

  // free
  c_str_destroy (&lflags);
  c_str_destroy (&link_with);
  c_str_destroy (&output);

  // free the strdup we created at the handler
  for (size_t i = start_cmd_len; i < end_cmd_len; ++i)
    {
      free (((char**) (cmd.data))[i]);
    }

Error_cmd:
  c_array_destroy (&cmd);

  return err;
}

// ------------------------------------------------------------------------//
// ------------------------------ Internals -------------------------------//
// ------------------------------------------------------------------------//

c_fs_error_t
internal_find_and_push_all_compiled_objects_handler (
    char path[], size_t path_len, void* extra_data
)
{
  (void) path_len;

  c_fs_error_t fs_err = { 0 };
  CArray* cmd = extra_data;

  if (strstr (path, default_builder->extension.object))
    {
      c_array_error_t arr_err = c_array_push (cmd, &(char*){ strdup (path) });
      assert (arr_err.code == 0);
    }

  return fs_err;
}

c_fs_error_t
internal_find_and_push_all_compiled_objects_cstr_handler (
    char* path, size_t path_len, void* extra_data
)
{
  (void) path_len;

  c_fs_error_t fs_err = { 0 };
  CArray* sources = extra_data;

  if (strstr (path, default_builder->extension.object))
    {
      CStr object;
      c_str_error_t str_err = c_str_create (path, path_len, &object);
      assert (str_err.code == 0);
      c_array_error_t arr_err = c_array_push (sources, &object);
      assert (arr_err.code == 0);
    }

  return fs_err;
}

CError
internal_cbuild_get_path (
    CTargetImpl* target, char const build_install_dir_name[], CStr* out_path
)
{
  *out_path = (CStr){ 0 };

  // <build_install_dir_name>
  c_str_error_t str_err =
      c_str_create_empty (c_fs_path_get_max_len (), out_path);
  assert (str_err.code == 0);

  char separator = c_fs_path_get_separator ();

  // <build_install_dir_name>/<target name>
  str_err = c_str_format (
      out_path,
      0,
      STR_INV ("%s%c%s"),
      build_install_dir_name,
      separator,
      target->name.data
  );
  assert (str_err.code == 0);

  return CERROR_none;
}

CError
internal_find_build_function_name (
    char build_file_path[],
    size_t build_file_path_len,
    CStr* out_function_name_buf
)
{
  char* orig_buf_data = NULL;
  CStr buf;
  c_str_error_t str_err = c_str_create_empty (BUFSIZ, &buf);
  assert (str_err.code == 0);

  CFile build_file;
  c_fs_error_t fs_err =
      c_fs_file_open (build_file_path, build_file_path_len, "r", &build_file);
  assert (fs_err.code == 0);
  fs_err = c_fs_file_read (&build_file, buf.data, buf.capacity, &buf.len);
  assert (str_err.code == 0);
  orig_buf_data = buf.data;

  // CError\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\(CBuild\s*\*\s*[a-zA-Z_][a-zA-Z0-9_]*\s*\)

  bool found = false;
  while ((str_err = c_str_find (&buf, STR ("CError"), &buf.data)).code == 0 &&
         buf.data)
    {
      buf.data += sizeof ("CError") - 1;
      buf.data = c_skip_whitespaces (buf.data);

      if (!(((*buf.data <= 'z') && (*buf.data >= 'a')) ||
            ((*buf.data <= 'Z') && (*buf.data >= 'A')) || *buf.data != '_'))
        {
          continue;
        }
      str_err = c_str_append_with_cstr (out_function_name_buf, buf.data, 1);
      assert (str_err.code == 0);
      buf.data++;

      while (((*buf.data <= 'z') && (*buf.data >= 'a')) ||
             ((*buf.data <= '9') && (*buf.data >= '0')) ||
             ((*buf.data <= 'Z') && (*buf.data >= 'A')) || *buf.data == '_')
        {
          str_err = c_str_append_with_cstr (out_function_name_buf, buf.data, 1);
          assert (str_err.code == 0);
          buf.data++;
        }
      out_function_name_buf->data[out_function_name_buf->len] = '\0';

      buf.data = c_skip_whitespaces (buf.data);

      if (*buf.data != '(')
        {
          continue;
        }
      buf.data++;

      if (strncmp (buf.data, STR ("CBuild")) != 0)
        {
          continue;
        }
      buf.data += sizeof ("CBuild") - 1;

      buf.data = c_skip_whitespaces (buf.data);
      if (*buf.data != '*')
        {
          continue;
        }
      buf.data++;
      buf.data = c_skip_whitespaces (buf.data);

      if (!(((*buf.data <= 'z') && (*buf.data >= 'a')) ||
            ((*buf.data <= 'Z') && (*buf.data >= 'A')) || *buf.data != '_'))
        {
          continue;
        }
      buf.data++;

      while (((*buf.data <= 'z') && (*buf.data >= 'a')) ||
             ((*buf.data <= '9') && (*buf.data >= '0')) ||
             ((*buf.data <= 'Z') && (*buf.data >= 'A')) || *buf.data == '_')
        {
          buf.data++;
        }

      buf.data = c_skip_whitespaces (buf.data);

      if (*buf.data != ')')
        {
          continue;
        }
      buf.data++;

      found = true;
      break;
    }
  assert (str_err.code == 0);

  // Error:
  buf.data = orig_buf_data;
  c_fs_file_close (&build_file);
  c_str_destroy (&buf);

  return found ? CERROR_none : CERROR_build_function_not_found;
}

CError
internal_compile_install_build_c (
    CBuild* self, CStr* out_cbuild_dll_dir, CStr* build_fn_name
)
{
  /// check build.c existance
  bool is_build_c_exists;
  c_fs_exists (STR ("./build.c"), &is_build_c_exists);
  assert (is_build_c_exists);

  // get main function inside build.c (that one responsible of building)
  CStr build_function_name;
  c_str_error_t str_err =
      c_str_create_empty (MAX_BUILD_FUNCTION_NAME_LEN, &build_function_name);
  assert (str_err.code == 0);
  CError err = internal_find_build_function_name (
      STR ("./build.c"), &build_function_name
  );
  assert (err.code == 0);

  /// create a shared library for build.c
  CTarget build_target;
  err = cbuild_shared_lib_create (
      self, STR (default_build_c_target_name), STR ("."), &build_target
  );
  assert (err.code == 0);

  err = cbuild_target_add_source (self, &build_target, STR ("build.c"));
  assert (err.code == 0);

  /// compile flags
  err = cbuild_target_add_compile_flag (
      self, &build_target, STR (default_pic_flag)
  );
  assert (err.code == 0);

#ifdef _WIN32
  /// get current executable path
  CStr cur_exe_dir;
  str_err = c_str_create_empty (c_fs_path_get_max_len (), &cur_exe_dir);
  assert (str_err.code == 0);
  c_fs_error_t fs_err = c_fs_get_current_exe_path (
      cur_exe_dir.data, cur_exe_dir.capacity, &cur_exe_dir.len
  );
  assert (fs_err.code == 0);
  fs_err = c_fs_path_get_parent (
      cur_exe_dir.data, cur_exe_dir.len, &cur_exe_dir.len
  );
  assert (fs_err.code == 0);

  // cflags
  CStr cflags;
  str_err = c_str_create_empty (c_fs_path_get_max_len (), &cflags);
  char path_separator = c_fs_path_get_separator ();

  // /I<include folder>
  str_err = c_str_format (
      &cflags,
      0,
      STR_INV ("%s%c%s%c%s"),
      cur_exe_dir.data,
      path_separator,
      "..",
      path_separator,
      "include"
  );
  assert (str_err.code == 0);
  err = cbuild_target_add_include_path_flag (
      self, &build_target, cflags.data, cflags.len
  );
  assert (err.code == 0);
  c_str_destroy (&cflags);

  // lflags
  CStr lflags;
  str_err = c_str_create_empty (c_fs_path_get_max_len (), &lflags);
  assert (str_err.code == 0);

  str_err = c_str_format (
      &lflags,
      0,
      STR_INV ("/EXPORT:%s %s%c%s%c%s%c%s"),
      build_function_name.data,
      cur_exe_dir.data,
      path_separator,
      "..",
      path_separator,
      "lib",
      path_separator,
      "cbuild.lib"
  );
  assert (str_err.code == 0);

  err = cbuild_target_add_link_flag (
      self, &build_target, lflags.data, lflags.len
  );
  assert (err.code == 0);
  c_str_destroy (&lflags);
  c_str_destroy (&cur_exe_dir);
#endif

  err = cbuild_target_build (self, build_target.impl);
  assert (err.code == 0);

  str_err = c_str_clone (&build_target.impl->install_path, out_cbuild_dll_dir);
  assert (str_err.code == 0);
  *build_fn_name = build_function_name;

  cbuild_target_destroy (self, &build_target);

  return CERROR_none;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
