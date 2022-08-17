/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support Traffic Control
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_AGENTS_UNIX_CONF_TC_INTERNAL_H_
#define __TE_AGENTS_UNIX_CONF_TC_INTERNAL_H_

#include "te_errno.h"

#include <netlink/netlink.h>

/**
 * Initialization of libnl socket, caches, qdisc
 *
 * @return Status code
 */
extern te_errno conf_tc_internal_init(void);

/**
 * Clean up socket, caches, qdisc
 */
extern void conf_tc_internal_fini(void);

/**
 * Get initialized libnl socket
 *
 * @return initialized libnl socket
 */
extern struct nl_sock *conf_tc_internal_get_sock(void);

/**
 * Get qdisc binded with interface name
 * @note must be given back with rtnl_qdisc_put() after usage
 *
 * @param if_name   interface name
 * @return qdisc context
 */
extern struct rtnl_qdisc *conf_tc_internal_try_get_qdisc(const char *if_name);

/**
 * Get qdisc binded with interface name if exist, otherwise create new
 * @note no back with rtnl_qdisc_put() after usage
 *
 * @param if_name   interface name
 * @return qdisc context
 */
extern struct rtnl_qdisc *conf_tc_internal_get_qdisc(const char *if_name);

/**
 * Convert libnl error to TE error
 * @note in case unknown error - function print WARN with interpretation
 * of libnl error and return TE_EUNKNOWN
 *
 * @param nl_error  libnl error
 * @return TE error
 * @retval TE_EUNKNOWN if conversation no success
 */
extern te_errno conf_tc_internal_nl_error2te_errno(int nl_error);

/**
 * Enable qdisc for interface
 *
 * @param if_name   interface name
 * @return TE error
 */
extern te_errno conf_tc_internal_qdisc_enable(const char *if_name);

/**
 * Disable qdisc for interface
 *
 * @param if_name   interface name
 * @return TE error
 */
extern te_errno conf_tc_internal_qdisc_disable(const char *if_name);

#endif /* __TE_AGENTS_UNIX_CONF_TC_INTERNAL_H_ */
