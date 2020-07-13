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
static te_errno module_set(unsigned int gid, const char *oid, char *value,
                           char *mod_name);
static te_errno module_get(unsigned int gid, const char *oid, char *value,
                           char *mod_name);

static te_errno module_filename_set(unsigned int gid, const char *oid,
                                    const char *value,
                                    const char *mod_name,...);
static te_errno module_filename_get(unsigned int gid, const char *oid,
                                    char *value, const char *mod_name,...);

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

RCF_PCH_CFG_NODE_RW(node_filename_load_dependencies,
                    "load_dependencies", NULL, NULL,
                    module_filename_load_dependencies_get,
                    module_filename_load_dependencies_set);

RCF_PCH_CFG_NODE_RW(node_filename, "filename",
                    &node_filename_load_dependencies, NULL,
                    module_filename_get, module_filename_set);

RCF_PCH_CFG_NODE_RW(node_module_unload_holders, "unload_holders",
                    NULL, &node_filename,
                    module_unload_holders_get, module_unload_holders_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_module_param, "parameter",
                               NULL, &node_module_unload_holders,
                               module_param_get, module_param_set,
                               module_param_add, module_param_del,
                               module_param_list, NULL);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_module, "module",
                               &node_module_param, NULL,
                               module_get, module_set,
                               module_add, module_del,
                               module_list, NULL);

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

static te_errno
mod_filename_modprobe_try_load_dependencies(te_kernel_module *module)
{
    te_string cmd = TE_STRING_INIT;
    te_errno  rc;

    if (!module->filename_load_dependencies)
        return 0;

    rc = te_string_append(&cmd,
                          "modinfo --field=depends %s | "
                          "xargs -d ',' -n1 | sed '$d' | "
                          "xargs -n1 modprobe",
                          module->filename);
    if (rc != 0)
        return rc;

    rc = ta_system(cmd.ptr);
    te_string_free(&cmd);
    return rc;
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

static int
mod_rmmod(const char *mod_name)
{
    /*
     * Hack: do not unload igb_uio until shareable resources are supported:
     * Bug 11092.
     */
    if (strcmp(mod_name, "igb_uio") == 0)
    {
        WARN("Module igb_uio is not unloaded");
        return 0;
    }

    TE_SPRINTF(buf, "rmmod %s", mod_name);

    return ta_system(buf);
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

static int
mod_modprobe(te_kernel_module *module)
{
    te_kernel_module_param *param;
    const char *cmd = module->filename != NULL ? "insmod" : "modprobe";
    te_string modprobe_cmd = TE_STRING_BUF_INIT(buf);

    /*
     * Do not load module without explicit filename since
     * modprobe is unreliable on systems with pre-loaded modules.
     */
    if (module->filename == NULL && mod_loaded(module->name))
        return 0;

    te_string_reset(&modprobe_cmd);
    te_string_append(&modprobe_cmd, "%s %s",
                     cmd, module->filename != NULL ?
                     module->filename : module->name);
    LIST_FOREACH(param, &module->params, list)
    {
        te_string_append(&modprobe_cmd,
                         " %s=%s", param->name, param->value);
    }

    return ta_system(buf);
}

static void
mod_consistentcy_check(te_kernel_module *module, te_bool loaded)
{
    if (module != NULL && (loaded ^ module->loaded))
        WARN("Inconsistent state of '%s' module : system=%s cache=%s",
             module->name, loaded ? "loaded" : "not loaded",
             module->loaded ? "loaded" : "not loaded");
}


/**
 * Callback used to check whether a given module is grabbed by TA
 * when creating a list of modules with help of get_dir_list().
 *
 * @param module_name       Module name.
 * @param data              Unused.
 *
 * @return @c TRUE if the module is grabbed, @c FALSE otherwise.
 */
static te_bool
module_list_include_callback(const char *module_name, void *data)
{
    UNUSED(data);

    return rcf_pch_rsrc_accessible_may_share("/agent:%s/module:%s",
                                             ta_name, module_name);
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
        te_errno  rc;
        te_kernel_module *module;

        te_string_reset(&buf_te_str);

        rc = get_dir_list(SYS_MODULE, buf, sizeof(buf), FALSE,
                          &module_list_include_callback, NULL);
        if (rc != 0)
            return rc;

        te_string_set_buf(&buf_te_str, buf, sizeof(buf), strlen(buf));
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
        te_errno  rc;
        char      path[PATH_MAX];
        te_kernel_module *module;

        if (mod_loaded(module_name))
        {
            char name[TE_MODULE_NAME_LEN];

            rc = mod_name_underscorify(module_name, name, sizeof(name));
            if (rc != 0)
                return rc;

            rc = te_snprintf(path, PATH_MAX, SYS_MODULE "/%s/parameters/",
                             name);
            if (rc != 0)
            {
                ERROR("%s(): te_snprintf() failed: %r", __FUNCTION__, rc);
                return TE_RC(TE_TA_UNIX, rc);
            }
            rc = get_dir_list(path, buf, sizeof(buf), TRUE,
                              NULL, NULL);
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
    int rc;

    rc = mod_name_underscorify(module_name, name, sizeof(name));
    if (rc != 0)
        return rc;

    rc = write_sys_value(value,
                         SYS_MODULE "/%s/parameters/%s",
                         module_name, param_name);
    if (rc == 0)
    {
        te_kernel_module *module = mod_find(module_name);

        if (module != NULL)
        {
            te_kernel_module_param *param;

            LIST_FOREACH(param, &module->params, list)
                if (strcmp(param->name, param_name) == 0)
                    TE_SPRINTF(param->value, "%s", value);
        }
    }

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
    te_kernel_module_param *param;

    UNUSED(gid);
    UNUSED(oid);

    if (module == NULL)
    {
        ERROR("You're trying to add param to a module '%s' "
              "that is not under our full control", mod_name);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    if (module->loaded)
    {
        ERROR("We don't support addition of module parameters "
              "when loaded and module '%s' is loaded", mod_name);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    param = calloc(1, sizeof(te_kernel_module_param));
    if (param == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    TE_SPRINTF(param->name, "%s", param_name);
    TE_SPRINTF(param->value, "%s", param_value);
    LIST_INSERT_HEAD(&module->params, param, list);

    return 0;
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

static te_errno
module_add(unsigned int gid, const char *oid,
           const char *mod_value, const char *mod_name)
{
    te_bool mod_enable;
    te_kernel_module *module;

    UNUSED(gid);
    UNUSED(oid);

    if (te_strtol_bool(mod_value, &mod_enable) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (mod_find(mod_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    module = calloc(1, sizeof(te_kernel_module));
    if (module == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    TE_SPRINTF(module->name, "%s", mod_name);
    module->filename = NULL;
    module->filename_load_dependencies = FALSE;
    LIST_INIT(&module->params);

    LIST_INSERT_HEAD(&modules, module, list);

    if (mod_enable)
    {
        if (mod_modprobe(module) != 0)
        {
            ERROR("Failed to load '%s' module", mod_name);
            LIST_REMOVE(module, list);
            free(module);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }
    }

    module->loaded = mod_loaded(mod_name);

    return 0;
}

static te_errno
module_del(unsigned int gid, const char *oid, const char *mod_name)
{
    int rc = 0;
    te_bool loaded = mod_loaded(mod_name);
    te_kernel_module *module;

    UNUSED(gid);
    UNUSED(oid);

    module = mod_find(mod_name);
    mod_consistentcy_check(module, loaded);

    if (loaded && (rc = mod_rmmod(mod_name)) != 0)
            ERROR("Failed to unload module '%s' from the system", mod_name);

    /* We keep module in the list if unload fail. This way we can at least
     * try to unload it once again. But overall system is probably in a bad
     * shape. */
    if (rc == 0 && module != NULL)
    {
        LIST_REMOVE(module, list);
        free(module->filename);
        free(module);
    }

    return rc == 0 ? 0 : TE_RC(TE_TA_UNIX, TE_EFAULT);
}

static te_errno
module_set(unsigned int gid, const char *oid, char *value,
           char *mod_name)
{
    te_kernel_module *module = mod_find(mod_name);
    te_bool loaded = mod_loaded(mod_name);
    te_bool load;
    int rc;

    UNUSED(gid);
    UNUSED(oid);

    if (module == NULL)
    {
        ERROR("Sorry, but you're not allowed to unload pre-loaded modules "
              "and '%s' was not loaded by TE", mod_name);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    mod_consistentcy_check(module, loaded);

    if (te_strtol_bool(value, &load))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (load)
    {
        rc = mod_filename_modprobe_try_load_dependencies(module);
        rc = (rc == 0) ? mod_modprobe(module) : rc;
    }
    else
    {
        if (module->unload_holders)
            mod_try_unload_holders(mod_name);

        rc = mod_rmmod(mod_name);
    }

    if (rc == 0 && module != NULL)
        module->loaded = load;

    return rc == 0 ? 0 : TE_RC(TE_TA_UNIX, TE_EFAULT);
}

static te_errno
module_get(unsigned int gid, const char *oid, char *value,
           char *mod_name)
{
    te_kernel_module *module = mod_find(mod_name);
    te_bool loaded = mod_loaded(mod_name);

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
