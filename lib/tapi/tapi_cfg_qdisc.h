/** @file
 * @brief Queuing Discipline configuration
 *
 * @defgroup tapi_conf_qdisc Queuing Discipline configuration
 * @ingroup tapi_conf
 * @{
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 *
 * @section tapi_cfg_qdisc_example Example of usage
 *
 * Usage of TAPI for control NetEm
 *
 * At first add ./cs.conf.inc.qdisc from ts_conf in your cs.conf
 *
 * Setup Agent Agt_A analog of command line:
 *
 * @cmd{tc qdisc add dev eth1 root netem delay 200ms}
 * @code
 * #include "tapi_cfg_netem.h"
 * #include "tapi_cfg_qdisc.h"
 * ...
 * const char *agent = "Agt_A";
 * const char *ifname = "eth1";
 *
 * CHECK_RC(tapi_cfg_qdisc_set_kind(agent, ifname, TAPI_CFG_QDISC_KIND_NETEM));
 * CHECK_RC(tapi_cfg_netem_set_delay(agent, ifname, TE_MS2US(200)));
 * CHECK_RC(tapi_cfg_qdisc_enable(agent, ifname));
 * @endcode
 *
 * @cmd{tc qdisc change dev eth1 root netem delay 100ms 10ms 25%}
 * @code
 * CHECK_RC(tapi_cfg_qdisc_set_kind(agent, ifname, TAPI_CFG_QDISC_KIND_NETEM));
 * CHECK_RC(tapi_cfg_netem_set_delay(agent, ifname, TE_MS2US(100)));
 * CHECK_RC(tapi_cfg_netem_set_jitter(agent, ifname, TE_MS2US(10)));
 * CHECK_RC(tapi_cfg_netem_set_delay_correlation(agent, ifname, 25.0));
 * CHECK_RC(tapi_cfg_qdisc_enable(agent, ifname));
 * @endcode
 *
 * @cmd{tc qdisc add dev eth1 root netem loss 10%}
 * @code
 * CHECK_RC(tapi_cfg_qdisc_set_kind(agent, ifname, TAPI_CFG_QDISC_KIND_NETEM));
 * CHECK_RC(tapi_cfg_netem_set_loss(agent, ifname, TE_MS2US(100)));
 * CHECK_RC(tapi_cfg_qdisc_enable(agent, ifname));
 * @endcode
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
    TAPI_CFG_QDISC_KIND_TBF,        /**< TBF qdisc kind */
    TAPI_CFG_QDISC_KIND_CLSACT,     /**< clsact qdisc */
} tapi_cfg_qdisc_kind_t;

#define TAPI_CFG_QDISC_PARAM_FMT \
    "/agent:%s/interface:%s/tc:/qdisc:/param:%s"

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

/**
 * Get value of qdisc parameter as string
 * @note value should be free() after use
 *
 * @param[in]  ta       Test Agent
 * @param[in]  if_name  Interface name
 * @param[in]  param    qdisc parameter name
 * @param[out] value    Parameter value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_qdisc_get_param(
    const char *ta, const char *if_name, const char *param, char **value);

/**
 * Set value of qdisc parameter as string
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param param     qdisc parameter name
 * @param value     Parameter value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_qdisc_set_param(
    const char *ta, const char *if_name, const char *param,
    const char *value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_QDISC_H__ */

/**@} <!-- END tapi_conf_qdisc --> */
