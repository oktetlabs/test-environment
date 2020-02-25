/** @file
 * @brief tc qdisc TBF configuration
 *
 * @defgroup tapi_conf_tbf tc qdisc TBF configuration
 * @ingroup tapi_conf_qdisc
 * @{
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Roman Zhukov <Roman.Zhukov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_TBF_H__
#define __TE_TAPI_CFG_TBF_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get rate of TBF qdisc on interface
 *
 * @param[in]  ta           Test Agent
 * @param[in]  if_name      Interface name
 * @param[out] rate         Rate value in bytes per second
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_get_rate(
    const char *ta, const char *if_name, uint32_t *rate);

/**
 * Set rate of TBF qdisc on interface
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param rate         Rate value in bytes per second
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_set_rate(
    const char *ta, const char *if_name, uint32_t rate);

/**
 * Get size of the TBF qdisc bucket on interface
 *
 * @param[in]  ta           Test Agent
 * @param[in]  if_name      Interface name
 * @param[out] bucket       Size of the bucket in bytes
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_get_bucket(
    const char *ta, const char *if_name, uint32_t *bucket);

/**
 * Set size of the TBF qdisc bucket on interface
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param bucket       Size of the bucket in bytes
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_set_bucket(
    const char *ta, const char *if_name, uint32_t bucket);

/**
 * Get size of a rate cell
 *
 * @param[in]  ta           Test Agent
 * @param[in]  if_name      Interface name
 * @param[out] cell         Size of a rate cell
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_get_cell(
    const char *ta, const char *if_name, uint32_t *cell);

/**
 * Set size of a rate cell
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param cell         Size of a rate cell
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_set_cell(
    const char *ta, const char *if_name, uint32_t cell);

/**
 * Get limit of TBF qdisc on interface
 *
 * @param[in]  ta           Test Agent
 * @param[in]  if_name      Interface name
 * @param[out] limit        Limit in bytes
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_get_limit(
    const char *ta, const char *if_name, uint32_t *limit);

/**
 * Set limit of TBF qdisc on interface
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param limit        Limit in bytes
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_set_limit(
    const char *ta, const char *if_name, uint32_t limit);

/**
 * Get limit of TBF qdisc by latency on interface
 *
 * @param[in]  ta           Test Agent
 * @param[in]  if_name      Interface name
 * @param[out] latency_us   Limit by latency in micro seconds
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_get_latency(
    const char *ta, const char *if_name, uint32_t *latency_us);

/**
 * Set limit of TBF qdisc by latency on interface
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param latency_us   Limit by latency in micro seconds
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_set_latency(
    const char *ta, const char *if_name, uint32_t latency_us);

/**
 * Get peakrate of TBF qdisc on interface
 *
 * @param[in]  ta           Test Agent
 * @param[in]  if_name      Interface name
 * @param[out] peakrate     Peakrate value in bytes per second
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_get_peakrate(
    const char *ta, const char *if_name, uint32_t *peakrate);

/**
 * Set peakrate of TBF qdisc on interface
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param peakrate     Peakrate value in bytes per second
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_set_peakrate(
    const char *ta, const char *if_name, uint32_t peakrate);

/**
 * Get size of the TBF qdisc mtu on interface
 *
 * @param[in]  ta           Test Agent
 * @param[in]  if_name      Interface name
 * @param[out] mtu          Size of the mtu in bytes
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_get_mtu(
    const char *ta, const char *if_name, uint32_t *mtu);

/**
 * Set size of the TBF qdisc mtu on interface
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param mtu          Size of the mtu in bytes
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tbf_set_mtu(
    const char *ta, const char *if_name, uint32_t mtu);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_TBF_H__ */

/**@} <!-- END tapi_conf_tbf --> */
