/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Data Center Bridging eXchange configuration
 *
 * @defgroup tapi_conf_dcbx Data Center Bridging eXchange configuration
 * @ingroup tapi_conf
 * @{
 *
 * @section tapi_cfg_dcbx_example Example of usage
 *
 * Using TAPI to configure DCBX.
 *
 * Retrieve DCBX configuration on agent Agt_A, similarly to:
 *
 * @cmd{dcb dcbx show dev eth1}
 * @code
 * #include "tapi_cfg_dcbx.h"
 *
 * const char *ta = "Agt_A";
 * const char *if_name = "eth1";
 * tapi_dcb_dcbx_state mode;
 *
 * CHECK_RC(tapi_cfg_dcbx_get(ta, if_name, &mode));
 * @endcode
 *
 * Configure the DCBX mode on agent Agt_A, similarly to:
 *
 * @cmd{dcb dcbx set dev eth1 host ieee}
 * @code
 * #include "tapi_cfg_dcbx.h"
 *
 * const char *ta = "Agt_A";
 * const char *if_name = "eth1";
 * const tapi_dcb_dcbx_state mode = TAPI_DCB_DCBX_HOST |
 *                                  TAPI_DCB_DCBX_VER_IEEE;
 *
 * CHECK_RC(tapi_cfg_dcbx_set(ta, if_name, mode));
 * @endcode
 */

#ifndef __TE_TAPI_CFG_DCBX_H__
#define __TE_TAPI_CFG_DCBX_H__

#include "tapi_dcb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Configuration OID format for a DCBX instance interface. */
#define TAPI_CFG_DCBX_FMT \
    "/agent:%s/interface:%s/dcb:/dcbx:"

/**
 * Check whether an interface has the DCBX instance.
 *
 * The instance exists if the interface supports DCB and its driver
 * reports the DCBX mode. The DCB subtree of the interface is
 * synchronized first.
 *
 * Use it to decide whether a test can run: once the instance is known to
 * exist, any error of the other DCBX functions is a failure,
 * @c TE_ENOENT included.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[out] present  Whether the instance exists.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval TE_EINVAL    An argument is @c NULL.
 * @retval Other        Synchronization of the DCB subtree failed.
 */
extern te_errno tapi_cfg_dcbx_is_present(const char *ta,
                                         const char *if_name,
                                         bool *present);

/**
 * Get bitmask of Data Center Bridging Exchange mode flags.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[out] dcbx     Retrieved DCBX state.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval TE_ENOENT    There is no DCBX instance: the interface does not
 *                      support DCB, its driver does not report the DCBX
 *                      mode, or the interface is gone.
 * @retval TE_EINVAL    An argument is @c NULL.
 * @retval Other        Error reported by the agent while reading the mode.
 */
extern te_errno tapi_cfg_dcbx_get(const char *ta,
                                  const char *if_name,
                                  tapi_dcb_dcbx_state *dcbx);

/**
 * Set bitmask of Data Center Bridging Exchange mode flags.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[in] dcbx      DCBX state.
 *
 * @return Status code.
 * @retval 0             The mode is set, or it is already in effect (the
 *                       driver is not asked then).
 * @retval TE_ENOENT     There is no DCBX instance (see tapi_cfg_dcbx_get()).
 * @retval TE_EOPNOTSUPP The driver cannot set the DCBX mode.
 * @retval TE_EFAIL      The driver returned a non-zero status that is not
 *                       an errno (in-tree drivers return 1 on any failure).
 * @retval TE_EINVAL     An argument is @c NULL.
 * @retval Other         Error returned by the kernel or by the driver, or
 *                       reported by the agent.
 */
extern te_errno tapi_cfg_dcbx_set(const char *ta,
                                  const char *if_name,
                                  tapi_dcb_dcbx_state dcbx);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_DCBX_H__ */

/**@} <!-- END tapi_conf_dcbx --> */
