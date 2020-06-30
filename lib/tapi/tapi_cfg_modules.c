/** @file
 * @brief Test API to configure kernel modules
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Conf Kernel Modules TAPI"

#include "te_config.h"
#include "te_defs.h"
#include "te_string.h"
#include "logger_api.h"
#include "te_log_stack.h"
#include "conf_api.h"

#define CFG_MODULE_OID_FMT "/agent:%s/module:%s"
#define CFG_MODULE_PARAM_OID_FMT CFG_MODULE_OID_FMT"/parameter:%s"

te_errno
tapi_cfg_module_add(const char *ta_name, const char *mod_name, te_bool load)
{
    int         rc;

    ENTRY("ta_name=%s mod_name=%s load=%s", ta_name, mod_name,
          load ? "true" : "false");

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, load ? 1 : 0),
                              CFG_MODULE_OID_FMT, ta_name, mod_name);
    if (rc != 0)
        te_log_stack_push("Addition of module '%s' on TA %s failed",
                          mod_name, ta_name);

    EXIT("%r", rc);

    return rc;
}

te_errno
tapi_cfg_module_load(const char *ta_name, const char *mod_name)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                CFG_MODULE_OID_FMT, ta_name, mod_name);
}

te_errno
tapi_cfg_module_unload(const char *ta_name, const char *mod_name)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                CFG_MODULE_OID_FMT, ta_name, mod_name);
}

te_errno
tapi_cfg_module_filename_set(const char *ta_name, const char *mod_name,
                             const char *filename)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, filename),
                                CFG_MODULE_OID_FMT"/filename:",
                                ta_name, mod_name);
}

te_errno
tapi_cfg_module_param_add(const char *ta_name, const char *mod_name,
                          const char *param ,const char *param_value)
{
    int         rc;

    ENTRY("ta_name=%s mod_name=%s param=%s value=%s",
          ta_name, mod_name, param, param_value);

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, param_value),
                              CFG_MODULE_PARAM_OID_FMT,
                              ta_name, mod_name, param);
    if (rc != 0)
        te_log_stack_push("Addition of module '%s' param '%s' with "
                          "value '%s' on TA %s failed",
                          mod_name, ta_name, param, param_value);

    EXIT("%r", rc);

    return rc;
}

te_errno
tapi_cfg_module_int_param_add(const char *ta_name, const char *mod_name,
                              const char *param , int param_value)
{
    char *str_value = te_sprintf("%d", param_value);

    return tapi_cfg_module_param_add(ta_name, mod_name, param, str_value);
}

te_errno
tapi_cfg_module_params_add(const char *ta_name, const char *mod_name,
                           ...)
{
    va_list ap;
    char *arg;
    int rc = 0;

    va_start(ap, mod_name);
    while ((arg = va_arg(ap, char *)) != NULL)
    {
        char *val = va_arg(ap, char *);
        if (val == NULL)
        {
            ERROR("You should supply param/value in pairs we "
                  "see no pair for '%s'", arg);
            return TE_EINVAL;
        }

        rc = tapi_cfg_module_param_add(ta_name, mod_name, arg, val);
        if (rc != 0)
            break;
    }
    va_end(ap);

    return rc;
}

te_errno
tapi_cfg_module_int_params_add(const char *ta_name, const char *mod_name,
                               ...)
{
    va_list ap;
    char *arg;
    int rc = 0;

    va_start(ap, mod_name);
    while ((arg = va_arg(ap, char *)) != NULL)
    {
        int val = va_arg(ap, int);

        rc = tapi_cfg_module_int_param_add(ta_name, mod_name, arg, val);
        if (rc != 0)
            break;
    }
    va_end(ap);

    return rc;
}

/* See description in 'tapi_cfg_modules.h' */
te_errno
tapi_cfg_module_add_from_ta_dir(const char *ta_name,
                                const char *module_name,
                                te_bool     load_dependencies)
{
    te_string     module_path = TE_STRING_INIT;
    char         *module_name_underscorified;
    cfg_val_type  cvt = CVT_STRING;
    cfg_val_type  cvt_int = CVT_INTEGER;
    cfg_handle    module_handle;
    char         *ta_dir;
    int           loaded;
    te_errno      rc;
    char         *cp;

    module_name_underscorified = strdup(module_name);
    if (module_name_underscorified == NULL)
    {
        ERROR("Failed to copy the module name");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    for (cp = module_name_underscorified; *cp != '\0'; ++cp)
        *cp = (*cp == '-') ? '_' : *cp;

    rc = cfg_get_instance_fmt(&cvt, &ta_dir, "/agent:%s/dir:", ta_name);
    if (rc != 0)
    {
        ERROR("Failed to get TA directory path");
        goto out;
    }

    rc = te_string_append(&module_path, "%s/%s.ko", ta_dir, module_name);
    if (rc != 0)
    {
        ERROR("Failed to construct the module path");
        goto out;
    }

    rc = cfg_add_instance_fmt(&module_handle, CVT_INTEGER, 0,
                              "/agent:%s/module:%s", ta_name,
                              module_name_underscorified);
    if (rc != 0)
    {
        ERROR("Failed to add the module instance");
        goto out;
    }

    rc = cfg_get_instance_fmt(&cvt_int, &loaded, "/agent:%s/module:%s",
                              ta_name, module_name_underscorified);
    if (rc != 0)
    {
        ERROR("Failed to get the module 'loaded' property");
        goto out;
    }

    if (loaded)
    {
        /*
         * Hack: do not unload igb_uio until shareable resources are supported:
         * Bug 11092.
         */
        if (strcmp(module_name_underscorified, "igb_uio") == 0)
        {
            WARN("Module igb_uio is not reloaded");
            rc = 0;
            goto out;
        }

        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                  "/agent:%s/module:%s/unload_holders:",
                                  ta_name, module_name_underscorified);
        if (rc != 0)
        {
            ERROR("Failed to set unload holders for the module instance");
            goto out;
        }

        rc = tapi_cfg_module_unload(ta_name, module_name_underscorified);
        if (rc != 0)
        {
            ERROR("Failed to unload the module instance");
            goto out;
        }
    }

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, module_path.ptr),
                              "/agent:%s/module:%s/filename:",
                              ta_name, module_name_underscorified);
    if (rc != 0)
    {
        ERROR("Failed to set the module path");
        goto out;
    }

    if (load_dependencies)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                  "/agent:%s/module:%s/filename:"
                                  "/load_dependencies:", ta_name,
                                  module_name_underscorified);
        if (rc != 0)
        {
            ERROR("Failed to arrange loading the module dependencies");
            goto out;
        }
    }

    rc = tapi_cfg_module_load(ta_name, module_name_underscorified);
    if (rc != 0)
        ERROR("Failed to request the module insertion");

out:
    te_string_free(&module_path);
    free(ta_dir);
    free(module_name_underscorified);

    return TE_RC(TE_TAPI, rc);
}
