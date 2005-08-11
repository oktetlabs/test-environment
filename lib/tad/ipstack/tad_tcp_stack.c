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
 * Author: Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
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
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include "tad_ipstack_impl.h"


int 
tcp_ip4_check_pdus(csap_p csap_descr, asn_value *traffic_nds)
{
    UNUSED(csap_descr);
    UNUSED(traffic_nds);
    VERB("%s(CSAP %d) called", __FUNCTION__, csap_descr->id);

    return 0;
}

 
/**
 * Callback for read data from media of tcpernet CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data in microseconds.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
tcp_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    int    rc; 
    int    layer;    
    int    recv_flags = 0;
    fd_set read_set;

    tcp_csap_specific_data_t *spec_data; 
    struct timeval            timeout_val; 
    
    layer = csap_descr->read_write_layer;
    
    spec_data = csap_descr->layers[layer].specific_data; 


    if(spec_data->socket < 0)
    {
        return -1;
    }

    if (spec_data->wait_length > 0)
    {
        if (buf_len < spec_data->wait_length)
        {
            csap_descr->last_errno = TE_ESMALLBUF;
            ERROR("%s(CSAP %d) wait buf passed %d, but buf_len %d",
                  __FUNCTION__, csap_descr->id,
                  spec_data->wait_length, buf_len);
            return -1;
        }
        else
        {
            buf_len = spec_data->wait_length;
            recv_flags |= MSG_WAITALL;
        }
    }
    FD_ZERO(&read_set);
    FD_SET(spec_data->socket, &read_set);

    if (timeout == 0)
    {
        timeout_val.tv_sec = 0;
        timeout_val.tv_usec = 100 * 1000; /* 0.1 sec */
    }
    else
    {
        timeout_val.tv_sec = timeout / 1000000L; 
        timeout_val.tv_usec = timeout % 1000000L;
    }
    
    rc = select(spec_data->socket + 1, &read_set, NULL, NULL,
                &timeout_val); 
    
    if (rc == 0)
        return 0;

    if (rc < 0)
        return -1;

    if (spec_data->data_tag == NDN_TAG_TCP_DATA_SERVER)
    {
        int acc_sock = accept(spec_data->socket, NULL, NULL);

        if (acc_sock < 0)
        {
            csap_descr->last_errno = errno;
            return -1;
        }
        RING("%s(CSAP %d) TCP 'server', accepted socket %d", 
             __FUNCTION__, csap_descr->id, acc_sock);
        *((int *)buf) = acc_sock;

        return sizeof(int);
    } 
    else 
        /* Note: possibly MSG_TRUNC and other flags are required */
        return recv(spec_data->socket, buf, buf_len, recv_flags); 
}

/**
 * Callback for write data to media of tcpernet CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
tcp_write_cb(csap_p csap_descr, char *buf, size_t buf_len)
{
    tcp_csap_specific_data_t * spec_data;
    int layer;    
    int rc;

    if (csap_descr == NULL)
        return -1;

    layer = csap_descr->read_write_layer;
    
    spec_data = csap_descr->layers[layer].specific_data; 

    if(spec_data->socket < 0)
        return -1;

    if (spec_data->data_tag == NDN_TAG_TCP_DATA_SERVER)
    {
        ERROR("%s(CSAP %d) write to TCP data 'server' is not allowed",
              __FUNCTION__, csap_descr->id);
        csap_descr->last_errno = TE_ETADLOWER;
        return -1;
    }

    rc = send(spec_data->socket, buf, buf_len, 0); 
    if (rc < 0) 
    {
        perror("tcp sendto fail");
        csap_descr->last_errno = errno;
    }

    return rc;
}

/**
 * Callback for write data to media of tcpernet CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap_id       identifier of CSAP.
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
tcp_write_read_cb(csap_p csap_descr, int timeout,
                  char *w_buf, size_t w_buf_len,
                  char *r_buf, size_t r_buf_len)
{
    int rc; 

    rc = tcp_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (rc == -1)  
        return rc;
    else 
        return tcp_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}

/**
 * Callback for init tcp CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
tcp_single_init_cb(int csap_id, const asn_value *csap_nds, int layer)
{
#if 0
    csap_p   csap_descr;          /**< csap description        */

    tcp_csap_specific_data_t *   tcp_spec_data; 
    struct sockaddr_in local;
    struct sockaddr_in *ifa;

    int             opt = 1;
    int             mode, len, rc;
    struct ifreq   *interface = calloc(sizeof(struct ifreq) + 
                                       sizeof(struct sockaddr_storage) - 
                                       sizeof(struct sockaddr), 1); 

    fprintf(stderr, "DHCP INIT called\n");
    if (csap_nds == NULL)
        return TE_EWRONGPTR;

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_ETADCSAPNOTEX;


    len = sizeof(mode);
    rc = asn_read_value_field(csap_nds, &mode, &len, "0.mode");
    if (rc)
        return rc; /* If this field is not set, then CSAP cannot process */ 

    tcp_spec_data = malloc(sizeof(tcp_csap_specific_data_t));
    
    if (tcp_spec_data == NULL)
    {
        return ENOMEM;
    }
    
    
    tcp_spec_data->ipaddr = malloc(INET_ADDRSTRLEN + 1);

    /* opening incoming socket */
    tcp_spec_data->in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
    if (tcp_spec_data->in < 0)
    {
        return errno;
    }
    if (setsockopt(tcp_spec_data->in, SOL_SOCKET, SO_REUSEADDR, 
                   (void *) &opt, sizeof(opt)) == -1)
    {
        return errno;
    }


    len = IFNAMSIZ;
    rc = asn_read_value_field(csap_nds, interface->ifr_ifrn.ifrn_name, &len, 
                                "0.iface");
    if (rc == 0)
    {
        if (setsockopt(tcp_spec_data->in, SOL_SOCKET, SO_BINDTODEVICE,
                       (char *)interface, sizeof(struct ifreq)) < 0) 
        {
            perror("setsockopt, BINDTODEVICE");
            rc  = errno;
        }
    }
    else if (rc == TE_EASNINCOMPLVAL) 
    {
        rc = 0;
    }

    if (rc)
    {
        tcp_single_destroy_cb(csap_id, layer);
        return rc;
    }

    if (setsockopt(tcp_spec_data->in, SOL_SOCKET, SO_BROADCAST, 
                   (void *) &opt, sizeof(opt)) == -1)
    {
        tcp_single_destroy_cb(csap_id, layer);
        return errno;
    }

    local.sin_family = AF_INET;
    local.sin_port = htons(mode == DHCPv4_CSAP_mode_server ? 
                                        DHCP_SERVER_PORT : DHCP_CLIENT_PORT); 
    local.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcp_spec_data->in, &local, sizeof(local)) == -1)
    {
        perror("tcp csap socket bind");
        return errno;
    }


    /* opening outgoing socket */
    tcp_spec_data->out = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
    if (tcp_spec_data->out < 0)
    {
        return errno;
    }
    if (setsockopt(tcp_spec_data->out, SOL_SOCKET, SO_REUSEADDR, 
                   (void *) &opt, sizeof(opt)) == -1)
    {
        return errno;
    } 

    if (setsockopt(tcp_spec_data->out, SOL_SOCKET, SO_BINDTODEVICE,
                   (char *)interface, sizeof(struct ifreq)) < 0) 
    {
        perror("setsockopt, BINDTODEVICE");
        rc  = errno;
    }

    if (ioctl(tcp_spec_data->in, SIOCGIFADDR, interface) < 0)
    {
        perror ("ioctl get ifaddr");
        return errno;
    }
    ifa = (struct sockaddr_in *) &interface->ifr_addr;
    strncpy(tcp_spec_data->ipaddr, 
            inet_ntoa(ifa->sin_addr.s_addr), 
            INET_ADDRSTRLEN);

    if (rc)
    {
        tcp_single_destroy_cb(csap_id, layer);
        return rc;
    }

    if (setsockopt(tcp_spec_data->out, SOL_SOCKET, SO_BROADCAST, 
                   (void *) &opt, sizeof(opt)) == -1)
    {
        tcp_single_destroy_cb(csap_id, layer);
        return errno;
    }

    local.sin_addr.s_addr = ifa->sin_addr.s_addr;

    if (bind(tcp_spec_data->out, &local, sizeof(local)) == -1)
    {
        perror ("tcp csap socket bind");
        return errno;
    }

    free(interface);


    /* default read timeout */
    tcp_spec_data->read_timeout = 200000;

    csap_descr->layers[layer].specific_data = tcp_spec_data;
    csap_descr->layers[layer].get_param_cb = tcp_get_param_cb;

    csap_descr->read_cb         = tcp_read_cb;
    csap_descr->write_cb        = tcp_write_cb;
    csap_descr->write_read_cb   = tcp_write_read_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 500000;

    return 0;
#else
    UNUSED(csap_id);
    UNUSED(csap_nds);
    UNUSED(layer);
    return 0;
#endif
}

/**
 * Callback for destroy tcpernet CSAP layer  if single in stack.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed 
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
tcp_single_destroy_cb(int csap_id, int layer)
{
#if 0
    csap_p csap_descr = csap_find(csap_id);

    tcp_csap_specific_data_t * spec_data = 
        (tcp_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 
     
    if(spec_data->in >= 0)
        close(spec_data->in);    

    if(spec_data->out >= 0)
        close(spec_data->out);    
#else
    UNUSED(csap_id);
    UNUSED(layer);
#endif
    return 0;
}


/**
 * Callback for init 'tcp' CSAP layer if over 'ip4' in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
tcp_ip4_init_cb(int csap_id, const asn_value *csap_nds, int layer)
{
    csap_p                    csap_descr;      
    tcp_csap_specific_data_t *spec_data; 
    ip4_csap_specific_data_t *ip4_spec_data; 
    const asn_value          *tcp_pdu;

    int32_t value_in_pdu;
    int     rc;

    VERB("%s called for csap %d, layer %d",
         __FUNCTION__, csap_id, layer); 

    if (csap_nds == NULL)
        return TE_EWRONGPTR;

    if ((csap_descr = csap_find (csap_id)) == NULL)
        return TE_ETADCSAPNOTEX;

    spec_data = calloc (1, sizeof(tcp_csap_specific_data_t));
    
    if (spec_data == NULL)
    {
        return ENOMEM;
    }

    csap_descr->layers[layer].specific_data = spec_data;
    csap_descr->layers[layer].get_param_cb = tcp_get_param_cb;

    tcp_pdu = csap_descr->layers[layer].csap_layer_pdu;

    csap_descr->check_pdus_cb = tcp_ip4_check_pdus; 

    if (layer + 1 >= csap_descr->depth)
    {
        ERROR("%s(CSAP %d) too large layer %d!, depth %d", 
              __FUNCTION__, csap_id, layer, csap_descr->depth);
        return EINVAL;
    }

    ip4_spec_data = (ip4_csap_specific_data_t *)
        csap_descr->layers[layer + 1].specific_data;

    if (ip4_spec_data != NULL && ip4_spec_data->protocol == 0)
        ip4_spec_data->protocol = IPPROTO_TCP;

    if (csap_descr->type == TAD_CSAP_DATA)
    {
        const asn_value *data_csap_spec, *subval;
        asn_tag_class    t_class;

        csap_descr->read_cb         = tcp_read_cb;
        csap_descr->write_cb        = tcp_write_cb;
        csap_descr->write_read_cb   = tcp_write_read_cb;
        csap_descr->read_write_layer = layer; 

        rc = asn_get_child_value(tcp_pdu, &data_csap_spec, 
                                 PRIVATE, NDN_TAG_TCP_DATA);
        if (rc == TE_EASNINCOMPLVAL)
        {
            ERROR("%s(CSAP %d) data TCP csap should have 'data' spec",
                  __FUNCTION__, csap_descr->id); 
            return TE_ETADWRONGNDS;
        }
        else if (rc != 0)
        {
            ERROR("%s(CSAP %d): unexpected error reading 'data': %X", 
                  __FUNCTION__, csap_descr->id, rc); 
            return rc;
        } 

        rc = asn_get_choice_value(data_csap_spec, &subval, 
                                  &t_class, &(spec_data->data_tag));
        if (rc != 0)
        {
            ERROR("%s(CSAP %d): error reading choice of 'data': %X", 
                  __FUNCTION__, csap_descr->id, rc); 
            return rc;
        } 

        if (spec_data->data_tag == NDN_TAG_TCP_DATA_SOCKET)
        {
            asn_read_int32(subval, &(spec_data->socket), "");
            /* nothing more to do */
            return 0;
        }
    }


    /*
     * Set local port
     */
    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_LOCAL_PORT,
                               &value_in_pdu);
    if (rc == 0)
    {
        F_VERB("%s(): set TCP CSAP %d default local port to %d", 
               __FUNCTION__, csap_descr->id, value_in_pdu);
        spec_data->local_port = value_in_pdu;
    }
    else if (rc == TE_EASNINCOMPLVAL)
    {
        F_VERB("%s(): set TCP CSAP %d default local port to zero", 
               __FUNCTION__, csap_descr->id);
        spec_data->local_port = 0;
    }
    else if (rc == TE_EASNOTHERCHOICE)
    {
        F_ERROR("%s(): TCP CSAP %d, non-plain local port not supported",
                __FUNCTION__, csap_descr->id);
        return TE_EOPNOTSUPP;
    }
    else
        return rc;

    /*
     * Set remote port
     */
    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_REMOTE_PORT,
                               &value_in_pdu);
    if (rc == 0)
    {
        F_VERB("%s(): set TCP CSAP %d default remote port to %d", 
               __FUNCTION__, csap_descr->id, value_in_pdu);
        spec_data->remote_port = value_in_pdu;
    }
    else if (rc == TE_EASNINCOMPLVAL)
    {
        F_VERB("%s(): set TCP CSAP %d default remote port to zero", 
               __FUNCTION__, csap_descr->id);
        spec_data->remote_port = 0;
    }
    else if (rc == TE_EASNOTHERCHOICE)
    {
        F_ERROR("%s(): TCP CSAP %d, non-plain remote port not supported",
                __FUNCTION__, csap_descr->id);
        return TE_EOPNOTSUPP;
    }
    else
        return rc;

    if (csap_descr->type == TAD_CSAP_DATA)
    {
        /* TODO: support of TCP over IPv6 */
        struct sockaddr_in local;

        local.sin_family = AF_INET;
        local.sin_addr = ip4_spec_data->local_addr;

        if ((spec_data->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            rc = errno;
            ERROR("%s(CSAP %d) socket create failed, errno %d", 
                  __FUNCTION__, csap_descr->id, rc);
            return rc;
        }

        if (bind(spec_data->socket, SA(&local), sizeof(local)) < 0)
        {
            rc = errno;
            ERROR("%s(CSAP %d) socket bind failed, errno %d", 
                  __FUNCTION__, csap_descr->id, rc);
            return rc;
        }

        switch (spec_data->data_tag)
        {
            case NDN_TAG_TCP_DATA_SERVER:
                if (listen(spec_data->socket, 10) < 0)
                {
                    rc = errno;
                    ERROR("%s(CSAP %d) listen failed, errno %d", 
                          __FUNCTION__, csap_descr->id, rc);
                    return rc;
                }
                break;

            case NDN_TAG_TCP_DATA_CLIENT: 
                {
                    struct sockaddr_in remote;

                    if (spec_data->remote_port == 0 ||
                        ip4_spec_data->remote_addr.s_addr == INADDR_ANY)
                    {
                        ERROR("%s(CSAP %d) client csap, remote need", 
                              __FUNCTION__, csap_descr->id);
                        return TE_ETADWRONGNDS;
                    }
                    remote.sin_family = AF_INET;
                    remote.sin_port = spec_data->remote_port;
                    remote.sin_addr = ip4_spec_data->remote_addr;

                    if (connect(spec_data->socket, SA(&remote), 
                                sizeof(remote)) < 0)
                    {
                        rc = errno;
                        ERROR("%s(CSAP %d) connect failed, errno %d", 
                              __FUNCTION__, csap_descr->id, rc);
                        return rc;
                    }
                }
                break;

            default:
                ERROR("%s(CSAP %d) unexpected tag of 'data' field %d", 
                      __FUNCTION__, csap_descr->id, spec_data->data_tag);
                return TE_ETADWRONGNDS;
        }
    }

    UNUSED(csap_nds);
    return 0;
}

/**
 * Callback for destroy 'tcp' CSAP layer if over 'ip4' in stack.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed 
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
tcp_ip4_destroy_cb(int csap_id, int layer)
{
    csap_p csap_descr;

    tcp_csap_specific_data_t *spec_data; 

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_ETADCSAPNOTEX;

    spec_data = csap_descr->layers[layer].specific_data;

    if (csap_descr->type == TAD_CSAP_DATA)
    {
        if (spec_data->socket > 0)
        {
            close(spec_data->socket);
            spec_data->socket = -1;
        }
    }

    return 0;
}

