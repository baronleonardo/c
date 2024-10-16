#include "ccmd.h"
#include "cbuild.h"
#include "cbuild_private.h"
#include "helpers.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dl_loader.h>
#include <fs.h>

#define STR(s) (s), sizeof (s) - 1
#define STR_INV(s) sizeof (s) - 1, (s)
#define ON_ERR(err)                                                            \
  ((err.code != 0)                                                             \
       ? (fprintf (stderr, "%d: %s\n", err.code, err.msg), abort ())           \
       : (void) 0)

static CError internal_ccmd_on_init (CCmd* self);
static CError internal_ccmd_on_build (CCmd* self);
static CError internal_ccmd_on_run (CCmd* self);
static CError internal_ccmd_on_test (CCmd* self);
static CError internal_ccmd_on_doc (CCmd* self);
static CError internal_ccmd_on_fmt (CCmd* self);
static CError internal_ccmd_on_help (CCmd* self);
static CError internal_ccmd_on_version (CCmd* self);

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
  /// TODO: we will have an option for custom path
  CStr project_path;
  c_str_error_t str_err =
      c_str_create_empty (c_fs_path_get_max_len (), &project_path);
  ON_ERR (str_err);
  c_fs_error_t fs_err = c_fs_dir_get_current (
      project_path.data, project_path.capacity, &project_path.len
  );
  ON_ERR (fs_err);

  size_t const orig_len = project_path.len;
  fs_err = c_fs_path_append (
      project_path.data,
      project_path.len,
      project_path.capacity,
      STR ("build.c"),
      &project_path.len
  );
  ON_ERR (fs_err);

  CFile file;
  fs_err = c_fs_file_open (project_path.data, project_path.len, "w", &file);
  ON_ERR (fs_err);

  fs_err = c_fs_file_write (
      &file,
      STR ("#include \"cbuild.h\"\n\n"
           "CError build(CBuild* self) {\n"
           "  CTarget target;\n"
           "  CError err = cbuild_exe_create (self, \"target\", "
           "sizeof(\"target\") - 1, &target);\n"
           "  if (err.code != 0)\n"
           "    {\n"
           "      return err;\n"
           "    }\n\n"
           "  err = cbuild_target_add_source (self, &target, "
           "\"module1/main.c\", "
           "  sizeof(\"module1/main.c\") - 1);\n"
           "  if (err.code != 0)\n"
           "    {\n"
           "      return err;\n"
           "    }\n\n"
           "  return CERROR_none;\n"
           "}\n"),
      NULL
  );
  ON_ERR (fs_err);

  fs_err = c_fs_file_close (&file);
  ON_ERR (fs_err);

  project_path.len = orig_len;
  project_path.data[project_path.len] = '\0';
  fs_err = c_fs_path_append (
      project_path.data,
      project_path.len,
      project_path.capacity,
      STR ("example"),
      &project_path.len
  );
  ON_ERR (fs_err);

  fs_err = c_fs_dir_create (project_path.data, project_path.len);
  ON_ERR (fs_err);

  // size_t example_dir_path_len = project_path.len;
  project_path.data[project_path.len] = '\0';
  fs_err = c_fs_path_append (
      project_path.data,
      project_path.len,
      project_path.capacity,
      STR ("example.c"),
      &project_path.len
  );
  ON_ERR (fs_err);

  fs_err = c_fs_file_open (project_path.data, project_path.len, "w", &file);
  ON_ERR (fs_err);

  fs_err = c_fs_file_write (
      &file,
      STR ("#include <stdio.h>\n\n"
           "int main() {\n"
           "  puts(\"Hello World\");\n"
           "}\n"),
      NULL
  );
  ON_ERR (fs_err);

  fs_err = c_fs_file_close (&file);
  ON_ERR (fs_err);

  return CERROR_none;
}

CError
internal_ccmd_on_build (CCmd* self)
{
  /// FIXME: "." should be taken as a parameter
  char project_path[] = ".";

  /// FIXME: this should not be debug
  CBuild cbuild;
  CError err = cbuild_create (CBUILD_TYPE_debug, STR (project_path), &cbuild);
  ON_ERR (err);

  err = cbuild_configure (&cbuild);
  ON_ERR (err);
  err = cbuild_build (&cbuild);
  ON_ERR (err);

  cbuild_destroy (&cbuild);

  return err;
}

CError
internal_ccmd_on_run (CCmd* self)
{
  return CERROR_none;
}

CError
internal_ccmd_on_test (CCmd* self)
{
  return CERROR_none;
}

CError
internal_ccmd_on_doc (CCmd* self)
{
  return CERROR_none;
}

CError
internal_ccmd_on_fmt (CCmd* self)
{
  return CERROR_none;
  // clang-format
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

  return CERROR_none;
}

CError
internal_ccmd_on_version (CCmd* self)
{
  (void) self;

  printf ("Version %s\n", C_VER);

  return CERROR_none;
}
