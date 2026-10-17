/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Enhanced Transmission Selection configuration
 *
 * @defgroup tapi_conf_ets Enhanced Transmission Selection configuration
 * @ingroup tapi_conf
 * @{
 *
 * @section tapi_cfg_ets_example Example of usage
 *
 * Retrieve ETS configuration on Test Agent @c Agt_A, similarly to:
 *
 * @cmd{dcb ets show dev eth1}
 * @code
 * #include "tapi_cfg_ets.h"
 *
 * const char *ta = "Agt_A";
 * const char *if_name = "eth1";
 * tapi_dcb_ets_state ets;
 *
 * CHECK_RC(tapi_cfg_ets_get(ta, if_name, &ets));
 * @endcode
 *
 * Enable the ETS willing state on Test Agent @c Agt_A, similarly to:
 *
 * @cmd{dcb ets set dev eth1 willing on}
 * @code
 * #include "tapi_cfg_ets.h"
 *
 * const char *ta = "Agt_A";
 * const char *if_name = "eth1";
 * tapi_dcb_ets_state ets;
 *
 * CHECK_RC(tapi_cfg_ets_get(ta, if_name, &ets));
 * ets.config.willing = true;
 * CHECK_RC(tapi_cfg_ets_set(ta, if_name, &ets.config));
 * @endcode
 */

#ifndef __TE_TAPI_CFG_ETS_H__
#define __TE_TAPI_CFG_ETS_H__

#include "tapi_dcb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Configuration OID format for an ETS interface instance. */
#define TAPI_CFG_ETS_FMT \
    "/agent:%s/interface:%s/dcb:/ets:"

/** Configuration OID format for an ETS scalar parameter. */
#define TAPI_CFG_ETS_PARAM_FMT \
    TAPI_CFG_ETS_FMT "/%s:"

/** Configuration OID format for an indexed ETS parameter. */
#define TAPI_CFG_ETS_PARAM_ID_FMT \
    TAPI_CFG_ETS_PARAM_FMT "%u"

/**
 * Check whether an interface has the ETS instance.
 *
 * The instance exists if the interface supports DCB and its driver
 * provides ETS data. The DCB subtree of the interface is synchronized
 * first.
 *
 * Use it to decide whether a test can run: once the instance is known to
 * exist, any error of the other ETS functions is a failure,
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
extern te_errno tapi_cfg_ets_is_present(const char *ta,
                                        const char *if_name,
                                        bool *present);

/**
 * Get the complete ETS configuration.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[out] ets      Retrieved ETS state.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval TE_ENOENT    There is no ETS instance: the interface does not
 *                      support DCB, its driver provides no ETS data (the
 *                      getter is absent or fails), or the interface is
 *                      gone.
 * @retval TE_EINVAL    An argument is @c NULL.
 * @retval Other        Error reported by the agent while reading the
 *                      configuration (e.g. @c TE_ENOTCONN, @c TE_EPROTO,
 *                      or an error returned by the kernel).
 */
extern te_errno tapi_cfg_ets_get(const char *ta, const char *if_name,
                                 tapi_dcb_ets_state *ets);

/**
 * Apply the ETS configuration using a single Configurator commit.
 *
 * @param[in] ta        Test Agent.
 * @param[in] if_name   Interface name.
 * @param[in] ets       ETS configuration.
 *
 * @return Status code.
 * @retval 0            The kernel accepted the request. This does not prove
 *                      that the driver applied it: the kernel reports
 *                      success if the driver has no setter, and the driver
 *                      may adjust the values. Read the configuration back
 *                      to check what is applied.
 * @retval TE_ENOENT    There is no ETS instance (see tapi_cfg_ets_get()).
 * @retval TE_EINVAL    An argument is @c NULL, or the driver rejected the
 *                      configuration with @c EINVAL.
 * @retval Other        Error returned by the driver (e.g. @c TE_EOPNOTSUPP,
 *                      @c TE_ERANGE, @c TE_EBUSY) or reported by the agent.
 *                      Configurator rolls back the local changes.
 */
extern te_errno tapi_cfg_ets_set(const char *ta, const char *if_name,
                                 const tapi_dcb_ets_config *ets);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_CFG_ETS_H__ */

/**@} <!-- END tapi_conf_ets --> */
