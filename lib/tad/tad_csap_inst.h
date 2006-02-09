/** @file
 * @brief TAD CSAP Instance
 *
 * Traffic Application Domain Command Handler.
 * Declarations of CSAP instance types and functions, used in common and 
 * protocol-specific modules implemnting TAD.
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_CSAP_INST_H__
#define __TE_TAD_CSAP_INST_H__ 

#ifndef PACKAGE_VERSION
#include "config.h"
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
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
#include "logger_api.h"
#include "asn_usr.h" 
#include "tad_common.h"

#include "tad_types.h"
#include "tad_pkt.h"
#include "tad_send_recv.h"
#include "tad_send.h"
#include "tad_recv.h"


/**
 * Locks access to CSAP shared flags and data. 
 * If already locked, wait until unlocked. 
 *
 * @param csap_inst_   CSAP instance
 */
#define CSAP_LOCK(csap_inst_) \
    do {                                                            \
        int rc_ = pthread_mutex_lock(&((csap_inst_)->lock));        \
                                                                    \
        if (rc_ != 0)                                               \
            ERROR("%s(): pthread_mutex_lock() failed %d, errno %d", \
                  __FUNCTION__, rc_, errno);                        \
    } while (0)

/**
 * Try to lock access to CSAP shared flags and data. 
 *
 * @param csap_inst_   CSAP instance
 * @param rc_          Variable for return code
 */
#define CSAP_TRYLOCK(csap_inst_, rc_) \
    do {                                                            \
        (rc_) = pthread_mutex_trylock(&((csap_inst_)->lock));       \
        if ((rc_ != 0) && (errno != EBUSY))                         \
            ERROR("%s(): pthread_mutex_trylock() failed %d, "       \
                  "errno %d", __FUNCTION__, rc_, errno);            \
    } while (0)

/**
 * Unlocks access to CSAP shared flags and data. 
 *
 * @param csap_inst_   CSAP instance
 */
#define CSAP_UNLOCK(csap_inst_)\
    do {                                                            \
        int rc_ = pthread_mutex_unlock(&((csap_inst_)->lock));      \
                                                                    \
        if (rc_ != 0)                                               \
            ERROR("%s(): pthread_mutex_unlock() failed %d, "        \
                  "errno %d", __FUNCTION__, rc_, errno);            \
    } while (0)


/* Forward declaration */
struct csap_spt_type_t;


/**
 * Collection of common protocol layer attributes of CSAP.
 */
typedef struct csap_layer_t { 
    char               *proto;      /**< Protocol layer text label */
    te_tad_protocols_t  proto_tag;  /**< Protocol layer int tag */

    void        *specific_data;     /**< Protocol-specific data */ 
    asn_value   *csap_layer_pdu;    /**< ASN value with CSAP
                                         specification layer PDU */

    struct csap_spt_type_t *proto_support; /**< Protocol layer 
                                                support descriptor */
} csap_layer_t;


/** @name CSAP processing flags */
enum {
    CSAP_STATE_IDLE       = 0x0001, /**< CSAP is idle */
    CSAP_STATE_SEND       = 0x0002, /**< CSAP is sending or idle after
                                         the send operation */
    CSAP_STATE_RECV       = 0x0004, /**< CSAP is receiving or idle after
                                         the receive operation */
    CSAP_STATE_DONE       = 0x0010, /**< Processing has been finished */
    CSAP_STATE_SEND_DONE  = 0x0020, /**< Send has been finished */
    CSAP_STATE_RECV_DONE  = 0x0040, /**< Receive has been finished */
    CSAP_STATE_COMPLETE   = 0x0100, /**< Receive operation complete */
    CSAP_STATE_RESULTS    = 0x0800, /**< Receive results are required */
    CSAP_STATE_FOREGROUND = 0x1000, /**< RCF answer is pending */
    CSAP_STATE_WAIT       = 0x2000, /**< User request to wait for
                                         end of processing */
    CSAP_STATE_STOP       = 0x4000, /**< User request to stop */
    CSAP_STATE_DESTROY    = 0x8000, /**< CSAP is being destroyed */
};
/*@}*/


/**
 * CSAP instance support resources and attributes.
 */
typedef struct csap_instance {
    unsigned int    id;         /**< CSAP ID */
    char           *csap_type;  /**< Pointer to original CSAP type, proto[]
                                     entries are blocks of this string */
    unsigned int    depth;      /**< Number of layers in stack */
    csap_layer_t   *layers;     /**< Array of protocol layer descroptors */

    unsigned int    rw_layer;   /**< Index of layer in protocol stack
                                     responsible for read and write
                                     operations, usually lower */
    void           *rw_data;    /**< Private data of read/write layer */

    unsigned int    timeout;    /**< Maximum timeout for read operations
                                     in microseconds (it affects latency
                                     of stop/destroy operations) */

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

    CSAP_LOCK(csap);
    while (~csap->state & state_bits)
    {
        rc = pthread_cond_wait(&csap->event, &csap->lock);
        if (rc != 0)
        {
            rc = TE_OS_RC(TE_TAD_CH, errno);
            assert(TE_RC_GET_ERROR(rc) != TE_ENOENT);
            ERROR("%s(): pthread_cond_wait() failed: %r",
                  __FUNCTION__, rc);
        }
    }
    CSAP_UNLOCK(csap);

    return rc;
}

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
