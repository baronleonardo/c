#include "cbuild.h"
#include "cbuild_private.h"
#include "cerror.h"
#include "cprocess.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <stb_ds.h>
#include <fs.h>

#if _WIN32 && (!_MSC_VER || !(_MSC_VER >= 1900))
#error "You need MSVC must be higher that or equal to 1900"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // disable warning about unsafe functions
#endif

static char default_cflags_none[] = "";
static char default_lflags_none[] = "";
#ifdef _WIN32
static char default_cflags_debug[] = "/Zi";
static char default_lflags_debug[] = "";
static char default_cflags_release[] = "/O2 /DNDEBUG";
static char default_lflags_release[] = "";
static char default_cflags_release_with_debug_info[] = "/O2 /Zi /DNDEBUG";
static char default_lflags_release_with_debug_info[] = "";
static char default_cflags_release_with_minimum_size[] = "/Os /DNDEBUG";
static char default_lflags_release_with_minimum_size[] = "";
static char default_compiler[] = "cl";
static char default_linker[] = "link";
static char compiler_compile_argument[] = "/c";
static char compiler_out_object_argument[] = "/Fo";
static char object_extension[] = "obj";
static char exe_extension[] = "exe";
static char linker_out_object_argument[] = "/out:";
#else
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
static char compiler_compile_argument[] = "-c";
static char object_extension[] = "o";
static char compiler_out_object_argument[] = "-o";
#endif

#define BUILD_PATH ".c_build"
#define INSTALL_PATH "c_out"
#define NAME_MAX_LEN 50
#define WHITESPACES " \t\n\v\f\r"

static CError
cbuild_target_build(CBuild *self, CBuildTargetImpl *target, char path_buf[], size_t path_buf_len, size_t path_buf_capacity);
static CError cbuild_target_link(CBuild *self, CBuildTargetImpl *target, char path_buf[], size_t path_buf_len, size_t path_buf_capacity);

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

CError cbuild_create(CBuildType btype, char const base_path[], size_t base_path_len, CBuild *out_cbuild)
{
    assert(base_path && base_path_len > 0);

    if (out_cbuild)
    {
        *out_cbuild = (CBuild){.btype = btype};
        out_cbuild->compiler = default_compiler;
        out_cbuild->linker = default_linker;
        // base path to absolute
        char *status = stbds_arrsetcap(out_cbuild->base_path, c_fs_path_get_max_len());
        if (!status)
        {
            return CERROR_memory_allocation;
        }
        size_t base_path_new_len = 0;
        c_fs_error_t fs_err = c_fs_path_to_absolute(base_path,
                                                    base_path_len,
                                                    out_cbuild->base_path,
                                                    stbds_arrcap(out_cbuild->base_path),
                                                    &base_path_new_len);
        assert(fs_err.code == 0);
        stbds_arrsetlen(out_cbuild->base_path, base_path_new_len);

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
        out_target->impl = calloc(1, sizeof(CBuildTargetImpl));
        if (!out_target->impl)
        {
            return CERROR_memory_allocation;
        }

        // target name
        char *status = arraddnptr(out_target->impl->name, name_len + 1);
        if (!status)
        {
            return CERROR_memory_allocation;
        }
        strncpy(out_target->impl->name, name, name_len);
        out_target->impl->name[name_len] = '\0';

        out_target->impl->cflags = self->cflags;
        out_target->impl->lflags = self->lflags;
        out_target->impl->ttype = CBUILD_TARGET_TYPE_static;

        stbds_arrpush(self->targets, out_target->impl);

        return CERROR_none;
    }

    return CERROR_out_is_null;
}

CError cbuild_shared_lib_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target)
{
    CError err = cbuild_static_lib_create(self, name, name_len, out_target);
    if (out_target)
    {
        out_target->impl->ttype = CBUILD_TARGET_TYPE_shared;
    }

    return err;
}

CError cbuild_exe_create(CBuild *self, char const *name, size_t name_len, CBuildTarget *out_target)
{
    CError err = cbuild_static_lib_create(self, name, name_len, out_target);
    if (out_target)
    {
        out_target->impl->ttype = CBUILD_TARGET_TYPE_executable;
    }

    return err;
}

CError cbuild_target_add_source(CBuild *self, CBuildTarget *target, const char source_path[], size_t source_path_len)
{
    assert(self && self->compiler && self->linker);
    assert(target && target->impl && target->impl->name);
    assert(source_path && source_path_len > 0);

    CTargetSource target_source = {0};
    target_source.path = malloc(c_fs_path_get_max_len());
    if (!target_source.path)
    {
        return CERROR_memory_allocation;
    }
    strncpy(target_source.path, source_path, source_path_len);
    target_source.path[source_path_len] = '\0';
    target_source.path_len = source_path_len;

    bool is_abs;
    bool exists;
    c_fs_path_is_absolute(source_path, source_path_len, &is_abs);
    if (is_abs)
    {
        c_fs_exists(source_path, source_path_len, &exists);
        if (!exists)
        {
            return CERROR_no_such_source;
        }
    }
    else
    {
        size_t base_path_len = stbds_arrlenu(self->base_path);
        size_t new_path_len = 0;
        c_fs_error_t fs_err = c_fs_path_append(self->base_path,
                                               base_path_len,
                                               stbds_arrcap(self->base_path),
                                               source_path,
                                               source_path_len,
                                               &new_path_len);
        assert(fs_err.code == 0);

        c_fs_exists(self->base_path, new_path_len, &exists);
        if (!exists)
        {
            return CERROR_no_such_source;
        }
        self->base_path[base_path_len] = '\0';
    }

    stbds_arrput(target->impl->sources, target_source);

    return CERROR_none;
}

void cbuild_target_destroy(CBuild *self, CBuildTarget *target)
{
    assert(self && self->compiler && self->linker);
    (void)target;
}

CError cbuild_build(CBuild *self)
{
    assert(self);

    CError err = CERROR_none;

    // save current path, change to the base path
    char *cur_dir_path = NULL;
    char *status = stbds_arrsetcap(cur_dir_path, c_fs_path_get_max_len());
    if (!status)
    {
        return CERROR_memory_allocation;
    }
    size_t cur_dir_path_len;
    c_fs_error_t fs_err = c_fs_dir_get_current(cur_dir_path, stbds_arrcap(cur_dir_path), &cur_dir_path_len);
    stbds_arrsetlen(cur_dir_path, cur_dir_path_len);
    fs_err = c_fs_dir_change_current(self->base_path, stbds_arrlenu(self->base_path));
    assert(fs_err.code == 0);

    char *path_buf = NULL;
    status = stbds_arrsetcap(path_buf, c_fs_path_get_max_len());
    if (!status)
    {
        err = CERROR_memory_allocation;
        goto Error;
    }
    size_t path_buf_len = sizeof(BUILD_PATH) - 1;

    /// build path
    stbds_arrsetlen(path_buf, path_buf_len);
    strncpy(path_buf, BUILD_PATH, path_buf_len);
    path_buf[path_buf_len] = '\0';

    bool exists = false;
    c_fs_dir_exists(path_buf, path_buf_len, &exists);
    if (!exists)
    {
        fs_err = c_fs_dir_create(path_buf, path_buf_len);
        assert(fs_err.code == 0);
    }

    /// install path
    path_buf_len = sizeof(INSTALL_PATH) - 1;
    strncpy(path_buf, INSTALL_PATH, path_buf_len);
    path_buf[path_buf_len] = '\0';

    c_fs_dir_exists(path_buf, path_buf_len, &exists);
    if (!exists)
    {
        fs_err = c_fs_dir_create(path_buf, path_buf_len);
        assert(fs_err.code == 0);
    }

    err = cbuild_target_build(self, *self->targets, path_buf, 0, c_fs_path_get_max_len());

    // restore old path
    fs_err = c_fs_dir_change_current(cur_dir_path, stbds_arrlenu(cur_dir_path));
    assert(fs_err.code == 0);

Error:
    stbds_arrfree(path_buf);
    stbds_arrfree(cur_dir_path);
    return err;
}

void cbuild_destroy(CBuild *self)
{
    (void)self;
}

CError cbuild_target_build(CBuild *self, CBuildTargetImpl *target, char path_buf[], size_t path_buf_len, size_t path_buf_capacity)
{
    if (!target)
        return CERROR_none;

    for (size_t iii = 0; iii < (size_t)stbds_arrlen(target->dependencies); iii++)
    {
        cbuild_target_build(self, target->dependencies[iii], path_buf, path_buf_len, path_buf_capacity);
    }

    CError err = CERROR_none;
    size_t base_path_buf_len = path_buf_len;

    char **cmd = NULL;
    stbds_arrput(cmd, self->compiler);

    char *subtoken = strtok(target->cflags, WHITESPACES);
    if (subtoken)
    {
        do
        {
            stbds_arrput(cmd, strdup(subtoken));
        } while ((subtoken = strtok(NULL, WHITESPACES)));
    }

    stbds_arrput(cmd, compiler_compile_argument);

#ifndef WIN32
    stbds_arrput(cmd, compiler_out_object_argument);
#endif

    // target output path
    path_buf_len = sizeof(BUILD_PATH) - 1;
    strncpy(path_buf, BUILD_PATH, path_buf_len);
    path_buf[path_buf_len] = '\0';

    // BUILD_PATH/<target_name>
    c_fs_error_t fs_err = c_fs_path_append(path_buf,
                                           path_buf_len,
                                           path_buf_capacity,
                                           target->name,
                                           strlen(target->name),
                                           &path_buf_len);
    assert(fs_err.code == 0);

    bool obj_out_path_exists = false;
    c_fs_dir_exists(path_buf, path_buf_len, &obj_out_path_exists);
    if (!obj_out_path_exists)
    {
        fs_err = c_fs_dir_create(path_buf, path_buf_len);
        assert(fs_err.code == 0);
    }

    // BUILD_PATH/<target_name>/<target_name>
    size_t cur_path_len;
    fs_err = c_fs_path_append(path_buf,
                              path_buf_len,
                              path_buf_capacity,
                              target->name,
                              strlen(target->name),
                              &cur_path_len);
    assert(fs_err.code == 0);

#ifdef WIN32
    char *tmp = malloc(c_fs_path_get_max_len());
    if (!tmp)
    {
        return CERROR_memory_allocation;
    }

    // /Fo<output obj file>
    snprintf(tmp, c_fs_path_get_max_len(), "%s%s.%s", compiler_out_object_argument, path_buf, object_extension);
    stbds_arrput(cmd, strdup(tmp));

    if (self->btype == CBUILD_TYPE_debug || self->btype == CBUILD_TYPE_release_with_debug_info)
    {
        // /Fd<output pdb file>
        snprintf(tmp, c_fs_path_get_max_len(), "/Fd%s.pdb", path_buf);
        stbds_arrput(cmd, strdup(tmp));
    }
#else
    (void)cur_path_len;
    stbds_arrput(cmd, path_buf);
#endif

    for (size_t iii = 0; iii < stbds_arrlenu(target->sources); ++iii)
    {
        stbds_arrput(cmd, target->sources[iii].path);
    }

    stbds_arrput(cmd, NULL);

    cprocess_exec((char const *const *)cmd, stbds_arrlen(cmd));

#ifdef WIN32
    free(tmp);
#endif

    // restore back the object path
    path_buf[base_path_buf_len] = '\0';
    path_buf_len = base_path_buf_len;
    if (target->ttype == CBUILD_TARGET_TYPE_executable)
    {
        err = cbuild_target_link(self, target, path_buf, path_buf_len, path_buf_capacity);
    }

    path_buf[base_path_buf_len] = '\0';
    path_buf_len = base_path_buf_len;

    stbds_arrfree(cmd);

    return err;
}

CError cbuild_target_link(CBuild *self, CBuildTargetImpl *target, char path_buf[], size_t path_buf_len, size_t path_buf_capacity)
{
    char **cmd = NULL;
    stbds_arrput(cmd, strdup(self->linker));

    char *subtoken = strtok(target->lflags, WHITESPACES);
    if (subtoken)
    {
        do
        {
            stbds_arrput(cmd, strdup(subtoken));
        } while ((subtoken = strtok(NULL, WHITESPACES)));
    }

    // target output path
    path_buf_len = sizeof(BUILD_PATH) - 1;
    strncpy(path_buf, BUILD_PATH, path_buf_len);
    path_buf[path_buf_len] = '\0';

    // BUILD_PATH/<target_name>
    c_fs_error_t fs_err = c_fs_path_append(path_buf,
                                           path_buf_len,
                                           path_buf_capacity,
                                           target->name,
                                           strlen(target->name),
                                           &path_buf_len);
    assert(fs_err.code == 0);

    c_fs_error_t link_handler(char path[], size_t path_len, void *extra_data);
    c_fs_foreach(path_buf, path_buf_len, path_buf_capacity, &link_handler, (void *)cmd);

#ifndef WIN32
    stbds_arrput(cmd, strdup(compiler_out_object_argument));
#endif

    // INSTALL_PATH/
    path_buf_len = sizeof(INSTALL_PATH) - 1;
    strncpy(path_buf, INSTALL_PATH, path_buf_len);
    path_buf[path_buf_len] = '\0';

    // INSTALL_PATH/<target_name>
    fs_err = c_fs_path_append(path_buf,
                              path_buf_len,
                              path_buf_capacity,
                              target->name,
                              strlen(target->name),
                              &path_buf_len);
    assert(fs_err.code == 0);

    bool obj_out_path_exists = false;
    c_fs_dir_exists(path_buf, path_buf_len, &obj_out_path_exists);
    if (!obj_out_path_exists)
    {
        fs_err = c_fs_dir_create(path_buf, path_buf_len);
        assert(fs_err.code == 0);
    }

    // INSTALL_PATH/<target_name>/<target_name>
    fs_err = c_fs_path_append(path_buf,
                              path_buf_len,
                              path_buf_capacity,
                              target->name,
                              strlen(target->name),
                              &path_buf_len);
    assert(fs_err.code == 0);

#ifdef WIN32
    char *tmp = malloc(path_buf_len + sizeof(linker_out_object_argument));
    if (!tmp)
    {
        return CERROR_memory_allocation;
    }

    snprintf(tmp, c_fs_path_get_max_len(), "%s%s.%s", linker_out_object_argument, path_buf, exe_extension);

    stbds_arrput(cmd, tmp);
#else
    stbds_arrput(cmd, strdup(path_buf));
#endif

    stbds_arrput(cmd, NULL);

    cprocess_exec((char const *const *)cmd, stbds_arrlen(cmd));

    // free
    for (size_t iii = 0; iii < stbds_arrlenu(cmd); ++iii)
    {
        free(cmd[iii]);
    }

    return CERROR_none;
}

c_fs_error_t link_handler(char path[], size_t path_len, void *extra_data)
{
    (void)path_len;

    char **cmd = extra_data;
    stbds_arrput(cmd, strdup(path));

    return (c_fs_error_t){0};
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
