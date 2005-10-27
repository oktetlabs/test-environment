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

#define TE_LGR_USER     "TAD iSCSI stack"

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

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#else
#error cannot compile without pthread library
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "tad_iscsi_impl.h"

#include "logger_api.h"


#define LOCK_QUEUE(_queue) \
    do {                                                                \
        int rc_;                                                        \
                                                                        \
        rc_ = pthread_mutex_trylock(&((_queue)->pkt_queue_lock));       \
        if (rc_ == EBUSY)                                               \
        {                                                               \
            INFO("mutex is locked, wait...");                           \
            rc_ = pthread_mutex_lock(&((_queue)->pkt_queue_lock));      \
        }                                                               \
                                                                        \
        if (rc_ != 0)                                                   \
            ERROR("mutex operation failed %d", rc_);                    \
    } while (0)

#define UNLOCK_QUEUE(_queue) \
    do {                                                                \
        int rc_;                                                        \
                                                                        \
        rc_ = pthread_mutex_unlock(&((_queue)->pkt_queue_lock));        \
                                                                        \
        if (rc_ != 0)                                                   \
            ERROR("mutex operation failed %d", rc_);                    \
    } while (0)
 

#ifndef CIRCLEQ_EMPTY
#define CIRCLEQ_EMPTY(head_) \
    ((void *)((head_)->cqh_first) == (void *)(head_))
#endif



/**
 * Prepare send callback
 *
 * @param csap_descr    CSAP descriptor structure. 
 *
 * @return status code.
 */ 
int
iscsi_prepare_send_cb(csap_p csap_descr)
{
#if 0
    iscsi_csap_specific_data_t *iscsi_spec_data; 

    if (csap_descr == NULL)
        return TE_ETADCSAPNOTEX;

    iscsi_spec_data = csap_descr->layers[0].specific_data; 

#else
    UNUSED(csap_descr);
#endif
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
iscsi_prepare_recv_cb(csap_p csap_descr)
{
#if 0
    iscsi_csap_specific_data_t *iscsi_spec_data; 
    int rc = 0;

    if (csap_descr == NULL)
        return TE_ETADCSAPNOTEX;

    iscsi_spec_data = csap_descr->layers[0].specific_data; 


    return rc;
#else
    UNUSED(csap_descr);
    return 0;
#endif
}

/**
 * Callback for read data from media of iSCSI CSAP. 
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
iscsi_read_cb(csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{
    iscsi_csap_specific_data_t *iscsi_spec_data;

    size_t    len = 0;
    
    if (csap_descr == NULL)
    {
        return -1;
    }
    iscsi_spec_data = csap_descr->layers[0].specific_data; 

    INFO("%s(CSAP %d) called, wait len %d, timeout %d",
         __FUNCTION__, csap_descr->id,
         iscsi_spec_data->wait_length, timeout);

    if (iscsi_spec_data->wait_length == 0)
        len = ISCSI_BHS_LENGTH;
    else
        len = iscsi_spec_data->wait_length -
              iscsi_spec_data->stored_length;

    if (buf_len > len)
        buf_len = len;

    {
        struct timeval tv = {0, 0};
        fd_set rset;
        int rc, fd = iscsi_spec_data->conn_fd[CSAP_SITE];

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
    }
    INFO("%s(CSAP %d), return %d", __FUNCTION__, csap_descr->id, len);

    return len;
}

/**
 * Callback for write data to media of iSCSI CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
iscsi_write_cb(csap_p csap_descr, const char *buf, size_t buf_len)
{
    iscsi_csap_specific_data_t *iscsi_spec_data;
    
    if (csap_descr == NULL)
    {
        return -1;
    }

    iscsi_spec_data = csap_descr->layers[0].specific_data; 

    {
        struct timeval tv = {0, 1000};
        int rc, fd = iscsi_spec_data->conn_fd[CSAP_SITE];

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

/**
 * Callback for write data to media of iSCSI CSAP and read
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
iscsi_write_read_cb(csap_p csap_descr, int timeout,
                    const char *w_buf, size_t w_buf_len,
                    char *r_buf, size_t r_buf_len)
{
#if 1
    csap_descr->last_errno = TE_EOPNOTSUPP;
    ERROR("%s() not allowed for iSCSI", __FUNCTION__);
    UNUSED(timeout);
    UNUSED(w_buf);
    UNUSED(w_buf_len);
    UNUSED(r_buf);
    UNUSED(r_buf_len);
    return -1;
#else
    int rc; 
    
    rc = iscsi_write_cb(csap_descr, w_buf, w_buf_len);
    
    if (rc == -1)  
        return rc;
    else 
        return iscsi_read_cb(csap_descr, timeout, r_buf, r_buf_len);;
#endif
}

/**
 * Callback for init iscsi CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
iscsi_single_init_cb(int csap_id, const asn_value *csap_nds, int layer)
{
    pthread_attr_t                pthread_attr; 
    iscsi_target_thread_params_t *thread_params;

    csap_p  csap_descr;
    int     rc;

    const asn_value            *iscsi_nds;
    iscsi_csap_specific_data_t *iscsi_spec_data; 


    if (csap_nds == NULL)
        return TE_EWRONGPTR;

    if ((csap_descr = csap_find(csap_id)) == NULL)
        return TE_ETADCSAPNOTEX;

    INFO("%s(CSAP %d, layer %d) called", __FUNCTION__, csap_id, layer); 

    rc = asn_get_indexed(csap_nds, &iscsi_nds, 0);
    if (rc != 0)
    {
        ERROR("%s() get iSCSI csap spec failed %r", __FUNCTION__, rc);
        return rc;
    } 
    
    if ((iscsi_spec_data = calloc(1, sizeof(iscsi_csap_specific_data_t)))
        == NULL)
        return TE_ENOMEM; 

    csap_descr->layers[layer].specific_data = iscsi_spec_data;
    csap_descr->layers[layer].get_param_cb = iscsi_get_param_cb;

    csap_descr->read_cb         = iscsi_read_cb;
    csap_descr->write_cb        = iscsi_write_cb;
    csap_descr->write_read_cb   = iscsi_write_read_cb;
    csap_descr->prepare_recv_cb = iscsi_prepare_recv_cb;
    csap_descr->prepare_send_cb = iscsi_prepare_send_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 100 * 1000;

    if ((rc = socketpair(AF_LOCAL, SOCK_STREAM, 0, 
                         iscsi_spec_data->conn_fd)) < 0)
    {
        csap_descr->last_errno = errno;
        ERROR("%s(CSAP %d) socketpair failed %d", 
              __FUNCTION__, csap_id, csap_descr->last_errno);
        return csap_descr->last_errno;
    }

    thread_params = calloc(1, sizeof(*thread_params));
    thread_params->send_recv_sock = iscsi_spec_data->conn_fd[TGT_SITE];

    if ((rc = pthread_attr_init(&pthread_attr)) != 0 ||
        (rc = pthread_attr_setdetachstate(&pthread_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0)
    {
        ERROR("Cannot initialize pthread attribute variable: %d", rc);
        return rc;
    } 

    rc = pthread_create(&iscsi_spec_data->iscsi_target_thread, 
                        &pthread_attr,
                        iscsi_server_rx_thread, thread_params);
    if (rc != 0)
    {
        ERROR("Cannot create a new iSCSI thread: %d", rc);
        return rc; 
    } 
    
    return 0;
}

/**
 * Callback for destroy iSCSI CSAP layer  if single in stack.
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
iscsi_single_destroy_cb(int csap_id, int layer)
{
    int                         rc = 0;
    csap_p                      csap_descr; 
    iscsi_csap_specific_data_t *iscsi_spec_data; 

    csap_descr = csap_find(csap_id);

    if (csap_descr == NULL)
        return TE_ETADCSAPNOTEX;

    iscsi_spec_data = csap_descr->layers[layer].specific_data; 

    pthread_cancel(iscsi_spec_data->iscsi_target_thread);
    close(iscsi_spec_data->conn_fd[CSAP_SITE]);
    /* leave TGT_SITE socket, there may be read or write */

    csap_descr->layers[layer].specific_data = NULL; 

    free(iscsi_spec_data);


    return rc;
}
