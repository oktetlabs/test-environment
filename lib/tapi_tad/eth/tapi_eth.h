/** @file
 * @brief TAPI TAD Ethernet
 *
 * Declarations of test API for Ethernet TAD.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_ETH_H__
#define __TE_TAPI_ETH_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "tapi_tad.h"


/**
 * Add Ethernet layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param device        Interface name on TA host or NULL (have to
 *                      be not-NULL, if it is the bottom layer)
 * @param recv_mode     Receive mode (bit scale defined by elements of
 *                      'enum tad_eth_recv_mode' in tad_common.h).
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 * @param local_addr    Default local MAC address, may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template), frames to all dst's will
 *                      be caugth.
 * @param ether_type    Pointer to default EtherType.
 *                      See description in IEEE 802.3.
 * @param tagged        Whether frames should be VLAN tagged, any or
 *                      untagged.
 * @param llc           Whether frames should be 802.2 LLC, any or 
 *                      Ethernet2.
 *
 * @retval Status code.
 *
 * @sa tapi_eth_add_csap_layer_tagged
 */
extern te_errno tapi_eth_add_csap_layer(asn_value      **csap_spec,
                                        const char      *device,
                                        unsigned int     recv_mode,
                                        const uint8_t   *remote_addr,
                                        const uint8_t   *local_addr,
                                        const uint16_t  *ether_type,
                                        te_bool3         tagged,
                                        te_bool3         llc);

/**
 * Add Ethernet PDU as the last PDU to the last unit of the traffic 
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param pdu           Location for ASN.1 value pointer with added PDU
 * @param is_pattern    Is the first argument template or pattern
 * @param dst_addr      Pointer to destination address or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation (as remove address for sending, as
 *                      local address for receiving).
 * @param src_addr      Pointer to source address or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation (as local address for sending, as
 *                      remote address for receiving).
 * @param ether_type    Pointer to EtherType or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation (otherwise, match any on receive).
 * @param tagged        Whether frames should be VLAN tagged, any or
 *                      untagged.
 * @param llc           Whether frames should be 802.2 LLC, any or
 *                      Ethernet2.
 *
 * @return Status code.
 */
extern te_errno tapi_eth_add_pdu(asn_value      **tmpl_or_ptrn,
                                 asn_value      **pdu,
                                 te_bool          is_pattern,
                                 const uint8_t   *dst_addr,
                                 const uint8_t   *src_addr,
                                 const uint16_t  *ether_type,
                                 te_bool3         tagged,
                                 te_bool3         llc);

/**
 * Add exact specification of the Length/Type field of the IEEE 802.3
 * frame.
 *
 * @param pdu           ANS.1 PDU to add tag header
 * @param len_type      Length/Type value
 *
 * @return Status code.
 */
extern te_errno tapi_eth_pdu_length_type(asn_value      *pdu,
                                         const uint16_t  len_type);

/**
 * Add IEEE Std 802.1Q tag header.
 *
 * @note CFI is not specified here, since it affects E-RIF presence.
 *       By default, CFI is zero on send and match any on receive.
 *
 * @param pdu           ANS.1 PDU to add tag header
 * @param priority      Pointer to priority or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation. Otherwise, match any on receive and
 *                      use 0 on send.
 * @param vlan_id       Pointer to VLAN identifier or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation. Otherwise, match any on receive and
 *                      use 0 on send.
 *
 * @return Status code.
 */
extern te_errno tapi_eth_pdu_tag_header(asn_value      *pdu,
                                        const uint8_t  *priority,
                                        const uint16_t *vlan_id);

/**
 * Add IEEE Std 802.2 LLC and 802 SNAP sublayer headers.
 *
 * @param pdu           ANS.1 PDU to add sublayer headers
 *
 * @return Status code.
 */
extern te_errno tapi_eth_pdu_llc_snap(asn_value *pdu);


/**
 * Create common Ethernet CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param device        Interface name on TA host
 * @param receive_mode  Receive mode for Ethernet CSAP on the Interface
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
 * @param len_type      Default Ethernet Length/Type field.
 *                      See description in IEEE Std 802.3.
 *                      If NULL, CSAP will have Length/Type unconfigured
 *                      and will require it in traffic template.
 * @param eth_csap      Identifier of created CSAP (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_eth_csap_create(const char     *ta_name,
                                     int             sid,
                                     const char     *device,
                                     unsigned int    receive_mode,
                                     const uint8_t  *remote_addr,
                                     const uint8_t  *local_addr,
                                     const uint16_t *len_type,
                                     csap_handle_t  *eth_csap);


/**
 * Callback function for the tapi_eth_pkt_handler_data @a callback
 * parameter, it is called for each packet received by CSAP.
 *
 * @param packet        Packet in ASN.1 representation.
 * @param layer         Index of the Ethernet layer in packet.
 * @param header        Structure with Ethernet header of the frame.
 * @param payload       Payload of the frame.
 * @param plen          Length of the frame payload.
 * @param user_data     Pointer to user data, specified in @a user_data
 *                      field of tapi_eth_pkt_handler_data.
 */
typedef void (*tapi_eth_frame_callback)
                  (const asn_value *packet, int layer,
                   const ndn_eth_header_plain *header,
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

#endif /* __TE_TAPI_ETH_H__ */
