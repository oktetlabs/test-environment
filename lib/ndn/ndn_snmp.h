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

#include "config.h"

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
    NDN_SNMP_MSG_GETBULK
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

extern asn_type_p ndn_snmp_message;
extern asn_type_p ndn_snmp_csap;

extern asn_type_p ndn_snmp_var_bind;
extern asn_type_p ndn_snmp_var_bind_seq;

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
