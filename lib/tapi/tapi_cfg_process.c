/** @file
 * @brief Test API to configure processes.
 *
 * @defgroup tapi_conf_process Processes configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of TAPI to configure processes.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Dilshod Urazov <Dilshod.Urazov@oktetlabs.ru>
 */

#include "te_config.h"

#include "te_defs.h"
#include "te_errno.h"
#include "logger_ten.h"
#include "conf_api.h"

#include "tapi_cfg_process.h"


#define TE_CFG_TA_PS    "/agent:%s/process:%s"


/* See descriptions in tapi_cfg_process.h */
te_errno
tapi_cfg_ps_add(const char *ta, const char *ps_name,
                  const char *exe, te_bool start)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL, TE_CFG_TA_PS, ta, ps_name);
    if (rc != 0)
    {
        ERROR("Cannot add process '%s' to TA '%s': %r", ps_name, ta, rc);
        return rc;
    }

    rc  = cfg_set_instance_fmt(CFG_VAL(STRING, exe),
                               TE_CFG_TA_PS "/exe:", ta, ps_name);
    if (rc != 0)
    {
        ERROR("Cannot set exe '%s' in process '%s': %r", exe, ps_name, rc);
        return tapi_cfg_ps_del(ta, ps_name);
    }

    return start ? tapi_cfg_ps_start(ta, ps_name) : 0;
}

/* See descriptions in tapi_cfg_process.h */
te_errno
tapi_cfg_ps_del(const char *ta, const char *ps_name)
{
    te_errno rc;

    rc = cfg_del_instance_fmt(FALSE, TE_CFG_TA_PS, ta, ps_name);
    if (rc != 0)
        ERROR("Cannot delete process '%s' from TA '%s': %r", ps_name, ta, rc);

    return rc;
}

/* See descriptions in tapi_cfg_process.h */
te_errno
tapi_cfg_ps_start(const char *ta, const char *ps_name)
{
    te_errno rc;

    rc  = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                               TE_CFG_TA_PS "/status:", ta, ps_name);
    if (rc != 0)
        ERROR("Cannot start process '%s' on TA '%s': %r", ps_name, ta, rc);

    return rc;
}

/* See descriptions in tapi_cfg_process.h */
te_errno
tapi_cfg_ps_stop(const char *ta, const char *ps_name)
{
    te_errno rc;

    rc  = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                               TE_CFG_TA_PS "/status:", ta, ps_name);
    if (rc != 0)
        ERROR("Cannot stop process '%s' on TA '%s': %r", ps_name, ta, rc);

    return rc;
}

/* See descriptions in tapi_cfg_process.h */
te_errno
tapi_cfg_ps_get_status(const char *ta, const char *ps_name, te_bool *status)
{
    te_errno rc;
    cfg_val_type type = CVT_INTEGER;

    rc = cfg_get_instance_fmt(&type, status, TE_CFG_TA_PS "/status:",
                              ta, ps_name);
    if (rc != 0)
        ERROR("Cannot get status (process '%s', TA '%s'): %r", ps_name, ta, rc);

    return rc;
}

te_errno
tapi_cfg_ps_add_arg(const char *ta, const char *ps_name,
                    unsigned int order, const char *arg)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, arg),
                              TE_CFG_TA_PS "/arg:%d", ta, ps_name, order);
    if (rc != 0)
    {
        ERROR("Cannot add argument '%s' (process '%s', TA '%s'): %r", arg,
              ps_name, ta, rc);
        return rc;
    }

    return rc;
}

te_errno
tapi_cfg_ps_add_env(const char *ta, const char *ps_name,
                    const char *env_name, const char *value)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, value),
                              TE_CFG_TA_PS "/env:%s", ta, ps_name, env_name);
    if (rc != 0)
    {
        ERROR("Cannot add env variable '%s' (process '%s', TA '%s'): %r",
              env_name, ps_name, ta, rc);
        return rc;
    }

    return rc;
}

te_errno
tapi_cfg_ps_add_opt(const char *ta, const char *ps_name,
                    const char *opt_name, const char *value)
{
    te_errno rc;

    if (value == NULL)
        value = "";

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, value),
                              TE_CFG_TA_PS "/option:%s", ta,
                              ps_name, opt_name);
    if (rc != 0)
    {
        ERROR("Cannot add option '%s' (process '%s', TA '%s'): %r",
              opt_name, ps_name, ta, rc);
    }

    return rc;
}

te_errno
tapi_cfg_ps_set_long_opt_sep(const char *ta, const char *ps_name,
                             const char *value)
{
    te_errno rc;

    if (value == NULL)
        value = "";

    rc  = cfg_set_instance_fmt(CFG_VAL(STRING, value),
                               TE_CFG_TA_PS "/long_option_value_separator:",
                               ta, ps_name);
    if (rc != 0)
    {
        ERROR("Cannot set separator (process '%s', TA '%s'): %r",
              ps_name, ta, rc);
    }

    return rc;
}

te_errno
tapi_cfg_ps_set_autorestart(const char *ta, const char *ps_name,
                            unsigned int value)
{
    te_errno rc;

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                              TE_CFG_TA_PS "/autorestart:", ta, ps_name);
    if (rc != 0)
    {
        ERROR("Cannot set autorestart value (process '%s', TA '%s'): %r",
              ps_name, ta, rc);
    }

    return rc;
}
