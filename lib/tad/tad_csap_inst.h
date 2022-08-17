/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD CSAP Instance
 *
 * Traffic Application Domain Command Handler.
 * Declarations of CSAP instance types and functions, used in common and
 * protocol-specific modules implemnting TAD.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_CSAP_INST_H__
#define __TE_TAD_CSAP_INST_H__

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tad_common.h"

#include "tad_types.h"
#include "tad_pkt.h"
#include "tad_send_recv.h"
#include "tad_send.h"
#include "tad_recv.h"
#include "tad_poll.h"


/**
 * Locks access to CSAP shared flags and data.
 * If already locked, wait until unlocked.
 *
 * @param csap_inst_   CSAP instance
 */
#define CSAP_LOCK(csap_inst_) \
    do {                                                        \
        int rc_ = pthread_mutex_lock(&((csap_inst_)->lock));    \
                                                                \
        if (rc_ != 0)                                           \
            ERROR("%s(): pthread_mutex_lock() failed %r",       \
                  __FUNCTION__, te_rc_os2te(rc_));              \
    } while (0)

/**
 * Try to lock access to CSAP shared flags and data.
 *
 * @param csap_inst_   CSAP instance
 * @param rc_          Variable for return code
 */
#define CSAP_TRYLOCK(csap_inst_, rc_) \
    do {                                                        \
        (rc_) = pthread_mutex_trylock(&((csap_inst_)->lock));   \
        if (((rc_) != 0) && ((rc_) != EBUSY))                   \
            ERROR("%s(): pthread_mutex_trylock() failed %r",    \
                  __FUNCTION__, te_rc_os2te(rc_));              \
    } while (0)

/**
 * Unlocks access to CSAP shared flags and data.
 *
 * @param csap_inst_   CSAP instance
 */
#define CSAP_UNLOCK(csap_inst_)\
    do {                                                        \
        int rc_ = pthread_mutex_unlock(&((csap_inst_)->lock));  \
                                                                \
        if (rc_ != 0)                                           \
            ERROR("%s(): pthread_mutex_unlock() failed %r",     \
                  __FUNCTION__, te_rc_os2te(rc_));              \
    } while (0)


/* Forward declaration */
struct csap_spt_type_t;


/**
 * Collection of common protocol layer attributes of CSAP.
 */
typedef struct csap_layer_t {
    char               *proto;               /**< Protocol layer text label */
    te_tad_protocols_t  proto_tag;           /**< Protocol layer int tag */

    void        *specific_data;              /**< Protocol-specific data */
    te_bool      rw_use_tad_pkt_seg_tagging; /**< This layer has to make use of
                                                  layer tag field in TAD packet
                                                  segment control blocks during
                                                  read-write opearation */
    asn_value   *nds;                        /**< ASN.1 value with CSAP
                                                  specification layer PDU */

    asn_value   *pdu;                        /**< Current value of PDU on
                                                  this layer to be sent.
                                                  It might be useful to allow
                                                  one layer to set/update PDU
                                                  fields of another layer.
                                                  (This field is used for
                                                   traffic templates only) */

    struct csap_spt_type_t *proto_support;   /**< Protocol layer
                                                  support descriptor */
} csap_layer_t;


/** @name CSAP processing flags */
enum {
    CSAP_STATE_IDLE       = 0x00001, /**< CSAP is idle */
    CSAP_STATE_SEND       = 0x00002, /**< CSAP is sending or idle after
                                         the send operation */
    CSAP_STATE_RECV       = 0x00004, /**< CSAP is receiving or idle after
                                         the receive operation */
    CSAP_STATE_DONE       = 0x00010, /**< Processing has been finished */
    CSAP_STATE_SEND_DONE  = 0x00020, /**< Send has been finished */
    CSAP_STATE_RECV_DONE  = 0x00040, /**< Receive has been finished */
    CSAP_STATE_COMPLETE   = 0x00100, /**< Receive operation complete */

    CSAP_STATE_RECV_SEQ_MATCH =     0x00200, /**< Pattern sequence matching */

    CSAP_STATE_RECV_MISMATCH =      0x00400, /**< Store mismatch packets
                                                  to get from test later */

    CSAP_STATE_PACKETS_NO_PAYLOAD = 0x00800, /**< Do not report payload of
                                                 received packets */
    CSAP_STATE_RESULTS    = 0x01000, /**< Receive results are required */

    CSAP_STATE_FOREGROUND = 0x02000, /**< RCF answer is pending */
    CSAP_STATE_WAIT       = 0x04000, /**< User request to wait for
                                         end of processing */
    CSAP_STATE_STOP       = 0x08000, /**< User request to stop */
    CSAP_STATE_DESTROY    = 0x10000, /**< CSAP is being destroyed */
};
/*@}*/


/**
 * CSAP instance support resources and attributes.
 */
typedef struct csap_instance {
    unsigned int    id;         /**< CSAP ID */
    unsigned int    ref;        /**< CSAP reference count (have to be >0) */
    char           *csap_type;  /**< Pointer to original CSAP type, proto[]
                                     entries are blocks of this string */
    asn_value      *nds;        /**< ASN.1 value with CSAP specification */
    unsigned int    depth;      /**< Number of layers in stack */
    csap_layer_t   *layers;     /**< Array of protocol layer descroptors */

    unsigned int    rw_layer;   /**< Index of layer in protocol stack
                                     responsible for read and write
                                     operations, usually lower */
    void           *rw_data;    /**< Private data of read/write layer */

    unsigned int    stop_latency_timeout;   /**< Maximum timeout for read
                                                 operations in
                                                 microseconds (it affects
                                                 latency of stop/destroy
                                                 operations) */
    unsigned int    recv_timeout;   /**< Default receive timeout */

    struct timeval  wait_for;   /**< Zero or moment of timeout
                                     current CSAP operation */
    struct timeval  first_pkt;  /**< Moment of first good packet
                                     processed: matched or sent */
    struct timeval  last_pkt;   /**< Moment of last good packet
                                     processed: matched or sent */

    pthread_cond_t  event;      /**< Event condition */
    unsigned int    state;      /**< Current state bitmask */
    pthread_mutex_t lock;       /**< Mutex for lock CSAP data which are
                                     changed from different threads
                                     (event, state, queue of received
                                     packets) */

    tad_send_context    sender;     /**< Sender context */
    tad_recv_context    receiver;   /**< Receiver context */

    TAILQ_HEAD(, tad_recv_op_context)   recv_ops;   /**< Receiver
                                                         operations
                                                         queue */

    unsigned int                    poll_id;    /**< ID of the last
                                                     poll request */
    LIST_HEAD(, tad_poll_context)   poll_ops;   /**< List of poll
                                                     requests */

} csap_instance;


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Create a new CSAP.
 * This method does not perform any actions related to CSAP functionality,
 * neither processing of CSAP init parameters, nor initialyzing some
 * communication media units (for example, sockets, etc.).
 * It only allocates memory for csap_instance sturture, set fields
 * 'id', 'depth' and 'proto' in it and allocates memory for 'layer_data'.
 *
 * @param type          Type of CSAP: dot-separated sequence of textual
 *                      layer labels
 * @param csap          Location for CSAP structure pointer
 *
 * @return Status code.
 */
extern te_errno csap_create(const char *type, csap_p *csap);

/**
 * Destroy CSAP.
 *
 * Before call this DB method, all protocol-specific data in
 * 'layer-data' and underground media resources should be freed.
 * This method will free all non-NULL pointers in 'layer-data', but
 * does not know nothing about what structures are pointed by them,
 * therefore if there are some more pointers in that structures,
 * memory may be lost.
 *
 * @param csap_id       Identifier of CSAP to be destroyed
 *
 * @return Status code.
 */
extern te_errno csap_destroy(csap_handle_t csap_id);

/**
 * Find CSAP by its identifier.
 *
 * @param csap_id       Identifier of CSAP
 *
 * @return Pointer to structure with internal CSAP information or NULL
 *         if not found.
 *
 * Change data in this structure if you really know what it means!
 */
extern csap_p csap_find(csap_handle_t csap_id);

/**
 * CSAP state transition by command.
 *
 * The function have to be called under CSAP lock only.
 *
 * @param csap          CSAP instance
 * @param command       Command
 *
 * @return Status code.
 *
 * @sa csap_command
 */
extern te_errno csap_command_under_lock(csap_p           csap,
                                        tad_traffic_op_t command);

/**
 * CSAP state transition by command.
 *
 * @param csap          CSAP instance
 * @param command       Command
 *
 * @return Status code.
 *
 * @sa csap_command_under_lock
 */
static inline te_errno
csap_command(csap_p csap, tad_traffic_op_t command)
{
    te_errno    rc;

    CSAP_LOCK(csap);

    rc = csap_command_under_lock(csap, command);

    CSAP_UNLOCK(csap);

    return rc;
}

/**
 * Wait for one of bit in CSAP state.
 *
 * @param csap          CSAP instance
 * @param state_bits    Set of state bits to wait for at least one of them
 *
 * @return Status code.
 */
static inline te_errno
csap_wait(csap_p csap, unsigned int state_bits)
{
    te_errno    rc = 0;
    int         ret;

    CSAP_LOCK(csap);
    while (~csap->state & state_bits)
    {
        ret = pthread_cond_wait(&csap->event, &csap->lock);
        if (ret != 0)
        {
            rc = TE_OS_RC(TE_TAD_CH, ret);
            assert(TE_RC_GET_ERROR(rc) != TE_ENOENT);
            ERROR("%s(): pthread_cond_wait() failed: %r",
                  __FUNCTION__, rc);
            break;
        }
    }
    CSAP_UNLOCK(csap);

    return rc;
}

/**
 * Wait for one of bit in CSAP state with timeout.
 *
 * @param csap          CSAP instance
 * @param state_bits    Set of state bits to wait for at least one of them
 * @param ms            Timeout in milliseconds
 *
 * @return Status code.
 */
extern te_errno csap_timedwait(csap_p csap, unsigned int state_bits,
                               unsigned int ms);


/**
 * Get CSAP read/write layer number.
 *
 * @param csap          CSAP instance
 *
 * @return Number of CSAP read/write layer.
 */
static inline unsigned int
csap_get_rw_layer(csap_p csap)
{
    return csap->rw_layer;
}

/**
 * Get read/write layer specific data.
 *
 * @param csap          CSAP instance
 *
 * @return Pointer to read/write layer specific data.
 */
static inline void *
csap_get_rw_data(csap_p csap)
{
    assert(csap != NULL);
    return csap->rw_data;
}

/**
 * Set read/write layer specific data.
 *
 * @param csap          CSAP instance
 * @param data          Pointer to protocol specific data
 */
static inline void
csap_set_rw_data(csap_p csap, void *data)
{
    assert(csap != NULL);
    csap->rw_data = data;
}

/**
 * Get protocol specific data of the layer.
 *
 * @param csap          CSAP instance
 * @param layer         Layer number
 *
 * @return Pointer to protocol specific data for the layer.
 */
static inline void *
csap_get_proto_spec_data(csap_p csap, unsigned int layer)
{
    assert(csap != NULL);
    assert(layer < csap->depth);
    return csap->layers[layer].specific_data;
}

/**
 * Set protocol specific data of the layer.
 *
 * @param csap          CSAP instance
 * @param layer         Layer number
 * @param data          Pointer to protocol specific data
 */
static inline void
csap_set_proto_spec_data(csap_p csap, unsigned int layer, void *data)
{
    assert(csap != NULL);
    assert(layer < csap->depth);
    csap->layers[layer].specific_data = data;
}

/**
 * Get protocol specific data of the layer.
 *
 * @param csap          CSAP instance
 * @param layer         Layer number
 *
 * @return Pointer to protocol support definition.
 */
static inline struct csap_spt_type_t *
csap_get_proto_support(csap_p csap, unsigned int layer)
{
    assert(csap != NULL);
    assert(layer < csap->depth);
    return csap->layers[layer].proto_support;
}

/**
 * Set protocol specific data of the layer.
 *
 * @param csap          CSAP instance
 * @param layer         Layer number
 * @param proto_support Protocol support description
 */
static inline void
csap_set_proto_support(csap_p csap, unsigned int layer,
                       struct csap_spt_type_t *proto_support)
{
    assert(csap != NULL);
    assert(layer < csap->depth);
    csap->layers[layer].proto_support = proto_support;
}

/**
 * Get TAD Sender context pointer.
 *
 * @param csap          CSAP instance
 */
static inline tad_send_context *
csap_get_send_context(csap_p csap)
{
    assert(csap != NULL);
    assert(csap->state & CSAP_STATE_SEND);
    return &csap->sender;
}

/**
 * Get TAD Receiver context pointer.
 *
 * @param csap          CSAP instance
 */
static inline tad_recv_context *
csap_get_recv_context(csap_p csap)
{
    assert(csap != NULL);
    assert(csap->state & CSAP_STATE_RECV);
    return &csap->receiver;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_CSAP_INST_H__ */
