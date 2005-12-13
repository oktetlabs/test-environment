/** @file
 * @brief Proteos, TAPI Ethernet.
 *
 * Declarations of API for TAPI Ethernet.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in the
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_ETH_H__
#define __TE_TAPI_ETH_H__

#include <stdio.h>
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "tapi_tad.h"

#include "tapi_test.h"


/**
 * Create ASN.1 value for Ethernet CSAP layer.
 *
 * @param device        Interface name on TA host
 * @param recv_mode     Pointer to receive mode or NULL, bit scale defined
 *                      by elements of 'enum eth_csap_receive_mode' in
 *                      ndn_eth.h. If NULL, keep it unspecified.
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param local_addr    Default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param eth_type_len  Pointer to default Ethernet type/length field.
 *                      See description in IEEE 802.3.
 *                      If NULL, CSAP will have eth-type/len unconfigured
 *                      and will require it in traffic template.
 * @param cfi           Pointer to CFI field in VLAN header extension or
 *                      NULL
 * @param priority      Pointer to priority field in VLAN header extension
 *                      or NULL
 * @param vlan_id       Pointer to VLAN identifier field in VLAN header
 *                      extension or NULL
 *
 * @retval Status code.
 */
extern te_errno tapi_eth_csap_layer(const char      *device,
                                    const uint8_t   *recv_mode,
                                    const uint8_t   *remote_addr,
                                    const uint8_t   *local_addr,
                                    const uint16_t  *eth_type_len,
                                    te_bool         *cfi,
                                    const uint8_t   *priority,
                                    const uint16_t  *vlan_id,
                                    asn_value      **eth_layer);

/**
 * Create common Ethernet CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param device        Interface name on TA host
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param local_addr    Default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param eth_type_len  Default Ethernet type/length field.
 *                      See description in IEEE 802.3.
 *                      If NULL, CSAP will have eth-type/len unconfigured
 *                      and will require it in traffic template.
 * @param eth_csap      Identifier of created CSAP (OUT)
 *
 * @return Zero on success, otherwise standard or common TE error code.
 */
extern int tapi_eth_csap_create(const char *ta_name, int sid,
                                const char *device,
                                const uint8_t *remote_addr,
                                const uint8_t *local_addr,
                                const uint16_t *eth_type_len,
                                csap_handle_t *eth_csap);


/**
 * Create common Ethernet CSAP with specified receive mode.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param device        Interface name on TA host.
 * @param recv_mode     Receive mode, bit scale defined by elements of
 *                      'enum eth_csap_receive_mode' in ndn_eth.h.
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 * @param local_addr    Default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 *                      If NULL, CSAP will have remote address unconfigured
 *                      and will require it in traffic template.
 * @param eth_type_len  Default Ethernet type/length field.
 *                      See description in IEEE 802.3.
 *                      If NULL, CSAP will have eth-type/len unconfigured
 *                      and will require it in traffic template.
 * @param eth_csap      Identifier of created CSAP (OUT).
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern int tapi_eth_csap_create_with_mode(const char *ta_name, int sid,
                                          const char *device,
                                          uint8_t recv_mode,
                                          const uint8_t *remote_addr,
                                          const uint8_t *local_addr,
                                          const uint16_t *eth_type_len,
                                          csap_handle_t *eth_csap);

/**
 * Create Ethernet CSAP for processing tagged frames.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param device        Interface name on TA host
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 * @param local_addr    Default local MAC address. may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template).
 * @param eth_type_len  Default Ethernet type/length field.
 *                      See description in IEEE 802.3.
 * @param cfi           CFI field in VLAN header extension
 * @param priority      Priority field in VLAN header extension
 * @param vlan_id       VLAN identifier field in VLAN header extension
 * @param eth_csap      Identifier of created CSAP (OUT)
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern int tapi_eth_tagged_csap_create(const char *ta_name, int sid,
                                       const char *device,
                                       uint8_t    *remote_addr,
                                       uint8_t    *local_addr,
                                       uint16_t    eth_type_len,
                                       int         cfi,
                                       uint8_t     priority,
                                       uint16_t    vlan_id,
                                       csap_handle_t *eth_csap);


/**
 * Callback function for the tapi_eth_pkt_handler_data @a callback
 * parameter, it is called for each packet received by CSAP.
 *
 * @param header        Structure with Ethernet header of the frame.
 * @param payload       Payload of the frame.
 * @param plen          Length of the frame payload.
 * @param user_data     Pointer to user data, specified in @a user_data
 *                      field of tapi_eth_pkt_handler_data.
 */
typedef void (*tapi_eth_frame_callback)
                  (const ndn_eth_header_plain *header,
                   const uint8_t *payload, uint16_t plen,
                   void *user_data);

/**
 * Prepare Ethernet layer callback data for tapi_tad_trrecv_get(),
 * tapi_tad_trrecv_stop() or tapi_tad_trrecv_wait() routines.
 *
 * @param callback      User callback to be called for each received
 *                      packet
 * @param user_data     Opaque user data to be passed to @a callback
 *
 * @return Allocated structure to be passed to tapi_tad_trrecv_get(),
 *         tapi_tad_trrecv_stop() or tapi_tad_trrecv_wait() as
 *         @a cb_data.
 */
extern tapi_tad_trrecv_cb_data *tapi_eth_trrecv_cb_data(
                                    tapi_eth_frame_callback  callback,
                                    void                    *user_data);


/**
 * Creates traffic    pattern for a single Ethernet frame.
 *
 * @param src_mac     Desired source MAC address value. if NULL, source
 *                    address
 *                    of the Ethernet frame can be any
 * @param dst_mac     Desired destination MAC address value. if NULL,
 *                    destination address of the Ethernet frame can be any
 * @param eth_type    Desired type of Ethernet payload
 * @param pattern     Placeholder for the pattern (OUT)
 *
 * @returns Status of the operation
 */
extern int tapi_eth_prepare_pattern(const uint8_t *src_mac,
                                    const uint8_t *dst_mac,
                                    const uint16_t *eth_type,
                                    asn_value **pattern);

/**
 * Creates ASN value of Traffic-Pattern-Unit type with single Ethernet PDU.
 *
 * @param src_mac           Desired source MAC address value, may be NULL
 *                          for no matching by source MAC. 
 * @param dst_mac           Desired destination MAC address value, 
 *                          may be NULL
 *                          for no matching by source MAC. 
 * @param eth_type          Desired type of Ethernet payload, zero value 
 *                          used to specify not matching by eth_type field. 
 * @param pattern_unit      Placeholder for the pattern-unit (OUT)
 *
 * @returns zero on success or error status
 */
extern int tapi_eth_prepare_pattern_unit(uint8_t *src_mac,
                                         uint8_t *dst_mac,
                                         uint16_t eth_type,
                                         asn_value **pattern_unit);

/**
 * Print ethernet address to the specified file stream.
 *
 * @param f     - file stream handle
 * @param addr  - pointer to the array with Ethernet MAC address
 *
 * @return nothing.
 */
extern void tapi_eth_fprint_mac(FILE *f, const uint8_t *addr);

#endif /* __TE_TAPI_ETH_H__ */
