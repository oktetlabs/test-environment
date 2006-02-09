/** @file
 * @brief TAD Receiver
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions for TAD Receiver.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_RECV_H__
#define __TE_TAD_RECV_H__ 

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#else
#error TAD cannot be compiled without sys/queue.h
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h" 
#include "comm_agent.h"

#include "tad_types.h"
#include "tad_recv_pkt.h"
#include "tad_send_recv.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Type for reference to user function for some magic processing 
 * with matched pkt
 *
 * @param csap          CSAP descriptor structure.
 * @param usr_param     String passed by user.
 * @param pkt           Packet binary data, as it was caught from net.
 * @param pkt_len       Length of pkt data.
 *
 * @return Status code.
 */
typedef te_errno (*tad_processing_pkt_method)(csap_p         csap,
                                              const char    *usr_param, 
                                              const uint8_t *pkt, 
                                              size_t         pkt_len);

/**
 * Action specification.
 */
typedef struct tad_action_spec {

    asn_tag_value   type;   /**< Type of the action */

    union {
        /** Parameters of the report action */
        struct {
        } report;

        /** Parameters of the break action */
        struct {
        } do_break;

        /** Parameters of the echo method */
        struct {
        } echo;

        /** Parameters of the packet processing method */
        struct {
            tad_processing_pkt_method   func;   /**< Function to call */
            char                       *opaque; /**< Opaque parameter */
        } function;

        /** Parameters of the Forward-Payload action */
        struct {
            unsigned int    csap_id;    /**< Target CSAP ID */
        } fwd_pld;
    };

} tad_action_spec;

/**
 * Per-pattern unit data of the TAD Receiver.
 */
typedef struct tad_recv_ptrn_unit_data {
    asn_value          *nds;            /**< ASN.1 value with traffic 
                                             pattern unit */
    tad_payload_spec_t  pld_spec;

    unsigned int        n_actions;      /**< Number of actions */
    tad_action_spec    *actions;        /**< Actions specification */
    te_bool             action_report;  /**< Does action report present? */

    void              **layer_opaque;

} tad_recv_ptrn_unit_data;

/**
 * Per-pattern data of the TAD Receiver.
 */
typedef struct tad_recv_pattern_data {
    asn_value                  *nds;        /**< ASN.1 value with
                                                 traffic pattern */
    unsigned int                n_units;    /**< Number of units in
                                                 the pattern */
    tad_recv_ptrn_unit_data    *units;      /**< Array with per-unit
                                                 unit data */
} tad_recv_pattern_data;


/** TAD Receiver context data. */
typedef struct tad_recv_context {
    tad_task_context            task;       /**< Task parameters */
    tad_recv_pattern_data       ptrn_data;  /**< Pattern data */

    tad_recv_pkts   packets;    /**< Received packets */

    te_errno        status;

    unsigned int    wait_pkts;  /**< Number of matched packets to wait */
    unsigned int    match_pkts; /**< Number of matched packets */
    unsigned int    got_pkts;   /**< Number of matched packets got via
                                     traffic receive get operation */
} tad_recv_context;


/** TAD Receiver stop/wait/get context data. */
typedef struct tad_recv_op_context {
    TAILQ_ENTRY(tad_recv_op_context)    links;  /**< List links */

    tad_task_context    task;   /**< Task parameters */
    tad_traffic_op_t    op;     /**< Operation */
} tad_recv_op_context;


/**
 * Initialize TAD Receiver context.
 *
 * @param context       Context to be initialized
 */
extern void tad_recv_init_context(tad_recv_context *context);

/**
 * Prepare TAD Receiver to match traffic by pattern to specified CSAP.
 *
 * @param csap          CSAP instance to match traffic
 * @param pattern       Traffic pattern (owned by the routine in any
 *                      case)
 * @param num           Number of packets to wait for (0 - unlimited)
 * @param timeout       Timeout in milliseconds
 * @param rcfc          RCF connection handle to be used for answers
 * @param answer_pfx    Test Protocol answer prefix
 * @param pfx_len       Length of the answer prefix
 *
 * @return Status code.
 */
extern te_errno tad_recv_prepare(csap_p                csap,
                                 asn_value            *ptrn_unit,
                                 unsigned int          num,
                                 unsigned int          timeout,
                                 rcf_comm_connection  *rcfc,
                                 const char           *answer_pfx,
                                 size_t                pfx_len);

/**
 * Release TAD Receiver context.
 *
 * @param csap          CSAP instance
 * @param context       TAD Receiver context pointer
 *
 * @return Status code.
 */
extern te_errno tad_recv_release(csap_p csap, tad_recv_context *context);

/**
 * Start routine for Receiver thread. 
 *
 * @param arg           Start argument, should be pointer to 
 *                      CSAP structure
 *
 * @return NULL 
 */
extern void *tad_recv_thread(void *arg);


/**
 * Enqueue traffic receive get/wait/stop/destroy operation.
 *
 * @param csap          CSAP
 * @param op            Operation (stop/wait/get)
 * @param rcfc          RCF communication handle
 * @param answer_pfx    RCF answer prefix
 * @param pfx_len       RCF answer prefix length
 *
 * @return Status code.
 */
extern te_errno tad_recv_op_enqueue(csap_p                csap,
                                    tad_traffic_op_t      op,
                                    rcf_comm_connection  *rcfc,
                                    const char           *answer_pfx,
                                    size_t                pfx_len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_RECV_H__ */
