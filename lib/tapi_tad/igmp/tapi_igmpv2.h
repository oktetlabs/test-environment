/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/ or
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef __TE_TAPI_IGMPV2_H__
#define __TE_TAPI_IGMPV2_H__


#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "tapi_tad.h"
#include "tapi_ip4.h"
#include "tapi_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add IGMPv2 layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 *
 * @retval Status code.
 */
extern te_errno tapi_igmpv2_add_csap_layer(asn_value **csap_spec);

/**
 * Add IGMPv2 PDU as the last PDU to the last unit of the traffic 
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param type          Type of IGMPv2 message or negative to keep
 *                      unspecified.
 * @param max_resp_time IGMP message maximum response   time,
 *                      or negative to keep unspecified.
 *
 * @return              Status code.
 */
extern te_errno tapi_igmpv2_add_pdu(asn_value         **tmpl_or_ptrn,
                                    asn_value         **pdu,
                                    te_bool             is_pattern,
                                    int                 type,
                                    int                 max_resp_time,
                                    in_addr_t           group_addr);


/**
 * Create 'igmpv2.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param eth_src       Local MAC address (or NULL)
 * @param src_addr      Local IP address in network byte order (or NULL)
 * @param icmp_csap     Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_igmpv2_ip4_eth_csap_create(
                    const char    *ta_name,
                    int            sid,
                    const char    *eth_dev,
                    unsigned int   receive_mode,
                    const uint8_t *eth_src,
                    in_addr_t      src_addr,
                    csap_handle_t *igmp_csap);

/**
 * Compose IGMPv2.IPv4.Eth PDU as the last PDU to the last unit
 * of the traffic template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param type          Type of IGMPv2 message
 * @param max_resp_time IGMP message maximum response time,
 *                      or negative to keep unspecified.
 * @param group_addr    Group Address field of IGMPv2 message
 * @param src_addr      IPv4 layer Source Address field or INADDR_ANY to
 *                      keep unspecified.
 * @param eth_src       Ethernet layer Source Address field or NULL to keep
 *                      unspecified.
 *
 * @note  Destination addresses for IPv4 and Ethernet layers are calculated
 *        from @p group_addr and @p type values.
 *
 * @return              Status code.
 */
extern te_errno tapi_igmpv2_ip4_eth_add_pdu(asn_value **tmpl_or_ptrn,
                                            asn_value **pdu,
                                            te_bool     is_pattern,
                                            int         type,
                                            int         max_resp_time,
                                            in_addr_t   group_addr,
                                            in_addr_t   src_addr,
                                            uint8_t    *eth_src);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_IGMPV2_H__ */
