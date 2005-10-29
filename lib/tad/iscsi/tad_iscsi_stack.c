/** @file
 * @brief iSCSI TAD
 *
 * Traffic Application Domain Command Handler
 * iSCSI CSAP, stack-related callbacks.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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

#define TE_LGR_USER     "TAD iSCSI stack"

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


/**
 * Prepare send callback
 *
 * @param csap_descr    CSAP descriptor structure. 
 *
 * @return status code.
 */ 
int
tad_iscsi_prepare_send_cb(csap_p csap_descr)
{
    assert(csap_descr != NULL);
    F_ENTRY("(%d:)", csap_descr->id);
    return 0;
}

/**
 * Prepare recv callback
 *
 * @param csap_descr    CSAP descriptor structure. 
 *
 * @return status code.
 */ 
int
tad_iscsi_prepare_recv_cb(csap_p csap_descr)
{
    assert(csap_descr != NULL);
    F_ENTRY("(%d:)", csap_descr->id);
    return 0;
}


/* See description tad_iscsi_impl.h */
int 
tad_iscsi_read_cb(csap_p csap_descr, int timeout,
                  char *buf, size_t buf_len)
{
    iscsi_csap_specific_data_t *spec_data;

    size_t    len = 0;

    assert(csap_descr != NULL);
    F_ENTRY("(%d:-) timeout=%d buf=%p buf_len=%u",
            csap_descr->id, timeout, buf, (unsigned)buf_len);

    spec_data = csap_descr->layers[0].specific_data; 

    INFO("%s(CSAP %d) called, wait len %d, timeout %d",
         __FUNCTION__, csap_descr->id,
         spec_data->wait_length, timeout);

    if (spec_data->wait_length == 0)
        len = ISCSI_BHS_LENGTH;
    else
        len = spec_data->wait_length -
              spec_data->stored_length;

    if (buf_len > len)
        buf_len = len;

    {
        struct timeval tv = {0, 0};
        fd_set rset;
        int rc, fd = spec_data->socket;

        /* timeout in microseconds */
        tv.tv_usec = timeout % (1000 * 1000);
        tv.tv_sec =  timeout / (1000 * 1000);

        FD_ZERO(&rset);
        FD_SET(fd, &rset);

        rc = select(fd + 1, &rset, NULL, NULL, &tv); 

        INFO("%s(CSAP %d): select on fd %d rc %d", 
             __FUNCTION__, csap_descr->id, fd, rc);

        if (rc > 0)
        {
            rc = read(fd, buf, buf_len);
            INFO("%s(CSAP %d): read rc %d", 
                 __FUNCTION__, csap_descr->id, rc);
            if (rc >= 0)
                len = rc;
            else
            {
                csap_descr->last_errno = errno;
                WARN("%s(CSAP %d) error %d on read", __FUNCTION__, 
                     csap_descr->id, csap_descr->last_errno);
                return -1;
            }
        }
        else 
            len = 0;
    }
    INFO("%s(CSAP %d), return %d", __FUNCTION__, csap_descr->id, len);

    return len;
}


/* See description tad_iscsi_impl.h */
int 
tad_iscsi_write_cb(csap_p csap_descr, const char *buf, size_t buf_len)
{
    iscsi_csap_specific_data_t *spec_data;

    assert(csap_descr != NULL);
    F_ENTRY("(%d:-) buf=%p buf_len=%u",
            csap_descr->id, buf, (unsigned)buf_len);

    spec_data = csap_descr->layers[0].specific_data; 

    {
        struct timeval tv = {0, 1000};
        int rc, fd = spec_data->socket;

        rc = send(fd, buf, buf_len, MSG_DONTWAIT);

        if (rc < 0)
        {
            if (errno == EAGAIN)
            {
                select(0, NULL, NULL, NULL, &tv); 
                rc = send(fd, buf, buf_len, MSG_DONTWAIT);
            } 
        }

        if (rc < 0)
        {
            csap_descr->last_errno = errno;
            WARN("%s(CSAP %d) error %d on read", __FUNCTION__, 
                 csap_descr->id, csap_descr->last_errno);
            return -1;
        } 
        INFO("%s(CSAP %d) written %d bytes to fd %d", 
             __FUNCTION__, csap_descr->id, rc, fd);
    }

    return buf_len;
}


/* See description tad_iscsi_impl.h */
int
tad_iscsi_write_read_cb(csap_p csap_descr, int timeout,
                        const char *w_buf, size_t w_buf_len,
                        char *r_buf, size_t r_buf_len)
{
    int rc = -1; 

    assert(csap_descr != NULL);
    F_ENTRY("(%d:-) w_buf=%p w_buf_len=%u timeout=%d r_buf=%p "
            "r_buf_len=%u", csap_descr->id,
            w_buf, (unsigned)w_buf_len, timeout,
            r_buf, (unsigned)r_buf_len);
#if 1
    csap_descr->last_errno = TE_EOPNOTSUPP;
    ERROR("%s() not allowed for iSCSI", __FUNCTION__);
#else
    rc = iscsi_write_cb(csap_descr, w_buf, w_buf_len);
    if (rc != -1)  
    {
        rc = iscsi_read_cb(csap_descr, timeout, r_buf, r_buf_len);
    }
#endif
    return rc;
}


/* See description tad_iscsi_impl.h */
te_errno
tad_iscsi_single_init_cb(int csap_id, const asn_value *csap_nds,
                         unsigned int layer)
{
    csap_p      csap_descr;
    te_errno    rc;
    int32_t     int32_val;

    const asn_value            *iscsi_nds;
    iscsi_csap_specific_data_t *spec_data; 


    ENTRY("(%d:%u) csap_nds=%p", csap_id, layer, (void *)csap_nds);

    if (csap_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EWRONGPTR);

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ETADCSAPNOTEX);

    spec_data = calloc(1, sizeof(*spec_data));
    if (spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = asn_get_indexed(csap_nds, &iscsi_nds, 0);
    if (rc != 0)
    {
        ERROR("%s() get iSCSI csap spec failed %r", __FUNCTION__, rc);
        free(spec_data);
        return TE_RC(TE_TAD_CSAP, rc);
    } 
    
    if ((rc = asn_read_int32(iscsi_nds, &int32_val, "socket")) != 0)
    {
        ERROR("%s(): asn_read_int32() failed for 'socket': %r", 
              __FUNCTION__, rc);
        free(spec_data);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    spec_data->socket = int32_val;

    if ((rc = asn_read_int32(iscsi_nds, &int32_val, "header-digest")) != 0)
    {
        ERROR("%s(): asn_read_bool() failed for 'header-digest': %r", 
              __FUNCTION__, rc);
        free(spec_data);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    spec_data->hdig = int32_val;

    if ((rc = asn_read_int32(iscsi_nds, &int32_val, "data-digest")) != 0)
    {
        ERROR("%s(): asn_read_bool() failed for 'data-digest': %r", 
              __FUNCTION__, rc);
        free(spec_data);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    spec_data->ddig = int32_val;

    csap_descr->layers[layer].specific_data = spec_data;
    csap_descr->layers[layer].get_param_cb = tad_iscsi_get_param_cb;

    csap_descr->read_cb          = tad_iscsi_read_cb;
    csap_descr->write_cb         = tad_iscsi_write_cb;
    csap_descr->write_read_cb    = tad_iscsi_write_read_cb;
    csap_descr->prepare_recv_cb  = tad_iscsi_prepare_recv_cb;
    csap_descr->prepare_send_cb  = tad_iscsi_prepare_send_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 100 * 1000; /* FIXME */


    return 0;
}


/* See description tad_iscsi_impl.h */
te_errno
tad_iscsi_single_destroy_cb(int csap_id, unsigned int layer)
{
    csap_p                      csap_descr; 
    iscsi_csap_specific_data_t *spec_data; 

    ENTRY("(%d:%u)", csap_id, layer);

    csap_descr = csap_find(csap_id);
    if (csap_descr == NULL)
        return TE_ETADCSAPNOTEX;

    spec_data = csap_descr->layers[layer].specific_data; 
    csap_descr->layers[layer].specific_data = NULL; 

    close(spec_data->socket);

    free(spec_data);

    return 0;
}
