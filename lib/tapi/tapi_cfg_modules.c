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
