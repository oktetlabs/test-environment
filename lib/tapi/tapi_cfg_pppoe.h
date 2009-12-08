/** @file
 * @brief Test API to configure PPPoE.
 *
 * Definition of API to configure PPPoE.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Igor Labutin <Igor.Labutin@oktetlabs.ru>
 *
 * $Id: tapi_cfg_dhcp.h 32571 2006-10-17 13:43:12Z edward $
 */

#ifndef __TE_TAPI_CFG_PPPOE_H__
#define __TE_TAPI_CFG_PPPOE_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "conf_api.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add interface to PPPoE server configuration on the Test Agent.
 *
 * @param ta            Test Agent
 * @param ifname        Interface to listen on
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_pppoe_server_if_add(const char *ta, const char *ifname);

/**
 * Delete interface from PPPoE server configuration on the Test Agent.
 *
 * @param ta            Test Agent
 * @param ifname        Interface to remove
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_pppoe_server_if_del(const char *ta, const char *ifname);

/**
 * Configure subnet of PPPoE server to allocate local and remote addresses.
 *
 * @param ta            Test Agent
 * @param subnet        String value of subnet XXX.XXX.XXX.XXX/prefix
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_pppoe_server_subnet_set(const char *ta, const char *subnet);

/**
 * Get subnet of PPPoE server to allocate local and remote addresses.
 *
 * @param ta            Test Agent
 * @param subnet_p      Returned pointer to subnet value
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_pppoe_server_subnet_get(const char *ta, const char **subnet_p);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PPPOE_H__ */
