#include "ccmd.h"
#include "cbuild.h"
#include "cbuild_private.h"
#include "helpers.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <defer.h>
#include <dl_loader.h>
#include <fs.h>

#define IS_HELP(s) ((strcmp((s), "--help")) || (strcmp((s), "-h") == 0))
#define ON_EXTRA_PARAM_ERR "error: unexpected extra parameter"
#define ON_ERR(err)                                                            \
  (fprintf(stderr, "Error: %d\n---\n%s\n", err.code, err.desc),                \
   fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __func__),                \
   (exit_status = EXIT_FAILURE))

// clang-format off
static char const* subcmd_helps[] = {
  [CSUB_CMD_init] = "Usage: c init\n\n"
    "  Initializes a sample C project in the current working\n"
    "  directory.\n\n"
    "Options:\n"
    "-h, --help             Print this help and exit\n",
  [CSUB_CMD_build] = "",
  [CSUB_CMD_run] = "",
  [CSUB_CMD_test] = "",
  [CSUB_CMD_doc] = "",
  [CSUB_CMD_fmt] = "",
  [CSUB_CMD_help] = "",
  [CSUB_CMD_version] = "",
};
// clang-format on

static int
internal_ccmd_on_init(CCmd* self);
static int
internal_ccmd_on_build(CCmd* self);
static int
internal_ccmd_on_run(CCmd* self);
static int
internal_ccmd_on_test(CCmd* self);
static int
internal_ccmd_on_doc(CCmd* self);
static int
internal_ccmd_on_fmt(CCmd* self);
static int
internal_ccmd_on_help(CCmd* self);
static int
internal_ccmd_on_version(CCmd* self);

CError
ccmd_create(int argc, char* argv[], CCmd* out_ccmd)
{
  if (argc <= 1) {
    internal_ccmd_on_help(NULL);
    return CERROR_wrong_options;
  }

  if (!out_ccmd) {
    internal_ccmd_on_help(NULL);
    return CERROR_none;
  }

  struct
  {
    char const* const subcmd;
    int (*handler)(CCmd* self);
  } const subcmds[] = {
    { "init", internal_ccmd_on_init }, { "build", internal_ccmd_on_build },
    { "run", internal_ccmd_on_run },   { "test", internal_ccmd_on_test },
    { "doc", internal_ccmd_on_doc },   { "fmt", internal_ccmd_on_fmt },
    { "help", internal_ccmd_on_help }, { "version", internal_ccmd_on_version },
  };
  size_t const subcmds_len = sizeof(subcmds) / sizeof(subcmds[0]);

  *out_ccmd = (CCmd){ 0 };
  out_ccmd->argc = argc - 2;
  out_ccmd->argv = argv + 2;

  for (size_t iii = 0; iii < subcmds_len; ++iii) {
    if (strncmp(
          argv[1], subcmds[iii].subcmd, sizeof(subcmds[iii].subcmd) - 1) == 0) {
      out_ccmd->subcmd = (CSubCmd)iii;
      if (subcmds[iii].handler(out_ccmd) != EXIT_SUCCESS) {
        return CERROR_failed_command;
      }
      break;
    }
  }

  return CERROR_none;
}

void
ccmd_destroy(CCmd* self)
{
  assert(self && self->argv);

  *self = (CCmd){ 0 };
}

int
internal_ccmd_on_init(CCmd* self)
{
  CStr project_path = { 0 };
  int exit_status = EXIT_SUCCESS;
  c_str_error_t str_err = C_STR_ERROR_none;
  c_fs_error_t fs_err = C_FS_ERROR_none;

  c_defer_init(10);

  if (self->argc >= 1) {
    if (IS_HELP(self->argv[0])) {
      puts(subcmd_helps[self->subcmd]);
      c_defer_check(false, NULL, NULL, exit_status = EXIT_SUCCESS);
    } else {
      printf("%s%s\n", ON_EXTRA_PARAM_ERR, self->argv[0]);
      c_defer_check(false, NULL, NULL, exit_status = EXIT_FAILURE);
    }
  } else {
    str_err = c_str_create_empty(c_fs_path_get_max_len(), &project_path);
    c_defer_err(str_err.code == 0,
                c_str_destroy,
                &project_path,
                exit_status = EXIT_FAILURE);
    fs_err = c_fs_dir_get_current(
      project_path.data, project_path.capacity, &project_path.len);
    c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));
  }

  /// <project folder>/build.c
  size_t const orig_len = project_path.len;
  fs_err = c_fs_path_append(project_path.data,
                            project_path.len,
                            project_path.capacity,
                            C_STR("build.c"),
                            &project_path.len);
  c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));

  CFile build_c_file = { 0 };
  fs_err =
    c_fs_file_open(project_path.data, project_path.len, "w", &build_c_file);
  c_defer_err(fs_err.code == 0,
              NULL,
              NULL,
              (c_fs_file_close(&build_c_file), ON_ERR(fs_err)));

  fs_err = c_fs_file_write(
    &build_c_file,
    // clang-format off
      C_STR ("#include \"cbuild.h\"\n\n"
           "CError build (CBuild* self)\n"
           "{\n"
           "  CTarget target;\n"
           "  CError err = cbuild_exe_create (\n"
           "    self, \"target\", sizeof (\"target\") - 1, &target);\n"
           "  if (err.code != 0)\n"
           "    {\n"
           "      return err;\n"
           "    }\n\n"
           "  err = cbuild_target_add_source (\n"
           "    self, &target, \"src.module1/main.c\", sizeof (\"src/module1/main.c\") - 1);\n"
           "  if (err.code != 0)\n"
           "    {\n"
           "      return err;\n"
           "    }\n\n"
           "  return CERROR_none;\n"
           "}\n"),
    // clang-format on
    NULL);
  c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));

  /// <project folder>
  project_path.len = orig_len;
  project_path.data[project_path.len] = '\0';
  /// <project folder>/src
  fs_err = c_fs_path_append(project_path.data,
                            project_path.len,
                            project_path.capacity,
                            C_STR("src"),
                            &project_path.len);
  c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));
  fs_err = c_fs_dir_create(project_path.data, project_path.len);
  c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));

  /// <project folder>/src/module1
  fs_err = c_fs_path_append(project_path.data,
                            project_path.len,
                            project_path.capacity,
                            C_STR("module1"),
                            &project_path.len);
  c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));
  fs_err = c_fs_dir_create(project_path.data, project_path.len);
  c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));

  /// <project folder>/src/module1/example.c
  fs_err = c_fs_path_append(project_path.data,
                            project_path.len,
                            project_path.capacity,
                            C_STR("example.c"),
                            &project_path.len);
  c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));

  CFile main_c_file = { 0 };
  fs_err =
    c_fs_file_open(project_path.data, project_path.len, "w", &main_c_file);
  c_defer_err(fs_err.code == 0,
              NULL,
              NULL,
              (c_fs_file_close(&main_c_file), ON_ERR(fs_err)));

  fs_err = c_fs_file_write(&main_c_file,
                           C_STR("#include <stdio.h>\n\n"
                                 "int main() {\n"
                                 "  puts(\"Hello World\");\n"
                                 "}\n"),
                           NULL);
  c_defer_check(fs_err.code == 0, NULL, NULL, ON_ERR(fs_err));

  c_defer_deinit();

  return exit_status;
}

int
internal_ccmd_on_build(CCmd* self)
{
  int exit_status = EXIT_SUCCESS;

  CError err = CERROR_none;

  c_defer_init(10);

  /// FIXME: "." should be taken as a parameter
  char project_path[] = ".";

  CBuild cbuild = { 0 };
  /// FIXME: this should not be debug
  err = cbuild_create(CBUILD_TYPE_debug, C_STR(project_path), &cbuild);
  c_defer_err(err.code == 0, cbuild_destroy, &cbuild, ON_ERR(err));

  err = cbuild_configure(&cbuild);
  c_defer_check(err.code == 0, NULL, NULL, ON_ERR(err));
  err = cbuild_build(&cbuild);
  c_defer_check(err.code == 0, NULL, NULL, ON_ERR(err));

  c_defer_deinit();

  return exit_status;
}

int
internal_ccmd_on_run(CCmd* self)
{
  return EXIT_SUCCESS;
}

int
internal_ccmd_on_test(CCmd* self)
{
  return EXIT_SUCCESS;
}

int
internal_ccmd_on_doc(CCmd* self)
{
  return EXIT_SUCCESS;
}

int
internal_ccmd_on_fmt(CCmd* self)
{
  return EXIT_SUCCESS;
}

int
internal_ccmd_on_help(CCmd* self)
{
  (void)self;

  printf("Usage: c [command] [options]\n\n"
         "Commands:\n\n"
         "  init\t\tInitialize project in the current directory\n"
         "  build\t\tBuild project from build.c\n"
         "  run\t\tBuild and Run\n"
         "  test\t\tPerform unit testing\n"
         "  doc\t\tGenerate doc and open it in browser\n"
         "  fmt\t\tFormat the current project\n"
         "  help\t\tPrint this message\n"
         "  version\tPrint c version\n");

  return EXIT_SUCCESS;
}

int
internal_ccmd_on_version(CCmd* self)
{
  (void)self;

  printf("Version %s\n", C_VER);

  return EXIT_SUCCESS;
}
