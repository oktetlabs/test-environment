/** @file
 * @brief Configurator API for Agent job control
 *
 * @defgroup tapi_cfg_job Configurator API for Agent job control (tapi_cfg_job)
 * @ingroup tapi_job
 * @{
 *
 * Configurator API for Agent job control.
 * Since the API is based on "/agent/process" Configurator subtree,
 * terms "job" and "process" are used interchangeably in this file.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Dilshod Urazov <Dilshod.Urazov@oktetlabs.ru>
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_JOB_H__
#define __TE_TAPI_CFG_JOB_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Cause of process termination */
typedef enum cfg_job_exit_status_type_t {
    /** Process terminated normally (by exit() or return from main) */
    CFG_JOB_EXIT_STATUS_EXITED,
    /** Process was terminated by a signal */
    CFG_JOB_EXIT_STATUS_SIGNALED,
    /** The cause of process termination is not known */
    CFG_JOB_EXIT_STATUS_UNKNOWN,
} cfg_job_exit_status_type_t;

/** Structure that represents status of a terminated process */
typedef struct cfg_job_exit_status_t {
    /** Cause of process termination */
    cfg_job_exit_status_type_t type;
    /**
     * Either an exit status of the process or a number of a signal
     * which caused the process termination
     */
    int value;
} cfg_job_exit_status_t;

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
extern te_errno cfg_job_add_arg(const char *ta, const char *ps_name,
                                unsigned int order, const char *arg);

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
extern te_errno cfg_job_add_env(const char *ta, const char *ps_name,
                                const char *env_name, const char *value);

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
extern te_errno cfg_job_add(const char *ta, const char *ps_name,
                            const char *exe, te_bool start);

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
 * @sa cfg_job_set_autorestart
 */
extern te_errno cfg_job_start(const char *ta, const char *ps_name);

/**
 * Send a signal to the process.
 *
 * @param      ta                 Test Agent.
 * @param      ps_name            Process.
 * @param      signo              Number of signal to send.
 *
 * @return     Status code
 */
extern te_errno cfg_job_kill(const char *ta, const char *ps_name, int signo);

/**
 * Send a signal to the process's porcess group.
 *
 * @param      ta                 Test Agent.
 * @param      ps_name            Process.
 * @param      signo              Number of signal to send.
 *
 * @return     Status code
 */
extern te_errno cfg_job_killpg(const char *ta, const char *ps_name, int signo);

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
 * @sa cfg_job_set_autorestart
 */
extern te_errno cfg_job_get_status(const char *ta, const char *ps_name,
                                   te_bool *status);

/**
 * Wait for a process completion (or check its status if @p timeout_ms is zero).
 *
 * @param[in]  ta                 Test Agent.
 * @param[in]  ps_name            Process.
 * @param[in]  timeout_ms         Time to wait for the process. @c 0 means to
 *                                check current status and exit, negative value
 *                                means that the call of the function blocks
 *                                until the process changes its status.
 * @param[out] exit_status        Process exit status location, may be @c NULL.
 *
 * @return     Status code
 * @retval     0               The process completed running or was never
 *                             started
 * @retval     TE_EINPROGRESS  The process is still running.
 *
 * @note Parameters of the process are allowed to be changed after
 *       successful call of this function.
 */
extern te_errno cfg_job_wait(const char *ta, const char *ps_name,
                             int timeout_ms,
                             cfg_job_exit_status_t *exit_status);

/**
 * Stop process.
 * For autorestart processes this function will stop the process and prevent
 * the autorestart subsystem from starting the process over until
 * cfg_job_start() is called.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process name.
 *
 * @return Status code
 *
 * @note Successfull call of this function guarantees that
 *       cfg_job_get_status() will return @c FALSE, thus the process
 *       parameters are allowed to be changed
 *       (using cfg_job_add_arg(), etc.).
 *
 * @sa cfg_job_set_autorestart
 */
extern te_errno cfg_job_stop(const char *ta, const char *ps_name);

/**
 * Delete process.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process name.
 *
 * @return Status code
 */
extern te_errno cfg_job_del(const char *ta, const char *ps_name);

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
extern te_errno cfg_job_set_autorestart(const char *ta, const char *ps_name,
                                        unsigned int value);
/**
 * Get autorestart timeout.
 *
 * @param[in]  ta       Test Agent.
 * @param[in]  ps_name  Process.
 * @param[out] value    Autorestart timeout in seconds. If @c 0
 *                      the autorestart is disabled.
 *
 * @return Status code
 */
extern te_errno cfg_job_get_autorestart(const char *ta, const char *ps_name,
                                        unsigned int *value);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_CFG_JOB_H__ */

/**@} <!-- END tapi_cfg_job --> */
