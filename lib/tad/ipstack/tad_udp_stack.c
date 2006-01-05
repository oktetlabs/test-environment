/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * UDP CSAP layer stack-related callbacks.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD UDP"

#include "te_config.h"
#if HAVE_CONFIG_H
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


/* See description tad_ipstack_impl.h */
int 
tad_udp_ip4_read_cb(csap_p csap, int timeout, char *buf, size_t buf_len)
{
    int    rc; 
    int    layer;    
    fd_set read_set;
    udp_csap_specific_data_t *spec_data;
    
    struct timeval timeout_val;
    
    
    layer = csap_get_rw_layer(csap);
    
    spec_data = (udp_csap_specific_data_t *) csap_get_proto_spec_data(csap, layer); 

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


/* See description tad_ipstack_impl.h */
te_errno
tad_udp_ip4_write_cb(csap_p csap, const tad_pkt *pkt)
{
#if 1
    const void *buf;
    size_t      buf_len;

    if (pkt == NULL || tad_pkt_get_seg_num(pkt) != 1)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    buf     = pkt->segs.cqh_first->data_ptr;
    buf_len = pkt->segs.cqh_first->data_len;
#endif
    udp_csap_specific_data_t *udp_spec_data;
    ip4_csap_specific_data_t *ip4_spec_data;
    struct sockaddr_in dest;
    struct sockaddr_in source;
    int layer;    
    int rc; 

    char new_port = 0;
    char new_addr = 0;

    memset (&source, 0, sizeof (source));
    memset (&dest, 0, sizeof (dest));
    
    layer = csap_get_rw_layer(csap);
    
    udp_spec_data = (udp_csap_specific_data_t *) csap_get_proto_spec_data(csap, layer); 
    ip4_spec_data = (ip4_csap_specific_data_t *) csap->layers[layer+1].specific_data; 
 
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
            csap->last_errno = errno;
            return TE_OS_RC(TE_TAD_CSAP, errno);
        }
    }

    rc = sendto(udp_spec_data->socket, buf, buf_len, 0, 
                (struct sockaddr *) &dest, sizeof(dest));
    if (rc < 0) 
    {
        perror("udp sendto fail");
        csap->last_errno = errno;
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
        rc = csap->last_errno = errno;
    }

    return TE_RC(TE_TAD_CSAP, rc);
}


/* See description tad_ipstack_impl.h */
te_errno 
tad_udp_ip4_init_cb(csap_p csap, unsigned int layer,
                    const asn_value *csap_nds)
{
    udp_csap_specific_data_t *udp_spec_data; 
    ip4_csap_specific_data_t *ip4_spec_data; 
    struct sockaddr_in        local;

    size_t len; 
    char   opt_label[100];
    int    rc; 

    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EWRONGPTR);

    if (layer + 1 >= csap->depth)
    {
        ERROR("%s(CSAP %d) too large layer %d!, depth %d", 
              __FUNCTION__, csap->id, layer, csap->depth);
        return TE_EINVAL;
    }

    ip4_spec_data = (ip4_csap_specific_data_t *)
        csap->layers[layer + 1].specific_data;

    if (ip4_spec_data != NULL && ip4_spec_data->protocol == 0)
        ip4_spec_data->protocol = IPPROTO_UDP;

    udp_spec_data = calloc (1, sizeof(udp_csap_specific_data_t));
    if (udp_spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* Init local port */
    sprintf(opt_label, "%d.local-port", layer);
    len = sizeof(udp_spec_data->local_port);
    rc = asn_read_value_field(csap_nds, &udp_spec_data->local_port,
                              &len, opt_label);
    if (rc != 0)
    {
        if (csap->type != TAD_CSAP_DATA)
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
        if (csap->type != TAD_CSAP_DATA)
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

    if (csap->type == TAD_CSAP_DATA)
    {
        /* opening incoming socket */
        udp_spec_data->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
        if (udp_spec_data->socket < 0)
        {
            free(udp_spec_data);
            return TE_OS_RC(TE_TAD_CSAP, errno);
        } 

        local.sin_family = AF_INET;
        local.sin_port = htons(udp_spec_data->local_port);

        sprintf (opt_label, "%d.local-addr", layer+1); /* Next layer is IPv4 */
        len = sizeof(struct in_addr);
        rc = asn_read_value_field(csap_nds, &local.sin_addr.s_addr, 
                                  &len, opt_label);
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
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

        csap->read_write_layer = layer; 
        csap->timeout          = 100000;
    }
    else
    {
        udp_spec_data->socket = -1;
    }

    csap_set_proto_spec_data(csap, layer, udp_spec_data);

    return 0;
}

/* See description tad_ipstack_impl.h */
te_errno 
tad_udp_ip4_destroy_cb(csap_p csap, unsigned int layer)
{
    udp_csap_specific_data_t * spec_data = 
        (udp_csap_specific_data_t *) csap_get_proto_spec_data(csap, layer); 
     
    if (spec_data->socket >= 0)
        close(spec_data->socket);    

    return 0;
}
