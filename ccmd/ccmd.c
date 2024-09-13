#include "ccmd.h"
#include "cbuild.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void ccmd_on_init(CCmd *self);
static void ccmd_on_build(CCmd *self);
static void ccmd_on_run(CCmd *self);
static void ccmd_on_test(CCmd *self);
static void ccmd_on_doc(CCmd *self);
static void ccmd_on_fmt(CCmd *self);
static void ccmd_on_help(CCmd *self);
static void ccmd_on_version(CCmd *self);

CCmd ccmd_create(int argc, char *argv[])
{
    if (argc <= 1)
    {
        ccmd_on_help(NULL);
        exit(EXIT_FAILURE);
    }

    struct
    {
        char const *const subcmd;
        void (*handler)(CCmd *self);
    } const subcmds[] = {
        {"init", ccmd_on_init},
        {"build", ccmd_on_build},
        {"run", ccmd_on_run},
        {"test", ccmd_on_test},
        {"doc", ccmd_on_doc},
        {"fmt", ccmd_on_fmt},
        {"help", ccmd_on_help},
        {"version", ccmd_on_version},
    };
    const size_t subcmds_len = sizeof(subcmds) / sizeof(subcmds[0]);

    CCmd ccmd = {0};

    for (size_t iii = 0; iii < subcmds_len; ++iii)
    {
        if (strncmp(argv[1], subcmds[iii].subcmd, sizeof(subcmds[iii].subcmd) - 1) == 0)
        {
            subcmds[iii].handler(&ccmd);
            exit(EXIT_SUCCESS);
        }
    }

    ccmd_on_help(NULL);
    exit(EXIT_FAILURE);
}

void ccmd_destroy(CCmd *self)
{
}

void ccmd_on_init(CCmd *self) {}

void ccmd_on_build(CCmd *self)
{
    (void)self;

    /// FIXME:
    // CBuild cbuild = cbuild_create(CBUILD_TYPE_debug);
    // cbuild_build(&cbuild);
    // cbuild_destroy(&cbuild);
}

void ccmd_on_run(CCmd *self) {}

void ccmd_on_test(CCmd *self) {}

void ccmd_on_doc(CCmd *self) {}

void ccmd_on_fmt(CCmd *self) {}

void ccmd_on_help(CCmd *self)
{
    (void)self;

    printf(
        "Usage: c [command] [options]\n\n"
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

void ccmd_on_version(CCmd *self)
{
    (void)self;

    printf("Version %s\n", C_VER);
}
