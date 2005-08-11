/** @file
 * @brief Proteos, TAPI STP - Spanning tree Protocol.
 *
 * Declarations of API for TAPI STP.
 *
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg Kravtsov <oleg@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_TAPI_BRIDGE_STP_H__
#define __TE_TAPI_BRIDGE_STP_H__

#include <sys/time.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_bridge.h"


/**
 * Creates STP CSAP that can be used for sending/receiving
 * Configuration and Notification BPDUs specified in 
 * Media Access Control (MAC) Bridges ANSI/IEEE Std. 802.1D,
 * 1998 Edition section 9
 *
 * @param ta_name        Test Agent name where CSAP will be created
 * @param sid            RCF session;
 * @param ifname         Name of an interface the CSAP is attached to
 *                       (frames are sent/captured from/on this interface)
 * @param own_mac_addr   Default MAC address used on the Agent:
 *                       - source MAC address of outgoing from the CASP
 *                         frames,
 * @param peer_mac_addr  Default peer MAC address:
 *                       - source MAC address of incoming into the CSAP
 *                         frames
 * @param stp_csap       Created STP CSAP (OUT)
 *
 * @return Status of the operation
 *
 * @note If "own_mac_addr" parameter is not NULL, then "peer_mac_addr" has
 * to be NULL, and vice versa - If "peer_mac_addr" parameter is not NULL,
 * then "own_mac_addr" has to be NULL.  If both "peer_mac_addr" and
 * "own_mac_addr" are NULL, then "own_mac_addr" is assumed to be MAC address
 * of the specified interface on the Agent.
 */
extern int tapi_stp_plain_csap_create(const char *ta_name, int sid,
                                      const char *ifname,
                                      const uint8_t *own_mac_addr,
                                      const uint8_t *peer_mac_addr,
                                      csap_handle_t *stp_csap);

/**
 * Sends STP BPDU from the specified CSAP
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session identifier
 * @param stp_csap      CSAP handle
 * @param templ         Traffic template
 *
 * @return Status of the operation
 * @retval TE_EINVAL  This code is returned if "peer_mac_addr" value wasn't
 *                 specified on creating the CSAP and "dst_mac_addr"
 *                 parameter is NULL.
 * @retval 0       BPDU is sent
 */
extern int tapi_stp_bpdu_send(const char *ta_name, int sid,
                              csap_handle_t stp_csap,
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
typedef void (*tapi_stp_bpdu_callback)
              (const ndn_stp_bpdu_t *bpdu, 
               const struct timeval *time_stamp,
               void *userdata);



/**
 * Start receive process on specified STP CSAP.
 * This function does not block the caller and they should use standard RCF
 * functions to manage with CSAP: 'rcf_ta_trrecv_wait', 'rcf_ta_trrecv_stop'
 * and 'rcf_ta_trrecv_get'.
 *
 * @param ta_name       Test Agent name where CSAP resides
 * @param sid           Session identifier
 * @param stp_csap      STP CSAP handle
 * @param pattern       Pattern used in matching incoming BPDU
 * @param callback      Callback function which will be called for every
 *                      received frame, may me NULL if frames are not need
 * @param callback_data Pointer to be passed to user callback
 * @param timeout       Timeout value to wait until STP BPDU is received
 * @param num           Number of wanted packets
 *
 * @return Status of the operation
 * @retval TIMEOUT
 */
extern int tapi_stp_bpdu_recv_start(const char *ta_name, int sid,
                                    csap_handle_t stp_csap,
                                    const asn_value *pattern,
                                    tapi_stp_bpdu_callback callback,
                                    void *callback_data,
                                    unsigned int timeout, int num);


#endif /* __TE_TAPI_BRIDGE_STP_H__ */
