#include "ccmd.h"
#include "cbuild.h"
#include "cbuild_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dl_loader.h>
#include <fs.h>

#define STR(s) (s), sizeof (s) - 1
#define ON_ERR(err)                                                            \
  ((err.code != 0) ? (fprintf (stderr, "%s\n", err.msg), abort ()) : (void) 0)

static CError internal_ccmd_on_init (CCmd* self);
static CError internal_ccmd_on_build (CCmd* self);
static CError internal_ccmd_on_run (CCmd* self);
static CError internal_ccmd_on_test (CCmd* self);
static CError internal_ccmd_on_doc (CCmd* self);
static CError internal_ccmd_on_fmt (CCmd* self);
static CError internal_ccmd_on_help (CCmd* self);
static CError internal_ccmd_on_version (CCmd* self);
static void internal_ccmd_on_fatal_error (
    char const* category, char const* msg
);

CError
ccmd_create (int argc, char* argv[], CCmd* out_ccmd)
{
  if (argc <= 1)
    {
      internal_ccmd_on_help (NULL);
      exit (EXIT_FAILURE);
    }

  struct
  {
    char const* const subcmd;
    CError (*handler) (CCmd* self);
  } const subcmds[] = {
    { "init", internal_ccmd_on_init }, { "build", internal_ccmd_on_build },
    { "run", internal_ccmd_on_run },   { "test", internal_ccmd_on_test },
    { "doc", internal_ccmd_on_doc },   { "fmt", internal_ccmd_on_fmt },
    { "help", internal_ccmd_on_help }, { "version", internal_ccmd_on_version },
  };
  size_t const subcmds_len = sizeof (subcmds) / sizeof (subcmds[0]);

  if (out_ccmd)
    {
      *out_ccmd = (CCmd){ 0 };

      for (size_t iii = 0; iii < subcmds_len; ++iii)
        {
          if (strncmp (
                  argv[1], subcmds[iii].subcmd, sizeof (subcmds[iii].subcmd) - 1
              ) == 0)
            {
              subcmds[iii].handler (out_ccmd);
              exit (EXIT_SUCCESS);
            }
        }
    }

  internal_ccmd_on_help (NULL);
  exit (EXIT_FAILURE);
}

void
ccmd_destroy (CCmd* self)
{
}

CError
internal_ccmd_on_init (CCmd* self)
{
}

CError
internal_ccmd_on_build (CCmd* self)
{
  CBuild cbuild;
  CError err = cbuild_create (CBUILD_TYPE_debug, ".", 1, &cbuild);
  ON_ERR (err);

  /// get current executable path
  CStr cur_exe_dir;
  c_str_error_t str_err =
      c_str_create_empty (c_fs_path_get_max_len (), &cur_exe_dir);
  ON_ERR (str_err);
  c_fs_error_t fs_err = c_fs_get_current_exe_path (
      cur_exe_dir.data, cur_exe_dir.capacity, &cur_exe_dir.len
  );
  ON_ERR (fs_err);
  fs_err = c_fs_path_get_parent (
      cur_exe_dir.data, cur_exe_dir.len, &cur_exe_dir.len
  );
  ON_ERR (fs_err);
  puts (cur_exe_dir.data);

  /// check build.c existance
  bool is_build_c_exists;
  c_fs_exists (STR ("./build.c"), &is_build_c_exists);
  if (!is_build_c_exists)
    {
      internal_ccmd_on_fatal_error ("build.c", "No such file or directory");
    }

  /// create a shared library for build.c
  CBuildTarget build_target;
  err = cbuild_shared_lib_create (&cbuild, STR ("_"), &build_target);
  ON_ERR (err);

  err = cbuild_target_add_source (&cbuild, &build_target, STR ("build.c"));
  ON_ERR (err);

  // compile flags
  CStr cflags;
  char path_separator;
  c_fs_path_get_separator (&path_separator);
  c_str_create_empty (c_fs_path_get_max_len (), &cflags);
  cflags.len = snprintf (
      cflags.data,
      cflags.capacity,
      "-fPIC -I%s%c%s",
      cur_exe_dir.data,
      path_separator,
      "include"
  );
  err = cbuild_target_add_compile_flag (
      &cbuild, &build_target, cflags.data, cflags.len
  );
  c_str_destroy (&cflags);

  err = cbuild_build (&cbuild);
  ON_ERR (err);

#ifdef WIN32_t
  char const cbuild_dll_name[] = "_.dll";
#else
  char const cbuild_dll_name[] = "lib_.so";
#endif

  CStr cbuild_dll_path;
  str_err = c_str_clone (&build_target.impl->install_path, &cbuild_dll_path);
  ON_ERR (str_err);
  str_err = c_str_set_capacity (
      &cbuild_dll_path, cbuild_dll_path.capacity + sizeof (cbuild_dll_name)
  );
  ON_ERR (str_err);

  fs_err = c_fs_path_append (
      cbuild_dll_path.data,
      cbuild_dll_path.len,
      cbuild_dll_path.capacity,
      cbuild_dll_name,
      sizeof (cbuild_dll_name) - 1,
      &cbuild_dll_path.len
  );
  ON_ERR (fs_err);

  CDLLoader dll_loader;
  c_dl_error_t dl_err = c_dl_loader_create (
      cbuild_dll_path.data, cbuild_dll_path.len, &dll_loader
  );
  ON_ERR (dl_err);

  CError (*build_fn) (CBuild*);
  dl_err = c_dl_loader_get (&dll_loader, STR ("build"), (void*) &build_fn);
  ON_ERR (dl_err);

  err = build_fn (&cbuild);
  ON_ERR (err);

  err = cbuild_build (&cbuild);
  ON_ERR (err);

  // free
  c_dl_loader_destroy (&dll_loader);
  c_str_destroy (&cbuild_dll_path);
  c_str_destroy (&cflags);
  c_str_destroy (&cur_exe_dir);
  cbuild_target_destroy (&cbuild, &build_target);
  cbuild_destroy (&cbuild);
}

CError
internal_ccmd_on_run (CCmd* self)
{
}

CError
internal_ccmd_on_test (CCmd* self)
{
}

CError
internal_ccmd_on_doc (CCmd* self)
{
}

CError
internal_ccmd_on_fmt (CCmd* self)
{
}

CError
internal_ccmd_on_help (CCmd* self)
{
  (void) self;

  printf ("Usage: c [command] [options]\n\n"
          "Commands:\n\n"
          "  init\t\tInitialize project in the current directory\n"
          "  build\t\tBuild project from build.c\n"
          "  run\t\tBuild and Run\n"
          "  test\t\tPerform unit testing\n"
          "  doc\t\tGenerate doc and open it in browser\n"
          "  fmt\t\tFormat the current project\n"
          "  help\t\tPrint this message\n"
          "  version\tPrint c version\n");
}

CError
internal_ccmd_on_version (CCmd* self)
{
  (void) self;

  printf ("Version %s\n", C_VER);
}

void
internal_ccmd_on_fatal_error (char const category[], char const msg[])
{
  fprintf (stderr, "Error(%s): %s\n", category, msg);
  exit (EXIT_FAILURE);
}
