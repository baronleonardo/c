#include "cbuild.h"
#include "cbuild_private.h"
#include "cerror.h"
#include "cprocess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs.h>

#if _WIN32 && (!_MSC_VER || !(_MSC_VER >= 1900))
#error "You need MSVC must be higher that or equal to 1900"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // disable warning about unsafe functions
#endif

static char const default_build_path[] = ".c_build";
static char const default_install_path[] = "c_out";
static char default_cflags_none[] = "";
static char default_lflags_none[] = "";
#ifdef _WIN32
static char default_cflags_debug[] = "/Zi /utf-8";
static char default_lflags_debug[] = "/PDB";
static char default_cflags_release[] = "/O2 /DNDEBUG";
static char default_lflags_release[] = "";
static char default_cflags_release_with_debug_info[] = "/O2 /Zi /DNDEBUG";
static char default_lflags_release_with_debug_info[] = "";
static char default_cflags_release_with_minimum_size[] = "/Os /DNDEBUG";
static char default_lflags_release_with_minimum_size[] = "";

static char const default_cmd_compiler[] = "cl";
static char const default_cmd_linker[] = "link";
static char const default_cmd_shared_library_creator[] = "link";
static char const default_cmd_static_library_creator[] = "lib";

static char const default_object_extension[] = ".obj";
static char const default_exe_extension[] = ".exe";
static char const default_shared_library_extension[] = ".dll";
static char const default_static_library_extension[] = ".lib";

static char const default_compiler_argument[] = "/c";
static char const default_out_argument[] = "/out:";
static char const default_object_argument[] = "";
static char const default_include_path_flag[] = "/I";
static char const default_link_library_path_flag[] = "/LIBPATH:";
static char const default_link_with_library_flag[] = "";
static char const default_shared_library_argument[] = "/DLL /DEBUG";
static char const default_static_library_argument[] = "";
#else
static char default_cflags_debug[] = "-g";
static char default_lflags_debug[] = "";
static char default_cflags_release[] = "-O3 -DNDEBUG";
static char default_lflags_release[] = "";
static char default_cflags_release_with_debug_info[] = "-O2 -g -DNDEBUG";
static char default_lflags_release_with_debug_info[] = "";
static char default_cflags_release_with_minimum_size[] = "-Os -DNDEBUG";
static char default_lflags_release_with_minimum_size[] = "";

static char const default_cmd_compiler[] = "gcc";
static char const default_cmd_linker[] = "gcc";
static char const default_cmd_shared_library_creator[] = "gcc";
static char const default_cmd_static_library_creator[] = "ar";

static char const default_object_extension[] = ".o";
static char const default_exe_extension[] = "";
static char const default_shared_library_extension[] = ".so";
static char const default_static_library_extension[] = ".a";

static char const default_compiler_argument[] = "-c";
static char const default_out_argument[] = "-o";
static char const default_object_argument[] = "";
static char const default_include_path_flag[] = "-I";
static char const default_link_library_path_flag[] = "-L";
static char const default_link_with_library_flag[] = "-l";
static char const default_shared_library_argument[] = "-shared";
static char const default_static_library_argument[] = "rcs";
#endif

#define STR(s) (s), sizeof (s) - 1
#define STR_INV(s) sizeof (s) - 1, (s)

static CError internal_cbuild_target_create (
    CBuild* self,
    char const* name,
    size_t name_len,
    CTarget* out_target,
    char const lflags[],
    size_t lflags_len,
    CTargetType ttype
);
static CError internal_cbuild_target_build (CBuild* self, CTargetImpl* target);
static CError internal_cbuild_target_compile (
    CBuild* self, CTargetImpl* target
);
static CError internal_cbuild_target_build_library (
    CBuild* self, CTargetImpl* target
);
static CError internal_cbuild_target_link (CBuild* self, CTargetImpl* target);
static CError internal_cbuild_get_path (
    CTargetImpl* target,
    char const* build_install_dir_name,
    size_t build_install_dir_name_len,
    CStr* out_path
);
static c_fs_error_t internal_find_and_push_all_compiled_objects_handler (
    char path[], size_t path_len, void* extra_data
);

#define CBUILD_CREATE_T(T)                                                     \
  /* cflags*/                                                                  \
  str_err = c_str_create (STR (default_cflags_##T), &out_cbuild->cflags);      \
  assert (str_err.code == 0);                                                  \
  /* lflags*/                                                                  \
  str_err = c_str_create (STR (default_lflags_##T), &out_cbuild->lflags);      \
  assert (str_err.code == 0);

CError
cbuild_create (
    CBuildType btype,
    char const base_path[],
    size_t base_path_len,
    CBuild* out_cbuild
)
{
  assert (base_path && base_path_len > 0);

  if (out_cbuild)
    {
      /// btype
      *out_cbuild = (CBuild){ .btype = btype };

      // base path to absolute
      c_str_error_t str_err =
          c_str_create (base_path, base_path_len, &out_cbuild->base_path);
      assert (str_err.code == 0);
      str_err =
          c_str_set_capacity (&out_cbuild->base_path, c_fs_path_get_max_len ());
      assert (str_err.code == 0);
      c_fs_error_t fs_err = c_fs_path_to_absolute (
          base_path,
          base_path_len,
          out_cbuild->base_path.data,
          out_cbuild->base_path.capacity,
          &out_cbuild->base_path.len
      );
      assert (fs_err.code == 0);

      /// compiler
      str_err =
          c_str_create (STR (default_cmd_compiler), &out_cbuild->cmds.compiler);
      assert (str_err.code == 0);

      /// linker
      str_err =
          c_str_create (STR (default_cmd_linker), &out_cbuild->cmds.linker);
      assert (str_err.code == 0);

      /// static library creator
      str_err = c_str_create (
          STR (default_cmd_static_library_creator),
          &out_cbuild->cmds.static_lib_creator
      );
      assert (str_err.code == 0);

      /// shared library creator
      str_err = c_str_create (
          STR (default_cmd_shared_library_creator),
          &out_cbuild->cmds.shared_lib_creator
      );
      assert (str_err.code == 0);

      switch (btype)
        {
        case CBUILD_TYPE_debug:
          CBUILD_CREATE_T (debug);
          break;

        case CBUILD_TYPE_release:
          CBUILD_CREATE_T (release);
          break;

        case CBUILD_TYPE_release_with_debug_info:
          CBUILD_CREATE_T (release_with_debug_info);
          break;

        case CBUILD_TYPE_release_with_minimum_size:
          CBUILD_CREATE_T (release_with_minimum_size)
          break;

        default:
          CBUILD_CREATE_T (none);
          break;
        }

      /// targets
      c_array_error_t arr_err =
          c_array_create (sizeof (CTargetImpl*), &out_cbuild->targets);
      assert (arr_err.code == 0);

      return CERROR_none;
    }

  return CERROR_out_is_null;
}

CError
cbuild_object_create (
    CBuild* self, char const* name, size_t name_len, CTarget* out_target
)
{
  return internal_cbuild_target_create (
      self,
      name,
      name_len,
      out_target,
      STR (default_object_argument),
      CTARGET_TYPE_object
  );
}

CError
cbuild_static_lib_create (
    CBuild* self, char const* name, size_t name_len, CTarget* out_target
)
{
  return internal_cbuild_target_create (
      self,
      name,
      name_len,
      out_target,
      STR (default_static_library_argument),
      CTARGET_TYPE_static
  );
}

CError
cbuild_shared_lib_create (
    CBuild* self, char const* name, size_t name_len, CTarget* out_target
)
{
  return internal_cbuild_target_create (
      self,
      name,
      name_len,
      out_target,
      STR (default_shared_library_argument),
      CTARGET_TYPE_shared
  );
}

CError
cbuild_exe_create (
    CBuild* self, char const* name, size_t name_len, CTarget* out_target
)
{
  return internal_cbuild_target_create (
      self, name, name_len, out_target, STR (""), CTARGET_TYPE_executable
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
  assert (self && self->targets.data);
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
      c_str_error_t str_err = c_str_clone (&self->base_path, &target_path);
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
cbuild_target_add_include_dir (
    CBuild* self,
    CTarget* target,
    char const include_path[],
    size_t include_path_len
)
{
  assert (self && self->targets.data);
  assert (target && target->impl);

  if (include_path[include_path_len] != '\0')
    {
      return CERROR_invalid_string;
    }

  c_str_error_t str_err = c_str_format (
      &target->impl->cflags,
      target->impl->cflags.len,
      STR_INV (" %s%s"),
      default_include_path_flag,
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
  assert (self && self->targets.data);
  assert (target && target->impl);
  assert (depend_on && depend_on->impl);

  c_str_error_t str_err = C_STR_ERROR_none;
  c_fs_error_t fs_err = C_FS_ERROR_NONE;

  switch (property)
    {
    case CTARGET_PROPERTY_objects:
      {
        c_fs_error_t internal_find_and_push_all_compiled_objects_cstr_handler (
            char* path, size_t path_len, void* extra_data
        );
        fs_err = c_fs_foreach (
            depend_on->impl->build_path.data,
            depend_on->impl->build_path.len,
            depend_on->impl->build_path.capacity,
            internal_find_and_push_all_compiled_objects_cstr_handler,
            &target->impl->sources
        );
        assert (fs_err.code == 0);
      }
      break;
    case CTARGET_PROPERTY_library:
    case CTARGET_PROPERTY_library_with_rpath:
      {
        // -L<library path> -l<library>
        str_err = c_str_format (
            &target->impl->lflags,
            target->impl->lflags.len,
            STR_INV (" %s%s %s%s"),
            default_link_library_path_flag,
            depend_on->impl->install_path.data,
            default_link_with_library_flag,
            depend_on->impl->name.data
        );
        assert (str_err.code == 0);
#ifndef _WIN32
        if (property == CTARGET_PROPERTY_library_with_rpath)
          {
            // -Wl,-rpath,<library path>
            str_err = c_str_format (
                &target->impl->lflags,
                target->impl->lflags.len,
                STR_INV (" -Wl,-rpath,%s"),
                depend_on->impl->install_path.data
            );
            assert (str_err.code == 0);
          }
#endif
#ifdef _WIN32
        str_err = c_str_append_with_cstr (
            &target->impl->lflags, STR (default_static_library_extension)
        );
        assert (str_err.code == 0);
#endif
      }
      break;
    case CTARGET_PROPERTY_cflags:
      {
        str_err = c_str_format (
            &target->impl->cflags,
            target->impl->cflags.len,
            STR_INV (" %s"),
            depend_on->impl->cflags.data
        );
        assert (str_err.code == 0);
      }
      break;
    case CTARGET_PROPERTY_lflags:
      {
        str_err = c_str_format (
            &target->impl->lflags,
            target->impl->lflags.len,
            STR_INV (" %s"),
            depend_on->impl->lflags.data
        );
        assert (str_err.code == 0);
      }
      break;
    default:
      break;
    }

  return CERROR_none;
}

CError
cbuild_target_add_compile_flag (
    CBuild* self, CTarget* target, char const flag[], size_t flag_len
)
{
  assert (self && self->targets.data);
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
  assert (self && self->targets.data);
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

void
cbuild_target_destroy (CBuild* self, CTarget* target)
{
  assert (self && self->targets.data);

  // remove it from target list from `self`
  for (size_t i = 0; i < self->targets.len; ++i)
    {
      if (target->impl == ((CTargetImpl**) self->targets.data)[i])
        {
          c_array_error_t arr_err = c_array_remove (&self->targets, i);
          assert (arr_err.code == 0);
          break;
        }
    }

  c_str_destroy (&target->impl->name);
  c_str_destroy (&target->impl->build_path);
  c_str_destroy (&target->impl->install_path);
  c_str_destroy (&target->impl->cflags);
  c_str_destroy (&target->impl->lflags);
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
cbuild_build (CBuild* self)
{
  assert (self);

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
  fs_err = c_fs_dir_change_current (self->base_path.data, self->base_path.len);
  assert (fs_err.code == 0);

  // create build path if not existing
  CStr build_path;
  str_err = c_str_create (
      default_build_path, sizeof (default_build_path) - 1, &build_path
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

  /// build all targets
  for (size_t i = 0; i < self->targets.len; ++i)
    {
      err = internal_cbuild_target_build (
          self, ((CTargetImpl**) self->targets.data)[i]
      );
    }

  // restore old path
  fs_err = c_fs_dir_change_current (cur_dir_path.data, cur_dir_path.len);
  assert (fs_err.code == 0);

  // Error:
  c_str_destroy (&cur_dir_path);
  c_str_destroy (&build_path);
  c_str_destroy (&install_path);

  return err;
}

void
cbuild_destroy (CBuild* self)
{
  assert (self && self->targets.data);

  c_str_destroy (&self->base_path);
  c_str_destroy (&self->cflags);
  c_str_destroy (&self->lflags);
  c_str_destroy (&self->cmds.compiler);
  c_str_destroy (&self->cmds.linker);
  c_str_destroy (&self->cmds.shared_lib_creator);
  c_str_destroy (&self->cmds.static_lib_creator);

  // targets
  for (size_t i = 0; i < self->targets.len; ++i)
    {
      cbuild_target_destroy (
          self, &(CTarget){ ((CTargetImpl**) self->targets.data)[i] }
      );
    }
  c_array_destroy (&self->targets);

  *self = (CBuild){ 0 };
}

// ------------------------------------------------------------------------//
// ------------------------------ Internals -------------------------------//
// ------------------------------------------------------------------------//

CError
internal_cbuild_target_build (CBuild* self, CTargetImpl* target)
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
      internal_cbuild_target_build (
          self, ((CTargetImpl**) target->dependencies.data)[iii]
      );
    }

  CError err = CERROR_none;

  // change to build dir
  c_fs_error_t fs_err =
      c_fs_dir_change_current (target->build_path.data, target->build_path.len);
  assert (fs_err.code == 0);

  // compile
  err = internal_cbuild_target_compile (self, target);

  // restore dir
  fs_err = c_fs_dir_change_current (self->base_path.data, self->base_path.len);
  assert (fs_err.code == 0);

  if (target->ttype == CTARGET_TYPE_executable)
    {
      err = internal_cbuild_target_link (self, target);
    }
  else if (target->ttype != CTARGET_TYPE_object)
    {
      internal_cbuild_target_build_library (self, target);
    }

  return err;
}

CError
internal_cbuild_target_compile (CBuild* self, CTargetImpl* target)
{
  CError err = CERROR_none;

  CArray cmd; // CArray < char* >
  c_array_error_t arr_err = c_array_create (sizeof (char*), &cmd);
  assert (arr_err.code == 0);

  // $ <compiler>
  arr_err = c_array_push (&cmd, &self->cmds.compiler.data);
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
  arr_err = c_array_push (&cmd, &(char const*){ default_compiler_argument });
  assert (arr_err.code == 0);

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

  return err;
}

CError
internal_cbuild_target_build_library (CBuild* self, CTargetImpl* target)
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

  size_t old_target_install_path_len = target->install_path.len;

  // $ <lib creator>
  if (target->ttype == CTARGET_TYPE_static)
    {
      arr_err = c_array_push (&cmd, &self->cmds.static_lib_creator.data);
      assert (arr_err.code == 0);
    }
  else
    {
      arr_err = c_array_push (&cmd, &self->cmds.shared_lib_creator.data);
      assert (arr_err.code == 0);
    }

  // object files
  c_fs_foreach (
      target->build_path.data,
      target->build_path.len,
      target->build_path.capacity,
      &internal_find_and_push_all_compiled_objects_handler,
      (void*) &cmd
  );

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

#ifndef WIN32
  if (target->ttype != CTARGET_TYPE_static)
    {
      arr_err = c_array_push (&cmd, &(char const*){ default_out_argument });
      assert (arr_err.code == 0);
    }
#endif

  char const* lib_extenstion = target->ttype == CTARGET_TYPE_shared
                                   ? default_shared_library_extension
                                   : default_static_library_extension;

  CStr lib_out_name;
  str_err = c_str_create_empty (
#ifndef _WIN32
      strlen ("lib") +
#endif
          target->name.len + strlen (lib_extenstion) + 1,
      &lib_out_name
  );
  assert (str_err.code == 0);

#ifdef _WIN32
  lib_out_name.len =
      sprintf (lib_out_name.data, "%s%s", target->name.data, lib_extenstion);
#else
  lib_out_name.len = sprintf (
      lib_out_name.data, "%s%s%s", "lib", target->name.data, lib_extenstion
  );
#endif

  // install_path/<target_name>/<lib out name>
  c_fs_error_t fs_err = c_fs_path_append (
      target->install_path.data,
      target->install_path.len,
      target->install_path.capacity,
      lib_out_name.data,
      lib_out_name.len,
      &target->install_path.len
  );
  assert (fs_err.code == 0);

#ifdef WIN32
  char* tmp = malloc (target->install_path.len + sizeof (default_out_argument));
  if (!tmp)
    {
      return CERROR_memory_allocation;
    }

  snprintf (
      tmp,
      c_fs_path_get_max_len (),
      "%s%s",
      default_out_argument,
      target->install_path.data
  );

  arr_err = c_array_push (&cmd, &tmp);
  assert (arr_err.code == 0);
#else
  arr_err = c_array_push (&cmd, &target->install_path.data);
  assert (arr_err.code == 0);
#endif

  size_t current_cmd_len = cmd.len;

  arr_err = c_array_push (&cmd, &(void*){ NULL });
  assert (arr_err.code == 0);

  int process_status = cprocess_exec ((char const* const*) cmd.data, cmd.len);
  assert (process_status == 0);

  target->install_path.data[old_target_install_path_len] = '\0';
  target->install_path.len = old_target_install_path_len;

  // free
#ifdef WIN32
  free (tmp);
#endif

  c_str_destroy (&lflags);
  c_str_destroy (&lib_out_name);

  // free the strdup we created at the handler
  for (size_t i = current_cmd_len; i < cmd.len; ++i)
    {
      free (((char**) (cmd.data))[i]);
    }

  c_array_destroy (&cmd);

  return err;
}

CError
internal_cbuild_target_link (CBuild* self, CTargetImpl* target)
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

  size_t old_target_install_path_len = target->install_path.len;

  // $ <linker>
  arr_err = c_array_push (&cmd, &self->cmds.linker.data);
  assert (arr_err.code == 0);

  // object files
  c_fs_foreach (
      target->build_path.data,
      target->build_path.len,
      target->build_path.capacity,
      &internal_find_and_push_all_compiled_objects_handler,
      (void*) &cmd
  );

  // $ <compiler> <lflags>
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

#ifndef WIN32
  arr_err = c_array_push (&cmd, &(char const*){ default_out_argument });
  assert (arr_err.code == 0);
#endif

  // install_path/<target_name>/<target_name>
  c_fs_error_t fs_err = c_fs_path_append (
      target->install_path.data,
      target->install_path.len,
      target->install_path.capacity,
      target->name.data,
      target->name.len,
      &target->install_path.len
  );
  assert (fs_err.code == 0);

#ifdef WIN32
  char* tmp = malloc (target->install_path.len + sizeof (default_out_argument));
  if (!tmp)
    {
      return CERROR_memory_allocation;
    }

  snprintf (
      tmp,
      c_fs_path_get_max_len (),
      "%s%s%s",
      default_out_argument,
      target->install_path.data,
      default_exe_extension
  );

  arr_err = c_array_push (&cmd, &tmp);
  assert (arr_err.code == 0);
#else
  arr_err = c_array_push (&cmd, &target->install_path.data);
  assert (arr_err.code == 0);
#endif

  size_t current_cmd_len = cmd.len;

  arr_err = c_array_push (&cmd, &(void*){ NULL });
  assert (arr_err.code == 0);

  int process_status = cprocess_exec ((char const* const*) cmd.data, cmd.len);
  assert (process_status == 0);

  target->install_path.data[old_target_install_path_len] = '\0';
  target->install_path.len = old_target_install_path_len;

  // free
#ifdef _WIN32
  free (tmp);
#endif

  c_str_destroy (&lflags);

  // free the strdup we created at the handler
  for (size_t i = current_cmd_len; i < cmd.len; ++i)
    {
      free (((char**) (cmd.data))[i]);
    }

  c_array_destroy (&cmd);

  return err;
}

c_fs_error_t
internal_find_and_push_all_compiled_objects_handler (
    char path[], size_t path_len, void* extra_data
)
{
  (void) path_len;

  c_fs_error_t fs_err = { 0 };
  CArray* cmd = extra_data;

  if (strstr (path, default_object_extension))
    {
      c_array_error_t arr_err = c_array_push (cmd, &(char*){ strdup (path) });
      assert (arr_err.code == 0);
    }

  return fs_err;
}

CError
internal_cbuild_target_create (
    CBuild* self,
    char const* name,
    size_t name_len,
    CTarget* out_target,
    char const lflags[],
    size_t lflags_len,
    CTargetType ttype
)
{
  assert (self && self->targets.data);

  if (out_target)
    {
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

      // build path
      CError err = internal_cbuild_get_path (
          out_target->impl,
          STR (default_build_path),
          &out_target->impl->build_path
      );
      if (err.code != C_FS_ERROR_NONE.code)
        {
          return err;
        }

      // install path
      err = internal_cbuild_get_path (
          out_target->impl,
          STR (default_install_path),
          &out_target->impl->install_path
      );
      if (err.code != C_FS_ERROR_NONE.code)
        {
          return err;
        }

      // cflags
      str_err = c_str_clone (&self->cflags, &out_target->impl->cflags);
      assert (str_err.code == 0);
      // lflags
      str_err = c_str_create (lflags, lflags_len, &out_target->impl->lflags);
      assert (str_err.code == 0);

      // sources
      c_array_error_t arr_err =
          c_array_create (sizeof (CStr), &out_target->impl->sources);
      assert (arr_err.code == 0);

      // dependencies
      arr_err = c_array_create (
          sizeof (CTargetImpl*), &out_target->impl->dependencies
      );
      assert (arr_err.code == 0);

      arr_err = c_array_push (&self->targets, &out_target->impl);
      assert (arr_err.code == 0);

      return CERROR_none;
    }

  return CERROR_out_is_null;
}

CError
internal_cbuild_get_path (
    CTargetImpl* target,
    char const* build_install_dir_name,
    size_t build_install_dir_name_len,
    CStr* out_path
)
{
  *out_path = (CStr){ 0 };

  // <build_install_dir_name>
  c_str_error_t str_err = c_str_create (
      build_install_dir_name, build_install_dir_name_len, out_path
  );
  assert (str_err.code == 0);
  str_err = c_str_set_capacity (out_path, c_fs_path_get_max_len ());
  assert (str_err.code == 0);

  // <build_install_dir_name>/<target name>
  c_fs_error_t fs_err = c_fs_path_append (
      out_path->data,
      out_path->len,
      out_path->capacity,
      target->name.data,
      target->name.len,
      &out_path->len
  );
  assert (fs_err.code == 0);

  return CERROR_none;
}

c_fs_error_t
internal_find_and_push_all_compiled_objects_cstr_handler (
    char* path, size_t path_len, void* extra_data
)
{
  (void) path_len;

  c_fs_error_t fs_err = { 0 };
  CArray* sources = extra_data;

  if (strstr (path, default_object_extension))
    {
      CStr object;
      c_str_error_t str_err = c_str_create (path, path_len, &object);
      assert (str_err.code == 0);
      c_array_error_t arr_err = c_array_push (sources, &object);
      assert (arr_err.code == 0);
    }

  return fs_err;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
