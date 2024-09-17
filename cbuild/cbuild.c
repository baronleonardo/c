#include "cbuild.h"
#include "cbuild_private.h"
#include "cerror.h"
#include "cprocess.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <stb_ds.h>
#include <fs.h>
#include <str.h>

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
static char object_extension[] = ".obj";
static char exe_extension[] = ".exe";
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
static char object_extension[] = ".o";
static char compiler_out_object_argument[] = "-o";
#endif

#define BUILD_PATH ".c_build"
#define INSTALL_PATH "c_out"
#define NAME_MAX_LEN 50
#define STR(s) (s), sizeof(s) - 1

static CError
cbuild_target_build(CBuild *self, CBuildTargetImpl *target);
static CError cbuild_target_link(CBuild *self, CBuildTargetImpl *target);

#define CBUILD_CREATE_T(T)                                                \
    /* cflags*/                                                           \
    str_err = c_str_create(STR(default_cflags_##T), &out_cbuild->cflags); \
    assert(str_err.code == 0);                                            \
    /* lflags*/                                                           \
    str_err = c_str_create(STR(default_lflags_##T), &out_cbuild->lflags); \
    assert(str_err.code == 0);

CError cbuild_create(CBuildType btype, char const base_path[], size_t base_path_len, CBuild *out_cbuild)
{
    assert(base_path && base_path_len > 0);

    if (out_cbuild)
    {
        *out_cbuild = (CBuild){.btype = btype};
        c_str_error_t str_err = c_str_create(STR(default_compiler), &out_cbuild->compiler);
        assert(str_err.code == 0);
        str_err = c_str_create(STR(default_linker), &out_cbuild->linker);
        assert(str_err.code == 0);

        // base path to absolute
        str_err = c_str_create(base_path, base_path_len, &out_cbuild->base_path);
        assert(str_err.code == 0);
        c_fs_error_t fs_err = c_fs_path_to_absolute(base_path,
                                                    base_path_len,
                                                    out_cbuild->base_path.data,
                                                    out_cbuild->base_path.capacity,
                                                    &out_cbuild->base_path.len);
        assert(fs_err.code == 0);

        switch (btype)
        {
        case CBUILD_TYPE_debug:
            str_err = c_str_create(STR(default_cflags_debug), &out_cbuild->cflags);
            assert(str_err.code == 0); /* lflags*/
            str_err = c_str_create(STR(default_lflags_debug), &out_cbuild->lflags);
            assert(str_err.code == 0);
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
    assert(self && self->compiler.data && self->linker.data);

    if (out_target)
    {
        *out_target = (CBuildTarget){0};
        out_target->impl = calloc(1, sizeof(CBuildTargetImpl));
        if (!out_target->impl)
        {
            return CERROR_memory_allocation;
        }

        // target name
        c_str_error_t str_err = c_str_create(name, name_len, &out_target->impl->name);
        assert(str_err.code == 0);

        /// FIXME: should we duplicate this ?
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
    assert(self && self->compiler.data && self->linker.data);
    assert(target && target->impl && target->impl->name.data);
    assert(source_path && source_path_len > 0);

    CStr target_str;
    c_str_error_t str_err = c_str_clone(&self->base_path, &target_str);
    assert(str_err.code == 0);
    str_err = c_str_set_capacity(&target_str, c_fs_path_get_max_len());
    assert(str_err.code == 0);

    c_fs_error_t fs_err = c_fs_path_append(self->base_path.data,
                                           self->base_path.len,
                                           self->base_path.capacity,
                                           source_path,
                                           source_path_len,
                                           &target_str.len);
    assert(fs_err.code == 0);

    bool exists;
    c_fs_exists(target_str.data, target_str.len, &exists);
    if (!exists)
    {
        return CERROR_no_such_source;
    }

    stbds_arrput(target->impl->sources, target_str);

    return CERROR_none;
}

void cbuild_target_destroy(CBuild *self, CBuildTarget *target)
{
    assert(self && self->compiler.data && self->linker.data);
    (void)target;
}

CError cbuild_build(CBuild *self)
{
    assert(self);

    CError err = CERROR_none;

    // save current path, change to the base path
    CStr cur_dir_path;
    c_str_error_t str_err = c_str_create_empty(c_fs_path_get_max_len(), &cur_dir_path);
    assert(str_err.code == 0);
    c_fs_error_t fs_err = c_fs_dir_get_current(cur_dir_path.data, cur_dir_path.capacity, &cur_dir_path.len);
    assert(fs_err.code == 0);

    fs_err = c_fs_dir_change_current(self->base_path.data, self->base_path.len);
    assert(fs_err.code == 0);

    // build path
    CStr build_path;
    str_err = c_str_create(BUILD_PATH, sizeof(BUILD_PATH) - 1, &build_path);
    assert(str_err.code == 0);

    bool exists = false;
    c_fs_dir_exists(build_path.data, build_path.len, &exists);
    if (!exists)
    {
        fs_err = c_fs_dir_create(build_path.data, build_path.len);
        assert(fs_err.code == 0);
    }

    /// install path
    CStr install_path;
    str_err = c_str_create(BUILD_PATH, sizeof(BUILD_PATH) - 1, &install_path);
    assert(str_err.code == 0);

    c_fs_dir_exists(install_path.data, install_path.len, &exists);
    if (!exists)
    {
        fs_err = c_fs_dir_create(install_path.data, install_path.len);
        assert(fs_err.code == 0);
    }

    err = cbuild_target_build(self, *self->targets);

    // restore old path
    fs_err = c_fs_dir_change_current(cur_dir_path.data, cur_dir_path.len);
    assert(fs_err.code == 0);

    // Error:
    c_str_destroy(&cur_dir_path);
    c_str_destroy(&build_path);
    c_str_destroy(&install_path);

    return err;
}

void cbuild_destroy(CBuild *self)
{
    (void)self;
}

CError cbuild_target_build(CBuild *self, CBuildTargetImpl *target)
{
    if (!target)
        return CERROR_none;

    for (size_t iii = 0; iii < (size_t)stbds_arrlen(target->dependencies); iii++)
    {
        cbuild_target_build(self, target->dependencies[iii]);
    }

    CError err = CERROR_none;

    char **cmd = NULL;
    stbds_arrput(cmd, self->compiler.data);

    char *subtoken = strtok(target->cflags.data, C_STR_WHITESPACES);
    if (subtoken)
    {
        do
        {
            stbds_arrput(cmd, strdup(subtoken));
        } while ((subtoken = strtok(NULL, C_STR_WHITESPACES)));
    }

    stbds_arrput(cmd, compiler_compile_argument);

#ifndef WIN32
    stbds_arrput(cmd, compiler_out_object_argument);
#endif

    // target output path
    CStr path_buf;
    c_str_error_t str_err = c_str_create(STR(BUILD_PATH), &path_buf);
    assert(str_err.code == 0);
    str_err = c_str_set_capacity(&path_buf, c_fs_path_get_max_len());
    assert(str_err.code == 0);

    // BUILD_PATH/<target_name>
    c_fs_error_t fs_err = c_fs_path_append(path_buf.data,
                                           path_buf.len,
                                           path_buf.capacity,
                                           target->name.data,
                                           target->name.len,
                                           &path_buf.len);
    assert(fs_err.code == 0);

    bool obj_out_path_exists = false;
    c_fs_dir_exists(path_buf.data, path_buf.len, &obj_out_path_exists);
    if (!obj_out_path_exists)
    {
        fs_err = c_fs_dir_create(path_buf.data, path_buf.len);
        assert(fs_err.code == 0);
    }

    // change to compile dir
    fs_err = c_fs_dir_change_current(path_buf.data, path_buf.len);
    assert(fs_err.code == 0);

    // BUILD_PATH/<target_name>/<target_name>
    size_t cur_path_len;
    fs_err = c_fs_path_append(path_buf.data,
                              path_buf.len,
                              path_buf.capacity,
                              target->name.data,
                              target->name.len,
                              &cur_path_len);
    assert(fs_err.code == 0);

#ifdef WIN32
#else
    (void)cur_path_len;
    stbds_arrput(cmd, path_buf.data);
#endif

    for (size_t iii = 0; iii < stbds_arrlenu(target->sources); ++iii)
    {
        stbds_arrput(cmd, target->sources[iii].data);
    }

    stbds_arrput(cmd, NULL);

    cprocess_exec((char const *const *)cmd, stbds_arrlen(cmd));

    // restore back the object path
    path_buf.data[path_buf.len] = '\0';

    // restore dir
    fs_err = c_fs_dir_change_current(self->base_path.data, self->base_path.len);
    assert(fs_err.code == 0);

    if (target->ttype == CBUILD_TARGET_TYPE_executable)
    {
        err = cbuild_target_link(self, target);
    }

    stbds_arrfree(cmd);
    c_str_destroy(&path_buf);

    return err;
}

CError cbuild_target_link(CBuild *self, CBuildTargetImpl *target)
{
    char **cmd = NULL;
    stbds_arrput(cmd, strdup(self->linker.data));

    char *subtoken = strtok(target->lflags.data, C_STR_WHITESPACES);
    if (subtoken)
    {
        do
        {
            stbds_arrput(cmd, strdup(subtoken));
        } while ((subtoken = strtok(NULL, C_STR_WHITESPACES)));
    }

    // target output path
    CStr path_buf;
    c_str_error_t str_err = c_str_create(STR(BUILD_PATH), &path_buf);
    assert(str_err.code == 0);
    str_err = c_str_set_capacity(&path_buf, c_fs_path_get_max_len());
    assert(str_err.code == 0);

    // BUILD_PATH/<target_name>
    c_fs_error_t fs_err = c_fs_path_append(path_buf.data,
                                           path_buf.len,
                                           path_buf.capacity,
                                           target->name.data,
                                           target->name.len,
                                           &path_buf.len);
    assert(fs_err.code == 0);

    c_fs_error_t link_handler(char path[], size_t path_len, void *extra_data);
    c_fs_foreach(path_buf.data, path_buf.len, path_buf.capacity, &link_handler, (void *)cmd);

#ifndef WIN32
    stbds_arrput(cmd, strdup(compiler_out_object_argument));
#endif

    // INSTALL_PATH/
    str_err = c_str_replace_at(&path_buf, 0, path_buf.len, STR(INSTALL_PATH), false);

    // INSTALL_PATH/<target_name>
    fs_err = c_fs_path_append(path_buf.data,
                              path_buf.len,
                              path_buf.capacity,
                              target->name.data,
                              target->name.len,
                              &path_buf.len);
    assert(fs_err.code == 0);

    bool obj_out_path_exists = false;
    c_fs_dir_exists(path_buf.data, path_buf.len, &obj_out_path_exists);
    if (!obj_out_path_exists)
    {
        fs_err = c_fs_dir_create(path_buf.data, path_buf.len);
        assert(fs_err.code == 0);
    }

    // INSTALL_PATH/<target_name>/<target_name>
    fs_err = c_fs_path_append(path_buf.data,
                              path_buf.len,
                              path_buf.capacity,
                              target->name.data,
                              target->name.len,
                              &path_buf.len);
    assert(fs_err.code == 0);

#ifdef WIN32
    char *tmp = malloc(path_buf_len + sizeof(linker_out_object_argument));
    if (!tmp)
    {
        return CERROR_memory_allocation;
    }

    snprintf(tmp, c_fs_path_get_max_len(), "%s%s%s", linker_out_object_argument, path_buf, exe_extension);

    stbds_arrput(cmd, tmp);
#else
    stbds_arrput(cmd, strdup(path_buf.data));
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
#ifdef _WIN32
    if (strstr(path, object_extension))
    {
#endif
        stbds_arrput(cmd, strdup(path));
#ifdef _WIN32
    }
#endif

    return (c_fs_error_t){0};
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
