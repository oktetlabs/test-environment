/** @file
 * @brief PCAP TAD
 *
 * Traffic Application Domain Command Handler
 * Ethernet-PCAP CSAP, stack-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Ethernet-PCAP"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IF_ETHER_H
#include <sys/sys_ether.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <assert.h>

#include <string.h>

#if 0
#define TE_LOG_LEVEL    0xff
#endif

#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_pcap_impl.h"
#include "ndn_pcap.h" 

#if 0
#define PCAP_DEBUG(args...) \
    do {                                        \
        fprintf(stdout, "\nTAD PCAP stack " args);    \
        INFO("TAD PCAP stack " args);                 \
    } while (0)

#undef ERROR
#define ERROR(args...) PCAP_DEBUG("ERROR: " args)

#undef RING
#define RING(args...) PCAP_DEBUG("RING: " args)

#undef WARN
#define WARN(args...) PCAP_DEBUG("WARN: " args)

#undef VERB
#define VERB(args...) PCAP_DEBUG("VERB: " args)
#endif


/**
 * Release CSAP resources. 
 *
 * @param csap_descr    CSAP descriptor
 *
 * @return status code
 */
int
pcap_release(csap_p csap_descr)
{
    pcap_csap_specific_data_p spec_data;
    int layer;
    int bpf_id;

    VERB("%s() started", __FUNCTION__);

    layer = csap_descr->read_write_layer; 
    spec_data = (pcap_csap_specific_data_p)
                 csap_descr->layers[layer].specific_data; 

    for (bpf_id = 1; bpf_id <= spec_data->bpf_count; bpf_id++)
    {
        if (spec_data->bpfs[bpf_id] != 0)
        {
            pcap_freecode(spec_data->bpfs[bpf_id]);
            free(spec_data->bpfs[bpf_id]);
            spec_data->bpfs[bpf_id] = NULL;
        }
    }

    spec_data->bpf_count = 0;

    if (spec_data->in >= 0)
    {
        VERB("%s: CSAP %d, close input socket %d", 
             __FUNCTION__, csap_descr->id, spec_data->in);
        if (close(spec_data->in) < 0)
        {
            csap_descr->last_errno = errno;
            perror("close input socket");
            WARN("%s: CLOSE input socket error %d", 
                 __FUNCTION__, csap_descr->last_errno);
        }
        spec_data->in = -1;
    }

    if (spec_data->out >= 0)
    {
        VERB("%s: CSAP %d, close output socket %d", 
             __FUNCTION__, csap_descr->id, spec_data->out);
        if (close(spec_data->out) < 0)
        {
            csap_descr->last_errno = errno;
            perror("close output socket");
            WARN("%s: CLOSE output socket error %d", 
                 __FUNCTION__, csap_descr->last_errno);
        }
        spec_data->out = -1;
    }

    return 0;
}

/**
 * Open receive socket for Ethernet-PCAP CSAP 
 *
 * @param csap_descr    CSAP descriptor
 *
 * @return status code
 */
int
pcap_prepare_recv(csap_p csap_descr)
{ 
    pcap_csap_specific_data_p spec_data;
    int layer;
    int buf_size, rc;

    VERB("%s() started", __FUNCTION__);

    layer = csap_descr->read_write_layer;
    
    spec_data =
        (pcap_csap_specific_data_p) csap_descr->layers[layer].specific_data;
    
    VERB("Before opened Socket %d", spec_data->in);

    /* opening incoming socket */
    if ((rc = open_packet_socket(spec_data->ifname, &spec_data->in)) != 0)
    {
        ERROR("open_packet_socket error %d", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }

    VERB("csap %d Opened Socket %d", csap_descr->id, spec_data->in);

    /* TODO: reasonable size of receive buffer to be investigated */
    buf_size = 0x100000; 
    if (setsockopt(spec_data->in, SOL_SOCKET, SO_RCVBUF,
                   &buf_size, sizeof(buf_size)) < 0)
    {
        perror ("set RCV_BUF failed");
        fflush (stderr);
    }

    return 0;
 }




/**
 * Open transmit socket for Ethernet-PCAP CSAP 
 *
 * @param csap_descr    CSAP descriptor
 *
 * @return status code
 */
int
pcap_prepare_send(csap_p csap_descr)
{ 
    pcap_csap_specific_data_p spec_data;
    int layer;
    int buf_size, rc;

    VERB("%s() started", __FUNCTION__);

    layer = csap_descr->read_write_layer;
    
    spec_data =
        (pcap_csap_specific_data_p) csap_descr->layers[layer].specific_data;
   
    /* outgoing socket */
    if ((rc = open_packet_socket(spec_data->ifname, &spec_data->out)) != 0)
    { 
        ERROR("open_packet_socket error %d", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }

    /* TODO: reasonable size of send buffer to be investigated */
    buf_size = 0x100000; 
    if (setsockopt(spec_data->out, SOL_SOCKET, SO_SNDBUF,
                &buf_size, sizeof(buf_size)) < 0)
    {
        perror ("set RCV_BUF failed");
        fflush (stderr);
    }
    return 0;
}


/**
 * Free all memory allocated by Ethernet-PCAP csap specific data
 *
 * @param pcap_csap_specific_data_p poiner to structure
 * @param is_complete if PCAP_COMPLETE_FREE the free()
 *        will be called on passed pointer
 *
 */ 
void 
free_pcap_csap_data(pcap_csap_specific_data_p spec_data, te_bool is_complete)
{
    VERB("%s() started", __FUNCTION__);

    if (is_complete)
       free(spec_data);   
}


/* See description tad_pcap_impl.h */
int
tad_pcap_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    pcap_csap_specific_data_p   spec_data;
    int                         ret_val;
    int                         layer;
    fd_set                      read_set;
    struct sockaddr_ll          from;
    socklen_t                   fromlen = sizeof(from);
    struct timeval              timeout_val;
    int                         pkt_size;

    VERB("%s() started", __FUNCTION__);

    if (csap_descr == NULL)
    {
        csap_descr->last_errno = TE_EINVAL;
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (pcap_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    VERB("IN pcap_read_cb: spec_data->in = %d", spec_data->in);

#ifdef TALOGDEBUG
    printf ("PCAP recv: timeout %d; spec_data->read_timeout: %d\n", 
            timeout, spec_data->read_timeout);

    printf("Reading data from the socket: %d", spec_data->in);
#endif       

    if (spec_data->in < 0)
    {
        assert(0);
        return -1;
    }

    FD_ZERO(&read_set);
    FD_SET(spec_data->in, &read_set);

    if (timeout == 0)
    {
        timeout_val.tv_sec = spec_data->read_timeout;
        timeout_val.tv_usec = 0;
    }
    else
    {
        timeout_val.tv_sec = timeout / 1000000L; 
        timeout_val.tv_usec = timeout % 1000000L;
    }
    
    ret_val = select(spec_data->in + 1, &read_set, NULL, NULL, &timeout_val); 

#ifdef TALOGDEBUG
    printf ("PCAP select return %d\n", ret_val);
#endif
    
    if (ret_val == 0)
        return 0;

    if (ret_val < 0)
    {
        VERB("select fails: spec_data->in = %d",
                   spec_data->in);

        csap_descr->last_errno = errno;
        return -1;
    }
    
    /* TODO: possibly MSG_TRUNC and other flags are required */

    pkt_size = recvfrom(spec_data->in, buf, buf_len, MSG_TRUNC,
                       (struct sockaddr *)&from, &fromlen);

    if (pkt_size < 0)
    {
        VERB("recvfrom fails: spec_data->in = %d",
                   spec_data->in);
        csap_descr->last_errno = errno;
        return -1;
    }
    if (pkt_size == 0)
        return 0;

    switch(from.sll_pkttype)
    {
        case PACKET_HOST:
            if ((spec_data->recv_mode & PCAP_RECV_HOST) == 0)
                return 0;
            break;

        case PACKET_BROADCAST:
            if ((spec_data->recv_mode & PCAP_RECV_BROADCAST) == 0)
                return 0;
            break;
        case PACKET_MULTICAST:
            if ((spec_data->recv_mode & PCAP_RECV_MULTICAST) == 0)
                return 0;
            break;
        case PACKET_OTHERHOST:
            if ((spec_data->recv_mode & PCAP_RECV_OTHERHOST) == 0)
                return 0;
            break;
        case PACKET_OUTGOING:
            if ((spec_data->recv_mode & PCAP_RECV_OUTGOING) == 0)
                return 0;
            break;
    }

    return pkt_size;
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_single_init_cb(csap_p csap_descr, unsigned int layer,
                        const asn_value *csap_nds)
{
    int      rc; 
    char     choice[100] = "";
    char     ifname[IFNAME_SIZE];    /**< ethernet interface id
                                          (e.g. eth0, eth1) */
    size_t   val_len;                /**< stores corresponding value length */

    pcap_csap_specific_data_p   pcap_spec_data; 
    const asn_value            *pcap_csap_spec; /**< ASN value with csap init
                                                     parameters  */
                        
    char                        str_index_buf[10];
    
    VERB("%s() started", __FUNCTION__);

    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EWRONGPTR);

    /* TODO correct with Read-only get_indexed method */
    sprintf (str_index_buf, "%d", layer);
    rc = asn_get_subvalue(csap_nds, &pcap_csap_spec, str_index_buf);
    if (rc)
    {
        val_len = asn_get_length(csap_nds, "");
        ERROR("pcap_single_init_cb called for csap %d, layer %d,"
              "rc %r getting '%s', ndn has len %d\n", 
              csap_descr->id, layer, rc, str_index_buf, val_len);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    rc = asn_get_choice(pcap_csap_spec, "", choice, sizeof(choice));
    VERB("eth_single_init_cb called for csap %d, layer %d, ndn with type %s\n", 
                csap_descr->id, layer, choice);
    
    val_len = sizeof(ifname);
    rc = asn_read_value_field(pcap_csap_spec, ifname, &val_len, "ifname");
    if (rc) 
    {
        F_ERROR("device-id for ethernet not found: %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    
    pcap_spec_data = calloc (1, sizeof(pcap_csap_specific_data_t));
    if (pcap_spec_data == NULL)
    {
        ERROR("Init, not memory for spec_data");
        return TE_RC(TE_TAD_CSAP,  TE_ENOMEM);
    }
    
    pcap_spec_data->in = -1;
    pcap_spec_data->out = -1;

    pcap_spec_data->ifname = strdup(ifname);
    
    val_len = sizeof(pcap_spec_data->recv_mode);
    rc = asn_read_value_field(pcap_csap_spec, &pcap_spec_data->recv_mode, 
                              &val_len, "receive-mode");
    if (rc != 0)
    {
        pcap_spec_data->recv_mode = PCAP_RECV_MODE_DEF;
    }

    pcap_spec_data->iftype = PCAP_LINKTYPE_DEFAULT;;

    /* default read timeout */
    pcap_spec_data->read_timeout = PCAP_CSAP_DEFAULT_TIMEOUT; 

    csap_descr->layers[layer].specific_data = pcap_spec_data;

    csap_descr->read_cb          = tad_pcap_read_cb;
    csap_descr->write_cb         = NULL;
    csap_descr->write_read_cb    = NULL;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 500000;
    csap_descr->prepare_recv_cb  = pcap_prepare_recv;
    csap_descr->prepare_send_cb  = pcap_prepare_send;
    csap_descr->release_cb       = pcap_release;
    csap_descr->echo_cb          = NULL;

    csap_descr->layers[layer].get_param_cb = tad_pcap_get_param_cb; 
    
    return 0;
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_single_destroy_cb(csap_p csap_descr, unsigned int layer)
{
    pcap_csap_specific_data_p spec_data = 
        (pcap_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    VERB("%s() started", __FUNCTION__);

    VERB("CSAP N %d", csap_descr->id);

    if (spec_data == NULL)
    {
        WARN("No PCAP CSAP %d special data found!", csap_descr->id);
        return 0;
    }

    if(spec_data->in >= 0)
    {
        VERB("csap # %d, CLOSE SOCKET (%d): %s:%d",
                   csap_descr->id, spec_data->in, __FILE__, __LINE__);
        if (close_packet_socket(spec_data->ifname, spec_data->in) < 0)
            assert(0);
        spec_data->in = -1;
    }
    
    if (spec_data->out >= 0)
    {
        VERB("csap # %d, CLOSE SOCKET (%d): %s:%d",
                   csap_descr->id, spec_data->out, __FILE__, __LINE__);
        if (close_packet_socket(spec_data->ifname, spec_data->out) < 0)
            assert(0);
        spec_data->in = -1;
    }

    free_pcap_csap_data(spec_data, PCAP_COMPLETE_FREE);

    csap_descr->layers[layer].specific_data = NULL; 

    return 0;
}
