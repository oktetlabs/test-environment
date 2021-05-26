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

#ifndef __TE_TAPI_CFG_PROCESS_H__
#define __TE_TAPI_CFG_PROCESS_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add process.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process name.
 * @param exe           Executable to run.
 * @param start         Start it just after addition
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_add(const char *ta, const char *ps_name,
                                  const char *exe, te_bool start);

/**
 * Delete process.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_del(const char *ta, const char *ps_name);

/**
 * Start process.
 * For autorestart processes this function should be called only once.
 * The following process executions will be done by the autorestart subsystem.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process name.
 *
 * @return Status code
 *
 * @sa tapi_cfg_ps_set_autorestart
 */
extern te_errno tapi_cfg_ps_start(const char *ta, const char *ps_name);

/**
 * Stop process.
 * For autorestart processes this function will stop the process and prevent
 * the autorestart subsystem from starting the process over until
 * tapi_cfg_ps_start() is called.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process name.
 *
 * @return Status code
 *
 * @note Successfull call of this function guarantees that
 *       tapi_cfg_ps_get_status() will return @c FALSE, thus the process
 *       parameters are allowed to be changed
 *       (using tapi_cfg_ps_add_arg(), etc.).
 *
 * @sa tapi_cfg_ps_set_autorestart
 */
extern te_errno tapi_cfg_ps_stop(const char *ta, const char *ps_name);

/**
 * Get current process status.
 *
 * @param[in]  ta       Test Agent.
 * @param[in]  ps_name  Process name.
 * @param[out] status   Process current status. For autorestart processes
 *                      @c TRUE means that the autorestart subsystem is working
 *                      with the process and it will be restarted when needed;
 *                      @c FALSE means that the process is most likely not
 *                      running and will not be started by the autorestart
 *                      subsystem. For other processes @c TRUE means that
 *                      the process is running, @c FALSE that it is not.
 *
 * @return Status code
 *
 * @note If @p status is @c FALSE, the process parameters are allowed
 *       to be changed.
 *
 * @sa tapi_cfg_ps_set_autorestart
 */
extern te_errno tapi_cfg_ps_get_status(const char *ta, const char *ps_name,
                                       te_bool *status);

/**
 * Add process argument.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process.
 * @param order         Relative order.
 * @param arg           Argument itself.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_add_arg(const char *ta, const char *ps_name,
                                    unsigned int order,
                                    const char *arg);

/**
 * Add environment variable.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process.
 * @param env_name      Variable name.
 * @param value         Variable value.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_add_env(const char *ta, const char *ps_name,
                                    const char *env_name,
                                    const char *value);

/**
 * Add option.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process.
 * @param opt_name      Option name.
 * @param value         Option value.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_add_opt(const char *ta, const char *ps_name,
                                    const char *opt_name,
                                    const char *value);

/**
 * Set long option value separator.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process.
 * @param value         Value to set.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_set_long_opt_sep(const char *ta,
                                             const char *ps_name,
                                             const char *value);

/**
 * Set autorestart timeout.
 * The value represents a frequency with which the autorestart subsystem
 * will check whether the process stopped running (regardless of the reason)
 * and restart it if it did.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process.
 * @param value         Autorestart timeout in seconds or @c 0 to disable
 *                      autorestart for the process.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_set_autorestart(const char *ta,
                                            const char *ps_name,
                                            unsigned int value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PROCESS_H__ */

/**@} <!-- END tapi_conf_process --> */
