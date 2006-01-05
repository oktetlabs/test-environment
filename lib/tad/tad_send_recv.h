/** @file
 * @brief TAD Sender/Receiver
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions for TAD Sender, Receiver and
 * Send-Receiver.
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

#ifndef __TE_TAD_SEND_RECV_H__
#define __TE_TAD_SEND_RECV_H__ 

#include "asn_usr.h" 
#include "comm_agent.h"
#include "tad_csap_inst.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Traffic operation thread special data. */
typedef struct tad_task_context {
    csap_p     csap; /**< Pointer to CSAP instance */
    asn_value *nds;  /**< ASN value with NDS */

    struct rcf_comm_connection *rcfc; /**< RCF connection to answer */
} tad_task_context;


/**
 * Per-template unit data of the TAD Sender.
 */
typedef struct tad_sender_tmpl_unit_data {
    asn_value              *nds;            /**< ASN.1 value with
                                                 traffic template unit */
    tad_payload_spec_t      pld_spec;
    unsigned int            arg_num;
    tad_tmpl_iter_spec_t   *arg_specs;
    tad_tmpl_arg_t         *arg_iterated;
    uint32_t                delay;

    void                  **layer_opaque;

} tad_sender_tmpl_unit_data;

/**
 * Per-template data of the TAD Sender.
 */
typedef struct tad_sender_template_data {
    asn_value                  *nds;        /**< ASN.1 value with
                                                 traffic template */
    unsigned int                n_units;    /**< Number of units in
                                                 the template */
    tad_sender_tmpl_unit_data  *units;      /**< Array with per-unit
                                                 unit data */
} tad_sender_template_data;


/** TAD Sender context data. */
typedef struct tad_sender_context {
    tad_task_context            task;
    tad_sender_template_data    tmpl_data;
} tad_sender_context;


/** TAD Receiver context data. */
typedef struct tad_receiver_context {
    tad_task_context    task;
} tad_receiver_context;


/**
 * Prepare TAD Sender to generate traffic by template to specified CSAP.
 *
 * @param csap          CSAP instance to generate traffic
 * @param template      Traffic template (owned by the routine in any
 *                      case)
 * @param rcfc          RCF connection handle to be used for answers
 * @param context       Location for TAD Sender context pointer
 *
 * @return Status code.
 */
extern te_errno tad_sender_prepare(csap_p                       csap,
                                   asn_value                   *tmpl_unit,
                                   struct rcf_comm_connection  *rcfc,
                                   tad_sender_context         **context);

/**
 * Release TAD Sender context.
 *
 * @param context       TAD Sender context pointer
 *
 * @return Status code.
 */
extern te_errno tad_sender_release(tad_sender_context *context);

/**
 * Start routine for Sender thread. 
 *
 * @param arg           start argument, should be pointer to 
 *                      tad_sender_context structure
 *
 * @return NULL 
 */
extern void *tad_sender_thread(void *arg);

/**
 * Start routine for Receiver thread. 
 *
 * @param arg           start argument, should be pointer to 
 *                      tad_receiver_context structure
 *
 * @return NULL 
 */
extern void *tad_receiver_thread(void *arg);


/**
 * Prepare binary data by NDS.
 *
 * @param csap    CSAP description structure
 * @param nds           ASN value with traffic-template NDS, should be
 *                      preprocessed (all iteration and function calls
 *                      performed)
 * @param args          array with template iteration parameter values,
 *                      may be used to prepare binary data, 
 *                      references to interation parameter values may
 *                      be set in ASN traffic template PDUs
 * @param arg_num       length of array above
 * @param pld_type      type of payload in nds, passed to make function
 *                      more fast
 * @param pld_data      payload data read from original NDS
 * @param pkts_per_layer    array with packets per generated layer (OUT)
 *
 * @return zero on success, otherwise error code.  
 */
extern int tad_tr_send_prepare_bin(csap_p csap, asn_value *nds, 
                                   const tad_tmpl_arg_t *args, 
                                   size_t arg_num, 
                                   tad_payload_spec_t *pld_data,
                                   void **layer_opaque,
                                   tad_pkts *pkts_per_layer);



/**
 * Confirm traffic template or pattern PDUS set with CSAP settings and 
 * protocol defaults. 
 * This function changes passed ASN value, user have to ensure that changes
 * will be set in traffic template or pattern ASN value which will be used 
 * in next operation. This may be done by such ways:
 *
 * Pass pointer got by asn_get_subvalue method, or write modified value 
 * into original NDS. 
 *
 * @param csap    CSAP descriptor.
 * @param recv          Is receive flow or send flow.
 * @param pdus          ASN value with SEQUENCE OF Generic-PDU (IN/OUT).
 * @param layer_opaque  Array for per-layer opaque data pointers
 *
 * @return zero on success, otherwise error code.
 */
extern int tad_confirm_pdus(csap_p csap, te_bool recv,
                            asn_value *pdus, void **layer_opaque);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_SEND_RECV_H__ */
