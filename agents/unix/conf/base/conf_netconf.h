/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef __TE_AGENTS_UNIX_CONF_BASE_CONF_NETCONF_H_
#define __TE_AGENTS_UNIX_CONF_BASE_CONF_NETCONF_H_

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_LIBNETCONF
#include <netconf.h>
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
