/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief System module configuration support
 *
 * Implementation of configuration nodes for system modules.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Unix Conf System Module"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#include <limits.h>

#include "rcf_pch.h"
#include "rcf_ch_api.h"
#include "rcf_pch_tree.h"
#include "conf_common.h"
#include "unix_internal.h"
#include "logger_api.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "te_vector.h"

/* Auxiliary buffer used to construct strings. */
static char buf[4096];

#define TE_MODULE_NAME_LEN 32
#define TE_MODULE_PARAM_NAME_LEN 32
#define TE_MODULE_PARAM_VALUE_LEN 128

#define SYS_MODULE "/sys/module"

typedef struct te_kernel_module_param {
    LIST_ENTRY(te_kernel_module_param) list;

    char name[TE_MODULE_PARAM_NAME_LEN];
    char value[TE_MODULE_PARAM_VALUE_LEN];
} te_kernel_module_param;

/* Module that is managed by TE but not in the system */
typedef struct te_kernel_module {
    LIST_ENTRY(te_kernel_module) list;  /**< Linked list of modules */

    char  name[TE_MODULE_NAME_LEN]; /*< Name of the module */
    char *filename; /*< Should be set only for modules that we add before
                     *  enabling */
    bool filename_load_dependencies; /**< Demands that dependencies be
                                         *   loaded prior loading the module
                                         *   by its filename
                                         */
    bool unload_holders; /**< Demands that module holders be
                             *   unloaded prior unloading the module
                             */
    bool loaded; /*< Is the module loaded into the system */
    bool fallback; /*< Load module shipped with the kernel if file
                          pointed by filename does not exist */
    bool fake_unload; /*< Flag to handle module unload when the module
                             is shared as a resource. */

    /*< List of parameters of unloaded module to pass later when load it */
    LIST_HEAD(te_kernel_module_params, te_kernel_module_param) params;
} te_kernel_module;

/* List of modules */
static LIST_HEAD(te_kernel_modules, te_kernel_module) modules;


static bool
module_is_exclusive_locked(const char *name)
{
    return rcf_pch_rsrc_accessible("/agent:%s/module:%s", ta_name, name);
}

static bool
module_is_locked(const char *name)
{
    return rcf_pch_rsrc_accessible_may_share("/agent:%s/module:%s", ta_name,
                                             name);
}

static te_errno
mod_name_underscorify(const char *mod_name, char *buf, size_t size)
{
    te_errno rc;
    char *p;

    rc = te_strlcpy_safe(buf, mod_name, size);
    if (rc != 0)
    {
        ERROR("Could not copy module name '%s' into buffer", mod_name);
        return rc;
    }

    p = buf;
    while ((p = strchr(p, '-')) != NULL)
        *p = '_';

    return 0;
}

static te_kernel_module *
mod_find(const char *mod_name)
{
    char name[TE_MODULE_NAME_LEN];
    te_kernel_module *module;
    te_errno rc;

    rc = mod_name_underscorify(mod_name, name, sizeof(name));
    if (rc != 0)
        return NULL;

    LIST_FOREACH(module, &modules, list)
    {
        char compare_name[TE_MODULE_NAME_LEN];

        rc = mod_name_underscorify(module->name, compare_name,
                                   sizeof(compare_name));
        if (rc != 0)
            return NULL;

        if (strcmp(name, compare_name) == 0)
            break;
    }

    return module;
}

static bool
mod_loaded(const char *mod_name)
{
    struct stat st;
    char name[TE_MODULE_NAME_LEN];

    mod_name_underscorify(mod_name, name, sizeof(name));

    TE_SPRINTF(buf, SYS_MODULE "/%s", name);

    return stat(buf, &st) == 0;
}

static bool
mod_filename_exist(te_kernel_module *module)
{
    struct stat st;

    if (module->filename == NULL)
        return false;

    return stat(module->filename, &st) == 0;
}

static te_errno
mod_get_module_res_name(const char *modname, const char *filename,
                        te_string *res_name)
{
    struct stat st;

    if (filename != NULL)
    {
        te_string_append(res_name, "%s", filename);
        return 0;
    }

    te_string_append(res_name, "%s/%s.ko", ta_lib_mod_dir, modname);

    if (stat(res_name->ptr, &st) == 0)
        return 0;

    te_string_free(res_name);
    te_string_append(res_name, "%s", modname);
    return 0;
}

static te_errno
mod_make_cmd_printing_dependencies(const char *modname,
                                   const char *filename,
                                   te_string *cmd)
{
    te_string res_name = TE_STRING_INIT;
    te_errno rc;

    rc = mod_get_module_res_name(modname, filename, &res_name);
    if (rc != 0)
        return rc;

    te_string_append(cmd, "modinfo --field=depends %s | "
                          "xargs -d ',' -n1 | sed '$d'",
                          res_name.ptr);

    te_string_free(&res_name);
    return 0;
}

static te_errno
mod_load_with_dependencies(const char *modname, const char *filename,
                           bool load_itself)
{
    te_string cmd = TE_STRING_INIT;
    te_errno rc;
    FILE *fp;
    pid_t cmd_pid;
    char dep_name[NAME_MAX + 1];
    char *c;

    if (mod_loaded(modname))
        return 0;

    rc = mod_make_cmd_printing_dependencies(modname, filename, &cmd);
    if (rc != 0)
        goto out;

    rc = ta_popen_r(cmd.ptr, &cmd_pid, &fp);
    if (rc != 0)
        goto out;

    while ((c = fgets(dep_name, sizeof(dep_name), fp)) != NULL)
    {
        while (c != (dep_name + sizeof(dep_name)) && *c++ != '\n');
        if (*(--c) != '\n')
            goto close;
        *c = '\0';
        RING("Loading dependent module %s for %s", dep_name, modname);
        rc = mod_load_with_dependencies(dep_name, NULL, true);
        if (rc != 0)
            goto close;
    }
    if (ferror(fp))
    {
        rc = te_rc_os2te(errno);
        goto close;
    }

    if (load_itself)
    {
        te_string_free(&cmd);
        te_string_append(&cmd,
            "path=%s/%s.ko ; test -f $path && insmod $path || modprobe %s",
            ta_lib_mod_dir, modname, modname);

        rc = ta_system(cmd.ptr) == 0 ? 0 : TE_RC(TE_TA_UNIX, TE_EFAIL);

        RING("Do '%s': %r", cmd.ptr, rc);
    }

close:
    ta_pclose_r(cmd_pid, fp);
out:
    te_string_free(&cmd);
    return rc;
}

static te_errno
mod_filename_modprobe_try_load_dependencies(te_kernel_module *module)
{
    if (!mod_filename_exist(module) && module->fallback)
        return 0;

    if (!module->filename_load_dependencies)
        return 0;

    return mod_load_with_dependencies(module->name, module->filename, false);
}

static te_errno
mod_insert_or_move_holder_uniq_tail(tqh_strings *holders, char *mod_name)
{
    tqe_string *p;

    for (p = TAILQ_FIRST(holders);
         p != NULL && strcmp(mod_name, p->v) != 0;
         p = TAILQ_NEXT(p, links));

    if (p != NULL)
    {
        TAILQ_REMOVE(holders, p, links);
        TAILQ_INSERT_TAIL(holders, p, links);

        return 0;
    }
    else
    {
        return tq_strings_add_uniq_dup(holders, mod_name);
    }
}

/**
 * Callback to use with get_dir_list() function. It filters out
 * all file names except those which look like PCI addresses.
 *
 * @param fn        File name
 * @param data      Not used
 *
 * @return @c true if file name looks like PCI address,
 *         @c false otherwise.
 */
static bool
filter_pci_addrs_cb(const char *fn, void *data)
{
    int i;
    bool point_found = false;

    UNUSED(data);

    for (i = 0; fn[i] != '\0'; i++)
    {
        if ((point_found || (fn[i] != ':' && fn[i] != '.')) &&
            !isxdigit(fn[i]))
        {
            return false;
        }

        if (fn[i] == '.')
            point_found = true;
    }

    return point_found;
}

/**
 * List file names inside some folder under /sys/<module>/.
 *
 * @param module_name       Kernel module name
 * @param names             Vector of heap-allocated names to append to
 * @param include_cb        Callback to use for filtering file names
 * @param cb_data           Data passed to the callback
 * @param fmt               Format string and arguments for a relative
 *                          path
 *
 * @return Status code.
 */
static te_errno
get_module_subdir_list(const char *module_name, te_vec *names,
                       include_callback_func include_cb,
                       void *cb_data, const char *fmt, ...)
{
    te_string path_str = TE_STRING_INIT;
    char name[TE_MODULE_NAME_LEN];
    va_list ap;
    te_errno rc;

    rc = mod_name_underscorify(module_name, name, sizeof(name));
    if (rc != 0)
        return rc;

    te_string_append(&path_str, SYS_MODULE "/%s/", name);

    va_start(ap, fmt);
    te_string_append_va(&path_str, fmt, ap);
    va_end(ap);

    rc = get_dir_list_vec(path_str.ptr, names, true,
                          include_cb, cb_data, NULL);

    te_string_free(&path_str);
    return rc;
}

/**
 * Insert holders of a module @p mod_name into a strings tailq.
 * Move holders to the tail if they are already present in tailq.
 * Insert them into the end (tail) of the queue so that modules
 * to unload first will be at the end of the queue.
 *
 * @note The function may fail yet insert some holders to the tailq
 * when creating a list of modules with help of get_dir_list().
 *
 * @param holders           Module holders queue
 * @param mod_name          Name of the module which holders should be
 *                          inserted
 *
 * @return Status code.
 */
static te_errno
mod_insert_or_move_holders_tail(tqh_strings *holders, const char *mod_name)
{
    struct dirent **names;
    te_errno rc = 0;
    char *dir;
    int n = 0;
    int i;
    char name[TE_MODULE_NAME_LEN];

    rc = mod_name_underscorify(mod_name, name, sizeof(name));
    if (rc != 0)
        return rc;

    dir = te_string_fmt(SYS_MODULE "/%s/holders", name);
    if (dir == NULL)
        return TE_ENOMEM;

    n = scandir(dir, &names, NULL, alphasort);
    free(dir);
    if (n < 0)
    {
        ERROR("Cannot get a list of module holders, rc=%d", rc);
        return TE_EFAIL;
    }
    if (n == 0)
        return 0;

    for (i = 0; i < n; i++)
    {
        if (strcmp(names[i]->d_name, ".") == 0 ||
            strcmp(names[i]->d_name, "..") == 0)
        {
            continue;
        }

        rc = mod_insert_or_move_holder_uniq_tail(holders, names[i]->d_name);
        if (rc != 0)
            goto out;
    }

out:
    for (i = 0; i < n; i++)
        free(names[i]);

    free(names);

    return rc;
}

static te_errno
mod_rmmod(const char *mod_name)
{
    te_errno rc;

    TE_SPRINTF(buf, "rmmod %s", mod_name);

    rc = ta_system(buf) == 0 ? 0 : TE_RC(TE_TA_UNIX, TE_EFAIL);

    RING("Do '%s': %r", buf, rc);

    return rc;
}

static void
mod_try_unload_holders(const char *mod_name)
{
    tqh_strings holders = TAILQ_HEAD_INITIALIZER(holders);
    tqe_string *name;
    te_errno rc;

    if (tq_strings_add_uniq_dup(&holders, mod_name) != 0)
    {
        ERROR("Failed to insert name to holders list");
        return;
    }

    TAILQ_FOREACH(name, &holders, links)
    {
        if (mod_insert_or_move_holders_tail(&holders, name->v) != 0)
        {
            ERROR("Failed to populate holders list");
            goto out;
        }
    }

    name = TAILQ_FIRST(&holders);
    TAILQ_REMOVE(&holders, name, links);
    free(name->v);
    free(name);

    TAILQ_FOREACH_REVERSE(name, &holders, tqh_strings, links)
    {
        rc = mod_rmmod(name->v);
        if (rc != 0)
            WARN("rmmod '%s' failed", name->v);
    }

out:
    tq_strings_free(&holders, free);
}

static const char *
mod_get_module_run_name(te_kernel_module *module)
{
    if (module->filename == NULL)
        return module->name;

    if (mod_filename_exist(module))
       return module->filename;
    else if (module->fallback)
        return module->name;

    return module->filename;
}

static const char *
mod_get_add_cmd_name(te_kernel_module *module)
{
    if (module->filename != NULL && !mod_filename_exist(module) &&
        module->fallback)
    {
            return "modprobe";
    }

    return module->filename != NULL ? "insmod" : "modprobe";
}

static te_errno
mod_modprobe(te_kernel_module *module)
{
    te_kernel_module_param *param;
    const char *cmd = mod_get_add_cmd_name(module);
    te_string modprobe_cmd = TE_STRING_BUF_INIT(buf);
    te_errno rc;

    /*
     * Do not load module without explicit filename since
     * modprobe is unreliable on systems with pre-loaded modules.
     */
    if (module->filename == NULL && mod_loaded(module->name))
        return 0;

    te_string_reset(&modprobe_cmd);
    te_string_append(&modprobe_cmd, "%s %s",
                     cmd, mod_get_module_run_name(module));
    LIST_FOREACH(param, &module->params, list)
    {
        te_string_append(&modprobe_cmd,
                         " %s=%s", param->name, param->value);
    }

    rc = ta_system(buf) == 0 ? 0 : TE_RC(TE_TA_UNIX, TE_EFAIL);

    RING("Do '%s': %r", buf, rc);

    return rc;
}

static void
mod_consistentcy_check(te_kernel_module *module, bool loaded)
{
    if (module != NULL && (loaded ^ module->loaded) && !module->fake_unload)
        WARN("Inconsistent state of '%s' module : system=%s cache=%s",
             module->name, loaded ? "loaded" : "not loaded",
             module->loaded ? "loaded" : "not loaded");
}


/**
 * Get list of module names.
 *
 * @param ctx           request context
 * @param names         vector of heap-allocated names to append to
 *
 * @return Status code.
 */
static te_errno
module_list(const ta_conf_ctx *ctx, te_vec *names)
{
    UNUSED(ctx);

#ifdef __linux__
    te_kernel_module *module;

    LIST_FOREACH(module, &modules, list)
    {
        char *name = TE_STRDUP(module->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
#else
    ERROR("%s(): getting list of system modules "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get module version.
 *
 * @param ctx           request context
 * @param val           location for the version string
 *
 * @return Status code.
 */
static te_errno
module_version_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *module_name = ta_conf_ctx_inst(ctx, "module");

#if __linux__
    char name[TE_MODULE_NAME_LEN];
    char version[RCF_MAX_VAL];
    te_errno rc;

    if (!mod_loaded(module_name))
        return 0;

    rc = mod_name_underscorify(module_name, name, sizeof(name));
    if (rc != 0)
        return rc;

    rc = read_sys_value(version, RCF_MAX_VAL, true,
                        SYS_MODULE "/%s/version",
                        name);
    if (rc != 0)
        return rc;

    te_string_append(val, "%s", version);
    return 0;
#else
    UNUSED(module_name);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

static void
module_param_create(te_kernel_module *module, const char *name,
                    const char *value)
{
    te_kernel_module_param *param;

    param = TE_ALLOC(sizeof(te_kernel_module_param));

    TE_SPRINTF(param->name, "%s", name);
    TE_SPRINTF(param->value, "%s", value);
    LIST_INSERT_HEAD(&module->params, param, list);
}

static te_errno
verify_loaded_module_param(const te_kernel_module *module,
                           const char *param_name,
                           const char *param_value)
{
    char name[TE_MODULE_NAME_LEN];
    char value[RCF_MAX_VAL];
    te_errno rc;

    rc = mod_name_underscorify(module->name, name, sizeof(name));
    if (rc != 0)
        return rc;

    rc = read_sys_value(value, RCF_MAX_VAL, true,
                        SYS_MODULE"/%s/parameters/%s",
                        name, param_name);
    if (rc != 0)
        return rc;

    if (strcmp(param_value, value) != 0)
    {
        ERROR("The value of the parameter '%s' = '%s' of the module '%s' "
              "differs from the value from sysfs = '%s'", param_name,
              param_value, module->name, value);
        rc = TE_EINVAL;
    }

    return rc;
}

static te_errno
verify_loaded_module_params(const te_kernel_module *module)
{
    te_errno rc = 0;
    te_kernel_module_param *param;

    LIST_FOREACH(param, &module->params, list)
    {
        rc = verify_loaded_module_param(module, param->name, param->value);
        if (rc != 0)
            break;
    }

    return rc;
}


/**
 * Get list of module parameter names.
 *
 * @param ctx           request context (parent instance is the module)
 * @param names         vector of heap-allocated names to append to
 *
 * @return Status code.
 */
static te_errno
module_param_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *module_name = ta_conf_ctx_inst(ctx, "module");

#ifdef __linux__
    if (mod_loaded(module_name))
    {
        te_errno rc;

        rc = get_module_subdir_list(module_name, names,
                                    NULL, NULL, "parameters");
        if (rc != 0)
            return rc;
    }
    else
    {
        te_kernel_module *module = mod_find(module_name);
        te_kernel_module_param *param;

        if (module == NULL)
            return 0;

        LIST_FOREACH(param, &module->params, list)
        {
            char *name = TE_STRDUP(param->name);

            TE_VEC_APPEND(names, name);
        }
    }

    return 0;
#else
    UNUSED(module_name);

    ERROR("%s(): getting list of system module parameters "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get value of module parameter.
 *
 * @param ctx           request context
 * @param val           location for the value
 *
 * @return Status code.
 */
static te_errno
module_param_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *module_name = ta_conf_ctx_inst(ctx, "module");
    const char *param_name = ta_conf_ctx_inst(ctx, "parameter");

#if __linux__
    te_kernel_module *module = mod_find(module_name);

    if (module == NULL || mod_loaded(module_name))
    {
        char name[TE_MODULE_NAME_LEN];
        char value[RCF_MAX_VAL];
        te_errno rc;

        rc = mod_name_underscorify(module_name, name, sizeof(name));
        if (rc != 0)
            return rc;

        rc = read_sys_value(value, RCF_MAX_VAL, true,
                            SYS_MODULE "/%s/parameters/%s",
                            name, param_name);
        if (rc != 0)
            return rc;

        te_string_append(val, "%s", value);
        return 0;
    }
    else
    {
        te_kernel_module_param *param;

        LIST_FOREACH(param, &module->params, list)
        {
            if (strcmp(param->name, param_name) == 0)
            {
                te_string_append(val, "%s", param->value);
                return 0;
            }
        }
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

#else
    UNUSED(module_name);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set value of module parameter.
 *
 * @param ctx           request context
 * @param value         value to set
 *
 * @return Status code.
 */
static te_errno
module_param_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *module_name = ta_conf_ctx_inst(ctx, "module");
    const char *param_name = ta_conf_ctx_inst(ctx, "parameter");

#if __linux__
    te_errno rc;
    te_kernel_module *module = mod_find(module_name);
    te_kernel_module_param *param;
    bool loaded = mod_loaded(module_name);
    bool found = false;

    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (loaded)
    {
        if (module_is_exclusive_locked(module_name))
        {
            char name[TE_MODULE_NAME_LEN];
            te_string path = TE_STRING_INIT_STATIC(PATH_MAX);

            rc = mod_name_underscorify(module_name, name, sizeof(name));
            if (rc != 0)
                return rc;

            te_string_append(&path, SYS_MODULE"/%s/parameters/%s",
                             name, param_name);

            if (access(path.ptr, W_OK) == 0)
            {
                rc = write_sys_value(value, path.ptr);
                if (rc != 0)
                    return rc;
            }
        }
        else
        {
            rc = verify_loaded_module_param(module, param_name, value);
            if (rc != 0)
                return rc;
        }
    }

    LIST_FOREACH(param, &module->params, list)
    {
        if (strcmp(param->name, param_name) == 0)
        {
            TE_SPRINTF(param->value, "%s", value);
            found = true;
            break;
        }
    }

    if (!found)
    {
        /* The parameter must be added before if module is not loaded yet */
        if (!loaded)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        module_param_create(module, param_name, value);
    }

    return 0;
#else
    UNUSED(value);
    UNUSED(module_name);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

static te_errno
module_param_add(const ta_conf_ctx *ctx, const char *param_value)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    const char *param_name = ta_conf_ctx_inst(ctx, "parameter");
    te_kernel_module *module = mod_find(mod_name);

    if (!module_is_locked(mod_name))
    {
        ERROR("Cannot add parameters of the not grabbed module");
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    if (module == NULL)
    {
        ERROR("You're trying to add param to a module '%s' "
              "that is not under our full control", mod_name);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    if (module->loaded)
    {
        /*
         * For already loaded module the list of parameters is exported
         * from /sys/module/modname/parameters. Only their values could be
         * changed with set.
         */
        ERROR("We don't support addition of module parameters "
              "when loaded and module '%s' is loaded", mod_name);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    module_param_create(module, param_name, param_value);

    return 0;
}

static te_errno
module_param_del(const ta_conf_ctx *ctx)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    const char *param_name = ta_conf_ctx_inst(ctx, "parameter");
    te_kernel_module *module = mod_find(mod_name);
    te_kernel_module_param *param;

    if (!module)
    {
        ERROR("You're trying to del param of a module '%s' "
              "that is not under our full control", mod_name);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    if (module->loaded)
    {
        /*
         * For already loaded module the list of parameters is exported
         * from /sys/module/modname/parameters. Only their values could be
         * changed with set.
         */
        ERROR("We don't support removal of module parameters "
              "when loaded and module '%s' is loaded", mod_name);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    LIST_FOREACH(param, &module->params, list)
    {
        if (strcmp(param->name, param_name) == 0)
        {
            LIST_REMOVE(param, list);
            free(param);
            return 0;
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

static te_errno
module_filename_fallback_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);

    te_string_append(val, "%s",
                     (module != NULL && module->fallback) ? "1" : "0");

    return 0;
}

static te_errno
module_filename_fallback_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);

    if (module == NULL)
        return 0;

    if (value[0] != '0' && value[0] != '1')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    module->fallback = (value[0] == '1');

    return 0;
}

static te_errno
module_filename_load_dependencies_get(const ta_conf_ctx *ctx, bool *val)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);

    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = module->filename_load_dependencies;

    return 0;
}

static te_errno
module_filename_load_dependencies_set(const ta_conf_ctx *ctx, bool val)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);

    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (module->filename == NULL)
        return TE_RC(TE_TA_UNIX, TE_EBADF);

    module->filename_load_dependencies = val;

    return 0;
}

static te_errno
module_filename_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);
    bool loaded;

    loaded = mod_loaded(mod_name);

    mod_consistentcy_check(module, loaded);
    if (loaded)
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);

    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);

    return string_replace(&module->filename, value);
}

static te_errno
module_filename_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);

    if (module != NULL && module->filename != NULL)
        te_string_append(val, "%s", module->filename);

    return 0;
}

static te_errno
module_unload_holders_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);

    if (module == NULL)
        return 0;

    module->unload_holders = (value[0] == '1');

    return 0;
}

static te_errno
module_unload_holders_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);

    te_string_append(val, "%s",
                     (module != NULL && module->unload_holders) ? "1" : "0");

    return 0;
}

/**
 * Get list of module driver names.
 *
 * @param ctx           request context (parent instance is the module)
 * @param names         vector of heap-allocated names to append to
 *
 * @return Status code.
 */
static te_errno
module_driver_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *module_name = ta_conf_ctx_inst(ctx, "module");

#ifdef __linux__
    if (mod_loaded(module_name))
    {
        te_errno rc;

        rc = get_module_subdir_list(module_name, names,
                                    NULL, NULL, "drivers");
        if (rc != 0)
            return rc;
    }

    return 0;
#else
    UNUSED(module_name);

    ERROR("%s(): getting list of system module drivers "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get list of device names for a given driver.
 *
 * @param ctx           request context (parent instance is the driver)
 * @param names         vector of heap-allocated names to append to
 *
 * @return Status code.
 */
static te_errno
driver_device_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *module_name = ta_conf_ctx_inst(ctx, "module");
    const char *driver_name = ta_conf_ctx_inst(ctx, "driver");

#ifdef __linux__
    if (mod_loaded(module_name) && strcmp_start("pci:", driver_name) == 0)
    {
        te_errno rc;

        rc = get_module_subdir_list(module_name, names,
                                    filter_pci_addrs_cb, NULL,
                                    "drivers/%s/", driver_name);
        if (rc != 0)
            return rc;
    }

    return 0;
#else
    UNUSED(module_name);
    UNUSED(driver_name);

    ERROR("%s(): getting list of devices is supported only for Linux",
          __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get device node value (bus address).
 *
 * @param ctx           request context
 * @param val           location for the value
 *
 * @return Status code.
 */
static te_errno
driver_device_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *driver_name = ta_conf_ctx_inst(ctx, "driver");
    const char *device_name = ta_conf_ctx_inst(ctx, "device");

    if (strcmp_start("pci:", driver_name) != 0)
    {
        /* Only PCI devices are supported here currently. */
        return 0;
    }

    te_string_append(val, "/agent:%s/hardware:/pci:/device:%s",
                     ta_name, device_name);
    return 0;
}

static te_errno
module_add(const ta_conf_ctx *ctx)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module;

    if (mod_find(mod_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (!module_is_locked(mod_name))
    {
        ERROR("Failed to add not grabbed module");
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    module = TE_ALLOC(sizeof(te_kernel_module));
    TE_SPRINTF(module->name, "%s", mod_name);
    module->filename = NULL;
    module->filename_load_dependencies = false;
    module->fallback = false;
    module->fake_unload = false;
    LIST_INIT(&module->params);

    LIST_INSERT_HEAD(&modules, module, list);

    module->loaded = mod_loaded(mod_name);

    return 0;
}

static te_errno
module_del(const ta_conf_ctx *ctx)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    bool loaded = mod_loaded(mod_name);
    te_kernel_module *module;

    module = mod_find(mod_name);
    mod_consistentcy_check(module, loaded);

    if (module != NULL)
    {
        LIST_REMOVE(module, list);
        free(module->filename);
        free(module);
    }

    return 0;
}

/**
 * If the module is used by another agent set fake_unload to @c true
 *
 * @param module Kernel module
 *
 * @return Status code
 */
static void
maybe_fake_unload(te_kernel_module *module)
{
    module->fake_unload = !module_is_exclusive_locked(module->name);
}

static te_errno
mod_unload(te_kernel_module *module)
{
    te_errno rc;

    maybe_fake_unload(module);

    if (module->fake_unload)
        return 0;

    if (module->unload_holders)
        mod_try_unload_holders(module->name);

    rc = mod_rmmod(module->name);
    if (rc != 0)
        ERROR("Failed to unload module '%s'", module->name);

    return rc;
}

static te_errno
mod_load(te_kernel_module *module)
{
    te_errno rc;

    if (mod_loaded(module->name))
    {
        RING("Module '%s' already loaded", module->name);
        return verify_loaded_module_params(module);
    }

    rc = mod_filename_modprobe_try_load_dependencies(module);
    if (rc != 0)
    {
        ERROR("Failed to load module '%s' dependencies", module->name);
        return rc;
    }

    rc = mod_modprobe(module);
    if (rc != 0)
        ERROR("Failed to load module '%s'", module->name);

    return rc;
}

static te_errno
module_loaded_set(const ta_conf_ctx *ctx, int32_t val)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);
    bool loaded = mod_loaded(mod_name);
    /*
     * Match legacy te_strtol_bool() semantics: any nonzero decimal
     * value means "load", not just 1.
     */
    bool load = (val != 0);
    te_errno rc = 0;

    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    mod_consistentcy_check(module, loaded);

    if (load)
    {
        rc = mod_load(module);
        module->fake_unload = false;
    }
    else
    {
        if (loaded)
        {
            rc = mod_unload(module);
        }
        else
        {
            RING("Module %s is not loaded, ignoring unload command", mod_name);
        }
    }

    if (rc == 0)
        module->loaded = load;

    return rc;
}

static te_errno
module_loaded_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);
    bool loaded;

    /*
     * mod_find() can return NULL for a module name too long for
     * mod_name_underscorify() to handle; the legacy handler
     * dereferenced module unconditionally here, unlike every other
     * accessor in this file, and crashed the agent in that case.
     */
    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    loaded = module->fake_unload ? false : mod_loaded(mod_name);

    mod_consistentcy_check(module, loaded);

    *val = loaded ? 1 : 0;
    return 0;
}

static te_errno
module_loaded_oper_get(const ta_conf_ctx *ctx, bool *val)
{
    const char *mod_name = ta_conf_ctx_inst(ctx, "module");
    te_kernel_module *module = mod_find(mod_name);
    bool loaded = mod_loaded(mod_name);

    mod_consistentcy_check(module, loaded);

    *val = loaded;
    return 0;
}

static const ta_conf_node *const node_module =
    TA_CONF_COLL("module", module_add, module_del, module_list,
        TA_CONF_RO_BOOL("loaded_oper", module_loaded_oper_get),
        TA_CONF_RW_INT32("loaded", module_loaded_get, module_loaded_set),
        TA_CONF_LIST("driver", module_driver_list,
            TA_CONF_RO_COLL("device", driver_device_get,
                            driver_device_list)),
        TA_CONF_COLL_STR_RW("parameter", module_param_get,
                            module_param_set, module_param_add,
                            module_param_del, module_param_list),
        TA_CONF_RW_STR("unload_holders", module_unload_holders_get,
                       module_unload_holders_set),
        TA_CONF_RO_STR("version", module_version_get),
        TA_CONF_RW_STR("filename", module_filename_get, module_filename_set,
            TA_CONF_RW_BOOL("load_dependencies",
                            module_filename_load_dependencies_get,
                            module_filename_load_dependencies_set),
            TA_CONF_RW_STR("fallback", module_filename_fallback_get,
                           module_filename_fallback_set)));

/**
 * Initialize configuration for system module nodes.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_module_init(void)
{
#ifdef __linux__
    te_errno rc;

    rc = ta_conf_register("/agent", node_module);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/module",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
#else
    INFO("System module configuration is not supported");
    return 0;
#endif
}
