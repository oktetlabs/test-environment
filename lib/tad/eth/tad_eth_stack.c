/** @file
 * @brief Test Environment: 
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
 * Author: Andrew Duka <duke@oktetlabs.ru>
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */

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



#define TE_LGR_USER     "TAD Ethernet"
#include "logger_ta.h"



#ifndef INSQUE
/* macros to insert element p into queue _after_ element q */
#define INSQUE(p, q) do {(p)->prev = q; (p)->next = (q)->next; \
                      (q)->next = p; (p)->next->prev = p; } while (0)
/* macros to remove element p from queue  */
#define REMQUE(p) do {(p)->prev->next = (p)->next; (p)->next->prev = (p)->prev; \
                   (p)->next = (p)->prev = p; } while(0)
#endif

                   

typedef struct iface_user_rec
{
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
        VERB("%s: CSAP %d, close input socket %d", 
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
        VERB("%s: CSAP %d, close output socket %d", 
             __FUNCTION__, csap_descr->id, spec_data->out);
        if (close_packet_socket(spec_data->interface->name, spec_data->out) < 0)
        {
            csap_descr->last_errno = errno;
            perror("close output socket");
            WARN("%s: CLOSE output socket error %d", 
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
    
    spec_data = (eth_csap_specific_data_p) csap_descr->layers[layer].specific_data; 
    
    VERB("Before opened Socket %d", spec_data->in);

    /* opening incoming socket */
    if ((rc = open_packet_socket(spec_data->interface->name,
                                 &spec_data->in)) != 0)
    {
        ERROR("open_packet_socket error %d", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }

    VERB("csap %d Opened Socket %d", 
                 csap_descr->id, spec_data->in);

    buf_size = 0x100000; 
    /* TODO: reasonable size of receive buffer to be investigated */
    if (setsockopt(spec_data->in, SOL_SOCKET, SO_RCVBUF,
                &buf_size, sizeof(buf_size)) < 0)
    {
        perror ("set RCV_BUF failed");
        fflush (stderr);
    }

    return 0;
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
   
    /* outgoing socket */
    if ((rc = open_packet_socket(spec_data->interface->name,
                                 &spec_data->out)) != 0)
    { 
        ERROR("open_packet_socket error %d", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }

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
 * Find number of ethernet layer in CSAP stack.
 *
 * @param csap_descr    CSAP description structure.
 * @param layer_name    Name of the layer to find.
 *
 * @return number of layer (start from zero) or -1 if not found. 
 */ 
int 
find_csap_layer(csap_p csap_descr, char *layer_name)
{
    int i; 
    for (i = 0; i < csap_descr->depth; i++)
        if (strcmp(csap_descr->layers[i].proto, layer_name) == 0)
            return i;
    return -1;
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



/**
 * Callback for read data from media of ethernet CSAP. 
 *
 * @param csap_descr    identifier of CSAP.
 * @param timeout       timeout of waiting for data in microseconds.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
eth_read_cb (csap_p csap_descr, int timeout, char *buf, size_t buf_len)
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
        F_ERROR("%s called with NULL csap_descr", __FUNCTION__);
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (eth_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    gettimeofday(&timeout_val, NULL);
    F_VERB("%s(CSAP %d): spec_data->in = %d, mcs %d",
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
    F_VERB("%s(): CSAP %d read %d bytes, pkttype %d, mcs %d, begin 0x%x, ethtype %x", 
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

/**
 * Callback for write data to media of ethernet CSAP. 
 *
 * @param csap_descr       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
eth_write_cb(csap_p csap_descr, char *buf, size_t buf_len)
{
    int ret_val = 0;
    eth_csap_specific_data_p spec_data;
    int layer;    
    fd_set write_set;
    int retries = TAD_WRITE_RETRIES;

   
    if (csap_descr == NULL)
    {
        csap_descr->last_errno = EINVAL;
        F_ERROR("%s: no csap descr", __FUNCTION__);
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (eth_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    F_VERB("%s: writing data to socket: %d", __FUNCTION__, spec_data->out);

    if(spec_data->out < 0)
    {
        F_ERROR("%s: no output socket", __FUNCTION__);
        csap_descr->last_errno = EINVAL;
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
            F_INFO("ETH write select timedout, retry %d\n", retries);
            continue;
        }

        if (ret_val == 1)
            ret_val = write(spec_data->out, buf, buf_len);

        if (ret_val < 0)
        {
            csap_descr->last_errno = errno;
            F_VERB("CSAP #%d, errno %d, retry %d",
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
                    F_ERROR("%s: internal socket error %d", __FUNCTION__, 
                            csap_descr->last_errno);
                    return -1;
            }
        } 
    }
    if (retries == TAD_WRITE_RETRIES)
    {
        F_ERROR("csap #%d, too many retries made, failed");
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

/**
 * Callback for write data to media of ethernet CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap_descr       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param w_buf         buffer with data to be written.
 * @param w_buf_len     length of data in w_buf.
 * @param r_buf         buffer for data to be read.
 * @param r_buf_len     available length r_buf.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
eth_write_read_cb (csap_p csap_descr, int timeout,
                   char *w_buf, size_t w_buf_len,
                   char *r_buf, size_t r_buf_len)
{
    int ret_val; 

    ret_val = eth_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (ret_val == -1)  
        return ret_val;
    else 
        return eth_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}


int 
eth_single_check_pdus(csap_p csap_descr, asn_value *traffic_nds)
{
    char choice_label[20];
    int rc;

    RING("%s(CSAP %d) called", __FUNCTION__, csap_descr->id);

    rc = asn_get_choice(traffic_nds, "pdus.0", choice_label, 
                        sizeof(choice_label));

    if (rc && rc != EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    if (rc == EASNINCOMPLVAL)
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
        return ETADWRONGNDS;
    }
    return 0;
}
/**
 * Callback for init ethernet CSAP layer  if single in stack.
 *
 * @param csap_descr       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
eth_single_init_cb (int csap_id, const asn_value *csap_nds, int layer)
{
    int      rc; 
    char     device_id[IFNAME_SIZE]; /**< ethernet interface id (e.g. eth0, eth1)      */
    char     local_addr[ETH_ALEN];   /**< local ethernet address                       */
    char     remote_addr[ETH_ALEN];  /**< remote ethernet address                      */    
    size_t   val_len;                /**< stores corresponding value length            */
    
    eth_interface_p iface_p; /**< pointer to interface structure to be used with csap */
    csap_p   csap_descr;          /**< csap description                                    */

    eth_csap_specific_data_p    eth_spec_data; 
    const asn_value            *eth_csap_spec; 
                        /**< ASN value with csap init parameters    */
    int                         eth_type;      /**< Ethernet type                          */
    char     str_index_buf[10];
    
    RING("%s called for csap %d, layer %d",
         __FUNCTION__, csap_id, layer); 

    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, ETEWRONGPTR);

    if ((csap_descr = csap_find (csap_id)) == NULL)
        return TE_RC(TE_TAD_CSAP, ETADCSAPNOTEX);

    /* TODO correct with Read-only get_indexed method */
    sprintf (str_index_buf, "%d", layer);
    rc = asn_get_subvalue(csap_nds, &eth_csap_spec, str_index_buf);
    if (rc)
    {
        val_len = asn_get_length(csap_nds, "");
        ERROR("%s(CSAP %d), layer %d, rc %X getting '%s', ndn has len %d", 
              __FUNCTION__, csap_id, layer, rc, str_index_buf, val_len);
        return TE_RC(TE_TAD_CSAP, EINVAL);
    }

    
    val_len = sizeof(device_id);
    rc = asn_read_value_field(eth_csap_spec, device_id, &val_len, "device-id");
    if (rc) 
    {
        F_ERROR("device-id for ethernet not found: %x\n", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    
    /* allocating new interface structure and trying to init by name */    
    iface_p = malloc(sizeof(eth_interface_t));
    
    if (iface_p == NULL)
    {
#ifdef TALOGDEBUG
        printf("Can't allocate memory for interface");
#endif    
        return TE_RC(TE_TAD_CSAP, ENOMEM);
    }
    
    if ((rc = eth_find_interface(device_id, iface_p)) != ETH_IFACE_OK)
    {
        switch (rc) 
        {
            case ETH_IFACE_HWADDR_ERROR:
                ERROR("Get iface <%s> hwaddr error", device_id);
                rc = errno; /* TODO: correct rc */
                break;
            case ETH_IFACE_IFINDEX_ERROR:
                ERROR("Interface index for <%s> could not get", 
                        device_id);
                rc = errno; /* TODO: correct rc */ 
                break;
            case ETH_IFACE_NOT_FOUND:
                rc = ENODEV;
                ERROR("Interface <%s> not found", device_id);
                break;
            default:
                ERROR("Interface <%s> config failure %x", 
                        device_id, rc);
        }
        free(iface_p);        
        return TE_RC(TE_TAD_CSAP, rc); 
    }

    eth_spec_data = calloc (1, sizeof(eth_csap_specific_data_t));
    
    if (eth_spec_data == NULL)
    {
        free(iface_p);
        ERROR("Init, not memory for spec_data");
        return TE_RC(TE_TAD_CSAP,  ENOMEM);
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
    
    if (rc == EASNINCOMPLVAL) 
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
            return TE_RC(TE_TAD_CSAP, ENOMEM); 
        }
        memcpy (eth_spec_data->remote_addr, remote_addr, ETH_ALEN);
    }

    /* setting local address */
    
    val_len = sizeof(local_addr); 
    rc = asn_read_value_field(eth_csap_spec, local_addr, &val_len, "local-addr");
    
    if (rc == EASNINCOMPLVAL) 
        eth_spec_data->local_addr = NULL;
    else if (rc)
    {
        free_eth_csap_data(eth_spec_data, ETH_COMPLETE_FREE);
        ERROR("ASN processing error %x", rc);
        return TE_RC(TE_TAD_CSAP, rc); 
    }
    else
    {
        eth_spec_data->local_addr = malloc(ETH_ALEN);
        if (eth_spec_data->local_addr == NULL)
        {
            free_eth_csap_data(eth_spec_data,ETH_COMPLETE_FREE);
            ERROR("Init, no mem");
            return TE_RC(TE_TAD_CSAP, ENOMEM); 
        }
        memcpy (eth_spec_data->local_addr, local_addr, ETH_ALEN);
    }
    
    /* setting ethernet type */
    val_len = sizeof(eth_type); 
    
    rc = asn_read_value_field(eth_csap_spec, &eth_type, &val_len, "eth-type");
    
    if (rc == EASNINCOMPLVAL) 
    {
        eth_spec_data->eth_type = DEFAULT_ETH_TYPE;
        rc = 0;
    }
    else if (rc != 0)
    {
        ERROR("ASN processing error %x", rc);
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
        csap_descr->check_pdus_cb = eth_single_check_pdus;

    csap_descr->layers[layer].specific_data = eth_spec_data;

    csap_descr->read_cb         = eth_read_cb;
    csap_descr->write_cb        = eth_write_cb;
    csap_descr->write_read_cb   = eth_write_read_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 500000;
    csap_descr->prepare_recv_cb  = eth_prepare_recv;
    csap_descr->prepare_send_cb  = eth_prepare_send;
    csap_descr->release_cb       = eth_release;
    csap_descr->echo_cb          = eth_echo_method;

    csap_descr->layers[layer].get_param_cb = eth_get_param_cb; 
    
    return 0;
}

/**
 * Callback for destroy ethernet CSAP layer  if single in stack.
 *
 * This callback should free all undeground media resources used by 
 * this layer and all memory used for layer-specific data and pointed 
 * in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
eth_single_destroy_cb (int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    VERB("CSAP N %d", csap_id);

    eth_csap_specific_data_p spec_data = 
        (eth_csap_specific_data_p) csap_descr->layers[layer].specific_data; 

    if (spec_data == NULL)
    {
        WARN("Not ethernet CSAP %d special data found!", csap_id);
        return 0;
    }

    eth_free_interface(spec_data->interface);

    if(spec_data->in >= 0)
    {
        VERB("csap # %d, CLOSE SOCKET (%d): %s:%d",
                   csap_descr->id, spec_data->in, __FILE__, __LINE__);
        if (close_packet_socket(spec_data->interface->name, spec_data->in) < 0)
            assert(0);
        spec_data->in = -1;
    }
    
    if (spec_data->out >= 0)
        if (close_packet_socket(spec_data->interface->name, spec_data->out) < 0)
            assert(0);

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
eth_echo_method (csap_p csap_descr, uint8_t *pkt, size_t len)
{
    uint8_t tmp_buffer [10000]; 

    if (csap_descr == NULL || pkt == NULL || len == 0)
        return EINVAL;
#if 1
    memcpy (tmp_buffer, pkt + ETH_ALEN, ETH_ALEN);
    memcpy (tmp_buffer + ETH_ALEN, pkt, ETH_ALEN);
    memcpy (tmp_buffer + 2*ETH_ALEN, pkt + 2*ETH_ALEN, len - 2*ETH_ALEN);

#if 0
    /* Correction for number of read bytes was insered to synchronize 
     * with OS interface statistics, but it cause many side effects, 
     * therefore it is disabled now. */

    eth_write_cb (csap_descr, tmp_buffer, len - ETH_TAILING_CHECKSUM);
#else
    eth_write_cb (csap_descr, tmp_buffer, len);
#endif


#endif

    return 0;
}


