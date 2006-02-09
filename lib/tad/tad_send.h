/** @file
 * @brief TAD Sender
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions for TAD Sender.
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

#ifndef __TE_TAD_SEND_H__
#define __TE_TAD_SEND_H__ 

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h" 
#include "comm_agent.h"

#include "tad_types.h"
#include "tad_pkt.h"
#include "tad_send_recv.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Per-template unit data of the TAD Sender.
 */
typedef struct tad_send_tmpl_unit_data {
    asn_value              *nds;            /**< ASN.1 value with
                                                 traffic template unit */
    tad_payload_spec_t      pld_spec;
    unsigned int            arg_num;
    tad_tmpl_iter_spec_t   *arg_specs;
    struct tad_tmpl_arg_t  *arg_iterated;
    uint32_t                delay;

    void                  **layer_opaque;

} tad_send_tmpl_unit_data;

/**
 * Per-template data of the TAD Sender.
 */
typedef struct tad_send_template_data {
    asn_value                  *nds;        /**< ASN.1 value with
                                                 traffic template */
    unsigned int                n_units;    /**< Number of units in
                                                 the template */
    tad_send_tmpl_unit_data  *units;      /**< Array with per-unit
                                                 unit data */
} tad_send_template_data;


/** TAD Sender context data. */
typedef struct tad_send_context {
    tad_task_context        task;
    tad_send_template_data  tmpl_data;
    te_errno                status;     /**< Status of the send operation
                                             to be returned on stop */
    unsigned int            sent_pkts;  /**< Number of sent packets */
} tad_send_context;


/**
 * Initialize TAD Sender context.
 *
 * @param context       Context to be initialized
 */
extern void tad_send_init_context(tad_send_context *context);

/**
 * Prepare TAD Sender to generate traffic by template to specified CSAP.
 *
 * @param csap          CSAP instance to generate traffic
 * @param template      Traffic template (owned by the routine in any
 *                      case)
 * @param rcfc          RCF connection handle to be used for answers
 * @param answer_pfx    Test Protocol answer prefix
 * @param pfx_len       Length of the answer prefix
 *
 * @return Status code.
 */
extern te_errno tad_send_prepare(csap_p                csap,
                                 asn_value            *tmpl_unit,
                                 rcf_comm_connection  *rcfc,
                                 const char           *answer_pfx,
                                 size_t                pfx_len);

/**
 * Release TAD Sender context.
 *
 * @param csap          CSAP instance
 * @param context       TAD Sender context pointer
 *
 * @return Status code.
 */
extern te_errno tad_send_release(csap_p csap, tad_send_context *context);


/**
 * Start routine for Sender thread. 
 *
 * @param arg           Start argument, should be pointer to 
 *                      CSAP structure
 *
 * @return NULL 
 */
extern void *tad_send_thread(void *arg);

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
extern int tad_send_prepare_bin(csap_p csap, asn_value *nds, 
                                const struct tad_tmpl_arg_t *args, 
                                size_t arg_num, 
                                tad_payload_spec_t *pld_data,
                                void **layer_opaque,
                                tad_pkts *pkts_per_layer);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_SEND_H__ */
