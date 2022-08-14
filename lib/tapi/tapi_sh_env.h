/** @file
 * @brief Test API to control environment variables on the agent side
 *
 * @defgroup tapi_conf_sh_env Environment variables configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of API to deal with thread-safe stack of jumps.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_SH_ENV_H__
#define __TE_TAPI_SH_ENV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tapi_test.h"
#include "rcf_rpc.h"

/**
 * Set shell environment for the given agent and may be restart
 * a PCO so it's aware.
 *
 * @param pco            PCO handle
 * @param env_name       Name of the environment variable
 * @param env_value      New value
 * @param force          Should we only add or overwrite?
 * @param restart        Should the PCO be restarted
 *
 * @result errno
 */
static inline te_errno
tapi_sh_env_set(rcf_rpc_server *pco,
                const char *env_name, const char *env_value,
                te_bool force, te_bool restart)
{
    cfg_handle handle;
    te_errno rc = 0;

    rc = cfg_find_fmt(&handle, "/agent:%s/env:%s", pco->ta, env_name);
    if (rc != 0)
    {
        rc = 0;
        /* add the instance */
        CHECK_RC(cfg_add_instance_fmt(&handle,
                                      CFG_VAL(STRING, env_value),
                                      "/agent:%s/env:%s",
                                      pco->ta, env_name));
    }
    else if (force)
        /* exists */
        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(STRING, env_value),
                                      "/agent:%s/env:%s",
                                      pco->ta, env_name));
    else
        rc = TE_EEXIST;

    if (rc == 0 && restart)
        rcf_rpc_server_restart(pco);

    return rc;
}

/**
 * Get shell environment for the given agent.
 *
 * @param pco            PCO handle
 * @param env_name       Name of the environment variable
 * @param val            Location for value, memory is allocated with
 *                       malloc, so the obtained string must be freed after
 *                       using
 *
 * @result errno
 */
static inline te_errno
tapi_sh_env_get(rcf_rpc_server *pco, const char *env_name, char **val)
{
    te_errno      rc;

    rc = cfg_get_instance_string_fmt(val, "/agent:%s/env:%s",
                                     pco->ta, env_name);
    if (rc != 0)
        ERROR("Failed to get env %s", env_name);

    return rc;
}

/**
 * Get int shell environment for the given agent.
 *
 * @param pco            PCO handle
 * @param env_name       Name of the environment variable
 * @param val            Location for value
 *
 * @result errno
 */
static inline te_errno
tapi_sh_env_get_int(rcf_rpc_server *pco, const char *env_name, int *val)
{
    te_errno rc;
    char *strval;

    if ((rc = tapi_sh_env_get(pco, env_name, &strval)) != 0)
        return rc;

    *val = atoi(strval);
    free(strval);

    return 0;
}

/**
 * Set integer shell environment for the given agent and may be restart
 * a PCO so it's aware.
 *
 * @param pco            PCO handle
 * @param env_name       Name of the environment variable
 * @param env_value      New value
 * @param force          Should we only add or overwrite?
 * @param restart        Should the PCO be restarted
 *
 * @result errno
 */
static inline te_errno
tapi_sh_env_set_int(rcf_rpc_server *pco,
                    const char *env_name, int env_value,
                    te_bool force, te_bool restart)
{
    char env_value_str[32];
    int  res;

    if ((res = snprintf(env_value_str, sizeof(env_value_str),
                        "%d", env_value)) < 0 ||
        res > (int)sizeof(env_value_str))
    {
        ERROR("Failed to convert int value to string to set env");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_sh_env_set(pco, env_name, env_value_str, force, restart);
}

/**
 * Check whether environment variable exists, save its current value
 * if it does, then set a new value.
 *
 * @param pco               PCO handle
 * @param env_name          Name of the environment variable
 * @param existed     [out] Whether this variable already existed
 *                          in an environment or not
 * @param old_value   [out] Where to save existing value
 * @param new_value         New value
 * @param restart           Should the PCO be restarted
 *
 * @result errno
 */
static inline te_errno
tapi_sh_env_save_set(rcf_rpc_server *pco,
                     const char *env_name, te_bool *existed,
                     char **old_value,
                     const char *new_value,
                     te_bool restart)
{
    cfg_handle  handle;
    te_errno    rc = 0;

    cfg_val_type     type;

    if (existed != NULL)
        *existed = FALSE;
    if (old_value != NULL)
        *old_value = NULL;

    rc = cfg_find_fmt(&handle, "/agent:%s/env:%s", pco->ta, env_name);
    if (rc != 0)
    {
        /* add the instance */
        CHECK_RC(cfg_add_instance_fmt(&handle,
                                      CFG_VAL(STRING, new_value),
                                      "/agent:%s/env:%s",
                                      pco->ta, env_name));
    }
    else
    {
        /* exists */
        if (existed != NULL)
            *existed = TRUE;

        if (old_value != NULL)
        {
            type = CVT_STRING;
            CHECK_RC(cfg_get_instance(handle, &type, old_value));
        }
        CHECK_RC(cfg_set_instance(handle, CVT_STRING, new_value));
    }

    if (restart)
        CHECK_RC(rcf_rpc_server_restart(pco));

    return 0;
}

/**
 * Unset environment for the agent and may be restart given PCO.
 *
 * @param pco         PCO handle
 * @param env_name    Name of the environment variable
 * @param force       Ignore if the variable was not set
 * @param restart     Should the PCO be restarted?
 */
static inline te_errno
tapi_sh_env_unset(rcf_rpc_server *pco, const char *env_name,
                   te_bool force, te_bool restart)
{
    cfg_handle handle;
    te_errno rc = 0;

    rc = cfg_find_fmt(&handle, "/agent:%s/env:%s", pco->ta, env_name);
    if (rc != 0)
        rc = TE_ENOENT;
    else
        CHECK_RC(cfg_del_instance(handle, 1));

    /* no need to restart if nothing was changed */
    if (rc == 0 && restart)
        rcf_rpc_server_restart(pco);

    return (force && rc == TE_ENOENT) ? 0 : rc;
}

/**
 * Set integer shell environment and save previous value if it is necessary.
 *
 * @param pco            PCO handle
 * @param env_name       Name of the environment variable
 * @param env_value      New value
 * @param force          Should we only add or overwrite?
 * @param restart        Should the PCO be restarted
 * @param existed        [out] Whether this variable already existed
 *                       in an environment or not
 * @param old_value      [out] Where to save existing value
 *
 * @result Status code
 */
static inline te_errno
tapi_sh_env_save_set_int(rcf_rpc_server *pco,
                         const char *env_name, int env_value,
                         te_bool restart, te_bool *existed,
                         int *old_value)
{
    char env_value_str[32];
    char *old_value_str = NULL;
    int  res;

    if ((res = snprintf(env_value_str, sizeof(env_value_str),
                        "%d", env_value)) < 0 ||
        res > (int)sizeof(env_value_str))
    {
        ERROR("Failed to convert int value to string to set env");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((res = tapi_sh_env_save_set(pco, env_name, existed, &old_value_str,
                                   env_value_str, restart)) != 0)
        return res;

    if (old_value != NULL && old_value_str != NULL)
        *old_value = strtol(old_value_str, NULL, 0);

    free(old_value_str);

    return 0;
}


/**
 * Rollback integer shell environment variable
 *
 * @param pco            PCO handle
 * @param env_name       The environment variable name
 * @param existed        Did this variable exist in an environment or not
 * @param env_value      Integer value to set if the env existed
 * @param restart        Should the PCO be restarted
 *
 * @result Status code
 */
static inline te_errno
tapi_sh_env_rollback_int(rcf_rpc_server *pco, const char *env_name,
                         te_bool existed, int env_value, te_bool restart)
{
    if (!existed)
        return tapi_sh_env_unset(pco, env_name, FALSE, restart);

    return tapi_sh_env_set_int(pco, env_name, env_value, TRUE, restart);
}

/**
 * Rollback environment variable
 *
 * @param pco            PCO handle
 * @param env_name       The environment variable name
 * @param existed        Did this variable exist in an environment or not
 * @param env_value      Value to set if the env existed
 * @param restart        Should the PCO be restarted
 *
 * @result Status code
 */
static inline te_errno
tapi_sh_env_rollback(rcf_rpc_server *pco, const char *env_name,
                     te_bool existed, const char *env_value, te_bool restart)
{
    if (!existed)
        return tapi_sh_env_unset(pco, env_name, FALSE, restart);

    return tapi_sh_env_set(pco, env_name, env_value, TRUE, restart);
}

/**
 * Get boolean environment variable on engine. The function stops the test
 * if the variable keeps unexpected value.
 *
 * @param var_name  The variable name
 *
 * @return Boolean value
 */
static inline te_bool
tapi_getenv_bool(const char *var_name)
{
    const char *val;
    val = getenv(var_name);
    if (val == NULL)
        return FALSE;

    if (strcmp(val, "0") == 0)
        return FALSE;

    if (strcmp(val, "1") != 0)
        TEST_FAIL("Environment variable %s keeps non-boolean value '%s'",
                  var_name, val);

    return TRUE;
}

/**
 * Append a location to the PATH on the agent. No restart of the PCOs is done.
 *
 * @param ta            Name of the test agent
 * @param dir           Directory to be added into the PATH.
 *
 * @result Status code
 */
static inline te_errno
tapi_sh_env_ta_path_append(const char *ta, const char *dir)
{
    char new_path_str[PATH_MAX];
    char *path;
    te_errno rc;
    int res;

    rc = cfg_get_instance_string_fmt(&path, "/agent:%s/env:PATH", ta);
    if (rc != 0)
    {
        ERROR("Failed to get PATH env from agent %s", ta);
        return rc;
    }

    res = snprintf(new_path_str, sizeof(new_path_str), "%s:%s", path, dir);
    if (res >= (int)sizeof(new_path_str))
    {
        /* not that this free matters */
        free(path);
        ERROR("We're seen PATH that is longer then %z", sizeof(new_path_str));
        return TE_RC(TE_TAPI, TE_ENOBUFS);
    }

    free(path);

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, new_path_str),
                              "/agent:%s/env:PATH", ta);

    return rc;
}

/**
 * Add directories to every test agent's PATH environment variable.
 *
 * @param dirs      Array of pointers to directory names terminated by @c NULL.
 *                  If @p dirs itself is @c NULL, add most commonly required
 *                  directories (e.g. 'sbin' directories).
 *
 * @return Status code.
 */
extern int tapi_expand_path_all_ta(const char **dirs);

#ifdef __cplusplus
}
#endif

#endif

/**@} <!-- END tapi_conf_sh_env --> */
