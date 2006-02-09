/** @file
 * @brief TAD iSCSI
 *
 * Traffic Application Domain Command Handler.
 * iSCSI CSAP, stack-related callbacks.
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

#define TE_LGR_USER     "TAD iSCSI"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "ndn.h"
#include "ndn_iscsi.h"
#include "asn_usr.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"

#include "tad_iscsi_impl.h"
#include "tad_utils.h"


/**
 * iSCSI layer read/write data
 */
typedef struct tad_iscsi_rw_data { 
    int     socket;
} tad_iscsi_rw_data;


/* See description tad_iscsi_impl.h */
te_errno
tad_iscsi_rw_init_cb(csap_p csap, const asn_value *csap_nds)
{
    te_errno            rc;
    int32_t             int32_val;
    asn_value          *iscsi_nds;
    tad_iscsi_rw_data  *rw_data; 


    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EWRONGPTR);

    rw_data = calloc(1, sizeof(*rw_data));
    if (rw_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    csap_set_rw_data(csap, rw_data);

    rc = asn_get_indexed(csap_nds, &iscsi_nds, 0, NULL);
    if (rc != 0)
    {
        ERROR("%s() get iSCSI csap spec failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    } 
    
    if ((rc = asn_read_int32(iscsi_nds, &int32_val, "socket")) != 0)
    {
        ERROR("%s(): asn_read_int32() failed for 'socket': %r", 
              __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    rw_data->socket = int32_val;

    return 0;
}

/* See description tad_iscsi_impl.h */
te_errno
tad_iscsi_rw_destroy_cb(csap_p csap)
{
    tad_iscsi_rw_data *rw_data; 

    rw_data = csap_get_rw_data(csap); 
    if (rw_data == NULL)
        return 0;

    csap_set_rw_data(csap, NULL); 

    close(rw_data->socket);

    free(rw_data);

    return 0;
}


/* See description tad_iscsi_impl.h */
te_errno
tad_iscsi_read_cb(csap_p csap, unsigned int timeout,
                  tad_pkt *pkt, size_t *pkt_len)
{
    tad_iscsi_rw_data      *rw_data = csap_get_rw_data(csap);
    tad_iscsi_layer_data   *layer_data =
        csap_get_proto_spec_data(csap, csap_get_rw_layer(csap));

    tad_pkt_seg    *seg = tad_pkt_first_seg(pkt);
    size_t          len = 0;
    te_errno        rc;

    F_ENTRY(CSAP_LOG_FMT "timeout=%u pkt=%p pkt_len=%p",
            CSAP_LOG_ARGS(csap), timeout, pkt, pkt_len);

    INFO("%s(CSAP %d) called, wait len %d, timeout %d",
         __FUNCTION__, csap->id, layer_data->wait_length, timeout);

    if (layer_data->wait_length == 0)
    {
        assert(layer_data->stored_length == 0);
        len = ISCSI_BHS_LENGTH;
    }
    else
    {
        assert(layer_data->wait_length > layer_data->stored_length);
        len = layer_data->wait_length - layer_data->stored_length;
    }

    if (seg == NULL)
    {
        seg = tad_pkt_alloc_seg(NULL, len, NULL);
        if (seg == NULL)
        {
            rc = TE_RC(TE_TAD_CSAP, TE_ENOMEM);
            return rc;
        }
        tad_pkt_append_seg(pkt, seg);
    }
    else if (seg->data_len < len)
    {
        void *mem = malloc(len);

        if (mem == NULL)
        {
            rc = TE_OS_RC(TE_TAD_CSAP, errno);
            assert(rc != 0);
            return rc;
        }
        VERB("%s(): reuse the first segment of packet", __FUNCTION__);
        tad_pkt_put_seg_data(pkt, seg,
                             mem, len, tad_pkt_seg_data_free);
    }

    {
        struct timeval tv;
        fd_set rset;
        int ret, fd = rw_data->socket;

        /* timeout in microseconds */
        TE_US2TV(timeout, &tv);

        FD_ZERO(&rset);
        FD_SET(fd, &rset);

        ret = select(fd + 1, &rset, NULL, NULL, &tv); 

        INFO("%s(CSAP %d): select on fd %d ret %d", 
             __FUNCTION__, csap->id, fd, ret);

        if (ret > 0)
        {
            ret = read(fd, seg->data_ptr, len);
            INFO("%s(CSAP %d): read ret %d", 
                 __FUNCTION__, csap->id, ret);

            if (ret == 0)
            {
                INFO(CSAP_LOG_FMT "Peer closed connection", 
                     CSAP_LOG_ARGS(csap));
                return TE_RC(TE_TAD_CSAP, TE_ETADENDOFDATA);
            }
            else if (ret > 0)
            {
                *pkt_len = ret;
                len = ret;
            }
            else
            {
                rc = te_rc_os2te(errno);
                WARN("%s(CSAP %d) error %r on read", __FUNCTION__, 
                     csap->id, rc);
                return TE_RC(TE_TAD_CSAP, rc);
            }
        }
        else if (ret == 0)
        {
            return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
        }
        else
        {
            rc = te_rc_os2te(errno);
            ERROR("%s(CSAP %d) select failed %r", __FUNCTION__,
                  csap->id, rc);
            return TE_RC(TE_TAD_CSAP, rc);
        }
    }
    INFO("%s(CSAP %d), return %d", __FUNCTION__, csap->id, len);

    layer_data->total_received += len;

    return 0;
}


/* See description tad_iscsi_impl.h */
te_errno
tad_iscsi_write_cb(csap_p csap, const tad_pkt *pkt)
{
#if 1
    const void *buf;
    size_t      buf_len;

    if (pkt == NULL || tad_pkt_seg_num(pkt) != 1)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    buf     = tad_pkt_first_seg(pkt)->data_ptr;
    buf_len = tad_pkt_first_seg(pkt)->data_len;
#endif
    int         fd;
    te_errno    rc = 0;
    ssize_t     sent;

    tad_iscsi_rw_data      *rw_data = csap_get_rw_data(csap);
    tad_iscsi_layer_data   *layer_data =
        csap_get_proto_spec_data(csap, csap_get_rw_layer(csap));;

    assert(csap != NULL);
    F_ENTRY("(%d:-) buf=%p buf_len=%u",
            csap->id, buf, (unsigned)buf_len);

    fd = rw_data->socket;

    switch (layer_data->send_mode)
    { 
        case ISCSI_SEND_USUAL:
            sent = send(fd, buf, buf_len, MSG_DONTWAIT);
            if (sent < 0)
            {
                rc = te_rc_os2te(errno); 
                if (rc == TE_EAGAIN)
                {
                    struct timeval tv = {0, 1000};

                    select(0, NULL, NULL, NULL, &tv); 
                    sent = send(fd, buf, buf_len, MSG_DONTWAIT);
                    if (sent < 0)
                        rc = te_rc_os2te(errno); 
                } 
            }
            break;

        case ISCSI_SEND_LAST:
            if ((rc = tad_tcp_push_fin(fd, buf, buf_len)) == 0)
            { 
                layer_data->send_mode = ISCSI_SEND_INVALID;
                sent = buf_len;
            }
            break;

        case ISCSI_SEND_INVALID:
            rc = TE_EPIPE;
            break;

        default:
            assert(FALSE);
    }

    if (rc != 0)
    {
        WARN("%s(CSAP %u) error %r on read", __FUNCTION__, csap->id, rc);
    } 
    else
    {
        INFO("%s(CSAP %u) written %d bytes to fd %d", 
             __FUNCTION__, csap->id, (int)sent, fd);
    }

    return TE_RC(TE_TAD_CSAP, rc);
}
