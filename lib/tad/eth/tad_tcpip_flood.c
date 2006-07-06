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
 * $Id$
 */

#define TE_LGR_USER     "TAD TCP/IP misc" 

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "tad_eth_impl.h"

/** Internal data of Ethernet service access point via PF_PACKET sockets */
typedef struct tad_eth_sap_pf_packet_data {
    unsigned int    ifindex;    /**< Interface index */
    int             in;         /**< Input socket (for receive) */
    int             out;        /**< Output socket (for send) */
    unsigned int    send_mode;  /**< Send mode */
    unsigned int    recv_mode;  /**< Receive mode */
} tad_eth_sap_pf_packet_data;


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
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);
    tad_eth_sap_pf_packet_data *data;


    int ret_val;
    te_errno     rc;
    tad_pkt     *pkt     = pkts->pkts.cqh_first;
    tad_pkt_seg *hdr_seg = pkt->segs.cqh_first;

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
    size_t                      iovlen = tad_pkt_seg_num(pkt);
    struct iovec                iov[iovlen];
    int out_socket;

    uint8_t *flat_frame = NULL, *p;
    size_t   frame_size = 0;

    uint16_t *chksum_place;
    uint32_t *seq_place;

    int flags;

    size_t tcp_payload_size = pkt->segs.cqh_last->data_len;




    data = spec_data->sap.data;
    out_socket = data->out;

#if 1
    flags = 0;
    fcntl(out_socket, F_GETFL, &flags);
    flags |= O_NONBLOCK;
    fcntl(out_socket, F_SETFL, flags);
#endif
    tad_pkt_segs_to_iov(pkt, iov, iovlen);

    rc = tad_pkt_flatten_copy(pkt, &flat_frame, &frame_size);

    if (rc != 0)
    {
        ERROR("Failed to convert segments to flat data: %r", rc);
        return rc;
    }

    p = flat_frame;
    /* shift over Ethernet header: */
    p += hdr_seg->data_len;
    hdr_seg = hdr_seg->links.cqe_next;

    /* shift over IP header: */
    p += hdr_seg->data_len;
    hdr_seg = hdr_seg->links.cqe_next; 

    RING("%s(): frame begin %p, ETH and IP headers: %d", 
         __FUNCTION__, flat_frame, (p - flat_frame));
    /* shift in TCP header to our places: */
    seq_place    = (uint32_t *)(p + TCP_SEQ_OFFSET);
    chksum_place = (uint16_t *)(p + TCP_CHKSUM_OFFSET);

    RING("%s(): seq_place %p, chksum place  %p", 
         __FUNCTION__, seq_place, chksum_place);

    write_cb = csap_get_proto_support(csap,
                   csap_get_rw_layer(csap))->write_cb;
    rc = write_cb(csap, pkt); 

    chksum  = ntohs(*chksum_place);

    RING("%s (file %s) started for %d pkts, init checksum %d(0x%x)",
         __FUNCTION__, __FILE__,  number_of_packets, (int)chksum, (int)chksum);


    while ((--number_of_packets) > 0)
    { 

#if 0
        chksum = ntohs(*((uint16_t *)
                         (iov[iovlen-2].iov_base + TCP_CHKSUM_OFFSET)));
        old_seq = ntohl(*((uint32_t *)
                          (iov[iovlen-2].iov_base + TCP_SEQ_OFFSET)));
#else
        chksum  = ntohs(*chksum_place);
        old_seq = ntohl(*seq_place);
#endif

        new_seq = old_seq + tcp_payload_size;

        old_seq_chksum = (old_seq & 0xffff) + (old_seq >> 16);
        new_seq_chksum = (new_seq & 0xffff) + (new_seq >> 16);

        chksum += old_seq_chksum;
        chksum -= new_seq_chksum;

        new_pkt_chksum = (chksum & 0xffff) + (chksum >> 16);

#if 0
        *((uint16_t *)(iov[iovlen-2].iov_base + TCP_CHKSUM_OFFSET)) =
            htons(new_pkt_chksum);
        *((uint32_t *)(iov[iovlen-2].iov_base + TCP_SEQ_OFFSET)) =
            htonl(new_seq);
#else
        *chksum_place = htons(new_pkt_chksum);
        *seq_place = htonl(new_seq);
#endif

once_more:
        if (1)
        {
                    struct timeval clr_delay = { 0, 20 };

                    select(0, NULL, NULL, NULL, &clr_delay);
        }

#if 0
        ret_val = writev(out_socket, iov, iovlen); 
#else
        // ret_val = send(out_socket, flat_frame, frame_size, 0);
        ret_val = write(out_socket, flat_frame, frame_size);
#endif

        if (ret_val < 0)
        {
            rc = te_rc_os2te(errno);
            RING("%s() write() failed, errno %r",
                   __FUNCTION__, rc);
            switch (rc)
            {
                case TE_ENOBUFS:
                case TE_EAGAIN:
                {
                    /* 
                     * It seems that 0..127 microseconds is enough
                     * to hope that buffers will be cleared and
                     * does not fall down performance.
                     */
                    struct timeval clr_delay = { 0, 10 };

                    select(0, NULL, NULL, NULL, &clr_delay);
                    goto once_more;
                }

                default:
                    ERROR("%s: internal error %r", 
                          __FUNCTION__, rc);
                    return rc;
            }
        }
    }
    RING("%s finished for %d pkts, rc %r",
         __FUNCTION__, number_of_packets, rc);

    return rc;
}
