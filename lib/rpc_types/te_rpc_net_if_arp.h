/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from net/if_arp.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
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
