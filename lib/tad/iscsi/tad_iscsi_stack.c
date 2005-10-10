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

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#else
#error cannot compile without pthread library
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "tad_iscsi_impl.h"

#define TE_LGR_USER     "TAD iSCSI stack"
#include "logger_ta.h"


#define LOCK_QUEUE(_spec_data) \
    do {                                                                \
        int rc_;                                                        \
                                                                        \
        rc_ = pthread_mutex_trylock(&_spec_data->pkt_queue_lock);       \
        if (rc_ == EBUSY)                                               \
        {                                                               \
            INFO("mutex is locked, wait...");                           \
            rc_ = pthread_mutex_lock(&_spec_data->pkt_queue_lock);      \
        }                                                               \
                                                                        \
        if (rc_ != 0)                                                   \
            ERROR("mutex operation failed %d", rc_);                    \
    } while (0)

#define UNLOCK_QUEUE(_spec_data) \
    do {                                                                \
        int rc_;                                                        \
                                                                        \
        rc_ = pthread_mutex_unlock(&_spec_data->pkt_queue_lock);        \
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

    packet_t *recv_pkt = NULL;
    size_t    len = 0;
    int       i;
    
    if (csap_descr == NULL)
    {
        return -1;
    }
    iscsi_spec_data = csap_descr->layers[0].specific_data; 

    RING("%s(CSAP %d) called", __FUNCTION__, csap_descr->id);

    for (i = 0; i < 2; i++)
    {
        LOCK_QUEUE(iscsi_spec_data);
        if (iscsi_spec_data->packets_from_tgt.cqh_first !=
            (void *)(&iscsi_spec_data->packets_from_tgt))
        {
            recv_pkt = iscsi_spec_data->packets_from_tgt.cqh_first;
            CIRCLEQ_REMOVE(&iscsi_spec_data->packets_from_tgt, 
                           recv_pkt, link);
        }
        UNLOCK_QUEUE(iscsi_spec_data); 
        if (recv_pkt == NULL && i == 0)
            usleep(timeout); /* wait a bit */
        RING("%s(CSAP %d), recv_pkt 0x%X", __FUNCTION__, csap_descr->id,
             recv_pkt);
    }

    if (recv_pkt != NULL)
    {
        len = recv_pkt->length;
        if (buf_len < recv_pkt->length)
        {
            WARN("%s(): packet received is too long: %d > buffer %d",
                 __FUNCTION__, recv_pkt->length, buf_len);
            len = buf_len;
        }
        memcpy(buf, recv_pkt->buffer, len); 

        free(recv_pkt->buffer);
        free(recv_pkt);
    }
    RING("%s(CSAP %d), return %d", __FUNCTION__, csap_descr->id, len);

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
    packet_t *send_pkt;
    
    if (csap_descr == NULL)
    {
        return -1;
    }

    iscsi_spec_data = csap_descr->layers[0].specific_data; 

    if ((send_pkt = calloc(1, sizeof(*send_pkt))) == NULL)
    {
        csap_descr->last_errno = TE_ENOMEM;
        return -1;
    }
    send_pkt->allocated_buffer = send_pkt->buffer = malloc(buf_len);
    send_pkt->length = buf_len;
    memcpy(send_pkt->buffer, buf, buf_len);
    

    LOCK_QUEUE(iscsi_spec_data);
    CIRCLEQ_INSERT_TAIL(&iscsi_spec_data->packets_to_tgt,
                        send_pkt, link);
    UNLOCK_QUEUE(iscsi_spec_data);

    write(iscsi_spec_data->conn_fd[1], "a", 1); 

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
    pthread_t                     send_thread = 0; 
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

    CIRCLEQ_INIT(&(iscsi_spec_data->packets_to_tgt));
    CIRCLEQ_INIT(&(iscsi_spec_data->packets_from_tgt));

    {
        pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
        iscsi_spec_data->pkt_queue_lock = fastmutex;
    }

    pipe(iscsi_spec_data->conn_fd); 

    thread_params = calloc(1, sizeof(*thread_params));
    thread_params->send_recv_csap = csap_descr->id;

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
    packet_t                   *recv_pkt = NULL; 
    iscsi_csap_specific_data_t *iscsi_spec_data; 
    void                       *iscsi_retval;

    csap_descr = csap_find(csap_id);

    if (csap_descr == NULL)
        return TE_ETADCSAPNOTEX;

    iscsi_spec_data = csap_descr->layers[layer].specific_data; 

    pthread_cancel(iscsi_spec_data->iscsi_target_thread);
    pthread_join(iscsi_spec_data->iscsi_target_thread, &iscsi_retval);

    LOCK_QUEUE(iscsi_spec_data);

    /* clear queue to target */
    while (!CIRCLEQ_EMPTY(&iscsi_spec_data->packets_to_tgt))
    { 
        recv_pkt = iscsi_spec_data->packets_to_tgt.cqh_first;

        free(recv_pkt->buffer);

        CIRCLEQ_REMOVE(&iscsi_spec_data->packets_to_tgt, 
                       recv_pkt, link);
        free(recv_pkt);
    }

    /* clear queue frœm target */
    while (!CIRCLEQ_EMPTY(&iscsi_spec_data->packets_from_tgt))
    { 
        recv_pkt = iscsi_spec_data->packets_from_tgt.cqh_first;

        free(recv_pkt->buffer);

        CIRCLEQ_REMOVE(&iscsi_spec_data->packets_from_tgt, 
                       recv_pkt, link);
        free(recv_pkt);
    }

    close(iscsi_spec_data->conn_fd[0]);
    close(iscsi_spec_data->conn_fd[1]);

    UNLOCK_QUEUE(iscsi_spec_data);

    csap_descr->layers[layer].specific_data = NULL; 

    free(iscsi_spec_data);


    return rc;
}


/**
 * Internal method for get some data from msg queue CSAP->Target. 
 * This method read no more then asked buf_len, if first pkt in queue 
 * is less then buf_len, it copies to the user buffer all data from pkt. 
 *
 * If first pkt in queue is greater then buf_len, it copies to the buffer
 * aѕked number of bytes, and rest in queue tail of packet. 
 *
 * @param csap_descr    pointer to CSAP descriptor
 * @param buffer        location for read data
 * @param buf_len       length of buffer
 *
 * @return number of read bytes, zero if CSAP destroyed or -1 on error.
 */
static inline int
iscsi_tad_recv_internal(csap_p csap_descr, uint8_t *buffer, size_t buf_len)
{
    iscsi_csap_specific_data_t *iscsi_spec_data;

    char        b;
    size_t      len = 0;
    packet_t   *recv_pkt = NULL; 
    uint8_t    *received_buffer = NULL;

    INFO("%s(CSAP %d): want %d bytes", __FUNCTION__, csap_descr->id, buf_len);

    iscsi_spec_data = csap_descr->layers[0].specific_data; 

    read(iscsi_spec_data->conn_fd[0], &b, 1); 

    if (b == 'z')
    {
        RING("%s(CSAP %d): Csap will be destroyed, last 'z' byte read",
             __FUNCTION__, csap_descr->id);
        return 0;
    }

    LOCK_QUEUE(iscsi_spec_data);
    if (iscsi_spec_data->packets_to_tgt.cqh_first !=
        (void *)(&iscsi_spec_data->packets_to_tgt))
    {
        recv_pkt = iscsi_spec_data->packets_to_tgt.cqh_first;

        len = recv_pkt->length;
        received_buffer = recv_pkt->buffer;

        if (buf_len < len)
        {
            INFO("%s(CSAP %d): received %d, asked %d, cut it",
                 __FUNCTION__, csap_descr->id, recv_pkt->length, buf_len);
            len = buf_len;
            recv_pkt->length -= buf_len;
            recv_pkt->buffer += buf_len;
            write(iscsi_spec_data->conn_fd[1], "a", 1); 
        }
        else 
            CIRCLEQ_REMOVE(&iscsi_spec_data->packets_to_tgt, 
                           recv_pkt, link);
    }
    UNLOCK_QUEUE(iscsi_spec_data); 

    if (recv_pkt != NULL)
    { 
        INFO("%s(CSAP %d): copy to user %d bytes from 0x%x addr", 
             __FUNCTION__, csap_descr->id, len, received_buffer);
        memcpy(buffer, received_buffer, len); 

        /* If all data from pkt was read, free memory */
        if (received_buffer == recv_pkt->buffer) 
        { 
            free(recv_pkt->allocated_buffer);
            free(recv_pkt);
        }
    } 

    return len;
}

/* see description in tad-iscsi_impl.h */
int
iscsi_tad_recv(int csap_id, uint8_t *buffer, size_t buf_len)
{
    csap_p  csap_descr;
    int     rx_loop = 0;
    size_t  rx_total = 0;


    csap_descr = csap_find(csap_id); 

    if (csap_descr == NULL || 
        csap_descr->layers[0].specific_data == NULL) 
    {
        return 0; 
    } 

    INFO("%s(CSAP %d): want %d bytes", __FUNCTION__, csap_id, buf_len);

    do {
        rx_loop = iscsi_tad_recv_internal(csap_descr, buffer, buf_len);
        if (rx_loop <= 0)
        {
            WARN("%s(CSAP %d) internal recv return %d ",
                 __FUNCTION__, csap_id, rx_loop);
            break;
        }
        if ((unsigned)rx_loop > buf_len)
        {
            WARN("%s() internal recv return %d, greater then buf_len",
                 __FUNCTION__, rx_loop);
            return -1;
        }

        buffer += rx_loop;
        buf_len -= rx_loop;
        rx_total += rx_loop;
    } while (buf_len > 0);



    return rx_total;

}


int
iscsi_tad_send(int csap_id, uint8_t *buffer, size_t buf_len)
{
    csap_p    csap_descr;
    packet_t *send_pkt;

    iscsi_csap_specific_data_t *iscsi_spec_data;

    INFO("%s(CSAP %d): send %d bytes", __FUNCTION__, csap_id, buf_len); 
    
    csap_descr = csap_find(csap_id);

    if (csap_descr == NULL)
    {
        ERROR("%s() CSAP %d not found", __FUNCTION__, csap_id);
        return -1;
    }

    iscsi_spec_data = csap_descr->layers[0].specific_data; 

    if ((send_pkt = calloc(1, sizeof(*send_pkt))) == NULL)
    {
        csap_descr->last_errno = TE_ENOMEM;
        return -1;
    }
    send_pkt->buffer = malloc(buf_len);
    send_pkt->length = buf_len;
    memcpy(send_pkt->buffer, buffer, buf_len);
    INFO("%s(CSAP %d): insert to out queue %d bytes from 0x%x addr", 
         __FUNCTION__, csap_id, buf_len, buffer);
    

    LOCK_QUEUE(iscsi_spec_data);
    CIRCLEQ_INSERT_TAIL(&iscsi_spec_data->packets_from_tgt,
                        send_pkt, link);
    UNLOCK_QUEUE(iscsi_spec_data);


    return buf_len;
}
