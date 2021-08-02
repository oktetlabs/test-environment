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
#include "tapi_cfg_modules.h"

#define CFG_MODULE_OID_FMT "/agent:%s/module:%s"
#define CFG_MODULE_PARAM_OID_FMT CFG_MODULE_OID_FMT"/parameter:%s"

static char *
tapi_cfg_module_rsrc_name(const char *mod_name)
{
    return te_string_fmt("module:%s", mod_name);
}

te_errno
tapi_cfg_module_change_finish(const char *ta_name, const char *mod_name)
{
    char *rsrc_name = tapi_cfg_module_rsrc_name(mod_name);
    te_errno rc;

    if (rsrc_name == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                              "/agent:%s/rsrc:%s/shared:",
                              ta_name, rsrc_name);
    if (rc != 0)
        ERROR("Failed to set shared property of the resource '%s' on %s: %r",
              rsrc_name, ta_name, rc);

    free(rsrc_name);

    return rc;
}

static te_errno
tapi_cfg_module_get_shared(const char *ta_name, const char *mod_name,
                              te_bool *shared)
{
    cfg_val_type val_type = CVT_INTEGER;
    int result_shared;
    char *rsrc_name = tapi_cfg_module_rsrc_name(mod_name);
    te_errno rc;

    if (rsrc_name == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = cfg_get_instance_fmt(&val_type, &result_shared,
                              "/agent:%s/rsrc:%s/shared:",
                              ta_name, rsrc_name);
    if (rc == 0)
        *shared = !!result_shared;
    else
        ERROR("Failed to get shared property of the resource '%s' on %s: %r",
              rsrc_name, ta_name, rc);

    free(rsrc_name);

    return rc;
}

static te_errno
tapi_cfg_module_check_exclusive_rsrc(const char *ta_name, const char *mod_name)
{
    te_bool shared;
    te_errno rc;

    rc = tapi_cfg_module_get_shared(ta_name, mod_name, &shared);
    if (rc != 0)
        return rc;

    if (shared)
    {
        ERROR("Module '%s' on %s must be grabbed as an exclusive resource",
              mod_name, ta_name);
        return TE_RC(TE_TAPI, TE_EPERM);
    }

    return 0;
}

static te_errno
tapi_cfg_module_grab(const char *ta_name, const char *mod_name,
                     te_bool *shared)
{
    static const unsigned int grab_timeout_ms = 3000;
    char *rsrc_name = NULL;
    char *module_oid = NULL;
    te_bool set_oid = TRUE;
    cfg_val_type val_type = CVT_STRING;
    char *old_oid;
    int result_shared;
    te_errno rc;

    rsrc_name = tapi_cfg_module_rsrc_name(mod_name);
    if (rsrc_name == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    module_oid = te_string_fmt(CFG_MODULE_OID_FMT, ta_name, mod_name);
    if (module_oid == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    rc = cfg_get_instance_fmt(&val_type, &old_oid, "/agent:%s/rsrc:%s",
                              ta_name, rsrc_name);
    if (rc == 0)
    {
        if (*old_oid != '\0' && strcmp(old_oid, module_oid))
        {
            ERROR("Failed to grab a module '%s': invalid existing resource",
                  mod_name);
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto out;
        }

        set_oid = *old_oid == '\0';
    }
    else
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, ""),
                                  "/agent:%s/rsrc:%s", ta_name, rsrc_name);
        if (rc != 0)
            goto out;
    }

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                              "/agent:%s/rsrc:%s/fallback_shared:",
                              ta_name, rsrc_name);
    if (rc != 0)
        goto out;

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, grab_timeout_ms),
                              "/agent:%s/rsrc:%s/acquire_attempts_timeout:",
                              ta_name, rsrc_name);
    if (rc != 0)
        goto out;

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, *shared),
                              "/agent:%s/rsrc:%s/shared:",
                              ta_name, rsrc_name);
    if (rc != 0)
        goto out;

    if (set_oid)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, module_oid),
                                  "/agent:%s/rsrc:%s", ta_name, rsrc_name);
        if (rc != 0)
            goto out;
    }

    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type, &result_shared,
                              "/agent:%s/rsrc:%s/shared:",
                              ta_name, rsrc_name);
    if (rc != 0)
        goto out;

    *shared = !!result_shared;

out:
    free(rsrc_name);
    free(module_oid);

    return rc;
}

te_errno
tapi_cfg_module_add(const char *ta_name, const char *mod_name, te_bool load)
{
    int         rc;
    te_bool shared = FALSE;
    cfg_val_type  cvt = CVT_INTEGER;
    int loaded;

    ENTRY("ta_name=%s mod_name=%s load=%s", ta_name, mod_name,
          load ? "true" : "false");

    rc = tapi_cfg_module_grab(ta_name, mod_name, &shared);
    if (rc != 0)
    {
        ERROR("Failed to grab module '%s' as a resource on %s: %r",
              mod_name, ta_name, rc);
        goto out;
    }

    rc = cfg_get_instance_fmt(NULL, NULL, CFG_MODULE_OID_FMT, ta_name,
                              mod_name);
    if (rc != 0)
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                                  CFG_MODULE_OID_FMT, ta_name, mod_name);
        if (rc != 0)
        {
            te_log_stack_push("Addition of module '%s' on TA %s failed",
                              mod_name, ta_name);
            goto out;
        }

        rc = cfg_get_instance_fmt(&cvt, &loaded, CFG_MODULE_OID_FMT "/loaded:",
                                  ta_name, mod_name);
        if (rc != 0)
        {
            ERROR("Cannot get module '%s' after addition on %s: %r",
                  mod_name, ta_name, rc);
            goto out;
        }
    }

    if (!loaded && load)
    {
        rc = tapi_cfg_module_load(ta_name, mod_name);
        if (rc != 0)
            goto out;
    }

out:
    EXIT("%r", rc);

    return rc;
}

static te_errno
tapi_cfg_module_loaded_set(const char *ta_name, const char *mod_name,
                           te_bool loaded)
{
    te_errno rc;

    rc = tapi_cfg_module_check_exclusive_rsrc(ta_name, mod_name);
    if (rc != 0)
        return rc;

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, loaded ? 1 : 0),
                                CFG_MODULE_OID_FMT "/loaded:", ta_name,
                                mod_name);
}

te_errno
tapi_cfg_module_load(const char *ta_name, const char *mod_name)
{
    return tapi_cfg_module_loaded_set(ta_name, mod_name, TRUE);
}

te_errno
tapi_cfg_module_unload(const char *ta_name, const char *mod_name)
{
    return tapi_cfg_module_loaded_set(ta_name, mod_name, FALSE);
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

    rc = tapi_cfg_module_check_exclusive_rsrc(ta_name, mod_name);
    if (rc != 0)
        return rc;

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

te_errno
tapi_cfg_module_add_from_ta_dir_fb(const char *ta_name,
                                   const char *module_name,
                                   te_bool     load_dependencies,
                                   te_bool     fallback)
{
    te_string     module_path = TE_STRING_INIT;
    cfg_val_type  cvt = CVT_STRING;
    cfg_val_type  cvt_int = CVT_INTEGER;
    char         *ta_dir;
    int           loaded;
    te_errno      rc;

    rc = cfg_get_instance_fmt(&cvt, &ta_dir, "/agent:%s/dir:", ta_name);
    if (rc != 0)
    {
        ERROR("Failed to get TA %s directory path: %r", ta_name, rc);
        goto out;
    }

    rc = te_string_append(&module_path, "%s/%s.ko", ta_dir, module_name);
    if (rc != 0)
    {
        ERROR("Failed to construct the module '%s' path: %r",
              module_name, rc);
        goto out;
    }

    rc = tapi_cfg_module_add(ta_name, module_name, 0);
    if (rc != 0)
    {
        ERROR("Failed to add the module '%s' on %s: %r",
              module_name, ta_name, rc);
        goto out;
    }

    te_bool shared;
    rc = tapi_cfg_module_get_shared(ta_name, module_name, &shared);
    if (rc != 0)
        goto out;

    rc = cfg_get_instance_fmt(&cvt_int, &loaded, "/agent:%s/module:%s/loaded:",
                              ta_name, module_name);
    if (rc != 0)
    {
        ERROR("Failed to get the module '%s' 'loaded' property on %s: %r",
              module_name, ta_name, rc);
        goto out;
    }

    if (shared)
    {
        if (!loaded)
        {
            ERROR("Module '%s' resource was grabbed as shared on %s and "
                  "module was not loaded", module_name, ta_name);
            rc = TE_RC(TE_TAPI, TE_EPERM);
            goto out;
        }

        /*
         * Module was grabbed as a shared resource and it is loaded,
         * modification is not allowed.
         */
        rc = 0;
        goto out;
    }

    if (loaded)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                  "/agent:%s/module:%s/unload_holders:",
                                  ta_name, module_name);
        if (rc != 0)
        {
            ERROR("Failed to set unload holders for the module '%s' on %s: %r",
                  module_name, ta_name, rc);
            goto out;
        }

        rc = tapi_cfg_module_unload(ta_name, module_name);
        if (rc != 0)
        {
            ERROR("Failed to unload the module '%s' on %s: %r",
                  module_name, ta_name, rc);
            goto out;
        }

        rc = tapi_cfg_module_add(ta_name, module_name, 0);
        if (rc != 0)
        {
            ERROR("Failed to add the module '%s' after unloading on %s: %r",
                  module_name, ta_name, rc);
            goto out;
        }
    }

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, module_path.ptr),
                              "/agent:%s/module:%s/filename:",
                              ta_name, module_name);
    if (rc != 0)
    {
        ERROR("Failed to set the module '%s' path on %s: %r",
              module_name, ta_name, rc);
        goto out;
    }

    if (load_dependencies)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                  "/agent:%s/module:%s/filename:"
                                  "/load_dependencies:", ta_name,
                                  module_name);
        if (rc != 0)
        {
            ERROR("Failed to arrange loading the module '%s' dependencies on %s: %r",
                  module_name, ta_name, rc);
            goto out;
        }
    }

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, fallback ? 1 : 0),
                              "/agent:%s/module:%s/filename:/fallback:",
                              ta_name, module_name);
    if (rc != 0)
    {
        ERROR("Failed to set fallback node for the module '%s' on %s: %r",
              module_name, ta_name, rc);
        goto out;
    }

    rc = tapi_cfg_module_load(ta_name, module_name);
    if (rc != 0)
        ERROR("Failed to request the module '%s' insertion on %s: %r",
              module_name, ta_name, rc);

out:
    te_string_free(&module_path);
    free(ta_dir);

    return TE_RC(TE_TAPI, rc);
}

/* See description in 'tapi_cfg_modules.h' */
te_errno
tapi_cfg_module_add_from_ta_dir(const char *ta_name,
                                const char *module_name,
                                te_bool     load_dependencies)
{
    return tapi_cfg_module_add_from_ta_dir_fb(ta_name, module_name,
                                              load_dependencies, FALSE);
}

/* See description in 'tapi_cfg_modules.h' */
te_errno
tapi_cfg_module_add_from_ta_dir_or_fallback(const char *ta_name,
                                            const char *module_name,
                                            te_bool     load_dependencies)
{
    return tapi_cfg_module_add_from_ta_dir_fb(ta_name, module_name,
                                              load_dependencies, TRUE);
}

/* See description in 'tapi_cfg_modules.h' */
te_errno
tapi_cfg_module_version_get(const char *ta_name, const char *module_name,
                            char **version)
{
    return cfg_get_instance_fmt(NULL, version, CFG_MODULE_OID_FMT
                                "/version:", ta_name, module_name);
}
