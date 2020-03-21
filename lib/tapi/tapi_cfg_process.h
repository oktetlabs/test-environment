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
 *
 * @param ta            Test Agent.
 * @param ps_name       Process name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_start(const char *ta, const char *ps_name);

/**
 * Stop process.
 *
 * @param ta            Test Agent.
 * @param ps_name       Process name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ps_stop(const char *ta, const char *ps_name);

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PROCESS_H__ */

/**@} <!-- END tapi_conf_process --> */
