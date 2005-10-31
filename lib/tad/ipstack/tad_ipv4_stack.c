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

#define TE_LGR_USER     "TAD IPv4"

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
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#include "logger_ta_fast.h"

#ifdef WITH_ETH
#include "../eth/tad_eth_impl.h"
#endif

#include "tad_ipstack_impl.h"


/* Forward declaration */
extern int tad_ip4_check_pdus(csap_p csap_descr, asn_value *traffic_nds);

 
/* See description tad_ipstack_impl.h */
int 
tad_ip4_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
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


/* See description tad_ipstack_impl.h */
int 
tad_ip4_write_cb(csap_p csap_descr, const char *buf, size_t buf_len)
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


/* See description tad_ipstack_impl.h */
int 
tad_ip4_write_read_cb(csap_p csap_descr, int timeout,
                      const char *w_buf, size_t w_buf_len,
                      char *r_buf, size_t r_buf_len)
{
    int rc; 

    rc = tad_ip4_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (rc == -1)  
        return rc;
    else 
        return tad_ip4_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
}


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_single_init_cb(int csap_id, const asn_value *csap_nds,
                       unsigned int layer)
{ 
    ip4_csap_specific_data_t *ip4_spec_data; 

    struct sockaddr_in local;

    csap_p csap_descr;
    int    opt = 1;
    int    rc;
    char   opt_label[100];
    size_t len;

    UNUSED(local);

    ENTRY("CSAP=%d NDS=%p layer=%u");

    if (csap_nds == NULL)
        return TE_EWRONGPTR;

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_ETADCSAPNOTEX;

    ip4_spec_data = calloc(1, sizeof(ip4_csap_specific_data_t));
    if (ip4_spec_data == NULL)
        return TE_ENOMEM;

    /* FIXME */
    sprintf(opt_label, "%u.local-addr", layer);
    len = sizeof(struct in_addr);
    rc = asn_read_value_field(csap_nds, 
                              &ip4_spec_data->sa_op.sin_addr.s_addr, &len, 
                              opt_label);
    if (rc != 0 && rc != TE_EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    rc = 0;

 
    ip4_spec_data->sa_op.sin_family = AF_INET;
    ip4_spec_data->sa_op.sin_port = 0;
    

    /* default read timeout */
    ip4_spec_data->read_timeout = 200000; /* FIXME */

    csap_descr->layers[layer].specific_data = ip4_spec_data;
    csap_descr->layers[layer].get_param_cb = tad_ip4_get_param_cb;

    if (csap_descr->type == TAD_CSAP_RAW)
    {
        /* opening incoming socket */
        ip4_spec_data->socket = socket(AF_INET, SOCK_RAW, IPPROTO_IP); 
        if (ip4_spec_data->socket < 0)
        {
            return TE_OS_RC(TE_TAD_CSAP, errno);
        }
        if (setsockopt(ip4_spec_data->socket, SOL_SOCKET, SO_REUSEADDR, 
                       &opt, sizeof(opt)) < 0)
        {
            return TE_OS_RC(TE_TAD_CSAP, errno);
        }

        csap_descr->read_cb          = tad_ip4_read_cb;
        csap_descr->write_cb         = tad_ip4_write_cb;
        csap_descr->write_read_cb    = tad_ip4_write_read_cb;
        csap_descr->read_write_layer = layer; 
        csap_descr->timeout          = 500000; /* FIXME */

    }
    else
    {
        ip4_spec_data->socket = -1;
    } 

    EXIT("OK");

    return 0;
}


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_single_destroy_cb(int csap_id, unsigned int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    ip4_csap_specific_data_t * spec_data = 
        (ip4_csap_specific_data_t *) csap_descr->layers[layer].specific_data; 
     
    if(spec_data->socket >= 0)
        close(spec_data->socket);    

    tad_data_unit_clear(&spec_data->du_version);
    tad_data_unit_clear(&spec_data->du_header_len);
    tad_data_unit_clear(&spec_data->du_tos);
    tad_data_unit_clear(&spec_data->du_ip_len);
    tad_data_unit_clear(&spec_data->du_ip_ident);
    tad_data_unit_clear(&spec_data->du_flags);
    tad_data_unit_clear(&spec_data->du_ip_offset);
    tad_data_unit_clear(&spec_data->du_ttl);
    tad_data_unit_clear(&spec_data->du_protocol);
    tad_data_unit_clear(&spec_data->du_h_checksum);
    tad_data_unit_clear(&spec_data->du_src_addr);
    tad_data_unit_clear(&spec_data->du_dst_addr);

    spec_data->socket = -1;

    return 0;
}


#ifdef WITH_ETH
/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_eth_init_cb(int csap_id, const asn_value *csap_nds, unsigned int layer)
{ 
    ip4_csap_specific_data_t *spec_data; 
    eth_csap_specific_data_t *eth_spec_data; 
    csap_p csap_descr;
    size_t val_len;
    int    rc;

    VERB("%s called for csap %d, layer %d",
         __FUNCTION__, csap_id, layer); 

    if (csap_nds == NULL)
        return TE_EWRONGPTR;

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_ETADCSAPNOTEX;

    if (layer + 1 >= csap_descr->depth)
    {
        ERROR("%s(CSAP %d) too large layer %d!, depth %d", 
              __FUNCTION__, csap_id, layer, csap_descr->depth);
        return TE_EINVAL;
    }

    spec_data = calloc(1, sizeof(ip4_csap_specific_data_t));
    if (spec_data == NULL)
        return TE_ENOMEM;

    eth_spec_data = (eth_csap_specific_data_t *)
        csap_descr->layers[layer + 1].specific_data;

    csap_descr->layers[layer].specific_data = spec_data;
    csap_descr->layers[layer].get_param_cb = tad_ip4_get_param_cb;

    csap_descr->check_pdus_cb = tad_ip4_check_pdus; 

    val_len = sizeof(spec_data->remote_addr);
    rc = asn_read_value_field(csap_descr->layers[layer].csap_layer_pdu,
                              &spec_data->remote_addr, &val_len,
                              "remote-addr.#plain");
    if (rc != 0)
    {
        INFO("%s(): read remote addr fails %X", __FUNCTION__, rc);
        spec_data->remote_addr.s_addr = 0;
    }

    val_len = sizeof(spec_data->local_addr);
    rc = asn_read_value_field(csap_descr->layers[layer].csap_layer_pdu,
                              &spec_data->local_addr, &val_len,
                              "local-addr.#plain");
    if (rc != 0)
    {
        INFO("%s(): read local addr fails %X", __FUNCTION__, rc);
        spec_data->local_addr.s_addr = 0;
    }

    F_VERB("%s(): csap %d, layer %d",
            __FUNCTION__, csap_id, layer); 

    if (eth_spec_data->eth_type == 0)
        eth_spec_data->eth_type = 0x0800;

    UNUSED(csap_nds);
    return 0;
}


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_eth_destroy_cb(int csap_id, unsigned int layer)
{ 
    UNUSED(csap_id);
    UNUSED(layer);
    return 0;
}

#endif /* WITH_ETH */


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_check_pdus(csap_p csap_descr, asn_value *traffic_nds)
{
    UNUSED(csap_descr);
    UNUSED(traffic_nds);

    INFO("%s(CSAP %d) called", __FUNCTION__, csap_descr->id);

    return 0;
}
