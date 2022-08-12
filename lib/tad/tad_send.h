/** @file
 * @brief TAD Sender
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions for TAD Sender.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 */

#ifndef __TE_TAD_SEND_H__
#define __TE_TAD_SEND_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h"

#include "tad_types.h"
#include "tad_pkt.h"
#include "tad_reply.h"
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
    tad_reply_context       reply_ctx;
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
 * @param reply_ctx     TAD async reply context
 *
 * @return Status code.
 */
extern te_errno tad_send_prepare(csap_p                     csap,
                                 asn_value                 *tmpl_unit,
                                 const tad_reply_context   *reply_ctx);

/**
 * Stop TAD Sender.
 *
 * @param csap          CSAP instance to stop generation traffic on
 * @param sent_pkts     Location for the number of sent packets
 *
 * @return Status code.
 */
extern te_errno tad_send_stop(csap_p csap, unsigned int *sent_pkts);


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
extern te_errno tad_send_prepare_bin(csap_p csap, asn_value *nds,
                                     const struct tad_tmpl_arg_t *args,
                                     size_t arg_num,
                                     tad_payload_spec_t *pld_data,
                                     void **layer_opaque,
                                     tad_pkts *pkts_per_layer);



/**
 * Type for reference to user function for some magic processing
 * with matched pkt
 *
 * @param csap          CSAP descriptor structure.
 * @param usr_param     String passed by user.
 * @param pkts          List of binary packets.
 *
 * @return Status code.
 */
typedef te_errno (*tad_special_send_pkt_cb)(csap_p csap,
                                            const char  *usr_param,
                                            tad_pkts *pkts);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_SEND_H__ */
