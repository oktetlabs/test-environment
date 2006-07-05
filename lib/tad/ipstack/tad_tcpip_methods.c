/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * TCP/IP special routines.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id: $
 */

#define TE_LGR_USER     "TAD TCP/IP misc" 

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "tad_ipstack_impl.h"

/**
 * Method to iterate huge number of TCP PUSH messages, using
 * one correctly generated frame with such message. 
 *
 * Prototype complies to function type tad_special_send_pkt_cb .
 *
 * User param should be string of format 
 * "<quantity of pkts>:<wanted throughput in byte per second>"
 */

te_errno
tad_tcpip_flood(csap_p csap, const char  *usr_param, tad_pkts *pkts)
{
//        tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    te_errno     rc;
    tad_pkt     *pkt = pkts->pkts.cqh_first;
    tad_pkt_seg *tcp_hdr_seg = pkt->segs.cqh_last->links.cqe_prev;

#define TCP_SEQ_OFFSET 4
#define TCP_CHKSUM_OFFSET 16
    uint32_t old_seq;
    uint32_t new_seq;
    uint16_t old_seq_chksum;
    uint16_t new_seq_chksum;

    uint32_t chksum;

    uint16_t new_pkt_chksum;

    int number_of_packets = (usr_param == NULL) ? 1 : atoi(usr_param);

    csap_write_cb_t write_cb;

    RING("%s started for %d pkts", __FUNCTION__, number_of_packets);

    write_cb = csap_get_proto_support(csap,
                   csap_get_rw_layer(csap))->write_cb;
    rc = write_cb(csap, pkt); 

    while ((--number_of_packets) > 0 && rc == 0)
    { 

        chksum = ntohs(*((uint16_t *)
                         (tcp_hdr_seg->data_ptr + TCP_CHKSUM_OFFSET)));
        old_seq = ntohl(*((uint32_t *)
                          (tcp_hdr_seg->data_ptr + TCP_SEQ_OFFSET)));

        new_seq = old_seq + pkt->segs.cqh_last->data_len;

        old_seq_chksum = (old_seq & 0xffff) + (old_seq >> 16);
        new_seq_chksum = (new_seq & 0xffff) + (new_seq >> 16);

        chksum += old_seq_chksum;
        chksum -= new_seq_chksum;

        new_pkt_chksum = (chksum & 0xffff) + (chksum >> 16);

        *((uint16_t *)(tcp_hdr_seg->data_ptr + TCP_CHKSUM_OFFSET)) =
            htons(new_pkt_chksum);
        *((uint32_t *)(tcp_hdr_seg->data_ptr + TCP_SEQ_OFFSET)) =
            htonl(new_seq);

        rc = write_cb(csap, pkt); 
    }
    RING("%s finished for %d pkts, rc %r",
         __FUNCTION__, number_of_packets, rc);

    return rc;
}
