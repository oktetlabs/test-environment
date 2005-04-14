/** @file
 * @brief Test Environment: 
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
 * $Id: $
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
#include "tad_pcap_impl.h"
#include "ndn_pcap.h" 

#define TE_LGR_USER     "TAD Ethernet-PCAP"
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

static iface_user_rec iface_users_root =
    {&iface_users_root, &iface_users_root, {0,},0};

static iface_user_rec *
find_iface_user_rec(const char *ifname)
{
    iface_user_rec *ir;

    for (ir = iface_users_root.next; ir != &iface_users_root; ir = ir->next)
        if (strncmp(ir->name, ifname, IFNAME_SIZE) == 0)
            return ir;

    return NULL;
}


#define USE_PROMISC_MODE 1

static int open_packet_socket(int pkt_type, int if_index, int *sock);

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
pcap_release(csap_p csap_descr)
{
    eth_csap_specific_data_p spec_data;
    int layer;

    layer = csap_descr->read_write_layer; 
    spec_data = (pcap_csap_specific_data_p)
                 csap_descr->layers[layer].specific_data; 

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
    

    layer = csap_descr->read_write_layer;
    
    spec_data =
        (pcpa_csap_specific_data_p) csap_descr->layers[layer].specific_data;
    
    VERB("Before opened Socket %d", spec_data->in);

    /* opening incoming socket */
    if ((rc = open_packet_socket(PACKET_HOST,
                                spec_data->interface->if_index,
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

    layer = csap_descr->read_write_layer;
    
    spec_data =
        (pcap_csap_specific_data_p) csap_descr->layers[layer].specific_data;
   
    /* outgoing socket */
    if ((rc = open_packet_socket(PACKET_OTHERHOST,
                                spec_data->interface->if_index,
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
 * Find ethernet interface by its name and initialize specified
 * structure with interface parameters
 *
 * @param name      symbolic name of interface to find (e.g. eth0, eth1)
 * @param iface     pointer to interface structure to be filled
 *                  with found parameters (OUT)
 *
 * @return ETH_IFACE_OK on success or one of the error codes
 *
 * @retval ETH_IFACE_OK            on success
 * @retval ETH_IFACE_NOT_FOUND     if config socket error occured or interface 
 *                                 can't be found by specified name
 * @retval ETH_IFACE_HWADDR_ERROR  if hardware address can't be extracted   
 * @retval ETH_IFACE_IFINDEX_ERROR if interface index can't be extracted
 */
int
pcap_find_interface(char *name, pcap_csap_interface_p iface) 
{
    char err_buf[200];
    int    cfg_socket;
    struct ifreq  if_req;
    int rc;

    if (name == NULL) 
    {
       return EINVAL;
    }

    VERB("%s('%s') start", __FUNCTION__, name);

    if ((cfg_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        return errno;

    memset(&if_req, 0, sizeof(if_req));
    strcpy(if_req.ifr_name, name);

    if (ioctl(cfg_socket, SIOCGIFHWADDR, &if_req))
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("get if addr error %d \"%s\"", rc, err_buf);
        return rc;
    }

    memcpy(iface->local_addr, if_req.ifr_hwaddr.sa_data, ETH_ALEN);
    strcpy(if_req.ifr_name, name);

    if (ioctl(cfg_socket, SIOCGIFINDEX, &if_req))
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("get if index error %d \"%s\"", rc, err_buf);
        return rc;
    }
    
    /* saving if index  */
    iface->if_index = if_req.ifr_ifindex;
    
    /* saving name     */
    strncpy (iface->name, name, (IFNAME_SIZE - 1));
    iface->name[IFNAME_SIZE - 1] = '\0';

#if USE_PROMISC_MODE

    memset(&if_req, 0, sizeof(if_req));
    strcpy(if_req.ifr_name, name);

    if (ioctl(cfg_socket, SIOCGIFFLAGS, &if_req) < 0)
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("get if flags error %d \"%s\"", rc, err_buf);
        return rc;
    }

    VERB("SIOCGIFFLAGS, promisc mode: %d, flags: %x\n", 
                (int)(if_req.ifr_flags & IFF_PROMISC), (int)if_req.ifr_flags);

    /* set interface to promiscuous mode */
    VERB("set to promisc mode");
    if_req.ifr_flags |= IFF_PROMISC;

    if (ioctl(cfg_socket, SIOCSIFFLAGS, &if_req))
    { 
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("set PROMISC mode error %d \"%s\"", rc, err_buf);
        return rc;
    }
#endif
    
    close(cfg_socket);

    {
        iface_user_rec *ir;

        ir = find_iface_user_rec(name);
        if (ir == NULL)
        {
            ir = calloc(1, sizeof(iface_user_rec));

            if (ir == NULL)
                return ENOMEM; 
            strncpy(ir->name, name, IFNAME_SIZE);
            INSQUE(ir, &iface_users_root); 
        }

        ir->num_users++; 
    }
    return ETH_IFACE_OK;
} 


/**
 * Find number of Ethernet-PCAP layer in CSAP stack.
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
    free(spec_data->interface);
    
    if (spec_data->remote_addr != NULL)
       free(spec_data->remote_addr);
    
    if (spec_data->local_addr != NULL)
       free(spec_data->local_addr);   
       
    if (is_complete)
       free(spec_data);   
}

/**
 * Create and bind packet socket to listen specified interface
 *
 * @param pkt_type  Type of packet socket (PACKET_HOST, PACKET_OTHERHOST,
 *                  PACKET_OUTGOING
 * @param if_index  interface index
 * @param sock      pointer to place where socket handler will be saved
 *
 * @retval 0 on succees, -1 on fail
 */
static int 
open_packet_socket(int pkt_type, int if_index, int *sock)
{
    struct sockaddr_ll  bind_addr;
    int rc;
    char err_buf[200];

    UNUSED(pkt_type);

    memset (&bind_addr, 0, sizeof(bind_addr));

    if (sock == NULL)
    {
        ERROR("Location for socket is NULL");
        return EINVAL;
    }

    /* Create packet socket */
    *sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    VERB("OPEN SOCKET (%d): %s:%d", *sock,
               __FILE__, __LINE__);

    VERB("socket system call returns %d\n", *sock);

    if (*sock < 0)
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("Socket creation failed, errno %d, \"%s\"\n", 
                rc, err_buf);
        return rc;
    }

    bind_addr.sll_family = AF_PACKET;
    bind_addr.sll_protocol = htons(ETH_P_ALL);
    bind_addr.sll_ifindex = if_index;
    bind_addr.sll_hatype = ARPHRD_ETHER;
    bind_addr.sll_pkttype = pkt_type;
    bind_addr.sll_halen = ETH_ALEN;

    VERB("RAW Socket opened");

#if 0
    switch (dir)
    {
        case IN_PACKET_SOCKET:
            bind_addr.sll_pkttype = pkt_type;
            shutdown_method = SHUT_WR;
            break;
        case OUT_PACKET_SOCKET:
            shutdown_method = SHUT_RD;
            break;
    }

    if (shutdown (*sock, shutdown_method) < 0)
    {
        perror ("ETH packet socket shutdown");
        return errno;
    }
#endif

    if (bind(*sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0)
    {
        rc = errno;
        strerror_r(rc, err_buf, sizeof(err_buf)); 
        ERROR("Socket bind failed, errno %d, \"%s\"\n", 
                rc, err_buf);

        VERB("CLOSE SOCKET (%d): %s:%d", 
                *sock, __FILE__, __LINE__);
        if (close(*sock) < 0)
            assert(0);

        *sock = -1;
        return rc;
    }

    return 0;
}

/**
 * Callback for read data from media of Ethernet-PCAP CSAP. 
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
pcap_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    pcap_csap_specific_data_p   spec_data;
    int                         ret_val;
    int                         layer;
    fd_set                      read_set;
    struct sockaddr_ll          from;
    socklen_t                   fromlen = sizeof(from);
    struct timeval              timeout_val;
    int                         pkt_size;

    if (csap_descr == NULL)
    {
        csap_descr->last_errno = EINVAL;
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

    return pkt_size;
}


int 
eth_single_check_pdus(csap_p csap_descr, asn_value *traffic_nds)
{
    char choice_label[20];
    int rc;

    UNUSED(csap_descr);

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
 * Callback for init Ethernet-PCAP CSAP layer  if single in stack.
 *
 * @param csap_descr       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
pcap_single_init_cb (int csap_id, const asn_value *csap_nds, int layer)
{
    int      rc; 
    char     choice[100] = "";
    char     device_id[IFNAME_SIZE]; /**< ethernet interface id
                                          (e.g. eth0, eth1) */
    size_t   val_len;                /**< stores corresponding value length */
    
    pcap_csap_interface_p iface_p;   /**< pointer to interface structure to be
                                          used with csap */
    csap_p   csap_descr;             /**< csap description */

    pcap_csap_specific_data_p   pcap_spec_data; 
    const asn_value_p           pcap_csap_spec; /**< ASN value with csap init
                                                     parameters  */
                        
    int                         eth_type;      /**< Ethernet type                          */
    char                        str_index_buf[10];
    

    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, ETEWRONGPTR);

    if ((csap_descr = csap_find (csap_id)) == NULL)
        return TE_RC(TE_TAD_CSAP, ETADCSAPNOTEX);

    /* TODO correct with Read-only get_indexed method */
    sprintf (str_index_buf, "%d", layer);
    rc = asn_get_subvalue(csap_nds, &pcap_csap_spec, str_index_buf);
    if (rc)
    {
        val_len = asn_get_length(csap_nds, "");
        ERROR("pcap_single_init_cb called for csap %d, layer %d,"
              "rc %X getting '%s', ndn has len %d\n", 
              csap_id, layer, rc, str_index_buf, val_len);
        return TE_RC(TE_TAD_CSAP, EINVAL);
    }

    rc = asn_get_choice(pcap_csap_spec, "", choice, sizeof(choice));
    VERB("eth_single_init_cb called for csap %d, layer %d, ndn with type %s\n", 
                csap_id, layer, choice);
    
    val_len = sizeof(device_id);
    rc = asn_read_value_field(pcap_csap_spec, device_id, &val_len, "device-id");
    if (rc) 
    {
        F_ERROR("device-id for ethernet not found: %x\n", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    
    /* allocating new interface structure and trying to init by name */    
    iface_p = malloc(sizeof(pcap_csap_interface_t));
    
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
                break;
        }
        free(iface_p);        
        return TE_RC(TE_TAD_CSAP, rc); 
    }

    pcap_spec_data = calloc (1, sizeof(pcap_csap_specific_data_t));
    if (pcap_spec_data == NULL)
    {
        free(iface_p);
        ERROR("Init, not memory for spec_data");
        return TE_RC(TE_TAD_CSAP,  ENOMEM);
    }
    
    pcap_spec_data->in = -1;
    pcap_spec_data->out = -1;

    /* setting default interface    */
    pcap_spec_data->interface = iface_p;

    
    val_len = sizeof(pcap_spec_data->recv_mode);
    rc = asn_read_value_field(pcap_csap_spec, &pcap_spec_data->recv_mode, 
                              &val_len, "receive-mode");
    if (rc != 0)
    {
        pcap_spec_data->recv_mode = ETH_RECV_MODE_DEF;
    }

    pcap_spec_data->eth_type = DEFAULT_ETH_TYPE;

    /* default read timeout */
    eth_spec_data->read_timeout = ETH_CSAP_DEFAULT_TIMEOUT; 

    if (csap_descr->check_pdus_cb == NULL)
        csap_descr->check_pdus_cb = pcap_single_check_pdus;

    csap_descr->layers[layer].specific_data = pcap_spec_data;

    csap_descr->read_cb          = pcap_read_cb;
    csap_descr->write_cb         = NULL;
    csap_descr->write_read_cb    = NULL;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 500000;
    csap_descr->prepare_recv_cb  = pcap_prepare_recv;
    csap_descr->prepare_send_cb  = pcap_prepare_send;
    csap_descr->release_cb       = pcap_release;
    csap_descr->echo_cb          = pcap_echo_method;

    csap_descr->layers[layer].get_param_cb = pcap_get_param_cb; 
    
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

#if USE_PROMISC_MODE
    do {
        iface_user_rec *ir;
        struct ifreq  if_req;
        int cfg_socket;

        VERB("unset to promisc mode iface %s",
                spec_data->interface->name);

        ir = find_iface_user_rec(spec_data->interface->name);
        VERB("found iface_user_rec %x", ir);

        if (ir == NULL || ir->num_users <= 1)
        {
            if ((cfg_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                break;

            VERB("unset promisc mode. do it. !!!", ir);

            memset(&if_req, 0, sizeof(if_req));
            strcpy(if_req.ifr_name, spec_data->interface->name);

            if (ioctl(cfg_socket, SIOCGIFFLAGS, &if_req) < 0)
            {
                ERROR("get flags of interface failed, errno %d", errno);
                break;
            }

            /* reset interface from promiscuous mode */
            if_req.ifr_flags &= ~IFF_PROMISC;

            if (ioctl(cfg_socket, SIOCSIFFLAGS, &if_req))
            { 
                ERROR("unset promisc mode failed %x\n", errno);
                break;
            }
            REMQUE(ir);
            free(ir);
        }
        else
            ir->num_users--;
    } while (0);
#endif

    if(spec_data->in >= 0)
    {
        VERB("csap # %d, CLOSE SOCKET (%d): %s:%d",
                   csap_descr->id, spec_data->in, __FILE__, __LINE__);
        if (close(spec_data->in) < 0)
            assert(0);
        spec_data->in = -1;
    }
    
    if (spec_data->out >= 0)
        close(spec_data->out);    

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


