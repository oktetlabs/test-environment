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

#ifndef __TE_LIB_TAPI_DHCP_H__
#define __TE_LIB_TAPI_DHCP_H__

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
 * @param pririty       Priority field in VLAN header extension
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
 * Sends traffic according specified template from the CSAP.
 * (the template can represent tagged and ordinary frames)
 * This function is blocking, returns after all packets are sent and
 * CSAP operation finished.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session identifier
 * @param eth_csap      CSAP handle
 * @param templ         Traffic template
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern int tapi_eth_send(const char *ta_name, int sid,
                         csap_handle_t eth_csap,
                         const asn_value *templ);

/**
 * Start send traffic process according template from specified CSAP.
 * (the template can represent tagged and ordinary frames).
 * This function is non-blocking, returns just after send command via RCF.
 * CSAP operation finished when all packets are send OR trsend_stop command
 * received.
 *
 * If all packets are successfully sent before trrecv_stop command, CSAP
 * status become 'CSAP_IDLE', i.e. free for new traffic activity, and no
 * trsend_stop command is need.
 *
 * If some packet was not sent by cause of some error, CSAP status become
 * CSAP_ERROR and CSAP waits 'trsend_stop' command to clear resources, rc
 * code to trsend_stop is error code related to packet sending failure.  In
 * this case 'trsend_stop' command should be sent by 'rcf_ta_trsend_stop'
 * function.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session identifier
 * @param eth_csap      CSAP handle
 * @param templ         Traffic template
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern int tapi_eth_send_start(const char *ta_name, int sid,
                               csap_handle_t eth_csap,
                               const asn_value *templ);

/**
 * Callback function for the tapi_eth_recv_start() routine, it is called
 * for each packet received for csap.
 *
 * @param header        Structure with Ethernet header of the frame.
 * @param payload       Payload of the frame.
 * @param plen          Length of the frame payload.
 * @param userdata      Pointer to user data, provided by  the caller of
 *                      tapi_eth_recv_start.
 */
typedef void (*tapi_eth_frame_callback)
                  (const ndn_eth_header_plain *header,
                   const uint8_t *payload, uint16_t plen,
                   void *userdata);

/**
 * Start receive process on specified Ethernet CSAP. If process
 * was started correctly (rc is OK) it can be managed by common RCF
 * methods 'rcf_ta_trrecv_wait', 'rcf_ta_trrecv_get' and
 * 'rcf_ta_trrecv_stop'.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param eth_csap      CSAP handle
 * @param pattern       ASN value with receive pattern
 * @param cb            Callback function which will be called for each
 *                      received frame, may me NULL if frames are not need
 * @param cb_data       Pointer to be passed to user callback
 * @param timeout       Timeout for receiving of packets, measured in
 *                      milliseconds
 * @param num           Number of packets caller wants to receive
 *
 * @return Zero on success, otherwise standard or common TE error code.
 */
extern int tapi_eth_recv_start(const char *ta_name, int sid,
                               csap_handle_t eth_csap,
                               const asn_value *pattern,
                               tapi_eth_frame_callback cb, void *cb_data,
                               unsigned int timeout, int num);

/**
 * Creates traffic pattern for a single Ethernet frame.
 *
 * @param src_mac  Desired source MAC address value. if NULL, source address
 *                 of the Ethernet frame can be any
 * @param dst_mac  Desired destination MAC address value. if NULL,
 *                 destination address of the Ethernet frame can be any
 * @param type     Desired type of Ethernet payload
 * @param pattern  Placeholder for the pattern (OUT)
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
 * @param src_mac       Desired source MAC address value, may be NULL
 *                      for no matching by source MAC. 
 * @param dst_mac       Desired destination MAC address value, may be NULL
 *                      for no matching by source MAC. 
 * @param type          Desired type of Ethernet payload, zero value 
 *                      used to specify not matching by eth_type field. 
 * @param pattern_unit  Placeholder for the pattern-unit (OUT)
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

#endif /* __TE_LIB_TAPI_DHCP_H__ */
