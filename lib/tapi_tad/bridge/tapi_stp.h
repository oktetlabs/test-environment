/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI TAD STP
 *
 * @defgroup tapi_tad_stp STP
 * @ingroup tapi_tad_main
 * @{
 *
 * Declarations of API for Spanning tree Protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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


#if 0
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
#endif

#endif /* __TE_TAPI_BRIDGE_STP_H__ */

/**@} <!-- END tapi_tad_stp --> */
