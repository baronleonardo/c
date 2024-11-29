#ifndef CBUILDER_PRIVATE_H
#define CBUILDER_PRIVATE_H

typedef struct CBuilder {
  struct {
    char const* none;
    char const* debug;
    char const* release;
    char const* release_with_debug_info;
    char const* release_with_minimum_size;
    char const* compile;
    char const* include_path;
  } cflags;

  struct {
    char const* none;
    char const* debug;
    char const* release;
    char const* release_with_debug_info;
    char const* release_with_minimum_size;
    char const* library_path;
    char const* library;
  } lflags;

  struct {
    char const* output;
    char const* shared_library;
    char const* static_library;
  } flags;

  struct {
    char const* exe;
    char const* object;
    char const* lib_shared;
    char const* lib_static;
  } extension;

  char const* compiler;
  char const* linker;
  char const* archiver_shared;
  char const* archiver_static;
} CBuilder;

enum CBuilderType {
  CBUILDER_TYPE_gcc,
  CBUILDER_TYPE_clang,
  CBUILDER_TYPE_msvc,
} CBuilderType;

static CBuilder builders[] = {
  [CBUILDER_TYPE_gcc] = {
      .compiler = "gcc",
      .linker = "gcc",
      .archiver_shared = "gcc",
      .archiver_static = "ar",
      .cflags = { "",
                  "-g",
                  "-O3 -DNDEBUG",
                  "-O2 -g -DNDEBUG",
                  "-Os -DNDEBUG",
                  "-c",
                  "-I" },
      .lflags = { "", "", "", "", "", "-L", "-l", },
      .flags = { "-o", "-shared", "rcs" },
      .extension = { "", ".o", ".so", ".a" }
  },
  [CBUILDER_TYPE_clang] = {
      .compiler = "clang",
      .linker = "clang",
      .archiver_shared = "clang",
      .archiver_static = "llvm-ar",
      .cflags = { "",
                  "-g",
                  "-O3 -DNDEBUG",
                  "-O2 -g -DNDEBUG",
                  "-Os -DNDEBUG",
                  "-c",
                  "-I" },
      .lflags = { "", "", "", "", "", "-L", "-l", },
      .flags = { "-o", "-shared", "rcs" },
      .extension = { "", ".o", ".so", ".a" }
  },
  [CBUILDER_TYPE_msvc] = {
      .compiler = "cl.exe",
      .linker = "link.exe",
      .archiver_shared = "link.exe",
      .archiver_static = "lib.exe",
      .cflags = { "",
                  "/Zi /utf-8",
                  "/O2 /DNDEBUG",
                  "/O2 /Zi /DNDEBUG",
                  "/Os /DNDEBUG",
                  "/c",
                  "/I" },
      .lflags = { "", "/PDB", "", "", "", "/LIBPATH:", "", },
      .flags = { "/out:", "/DLL /DEBUG", "" },
      .extension = { ".exe", ".obj", ".dll", ".lib" }
  },
};

#ifdef _WIN32
static char const builder_windows_compile_flag_obj_output_path[] = "/FoC:";
static char const builder_windows_compile_flag_pdb_output_path[] = "/FdC:";
#endif

#endif // CBUILDER_PRIVATE_H
