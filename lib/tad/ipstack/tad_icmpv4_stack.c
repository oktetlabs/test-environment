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
 * Callback for read data from media of icmpernet CSAP. 
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
icmp4_read_cb (csap_p csap_descr, int timeout, char *buf, size_t buf_len)
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

/**
 * Callback for write data to media of icmpernet CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
icmp4_write_cb (csap_p csap_descr, char *buf, size_t buf_len)
{
#if 0
    icmp4_csap_specific_data_t * spec_data;
    int layer;    
    int rc;
    struct sockaddr_in dest;
    
    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        return -1;
    }
    
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

/**
 * Callback for write data to media of icmpernet CSAP and read
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
icmp4_write_read_cb (csap_p csap_descr, int timeout,
                   char *w_buf, size_t w_buf_len,
                   char *r_buf, size_t r_buf_len)
{
    int rc; 

    rc = icmp4_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (rc == -1)  
        return rc;
    else 
        return icmp4_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}

/**
 * Callback for init icmp CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
icmp4_single_init_cb (int csap_id, const asn_value *csap_nds, int layer)
{ 
    csap_p   csap_descr;          /**< csap descriptor        */ 

    UNUSED(csap_nds);

    if ((csap_descr = csap_find (csap_id)) == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ETADCSAPNOTEX);


    csap_descr->layers[layer].specific_data = NULL;
    csap_descr->layers[layer].get_param_cb = NULL;

    csap_descr->read_cb         = icmp4_read_cb;
    csap_descr->write_cb        = icmp4_write_cb;
    csap_descr->write_read_cb   = icmp4_write_read_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 500000;

    return 0;
}

/**
 * Callback for destroy icmpernet CSAP layer  if single in stack.
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
icmp4_single_destroy_cb (int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);
    UNUSED(layer);
    UNUSED(csap_descr);
    return 0;
}


