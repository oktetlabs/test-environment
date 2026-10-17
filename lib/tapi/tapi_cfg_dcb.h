/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Data Center Bridging configuration common helpers
 *
 * @defgroup tapi_conf_dcb Data Center Bridging configuration helpers
 * @ingroup tapi_conf
 * @{
 *
 * Helpers shared by the DCBX, ETS and PFC configuration TAPIs.
 */

#ifndef __TE_TAPI_CFG_DCB_H__
#define __TE_TAPI_CFG_DCB_H__

#include "tapi_dcb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Configuration OID format for the DCB node of an interface. */
#define TAPI_CFG_DCB_FMT "/agent:%s/interface:%s/dcb:"

/**
 * Check whether an interface has an instance of a DCB node.
 *
 * The DCB subtree of the interface is synchronized first.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[in] node      Name of the node under the DCB node of the
 *                      interface (e.g. @c "ets").
 * @param[out] present  Whether the instance exists.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval TE_EINVAL    An argument is @c NULL.
 * @retval Other        Synchronization of the DCB subtree failed.
 */
extern te_errno tapi_cfg_dcb_node_is_present(const char *ta,
                                             const char *if_name,
                                             const char *node,
                                             bool *present);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_DCB_H__ */

/**@} <!-- END tapi_conf_dcb --> */
