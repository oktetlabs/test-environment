/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for DHCP (over IPv4) protocol.
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
#ifndef __TE_NDN_DHCP_H__
#define __TE_NDN_DHCP_H__

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 tags for DHCP CSAP NDN
 */
typedef enum {
    NDN_DHCP_OP,
    NDN_DHCP_HTYPE,
    NDN_DHCP_HLEN,
    NDN_DHCP_HOPS,
    NDN_DHCP_XID,
    NDN_DHCP_SECS,
    NDN_DHCP_FLAGS,
    NDN_DHCP_CIADDR,
    NDN_DHCP_YIADDR,
    NDN_DHCP_SIADDR,
    NDN_DHCP_GIADDR,
    NDN_DHCP_CHADDR,
    NDN_DHCP_SNAME,
    NDN_DHCP_FILE,
    NDN_DHCP_OPTIONS,
    NDN_DHCP_TYPE,
    NDN_DHCP_LENGTH,
    NDN_DHCP_VALUE,
    NDN_DHCP_MODE,
    NDN_DHCP_IFACE,
} ndn_dhcp_tags_t;

/**
 * ASN.1 tags for DHCP6 CSAP NDN
 */
typedef enum {
    NDN_DHCP6_TYPE,
    NDN_DHCP6_LENGTH,
    NDN_DHCP6_TID,
    NDN_DHCP6_HOPCNT,
    NDN_DHCP6_LADDR,
    NDN_DHCP6_PADDR,
    NDN_DHCP6_OPTIONS,
    NDN_DHCP6_MODE,
    NDN_DHCP6_VALUE,
    NDN_DHCP6_IFACE,
    NDN_DHCP6_ENTERPRISE_NUMBER,
    NDN_DHCP6_DUID,
    NDN_DHCP6_DUID_TYPE,
    NDN_DHCP6_DUID_HWTYPE,
    NDN_DHCP6_DUID_LL_ADDR,
    NDN_DHCP6_DUID_IDENTIFIER,
    NDN_DHCP6_IA_NA,
    NDN_DHCP6_IA_TA,
    NDN_DHCP6_IA_ADDR,
    NDN_DHCP6_IAID,
    NDN_DHCP6_TIME,
    NDN_DHCP6_ORO,
    NDN_DHCP6_OPCODE,
    NDN_DHCP6_IP6_ADDR,
    NDN_DHCP6_IP6_PREFIX,
    NDN_DHCP6_RELAY_MESSAGE,
    NDN_DHCP6_AUTH,
    NDN_DHCP6_AUTH_PROTO,
    NDN_DHCP6_AUTH_ALG,
    NDN_DHCP6_AUTH_RDM,
    NDN_DHCP6_AUTH_RELAY_DETECT,
    NDN_DHCP6_AUTH_INFO,
    NDN_DHCP6_SERVADDR,
    NDN_DHCP6_STATUS,
    NDN_DHCP6_STATUS_CODE,
    NDN_DHCP6_STATUS_MESSAGE,
    NDN_DHCP6_USER_CLASS,
    NDN_DHCP6_CLASS_DATA,
    NDN_DHCP6_CLASS_DATA_LEN,
    NDN_DHCP6_CLASS_DATA_OPAQUE,
    NDN_DHCP6_VENDOR_CLASS,
    NDN_DHCP6_VENDOR_CLASS_DATA,
    NDN_DHCP6_VENDOR_SPECIFIC,
    NDN_DHCP6_VENDOR_SPECIFIC_DATA,
    NDN_DHCP6_ELAPSED_TIME,
    NDN_DHCP6_IA_PD,
    NDN_DHCP6_IA_PREFIX,
} ndn_dhcp6_tags_t;

typedef enum {
    DHCP6_MSG_SOLICIT = 1,
    DHCP6_MSG_ADVERTIZE, /*2*/
    DHCP6_MSG_REQUEST, /*3*/
    DHCP6_MSG_CONFIRM, /*4*/
    DHCP6_MSG_RENEW, /*5*/
    DHCP6_MSG_REBIND, /*6*/
    DHCP6_MSG_REPLY, /*7*/
    DHCP6_MSG_RELEASE, /*8*/
    DHCP6_MSG_DECLINE, /*9*/
    DHCP6_MSG_RECONFIGURE, /*10*/
    DHCP6_MSG_INFORMATION_REQUEST, /*11*/
    DHCP6_MSG_RELAY_FORW, /*12*/
    DHCP6_MSG_RELAY_REPL, /*13*/
} ndn_dhcp6_msg_type;

typedef enum {
    DHCP6_OPT_CLIENTID = 1,
    DHCP6_OPT_SERVERID = 2,
    DHCP6_OPT_IA_NA = 3,
    DHCP6_OPT_IA_TA = 4,
    DHCP6_OPT_IAADDR = 5,
    DHCP6_OPT_ORO = 6,
    DHCP6_OPT_PREFERENCE = 7,
    DHCP6_OPT_ELAPSED_TIME = 8,
    DHCP6_OPT_RELAY_MSG = 9,
    /* Option type 10 skipped */
    DHCP6_OPT_AUTH = 11,
    DHCP6_OPT_UNICAST = 12,
    DHCP6_OPT_STATUS_CODE = 13,
    DHCP6_OPT_RAPID_COMMIT = 14,
    DHCP6_OPT_USER_CLASS = 15,
    DHCP6_OPT_VENDOR_CLASS = 16,
    DHCP6_OPT_VENDOR_OPTS = 17,
    DHCP6_OPT_INTERFACE_ID = 18,
    DHCP6_OPT_RECONF_MSG = 19,
    DHCP6_OPT_RECONF_ACCEPT = 20,
    /* Options 21, 22? */
    DHCP6_OPT_DNS_RECURSIVE = 23,
    DHCP6_OPT_DOMAIN_SEARCH_LIST = 24,
    DHCP6_OPT_IA_PD = 25,
    DHCP6_OPT_IA_PREFIX =26,
    /* Options 27 - 30? */
    DHCP6_OPT_SNTP_SERVER = 31,
} ndn_dhcp_opt_type;

typedef enum {
    DUID_LLT = 1,
    DUID_LL = 2,
    DUID_EN = 3,
} ndn_dhcp_duid_type;

extern asn_type *ndn_dhcpv4_message;
extern asn_type *ndn_dhcpv4_options;
extern asn_type *ndn_dhcpv4_option;
extern asn_type *ndn_dhcpv4_end_pad_option;
extern asn_type *ndn_dhcpv4_csap;

extern asn_type *ndn_dhcpv6_duid;
extern asn_type *ndn_dhcpv6_ia_na;
extern asn_type *ndn_dhcpv6_ia_ta;
extern asn_type *ndn_dhcpv6_ia_addr;
extern asn_type *ndn_dhcpv6_ia_pd;
extern asn_type *ndn_dhcpv6_ia_prefix;
extern asn_type *ndn_dhcpv6_opcode;
extern asn_type *ndn_dhcpv6_oro;
extern asn_type *ndn_dhcpv6_auth;
extern asn_type *ndn_dhcpv6_status;
extern asn_type *ndn_dhcpv6_class_data_list;
extern asn_type *ndn_dhcpv6_class_data;
extern asn_type *ndn_dhcpv6_vendor_class;
extern asn_type *ndn_dhcpv6_vendor_specific;
extern asn_type *ndn_dhcpv6_message;
extern asn_type *ndn_dhcpv6_options;
extern asn_type *ndn_dhcpv6_option;
extern asn_type *ndn_dhcpv6_csap;

typedef enum dhcp_csap_mode {
    DHCP4_CSAP_MODE_SERVER = 1,
    DHCP4_CSAP_MODE_CLIENT = 2,
} dhcp_csap_mode;

typedef enum dhcp6_csap_mode {
    DHCP6_CSAP_MODE_SERVER = 1,
    DHCP6_CSAP_MODE_CLIENT = 2,
} dhcp6_csap_mode;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_DHCP_H__ */
