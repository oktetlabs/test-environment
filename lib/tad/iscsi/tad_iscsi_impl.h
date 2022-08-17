/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD iSCSI
 *
 * Traffic Application Domain Command Handler.
 * iSCSI CSAP implementaion internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_ISCSI_IMPL_H__
#define __TE_TAD_ISCSI_IMPL_H__


#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_errno.h"
#include "asn_usr.h"
#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "ndn_iscsi.h"

#include "tad_utils.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    ISCSI_SEND_USUAL,
    ISCSI_SEND_LAST,
    ISCSI_SEND_INVALID,
} tad_iscsi_send_mode_t;

/**
 * iSCSI CSAP layer specific data
 */
typedef struct tad_iscsi_layer_data {
    iscsi_digest_type   hdig;
    iscsi_digest_type   ddig;

    size_t      wait_length;
    size_t      stored_length;
    uint8_t    *stored_buffer;

    tad_iscsi_send_mode_t send_mode;

    uint32_t    total_received;

    tad_data_unit_t  du_i_bit;
    tad_data_unit_t  du_opcode;
    tad_data_unit_t  du_f_bit;
} tad_iscsi_layer_data;


/**
 * Callback for init iSCSI CSAP layer.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */
extern te_errno tad_iscsi_rw_init_cb(csap_p csap);

/**
 * Callback for destroy iSCSI CSAP layer.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */
extern te_errno tad_iscsi_rw_destroy_cb(csap_p csap);

/**
 * Callback for read data from media of iSCSI CSAP.
 *
 * The function complies with csap_read_cb_t prototype.
 */
extern te_errno tad_iscsi_read_cb(csap_p csap, unsigned int timeout,
                                  tad_pkt *pkt, size_t *pkt_len);

/**
 * Callback for write data to media of iSCSI CSAP.
 *
 * The function complies with csap_write_cb_t prototype.
 */
extern te_errno tad_iscsi_write_cb(csap_p csap, const tad_pkt *pkt);


/**
 * Callback for init iSCSI CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_iscsi_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy iSCSI CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_iscsi_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback for read parameter value of iSCSI CSAP layer.
 *
 * The function complies with csap_layer_get_param_cb_t prototype.
 */
extern char *tad_iscsi_get_param_cb(csap_p        csap,
                                    unsigned int  layer,
                                    const char   *param);


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_iscsi_gen_bin_cb(csap_p                csap,
                                     unsigned int          layer,
                                     const asn_value      *tmpl_pdu,
                                     void                 *opaque,
                                     const tad_tmpl_arg_t *args,
                                     size_t                arg_num,
                                     tad_pkts             *sdus,
                                     tad_pkts             *pdus);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_iscsi_match_bin_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);

/**
 * Callback for generating pattern to filter just one response to
 * the packet which will be sent by this CSAP according to this template.
 *
 * The function complies with csap_layer_gen_pattern_cb_t prototype.
 */
extern te_errno tad_iscsi_gen_pattern_cb(csap_p            csap,
                                         unsigned int      layer,
                                         const asn_value  *tmpl_pdu,
                                         asn_value       **pattern_pdu);

/**
 * Prepare send callback
 *
 * @param csap    CSAP descriptor structure.
 *
 * @return status code.
 */
extern te_errno tad_iscsi_prepare_send_cb(csap_p csap);

/**
 * Prepare recv callback
 *
 * @param csap    CSAP descriptor structure.
 *
 * @return status code.
 */
extern te_errno tad_iscsi_prepare_recv_cb(csap_p csap);



/**
 * Confirm pattern callback
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 *
 * @return status code.
 */
extern te_errno tad_iscsi_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                                          asn_value *layer_pdu,
                                          void **p_opaque);

typedef enum
{
    ISCSI_DUMP_SEND,
    ISCSI_DUMP_RECV,
} iscsi_dump_mode_t;
/**
 * Dump some significant fields from iSCSI PDU to log.
 *
 * @param buffer        Buffer with PDU.
 * @param mode          Dump mode, take influence only to words.
 *
 * @return status code
 */
extern te_errno tad_iscsi_dump_iscsi_pdu(const uint8_t *buffer,
                                         iscsi_dump_mode_t mode);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_ISCSI_IMPL_H__ */
