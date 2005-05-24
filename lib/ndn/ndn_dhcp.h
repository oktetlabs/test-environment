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

extern asn_type_p ndn_dhcpv4_message;
extern asn_type_p ndn_dhcpv4_options;
extern asn_type_p ndn_dhcpv4_option;
extern asn_type_p ndn_dhcpv4_csap;

typedef enum dhcp_csap_mode {
    DHCP4_CSAP_MODE_SERVER = 1,
    DHCP4_CSAP_MODE_CLIENT = 2,
} dhcp_csap_mode;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_DHCP_H__ */
