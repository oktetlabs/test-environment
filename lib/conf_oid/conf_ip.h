/** @file
 * @brief Type for ip rule and ip route
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
