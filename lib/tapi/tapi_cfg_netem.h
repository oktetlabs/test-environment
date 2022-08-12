/** @file
 * @brief Network Emulator configuration
 *
 * @defgroup tapi_conf_netem Network Emulator configuration
 * @ingroup tapi_conf
 * @{
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_NETEM_H__
#define __TE_TAPI_CFG_NETEM_H__

#include "te_defs.h"
#include "te_errno.h"

#define TAPI_CFG_NETEM_MAX_PARAM_LEN    (64)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get value of NetEm parameter as string
 * @note value should be free() after use
 *
 * @param[in]  ta       Test Agent
 * @param[in]  if_name  Interface name
 * @param[in]  param    NetEm parameter name
 * @param[out] value    Parameter value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_param(
    const char *ta, const char *if_name, const char *param, char **value);

/**
 * Set value of NetEm parameter as string
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param param     NetEm parameter name
 * @param value     Parameter value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_param(
    const char *ta, const char *if_name, const char *param,
    const char *value);

/**
 * Set packet delay on interface
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param delay_us  Delay value in usec
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_delay(
    const char *ta, const char *if_name, uint32_t delay_us);

/**
 * Get packet delay on interface
 *
 * @param[in]  ta       Test Agent
 * @param[in]  if_name  Interface name
 * @param[out] delay_us Delay value in usec
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_delay(
    const char *ta, const char *if_name, uint32_t *delay_us);

/**
 * Set packet jitter on interface
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param jitter_us    Jitter value in usec
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_jitter(
    const char *ta, const char *if_name, uint32_t jitter_us);

/**
 * Get packet jitter on interface
 *
 * @param[in]  ta           Test Agent
 * @param[in]  if_name      Interface name
 * @param[out] jitter_us    Jitter value in usec
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_jitter(
    const char *ta, const char *if_name, uint32_t *jitter_us);

/**
 * Set packet delay correlation on interface
 *
 * @param ta                Test Agent
 * @param if_name           Interface name
 * @param delay_correlation Delay correlation value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_delay_correlation(
    const char *ta, const char *if_name, double delay_correlation);

/**
 * Get packet delay correlation on interface
 *
 * @param[in]  ta                   Test Agent
 * @param[in]  if_name              Interface name
 * @param[out] delay_correlation    Delay correlation value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_delay_correlation(
    const char *ta, const char *if_name, double *delay_correlation);

/**
 * Set packet loss on interface
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param loss      Loss probability [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_loss(
    const char *ta, const char *if_name, double loss);

/**
 * Get packet loss on interface
 *
 * @param[in]  ta        Test Agent
 * @param[in]  if_name   Interface name
 * @param[out] loss Loss probability [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_loss(
    const char *ta, const char *if_name, double *loss);

/**
 * Set packet loss correlation on interface
 *
 * @param ta                Test Agent
 * @param if_name           Interface name
 * @param loss_correlation  Loss correlation [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_loss_correlation(
    const char *ta, const char *if_name, double loss_correlation);

/**
 * Get packet loss correlation on interface
 *
 * @param[in]  ta               Test Agent
 * @param[in]  if_name          Interface name
 * @param[out] loss_correlation Loss correlation [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_loss_correlation(
    const char *ta, const char *if_name, double *loss_correlation);

/**
 * Set packet duplicate on interface
 *
 * @param ta           Test Agent
 * @param if_name      Interface name
 * @param duplicate    Duplicate probability [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_duplicate(
    const char *ta, const char *if_name, double duplicate);

/**
 * Get packet duplicate on interface
 *
 * @param[in]  ta            Test Agent
 * @param[in]  if_name       Interface name
 * @param[out] duplicate    Duplicate probability [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_duplicate(
    const char *ta, const char *if_name, double *duplicate);

/**
 * Set packet duplicate correlation on interface
 *
 * @param ta                    Test Agent
 * @param if_name               Interface name
 * @param duplicate_correlation Duplicate correlation [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_duplicate_correlation(
    const char *ta, const char *if_name, double duplicate_correlation);

/**
 * Get packet duplicate correlation on interface
 *
 * @param[in]  ta                    Test Agent
 * @param[in]  if_name               Interface name
 * @param[out] duplicate_correlation Duplicate correlation [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_duplicate_correclation(
    const char *ta, const char *if_name, double *duplicate_correlation);

/**
 * Set packet limit on interface
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param limit     Limit value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_limit(
    const char *ta, const char *if_name, uint32_t limit);

/**
 * Get packet limit on interface
 *
 * @param[in]  ta       Test Agent
 * @param[in]  if_name  Interface name
 * @param[out] limit    Limit value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_limit(
    const char *ta, const char *if_name, uint32_t *limit);

/**
 * Set packet gap on interface
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param gap       Gap value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_gap(
    const char *ta, const char *if_name, uint32_t gap);

/**
 * Get packet gap on interface
 *
 * @param[in]  ta       Test Agent
 * @param[in]  if_name  Interface name
 * @param[out] gap      Gap value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_gap(
    const char *ta, const char *if_name, uint32_t *gap);

/**
 * Set packet reorder on interface
 *
 * @param ta                    Test Agent
 * @param if_name               Interface name
 * @param reorder_probability   Reorder probability value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_reorder_probability(
    const char *ta, const char *if_name, double reorder_probability);

/**
 * Get packet reorder on interface
 *
 * @param[in]  ta                    Test Agent
 * @param[in]  if_name               Interface name
 * @param[out] reorder_probability   Reorder probability value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_reorder_probability(
    const char *ta, const char *if_name, double *reorder_probability);

/**
 * Set packet reorder correlation on interface
 *
 * @param ta                    Test Agent
 * @param if_name               Interface name
 * @param reorder_correlation   Reorder correlation value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_reorder_correlation(
    const char *ta, const char *if_name, double reorder_correlation);

/**
 * Get packet reorder correlation on interface
 *
 * @param[in]  ta                   Test Agent
 * @param[in]  if_name              Interface name
 * @param[out] reorder_correlation  Reorder probability value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_reorder_correlation(
    const char *ta, const char *if_name, double *reorder_correlation);

/**
 * Set packet corruption probability on interface
 *
 * @param ta                      Test Agent
 * @param if_name                 Interface name
 * @param corruption_probability  Corruption probability value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_corruption_probability(
    const char *ta, const char *if_name, double corruption_probability);

/**
 * Get packet corruption probability on interface
 *
 * @param[in]  ta                      Test Agent
 * @param[in]  if_name                 Interface name
 * @param[out] corruption_probability  Corruption probability value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_corruption_probability(
    const char *ta, const char *if_name, double *corruption_probability);

/**
 * Set packet corruption correlation on interface
 *
 * @param ta                       Test Agent
 * @param if_name                  Interface name
 * @param corruption_correlation   Corruption correlation value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_set_corruption_correlation(
    const char *ta, const char *if_name, double corruption_correlation);

/**
 * Get packet corruption correlation on interface
 *
 * @param[in]  ta                       Test Agent
 * @param[in]  if_name                  Interface name
 * @param[out] corruption_correlation   Corruption correlation value [0-100]%
 *
 * @return Status code
 */
extern te_errno tapi_cfg_netem_get_corruption_correlation(
    const char *ta, const char *if_name, double *corruption_correlation);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_NETEM_H__ */

/**@} <!-- END tapi_conf_netem --> */
