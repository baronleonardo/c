#include "ccmd.h"
#include "cbuild.h"
#include "cbuild_private.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dl_loader.h>
#include <fs.h>

#define STR(s) (s), sizeof (s) - 1
#define ON_ERR(err)                                                            \
  ((err.code != 0)                                                             \
       ? (fprintf (stderr, "%d: %s\n", err.code, err.msg), abort ())           \
       : (void) 0)
#define MAX_BUILD_FUNCTION_NAME_LEN 1000

#ifdef _WIN32
static char const default_pic_flag[] = "";
static char const default_include_dir_flag[] = "/I";
#else
static char const default_pic_flag[] = "-fPIC";
static char const default_include_dir_flag[] = "-I";
#endif

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
static CError internal_find_build_function_name (
    char build_file_path[],
    size_t build_file_path_len,
    CStr* out_function_name_buf
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
           "  CBuildTarget target;\n"
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
}

CError
internal_ccmd_on_build (CCmd* self)
{
  CBuild cbuild;
  CError err = cbuild_create (CBUILD_TYPE_debug, STR ("."), &cbuild);
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

  /// check build.c existance
  bool is_build_c_exists;
  c_fs_exists (STR ("./build.c"), &is_build_c_exists);
  if (!is_build_c_exists)
    {
      internal_ccmd_on_fatal_error ("build.c", "No such file or directory");
    }

  CStr build_function_name;
  str_err =
      c_str_create_empty (MAX_BUILD_FUNCTION_NAME_LEN, &build_function_name);
  ON_ERR (str_err);
  err = internal_find_build_function_name (
      STR ("./build.c"), &build_function_name
  );
  ON_ERR (err);

  /// create a shared library for build.c
  CBuildTarget build_target;
  err = cbuild_shared_lib_create (&cbuild, STR ("_"), &build_target);
  ON_ERR (err);

  err = cbuild_target_add_source (&cbuild, &build_target, STR ("build.c"));
  ON_ERR (err);

  /// compile flags
  CStr cflags;
  char path_separator;
  c_fs_path_get_separator (&path_separator);
  c_str_create_empty (c_fs_path_get_max_len (), &cflags);
  // "-fpic -I<exe dir>/include"
  cflags.len = snprintf (
      cflags.data,
      cflags.capacity,
      "%s %s%s%c%s",
      default_pic_flag,
      default_include_dir_flag,
      cur_exe_dir.data,
      path_separator,
      "include"
  );
  err = cbuild_target_add_compile_flag (
      &cbuild, &build_target, cflags.data, cflags.len
  );
  ON_ERR (err);
  c_str_destroy (&cflags);

#ifdef _WIN32
  err = cbuild_target_add_link_flag (
      &cbuild, &build_target, STR (" /FORCE:UNRESOLVED /EXPORT:build ")
  );
  ON_ERR (err);
#endif

  err = cbuild_build (&cbuild);
  ON_ERR (err);

#ifdef _WIN32
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
  dl_err = c_dl_loader_get (
      &dll_loader,
      build_function_name.data,
      build_function_name.len,
      (void*) &build_fn
  );
  ON_ERR (dl_err);

  err = build_fn (&cbuild);
  ON_ERR (err);

  err = cbuild_build (&cbuild);
  ON_ERR (err);

  // free
  c_dl_loader_destroy (&dll_loader);
  c_str_destroy (&build_function_name);
  c_str_destroy (&cbuild_dll_path);
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

char*
internal_skip_whitespaces (char* needle)
{
  while (*needle && isspace (*needle))
    {
      needle++;
    }

  return needle;
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
  ON_ERR (str_err);

  CFile build_file;
  c_fs_error_t fs_err =
      c_fs_file_open (build_file_path, build_file_path_len, "r", &build_file);
  ON_ERR (fs_err);
  fs_err = c_fs_file_read (&build_file, buf.data, buf.capacity, &buf.len);
  ON_ERR (str_err);
  orig_buf_data = buf.data;

  // CError\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\(CBuild\s*\*\s*[a-zA-Z_][a-zA-Z0-9_]*\s*\)

  bool found = false;
  while ((str_err = c_str_find (&buf, STR ("CError"), &buf.data)).code == 0 &&
         buf.data)
    {
      buf.data += sizeof ("CError") - 1;
      buf.data = internal_skip_whitespaces (buf.data);

      if (!(((*buf.data <= 'z') && (*buf.data >= 'a')) ||
            ((*buf.data <= 'Z') && (*buf.data >= 'A')) || *buf.data != '_'))
        {
          continue;
        }
      str_err = c_str_concatenate_with_cstr (
          out_function_name_buf, buf.data, 1, true
      );
      ON_ERR (str_err);
      buf.data++;

      while (((*buf.data <= 'z') && (*buf.data >= 'a')) ||
             ((*buf.data <= '9') && (*buf.data >= '0')) ||
             ((*buf.data <= 'Z') && (*buf.data >= 'A')) || *buf.data == '_')
        {
          str_err = c_str_concatenate_with_cstr (
              out_function_name_buf, buf.data, 1, true
          );
          ON_ERR (str_err);
          buf.data++;
        }
      out_function_name_buf->data[out_function_name_buf->len] = '\0';

      buf.data = internal_skip_whitespaces (buf.data);

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

      buf.data = internal_skip_whitespaces (buf.data);
      if (*buf.data != '*')
        {
          continue;
        }
      buf.data++;
      buf.data = internal_skip_whitespaces (buf.data);

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

      buf.data = internal_skip_whitespaces (buf.data);

      if (*buf.data != ')')
        {
          continue;
        }
      buf.data++;

      found = true;
      break;
    }
  ON_ERR (str_err);

  // Error:
  buf.data = orig_buf_data;
  c_fs_file_close (&build_file);
  c_str_destroy (&buf);

  return found ? CERROR_none : CERROR_build_function_not_found;
}
