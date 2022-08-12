/** @file
 * @brief Test API to configure processes.
 *
 * @defgroup tapi_conf_process Processes configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of TAPI to configure processes.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

#include "te_config.h"

#include "te_defs.h"
#include "te_errno.h"
#include "te_sigmap.h"
#include "te_sleep.h"
#include "te_time.h"
#include "logger_ten.h"
#include "conf_api.h"

#include "tapi_cfg_process.h"


#define TE_CFG_TA_PS    "/agent:%s/process:%s"
/** Default timeout to sleep for between polling process status */
#define DEFAULT_POLL_FREQUENCY_MS  1000

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
    int val;

    rc = cfg_get_instance_int_sync_fmt(&val, TE_CFG_TA_PS "/status:",
                                       ta, ps_name);
    if (rc != 0)
    {
        ERROR("Cannot get status (process '%s', TA '%s'): %r", ps_name, ta, rc);
        return rc;
    }

    switch (val)
    {
        case 0:
            *status = FALSE;
            break;

        case 1:
            *status = TRUE;
            break;

        default:
            ERROR("Unsupported " TE_CFG_TA_PS "/status: value", ta, ps_name);
            return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
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

te_errno
tapi_cfg_ps_get_autorestart(const char *ta, const char *ps_name,
                            unsigned int *value)
{
    te_errno rc;
    cfg_val_type type = CVT_INTEGER;

    if (ta == NULL)
    {
        ERROR("%s: test agent name must not be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (ps_name == NULL)
    {
        ERROR("%s: process name must not be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_get_instance_fmt(&type, value,
                              TE_CFG_TA_PS "/autorestart:", ta, ps_name);
    if (rc != 0)
    {
        ERROR("Cannot get autorestart value (process '%s', TA '%s'): %r",
              ps_name, ta, rc);
    }

    return rc;
}

/** @param killpg   If @c TRUE, send signal to process group, else to process */
static te_errno
tapi_cfg_ps_kill_common(const char *ta, const char *ps_name, int signo,
                        te_bool killpg)
{
    te_errno rc;
    char *signame = map_signo_to_name(signo);

    if (signame == NULL)
    {
        ERROR("Cannot send signal with number %d (process '%s', TA '%s'): "
              "invalid signal number specified", signo, ps_name, ta);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (killpg)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, signame),
                                  TE_CFG_TA_PS "/kill:/group:",
                                  ta, ps_name);
    }
    else
    {
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, signame),
                                  TE_CFG_TA_PS "/kill:/self:",
                                  ta, ps_name);
    }

    free(signame);

    if (rc != 0)
    {
        ERROR("Cannot send a signal to %sprocess '%s' on TA '%s': %r",
              (killpg ? "group of " : ""), ps_name, ta, rc);
    }

    return rc;
}

/* See descriptions in tapi_cfg_process.h */
te_errno
tapi_cfg_ps_kill(const char *ta, const char *ps_name, int signo)
{
    return tapi_cfg_ps_kill_common(ta, ps_name, signo, FALSE);
}

/* See descriptions in tapi_cfg_process.h */
te_errno
tapi_cfg_ps_killpg(const char *ta, const char *ps_name, int signo)
{
    return tapi_cfg_ps_kill_common(ta, ps_name, signo, TRUE);
}

static te_errno
tapi_cfg_ps_get_exit_status(const char *ta, const char *ps_name,
                            tapi_cfg_ps_exit_status_t *exit_status)
{
    te_errno rc;
    tapi_cfg_ps_exit_status_t result;
    int result_type;

    assert(exit_status != NULL);

    rc = cfg_get_instance_int_sync_fmt(&result_type, TE_CFG_TA_PS
                                       "/status:/exit_status:/type:",
                                       ta, ps_name);
    if (rc != 0)
    {
        ERROR("Cannot get exit status type (process '%s', TA '%s'): %r",
              ps_name, ta, rc);
        return rc;
    }
    /*
     * Here we rely on the fact that ta_job_status_type_t and
     * tapi_cfg_ps_exit_status_type_t are actually the same
     */
    result.type = result_type;

    rc = cfg_get_instance_int_sync_fmt(&result.value, TE_CFG_TA_PS
                                       "/status:/exit_status:/value:",
                                       ta, ps_name);
    if (rc != 0)
    {
        ERROR("Cannot get exit status value (process '%s', TA '%s'): "
              "%r", ps_name, ta, rc);
        return rc;
    }

    *exit_status = result;

    return 0;
}

/* See descriptions in tapi_cfg_process.h */
te_errno
tapi_cfg_ps_wait(const char *ta, const char *ps_name, int timeout_ms,
                 tapi_cfg_ps_exit_status_t *exit_status)
{
    te_errno rc;
    struct timeval tv_start;
    struct timeval tv_now;

    if (timeout_ms >= 0)
    {
        rc = te_gettimeofday(&tv_start, NULL);
        if (rc != 0)
            return rc;
    }

    while (1)
    {
        te_bool status;

        rc = tapi_cfg_ps_get_status(ta, ps_name, &status);
        if (rc != 0)
            return rc;

        /* Process is not running, get exit status if needed and quit */
        if (!status)
        {
            if (exit_status != NULL)
                return tapi_cfg_ps_get_exit_status(ta, ps_name, exit_status);
            else
                return 0;
        }

        if (timeout_ms >= 0)
        {
            rc = te_gettimeofday(&tv_now, NULL);
            if (rc != 0)
                return rc;

            /* The behaviour is similar to ta_job_wait() */
            if (TE_US2MS(TIMEVAL_SUB(tv_now, tv_start)) > timeout_ms)
                return TE_RC(TE_TAPI, TE_EINPROGRESS);
        }

        /*
         * We do not use VSLEEP here since the sleep may be called a huge number
         * of times and the logs would become dirty
         */
        usleep(TE_MS2US(DEFAULT_POLL_FREQUENCY_MS));
    }
}
