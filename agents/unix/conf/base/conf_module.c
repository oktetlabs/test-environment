/** @file
 * @brief System module configuration support
 *
 * Implementation of configuration nodes for system modules.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
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
#include "conf_common.h"
#include "unix_internal.h"
#include "logger_api.h"
#include "te_str.h"
#include "te_string.h"

/* Auxiliary buffer used to construct strings. */
static char buf[4096];
static te_string buf_te_str = TE_STRING_BUF_INIT(buf);

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
    te_bool filename_load_dependencies; /**< Demands that dependencies be
                                         *   loaded prior loading the module
                                         *   by its filename
                                         */
    te_bool unload_holders; /**< Demands that module holders be
                             *   unloaded prior unloading the module
                             */
    te_bool loaded; /*< Is the module loaded into the system */
    te_bool fallback; /*< Load module shipped with the kernel if file
                          pointed by filename does not exist */
    te_bool fake_unload; /*< Flag to handle module unload when the module
                             is shared as a resource. */

    /*< List of parameters of unloaded module to pass later when load it */
    LIST_HEAD(te_kernel_module_params, te_kernel_module_param) params;
} te_kernel_module;

/* List of modules */
static LIST_HEAD(te_kernel_modules, te_kernel_module) modules;


static te_errno module_list(unsigned int, const char *,
                            const char *, char **);
static te_errno module_del(unsigned int gid, const char *oid,
                           const char *mod_name);
static te_errno module_add(unsigned int gid, const char *oid,
                           const char *mod_value, const char *mod_name);

static te_errno module_loaded_set(unsigned int gid, const char *oid,
                                  char *value, char *mod_name);
static te_errno module_loaded_get(unsigned int gid, const char *oid,
                                  char *value, char *mod_name);
static te_errno module_filename_set(unsigned int gid, const char *oid,
                                    const char *value,
                                    const char *mod_name,...);
static te_errno module_filename_get(unsigned int gid, const char *oid,
                                    char *value, const char *mod_name,...);

static te_errno module_version_get(unsigned int gid, const char *oid,
                                   char *value, const char *module_name);

static te_errno module_unload_holders_get(unsigned int gid, const char *oid,
                                          char *value, const char *mod_name,
                                          ...);
static te_errno module_unload_holders_set(unsigned int gid, const char *oid,
                                          const char *value,
                                          const char *mod_name, ...);

static te_errno module_filename_load_dependencies_get(unsigned int  gid,
                                                      const char   *oid,
                                                      char         *value,
                                                      const char   *mod_name);

static te_errno module_filename_load_dependencies_set(unsigned int  gid,
                                                      const char   *oid,
                                                      const char   *value,
                                                      const char   *mod_name);

static te_errno module_param_list(unsigned int, const char *,
                                  const char *, char **,
                                  const char *);
static te_errno module_param_get(unsigned int, const char *, char *,
                                 const char *, const char *);
static te_errno module_param_set(unsigned int, const char *, const char *,
                                 const char *, const char *);
static te_errno module_param_add(unsigned int gid, const char *oid,
                                 const char *param_value,
                                 const char *mod_name,
                                 const char *param_name,...);
static te_errno module_param_del(unsigned int gid, const char *oid,
                                 const char *param_name,
                                 const char *mod_name,...);
static te_errno module_filename_fallback_get(unsigned int gid, const char *oid,
                                             char *value,
                                             const char *mod_name, ...);
static te_errno module_filename_fallback_set(unsigned int gid, const char *oid,
                                             const char *value,
                                             const char *mod_name, ...);

static te_errno module_driver_list(unsigned int gid, const char *oid,
                                   const char *sub_id, char **list,
                                   const char *module_name);

static te_errno driver_device_list(unsigned int gid, const char *oid,
                                   const char *sub_id, char **list,
                                   const char *module_name,
                                   const char *driver_name);
static te_errno driver_device_get(unsigned int gid, const char *oid,
                                  char *value,
                                  const char *module_name,
                                  const char *driver_name,
                                  const char *device_name);

RCF_PCH_CFG_NODE_RW(node_filename_fallback,
                    "fallback", NULL, NULL,
                    module_filename_fallback_get,
                    module_filename_fallback_set);

RCF_PCH_CFG_NODE_RW(node_filename_load_dependencies,
                    "load_dependencies", NULL, &node_filename_fallback,
                    module_filename_load_dependencies_get,
                    module_filename_load_dependencies_set);

RCF_PCH_CFG_NODE_RW(node_filename, "filename",
                    &node_filename_load_dependencies, NULL,
                    module_filename_get, module_filename_set);

RCF_PCH_CFG_NODE_RO(node_version, "version",
                    NULL, &node_filename,
                    module_version_get);

RCF_PCH_CFG_NODE_RW(node_module_unload_holders, "unload_holders",
                    NULL, &node_version,
                    module_unload_holders_get, module_unload_holders_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_module_param, "parameter",
                               NULL, &node_module_unload_holders,
                               module_param_get, module_param_set,
                               module_param_add, module_param_del,
                               module_param_list, NULL);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_driver_device, "device", NULL,
                               NULL, &driver_device_get,
                               &driver_device_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_module_driver, "driver",
                               &node_driver_device,
                               &node_module_param, NULL,
                               &module_driver_list);

RCF_PCH_CFG_NODE_RW(node_module_loaded, "loaded",
                    NULL, &node_module_driver,
                    module_loaded_get, module_loaded_set);

RCF_PCH_CFG_NODE_COLLECTION(node_module, "module",
                            &node_module_loaded, NULL,
                            module_add, module_del,
                            module_list, NULL);

static te_bool
module_is_exclusive_locked(const char *name)
{
    return rcf_pch_rsrc_accessible("/agent:%s/module:%s", ta_name, name);
}

static te_bool
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

static te_bool
mod_loaded(const char *mod_name)
{
    struct stat st;
    char name[TE_MODULE_NAME_LEN];

    mod_name_underscorify(mod_name, name, sizeof(name));

    TE_SPRINTF(buf, SYS_MODULE "/%s", name);

    return stat(buf, &st) == 0;
}

static te_bool
mod_filename_exist(te_kernel_module *module)
{
    struct stat st;

    if (module->filename == NULL)
        return FALSE;

    return stat(module->filename, &st) == 0;
}

static te_errno
mod_get_module_res_name(const char *modname, const char *filename,
                        te_string *res_name)
{
    struct stat st;
    te_errno rc;

    if (filename != NULL)
        return te_string_append(res_name, "%s", filename);

    rc = te_string_append(res_name, "%s/%s.ko", ta_dir, modname);
    if (rc != 0)
        return rc;

    if (stat(res_name->ptr, &st) == 0)
        return 0;

    te_string_free(res_name);
    return te_string_append(res_name, "%s", modname);
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

    rc = te_string_append(cmd,
                          "modinfo --field=depends %s | "
                          "xargs -d ',' -n1 | sed '$d'",
                          res_name.ptr);

    te_string_free(&res_name);
    return rc;
}

static te_errno
mod_load_with_dependencies(const char *modname, const char *filename,
                           te_bool load_itself)
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
        rc = mod_load_with_dependencies(dep_name, NULL, TRUE);
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
        rc = te_string_append(&cmd,
                "path=%s/%s.ko ; test -f $path && insmod $path || modprobe %s",
                ta_dir, modname, modname);
        if (rc != 0)
            goto close;

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

    return mod_load_with_dependencies(module->name, module->filename, FALSE);
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
 * @return @c TRUE if file name looks like PCI address,
 *         @c FALSE otherwise.
 */
static te_bool
filter_pci_addrs_cb(const char *fn, void *data)
{
    int i;
    te_bool point_found = FALSE;

    UNUSED(data);

    for (i = 0; fn[i] != '\0'; i++)
    {
        if ((point_found || (fn[i] != ':' && fn[i] != '.')) &&
            !isxdigit(fn[i]))
        {
            return FALSE;
        }

        if (fn[i] == '.')
            point_found = TRUE;
    }

    return point_found;
}

/**
 * List file names inside some folder under /sys/<module>/.
 *
 * @param module_name       Kernel module name
 * @param buf               Buffer where to save list of file names
 * @param len               Size of the buffer
 * @param include_cb        Callback to use for filtering file names
 * @param cb_data           Data passed to the callback
 * @param fmt               Format string and arguments for a relative
 *                          path
 *
 * @return Status code.
 */
static te_errno
get_module_subdir_list(const char *module_name, char *buf, size_t len,
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

    rc = te_string_append(&path_str, SYS_MODULE "/%s/", name);
    if (rc != 0)
        goto cleanup;

    va_start(ap, fmt);
    rc = te_string_append_va(&path_str, fmt, ap);
    va_end(ap);
    if (rc != 0)
        goto cleanup;

    rc = get_dir_list(path_str.ptr, buf, len, TRUE,
                      include_cb, cb_data);

cleanup:

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
mod_consistentcy_check(te_kernel_module *module, te_bool loaded)
{
    if (module != NULL && (loaded ^ module->loaded) && !module->fake_unload)
        WARN("Inconsistent state of '%s' module : system=%s cache=%s",
             module->name, loaded ? "loaded" : "not loaded",
             module->loaded ? "loaded" : "not loaded");
}


/**
 * Get list of module names.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param sub_id        ID of the object to be listed (unused).
 * @param list          Where to save the list.
 *
 * @return Status code.
 */
static te_errno
module_list(unsigned int gid, const char *oid,
            const char *sub_id, char **list)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

#ifdef __linux__
    {
        te_kernel_module *module;

        te_string_reset(&buf_te_str);

        LIST_FOREACH(module, &modules, list)
            te_string_append(&buf_te_str, "%s ", module->name);
    }
#else
    ERROR("%s(): getting list of system modules "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get module version.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param value         Where to save the version string.
 * @param module_name   Name of the module.
 *
 * @return Status code.
 */
static te_errno
module_version_get(unsigned int gid, const char *oid, char *value,
                   const char *module_name)
{
    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    char name[TE_MODULE_NAME_LEN];
    te_errno rc;

    if (!mod_loaded(module_name))
    {
        *value = '\0';
        return 0;
    }

    rc = mod_name_underscorify(module_name, name, sizeof(name));
    if (rc != 0)
        return rc;

    return read_sys_value(value, RCF_MAX_VAL, TRUE,
                          SYS_MODULE "/%s/version",
                          name);
#else
    UNUSED(value);
    UNUSED(module_name);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

static te_errno
module_param_create(te_kernel_module *module, const char *name,
                    const char *value)
{
    te_kernel_module_param *param;

    param = calloc(1, sizeof(te_kernel_module_param));
    if (param == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    TE_SPRINTF(param->name, "%s", name);
    TE_SPRINTF(param->value, "%s", value);
    LIST_INSERT_HEAD(&module->params, param, list);

    return 0;
}

static te_errno
verify_loaded_module_param(const te_kernel_module *module,
                           const char *param_name,
                           const char *param_value)
{
    char value[RCF_MAX_VAL];
    te_errno rc;

    rc = read_sys_value(value, RCF_MAX_VAL, TRUE, SYS_MODULE"/%s/parameters/%s",
                        module->name, param_name);
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
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param sub_id        ID of the object to be listed (unused).
 * @param list          Where to save the list.
 * @param module_name   Name of the module.
 *
 * @return Status code.
 */
static te_errno
module_param_list(unsigned int gid, const char *oid,
                  const char *sub_id, char **list,
                  const char *module_name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

#ifdef __linux__
    {
        te_kernel_module *module;
        te_errno rc;

        if (mod_loaded(module_name))
        {
            rc = get_module_subdir_list(module_name, buf, sizeof(buf),
                                        NULL, NULL, "parameters");
            if (rc != 0)
                return rc;
        }
        else
        {
            if ((module = mod_find(module_name)) != NULL)
            {
                te_kernel_module_param *param;

                te_string_reset(&buf_te_str);
                LIST_FOREACH(param, &module->params, list)
                    te_string_append(&buf_te_str, "%s ", param->name);
            }
        }
    }
#else
    UNUSED(module_name);

    ERROR("%s(): getting list of system module parameters "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get value of module parameter.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param value         Where to save the value.
 * @param module_name   Name of the module.
 * @param param_name    Name of the parameter.
 *
 * @return Status code.
 */
static te_errno
module_param_get(unsigned int gid, const char *oid, char *value,
                 const char *module_name, const char *param_name)
{
    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    te_kernel_module *module = mod_find(module_name);

    if (module == NULL || mod_loaded(module_name))
    {
        char name[TE_MODULE_NAME_LEN];
        te_errno rc;

        rc = mod_name_underscorify(module_name, name, sizeof(name));
        if (rc != 0)
            return rc;

        return read_sys_value(value, RCF_MAX_VAL, TRUE,
                              SYS_MODULE "/%s/parameters/%s",
                              name, param_name);
    }
    else
    {
        te_kernel_module_param *param;

        LIST_FOREACH(param, &module->params, list)
        {
            if (strcmp(param->name, param_name) == 0)
            {
                te_errno rc;

                rc = te_snprintf(value, RCF_MAX_VAL, "%s", param->value);
                return TE_RC(TE_TA_UNIX, rc);
            }
        }
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

#else
    UNUSED(value);
    UNUSED(module_name);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set value of module parameter.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance (unused).
 * @param value         Value to set.
 * @param module_name   Name of the module.
 * @param param_name    Name of the parameter.
 *
 * @return Status code.
 */
static te_errno
module_param_set(unsigned int gid, const char *oid, const char *value,
                 const char *module_name, const char *param_name)
{
    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    char name[TE_MODULE_NAME_LEN];
    te_errno rc;
    te_kernel_module *module = mod_find(module_name);
    te_kernel_module_param *param;
    te_bool found = FALSE;

    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (!mod_loaded(module_name))
    {
        ERROR("Cannot change the parameters of the not loaded module");
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    if (module_is_exclusive_locked(module_name))
    {
        te_string path = TE_STRING_INIT_STATIC(PATH_MAX);

        rc = te_string_append(&path, SYS_MODULE"/%s/parameters/%s",
                              name, param_name);

        if (access(path.ptr, W_OK) == 0)
        {
            rc = mod_name_underscorify(module_name, name, sizeof(name));
            if (rc != 0)
                return rc;

            rc = write_sys_value(value, path.ptr);
            if (rc != 0)
                return rc;
        }
    }

    LIST_FOREACH(param, &module->params, list)
    {
        if (strcmp(param->name, param_name) == 0)
        {
            TE_SPRINTF(param->value, "%s", value);
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        rc = module_param_create(module, param_name, value);
        if (rc != 0)
            return TE_RC(TE_TA_UNIX, rc);
    }

     if (!module_is_exclusive_locked(module_name))
        rc = verify_loaded_module_param(module, param_name, value);

    return rc;
#else
    UNUSED(value);
    UNUSED(module_name);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

static te_errno
module_param_add(unsigned int gid, const char *oid,
                 const char *param_value, const char *mod_name,
                 const char *param_name,...)
{
    te_kernel_module *module = mod_find(mod_name);
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

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

    rc = module_param_create(module, param_name, param_value);
    if (rc != 0)
        ERROR("Failed to create module parameter: %r", rc);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
module_param_del(unsigned int gid, const char *oid,
                 const char *mod_name, const char *param_name,...)
{
    te_kernel_module *module = mod_find(mod_name);
    te_kernel_module_param *param;

    UNUSED(gid);
    UNUSED(oid);

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
module_filename_fallback_get(unsigned int gid, const char *oid,
                             char *value, const char *mod_name, ...)
{
    te_kernel_module *module = mod_find(mod_name);

    UNUSED(gid);
    UNUSED(oid);

    value[0] = (module != NULL && module->fallback) ? '1' : '0';
    value[1] = '\0';

    return 0;
}

static te_errno
module_filename_fallback_set(unsigned int gid, const char *oid,
                             const char *value, const char *mod_name, ...)
{
    te_kernel_module *module = mod_find(mod_name);

    UNUSED(gid);
    UNUSED(oid);

    if (module == NULL)
        return 0;

    if (value[0] != '0' && value[0] != '1')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    module->fallback = (value[0] == '1');

    return 0;
}

static te_errno
module_filename_load_dependencies_get(unsigned int  gid,
                                      const char   *oid,
                                      char         *value,
                                      const char   *mod_name)
{
    te_kernel_module *module;

    UNUSED(gid);
    UNUSED(oid);

    module = mod_find(mod_name);
    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    value[0] = (module->filename_load_dependencies) ? '1' : '0';
    value[1] = '\0';

    return 0;
}

static te_errno
module_filename_load_dependencies_set(unsigned int  gid,
                                      const char   *oid,
                                      const char   *value,
                                      const char   *mod_name)
{
    te_kernel_module *module;

    UNUSED(gid);
    UNUSED(oid);

    module = mod_find(mod_name);
    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (module->filename == NULL)
        return TE_RC(TE_TA_UNIX, TE_EBADF);

    if ((value[0] != '0' && value[0] != '1') || value[1] != '\0')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    module->filename_load_dependencies = (value[0] == '1');

    return 0;
}

static te_errno
module_filename_set(unsigned int gid, const char *oid,
                    const char *value, const char *mod_name, ...)
{
    te_kernel_module *module = mod_find(mod_name);
    te_bool loaded;

    UNUSED(gid);
    UNUSED(oid);

    loaded = mod_loaded(mod_name);

    mod_consistentcy_check(module, loaded);
    if (loaded)
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);

    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);

    return string_replace(&module->filename, value);
}
static te_errno
module_filename_get(unsigned int gid, const char *oid,
                    char *value, const char *mod_name, ...)
{
    te_kernel_module *module = mod_find(mod_name);
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if (module != NULL && module->filename != NULL)
        rc = te_snprintf(value, RCF_MAX_VAL, "%s", module->filename);
    else
        rc = te_strlcpy(value, "", RCF_MAX_VAL);

    return rc;
}

static te_errno
module_unload_holders_set(unsigned int gid, const char *oid,
                          const char *value, const char *mod_name, ...)
{
    te_kernel_module *module = mod_find(mod_name);

    UNUSED(gid);
    UNUSED(oid);

    if (module == NULL)
        return 0;

    module->unload_holders = (value[0] == '1');

    return 0;
}

static te_errno
module_unload_holders_get(unsigned int gid, const char *oid,
                          char *value, const char *mod_name, ...)
{
    te_kernel_module *module = mod_find(mod_name);

    UNUSED(gid);
    UNUSED(oid);

    value[0] = (module != NULL && module->unload_holders) ? '1' : '0';
    value[1] = '\0';

    return 0;
}

/**
 * Get list of module driver names.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full identifier of the father instance (unused)
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          Where to save the list
 * @param module_name   Name of the module
 *
 * @return Status code.
 */
static te_errno
module_driver_list(unsigned int gid, const char *oid,
                   const char *sub_id, char **list,
                   const char *module_name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

#ifdef __linux__
    if (mod_loaded(module_name))
    {
        te_errno rc;

        rc = get_module_subdir_list(module_name, buf, sizeof(buf),
                                    NULL, NULL, "drivers");
        if (rc != 0)
            return rc;
    }
    else
    {
        buf[0] = '\0';
    }
#else
    UNUSED(module_name);

    ERROR("%s(): getting list of system module drivers "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get list of device names for a given driver.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full identifier of the father instance (unused)
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          Where to save the list
 * @param module_name   Name of the module
 * @param driver_name   Name of the driver
 *
 * @return Status code.
 */
static te_errno
driver_device_list(unsigned int gid, const char *oid,
                   const char *sub_id, char **list,
                   const char *module_name, const char *driver_name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

#ifdef __linux__
    if (mod_loaded(module_name) && strcmp_start("pci:", driver_name) == 0)
    {
        te_errno rc;

        rc = get_module_subdir_list(module_name, buf, sizeof(buf),
                                    filter_pci_addrs_cb, NULL,
                                    "drivers/%s/", driver_name);
        if (rc != 0)
            return rc;
    }
    else
    {
        buf[0] = '\0';
    }
#else
    UNUSED(module_name);
    UNUSED(driver_name);

    ERROR("%s(): getting list of devices is supported only for Linux",
          __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get device node value (bus address).
 *
 * @param gid           Group identifier (unused)
 * @param oid           Full identifier of the father instance (unused)
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          Where to save the list
 * @param module_name   Name of the module
 * @param driver_name   Name of the driver
 * @param device_name   Name of the device
 *
 * @return Status code.
 */
static te_errno
driver_device_get(unsigned int gid, const char *oid,
                  char *value, const char *module_name,
                  const char *driver_name,
                  const char *device_name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(module_name);

    if (strcmp_start("pci:", driver_name) != 0)
    {
        /* Only PCI devices are supported here currently. */
        *value = '\0';
        return 0;
    }

    return te_snprintf(value, RCF_MAX_VAL,
                       "/agent:%s/hardware:/pci:/device:%s",
                       ta_name, device_name);
}

static te_errno
module_add(unsigned int gid, const char *oid,
           const char *unused, const char *mod_name)
{
    te_kernel_module *module;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused);

    if (mod_find(mod_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (!module_is_locked(mod_name))
    {
        ERROR("Failed to add not grabbed module");
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    module = calloc(1, sizeof(te_kernel_module));
    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    TE_SPRINTF(module->name, "%s", mod_name);
    module->filename = NULL;
    module->filename_load_dependencies = FALSE;
    module->fallback = FALSE;
    module->fake_unload = FALSE;
    LIST_INIT(&module->params);

    LIST_INSERT_HEAD(&modules, module, list);

    module->loaded = mod_loaded(mod_name);

    return 0;
}

static te_errno
module_del(unsigned int gid, const char *oid, const char *mod_name)
{
    te_bool loaded = mod_loaded(mod_name);
    te_kernel_module *module;

    UNUSED(gid);
    UNUSED(oid);

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
 * If the module is used by another agent set fake_unload to @c TRUE
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
module_loaded_set(unsigned int gid, const char *oid, char *value,
                  char *mod_name)
{
    te_kernel_module *module = mod_find(mod_name);
    te_bool loaded = mod_loaded(mod_name);
    te_bool load;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    mod_consistentcy_check(module, loaded);

    if (te_strtol_bool(value, &load))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (load)
    {
        rc = mod_load(module);
        module->fake_unload = FALSE;
    }
    else
    {
        rc = mod_unload(module);
    }

    if (rc == 0)
        module->loaded = load;

    return rc;
}

static te_errno
module_loaded_get(unsigned int gid, const char *oid, char *value,
                  char *mod_name)
{
    te_kernel_module *module = mod_find(mod_name);
    te_bool loaded = module->fake_unload ? FALSE : mod_loaded(mod_name);

    UNUSED(gid);
    UNUSED(oid);

    mod_consistentcy_check(module, loaded);

    return te_snprintf(value, RCF_MAX_VAL, "%s", loaded ? "1" : "0");
}

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

    rc = rcf_pch_add_node("/agent/", &node_module);
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
