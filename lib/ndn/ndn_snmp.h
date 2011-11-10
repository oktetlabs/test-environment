/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for file protocol.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_NDN_SNMP_H__
#define __TE_NDN_SNMP_H__

#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NDN_SNMP_MSG_GET,
    NDN_SNMP_MSG_GETNEXT,
    NDN_SNMP_MSG_RESPONSE,
    NDN_SNMP_MSG_SET,
    NDN_SNMP_MSG_TRAP1,
    NDN_SNMP_MSG_TRAP2,
    NDN_SNMP_MSG_GETBULK,
    NDN_SNMP_MSG_INFORM
} ndn_snmp_msg_t;

typedef enum {
    NDN_SNMP_OBJSYN_INT,
    NDN_SNMP_OBJSYN_STR,
    NDN_SNMP_OBJSYN_OID,
    NDN_SNMP_OBJSYN_IPADDR,
    NDN_SNMP_OBJSYN_COUNTER,
    NDN_SNMP_OBJSYN_TIMETICKS,
    NDN_SNMP_OBJSYN_ARB,
    NDN_SNMP_OBJSYN_BIGCOUNTER,
    NDN_SNMP_OBJSYN_UINT
} ndn_snmp_objsyn_t;

/** SNMPv3 USM Authentication protocol */
typedef enum {
    NDN_SNMP_AUTH_PROTO_DEFAULT,    /**< NET-SNMP default */
    NDN_SNMP_AUTH_PROTO_MD5,        /**< usmHMACMD5AuthProtocol */
    NDN_SNMP_AUTH_PROTO_SHA,        /**< usmHMACSHA1AuthProtocol */
} ndn_snmp_auth_proto_t;

/** SNMPv3 USM Privacy protocol */
typedef enum {
    NDN_SNMP_PRIV_PROTO_DEFAULT,    /**< NET-SNMP default */
    NDN_SNMP_PRIV_PROTO_DES,        /**< usmDESPrivProtocol */
    NDN_SNMP_PRIV_PROTO_AES,        /**< usmAESPrivProtocol */
} ndn_snmp_priv_proto_t;

/** SNMPv3 USM Security level */
typedef enum {
    NDN_SNMP_SEC_LEVEL_NOAUTH,
    NDN_SNMP_SEC_LEVEL_AUTHNOPRIV,
    NDN_SNMP_SEC_LEVEL_AUTHPRIV,
} ndn_snmp_sec_level_t;

/** SNMP Security model */
typedef enum {
    NDN_SNMP_SEC_MODEL_V2C,         /**< Community-based */
    NDN_SNMP_SEC_MODEL_USM,         /**< SNMPv3 User-based */
} ndn_snmp_sec_model_t;

/** Default SNMP security model */
#define NDN_SNMP_SEC_MODEL_DEFAULT NDN_SNMP_SEC_MODEL_V2C

extern asn_type *ndn_snmp_message;
extern asn_type *ndn_snmp_csap;

extern asn_type *ndn_snmp_var_bind;
extern asn_type *ndn_snmp_var_bind_seq;

#if 0
/**
 * Convert NDN SNMP Object syntax tag value
 * (from enum 'ndn_snmp_objsyn_t') int generic SNMP ASN syntax
 */
extern int snmp_asn_syntaxes(unsigned short ndn_snmp_objsyn_tag);
#endif
extern int snmp_asn_syntaxes[];

extern const asn_type * const ndn_snmp_obj_syntax_integer;
extern const asn_type * const ndn_snmp_obj_syntax_string;
extern const asn_type * const ndn_snmp_obj_syntax_objid;
extern const asn_type * const ndn_snmp_obj_syntax_ipaddr;
extern const asn_type * const ndn_snmp_obj_syntax_counter;
extern const asn_type * const ndn_snmp_obj_syntax_timeticks;
extern const asn_type * const ndn_snmp_obj_syntax_arbitrary;
extern const asn_type * const ndn_snmp_obj_syntax_big_counter;
extern const asn_type * const ndn_snmp_obj_syntax_unsigned;

/**
 * Convers NDN SNMP Message types from numeric to string format
 *
 * @param msg_type  Message type in numeric format
 *
 * @return Message type in  string format
 */
extern const char *ndn_snmp_msg_type_h2str(ndn_snmp_msg_t msg_type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_SNMP_H__ */
