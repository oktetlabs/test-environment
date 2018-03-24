/** @file
 * @brief TAD API
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions which may be used outside RCF.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TAD_API__
#define __TE_TAD_API__

#include "te_defs.h"
#include "te_errno.h"
#include "tad_types.h"
#include "tad_reply.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create CSAP.
 *
 * @param stack         CSAP protocols stack
 * @param spec_str      CSAP specification string
 * @param new_csap_p    Location for CSAP instance
 *
 * @return Status code.
 */
extern te_errno tad_csap_create(const char *stack, const char *spec_str,
                                csap_p *new_csap_p);

/** Destroy CSAP */
extern te_errno tad_csap_destroy(csap_p csap);

/**
 * Preprocess traffic template arguments.
 *
 * @param csap          CSAP instance
 * @param tmpl_unit     Traffic template unit
 * @param data          Location for template unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
extern te_errno tad_send_preprocess_args(csap_p                      csap,
                                         const asn_value            *tmpl_unit,
                                         tad_send_tmpl_unit_data    *data);

/**
 * Prepare TAD Sender to start traffic generation.
 *
 * @param csap          CSAP instance to generate traffic
 * @param tmpl_str      Traffic template in string format
 * @param postponed     "postponed" flag value
 * @param reply_ctx     TAD async reply context
 *
 * @return Status code.
 */
extern te_errno tad_send_start_prepare(csap_p                   csap,
                                       const char              *tmpl_str,
                                       te_bool                  postponed,
                                       const tad_reply_context *reply_ctx);

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
 * Run Sender.
 *
 * @param csap          CSAP instance
 *
 * @return Status code.
 *
 * @note It releases sender context in any case.
 */
extern te_errno tad_send_do(csap_p csap);

/**
 * Prepare TAD Receiver to start matching traffic.
 *
 * @param csap          CSAP instance to match traffic
 * @param ptrn_str      Traffic pattern in string representation
 * @param num           Number of packets to wait for (0 - unlimited)
 * @param timeout       Timeout in milliseconds
 * @param flags         Receive mode flags bitmask
 * @param reply_ctx     TAD async reply context
 *
 * @return Status code.
 */
extern te_errno tad_recv_start_prepare(csap_p                   csap,
                                       const char              *ptrn_str,
                                       unsigned int             num,
                                       unsigned int             timeout,
                                       unsigned int             flags,
                                       const tad_reply_context *reply_ctx);

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
 * Run Receiver.
 *
 * @param csap          CSAP instance
 *
 * @return Status code.
 *
 * @note It releases receiver context in any case.
 */
extern te_errno tad_recv_do(csap_p csap);

/**
 * Get matched packet from TAD receiver packets queue.
 *
 * @param csap      CSAP structure
 * @param reply_ctx TAD reply context for reporting
 * @param wait      Wait for more packets or end of processing
 * @param got       Location for number of got packets
 *
 * @return Status code.
 */
extern te_errno tad_recv_get_packets(csap_p              csap,
                                     tad_reply_context  *reply_ctx,
                                     te_bool             wait,
                                     unsigned int       *got);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_API__ */
