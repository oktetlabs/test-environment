/** @file
 * @brief TAD Ethernet
 *
 * Traffic Application Domain Command Handler.
 * Ethernet CSAP, stack-related callbacks.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Duka <duke@oktet.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Ethernet"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
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
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "logger_api.h"
#include "logger_ta_fast.h"

#include "ndn_eth.h" 
#include "tad_eth_impl.h"


/** Ethernet layer read/write specific data */
typedef struct tad_eth_rw_data {
    eth_interface_p interface;  /**< Ethernet interface data */

    int     out;            /**< Socket for sending data to the media */
    int     in;             /**< Socket for receiving data */
   
    int     read_timeout;   /**< Number of second to wait for data */

    uint8_t recv_mode;      /**< Receive mode, bit mask from values in 
                                 enum eth_csap_receive_mode in ndn_eth.h */
} tad_eth_rw_data;
                   

/* See description tad_eth_impl.h */
te_errno
tad_eth_prepare_send(csap_p csap)
{ 
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    int buf_size, rc;


    if (spec_data->out > 0) 
        return 0;
   
    /* outgoing socket */
    if ((rc = open_packet_socket(spec_data->interface->name,
                                 &spec_data->out)) != 0)
    { 
        ERROR("open_packet_socket error %d", rc);
        return TE_OS_RC(TE_TAD_CSAP, rc);
    }
    INFO(CSAP_LOG_FMT "open output socket %d", 
         CSAP_LOG_ARGS(csap), spec_data->out);

    buf_size = 0x100000; 
    /* TODO: reasonable size of send buffer to be investigated */
    if (setsockopt(spec_data->out, SOL_SOCKET, SO_SNDBUF,
                   &buf_size, sizeof(buf_size)) < 0)
    {
        perror ("set RCV_BUF failed");
        fflush (stderr);
    }

    return 0;
}

/* See description tad_eth_impl.h */
te_errno
tad_eth_shutdown_send(csap_p csap)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    if (spec_data->out >= 0)
    {
        fd_set          write_set;
        int             ret_val;
        struct timeval  wr_timeout = TAD_WRITE_TIMEOUT_DEFAULT; 

        /* check that all data in socket is sent */
        FD_ZERO(&write_set);
        FD_SET(spec_data->out, &write_set);

        ret_val = select(spec_data->out + 1, NULL, &write_set, NULL,
                         &wr_timeout);
        if (ret_val == 0)
            INFO("%s(CSAP %d): output socket %d is not ready for write", 
                 __FUNCTION__, csap->id, spec_data->out);
        else if (ret_val < 0)
            WARN("%s(CSAP %d): system errno on select, %d", 
                 __FUNCTION__, csap->id, errno);

        INFO(CSAP_LOG_FMT "close output socket %d", 
             CSAP_LOG_ARGS(csap), spec_data->out);
        if (close_packet_socket(spec_data->interface->name,
                                spec_data->out) < 0)
        {
            csap->last_errno = errno;
            WARN("%s: CLOSE output socket error 0x%X", 
                 __FUNCTION__, csap->last_errno);
        }
        spec_data->out = -1;
    }

    return 0;
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_prepare_recv(csap_p csap)
{ 
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    int buf_size, rc;
    

    VERB("Before opened Socket %d", spec_data->in);

    /* opening incoming socket */
    if ((rc = open_packet_socket(spec_data->interface->name,
                                 &spec_data->in)) != 0)
    {
        rc = TE_OS_RC(TE_TAD_CSAP, rc);
        ERROR("%s(): open_packet_socket() error: %r", __FUNCTION__, rc);
        return rc;
    }
    INFO(CSAP_LOG_FMT "open input socket %d", 
         CSAP_LOG_ARGS(csap), spec_data->in);

    /* TODO: reasonable size of receive buffer to be investigated */
    buf_size = 0x100000; 
    if (setsockopt(spec_data->in, SOL_SOCKET, SO_RCVBUF,
                   &buf_size, sizeof(buf_size)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_CSAP, errno);
        ERROR("%s(): set SO_RCVBUF to %d failed: %r", __FUNCTION__,
              buf_size, rc);
        /* FIXME close opened socket */
    }

    return rc;
}

/* See description tad_eth_impl.h */
te_errno
tad_eth_shutdown_recv(csap_p csap)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    if (spec_data->in >= 0)
    {
        INFO(CSAP_LOG_FMT "close input socket %d", 
             CSAP_LOG_ARGS(csap), spec_data->in);
        if (close_packet_socket(spec_data->interface->name,
                                spec_data->in) < 0)
        {
            csap->last_errno = errno;
            WARN("%s: CLOSE input socket error %d", 
                 __FUNCTION__, csap->last_errno);
        }
        spec_data->in = -1;
    }

#if 0
    tad_data_unit_clear(&spec_data->du_dst_addr);
    tad_data_unit_clear(&spec_data->du_src_addr);
    tad_data_unit_clear(&spec_data->du_eth_type);
    tad_data_unit_clear(&spec_data->du_cfi);
    tad_data_unit_clear(&spec_data->du_priority);
    tad_data_unit_clear(&spec_data->du_vlan_id);
#endif

    return 0;
}


/**
 * Free all memory allocated by eth csap specific data
 *
 * @param tad_eth_rw_data * poiner to structure
 * @param is_complete if ETH_COMPLETE_FREE the free() will be called on passed pointer
 *
 */ 
void 
free_eth_csap_data(tad_eth_rw_data *spec_data, char is_complete)
{
    free(spec_data->interface);
#if 0
    free(spec_data->remote_addr);
    free(spec_data->local_addr);   
#endif
       
    if (is_complete)
       free(spec_data);   
}


/* See description tad_eth_impl.h */
int 
tad_eth_read_cb(csap_p csap, int timeout, char *buf, size_t buf_len)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    int                      ret_val;
    int                      layer;
    fd_set                   read_set;
    struct sockaddr_ll       from;
    socklen_t                fromlen = sizeof(from);
    struct timeval           timeout_val;
    int                      pkt_size;

    if (csap == NULL)
    {
        ERROR("%s called with NULL csap", __FUNCTION__);
        return -1;
    }
    
    layer = csap_get_rw_layer(csap);
    
    gettimeofday(&timeout_val, NULL);
    VERB("%s(CSAP %d): spec_data->in = %d, mcs %d",
           __FUNCTION__, csap->id, spec_data->in,
           timeout_val.tv_usec);

#ifdef TALOGDEBUG
    printf ("ETH recv: timeout %d; spec_data->read_timeout: %d\n", 
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
    printf ("ETH select return %d\n", ret_val);
#endif
    
    if (ret_val == 0)
        return 0;

    if (ret_val < 0)
    {
        VERB("select fails: spec_data->in = %d",
                   spec_data->in);

        csap->last_errno = errno;
        return -1;
    }
    
    /* TODO: possibly MSG_TRUNC and other flags are required */

    pkt_size = recvfrom(spec_data->in, buf, buf_len, MSG_TRUNC,
                       (struct sockaddr *)&from, &fromlen);

    if (pkt_size < 0)
    {
        csap->last_errno = errno;
        WARN("recvfrom fails: spec_data->in = %d",
                   spec_data->in);
        return -1;
    }
    if (pkt_size == 0)
        return 0;

    gettimeofday(&timeout_val, NULL);
    VERB("%s(): CSAP %d read %d bytes, pkttype %d, mcs %d, begin 0x%x, "
         "ethtype %x", 
         __FUNCTION__, csap->id, pkt_size, from.sll_pkttype,
         timeout_val.tv_usec, *((uint32_t *)buf),
         (uint32_t)(*((uint16_t *)(buf + 12))));

    switch(from.sll_pkttype)
    {
        case PACKET_HOST:
            if ((spec_data->recv_mode & ETH_RECV_HOST) == 0)
                return 0;
            break;

        case PACKET_BROADCAST:
            if ((spec_data->recv_mode & ETH_RECV_BROADCAST) == 0)
                return 0;
            break;
        case PACKET_MULTICAST:
            if ((spec_data->recv_mode & ETH_RECV_MULTICAST) == 0)
                return 0;
            break;
        case PACKET_OTHERHOST:
            if ((spec_data->recv_mode & ETH_RECV_OTHERHOST) == 0)
                return 0;
            break;
        case PACKET_OUTGOING:
            if ((spec_data->recv_mode & ETH_RECV_OUTGOING) == 0)
                return 0;
            break;
    }


#if 0 
    /* Correction for number of read bytes was insered to synchronize 
     * with OS interface statistics, but it cause many side effects, 
     * therefore it is disabled now. */
    return pkt_size + ETHER_CRC_LEN;
#else
    return pkt_size;
#endif
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_write_cb(csap_p csap, const tad_pkt *pkt)
{
    size_t              iovlen = tad_pkt_seg_num(pkt);
    struct iovec        iov[iovlen];
    tad_eth_rw_data    *spec_data = csap_get_rw_data(csap);
    te_errno            rc;
    unsigned int        retries = TAD_WRITE_RETRIES;
    int                 ret_val = 0;
    fd_set              write_set;


    F_VERB("%s: writing data to socket: %d", __FUNCTION__, spec_data->out);

    if (spec_data->out < 0)
    {
        ERROR("%s(): no output socket", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    /* Convert packet segments to IO vector */
    rc = tad_pkt_segs_to_iov(pkt, iov, iovlen);
    if (rc != 0)
    {
        ERROR("Failed to convert segments to I/O vector: %r", rc);
        return rc;
    }

    for (retries = 0; ret_val <= 0 && retries < TAD_WRITE_RETRIES; retries++)
    {
        /* TODO: investigate question of write wait timeout */
        struct timeval wr_timeout = TAD_WRITE_TIMEOUT_DEFAULT; 
    
        FD_ZERO(&write_set);
        FD_SET(spec_data->out, &write_set);

        ret_val = select(spec_data->out + 1, NULL, &write_set, NULL,
                         &wr_timeout);
        if (ret_val == 0)
        {
            csap->last_errno = ETIMEDOUT; /* ??? */
            F_INFO("ETH write select timedout, retry %d", retries);
            continue;
        }

        if (ret_val == 1)
            ret_val = writev(spec_data->out, iov, iovlen);

        if (ret_val < 0)
        {
            csap->last_errno = errno;
            VERB("CSAP #%d, errno %d, retry %d",
                   csap->id, errno, retries);
            switch (errno)
            {
                case ENOBUFS:
                    {
                        /* 
                         * It seems that 0..127 microseconds is enough
                         * to hope that buffers will be cleared and
                         * does not fall down performance.
                         */
                        struct timeval clr_delay = { 0, rand() & 0x3f };

                        select(0, NULL, NULL, NULL, &clr_delay);
                    }
                    continue;

                default:
#if 0
                    {
                        unsigned int    i;
                        size_t          total = 0;

                        ERROR("pkt=%p iovlen=%u iov=%p",
                              pkt, (unsigned)iovlen, iov);
                        for (i = 0; i < iovlen; ++i)
                        {
                            total += iov[i].iov_len;
                            ERROR("#%u: len=%u ptr=%p", i,
                                  iov[i].iov_len, iov[i].iov_base);
                        }
                        ERROR("total=%u", (unsigned)total);
                    }
#endif
                    ERROR("%s(CSAP %d): internal error %d, socket %d", 
                          __FUNCTION__, csap->id,
                          csap->last_errno, spec_data->out);
                    return TE_OS_RC(TE_TAD_CSAP, errno);
            }
        } 
    }
    if (retries == TAD_WRITE_RETRIES)
    {
        ERROR("csap #%d, too many retries made, failed");
        csap->last_errno = ENOBUFS;
        return TE_RC(TE_TAD_CSAP, TE_ENOBUFS);;
    }

    F_VERB("csap #%d, system write return %d", 
            csap->id, ret_val); 

    if (ret_val < 0)
        return TE_OS_RC(TE_TAD_CSAP, errno);

    return 0;
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_rw_init_cb(csap_p csap, const asn_value *csap_nds)
{
    te_errno            rc; 
    unsigned int        layer = csap_get_rw_layer(csap);
    char                device_id[IFNAME_SIZE];
    size_t              val_len;
    eth_interface_p     iface_p;
    tad_eth_rw_data    *spec_data; 
    const asn_value    *eth_csap_spec; 
    

    UNUSED(csap_nds);
    eth_csap_spec = csap->layers[layer].csap_layer_pdu;
    
    val_len = sizeof(device_id);
    rc = asn_read_value_field(eth_csap_spec, device_id, &val_len,
                              "device-id");
    if (rc != 0) 
    {
        ERROR("device-id for ethernet not found: %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    
    /* allocating new interface structure and trying to init by name */    
    iface_p = malloc(sizeof(*iface_p));
    if (iface_p == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    
    if ((rc = eth_find_interface(device_id, iface_p)) != ETH_IFACE_OK)
    {
        switch (rc) 
        {
            case ETH_IFACE_HWADDR_ERROR:
                ERROR("Get iface <%s> hwaddr error", device_id);
                rc = TE_RC(TE_TAD_CSAP, TE_EFAIL); /* TODO: correct rc */
                break;
                
            case ETH_IFACE_IFINDEX_ERROR:
                ERROR("Interface index for <%s> could not get", 
                        device_id);
                rc = TE_RC(TE_TAD_CSAP, TE_EFAIL); /* TODO: correct rc */ 
                break;
                
            case ETH_IFACE_NOT_FOUND:
                ERROR("Interface <%s> not found", device_id);
                rc = TE_RC(TE_TAD_CSAP, TE_ENODEV); /* TODO: correct rc */ 
                break;
                
            default:
                ERROR("Interface <%s> config failure %r", device_id, rc);
                rc = TE_RC(TE_TAD_CSAP, TE_EFAIL);
        }
        free(iface_p);        
        return rc; 
    }

    spec_data = calloc(1, sizeof(*spec_data));
    if (spec_data == NULL)
    {
        free(iface_p);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    
    spec_data->in = -1;
    spec_data->out = -1;

    /* setting default interface    */
    spec_data->interface = iface_p;

    val_len = sizeof(spec_data->recv_mode);
    rc = asn_read_value_field(eth_csap_spec, &spec_data->recv_mode, 
                              &val_len, "receive-mode");
    if (rc != 0)
    {
        spec_data->recv_mode = ETH_RECV_MODE_DEF;
    }

    /* default read timeout */
    spec_data->read_timeout = ETH_CSAP_DEFAULT_TIMEOUT; 

    csap_set_rw_data(csap, spec_data);

    csap->timeout = 500000;

    return 0;
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_rw_destroy_cb(csap_p csap)
{
    tad_eth_rw_data    *spec_data = csap_get_rw_data(csap); 

    if (spec_data == NULL)
    {
        WARN("Not ethernet CSAP %d special data found!", csap->id);
        return 0;
    }

    eth_free_interface(spec_data->interface);

    if (spec_data->in >= 0)
    {
        INFO("%s(CSAP %d), close in socket %d",
             __FUNCTION__, csap->id, spec_data->in);
        if (close_packet_socket(spec_data->interface->name, spec_data->in) < 0)
            assert(0);
        spec_data->in = -1;
    }
    
    if (spec_data->out >= 0)
    {
        INFO("%s(CSAP %d), close out socket %d",
             __FUNCTION__, csap->id, spec_data->out);
        if (close_packet_socket(spec_data->interface->name, spec_data->out) < 0)
            assert(0);
        spec_data->out = -1;
    }

    free_eth_csap_data(spec_data, ETH_COMPLETE_FREE);

    csap_set_rw_data(csap, NULL); 

    return 0;
}


#if 0
/**
 * Ethernet echo CSAP method. 
 * Method should prepare binary data to be send as "echo" and call 
 * respective write method to send it. 
 * Method may change data stored at passed location.  
 *
 * @param csap    identifier of CSAP
 * @param pkt           Got packet, plain binary data. 
 * @param len           length of data.
 *
 * @return zero on success or error code.
 */
int 
eth_echo_method(csap_p csap, uint8_t *pkt, size_t len)
{
    uint8_t tmp_buffer [10000]; 

    if (csap == NULL || pkt == NULL || len == 0)
        return TE_EINVAL;
#if 1
    memcpy(tmp_buffer, pkt + ETHER_ADDR_LEN, ETHER_ADDR_LEN);
    memcpy(tmp_buffer + ETHER_ADDR_LEN, pkt, ETHER_ADDR_LEN);
    memcpy(tmp_buffer + 2*ETHER_ADDR_LEN, pkt + 2*ETHER_ADDR_LEN,
           len - 2 * ETHER_ADDR_LEN);

#if 0
    /* Correction for number of read bytes was insered to synchronize 
     * with OS interface statistics, but it cause many side effects, 
     * therefore it is disabled now. */

    tad_eth_write_cb(csap, tmp_buffer, len - ETHER_CRC_LEN);
#else
    tad_eth_write_cb(csap, tmp_buffer, len);
#endif


#endif

    return 0;
    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}
#endif
