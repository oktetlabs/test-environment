/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_AGENTS_UNIX_CONF_BASE_CONF_NETCONF_H_
#define __TE_AGENTS_UNIX_CONF_BASE_CONF_NETCONF_H_

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_LIBNETCONF
#include "netconf.h"
#endif

#if ((!defined(__linux__)) && (defined(USE_LIBNETCONF)))
#error "netlink can be used on Linux only"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_LIBNETCONF
extern netconf_handle nh;
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_AGENTS_UNIX_CONF_BASE_CONF_NETCONF_H_ */
