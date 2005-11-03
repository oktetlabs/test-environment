/** @file
 * @brief IP Stack TAD
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
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

 
/* See description tad_ipstack_impl.h */
int 
tad_icmp4_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
#if 0
    int    rc; 
    int    layer;    
    fd_set read_set;
    icmp4_csap_specific_data_t *spec_data;
    
    struct timeval timeout_val;
       
    layer = csap_descr->read_write_layer;
    
    spec_data = (icmp4_csap_specific_data_t *) csap_descr->layer_data[layer]; 

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
    
    if (rc == 0)
        return 0;

    if (rc < 0)
        return -1;
    
    /* Note: possibly MSG_TRUNC and other flags are required */
    return recv (spec_data->in, buf, buf_len, 0); 
#else
    UNUSED(csap_descr);
    UNUSED(timeout);
    UNUSED(buf);
    UNUSED(buf_len);
    return -1;
#endif
}


/* See description tad_ipstack_impl.h */
int 
tad_icmp4_write_cb(csap_p csap_descr, const char *buf, size_t buf_len)
{
#if 0
    icmp4_csap_specific_data_t * spec_data;
    int layer;    
    int rc;
    struct sockaddr_in dest;
    
    dest.sin_family = AF_INET;
    dest.sin_port = htons(68);
    dest.sin_addr.s_addr = INADDR_BROADCAST;
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (icmp4_csap_specific_data_t *) csap_descr->specific_data[layer]; 

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
        perror("icmp sendto fail");
        csap_descr->last_errno = errno;
    }

    return rc;
#else
    UNUSED(csap_descr);
    UNUSED(buf);
    UNUSED(buf_len);
    return -1;
#endif
}


/* See description tad_ipstack_impl.h */
int 
tad_icmp4_write_read_cb(csap_p csap_descr, int timeout,
                        const char *w_buf, size_t w_buf_len,
                        char *r_buf, size_t r_buf_len)
{
    int rc; 

    rc = tad_icmp4_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (rc == -1)  
        return rc;
    else 
        return tad_icmp4_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}


/* See description tad_ipstack_impl.h */
te_errno
tad_icmp4_single_init_cb(csap_p csap_descr, unsigned int layer,
                         const asn_value *csap_nds)
{ 
    UNUSED(csap_nds);

    csap_descr->layers[layer].specific_data = NULL;
    csap_descr->layers[layer].get_param_cb = NULL;

    csap_descr->read_cb          = tad_icmp4_read_cb;
    csap_descr->write_cb         = tad_icmp4_write_cb;
    csap_descr->write_read_cb    = tad_icmp4_write_read_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 500000;

    return 0;
}


/* See description tad_ipstack_impl.h */
te_errno 
icmp4_single_destroy_cb(csap_p csap_descr, unsigned int layer)
{
    UNUSED(csap_descr);
    UNUSED(layer);
    return 0;
}
