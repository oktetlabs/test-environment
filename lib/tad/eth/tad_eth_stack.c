/** @file
 * @brief Ethernet TAD
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP, stack-related callbacks.
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
 * @author Andrew Duka <duke@oktet.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Ethernet"

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
#include "tad_eth_impl.h"
#include "ndn_eth.h" 

#include "logger_api.h"
#include "logger_ta_fast.h"



#ifndef INSQUE
/* macros to insert element p into queue _after_ element q */
#define INSQUE(p, q) do {(p)->prev = q; (p)->next = (q)->next; \
                      (q)->next = p; (p)->next->prev = p; } while (0)
/* macros to remove element p from queue  */
#define REMQUE(p) do {(p)->prev->next = (p)->next; (p)->next->prev = (p)->prev; \
                   (p)->next = (p)->prev = p; } while(0)
#endif

                   

typedef struct iface_user_rec {
    struct iface_user_rec *next, *prev;
    char name[IFNAME_SIZE];
    int  num_users;
} iface_user_rec;

static iface_user_rec iface_users_root = {&iface_users_root, &iface_users_root, {0,},0};


typedef enum {
    IN_PACKET_SOCKET,
    OUT_PACKET_SOCKET,
} tad_packet_dir_t;

/**
 * Release CSAP resources. 
 *
 * @param csap_descr    CSAP descriptor
 *
 * @return status code
 */
int
eth_release(csap_p csap_descr)
{
    eth_csap_specific_data_p spec_data;
    int layer;

    layer = csap_descr->read_write_layer; 
    spec_data = (eth_csap_specific_data_p)
                    csap_descr->layers[layer].specific_data; 

    if (spec_data->in >= 0)
    {
        INFO("%s(): CSAP %d, close input socket %d", 
             __FUNCTION__, csap_descr->id, spec_data->in);
        if (close_packet_socket(spec_data->interface->name, spec_data->in) < 0)
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
        fd_set          write_set;
        int             ret_val;
        struct timeval  wr_timeout = TAD_WRITE_TIMEOUT_DEFAULT; 

        /* check that all data in socket is sent */
        FD_ZERO(&write_set);
        FD_SET(spec_data->out, &write_set);

        ret_val = select(spec_data->out + 1, NULL, &write_set, NULL,
                         &wr_timeout);
        if (ret_val == 0)
            RING("%s(CSAP %d): output socket %d is not ready for write", 
                 __FUNCTION__, csap_descr->id, spec_data->out);
        else if (ret_val < 0)
            WARN("%s(CSAP %d): system errno on select, %d", 
                 __FUNCTION__, csap_descr->id, errno);

        INFO("%s(CSAP %d): close output socket %d", 
             __FUNCTION__, csap_descr->id, spec_data->out);
        if (close_packet_socket(spec_data->interface->name, spec_data->out) < 0)
        {
            csap_descr->last_errno = errno;
            perror("close output socket");
            WARN("%s: CLOSE output socket error 0x%X", 
                 __FUNCTION__, csap_descr->last_errno);
        }
        spec_data->out = -1;
    }

    tad_data_unit_clear(&spec_data->du_dst_addr);
    tad_data_unit_clear(&spec_data->du_src_addr);
    tad_data_unit_clear(&spec_data->du_eth_type);
    tad_data_unit_clear(&spec_data->du_cfi);
    tad_data_unit_clear(&spec_data->du_priority);
    tad_data_unit_clear(&spec_data->du_vlan_id);

    return 0;
}

/**
 * Open receive socket for ethernet CSAP 
 *
 * @param csap_descr    CSAP descriptor
 *
 * @return status code
 */
int
eth_prepare_recv(csap_p csap_descr)
{ 
    eth_csap_specific_data_p spec_data;
    int layer;
    int buf_size, rc;
    

    layer = csap_descr->read_write_layer;
    
    spec_data =
        (eth_csap_specific_data_p)csap_descr->layers[layer].specific_data;
    
    VERB("Before opened Socket %d", spec_data->in);

    /* opening incoming socket */
    if ((rc = open_packet_socket(spec_data->interface->name,
                                 &spec_data->in)) != 0)
    {
        rc = TE_OS_RC(TE_TAD_CSAP, rc);
        ERROR("%s(): open_packet_socket() error: %r", __FUNCTION__, rc);
        return rc;
    }

    INFO("%s(CSAP %d) open in socket %d", 
         __FUNCTION__, csap_descr->id, spec_data->in);

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




/**
 * Open transmit socket for ethernet CSAP 
 *
 * @param csap_descr    CSAP descriptor
 *
 * @return status code
 */
int
eth_prepare_send(csap_p csap_descr)
{ 
    eth_csap_specific_data_p spec_data;
    int layer;
    int buf_size, rc;

    layer = csap_descr->read_write_layer;
    
    spec_data = (eth_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    if (spec_data->out > 0) 
        return 0;
   
    /* outgoing socket */
    if ((rc = open_packet_socket(spec_data->interface->name,
                                 &spec_data->out)) != 0)
    { 
        ERROR("open_packet_socket error %d", rc);
        return TE_OS_RC(TE_TAD_CSAP, rc);
    }
    RING("%s(CSAP %d) open out socket %d", 
         __FUNCTION__, csap_descr->id, spec_data->out);

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

/**
 * Free all memory allocated by eth csap specific data
 *
 * @param eth_csap_specific_data_p poiner to structure
 * @param is_complete if ETH_COMPLETE_FREE the free() will be called on passed pointer
 *
 */ 
void 
free_eth_csap_data(eth_csap_specific_data_p spec_data, char is_complete)
{
    free(spec_data->interface);
    
    if (spec_data->remote_addr != NULL)
       free(spec_data->remote_addr);
    
    if (spec_data->local_addr != NULL)
       free(spec_data->local_addr);   
       
    if (is_complete)
       free(spec_data);   
}


/* See description tad_eth_impl.h */
int 
tad_eth_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    eth_csap_specific_data_p spec_data;
    int                      ret_val;
    int                      layer;
    fd_set                   read_set;
    struct sockaddr_ll       from;
    socklen_t                fromlen = sizeof(from);
    struct timeval           timeout_val;
    int                      pkt_size;

    if (csap_descr == NULL)
    {
        ERROR("%s called with NULL csap_descr", __FUNCTION__);
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (eth_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    gettimeofday(&timeout_val, NULL);
    VERB("%s(CSAP %d): spec_data->in = %d, mcs %d",
           __FUNCTION__, csap_descr->id, spec_data->in,
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

        csap_descr->last_errno = errno;
        return -1;
    }
    
    /* TODO: possibly MSG_TRUNC and other flags are required */

    pkt_size = recvfrom(spec_data->in, buf, buf_len, MSG_TRUNC,
                       (struct sockaddr *)&from, &fromlen);

    if (pkt_size < 0)
    {
        csap_descr->last_errno = errno;
        WARN("recvfrom fails: spec_data->in = %d",
                   spec_data->in);
        return -1;
    }
    if (pkt_size == 0)
        return 0;

    gettimeofday(&timeout_val, NULL);
    VERB("%s(): CSAP %d read %d bytes, pkttype %d, mcs %d, begin 0x%x, "
         "ethtype %x", 
         __FUNCTION__, csap_descr->id, pkt_size, from.sll_pkttype,
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
    return pkt_size + ETH_TAILING_CHECKSUM;
#else
    return pkt_size;
#endif
}


/* See description tad_eth_impl.h */
int 
tad_eth_write_cb(csap_p csap_descr, const char *buf, size_t buf_len)
{
    int ret_val = 0;
    eth_csap_specific_data_p spec_data;
    int layer;    
    fd_set write_set;
    int retries = TAD_WRITE_RETRIES;

   
    if (csap_descr == NULL)
    {
        csap_descr->last_errno = TE_EINVAL;
        ERROR("%s: no csap descr", __FUNCTION__);
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (eth_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    F_VERB("%s: writing data to socket: %d", __FUNCTION__, spec_data->out);

    if(spec_data->out < 0)
    {
        ERROR("%s: no output socket", __FUNCTION__);
        csap_descr->last_errno = TE_EINVAL;
        return -1;
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
            csap_descr->last_errno = ETIMEDOUT; /* ??? */
            F_INFO("ETH write select timedout, retry %d", retries);
            continue;
        }

        if (ret_val == 1)
            ret_val = write(spec_data->out, buf, buf_len);

        if (ret_val < 0)
        {
            csap_descr->last_errno = errno;
            VERB("CSAP #%d, errno %d, retry %d",
                   csap_descr->id, errno, retries);
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
                    ERROR("%s(CSAP %d): internal error %d, socket %d", 
                          __FUNCTION__, csap_descr->id,
                          csap_descr->last_errno, spec_data->out);
                    return -1;
            }
        } 
    }
    if (retries == TAD_WRITE_RETRIES)
    {
        ERROR("csap #%d, too many retries made, failed");
        csap_descr->last_errno = ENOBUFS;
        return -1;
    }

    F_VERB("csap #%d, system write return %d", 
            csap_descr->id, ret_val); 

    if (ret_val < 0)
        return -1;

#if 0 
    /* Correction for number of read bytes was insered to synchronize 
     * with OS interface statistics, but it cause many side effects, 
     * therefore it is disabled now. */
    return ret_val + ETH_TAILING_CHECKSUM;
#else
    return ret_val;
#endif
}


/* See description tad_eth_impl.h */
int 
tad_eth_write_read_cb(csap_p csap_descr, int timeout,
                      const char *w_buf, size_t w_buf_len,
                      char *r_buf, size_t r_buf_len)
{
    int ret_val; 

    ret_val = tad_eth_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (ret_val == -1)  
        return ret_val;
    else 
        return tad_eth_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_single_check_pdus(csap_p csap_descr, asn_value *traffic_nds)
{
    char     choice_label[20];
    te_errno rc;

    RING("%s(CSAP %d) called", __FUNCTION__, csap_descr->id);

    rc = asn_get_choice(traffic_nds, "pdus.0", choice_label, 
                        sizeof(choice_label));

    if (rc && TE_RC_GET_ERROR(rc) != TE_EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        asn_value *eth_pdu = asn_init_value(ndn_eth_header); 
        asn_value *asn_pdu    = asn_init_value(ndn_generic_pdu); 
        
        asn_write_component_value(asn_pdu, eth_pdu, "#eth");
        asn_insert_indexed(traffic_nds, asn_pdu, 0, "pdus"); 

        asn_free_value(asn_pdu);
        asn_free_value(eth_pdu);
    } 
    else if (strcmp (choice_label, "eth") != 0)
    {
        return TE_ETADWRONGNDS;
    }
    return 0;
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_single_init_cb(csap_p csap_descr, unsigned int layer,
                       const asn_value *csap_nds)
{
    te_errno rc; 
    char     device_id[IFNAME_SIZE];
    char     local_addr[ETH_ALEN];
    char     remote_addr[ETH_ALEN];
    size_t   val_len;
    
    eth_interface_p iface_p; /**< pointer to interface structure to be used with csap */

    eth_csap_specific_data_p    eth_spec_data; 
    const asn_value            *eth_csap_spec; 
                        /**< ASN value with csap init parameters    */
    int                         eth_type;      /**< Ethernet type                          */
    char     str_index_buf[10];
    
    INFO("%s called for csap %d, layer %d",
         __FUNCTION__, csap_descr->id, layer); 

    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EWRONGPTR);

    /* TODO correct with Read-only get_indexed method */
    sprintf (str_index_buf, "%d", layer);
    rc = asn_get_subvalue(csap_nds, &eth_csap_spec, str_index_buf);
    if (rc)
    {
        val_len = asn_get_length(csap_nds, "");
        ERROR("%s(CSAP %d), layer %d, rc %r getting '%s', ndn has len %d", 
              __FUNCTION__, csap_descr->id, layer, rc, str_index_buf, val_len);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    
    val_len = sizeof(device_id);
    rc = asn_read_value_field(eth_csap_spec, device_id, &val_len, "device-id");
    if (rc) 
    {
        F_ERROR("device-id for ethernet not found: %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    
    /* allocating new interface structure and trying to init by name */    
    iface_p = malloc(sizeof(eth_interface_t));
    
    if (iface_p == NULL)
    {
#ifdef TALOGDEBUG
        printf("Can't allocate memory for interface");
#endif    
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    
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
                rc = TE_RC(TE_TAD_CSAP, TE_ENODEV); /* TODO: correct rc */ 
                ERROR("Interface <%s> not found", device_id);
                break;
                
            default:
                rc = TE_RC(TE_TAD_CSAP, TE_EFAIL);
                ERROR("Interface <%s> config failure %r", device_id, rc);
        }
        free(iface_p);        
        return rc; 
    }

    eth_spec_data = calloc (1, sizeof(eth_csap_specific_data_t));
    
    if (eth_spec_data == NULL)
    {
        free(iface_p);
        ERROR("Init, not memory for spec_data");
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    
    eth_spec_data->in = -1;
    eth_spec_data->out = -1;

    /* setting default interface    */
    eth_spec_data->interface = iface_p;

    
    val_len = sizeof(eth_spec_data->recv_mode);
    rc = asn_read_value_field(eth_csap_spec, &eth_spec_data->recv_mode, 
            &val_len, "receive-mode");
    if (rc)
    {
        eth_spec_data->recv_mode = ETH_RECV_MODE_DEF;
    }

    /* setting remote address */
    val_len = sizeof(remote_addr);
    rc = asn_read_value_field(eth_csap_spec, remote_addr, &val_len, "remote-addr");
    
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) 
    {
        eth_spec_data->remote_addr = NULL;
    }
    else if (rc)
    {
        free_eth_csap_data(eth_spec_data, ETH_COMPLETE_FREE);
        ERROR("Init, asn read of rem_addr");
        return TE_RC(TE_TAD_CSAP, rc);
    }
    else
    {
        eth_spec_data->remote_addr = malloc(ETH_ALEN);
    
        if (eth_spec_data->remote_addr == NULL)
        {
            free_eth_csap_data(eth_spec_data, ETH_COMPLETE_FREE);
            ERROR("Init, no mem");
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM); 
        }
        memcpy (eth_spec_data->remote_addr, remote_addr, ETH_ALEN);
    }

    /* setting local address */
    
    val_len = sizeof(local_addr); 
    rc = asn_read_value_field(eth_csap_spec, local_addr, &val_len, "local-addr");
    
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) 
        eth_spec_data->local_addr = NULL;
    else if (rc)
    {
        free_eth_csap_data(eth_spec_data, ETH_COMPLETE_FREE);
        ERROR("ASN processing error %r", rc);
        return TE_RC(TE_TAD_CSAP, rc); 
    }
    else
    {
        eth_spec_data->local_addr = malloc(ETH_ALEN);
        if (eth_spec_data->local_addr == NULL)
        {
            free_eth_csap_data(eth_spec_data,ETH_COMPLETE_FREE);
            ERROR("Init, no mem");
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM); 
        }
        memcpy (eth_spec_data->local_addr, local_addr, ETH_ALEN);
    }
    
    /* setting ethernet type */
    val_len = sizeof(eth_type); 
    
    rc = asn_read_value_field(eth_csap_spec, &eth_type, &val_len, "eth-type");
    
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) 
    {
        eth_spec_data->eth_type = DEFAULT_ETH_TYPE;
        rc = 0;
    }
    else if (rc != 0)
    {
        ERROR("ASN processing error %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    else 
    {
        eth_spec_data->eth_type = eth_type;
    }    

    /* Read CFI */
    val_len = sizeof(eth_spec_data->cfi);
    rc = asn_read_value_field(eth_csap_spec, &eth_spec_data->cfi, 
            &val_len, "cfi");
    if (rc)
        eth_spec_data->cfi = -1;

    /* Read VLAN ID */
    val_len = sizeof(eth_spec_data->vlan_id);
    rc = asn_read_value_field(eth_csap_spec, &eth_spec_data->vlan_id, 
            &val_len, "vlan-id");
    if (rc)
        eth_spec_data->vlan_id = -1;

    /* Read priority */
    val_len = sizeof(eth_spec_data->priority);
    rc = asn_read_value_field(eth_csap_spec, &eth_spec_data->priority, 
            &val_len, "priority");
    if (rc)
        eth_spec_data->priority = -1;

    /* default read timeout */
    eth_spec_data->read_timeout = ETH_CSAP_DEFAULT_TIMEOUT; 

    if (csap_descr->check_pdus_cb == NULL)
        csap_descr->check_pdus_cb = tad_eth_single_check_pdus;

    csap_descr->layers[layer].specific_data = eth_spec_data;

    csap_descr->read_cb          = tad_eth_read_cb;
    csap_descr->write_cb         = tad_eth_write_cb;
    csap_descr->write_read_cb    = tad_eth_write_read_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 500000;
    csap_descr->prepare_recv_cb  = eth_prepare_recv;
    csap_descr->prepare_send_cb  = eth_prepare_send;
    csap_descr->release_cb       = eth_release;
    csap_descr->echo_cb          = eth_echo_method;

    csap_descr->layers[layer].get_param_cb = tad_eth_get_param_cb; 
    
    return 0;
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_single_destroy_cb(csap_p csap_descr, unsigned int layer)
{
    VERB("CSAP N %d", csap_descr->id);

    eth_csap_specific_data_p spec_data = 
        (eth_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    if (spec_data == NULL)
    {
        WARN("Not ethernet CSAP %d special data found!", csap_descr->id);
        return 0;
    }

    eth_free_interface(spec_data->interface);

    if(spec_data->in >= 0)
    {
        RING("%s(CSAP %d), close in socket %d",
             __FUNCTION__, csap_descr->id, spec_data->in);
        if (close_packet_socket(spec_data->interface->name, spec_data->in) < 0)
            assert(0);
        spec_data->in = -1;
    }
    
    if (spec_data->out >= 0)
    {
        RING("%s(CSAP %d), close out socket %d",
             __FUNCTION__, csap_descr->id, spec_data->out);
        if (close_packet_socket(spec_data->interface->name, spec_data->out) < 0)
            assert(0);
        spec_data->out = -1;
    }

    free_eth_csap_data(spec_data, ETH_COMPLETE_FREE);

    csap_descr->layers[layer].specific_data = NULL; 
    return 0;
}


/**
 * Ethernet echo CSAP method. 
 * Method should prepare binary data to be send as "echo" and call 
 * respective write method to send it. 
 * Method may change data stored at passed location.  
 *
 * @param csap_descr    identifier of CSAP
 * @param pkt           Got packet, plain binary data. 
 * @param len           length of data.
 *
 * @return zero on success or error code.
 */
int 
eth_echo_method(csap_p csap_descr, uint8_t *pkt, size_t len)
{
    uint8_t tmp_buffer [10000]; 

    if (csap_descr == NULL || pkt == NULL || len == 0)
        return TE_EINVAL;
#if 1
    memcpy (tmp_buffer, pkt + ETH_ALEN, ETH_ALEN);
    memcpy (tmp_buffer + ETH_ALEN, pkt, ETH_ALEN);
    memcpy (tmp_buffer + 2*ETH_ALEN, pkt + 2*ETH_ALEN, len - 2*ETH_ALEN);

#if 0
    /* Correction for number of read bytes was insered to synchronize 
     * with OS interface statistics, but it cause many side effects, 
     * therefore it is disabled now. */

    tad_eth_write_cb(csap_descr, tmp_buffer, len - ETH_TAILING_CHECKSUM);
#else
    tad_eth_write_cb(csap_descr, tmp_buffer, len);
#endif


#endif

    return 0;
}
