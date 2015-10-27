/** @file
 * @breaf TE TAD Forwarder additional module internal implementation
 *
 * @author Konstantin Ushakov <kostik@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __USE_UNIX98
#define __USE_UNIX98
#endif

#include "te_config.h" 
#include "config.h" 

#define TE_LGR_USER "TE Forw/SendQ"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "logger_api.h"
#include "logger_ta_fast.h"
#include "send_queue.h"
#include "tad_csap_support.h"

/** Types of messages received via sync pipe of the send queue */
typedef enum {
    TADF_SYNC_MSG_TYPE_WAKE = 0,    /**< Wakes up the sending thread */
    TADF_SYNC_MSG_TYPE_EXIT,        /**< Terminates the sending thread */
} tadf_sync_msg_t;

/** 
 * Sendq list. It contains pointers to all send queues. When somebody
 * wants to create a new sendq, we choose first free place
 * (sendq_list[place] == NULL) in the sendq_list and put pointer to the 
 * created sendq there. 
 */
static sendq_t         *sendq_list[TADF_SENDQ_LIST_SIZE_MAX];

/** As the sendq_list is a shared resource there should be a mutex lock */
static pthread_mutex_t  sendq_list_lock;

/** 
 * The macro determines what time is later
 */
#define MAX_TV(ts1, ts2) \
    ((timeval_compare(ts1, ts2) == 1) ? ts1 : ts2)

/**
 * Inserts the entry to the send queue due to it's send_time 
 *
 * @param queue     The queue to insert entry in.
 * @param entry     Pointer to the filled entry which should be inserter.
 *
 * @return          0 - if the function ends correctly
 *                  ENOBUFS - if the buffer is full
 */
static inline int 
tadf_sendq_entry_insert(sendq_t *queue, sendq_entry_t *entry)
{
    sendq_entry_t *current;
    
    pthread_mutex_lock(&queue->sendq_lock);
    current = queue->head;
    
    VERB("SendQ %d (csap %d), Adding a packet: sendq size = %d, "
         "sendq max size = %d", 
         queue->id, queue->csap->id,
         queue->queue_size, queue->queue_size_max);
    /* if the sendq is full then we simply exit */
    if (queue->queue_size_max <= queue->queue_size)
    {
        pthread_mutex_unlock(&queue->sendq_lock);
        return TE_RC(TE_TA_EXT, TE_ENOBUFS);
    }
    
    entry->next = NULL;
    entry->prev = NULL;
    
    queue->queue_size++;
    
    /* The sendq is empty */
    if (current == NULL)
    {
        queue->head = entry;
        queue->tail = entry;
        goto finish;
    }

    if (timeval_compare(entry->send_time, current->send_time) >= 0)
    {
        entry->next = current;
        current->prev = entry;
        queue->head = entry;
        goto finish;
    }
    current = current->next;
    
    while ((current != NULL) && 
           (timeval_compare(entry->send_time, current->send_time) == -1))
        current = current->next;

    if (current == NULL)
    {
        entry->prev = queue->tail;
        queue->tail->next = entry;
        queue->tail = entry;
    } 
    else
    {
        (current->prev)->next = entry;
        entry->prev = current->prev;
        entry->next = current;
        current->prev = entry;
    }
/* If we are here we heve inserted the entry */
finish:
    pthread_mutex_unlock(&queue->sendq_lock);
    return 0;
}

/**
 * Removes given entry from the send queue.
 *
 * @param queue     The send queue to remove entry from 
 * @param entry     The entry to remove from the queue 
 *
 * @return          0 - if the pointer was not NULL
 *                  TE_EWRONGPTR - otherwise
 */
static inline int 
tadf_sendq_entry_remove(sendq_t *queue, sendq_entry_t  *entry)
{
    
    pthread_mutex_lock(&queue->sendq_lock);

    if ((entry == NULL) || (entry->pkt == NULL))
    {
        WARN("NULL entry could not be removed from the queue");
        pthread_mutex_unlock(&queue->sendq_lock);
        return TE_RC(TE_TA_EXT, TE_EWRONGPTR);
    }
    
    if (entry->prev != NULL)
        (entry->prev)->next = entry->next;
    else
        queue->head = entry->next;
    if (entry->next != NULL)
        (entry->next)->prev = entry->prev;
    else
        queue->tail = entry->prev;
 
    queue->queue_size--;
    free(entry->pkt);
    free(entry);
    pthread_mutex_unlock(&queue->sendq_lock);
    return 0;
}

#if 0
/* this code is not used */
/**
 * Finds the entry in the send queue due to it's send time.
 * The function sets the entry pointer to the entry (in case
 * there are many entries with such send_time the pointer is
 * set to the entry which is later in the sendq).
 *
 * @param queue       The queue in which we will search.
 * @param entry       The pointer to the found entry.
 * @param send_time   The send time of the entry we should search for.
 *
 * @return            0 - if the function ends correctly and the entry
 *                        is found
 *                    TE_EINVAL - if there was no such entry
 */
static inline int 
tadf_sendq_entry_find(sendq_t *queue,
                      sendq_entry_t **entry,
                      struct timeval *send_time)
{
    sendq_entry_t *current;

    pthread_mutex_lock(&queue->sendq_lock);
    current = queue->head;

    while (current != NULL)
    {
        if ((current->send_time.tv_sec == send_time->tv_sec) &&
            (current->send_time.tv_usec == send_time->tv_usec))
        {
            *entry = &(current);
            pthread_mutex_unlock(&queue->sendq_lock);
            return 0;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&queue->sendq_lock);
    /* if we are here we failed to find the entry */
    return TE_RC(TE_TA_EXT, TE_ENOENT);
}
#endif

/**
 * This function initalizes the objects of the send queue:
 * 1) sendq parameters
 * 2) sendq mutex lock
 * 3) sending thread of the send queue
 * 4) sync pipe
 *
 * @param queue       Location for pointer to the initalised queue 
 *                    structure (OUT)
 * @param csap        Pointer to the descriptor of destination CSAP
 * @param bandwidth   Required Send Queue bandwidth
 * @param size_max    Required Send Queue buffer size
 *
 * @return             0 - if the function ends correctly
 *                    errno - otherwise
 */
int 
tadf_sendq_init(sendq_t **queue, csap_p csap, int bandwidth, int size_max)
{
    sendq_t *sendq = NULL;
    int      rc;
    static   pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
    
    sendq = calloc(1, sizeof(sendq_t));

    if (sendq == NULL)
        return TE_RC(TE_TA_EXT, TE_ENOMEM);

    /* Basic parameters initialisation */
    sendq->csap = csap;

    /* Default values */
    sendq->queue_size_max = size_max;
    sendq->queue_size = 0;
    sendq->queue_bandwidth = bandwidth;
    
    sendq->sendq_lock = lock;
    
    /* 
     * Sync channel init, sendq_sync_sockets[0] - for writing, 
     * sendq_sync_sockets[1] 
     */
    rc = socketpair(AF_LOCAL, SOCK_STREAM, 0, sendq->sendq_sync_sockets);
    if (rc != 0)
    {
        ERROR("Failed to create socket connection in the send queue");
        goto error;
    }

    /* Sending thread creation */
    rc = pthread_create(&sendq->send_thread, NULL, 
                        &tadf_sendq_send_thread_main, sendq);
    if (rc != 0)
    {
        ERROR("Failed to create sending thread in the send queue");
        goto error;
    }

    *queue = sendq;
    return 0;

error:
    free(sendq);
    return TE_RC(TE_TA_EXT, rc);
}

/**
 * This function destroyes the send queue and all related objects.
 * 
 * @param sendq        The send queue to be destroyed.
 *
 * @return              0 - if the function ends correctly
 *                     errno - otherwise
 */
int 
tadf_sendq_free(sendq_t *sendq)
{
    int      rc = -1;
    void    *thread_rc = NULL;
    uint8_t  msg_type = TADF_SYNC_MSG_TYPE_EXIT;
    
    sendq_entry_t *current = sendq->head;
    sendq_entry_t *next;
    
    rc = write(sendq->sendq_sync_sockets[0], &msg_type, sizeof(msg_type));
    if (rc == -1)
    {
        ERROR("Failed to send message to the send queue send thread");
        return TE_RC(TE_TA_EXT, rc);
    }

    pthread_join(sendq->send_thread, &thread_rc);
    if (thread_rc == NULL)
        ERROR("The send thread of the send queue failed to exit correctly");
    
    /* Closing the socket connection */
    if (close(sendq->sendq_sync_sockets[0]) < 0)
        ERROR("Failed to close socket connection  of the send queue");

    if (close(sendq->sendq_sync_sockets[1]) < 0)
        ERROR("Failed to close socket connection  of the send queue");
    
    /* Removing the sendq entries */
    while (current != NULL)
    {
        next = current->next;
        tadf_sendq_entry_remove(sendq, current);
        current = next;
    }
    
    /* Destroing the mutex lock */
    if (pthread_mutex_destroy(&sendq->sendq_lock) != 0)
        ERROR("Failed to destroy mutex lock in the send queue");

    free(sendq);
    return 0;
}

/**
 * This function puts packet in the send queue due to the send time.
 * If the packet inserted to the top of the send queue, the function
 * informs  sending thread via send queue sync pipe.
 *
 * @param queue          The queue the packet should be putted to.
 * @param pkt            Packet data.
 * @param pkt_len        The length of the packet data, including headers.
 * @param mdump          The time packet should be send at.
 *
 * @return                0 - if the packet was inserted correctly
 *                       errno - otherwise
 */
int 
tadf_sendq_put_pkt(sendq_t *queue,
                   const uint8_t *pkt,
                   const size_t pkt_len,
                   struct timeval mdump)
{
    int            rc = -1;
    uint8_t        msg_type = TADF_SYNC_MSG_TYPE_WAKE;

    sendq_entry_t *entry = (sendq_entry_t *)malloc(sizeof(sendq_entry_t));

    if (pkt == NULL)
    {
        free(entry);
        WARN("Wrong data pointer");
        return TE_RC(TE_TA_EXT, TE_EWRONGPTR);
    }

    entry->pkt = (uint8_t *)malloc(pkt_len);
    if (entry->pkt == NULL)
        return TE_RC(TE_TA_EXT, TE_ENOMEM);

    memcpy(entry->pkt, pkt, pkt_len);
    entry->pkt_len = pkt_len;
    entry->send_time = mdump;

    pthread_mutex_lock(&queue->sendq_lock);
    /* Insertig the packet */
    rc = tadf_sendq_entry_insert(queue, entry);
    if (rc != 0)
    {
        WARN("Failed to insert the entry in the send queue of the "
             "Forwarder CSAP");
        pthread_mutex_unlock(&queue->sendq_lock);
        goto error;
    }

    /* 
     * If we inserter the packet to the top of the send queue, the function
     * informs  sending thread via send queue sync pipe. 
     */
    if (entry->next == NULL)
    {
        pthread_mutex_unlock(&queue->sendq_lock);
        rc = write(queue->sendq_sync_sockets[0], &msg_type, 
                   sizeof(msg_type));
        VERB("Sending sync message to the main sending thread, rc = %d", 
             rc);
        if (rc < 0)
        {
            ERROR("Failed to send message to the thread of the send queue");
            goto error;
        }
        return 0;
    }
    pthread_mutex_unlock(&queue->sendq_lock);
    return 0;
error: 
    free(entry->pkt);
    free(entry);
    return TE_RC(TE_TA_EXT, rc);
}

/**
 * Gets the parameter of the send queue due to parameter name.
 *
 * @param sendq_id        The ID of the queue the parameter of which 
 *                        should be
 *                        outputed.
 * @param param_spec      The name of the parameter in human-readable form.
 *
 * @return                Value of the parameter or -1 if there is no
 *                        parameter with such name.
 */
int
tadf_sendq_get_param(int sendq_id,
                     const char *param_spec)
{
    sendq_t *queue;

    queue = tadf_sendq_find(sendq_id);
    if (queue == NULL)
    {
        WARN("No send queue with such ID");
        return -1;
    }
    if (strncmp("size_max", param_spec, strlen("size_max")) == 0)
    {
         return queue->queue_size_max;
    }
    else if (strncmp("size", param_spec, strlen("size_max")) == 0)
    {
        return queue->queue_size;
    }
    else if (strncmp("bandwidth", param_spec, strlen("bandwidth")) == 0)
    {
        return queue->queue_bandwidth;
    } else
    {
        return -1;
    }
}

/**
 * Sets the parameter of the send queue due to parameter name and
 * new value.
 *
 * @param sendq_id        The ID of the queue the parameter of which 
 *                        should be changed.
 * @param param_spec      The name of the parameter in human-readable form.
 * @param value           New value of the parameter.* 
 * 
 * @return                0 - if the function ends correctly
 *                        TE_EINVAL - if there is no such parameter
 */
int 
tadf_sendq_set_param(int sendq_id,
                     const char *param_spec, int value)
{
    sendq_t *queue;

    queue = tadf_sendq_find(sendq_id);
    if (queue == NULL)
    {
        WARN("No send queue with such ID");
        return TE_RC(TE_TA_EXT, TE_EINVAL);
    }

    pthread_mutex_lock(&queue->sendq_lock);

    if (strncmp("size_max", param_spec, strlen("size_max")) == 0)
    {
        queue->queue_size_max = value;
        VERB("Max size of the sendq with ID %d changed to %d",
             sendq_id, value);
    }
    else if (strncmp("bandwidth", param_spec, strlen("bandwidth")) == 0)
    {
        VERB("Bandwidth of the sendq with ID %d changed to %d", 
             sendq_id, value);
        queue->queue_bandwidth = value;
    } else
    {
        VERB("No such sendq parameter");
        pthread_mutex_unlock(&queue->sendq_lock);
        return TE_RC(TE_TA_EXT, TE_EINVAL);
    }
    pthread_mutex_unlock(&queue->sendq_lock);
    return 0;
}

/** Fake function to free segment data - does nothing */
static void 
tadf_seg_free_fake(void *ptr, size_t len)
{
    UNUSED(ptr);
    UNUSED(len);
}

/** Fake function to free segment control - does nothing */
static void 
tadf_pkt_ctrl_free_fake(void *ptr)
{
    UNUSED(ptr);
}

/** 
 * Send a packet from the queue tail.
 *
 * @param sendq         queue to send packet from
 *
 * @return Status code.
 */
static te_errno
tadf_send_pkt(sendq_t *sendq)
{
     tad_pkt     pkt;
     tad_pkt_seg seg;
     
     memset(&pkt, 0, sizeof(pkt));
     pkt.my_free = tadf_pkt_ctrl_free_fake;
     tad_pkt_init_segs(&pkt);
     
     memset(&seg, 0, sizeof(seg));
     seg.my_free = tadf_pkt_ctrl_free_fake;
     tad_pkt_init_seg_data(&seg, sendq->tail->pkt, 
                           sendq->tail->pkt_len, tadf_seg_free_fake);

     tad_pkt_append_seg(&pkt, &seg);
     
     return sendq->csap->layers[csap_get_rw_layer(sendq->csap)].
                proto_support->write_cb(sendq->csap, &pkt);
}

/**
 * The main sending thread function. The function sends packets
 * to their destination using write_cb of the corresponding CSAP.
 *
 * @param queue         The queue the thread should work with
 *
 * @return              Not NULL - if the thread exits correctly
 *                      NULL - otherwise
 */
void *
tadf_sendq_send_thread_main(void *queue)
{
    sendq_t *sendq = (sendq_t *)queue;
    struct   timeval curr_time;
    struct   timeval sleep_tv;

    uint8_t  msg_type = TADF_SYNC_MSG_TYPE_WAKE;

    int      rc;

    sendq_entry_t *tail;

    long long  time_stamp;

    fd_set read_fd;

    for (;;)
    {
        pthread_mutex_lock(&sendq->sendq_lock);

        tail = sendq->tail;
        gettimeofday(&curr_time, NULL);

        /* In case we have a limited bandwidth */
        if (sendq->queue_bandwidth > 0)
        {
            VERB("SendQ %d (csap %d), Limited bw case",
                 sendq->id, sendq->csap->id);
            /*
             * We should check, if we can send packets now due to
             * the bandwidth parameter 
             */
            if (timeval_compare(sendq->bandwidth_ts, curr_time) > 0)
            {
                /* The time we still should sleep */
                SUB_TV(sleep_tv, sendq->bandwidth_ts, curr_time);
                VERB("SendQ %d (csap %d), "
                     "We still have to sleep to some time, "
                     "sendq->bandwidth_ts: (%d.%d)",
                     sendq->id, sendq->csap->id,
                     sendq->bandwidth_ts.tv_sec,
                     sendq->bandwidth_ts.tv_usec);
                goto sleep;
            }

            memset(&(sendq->bandwidth_ts), 0, sizeof(struct timeval));

            if (tail == NULL)
            {
                memset(&sleep_tv, 0, sizeof(struct timeval));
                goto sleep;
            }
            else
                SUB_TV(sleep_tv, tail->send_time, curr_time);

            if (timeval_compare(curr_time, tail->send_time) >= 0)
            {
                struct timeval ts_diff;

                VERB("SendQ %d (csap %d), send_time=(%d, %d), bandwidth=%d",
                     sendq->id, sendq->csap->id,
                     tail->send_time.tv_sec, tail->send_time.tv_usec,
                     sendq->queue_bandwidth);
                /*
                 * The sleep time is maximum of the time we should sleep
                 * due to the bandwidth and to the time till next packet
                 * should be sent
                 */
                time_stamp =
                    (tail->pkt_len * 1000000) / sendq->queue_bandwidth;
                sleep_tv.tv_sec = time_stamp / 1000000;
                sleep_tv.tv_usec = time_stamp % 1000000;

                /*
                 * We should set bandwidth wake up time. If next time we
                 * will wake up earlier than senq->bandwidth_ts we can't
                 * send any packets.
                 */
                ADD_TV(sendq->bandwidth_ts, curr_time, sleep_tv);

                /*
                 * If there are no packets to send for some time we can
                 * simply sleep until there are some or until we are awaken
                 */
                if (tail->prev != NULL)
                {
                    SUB_TV(ts_diff, tail->prev->send_time, curr_time);
                    sleep_tv =  MAX_TV(sleep_tv, ts_diff);
                }

                /* Sending packet using Ethernet CSAP write_cb */
                rc = tadf_send_pkt(sendq);
                if (rc != 0)
                {
                    ERROR("SendQ %d (csap %d), Failed to send data via "
                          "Ethernet CSAP: %r",
                          sendq->id, sendq->csap->id, rc);

                    pthread_mutex_unlock(&sendq->sendq_lock);
                    return NULL;
                }
                F_VERB("SendQ %d (csap %d), csap write cb return %d "
                       "for pkt with len %d", 
                       sendq->id, sendq->csap->id,
                       rc, tail->pkt_len);
                F_VERB("SendQ %d (csap %d), Packet sent: %d, %d , "
                       "sendq size = %d\n", 
                       sendq->id, sendq->csap->id,
                       tail->send_time.tv_sec, tail->send_time.tv_usec,
                       sendq->queue_size);

                tadf_sendq_entry_remove(sendq, tail);
            }
        }
        else
        {
            /*
             * We should send all packets with send_time less or equal
             * current time
             */
            memset(&sleep_tv, 0, sizeof(sleep_tv));
            while ((tail != NULL) &&
                   (timeval_compare(curr_time, tail->send_time) >= 0))
            {
                /* Sending packet using Ethernet CSAP write_cb */
                rc = tadf_send_pkt(sendq);
                if (rc != 0)
                {
                    ERROR("SendQ %d (csap %d), Failed to send data via "
                          "Ethernet CSAP: %r",
                          sendq->id, sendq->csap->id, rc);

                    pthread_mutex_unlock(&sendq->sendq_lock);
                    return NULL;
                }

                tadf_sendq_entry_remove(sendq, tail);
                VERB("SendQ %d (csap %d), Packet sent: %d, %d , "
                     "sendq size = %d\n",
                     sendq->id, sendq->csap->id,
                     tail->send_time.tv_sec, tail->send_time.tv_usec,
                     sendq->queue_size);

                tail = sendq->tail;
            }

            /*
             * If during our sleep somebody will change the sendq_bandwidth
             * this thing will help us not to became crazy
             */
            memset(&sendq->bandwidth_ts, 0, sizeof(struct timeval));
            if (tail != NULL)
                SUB_TV(sleep_tv, tail->send_time, curr_time);
        }

sleep:
        FD_ZERO(&read_fd);
        FD_SET(sendq->sendq_sync_sockets[1], &read_fd);

        pthread_mutex_unlock(&sendq->sendq_lock);

        /*
         * Now we sleep until any message is received via communication
         * channel of the send queue
         */
        if (IS_ZERO(sleep_tv))
        {
            VERB("SendQ %d (csap %d), Going to unlimited sleep",
                 sendq->id, sendq->csap->id);
            rc = select(sendq->sendq_sync_sockets[1] + 1, &read_fd,
                        NULL, NULL, NULL);
        }
        else
        {
            VERB("SendQ %d (csap %d), Going to sleep, time = (%d, %d)",
                 sendq->id, sendq->csap->id,
                 sleep_tv.tv_sec, sleep_tv.tv_usec);
            rc = select(sendq->sendq_sync_sockets[1] + 1, &read_fd,
                        NULL, NULL, &sleep_tv);
        }
        if (rc < 0)
        {
            ERROR("SendQ %d (csap %d), Select() in main send thread "
                  "returned %d, errno = %d",
                  sendq->id, sendq->csap->id,
                  errno);
            return NULL;
        }

        /*
         * If we received TADF_SYNC_MSG_TYPE_EXIT type message via
         * sync pipe we exit the thread
         */
        if (rc > 0)
        {
            rc = read(sendq->sendq_sync_sockets[1],
                      &msg_type, sizeof(msg_type));
            if (rc < 0)
            {
                ERROR("SendQ %d (csap %d), Failed to receive sync message "
                      "in the main send thread, rc = %d",
                      sendq->id, sendq->csap->id, rc);
                return NULL;
            }

            if (msg_type == TADF_SYNC_MSG_TYPE_EXIT)
                return sendq;
        }
    }

    return sendq;
}

/**
 * This function inits the sendq_list and
 * sendq list mutex lock.
 */
void
tadf_sendq_list_create(void)
{
    /* all pointers are initalized to NULL */
    memset(sendq_list, 0, sizeof(sendq_list));

    pthread_mutex_init(&sendq_list_lock, NULL);
}

/**
 * Function creates sendq in the sendq_list.
 * Function choses the first free place in the sendq_list and puts the
 * created sendq there.
 *
 * @param csap_id   Id of CSAP via which the data should be sent
 * @param bandwidth   Required Send Queue bandwidth
 * @param size_max    Required Send Queue buffer size
 *
 * @return          ID of the created sendq if the function ends correctly, 
 *                  or -1 on error
 */
int
tadf_sendq_create(csap_handle_t csap_id, int bandwidth, int size_max)
{
    int      sendq_id = 0;
    int      rc;
    sendq_t *sendq;
    
    csap_p   csap = csap_find(csap_id);

    if (csap == NULL)
    {
        ERROR("%s failed: csap %d not found", __FUNCTION__, csap_id);
        return -1;
    }

    if (csap->layers[csap_get_rw_layer(csap)].
            proto_support->prepare_send_cb != NULL)
    {
        rc = csap->layers[csap_get_rw_layer(csap)].
                 proto_support->prepare_send_cb(csap);
        if (rc != 0)
        {
            ERROR("Failed to prepare csap for sending, rc %r", rc);
            return -1;
        }
    }

    pthread_mutex_lock(&sendq_list_lock);
    /* We should find first NULL pointer */
    while (sendq_id < TADF_SENDQ_LIST_SIZE_MAX &&
           sendq_list[sendq_id] != NULL)
        sendq_id++;
    
    if (sendq_id == TADF_SENDQ_LIST_SIZE_MAX)
        return -1;

    /* Initalizing sendq */
    rc = tadf_sendq_init(&sendq, csap, bandwidth, size_max);
    if (rc != 0)
    {
        pthread_mutex_unlock(&sendq_list_lock);
        ERROR("Failed to init sendq, rc %X", rc);
        return -1;
    }
    
    sendq_list[sendq_id] = sendq;
    sendq->id = sendq_id;
    pthread_mutex_unlock(&sendq_list_lock);

    INFO("Sendq #%d created for CSAP %d", sendq_id, csap_id);
    return sendq_id;
}

/**
 * Function returns pointer to the sendq with the corresponding ID.
 *
 * @param sendq_id         ID of the sendq to search for.
 *
 * @return                 Pointer to the sendq, even if it is NULL,
 *                         NULL is also returned in case the sendq_id
 *                         is out of range
 */
sendq_t *
tadf_sendq_find(int sendq_id)
{
    if ((sendq_id < 0) || (sendq_id >= TADF_SENDQ_LIST_SIZE_MAX))
    {
        ERROR("Invalid sendq_id passed to the tadf_sendq_find()");
        return NULL;
    }
    return sendq_list[sendq_id];
}

/**
 * Function destroyes the sendq due to the sendq ID.
 * The sendq_list[ID] is set to NULL.
 *
 * @param sendq_id     The ID of the sendq to be destroyed.
 *
 * @return             0 - if the function ends correctly
 *                     TE_EINVAL - if there is no sendq with such ID
 */
int
tadf_sendq_destroy(int sendq_id)
{
    sendq_t *sendq;
    
    pthread_mutex_lock(&sendq_list_lock);
    /* Find sendq due to the sendq_id */
    sendq = tadf_sendq_find(sendq_id);
    if (sendq == NULL)
    {
        WARN("Trying to destroy a non existing sendq");
        pthread_mutex_unlock(&sendq_list_lock);
        return TE_RC(TE_TA_EXT, TE_EINVAL);
    }
    
    /* Removing sendq related objects */
    tadf_sendq_free(sendq);
    
    /* 
     * Sendq_list[sendq_id] should be set to NULL, that means that now that
     * id is free for use 
     */
    sendq_list[sendq_id] = NULL;
    
    pthread_mutex_unlock(&sendq_list_lock);
    return 0;
}

/**
 * Function destroyes the sendq list. All sendqs in the list are also
 * destroyed.
 *
 * @return            0 - if the list was destroyed correctly
 *                   errno - otherwise
 */
int
tadf_sendq_list_destroy(void)
{
    int rc;
    
    if ((rc = tadf_sendq_list_clear()) != 0)
    {
        WARN("Failed to destroy sendq list");
        return TE_RC(TE_TA_EXT, rc);
    }
    
    pthread_mutex_destroy(&sendq_list_lock);
    
    return 0;
}

/**
 * Function cleares the sendq list. All sendqs from are destroyed.
 *
 * @return            0 - if the list was destroyed correctly
 *                   errno - otherwise
 */
int
tadf_sendq_list_clear(void)
{
    int i;
    int rc;
    
    for (i = 0; i < TADF_SENDQ_LIST_SIZE_MAX; i++)
    {
        if (sendq_list[i] != NULL)
        {
            rc = tadf_sendq_destroy(i);
            if (rc < 0)
            {
                ERROR("Failed to clear sendq list");
                return TE_RC(TE_TA_EXT, rc);
            }
        }
        sendq_list[i] = NULL;
    }
    return 0;
}
