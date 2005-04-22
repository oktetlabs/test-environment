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

#define TE_LGR_USER     "TAD IPv4"

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


/* Forward declaration */
extern int ip4_check_pdus(csap_p csap_descr, asn_value *traffic_nds);

 
/**
 * Callback for read data from media of ipernet CSAP. 
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
ip4_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    int    rc; 
    int    layer;    
    fd_set read_set;
    ip4_csap_specific_data_t *spec_data;
    
    struct timeval timeout_val;
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (ip4_csap_specific_data_t *)
        csap_descr->layers[layer].specific_data; 

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
        timeout_val.tv_sec = spec_data->read_timeout;
        timeout_val.tv_usec = 0;
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
 * Callback for write data to media of ipernet CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
ip4_write_cb(csap_p csap_descr, char *buf, size_t buf_len)
{
    ip4_csap_specific_data_t * spec_data;
    int layer;    
    int rc;
    
       
    layer = csap_descr->read_write_layer;
    
    spec_data = (ip4_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 

#ifdef TALOGDEBUG
    printf("Writing data to socket: %d", spec_data->out);
#endif        

    if(spec_data->socket < 0)
    {
        return -1;
    }

    rc = sendto (spec_data->socket, buf, buf_len, 0, 
                 (struct sockaddr *)&spec_data->sa_op, 
                 sizeof(spec_data->sa_op));
    if (rc < 0) 
    {
        perror("ip sendto fail");
        csap_descr->last_errno = errno;
    }

    return rc;
}

/**
 * Callback for write data to media of ipernet CSAP and read
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
ip4_write_read_cb(csap_p csap_descr, int timeout,
                  char *w_buf, size_t w_buf_len,
                  char *r_buf, size_t r_buf_len)
{
    int rc; 

    rc = ip4_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (rc == -1)  
        return rc;
    else 
        return ip4_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}

/**
 * Callback for init ip CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
ip4_single_init_cb(int csap_id, const asn_value *csap_nds, int layer)
{ 
    ip4_csap_specific_data_t *ip4_spec_data; 

    struct sockaddr_in local;

    csap_p csap_descr;
    int    opt = 1;
    int    rc;
    char   opt_label[100];
    size_t len;

    UNUSED(local);

    VERB ( "IPv4 INIT called\n");
    if (csap_nds == NULL)
        return ETEWRONGPTR;

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return ETADCSAPNOTEX;

    ip4_spec_data = calloc(1, sizeof(ip4_csap_specific_data_t));
    
    if (ip4_spec_data == NULL)
    {
        return ENOMEM;
    }
    

    sprintf(opt_label, "%d.local-addr", layer);
    len = sizeof(struct in_addr);
    rc = asn_read_value_field(csap_nds, 
                              &ip4_spec_data->sa_op.sin_addr.s_addr, &len, 
                              opt_label);
    if (rc)
        return TE_RC(TE_TAD_CSAP, rc); /* If this field is not set, then CSAP cannot process */ 


 
    ip4_spec_data->sa_op.sin_family = AF_INET;
    ip4_spec_data->sa_op.sin_port = 0;
    

    /* default read timeout */
    ip4_spec_data->read_timeout = 200000;

    csap_descr->layers[layer].specific_data = ip4_spec_data;
    csap_descr->layers[layer].get_param_cb = ip4_get_param_cb;

    if (csap_descr->type == TAD_CSAP_RAW)
    {
        /* opening incoming socket */
        ip4_spec_data->socket = socket(AF_INET, SOCK_RAW, IPPROTO_IP); 
        if (ip4_spec_data->socket < 0)
        {
            return errno;
        }
        if (setsockopt(ip4_spec_data->socket, SOL_SOCKET, SO_REUSEADDR, 
                       (void *) &opt, sizeof(opt)) == -1)
        {
            return errno;
        }

        csap_descr->read_cb         = ip4_read_cb;
        csap_descr->write_cb        = ip4_write_cb;
        csap_descr->write_read_cb   = ip4_write_read_cb;
        csap_descr->read_write_layer = layer; 
        csap_descr->timeout          = 500000;

    }
    else
    {
        ip4_spec_data->socket = -1;
    } 

    return 0;
}

/**
 * Callback for destroy ipernet CSAP layer  if single in stack.
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
ip4_single_destroy_cb(int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    ip4_csap_specific_data_t * spec_data = 
        (ip4_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 
     
    if(spec_data->socket >= 0)
        close(spec_data->socket);    

    spec_data->socket = -1;

    return 0;
}


/**
 * Callback for init ip CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
ip4_eth_init_cb(int csap_id, const asn_value *csap_nds, int layer)
{ 
    ip4_csap_specific_data_t *spec_data; 
    const asn_value          *ip4_pdu;   

    csap_p csap_descr;      /**< csap descriptor   */ 
    size_t val_len;
    int    rc;

    if (csap_nds == NULL)
        return ETEWRONGPTR;

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return ETADCSAPNOTEX;

    spec_data = calloc(1, sizeof(ip4_csap_specific_data_t));
    
    if (spec_data == NULL)
        return ENOMEM;

    csap_descr->layers[layer].specific_data = spec_data;
    csap_descr->layers[layer].get_param_cb = ip4_get_param_cb;

    csap_descr->check_pdus_cb = ip4_check_pdus; 

    rc = asn_read_value_field(csap_descr->layers[layer].csap_layer_pdu,
                              &spec_data->remote_addr, 4,
                              "remote-addr.#plain");
    if (rc != 0);
        WARN("%s(): read remote addr fails %X", __FUNCTION__, rc);

    rc = asn_read_value_field(csap_descr->layers[layer].csap_layer_pdu,
                              &spec_data->local_addr, 4,
                              "local-addr.#plain");
    if (rc != 0);
        WARN("%s(): read local addr fails %X", __FUNCTION__, rc);

    F_VERB("%s(): csap %d, layer %d",
            __FUNCTION__, csap_id, layer); 

    UNUSED(csap_nds);
    return 0;
}




/**
 * Callback for destroy ipernet CSAP layer  if single in stack.
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
ip4_eth_destroy_cb(int csap_id, int layer)
{ 
    UNUSED(csap_id);
    UNUSED(layer);
    return 0;
}


int 
ip4_check_pdus(csap_p csap_descr, asn_value *traffic_nds)
{
    UNUSED(csap_descr);
    UNUSED(traffic_nds);
    return 0;
}

