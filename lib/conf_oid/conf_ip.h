/** @file
 * @brief Type for ip rule and ip route
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
 * $Id$
 */

#ifndef __TE_LIB_CONF_OID_CONF_IP_H_
#define __TE_LIB_CONF_OID_CONF_IP_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Routing types */
typedef enum te_ip_type {
    TE_IP_TYPE_UNSPEC,
    TE_IP_TYPE_UNICAST,
    TE_IP_TYPE_LOCAL,
    TE_IP_TYPE_BROADCAST,
    TE_IP_TYPE_ANYCAST,
    TE_IP_TYPE_MULTICAST,
    TE_IP_TYPE_BLACKHOLE,
    TE_IP_TYPE_UNREACHABLE,
    TE_IP_TYPE_PROHIBIT,
    TE_IP_TYPE_THROW,
    TE_IP_TYPE_NAT
} te_ip_type;

/** Routing table IDs */
typedef enum te_ip_table_id_t {
    TE_IP_TABLE_ID_UNSPEC     = 0,
    TE_IP_TABLE_ID_DEFAULT    = 253,
    TE_IP_TABLE_ID_MAIN       = 254,
    TE_IP_TABLE_ID_LOCAL      = 255
} te_ip_table_id;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_LIB_CONF_OID_CONF_IP_H_ */
