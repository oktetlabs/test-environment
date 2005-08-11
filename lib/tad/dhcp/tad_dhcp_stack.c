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

#include <sys/ioctl.h>

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

#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include "tad_dhcp_impl.h"

#define TE_LGR_USER     "TAD DHCP stack"
#include "logger_ta.h"

 
/**
 * Callback for read data from media of dhcpernet CSAP. 
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
dhcp_read_cb (csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    int    rc; 
    int    layer;    
    fd_set read_set;
    dhcp_csap_specific_data_t *spec_data;
    
    struct timeval timeout_val;
    
    if (csap_descr == NULL)
    {
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (dhcp_csap_specific_data_t *)
        csap_descr->layers[layer].specific_data; 

#ifdef TALOGDEBUG
    printf("Reading data from the socket: %d", spec_data->in);
#endif       

    if(spec_data->in < 0)
    {
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
    
    rc = select(spec_data->in + 1, &read_set, NULL, NULL, &timeout_val); 
    VERB("%s(): select = %d", __FUNCTION__, rc);
    
    if (rc == 0)
        return 0;

    if (rc < 0)
        return -1;
    
    /* Note: possibly MSG_TRUNC and other flags are required */
    return recv (spec_data->in, buf, buf_len, 0); 
}

/**
 * Callback for write data to media of dhcpernet CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
dhcp_write_cb (csap_p csap_descr, char *buf, size_t buf_len)
{
    dhcp_csap_specific_data_t * spec_data;
    int layer;    
    int rc;
    struct sockaddr_in dest;
    
    if (csap_descr == NULL)
    {
        return -1;
    }
    
    layer = csap_descr->read_write_layer;
    spec_data = (dhcp_csap_specific_data_t *)
        csap_descr->layers[layer].specific_data; 
    dest.sin_family = AF_INET;
    dest.sin_port = htons(spec_data->mode == DHCP4_CSAP_MODE_SERVER ? 
                          DHCP_CLIENT_PORT : DHCP_SERVER_PORT);
    dest.sin_addr.s_addr = INADDR_BROADCAST;
    
    

#ifdef TALOGDEBUG
    printf("Writing data to socket: %d", spec_data->out);
#endif        

    if(spec_data->out < 0)
    {
        return -1;
    }
    rc = sendto (spec_data->out, buf, buf_len, 0, 
                 (struct sockaddr *)&dest, sizeof(dest));
    if (rc < 0) 
    {
        perror("dhcp sendto fail");
        csap_descr->last_errno = errno;
    }

    return rc;
}

/**
 * Callback for write data to media of dhcpernet CSAP and read
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
dhcp_write_read_cb (csap_p csap_descr, int timeout,
                   char *w_buf, size_t w_buf_len,
                   char *r_buf, size_t r_buf_len)
{
    int rc; 
    
    rc = dhcp_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (rc == -1)  
        return rc;
    else 
        return dhcp_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}

/**
 * Callback for init dhcp CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
dhcp_single_init_cb(int csap_id, const asn_value *csap_nds, int layer)
{
    csap_p   csap_descr;          /**< csap description        */

    dhcp_csap_specific_data_t *   dhcp_spec_data; 
    struct sockaddr_in local;
    struct sockaddr_in *ifa;

    int             opt = 1;
    int             mode, rc;
    size_t          len;
    struct ifreq   *interface = calloc(sizeof(struct ifreq) + 
                                       sizeof(struct sockaddr_storage) - 
                                       sizeof(struct sockaddr), 1); 

    if (csap_nds == NULL)
        return TE_EWRONGPTR;

    if ((csap_descr = csap_find (csap_id)) == NULL)
        return TE_ETADCSAPNOTEX;


    len = sizeof(mode);
    rc = asn_read_value_field(csap_nds, &mode, &len, "0.mode");
    if (rc)
        return rc; /* If this field is not set, then CSAP cannot process */ 

    dhcp_spec_data = malloc (sizeof(dhcp_csap_specific_data_t));
    
    if (dhcp_spec_data == NULL)
    {
        return TE_ENOMEM;
    }
    
    
    dhcp_spec_data->ipaddr = malloc(INET_ADDRSTRLEN + 1);
    dhcp_spec_data->mode = mode;

    /* opening incoming socket */
    dhcp_spec_data->in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
    if (dhcp_spec_data->in < 0)
    {
        return errno;
    }

    opt = 1;
    if (setsockopt(dhcp_spec_data->in, SOL_SOCKET, SO_REUSEADDR, 
                   (void *)&opt, sizeof(opt)) != 0)
    {
        return errno;
    }

    local.sin_family = AF_INET;
    local.sin_port = htons(mode == DHCP4_CSAP_MODE_SERVER ? 
                           DHCP_SERVER_PORT : DHCP_CLIENT_PORT); 
    local.sin_addr.s_addr = INADDR_ANY;

    if (bind(dhcp_spec_data->in, SA(&local), sizeof(local)) != 0)
    {
        perror ("dhcp csap socket bind");
        return errno;
    }
    /* 
     * shutdown(SHUT_WR) looks reasonable here, but it can't be
     * called on not connected socket.
     */


    /* opening outgoing socket */
    dhcp_spec_data->out = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
    if (dhcp_spec_data->out < 0)
    {
        return errno;
    }

    opt = 1;
    if (setsockopt(dhcp_spec_data->out, SOL_SOCKET, SO_REUSEADDR, 
                   (void *)&opt, sizeof(opt)) != 0)
    {
        return errno;
    } 

    len = IFNAMSIZ;
    rc = asn_read_value_field(csap_nds, interface->ifr_ifrn.ifrn_name,
                              &len, "0.iface");
    if (rc == 0)
    {
        if (setsockopt(dhcp_spec_data->out, SOL_SOCKET, SO_BINDTODEVICE,
                       interface->ifr_ifrn.ifrn_name,
                       strlen(interface->ifr_ifrn.ifrn_name) + 1) != 0) 
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
        dhcp_single_destroy_cb(csap_id, layer);
        return rc;
    }

    if (ioctl(dhcp_spec_data->in, SIOCGIFADDR, interface) != 0)
    {
        perror ("ioctl get ifaddr");
        return errno;
    }
    ifa = (struct sockaddr_in *) &interface->ifr_addr;
    strncpy(dhcp_spec_data->ipaddr, 
            inet_ntoa(ifa->sin_addr), 
            INET_ADDRSTRLEN);

    if (rc)
    {
        dhcp_single_destroy_cb(csap_id, layer);
        return rc;
    }

    opt = 1;
    if (setsockopt(dhcp_spec_data->out, SOL_SOCKET, SO_BROADCAST, 
                   (void *)&opt, sizeof(opt)) != 0)
    {
        dhcp_single_destroy_cb(csap_id, layer);
        return errno;
    }

    local.sin_addr.s_addr = ifa->sin_addr.s_addr;

    if (bind(dhcp_spec_data->out, SA(&local), sizeof(local)) != 0)
    {
        perror ("dhcp csap socket bind");
        return errno;
    }
    /* 
     * shutdown(SHUT_RD) looks reasonable here, but it can't be
     * called on not connected socket.
     */

    free(interface);


    /* default read timeout */
    dhcp_spec_data->read_timeout = 200000;

    csap_descr->layers[layer].specific_data = dhcp_spec_data;
    csap_descr->layers[layer].get_param_cb = dhcp_get_param_cb;

    csap_descr->read_cb         = dhcp_read_cb;
    csap_descr->write_cb        = dhcp_write_cb;
    csap_descr->write_read_cb   = dhcp_write_read_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 500000;
    
    return 0;
}

/**
 * Callback for destroy dhcpernet CSAP layer  if single in stack.
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
dhcp_single_destroy_cb (int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    dhcp_csap_specific_data_t * spec_data = 
        (dhcp_csap_specific_data_t *)
        csap_descr->layers[layer].specific_data; 
     
    if(spec_data->in >= 0)
        close(spec_data->in);    

    if(spec_data->out >= 0)
        close(spec_data->out);    
    return 0;
}


