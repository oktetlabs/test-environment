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

#ifndef __TE_TAPI_PCAP_H__
#define __TE_TAPI_PCAP_H__

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
#include "tapi_tad.h"


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
 * Callback function for the tapi_pcap_pkt_handler() routine,
 * it is called for each packet received for csap.
 *
 * @param filter_id     Filter ID that corresponds to received packet.
 * @param pkt_data      Received packet in binary form.
 * @param pkt_len       Length of the received packet.
 * @param user_data     Pointer to user data, passed to
 *                      tapi_pcap_pkt_handler() in @a user_param as
 *                      @a user_data field of tapi_pcap_pkt_handler_data.
 */
typedef void (*tapi_pcap_recv_callback)(int            filter_id,
                                        const uint8_t *pkt_data,
                                        uint16_t       pkt_len,
                                        void          *user_data);

/**
 * Prepare PCAP callback data for tapi_tad_trrecv_get(),
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
extern tapi_tad_trrecv_cb_data *tapi_pcap_trrecv_cb_data(
                                    tapi_pcap_recv_callback  callback,
                                    void                    *user_data);


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

#endif /* __TE_TAPI_PCAP_H__ */
