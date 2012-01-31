/** @file
 * @brief Test API for TAD. IPv6 CSAPs
 *
 * Implementation of Test API
 *
 * Copyright (C) 2012 Test Environment authors (see file AUTHORS in the
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
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_IP6_H__
#define __TE_TAPI_IP6_H__

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tapi_ip6_packet_t {
    uint8_t     src_addr[16];
    uint8_t     dst_addr[16];
    uint16_t    data_len;
    uint8_t     next_header;
    uint8_t     hop_limit;
    uint32_t    flow_label;
    /*
     * Field 'priority' if necessary
     */
    uint8_t    *payload;
    size_t      pld_len;
} tapi_ip6_packet_t;

typedef void (*ip6_callback)(const tapi_ip6_packet_t *pkt, void *userdata);

extern tapi_tad_trrecv_cb_data *tapi_ip6_eth_trrecv_cb_data(
                                                    ip6_callback  callback,
                                                    void         *user_data);

/**
 * Add IPv6 layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param local_addr    Default local IPv6 address
 * @param remote_addr   Default remote IPv6 address
 * @param next_header   Protocol or negative to keep unspecified.
 *
 * @retval Status code.
 */
extern te_errno tapi_ip6_add_csap_layer(asn_value **csap_spec,
                                        const uint8_t *local_addr,
                                        const uint8_t *remote_addr,
                                        int next_header);

/**
 * Create 'ip6.eth' CSAP
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Receive mode for Ethernet Layer on the Interface
 * @param loc_mac_addr  Local MAC address (or NULL)
 * @param rem_mac_addr  Remote MAC address (or NULL)
 * @param loc_ip6_addr  Local IPv6 address
 * @param rem_ip6_addr  Remote IPv6 address
 * @param next_header   Protocol over IPv6 or negative number
 * @param ip6_csap      Location for the IPv6 CSAP handle (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_ip6_eth_csap_create(const char *ta_name, int sid,
                                         const char *eth_dev,
                                         unsigned int receive_mode,
                                         const uint8_t *loc_mac_addr,
                                         const uint8_t *rem_mac_addr,
                                         const uint8_t *loc_ip6_addr,
                                         const uint8_t *rem_ip6_addr,
                                         int next_header,
                                         csap_handle_t *ip6_csap);

/**
 * Add IPv6 PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param src_addr      Source IPv6 address
 * @param dst_addr      Destination IPv6 address
 * @param next_header   Protocol or negative to keep unspecified.
 * @param hop_limit     Time-to-live or negative to keep unspecified.
 *
 * @return Status code.
 */
extern te_errno tapi_ip6_add_pdu(asn_value **tmpl_or_ptrn, asn_value **pdu,
                                 te_bool is_pattern,
                                 const uint8_t *src_addr,
                                 const uint8_t *dst_addr,
                                 int next_header, int hop_limit);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_IP6_H__ */
