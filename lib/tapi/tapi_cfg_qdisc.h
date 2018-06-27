/** @file
 * @brief Queuing Discipline configuration
 *
 * @defgroup tapi_conf_tc Queuing Discipline configuration
 * @ingroup tapi_conf
 * @{
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_QDISC_H__
#define __TE_TAPI_CFG_QDISC_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Qdisc kind support */
typedef enum tapi_cfg_qdisc_kind_t {
    TAPI_CFG_QDISC_KIND_UNKNOWN,    /**< Unknown qdisc kind */
    TAPI_CFG_QDISC_KIND_NETEM,      /**< NetEm qdisc kind */
} tapi_cfg_qdisc_kind_t;

/**
 * Get status of qdisc
 *
 * @param[in]   ta          Test Agent
 * @param[in]   if_name     Interface Name
 * @param[out]  enabled     Activated or deactivated
 *
 * @return Status Code
 */
extern te_errno tapi_cfg_qdisc_get_enabled(const char *ta,
                                           const char *if_name,
                                           te_bool *enabled);

/**
 * Set status: activated or deactivated
 *
 * @param ta          Test Agent
 * @param if_name     Interface Name
 * @param enabled     Activate or deactivate
 *
 * @return Status Code
 */
extern te_errno tapi_cfg_qdisc_set_enabled(const char *ta,
                                           const char *if_name,
                                           te_bool enabled);

/**
 * Activate qdisc for interface
 *
 * @param ta        Test Agent
 * @param if_name   Interface Name
 *
 * @return Status Code
 */
static inline te_errno
tapi_cfg_qdisc_enable(const char *ta, const char *if_name)
{
    return tapi_cfg_qdisc_set_enabled(ta, if_name, TRUE);
}

/**
 * Deactivate qdisc for interface
 *
 * @param ta        Test Agent
 * @param if_name   Interface Name
 *
 * @return Status Code
 */
static inline te_errno
tapi_cfg_qdisc_disable(const char *ta, const char *if_name)
{
    return tapi_cfg_qdisc_set_enabled(ta, if_name, FALSE);
}

/**
 * Set NetEm kind
 *
 * @param ta        Test Agent
 * @param if_name   Interface Name
 * @param kind      NetEm kind
 *
 * @return Status Code
 */
extern te_errno tapi_cfg_qdisc_set_kind(const char *ta,
                                        const char *if_name,
                                        tapi_cfg_qdisc_kind_t kind);

/**
 * Get NetEm kind
 *
 * @param ta        Test Agent
 * @param if_name   Interface Name
 * @param kind      NetEm kind
 *
 * @return Status Code
 */
extern te_errno tapi_cfg_qdisc_get_kind(const char *ta,
                                        const char *if_name,
                                        tapi_cfg_qdisc_kind_t *kind);

/**
 * Convert qdisc kind enum to string
 *
 * @param kind      Qdisc kind
 *
 * @return kind string
 */
extern const char *tapi_cfg_qdisc_kind2str(tapi_cfg_qdisc_kind_t kind);

/**
 * Convert qdisc kind string to kind enum
 *
 * @param string    Qdisc kind
 *
 * @return kind enum
 */
extern tapi_cfg_qdisc_kind_t tapi_cfg_qdisc_str2kind(const char *string);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_QDISC_H__ */

/**@} <!-- END tapi_conf_qdisc --> */
