/** @file
 * @brief TAPI to handle Configurator job factory
 *
 * @defgroup tapi_job_factory_cfg Configurator job factory control (tapi_job_factory_cfg)
 * @ingroup tapi_job
 * @{
 *
 * TAPI to handle Configurator job factory.
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TAPI_JOB_FACTORY_CFG_H__
#define __TAPI_JOB_FACTORY_CFG_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tapi_job_factory_t;

/**
 * Create a job factory by Configurator on Test Agent @p ta.
 *
 * @param[in]  ta              Test Agent
 * @param[out] factory         Job factory handle
 */
extern te_errno tapi_job_factory_cfg_create(
                                        const char *ta,
                                        struct tapi_job_factory_t **factory);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_JOB_FACTORY_CFG_H__ */

/** @} <!-- END tapi_job_factory_cfg --> */