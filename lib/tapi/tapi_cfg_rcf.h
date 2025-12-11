/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RCF interface via CS
 *
 * @defgroup tapi_conf_rcf RCF interface via Configurator
 * @ingroup tapi_conf
 * @{
 */

#ifndef __TE_TAPI_CFG_RCF_H__
#define __TE_TAPI_CFG_RCF_H__

#include "te_kvpair.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create Test Agent via Configurator.
 *
 * @param ta                Test agent name.
 * @param type              Test agent type.
 * @param rcflib            RCF library name to create Test Agent.
 * @param conf              Test agent configuration.
 * @param flags             Test agent flags.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_rcf_add_ta(const char *ta,
                                    const char *type,
                                    const char *rcflib,
                                    const te_kvpair_h *conf,
                                    unsigned int flags);

/**
 * Destroy Test Agent via Configurator.
 *
 * @param ta                Test agent name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_rcf_del_ta(const char *ta);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_RCF_H__ */

/**@} <!-- END tapi_conf_rcf --> */
