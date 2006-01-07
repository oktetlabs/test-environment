/** @file
 * @brief TAD Socket
 *
 * Traffic Application Domain Command Handler.
 * Socket CSAP stack-related callbacks.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "TAD Socket"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"
#include "ndn.h"
#include "ndn_socket.h"

#include "tad_csap_inst.h"
#include "tad_utils.h"
#include "tad_socket_impl.h"


/* See description tad_socket_impl.h */
te_errno
tad_socket_rw_init_cb(csap_p csap, const asn_value *csap_nds)
{
    te_errno            rc; 
    unsigned int        layer = csap_get_rw_layer(csap);
    const asn_value    *csap_spec = csap->layers[layer].csap_layer_pdu; 
    tad_socket_rw_data *spec_data; 
    
    const asn_value    *data_csap_spec, *subval;
    asn_tag_class       t_class;
    int32_t             value_in_pdu;
    struct sockaddr_in  local;
    int                 opt = 1;


    UNUSED(csap_nds);
    
    spec_data = calloc(1, sizeof(*spec_data));
    if (spec_data == NULL)
    {
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    csap_set_rw_data(csap, spec_data);


    rc = asn_get_child_value(csap_spec, &data_csap_spec, 
                             PRIVATE, NDN_TAG_SOCKET_TYPE);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        ERROR("%s(CSAP %d) socket csap have to have 'type' spec",
              __FUNCTION__, csap->id); 
        return TE_ETADWRONGNDS;
    }
    else if (rc != 0)
    {
        ERROR("%s(CSAP %d): unexpected error reading 'data': %r", 
              __FUNCTION__, csap->id, rc); 
        return rc;
    } 

    rc = asn_get_choice_value(data_csap_spec, &subval, 
                              &t_class, &(spec_data->data_tag));
    if (rc != 0)
    {
        ERROR("%s(CSAP %d): error reading choice of 'data': %r", 
              __FUNCTION__, csap->id, rc); 
        return rc;
    } 

    INFO("tag of socket CSAP: %d, socket tag is %d", 
         spec_data->data_tag, NDN_TAG_SOCKET_TYPE_FD);
    if (spec_data->data_tag == NDN_TAG_SOCKET_TYPE_FD)
    {
        struct sockaddr_storage   remote_sa;
        socklen_t                 remote_len = sizeof(remote_sa);

        asn_read_int32(subval, &(spec_data->socket), "");

        if (getpeername(spec_data->socket, SA(&remote_sa), &remote_len)
            < 0)
            WARN("%s(CSAP %d) getpeername(sock %d) failed, errno %d", 
                 __FUNCTION__, csap->id, 
                 spec_data->socket, errno);
        else
        {
            spec_data->remote_port = SIN(&remote_sa)->sin_port;
            spec_data->remote_addr = SIN(&remote_sa)->sin_addr;

            RING("init CSAP on accepted connection from %s:%d", 
                 inet_ntoa(spec_data->remote_addr), 
                 ntohs(spec_data->remote_port));
        }

        /* nothing more to do */
        return 0;
    }


    /*
     * Set local port
     */
    rc = ndn_du_read_plain_int(csap_spec, NDN_TAG_SOCKET_LOCAL_PORT,
                               &value_in_pdu);
    if (rc == 0)
    {
        VERB("%s(): set TCP CSAP %d default local port to %d", 
             __FUNCTION__, csap->id, value_in_pdu);
        spec_data->local_port = value_in_pdu;
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB("%s(): set TCP CSAP %d default local port to zero", 
             __FUNCTION__, csap->id);
        spec_data->local_port = 0;
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNOTHERCHOICE)
    {
        ERROR("%s(): TCP CSAP %d, non-plain local port not supported",
              __FUNCTION__, csap->id);
        return TE_EOPNOTSUPP;
    }
    else
        return rc;

    /*
     * Set remote port
     */
    rc = ndn_du_read_plain_int(csap_spec, NDN_TAG_SOCKET_REMOTE_PORT,
                               &value_in_pdu);
    if (rc == 0)
    {
        VERB("%s(): set TCP CSAP %d default remote port to %d", 
             __FUNCTION__, csap->id, value_in_pdu);
        spec_data->remote_port = value_in_pdu;
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB("%s(): set TCP CSAP %d default remote port to zero", 
             __FUNCTION__, csap->id);
        spec_data->remote_port = 0;
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNOTHERCHOICE)
    {
        ERROR("%s(): TCP CSAP %d, non-plain remote port not supported",
              __FUNCTION__, csap->id);
        return TE_EOPNOTSUPP;
    }
    else
        return rc;

    /* TODO: support of TCP over IPv6 */

    local.sin_family = AF_INET;
    local.sin_addr = spec_data->local_addr;
    local.sin_port = htons(spec_data->local_port);

    if ((spec_data->socket = socket(AF_INET,
                                    (spec_data->data_tag ==
                                         NDN_TAG_SOCKET_TYPE_UDP) ?
                                    SOCK_DGRAM : SOCK_STREAM, 0)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_CSAP, errno);
        ERROR("%s(CSAP %d) socket create failed, errno %r", 
              __FUNCTION__, csap->id, rc);
        return rc;
    }
    INFO(CSAP_LOG_FMT "opened socket %d", CSAP_LOG_ARGS(csap),
         spec_data->socket);

    if (setsockopt(spec_data->socket, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) != 0)
    {
        rc = TE_OS_RC(TE_TAD_CSAP, errno);
        ERROR("%s(CSAP %d) set SO_REUSEADDR failed, errno %r", 
              __FUNCTION__, csap->id, rc);
        return rc;
    }
    VERB(CSAP_LOG_FMT "SO_REUSEADDR is enabled", CSAP_LOG_ARGS(csap));

    if (bind(spec_data->socket, SA(&local), sizeof(local)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_CSAP, errno);
        ERROR("%s(CSAP %d) socket bind failed, errno %r", 
              __FUNCTION__, csap->id, rc);
        return rc;
    }
    INFO(CSAP_LOG_FMT "bound to %s:%u", CSAP_LOG_ARGS(csap),
         inet_ntoa(local.sin_addr), ntohs(local.sin_port));

    switch (spec_data->data_tag)
    {
        case NDN_TAG_SOCKET_TYPE_TCP_SERVER:
            if (listen(spec_data->socket, 10) < 0)
            {
                rc = TE_OS_RC(TE_TAD_CSAP, errno);
                ERROR("%s(CSAP %d) listen failed, errno %r", 
                      __FUNCTION__, csap->id, rc);
                return rc;
            }
            INFO(CSAP_LOG_FMT "listen() success", CSAP_LOG_ARGS(csap));
            break;

#if 0
        case NDN_TAG_SOCKET_TYPE_TCP_CLIENT: 
            {
                struct sockaddr_in remote;

                if (spec_data->remote_port == 0 ||
                    spec_data->remote_addr.s_addr == INADDR_ANY)
                {
                    ERROR("%s(CSAP %d) client csap, remote need", 
                          __FUNCTION__, csap->id);
                    return TE_ETADWRONGNDS;
                }
                remote.sin_family = AF_INET;
                remote.sin_port = spec_data->remote_port;
                remote.sin_addr = ip4_spec_data->remote_addr;

                if (connect(spec_data->socket, SA(&remote), 
                            sizeof(remote)) < 0)
                {
                    rc = TE_OS_RC(TE_TAD_CSAP, errno);
                    ERROR("%s(CSAP %d) connect failed, errno %r", 
                          __FUNCTION__, csap->id, rc);
                    return rc;
                }
            }
            break;
#endif

        default:
            ERROR("%s(CSAP %d) unexpected tag of 'data' field %d", 
                  __FUNCTION__, csap->id, spec_data->data_tag);
            return TE_ETADWRONGNDS;
    }

    csap->timeout = 100000;

    return 0;
}

/* See description tad_socket_impl.h */
te_errno
tad_socket_rw_destroy_cb(csap_p csap)
{
    tad_socket_rw_data *spec_data = csap_get_rw_data(csap); 

    if (spec_data == NULL)
    {
        WARN("Not socketernet CSAP %d special data found!", csap->id);
        return 0;
    }
    csap_set_rw_data(csap, NULL); 

    if (spec_data->socket >= 0)
    {
        close(spec_data->socket);
        spec_data->socket = -1;
    }

    return 0;
}


/* See description tad_socket_impl.h */
int 
tad_socket_read_cb(csap_p csap, int timeout, char *buf, size_t buf_len)
{
    tad_socket_rw_data *spec_data = csap_get_rw_data(csap);

    fd_set              read_set;
    struct timeval      timeout_val;
    int                 ret;

    if (spec_data->socket < 0)
    {
        ERROR("%s(): no input socket", __FUNCTION__);
        return -1;
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
    VERB("%s(): timeout set to %u.%u", __FUNCTION__, 
         timeout_val.tv_sec, timeout_val.tv_usec);

    ret = select(spec_data->socket + 1, &read_set, NULL, NULL,
                     &timeout_val); 

    VERB("%s(): select return %d", __FUNCTION__,  ret);
    
    if (ret == 0)
        return 0;

    if (ret < 0)
    {
        VERB("select fails: spec_data->socket = %d", spec_data->socket);
        csap->last_errno = errno;
        return -1;
    }

    if (spec_data->data_tag == NDN_TAG_SOCKET_TYPE_TCP_SERVER)
    {
        ret = accept(spec_data->socket, NULL, NULL);

        if (ret < 0)
        {
            csap->last_errno = errno;
            return -1;
        }
        INFO("%s(CSAP %d) TCP 'server', accepted socket %d", 
             __FUNCTION__, csap->id, ret);

        assert(buf_len >= sizeof(int));
        /* FIXME: Not-aligned access */
        *((int *)buf) = ret; 

        return sizeof(int);
    } 
    else 
    {
        if (spec_data->wait_length > 0)
        {
            size_t rest_length = spec_data->wait_length -
                                 spec_data->stored_length;

            if (buf_len > rest_length)
                buf_len = rest_length;
        }

        /* Note: possibly MSG_TRUNC and other flags are required */
        ret = recv(spec_data->socket, buf, buf_len, 0); 
        if (ret == 0)
        {
            INFO(CSAP_LOG_FMT "Peer closed connection", 
                 CSAP_LOG_ARGS(csap));
            csap->last_errno = TE_ETADENDOFDATA;
            return -1;
        }
        return ret;
    }
}


/* See description tad_socket_impl.h */
te_errno
tad_socket_write_cb(csap_p csap, const tad_pkt *pkt)
{
    tad_socket_rw_data *spec_data = csap_get_rw_data(csap);
    size_t              iovlen = tad_pkt_get_seg_num(pkt);
    struct iovec        iov[iovlen];
    struct msghdr       msg;
    te_errno            rc;
    ssize_t             ret;


    /* Capabilities have to be verified by confirm_tmpl_cb */
    assert(spec_data->data_tag != NDN_TAG_SOCKET_TYPE_TCP_SERVER);

    if (spec_data->socket < 0)
    {
        ERROR("%s(): no output socket", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    /* Convert packet segments to IO vector */
    rc = tad_pkt_segs_to_iov(pkt, iov, iovlen);
    if (rc != 0)
    {
        ERROR("Failed to convert segments to IO vector: %r", rc);
        return rc;
    }

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
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
#if 0 /* Old UDP-data write callback */
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
#endif
}
