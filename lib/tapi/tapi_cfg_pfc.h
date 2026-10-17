/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Priority-based Flow Control configuration
 *
 * @defgroup tapi_conf_pfc Priority-based Flow Control configuration
 * @ingroup tapi_conf
 * @{
 *
 * @section tapi_cfg_pfc_example Example of usage
 *
 * Using TAPI to configure PFC.
 *
 * Retrieve PFC configuration on agent Agt_A, similarly to:
 *
 * @cmd{dcb pfc show dev eth1}
 * @code
 * #include "tapi_cfg_pfc.h"
 *
 * const char *ta = "Agt_A";
 * const char *if_name = "eth1";
 * tapi_dcb_pfc_state pfc;
 *
 * CHECK_RC(tapi_cfg_pfc_get(ta, if_name, &pfc));
 * @endcode
 *
 * Configure the PFC delay on agent Agt_A, similarly to:
 *
 * @cmd{dcb pfc set dev eth1 delay 0}
 * @code
 * #include "tapi_cfg_pfc.h"
 *
 * const char *ta = "Agt_A";
 * const char *if_name = "eth1";
 * tapi_dcb_pfc_state pfc;
 *
 * CHECK_RC(tapi_cfg_pfc_get(ta, if_name, &pfc));
 * pfc.config.delay = 0;
 * CHECK_RC(tapi_cfg_pfc_set(ta, if_name, &pfc.config));
 * @endcode
 */

#ifndef __TE_TAPI_CFG_PFC_H__
#define __TE_TAPI_CFG_PFC_H__

#include "tapi_dcb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Configuration OID format for a PFC instance interface. */
#define TAPI_CFG_PFC_FMT \
    "/agent:%s/interface:%s/dcb:/pfc:"

/** Configuration OID format for a PFC parameter. */
#define TAPI_CFG_PFC_PARAM_FMT \
    TAPI_CFG_PFC_FMT "/%s:"

/** Configuration OID format for a indexed PFC parameter. */
#define TAPI_CFG_PFC_PARAM_ID_FMT \
    TAPI_CFG_PFC_PARAM_FMT "%u"

/**
 * Check whether an interface has the PFC instance.
 *
 * The instance exists if the interface supports DCB and its driver
 * provides PFC data. The DCB subtree of the interface is synchronized
 * first.
 *
 * Use it to decide whether a test can run: once the instance is known to
 * exist, any error of the other PFC functions is a failure,
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
extern te_errno tapi_cfg_pfc_is_present(const char *ta,
                                        const char *if_name,
                                        bool *present);

/**
 * Retrieve PFC configuration.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[out] pfc      Retrieved PFC state.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval TE_ENOENT    There is no PFC instance: the interface does not
 *                      support DCB, its driver provides no PFC data (the
 *                      getter is absent or fails), or the interface is
 *                      gone.
 * @retval TE_EINVAL    An argument is @c NULL.
 * @retval Other        Error reported by the agent while reading the
 *                      configuration (e.g. @c TE_ENOTCONN, @c TE_EPROTO,
 *                      or an error returned by the kernel).
 */
extern te_errno tapi_cfg_pfc_get(const char *ta, const char *if_name,
                                 tapi_dcb_pfc_state *pfc);

/**
 * Set PFC configuration using a single Configurator commit.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[in] pfc       PFC configuration.
 *
 * @return Status code.
 * @retval 0            The kernel accepted the request. This does not prove
 *                      that the driver applied it: the kernel reports
 *                      success if the driver has no setter, and the driver
 *                      may adjust the values. Read the configuration back
 *                      to check what is applied.
 * @retval TE_ENOENT    There is no PFC instance (see tapi_cfg_pfc_get()).
 * @retval TE_EINVAL    An argument is @c NULL, or the driver rejected the
 *                      configuration with @c EINVAL.
 * @retval Other        Error returned by the driver (e.g. @c TE_EOPNOTSUPP,
 *                      @c TE_ERANGE, @c TE_EBUSY) or reported by the agent.
 *                      Configurator rolls back the local changes.
 */
extern te_errno tapi_cfg_pfc_set(const char *ta, const char *if_name,
                                 const tapi_dcb_pfc_config *pfc);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PFC_H__ */

/**@} <!-- END tapi_conf_pfc --> */
