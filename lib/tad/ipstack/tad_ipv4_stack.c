/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * IPv4 CSAP layer stack-related callbacks.
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "TAD IPv4"

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
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
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


/** IPv4 layer as read/write specific data */
typedef struct tad_ip4_rw_data {
    int                 socket;
    int                 read_timeout;
    struct sockaddr_in  sa_op;
} tad_ip4_rw_data;


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_rw_init_cb(csap_p csap, const asn_value *csap_nds)
{ 
    tad_ip4_rw_data    *spec_data;

    struct sockaddr_in local;

    int    opt = 1;
    int    rc;
    char   opt_label[100];
    size_t len;

    UNUSED(local);


    if (csap_nds == NULL)
        return TE_EWRONGPTR;

    spec_data = calloc(1, sizeof(*spec_data));
    if (spec_data == NULL)
        return TE_ENOMEM;
    csap_set_rw_data(csap, spec_data);

    /* FIXME */
    sprintf(opt_label, "%u.local-addr", csap_get_rw_layer(csap));
    len = sizeof(struct in_addr);
    rc = asn_read_value_field(csap_nds, 
                              &spec_data->sa_op.sin_addr.s_addr, &len, 
                              opt_label);
    if (rc != 0 && rc != TE_EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    rc = 0;
 
    spec_data->sa_op.sin_family = AF_INET;
    spec_data->sa_op.sin_port = 0;

    /* default read timeout */
    spec_data->read_timeout = 200000; /* FIXME */


    if (csap->type == TAD_CSAP_RAW)
    {
        /* opening incoming socket */
        spec_data->socket = socket(AF_INET, SOCK_RAW, IPPROTO_IP); 
        if (spec_data->socket < 0)
        {
            return TE_OS_RC(TE_TAD_CSAP, errno);
        }
        if (setsockopt(spec_data->socket, SOL_SOCKET, SO_REUSEADDR, 
                       &opt, sizeof(opt)) < 0)
        {
            return TE_OS_RC(TE_TAD_CSAP, errno);
        }

        csap->timeout          = 500000; /* FIXME */

    }
    else
    {
        spec_data->socket = -1;
    } 

    return 0;
}

/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_rw_destroy_cb(csap_p csap)
{
    tad_ip4_rw_data    *spec_data = csap_get_rw_data(csap);
     
    if (spec_data->socket >= 0)
    {
        close(spec_data->socket);    
        spec_data->socket = -1;
    }

    return 0;
}


/* See description tad_ipstack_impl.h */
int 
tad_ip4_read_cb(csap_p csap, int timeout, char *buf, size_t buf_len)
{
    fd_set              read_set;
    tad_ip4_rw_data    *spec_data;
    int                 ret; 
    
    struct timeval timeout_val;
    
    spec_data = csap_get_rw_data(csap); 

    if (spec_data->socket < 0)
    {
        return -1; /* TE_EIO */
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
    
    ret = select(spec_data->socket + 1, &read_set, NULL, NULL,
                 &timeout_val); 
    
    if (ret == 0)
        return 0;

    if (ret < 0)
        return -1;
    
    /* Note: possibly MSG_TRUNC and other flags are required */
    return recv(spec_data->socket, buf, buf_len, 0); 
}


/* See description tad_ipstack_impl.h */
te_errno
tad_ip4_write_cb(csap_p csap, const tad_pkt *pkt)
{
    tad_ip4_rw_data    *spec_data = csap_get_rw_data(csap);
    ssize_t             ret;
    struct msghdr       msg;
    size_t              iovlen = tad_pkt_get_seg_num(pkt);
    struct iovec        iov[iovlen];
    te_errno            rc;

    if (spec_data->socket < 0)
    {
        return TE_RC(TE_TAD_CSAP, TE_EIO);
    }

    /* Convert packet segments to IO vector */
    rc = tad_pkt_segs_to_iov(pkt, iov, iovlen);
    if (rc != 0)
    {
        ERROR("Failed to convert segments to IO vector: %r", rc);
        return rc;
    }

    msg.msg_name = (struct sockaddr *)&spec_data->sa_op;
    msg.msg_namelen = sizeof(spec_data->sa_op);
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    ret = sendmsg(spec_data->socket, &msg, 0);
    if (ret < 0) 
    {
        csap->last_errno = errno;
        return TE_OS_RC(TE_TAD_CSAP, csap->last_errno);
    }

    return 0;
}
