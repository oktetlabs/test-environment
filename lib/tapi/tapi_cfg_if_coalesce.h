/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Definition of test API for Network Interface Interrupt
 * Coalescing settings.
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_IF_COALESCE_H__
#define __TE_TAPI_CFG_IF_COALESCE_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get interrupt coalescing parameter value.
 *
 * @param ta        Test Agent name
 * @param if_name   Interface name
 * @param param     Parameter name
 * @param val       Where to save obtained value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_coalesce_get(const char *ta,
                                         const char *if_name,
                                         const char *param,
                                         uint64_t *val);
/**
 * Set interrupt coalescing parameter value.
 *
 * @param ta        Test Agent name
 * @param if_name   Interface name
 * @param param     Parameter name
 * @param val       Value to set
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_coalesce_set(const char *ta,
                                         const char *if_name,
                                         const char *param,
                                         uint64_t val);

/**
 * Set interrupt coalescing parameter value locally, to commit
 * it later (may be together with other changes).
 *
 * @param ta        Test Agent name
 * @param if_name   Interface name
 * @param param     Parameter name
 * @param val       Value to set
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_coalesce_set_local(const char *ta,
                                               const char *if_name,
                                               const char *param,
                                               uint64_t val);

/**
 * Commit changes made by a few tapi_cfg_if_coalesce_set_local() calls.
 *
 * @param ta        Test Agent name
 * @param if_name   Interface name
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_coalesce_commit(const char *ta,
                                            const char *if_name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CFG_IF_COALESCE_H__ */
