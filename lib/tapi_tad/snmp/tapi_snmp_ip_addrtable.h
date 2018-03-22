/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * SNMP protocol implementaion internal declarations.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Anastasia Kazakova <Anastasia.Kazakova@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE__TAPI_SNMP_IP_ADDRTABLE_H__
#define __TE__TAPI_SNMP_IP_ADDRTABLE_H__ 

/** Rows in an instance of an ipAddrTable */
typedef struct tapi_snmp_ip_addrtable_row_t {
    tapi_snmp_oid_t *index_suffix;   /**< index_suffix */

    struct in_addr   *ipAdEntAddr;                /**< ipAddress,*/
    int              *ipAdEntIfIndex;             /**< INTEGER,*/
    struct in_addr   *ipAdEntNetMask;             /**< ipAddress,*/
    int              *ipAdEntBcastAddr;           /**< INTEGER,*/
    int              *ipAdEntReasmMaxSize         /**< INTEGER */

} tapi_snmp_ip_addrtable_row_t;


#endif /* __TE__TAPI_SNMP_IP_ADDRTABLE_H__ */
