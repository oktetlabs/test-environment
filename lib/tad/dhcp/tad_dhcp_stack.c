/** @file
 * @brief TAD DHCP
 *
 * Traffic Application Domain Command Handler.
 * DHCP CSAP, stack-related callbacks.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "TAD DHCP"

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

#include "logger_api.h"
#include "tad_dhcp_impl.h"


/* See description tad_dhcp_impl.h */
te_errno
tad_dhcp_rw_init_cb(csap_p csap, const asn_value *csap_nds)
{
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
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) 
    {
        rc = 0;
    }

    if (rc)
    {
        tad_dhcp_rw_destroy_cb(csap);
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

    if (rc != 0)
    {
        tad_dhcp_rw_destroy_cb(csap);
        return rc;
    }

    opt = 1;
    if (setsockopt(dhcp_spec_data->out, SOL_SOCKET, SO_BROADCAST, 
                   (void *)&opt, sizeof(opt)) != 0)
    {
        tad_dhcp_rw_destroy_cb(csap);
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

    csap_set_rw_data(csap, dhcp_spec_data);

    return 0;
}


/* See description tad_dhcp_impl.h */
te_errno
tad_dhcp_rw_destroy_cb(csap_p csap)
{
    dhcp_csap_specific_data_t *spec_data = csap_get_rw_data(csap); 
     
    if (spec_data->in >= 0)
        close(spec_data->in);    

    if (spec_data->out >= 0)
        close(spec_data->out);    
    return 0;
}

 
/* See description tad_dhcp_impl.h */
te_errno
tad_dhcp_read_cb(csap_p csap, unsigned int timeout,
                 tad_pkt *pkt, size_t *pkt_len)
{
    dhcp_csap_specific_data_t  *spec_data = csap_get_rw_data(csap);
    te_errno                    rc;

    rc = tad_common_read_cb_sock(csap, spec_data->in, 0, timeout,
                                 pkt, NULL, NULL, pkt_len);

    return rc;
}


/* See description tad_dhcp_impl.h */
te_errno
tad_dhcp_write_cb(csap_p csap, const tad_pkt *pkt)
{
    dhcp_csap_specific_data_t  *spec_data;

    struct sockaddr_in  dest;
    ssize_t             ret;
    struct msghdr       msg;
    size_t              iovlen = tad_pkt_seg_num(pkt);
    struct iovec        iov[iovlen];
    te_errno            rc;

    assert(csap != NULL);
    spec_data = csap_get_rw_data(csap); 

    if (spec_data->out < 0)
    {
        ERROR(CSAP_LOG_FMT "no output socket", CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_EIO);
    }

    /* Convert packet segments to IO vector */
    rc = tad_pkt_segs_to_iov(pkt, iov, iovlen);
    if (rc != 0)
    {
        ERROR("Failed to convert segments to IO vector: %r", rc);
        return rc;
    }

    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(spec_data->mode == DHCP4_CSAP_MODE_SERVER ? 
                          DHCP_CLIENT_PORT : DHCP_SERVER_PORT);
    dest.sin_addr.s_addr = INADDR_BROADCAST;

    msg.msg_name = SA(&dest);
    msg.msg_namelen = sizeof(dest);
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    ret = sendmsg(spec_data->out, &msg, 0);
    if (ret < 0) 
    {
        return TE_OS_RC(TE_TAD_CSAP, errno);
    }

    return 0;
}
