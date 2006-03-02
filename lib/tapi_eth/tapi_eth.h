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
 *                      'enum eth_csap_receive_mode' in ndn_eth.h).
 *                      Use ETH_RECV_DEF by default.
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 * @param local_addr    Default local MAC address, may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template), frames to all dst's will
 *                      be caugth.
 * @param len_type      Pointer to default Ethernet Length/Type field.
 *                      See description in IEEE 802.3.
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
                                        const uint16_t  *len_type);

/**
 * Add Ethernet/802.1Q layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param device        Interface name on TA host or NULL (have to
 *                      be not-NULL, if it is the bottom layer)
 * @param recv_mode     Receive mode (bit scale defined by elements of
 *                      'enum eth_csap_receive_mode' in ndn_eth.h).
 *                      Use ETH_RECV_DEF by default.
 * @param remote_addr   Default remote MAC address, may be NULL - in this
 *                      case frames will be sent only dst is specified in
 *                      template, and frames from all src's will be
 *                      catched.
 * @param local_addr    Default local MAC address, may be NULL - in this
 *                      case frames will be sent with src specifed in
 *                      template or native for outgoing device (if not
 *                      present in template), frames to all dst's will
 *                      be caugth.
 * @param len_type      Pointer to default Ethernet Length/Type field.
 *                      See description in IEEE 802.3.
 * @param priority      Pointer to priority field in 802.1Q header
 *                      extension or NULL (0 on transmit by default,
 *                      match any on receive)
 * @param cfi           Pointer to CFI field in 802.1Q header extension or
 *                      NULL (0 on transmit by default, match any on
 *                      receive)
 * @param vlan_id       Pointer to VLAN identifier field in 802.1Q header
 *                      extension or NULL (0 on transmit by default,
 *                      match any on receive)
 *
 * @retval Status code.
 *
 * @sa tapi_eth_add_csap_layer
 */
extern te_errno tapi_eth_add_csap_layer_tagged(
                    asn_value      **csap_spec,
                    const char      *device,
                    unsigned int     recv_mode,
                    const uint8_t   *remote_addr,
                    const uint8_t   *local_addr,
                    const uint16_t  *len_type,
                    const uint8_t   *priority,
                    te_bool         *cfi,
                    const uint16_t  *vlan_id);


/**
 * Add Ethernet PDU as the last PDU to the last unit of the traffic 
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param is_pattern    Is the first argument template or pattern
 * @param dst_addr      Pointer to destination address or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation (as remove address for sending, as
 *                      local address for receiving).
 * @param src_addr      Pointer to source address or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation (as local address for sending, as
 *                      remote address for receiving).
 * @param len_type      Pointer to Length/Type or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation (otherwise, match any on receive).
 *
 * @return Status code.
 */
extern te_errno tapi_eth_add_pdu(asn_value     **tmpl_or_ptrn,
                                 te_bool         is_pattern,
                                 const uint8_t  *dst_addr,
                                 const uint8_t  *src_addr,
                                 const uint16_t *len_type);


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
 * @param len_type      Default Ethernet type/length field.
 *                      See description in IEEE 802.3.
 *                      If NULL, CSAP will have eth-type/len unconfigured
 *                      and will require it in traffic template.
 * @param eth_csap      Identifier of created CSAP (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_eth_csap_create(const char     *ta_name,
                                     int             sid,
                                     const char     *device,
                                     const uint8_t  *remote_addr,
                                     const uint8_t  *local_addr,
                                     const uint16_t *len_type,
                                     csap_handle_t  *eth_csap);


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

#endif /* __TE_TAPI_ETH_H__ */
