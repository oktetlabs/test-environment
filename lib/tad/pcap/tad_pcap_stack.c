/** @file
 * @brief TAD PCAP
 *
 * Traffic Application Domain Command Handler.
 * Ethernet-PCAP CSAP stack-related callbacks.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Ethernet-PCAP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_IF_ETHER_H
#include <sys/sys_ether.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <assert.h>

#include <string.h>

#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_pcap_impl.h"
#include "ndn_pcap.h" 


/* Default recv mode: all except OUTGOING packets. */
#define PCAP_RECV_MODE_DEF (PCAP_RECV_HOST      | PCAP_RECV_BROADCAST | \
                            PCAP_RECV_MULTICAST | PCAP_RECV_OTHERHOST )

#ifndef IFNAME_SIZE
#define IFNAME_SIZE 256
#endif


typedef struct tad_pcap_rw_data {
    char       *ifname;         /**< Name of the net interface to filter
                                     packet on */

    int         in;             /**< Socket for receiving data */
   
    uint8_t     recv_mode;      /**< Receive mode, bit mask from values
                                     in enum pcap_csap_receive_mode i
                                     n ndn_pcap.h */
} tad_pcap_rw_data;


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_rw_init_cb(csap_p csap, const asn_value *csap_nds)
{
    int      rc; 
    char     choice[100] = "";
    char     ifname[IFNAME_SIZE];    /**< ethernet interface id
                                          (e.g. eth0, eth1) */
    size_t   val_len;                /**< stores corresponding value length */

    tad_pcap_rw_data   *spec_data; 
    const asn_value    *pcap_csap_spec; /**< ASN value with csap init
                                                     parameters  */
                        
    
    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EWRONGPTR);

    pcap_csap_spec = csap->layers[csap_get_rw_layer(csap)].csap_layer_pdu;

    rc = asn_get_choice(pcap_csap_spec, "", choice, sizeof(choice));
    VERB("eth_single_init_cb called for csap %d, ndn with type %s\n", 
                csap->id, choice);
    
    val_len = sizeof(ifname);
    rc = asn_read_value_field(pcap_csap_spec, ifname, &val_len, "ifname");
    if (rc) 
    {
        F_ERROR("device-id for ethernet not found: %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    
    spec_data = calloc (1, sizeof(*spec_data));
    if (spec_data == NULL)
    {
        ERROR("Init, not memory for spec_data");
        return TE_RC(TE_TAD_CSAP,  TE_ENOMEM);
    }
    csap_set_rw_data(csap, spec_data);
    
    spec_data->in = -1;

    spec_data->ifname = strdup(ifname);
    
    val_len = sizeof(spec_data->recv_mode);
    rc = asn_read_value_field(pcap_csap_spec, &spec_data->recv_mode, 
                              &val_len, "receive-mode");
    if (rc != 0)
    {
        spec_data->recv_mode = PCAP_RECV_MODE_DEF;
    }

    return 0;
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_rw_destroy_cb(csap_p csap)
{
    tad_pcap_rw_data   *spec_data; 

    /* FIXME: Is it necessary to do it here */
    (void)tad_pcap_shutdown_recv(csap);

    spec_data = csap_get_rw_data(csap);
    if (spec_data == NULL)
    {
        WARN("No PCAP CSAP %d special data found!", csap->id);
        return 0;
    }
    csap_set_rw_data(csap, NULL); 
    
    free(spec_data);

    return 0;
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_prepare_recv(csap_p csap)
{ 
    tad_pcap_rw_data   *spec_data = csap_get_rw_data(csap);
    int                 buf_size, rc;

    VERB("Before opened Socket %d", spec_data->in);

    /* opening incoming socket */
    if ((rc = open_packet_socket(spec_data->ifname, &spec_data->in)) != 0)
    {
        ERROR("open_packet_socket error %d", rc);
        return TE_OS_RC(TE_TAD_CSAP, rc);
    }

    VERB("csap %d Opened Socket %d", csap->id, spec_data->in);

    /* TODO: reasonable size of receive buffer to be investigated */
    buf_size = 0x100000; 
    if (setsockopt(spec_data->in, SOL_SOCKET, SO_RCVBUF,
                   &buf_size, sizeof(buf_size)) < 0)
    {
        /* FIXME */
        perror ("set RCV_BUF failed");
        fflush (stderr);
    }

    return 0;
}

/* See description tad_pcap_impl.h */
te_errno
tad_pcap_shutdown_recv(csap_p csap)
{
    tad_pcap_rw_data   *spec_data = csap_get_rw_data(csap);
    te_errno            rc = 0;

    if (spec_data->in >= 0)
    {
        VERB("%s: CSAP %d, close input socket %d", 
             __FUNCTION__, csap->id, spec_data->in);
        if (close(spec_data->in) < 0)
        {
            rc = TE_OS_RC(TE_TAD_CSAP, errno);
            WARN("%s: CLOSE input socket error %r", 
                 __FUNCTION__, rc);
        }
        spec_data->in = -1;
    }

    return rc;
}

/* See description tad_pcap_impl.h */
te_errno
tad_pcap_read_cb(csap_p csap, unsigned int timeout,
                 tad_pkt *pkt, size_t *pkt_len)
{
    tad_pcap_rw_data   *spec_data = csap_get_rw_data(csap);
    struct sockaddr_ll  from;
    socklen_t           fromlen = sizeof(from);
    te_errno            rc;

    rc = tad_common_read_cb_sock(csap, spec_data->in, MSG_TRUNC, timeout,
                                 pkt, SA(&from), &fromlen, pkt_len);
    if (rc != 0)
        return rc;

    switch (from.sll_pkttype)
    {
        case PACKET_HOST:
            if ((spec_data->recv_mode & PCAP_RECV_HOST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_BROADCAST:
            if ((spec_data->recv_mode & PCAP_RECV_BROADCAST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;
        case PACKET_MULTICAST:
            if ((spec_data->recv_mode & PCAP_RECV_MULTICAST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;
        case PACKET_OTHERHOST:
            if ((spec_data->recv_mode & PCAP_RECV_OTHERHOST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;
        case PACKET_OUTGOING:
            if ((spec_data->recv_mode & PCAP_RECV_OUTGOING) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;
    }

    return 0;
}
