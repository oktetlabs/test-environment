/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * TCP/IP special routines.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD TCP/IP misc" 

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

/* whole file have no any sence without these libraries */
#if HAVE_NETPACKET_PACKET_H && HAVE_SYS_IOCTL_H && HAVE_NETINET_IN_H

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#if HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#endif

#include <fcntl.h>

#include <string.h>
#include <stdlib.h>

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "tad_eth_impl.h"
#include "logger_ta_fast.h" 


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


    te_errno     rc = 0;
    tad_pkt     *pkt     = pkts->pkts.cqh_first;
    tad_pkt_seg *hdr_seg = pkt->segs.cqh_first;

    uint32_t old_seq;
    uint32_t new_seq;
    uint32_t old_seq_chksum;
    uint32_t new_seq_chksum;
    uint16_t new_pkt_chksum;
    uint32_t chksum;

    uint8_t    *flat_frame = NULL, *p;
    size_t      frame_size = 0;

    uint16_t   *chksum_place;
    uint32_t   *seq_place;
    size_t      tcp_payload_size = pkt->segs.cqh_last->data_len;

    int flags;
    int ret_val;
    int number_of_packets = (usr_param == NULL) ? 1 : atoi(usr_param);
    int out_socket;

    struct timeval tv_start, tv_end;

    uint64_t frames_per_sec = number_of_packets;



    /*
     * ============== Prepare output packet socket =================
     */
    {
        tad_eth_rw_data     *spec_data = csap_get_rw_data(csap);
        struct sockaddr_ll   bind_addr;
        int                  buf_size;
        unsigned int         ifindex;

        ifindex = if_nametoindex(spec_data->sap.name);
        if (ifindex == 0)
        {
            rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
            ERROR("%s(): if_nametoindex(%s) failed: %r",
                  __FUNCTION__, spec_data->sap.name, rc);
            return rc;
        }

        out_socket = socket(PF_PACKET, SOCK_RAW, htons(0));
        if (out_socket < 0)
        {
            rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
            ERROR("%s(): socket(PF_PACKET, SOCK_RAW, 0) failed: %r",
                  __FUNCTION__, rc);
            return rc;
        }

        /*
         * Set send buffer size.
         * TODO: reasonable size of send buffer to be investigated.
         */
        buf_size = 0x100000; 
        if (setsockopt(out_socket, SOL_SOCKET, SO_SNDBUF,
                       &buf_size, sizeof(buf_size)) < 0)
        {
            rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
            ERROR("%s(): setsockopt(SO_SNDBUF) failed: %r", rc);
            return rc;
        }

        /* 
         * Bind PF_PACKET socket:
         *  - sll_protocol: 0 - do not receive any packets
         *  - sll_hatype. sll_pkttype, sll_halen, sll_addr are not used for
         *    binding
         */
        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sll_family = AF_PACKET;
        bind_addr.sll_protocol = htons(0);
        bind_addr.sll_ifindex = ifindex;

        if (bind(out_socket, SA(&bind_addr), sizeof(bind_addr)) < 0)
        {
            rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
            ERROR("Failed to bind PF_PACKET socket: %r", rc);
            return rc;
        }

        flags = 0;
        fcntl(out_socket, F_GETFL, &flags);
        flags |= O_NONBLOCK;
        fcntl(out_socket, F_SETFL, flags);

    }
    /* ======== Prepare output packet socket finished ========= */


    /* ============= Prepare frame for sending ================ */

#define TCP_SEQ_OFFSET 4
#define TCP_CHKSUM_OFFSET 16

    rc = tad_pkt_flatten_copy(pkt, &flat_frame, &frame_size);

    if (rc != 0)
    {
        ERROR("Failed to convert segments to flat data: %r", rc);
        close(out_socket);
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



    chksum  = ntohs(*chksum_place);

    /* ===================== Start sending ===================== */

    ret_val = write(out_socket, flat_frame, frame_size);

    RING("%s (file %s) started for %d pkts, init checksum %d(0x%x)",
         __FUNCTION__, __FILE__,  number_of_packets, (int)chksum, (int)chksum);


    chksum  = ntohs(*chksum_place);
    old_seq = ntohl(*seq_place);
    old_seq_chksum = (old_seq & 0xffff) + (old_seq >> 16);
    new_seq = old_seq;

    gettimeofday(&tv_start, NULL);

    while ((--number_of_packets) > 0)
    { 

        new_seq += tcp_payload_size;

        new_seq_chksum = (new_seq & 0xffff) + (new_seq >> 16);

        chksum += old_seq_chksum;
        chksum -= new_seq_chksum;

        chksum = new_pkt_chksum = (chksum & 0xffff) + (chksum >> 16);

        *chksum_place = htons(new_pkt_chksum);
        *seq_place = htonl(new_seq);
        old_seq_chksum = new_seq_chksum;

once_more:
#if 1
        if ((number_of_packets & 0xff) == 0)
        {
            fd_set wr_set;
            struct timeval clr_delay = { 0, 1 };
            FD_ZERO(&wr_set);
            FD_SET(out_socket, &wr_set);

            select(out_socket + 1, NULL, &wr_set, NULL, &clr_delay);
        }
#endif

        ret_val = write(out_socket, flat_frame, frame_size);

        if (ret_val < 0)
        {
            switch (errno)
            {
                case ENOBUFS:
                case EAGAIN:
                {
                    /* 
                     * It seems that 5 microseconds is enough
                     * to hope that buffers will be cleared and
                     * does not fall down performance.
                     */
                    struct timeval clr_delay = { 0, 1 };
                    fd_set wr_set;
                    FD_ZERO(&wr_set);
                    FD_SET(out_socket, &wr_set);
                    F_RING("try once more: %d errno, %d pkt rest", 
                           errno, number_of_packets); 
#if 1
                    select(out_socket + 1, NULL, &wr_set, NULL,
                           &clr_delay);
#endif
                    goto once_more;
                }

                default:
                    rc = te_rc_os2te(errno);
                    ERROR("%s() write() failed, errno %r",
                           __FUNCTION__, rc);
                    break;
            }
            if (rc != 0)
                break;
        }
    }
    gettimeofday(&tv_end, NULL);

    {
        uint64_t mcs_interval = (tv_end.tv_sec - tv_start.tv_sec) 
                                * 1000000;
        mcs_interval += tv_end.tv_usec;
        mcs_interval -= tv_start.tv_usec;

        frames_per_sec *= 1000000; 
        frames_per_sec /= mcs_interval;

        RING("%s finished rc %r, time %d mcs, speed %d frames/sec",
             __FUNCTION__, rc, (int)mcs_interval, (int)frames_per_sec);
    }

    close(out_socket);
    return rc;
}

/* HAVE_NETPACKET_PACKET_H && HAVE_SYS_IOCTL_H && HAVE_NETINET_IN_H */
#endif 
