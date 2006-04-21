/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from net/if_arp.h.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_NET_IF_ARP_H__
#define __TE_RPC_NET_IF_ARP_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/* arpreq flags */
typedef enum rpc_arp_flags {
    RPC_ATF_COM = 0x0001,         /**< Lookup complete */
    RPC_ATF_PERM = 0x0002,        /**< Permanent entry */
    RPC_ATF_PUBL = 0x0004,        /**< Publish entry */
    RPC_ATF_NETMASK = 0x0008,     /**< Use a netmask */
    RPC_ATF_DONTPUB = 0x0010      /**< Don't answer */
} rpc_arp_fl;    

#define RPC_ARP_FLAGS_ALL \
    (RPC_ATF_COM | RPC_ATF_PERM | RPC_ATF_PUBL |  \
     RPC_ATF_NETMASK | RPC_ATF_DONTPUB)

/** List of mapping numerical value to string for 'rpc_io_fl' */
#define ARP_FL_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(ATF_COM), \
    RPC_BIT_MAP_ENTRY(ATF_PERM), \
    RPC_BIT_MAP_ENTRY(ATF_PUBL), \
    RPC_BIT_MAP_ENTRY(ATF_NETMASK), \
    RPC_BIT_MAP_ENTRY(ATF_DONTPUB)


extern unsigned int arp_fl_rpc2h(unsigned int flags);

extern unsigned int arp_fl_h2rpc(unsigned int flags);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_NET_IF_ARP_H__ */
