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


 
/**
 * Callback for read data from media of udpernet CSAP. 
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
udp_ip4_read_cb (csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    int    rc; 
    int    layer;    
    fd_set read_set;
    udp_csap_specific_data_t *spec_data;
    
    struct timeval timeout_val;
    
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (udp_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 

#ifdef TALOGDEBUG
    printf("Reading data from the socket: %d", spec_data->in);
#endif       

    if(spec_data->socket < 0)
    {
        return -1;
    }

    FD_ZERO(&read_set);
    FD_SET(spec_data->socket, &read_set);

    if (timeout == 0)
    {
        timeout_val.tv_sec = 0;  
        timeout_val.tv_usec = 100000;/* 0.1 of second */
    }
    else
    {
        timeout_val.tv_sec = timeout / 1000000L; 
        timeout_val.tv_usec = timeout % 1000000L;
    }
    
    rc = select(spec_data->socket + 1, &read_set, NULL, NULL, &timeout_val); 
    
    if (rc == 0)
        return 0;

    if (rc < 0)
        return -1;
    
    /* Note: possibly MSG_TRUNC and other flags are required */
    return recv (spec_data->socket, buf, buf_len, 0); 
}

/**
 * Callback for write data to media of udpernet CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
udp_ip4_write_cb (csap_p csap_descr, char *buf, size_t buf_len)
{
    udp_csap_specific_data_t * udp_spec_data;
    ip4_csap_specific_data_t * ip4_spec_data;
    struct sockaddr_in dest;
    struct sockaddr_in source;
    int layer;    
    int rc; 

    char new_port = 0;
    char new_addr = 0;

    memset (&source, 0, sizeof (source));
    memset (&dest, 0, sizeof (dest));
    
    layer = csap_descr->read_write_layer;
    
    udp_spec_data = (udp_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 
    ip4_spec_data = (ip4_csap_specific_data_t *) csap_descr->layers[layer+1].specific_data; 
 
    dest.sin_family  = AF_INET;
    if (udp_spec_data->dst_port)
        dest.sin_port    = htons(udp_spec_data->dst_port);
    else 
        dest.sin_port    = htons(udp_spec_data->remote_port);

    if (ip4_spec_data->dst_addr.s_addr != INADDR_ANY)
        dest.sin_addr = ip4_spec_data->dst_addr;
    else
        dest.sin_addr = ip4_spec_data->remote_addr;

#ifdef TALOGDEBUG
    printf("Writing data to socket: %d", (int)udp_spec_data->socket);
#endif        

    if(udp_spec_data->socket < 0)
    {
        return -1;
    }

    source.sin_port = htons(udp_spec_data->local_port);
    source.sin_addr = ip4_spec_data->local_addr; 

    if (udp_spec_data->src_port != 0 && 
        udp_spec_data->src_port != udp_spec_data->local_port)
    {
        source.sin_port = htons(udp_spec_data->src_port);
        new_port = 1;
    }

    if (ip4_spec_data->src_addr.s_addr != INADDR_ANY && 
        ip4_spec_data->src_addr.s_addr != ip4_spec_data->local_addr.s_addr)
    { 
        source.sin_addr = ip4_spec_data->src_addr; 
        new_addr = 1;
    }

    if (new_port || new_addr)
    {
        /* need rebind socket */
        if (bind(udp_spec_data->socket, 
                 (struct sockaddr *)&source, sizeof(source)) == -1)
        {
            perror ("udp csap socket bind");
            csap_descr->last_errno = errno;
            return TE_RC(TE_TAD_CSAP, errno);
        }
    }

    rc = sendto(udp_spec_data->socket, buf, buf_len, 0, 
                (struct sockaddr *) &dest, sizeof(dest));
    if (rc < 0) 
    {
        perror("udp sendto fail");
        csap_descr->last_errno = errno;
    }

    ip4_spec_data->src_addr.s_addr = INADDR_ANY;
    ip4_spec_data->dst_addr.s_addr = INADDR_ANY;
    udp_spec_data->src_port = 0;
    udp_spec_data->dst_port = 0;

    source.sin_port = htons(udp_spec_data->local_port);
    source.sin_addr = ip4_spec_data->local_addr; 

    if (bind(udp_spec_data->socket, (struct sockaddr *)&source, sizeof(source)) == -1)
    {
        perror ("udp csap socket reverse bind");
        rc = csap_descr->last_errno = errno;
    }

    return TE_RC(TE_TAD_CSAP, rc);
}

/**
 * Callback for write data to media of udpernet CSAP and read
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
udp_ip4_write_read_cb (csap_p csap_descr, int timeout,
                   char *w_buf, size_t w_buf_len,
                   char *r_buf, size_t r_buf_len)
{
    int rc; 

    rc = udp_ip4_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (rc == -1)  
        return rc;
    else 
        return udp_ip4_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}

/**
 * Callback for init udp CSAP layer  if over 'ip4' in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters.
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
udp_ip4_init_cb (int csap_id, const asn_value *csap_nds, int layer)
{
    csap_p                    csap_descr;
    udp_csap_specific_data_t *udp_spec_data; 
    ip4_csap_specific_data_t *ip4_spec_data; 
    struct sockaddr_in        local;

    size_t len; 
    char   opt_label[100];
    int    rc; 

    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, ETEWRONGPTR);

    if ((csap_descr = csap_find (csap_id)) == NULL)
        return TE_RC(TE_TAD_CSAP, ETADCSAPNOTEX);

    if (layer + 1 >= csap_descr->depth)
    {
        ERROR("%s(CSAP %d) too large layer %d!, depth %d", 
              __FUNCTION__, csap_id, layer, csap_descr->depth);
        return EINVAL;
    }

    ip4_spec_data = (ip4_csap_specific_data_t *)
        csap_descr->layers[layer + 1].specific_data;

    if (ip4_spec_data != NULL && ip4_spec_data->protocol == 0)
        ip4_spec_data->protocol = IPPROTO_UDP;

    udp_spec_data = calloc (1, sizeof(udp_csap_specific_data_t));
    if (udp_spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, ENOMEM);

    /* Init local port */
    sprintf(opt_label, "%d.local-port", layer);
    len = sizeof(udp_spec_data->local_port);
    rc = asn_read_value_field(csap_nds, &udp_spec_data->local_port,
                              &len, opt_label);
    if (rc != 0)
    {
        if (csap_descr->type != TAD_CSAP_DATA)
        {
            WARN("%s: %d.local-port is not found in CSAP pattern, set to 0",
                 __FUNCTION__, layer);
            udp_spec_data->local_port = 0;
        }
        else
        {
            free(udp_spec_data);
            ERROR("%s: %d.local-port is not specified", __FUNCTION__, layer);
            return TE_RC(TE_TAD_CSAP, rc);
        }
    }

    /* Init remote port */
    sprintf(opt_label, "%d.remote-port", layer);
    len = sizeof(udp_spec_data->remote_port);
    rc = asn_read_value_field(csap_nds, &udp_spec_data->remote_port, 
                              &len, opt_label);
    if (rc != 0)
    {
        if (csap_descr->type != TAD_CSAP_DATA)
        {
            WARN("%s: %d.remote-port is not found in CSAP pattern, set to 0",
                 __FUNCTION__, layer);
            udp_spec_data->remote_port = 0;
        }
        else
        {
            free(udp_spec_data);
            ERROR("%s: %d.remote-port is not specified", __FUNCTION__, layer);
            return TE_RC(TE_TAD_CSAP, rc);
        }
    }

    if (csap_descr->type == TAD_CSAP_DATA)
    {
        /* opening incoming socket */
        udp_spec_data->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
        if (udp_spec_data->socket < 0)
        {
            free(udp_spec_data);
            return TE_RC(TE_TAD_CSAP, errno);
        } 

        local.sin_family = AF_INET;
        local.sin_port = htons(udp_spec_data->local_port);

        sprintf (opt_label, "%d.local-addr", layer+1); /* Next layer is IPv4 */
        len = sizeof(struct in_addr);
        rc = asn_read_value_field(csap_nds, &local.sin_addr.s_addr, 
                                  &len, opt_label);
        if (rc == EASNINCOMPLVAL)
        {
            local.sin_addr.s_addr = INADDR_ANY; 
            rc = 0;
        } 
        else if (rc)
        {
            free(udp_spec_data);
            return TE_RC(TE_TAD_CSAP, rc); 
        }

        if ((rc == 0 ||            /* there is some meaning IP-address */
             local.sin_port != 0 ) /* there is some meaning UDP-port */
            && bind(udp_spec_data->socket, 
                    (struct sockaddr *)&local, sizeof(local)) == -1)
        {
            perror ("udp csap socket bind");
            free(udp_spec_data);
            return errno;
        }

        csap_descr->read_cb         = udp_ip4_read_cb;
        csap_descr->write_cb        = udp_ip4_write_cb;
        csap_descr->write_read_cb   = udp_ip4_write_read_cb;
        csap_descr->read_write_layer = layer; 
        csap_descr->timeout          = 100000;
    }
    else
    {
        udp_spec_data->socket = -1;
    }



    csap_descr->layers[layer].specific_data = udp_spec_data;
    csap_descr->layers[layer].get_param_cb = udp_get_param_cb;

    return 0;
}

/**
 * Callback for destroy udpernet CSAP layer  if single in stack.
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
udp_ip4_destroy_cb (int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    udp_csap_specific_data_t * spec_data = 
        (udp_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 
     
    if(spec_data->socket >= 0)
        close(spec_data->socket);    

    return 0;
}


