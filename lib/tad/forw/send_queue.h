/** @file
 * @breaf TE TAD Forwarder additional module internal declarations
 *
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_FORWARDER_SEND_QUEUE_H__
#define __TE_FORWARDER_SEND_QUEUE_H__

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* For 'struct timeval' */ 
#endif
#include "tad_csap_inst.h"

/** The maximum number of send queues created on the forwarder host */
#define TADF_SENDQ_LIST_SIZE_MAX 1000

/** The macro determines if the time val is zero */
#define IS_ZERO(ts)                    \
    (ts.tv_sec == 0 && ts.tv_usec == 0)

/** Amount of microseconds in one second */
#define TV_RADIX 1000000

/**
 * Calculates the sum of timevals, ts1 + ts2.
 *
 * @param res     The result timeval.
 * @param ts1     Time stamp to be reduced 
 * @param ts2     On which value to reduce 
 */
#define ADD_TV(res, ts1, ts2)                                       \
    do {                                                            \
        int64_t usecs = (ts1.tv_sec + ts2.tv_sec) * 1000000 +       \
                        (ts1.tv_usec + ts2.tv_usec);                \
        res.tv_sec = usecs / 1000000;                               \
        res.tv_usec = usecs % 1000000;                              \
    } while(0)

/**
 * Calculates substraction of timevals, ts1 - ts2.
 *
 * @param res     The result timeval.
 * @param ts1     Time stamp to be reduced 
 * @param ts2     On which value to reduce 
 */
#define SUB_TV(res, ts1, ts2)                                       \
    do {                                                            \
        int64_t usecs = (ts1.tv_sec - ts2.tv_sec) * 1000000 +       \
                        (ts1.tv_usec - ts2.tv_usec);                \
        res.tv_sec = usecs / 1000000;                               \
        res.tv_usec = usecs % 1000000;                              \
    } while(0)

/** Send queue entry structure */
typedef struct sendq_entry_s {
    struct sendq_entry_s *next; /**< previous packet in the queue */
    struct sendq_entry_s *prev; /**< next packet in the queue     */

    uint8_t        *pkt;        /**< packet data                  */
    size_t          pkt_len;    /**< packet data size             */
    struct timeval  send_time;  /**< packet send time             */
} sendq_entry_t;

/**
 * Send queue structure. Packets from the tail of the queue should 
 * be sent first
 */
typedef struct sendq_s {
    sendq_entry_t  *head; /**< head of the queue */
    sendq_entry_t  *tail; /**< tail of the queue */

    int             id;   /**< Unique send queue identifier */

    pthread_t       send_thread; /**< sending thread id */
    
    csap_p          csap;        /**< csap used by this sendq to send data */
    pthread_mutex_t sendq_lock;  /**< mutex lock for the shared data       */
   
    int             sendq_sync_sockets[2]; /**< pipe for sync of threads */

    int             queue_size_max;  /**< queue max size     */
    int             queue_size;      /**< queue current size */
    int             queue_bandwidth; /**< sendq bandwidth    */

    struct timeval  bandwidth_ts;    /**< bandwidth timestamp */
} sendq_t;

/**
 * This function inits the sendq_list and
 * sendq list mutex lock.
 */
extern void tadf_sendq_list_create(void);

/**
 * Function destroyes the sendq list. All sendqs in the list are also
 * destroyed.
 *
 * @return            0 - if the list was destroyed correctly
 *                   errno - otherwise
 */
extern int tadf_sendq_list_destroy(void);

/**
 * Function cleares the sendq list. All sendqs from are destroyed.
 *
 * @return            0 - if the list was destroyed correctly
 *                   errno - otherwise
 */
extern int tadf_sendq_list_clear(void);


/**
 * Function returns pointer to the sendq with the corresponding ID.
 *
 * @param sendq_id         ID of the sendq to search for.
 *
 * @return                 Pointer to the sendq, even if it is NULL,
 *                         NULL is also returned in case the sendq_id
 *                         is out of range
 */
extern sendq_t *tadf_sendq_find(int sendq_id);

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
extern int tadf_sendq_create(csap_handle_t csap_id, 
                             int bandwidth, int size_max);

/**
 * Function destroyes the sendq due to the sendq ID.
 * The sendq_list[ID] is set to NULL.
 *
 * @param sendq_id     The ID of the sendq to be destroyed.
 *
 * @return             0 - if the function ends correctly
 *                     TE_EINVAL - if there is no sendq with such ID
 */
extern int tadf_sendq_destroy(int sendq_id);

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
extern int tadf_sendq_set_param(int sendq_id,
                                const char *param_spec, int value);

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
extern int tadf_sendq_get_param(int sendq_id,
                                const char *param_spec);


/* sendq interface */
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
extern int tadf_sendq_init(sendq_t **queue, csap_p csap,
                           int bandwidth, int size_max);

/**
 * This function destroyes the send queue and all related objects.
 * 
 * @param sendq        The send queue to be destroyed.
 *
 * @return              0 - if the function ends correctly
 *                     errno - otherwise
 */
extern int tadf_sendq_free(sendq_t *queue);

/**
 * The main sending thread function. The function sends packets
 * to their destination using write_cb of the corresponding CSAP.
 *
 * @param queue         The queue the thread should work with
 *
 * @return               0 - if the thread exits correctly
 *                      -1 - otherwise
 */
extern void *tadf_sendq_send_thread_main(void *queue);

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
extern int tadf_sendq_put_pkt(sendq_t *queue,
                              const uint8_t *pkt,
                              const size_t pkt_len,
                              struct timeval mdump);

/**
 * Compares two time values.
 *
 * @param tv1           First timeval
 * @param tv2           Second timeval
 *
 * @retval  -1 if tv1 is earlier then tv2.
 * @retval   0 if tv1 is equal to tv2.
 * @retval   1 if tv1 is later then tv2.
 */
static inline int
timeval_compare(struct timeval tv1, struct timeval tv2)
{
    if (tv1.tv_sec > tv2.tv_sec)
        return 1;
    else if (tv1.tv_sec < tv2.tv_sec)
        return -1;

    /* seconds are equal, compare microseconds */
    if (tv1.tv_usec > tv2.tv_usec)
        return 1;
    else if (tv1.tv_usec < tv2.tv_usec)
        return -1;

    return 0;
}

#endif /* __TE_FORWARDER_SEND_QUEUE_H__ */
