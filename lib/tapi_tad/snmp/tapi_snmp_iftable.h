/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment:
 *
 * Traffic Application Domain Command Handler
 * SNMP protocol implementaion internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE__TAPI_SNMP_IFTABLE_H__
#define __TE__TAPI_SNMP_IFTABLE_H__

/** Rows in an instance of an ifTable */
typedef struct tapi_snmp_if_table_row_t {
    tapi_snmp_oid_t *index_suffix;   /**< index_suffix */
/** First octet of object identifier */
#define ifTable_ifIndex index_suffix->id[0]

    int     *ifIndex;                /**< InterfaceIndex,*/
    tapi_snmp_oct_string_t *ifDescr; /**< DisplayString,*/
    int     *ifType;                 /**< IANAifType,*/
    int     *ifMtu;                  /**< Integer32,*/
    int     *ifSpeed;                /**< Gauge32,*/
    tapi_snmp_oct_string_t *ifPhysAddress;       /**< PhysAddress,*/
    int     *ifAdminStatus;          /**< INTEGER,*/
    int     *ifOperStatus;           /**< INTEGER,*/
    int     *ifLastChange;           /**< TimeTicks,*/
    int     *ifInOctets;             /**< Counter32,*/
    int     *ifInUcastPkts;          /**< Counter32,*/
    int     *ifInNUcastPkts;         /**< Counter32,  -- deprecated*/
    int     *ifInDiscards;           /**< Counter32,*/
    int     *ifInErrors;             /**< Counter32,*/
    int     *ifInUnknownProtos;      /**< Counter32,*/
    int     *ifOutOctets;            /**< Counter32,*/
    int     *ifOutUcastPkts;         /**< Counter32,*/
    int     *ifOutNUcastPkts;        /**< Counter32,  -- deprecated*/
    int     *ifOutDiscards;          /**< Counter32,*/
    int     *ifOutErrors;            /**< Counter32,*/
    int     *ifOutQLen;              /**< Gauge32,    -- deprecated*/
    tapi_snmp_oid_t *ifSpecific;     /**<  OBJECT IDENTIFIER -- deprecated*/

} tapi_snmp_if_table_row_t;


#endif /* __TE__TAPI_SNMP_IFTABLE_H__ */
