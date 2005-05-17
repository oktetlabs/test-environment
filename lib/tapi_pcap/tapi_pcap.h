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
#include "ndn_pcap.h"


/* Default recv mode: all except OUTGOING packets. */
#define PCAP_RECV_MODE_DEF (PCAP_RECV_HOST      | PCAP_RECV_BROADCAST | \
                            PCAP_RECV_MULTICAST | PCAP_RECV_OTHERHOST )

#define PCAP_LINKTYPE_DEFAULT   DLT_EN10MB


/**
 * Create common Ethernet-PCAP CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param ifname        Interface name on TA host
 * @param iftype        Interface datalink type (see man pcap)
 * @param recv_mode     Receive mode, bit scale defined by elements of
 *                      'enum pcap_csap_receive_mode' in ndn_pcap.h.
 * @param pcap_csap     Identifier of created CSAP (OUT)
 *
 * @return Zero on success, otherwise standard or common TE error code.
 */
extern int tapi_pcap_csap_create(const char *ta_name, int sid,
                                 const char *ifname, int iftype, 
                                 int recv_mode,
                                 csap_handle_t *pcap_csap);


/**
 * Callback function for the tapi_pcap_recv_start() routine, it is called
 * for each packet received for csap.
 *
 * @param filter_id     Filter ID that corresponds to received packet.
 * @param pkt_data      Received packet in binary form.
 * @param pkt_len       Length of the received packet.
 * @param userdata      Pointer to user data, provided by  the caller of
 *                      tapi_pcap_recv_start.
 */
typedef void (*tapi_pcap_recv_callback) (const int filter_id,
                                         const uint8_t *pkt_data,
                                         const uint16_t pkt_len,
                                         void *userdata);

/**
 * Start receive process on specified Ethernet-PCAP CSAP. If process
 * was started correctly (rc is OK) it can be managed by common RCF
 * methods 'rcf_ta_trrecv_wait', 'rcf_ta_trrecv_get' and
 * 'rcf_ta_trrecv_stop'.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param pcap_csap      CSAP handle
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
extern int tapi_pcap_recv_start(const char *ta_name, int sid,
                                csap_handle_t pcap_csap,
                                const asn_value *pattern,
                                tapi_pcap_recv_callback cb, void *cb_data,
                                unsigned int timeout, int num);

/**
 * Creates traffic    pattern for a single Ethernet-PCAP frame.
 *
 * @param filter      Tcpdump-like filtering rule
 * @param filter_id   Value that should be responsed when packet
 *                    match filtering rule 
 * @param pattern     Placeholder for the pattern (OUT)
 *
 * @returns Status of the operation
 */
extern int tapi_pcap_pattern_add(const char *filter,
                                 const int   filter_id,
                                 asn_value **pattern);

#endif /* __TE_TAPI_ETH_H__ */
