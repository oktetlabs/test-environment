/** @file
 * @brief TAD iSCSI
 *
 * Traffic Application Domain Command Handler.
 * iSCSI CSAP implementaion internal declarations.
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
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
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_iscsi_rw_init_cb(csap_p csap,
                                     const asn_value *csap_nds);

/**
 * Callback for destroy iSCSI CSAP layer.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
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
                                          asn_value_p layer_pdu,
                                          void **p_opaque);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_ISCSI_IMPL_H__ */
