#include "cbuild.h"
#include "cerror.h"
#include "cprocess.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <stb_ds.h>
#include <fs.h>

static char default_cflags_none[] = "";
static char default_lflags_none[] = "";
static char default_cflags_debug[] = "-g";
static char default_lflags_debug[] = "";
static char default_cflags_release[] = "-O3 -DNDEBUG";
static char default_lflags_release[] = "";
static char default_cflags_release_with_debug_info[] = "-O2 -g -DNDEBUG";
static char default_lflags_release_with_debug_info[] = "";
static char default_cflags_release_with_minimum_size[] = "-Os -DNDEBUG";
static char default_lflags_release_with_minimum_size[] = "";
static char default_compiler[] = "clang";
static char default_linker[] = "clang";

#define BUILD_PATH ".c_build"
#define INSTALL_PATH "c_out"
#define NAME_MAX_LEN 50

static CError cbuild_target_build(CBuild *self, CBuildTarget *target);
static CError cbuild_target_link(CBuild *self, CBuildTarget *target, char buf[], size_t buf_len, size_t buf_capacity);

#define CBUILD_CREATE_T(T)                                      \
    /* cflags*/                                                 \
    out_cbuild->cflags = calloc(1, sizeof(default_cflags_##T)); \
    if (!out_cbuild->cflags)                                    \
        return CERROR_memory_allocation;                        \
    strcpy(out_cbuild->cflags, default_cflags_##T);             \
    /* lflags*/                                                 \
    out_cbuild->lflags = calloc(1, sizeof(default_lflags_##T)); \
    if (!out_cbuild->lflags)                                    \
        return CERROR_memory_allocation;                        \
    strcpy(out_cbuild->lflags, default_lflags_##T);

CError cbuild_create(CBuildType btype, CBuild *out_cbuild)
{
    if (out_cbuild)
    {
        *out_cbuild = (CBuild){.btype = btype};
        out_cbuild->compiler = default_compiler;
        out_cbuild->linker = default_linker;

        switch (btype)
        {
        case CBUILD_TYPE_debug:
            CBUILD_CREATE_T(debug);
            break;

        case CBUILD_TYPE_release:
            CBUILD_CREATE_T(release);
            break;

        case CBUILD_TYPE_release_with_debug_info:
            CBUILD_CREATE_T(release_with_debug_info);
            break;

        case CBUILD_TYPE_release_with_minimum_size:
            CBUILD_CREATE_T(release_with_minimum_size)
            break;

        default:
            CBUILD_CREATE_T(none);
            break;
        }

        return CERROR_none;
    }

    return CERROR_out_is_null;
}

CError cbuild_static_lib_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target)
{
    assert(self && self->compiler && self->linker);

    if (out_target)
    {
        *out_target = (CBuildTarget){0};

        out_target->name = malloc(name_len);
        if (!out_target->name)
        {
            return CERROR_memory_allocation;
        }
        strcpy(out_target->name, name);

        out_target->cflags = self->cflags;
        out_target->lflags = self->lflags;
        out_target->ttype = CBUILD_TARGET_TYPE_static;

        return CERROR_none;
    }

    return CERROR_out_is_null;
}

CError cbuild_shared_lib_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target)
{
    CError err = cbuild_static_lib_create(self, name, name_len, out_target);
    if (out_target)
    {
        out_target->ttype = CBUILD_TARGET_TYPE_shared;
    }

    return err;
}

CError cbuild_exe_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target)
{
    CError err = cbuild_static_lib_create(self, name, name_len, out_target);
    if (out_target)
    {
        out_target->ttype = CBUILD_TARGET_TYPE_executable;
    }

    return err;
}

CError cbuild_target_push(CBuild *self, CBuildTarget *target)
{
    assert(self && self->compiler && self->linker);
    stbds_arrpush(self->targets, target);

    return CERROR_none;
}

void cbuild_target_destroy(CBuild *self, CBuildTarget *target)
{
    assert(self && self->compiler && self->linker);
}

CError cbuild_build(CBuild *self)
{
    assert(self);

    bool exists = false;
    c_fs_error_t err = c_fs_dir_exists(BUILD_PATH, sizeof(BUILD_PATH) - 1, &exists);
    assert(err.code == 0);
    if (!exists)
    {
        err = c_fs_dir_create(BUILD_PATH, sizeof(BUILD_PATH) - 1);
        assert(err.code == 0);
    }

    err = c_fs_dir_exists(INSTALL_PATH, sizeof(INSTALL_PATH) - 1, &exists);
    assert(err.code == 0);
    if (!exists)
    {
        err = c_fs_dir_create(INSTALL_PATH, sizeof(INSTALL_PATH) - 1);
        assert(err.code == 0);
    }

    cbuild_target_build(self, *self->targets);

    return CERROR_none;
}

void cbuild_destroy(CBuild *self)
{
    (void)self;
}

CError cbuild_target_build(CBuild *self, CBuildTarget *target)
{
    if (!target)
        return CERROR_none;

    for (size_t iii = 0; iii < (size_t)stbds_arrlen(target->dependencies); iii++)
    {
        cbuild_target_build(self, target->dependencies[iii]);
    }

    char **cmd = NULL;
    stbds_arrput(cmd, self->compiler);

    char *subtoken;
    while ((subtoken = strsep(&target->cflags, " \t")))
    {
        stbds_arrput(cmd, subtoken);
    }

    stbds_arrput(cmd, "-c");
    /// FIXME:
    for (size_t iii = 0; iii < 1; ++iii)
    {
        stbds_arrput(cmd, target->sources[iii]);
    }

    stbds_arrput(cmd, "-o");

    // target output path
    size_t obj_out_path_len = sizeof(BUILD_PATH) - 1;
    char *obj_out_path = malloc(c_fs_path_get_max_len());
    if (!obj_out_path)
    {
        return CERROR_memory_allocation;
    }
    strncpy(obj_out_path, BUILD_PATH, obj_out_path_len);

    // BUILD_PATH/<target_name>
    c_fs_error_t fs_err = c_fs_path_append(obj_out_path,
                                           obj_out_path_len,
                                           c_fs_path_get_max_len(),
                                           target->name, strlen(target->name),
                                           &obj_out_path_len);
    assert(fs_err.code == 0);

    bool obj_out_path_exists = false;
    fs_err = c_fs_dir_exists(obj_out_path, obj_out_path_len, &obj_out_path_exists);
    assert(fs_err.code == 0);
    if (!obj_out_path_exists)
    {
        fs_err = c_fs_dir_create(obj_out_path, obj_out_path_len);
        assert(fs_err.code == 0);
    }

    // BUILD_PATH/<target_name>/<target_name>
    fs_err = c_fs_path_append(obj_out_path,
                              obj_out_path_len,
                              c_fs_path_get_max_len(),
                              target->name,
                              strlen(target->name),
                              NULL);
    assert(fs_err.code == 0);

    stbds_arrput(cmd, obj_out_path);

    cprocess_exec((char const *const *)cmd, stbds_arrlen(cmd));

    // restore back the object path
    obj_out_path[obj_out_path_len] = '\0';
    if (target->ttype == CBUILD_TARGET_TYPE_executable)
    {
        cbuild_target_link(self, target, obj_out_path, obj_out_path_len, c_fs_path_get_max_len());
    }

    free(obj_out_path);
    stbds_arrfree(cmd);

    return CERROR_none;
}

CError cbuild_target_link(CBuild *self, CBuildTarget *target, char build_path[], size_t build_path_len, size_t buf_capacity)
{
    char **cmd = NULL;
    stbds_arrput(cmd, strdup(self->linker));

    char *subtoken;
    while ((subtoken = strsep(&target->lflags, " \t")))
    {
        stbds_arrput(cmd, strdup(subtoken));
    }

    c_fs_error_t link_handler(char path[], size_t path_len, void *extra_data);
    c_fs_foreach(build_path, build_path_len, buf_capacity, &link_handler, (void *)cmd);

    stbds_arrput(cmd, strdup("-o"));

    size_t obj_out_path_len = sizeof(INSTALL_PATH) - 1;
    char *obj_out_path = malloc(c_fs_path_get_max_len());
    if (!obj_out_path)
    {
        return CERROR_memory_allocation;
    }
    strncpy(obj_out_path, INSTALL_PATH, obj_out_path_len);

    c_fs_error_t fs_err = c_fs_path_append(obj_out_path, obj_out_path_len, c_fs_path_get_max_len(), target->name, strlen(target->name), &obj_out_path_len);
    assert(fs_err.code == 0);

    bool obj_out_path_exists = false;
    fs_err = c_fs_dir_exists(obj_out_path, obj_out_path_len, &obj_out_path_exists);
    assert(fs_err.code == 0);
    if (!obj_out_path_exists)
    {
        fs_err = c_fs_dir_create(obj_out_path, obj_out_path_len);
        assert(fs_err.code == 0);
    }

    fs_err = c_fs_path_append(obj_out_path, obj_out_path_len, c_fs_path_get_max_len(), target->name, strlen(target->name), &obj_out_path_len);
    assert(fs_err.code == 0);

    stbds_arrput(cmd, strdup(obj_out_path));

    cprocess_exec((char const *const *)cmd, stbds_arrlen(cmd));

    // free
    for (size_t iii = 0; iii < stbds_arrlenu(cmd); ++iii)
    {
        free(cmd[iii]);
    }
    free(obj_out_path);

    return CERROR_none;
}

c_fs_error_t link_handler(char path[], size_t path_len, void *extra_data)
{
    char **cmd = extra_data;
    stbds_arrput(cmd, strndup(path, path_len));

    return (c_fs_error_t){0};
}
