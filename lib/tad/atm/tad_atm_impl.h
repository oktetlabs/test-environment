/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD ATM
 *
 * Traffic Application Domain Command Handler.
 * ATM CSAP support internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_ATM_IMPL_H__
#define __TE_TAD_ATM_IMPL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "asn_usr.h"

#include "tad_csap_support.h"
#include "tad_csap_inst.h"


/**
 * ATM cell control data specified by upper layers.
 *
 * Pointer to this structure may be attached to ATM cell packets
 * as opaque data.
 */
typedef struct tad_atm_cell_ctrl_data {
    te_bool     indication; /**< ATM-user-to-ATM-user indication */
    te_bool     user_data;  /**< User data cell (received cell only) */
    uint16_t    vpi;        /**< VPI (received cell only) */
    uint16_t    vci;        /**< VCI (received cell only) */
} tad_atm_cell_ctrl_data;


/**
 * Callback for init 'atm' CSAP layer if single in stack.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */
extern te_errno tad_atm_rw_init_cb(csap_p csap);

/**
 * Callback for destroy 'atm' CSAP layer if single in stack.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */
extern te_errno tad_atm_rw_destroy_cb(csap_p csap);


/**
 * Open transmit socket for ATM CSAP.
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_atm_prepare_send(csap_p csap);

/**
 * Close transmit socket for ATM CSAP.
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_atm_shutdown_send(csap_p csap);

/**
 * Callback for write data to media of ATM CSAP.
 *
 * The function complies with csap_write_cb_t prototype.
 */
extern te_errno tad_atm_write_cb(csap_p csap, const tad_pkt *pkt);

/**
 * Open receive socket for ATM CSAP.
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_atm_prepare_recv(csap_p csap);

/**
 * Close receive socket for ATM CSAP.
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_atm_shutdown_recv(csap_p csap);

/**
 * Callback for read data from media of ATM CSAP.
 *
 * The function complies with csap_read_cb_t prototype.
 */
extern te_errno tad_atm_read_cb(csap_p csap, unsigned int timeout,
                                tad_pkt *pkt, size_t *pkt_len);


/**
 * Callback to initialize 'atm' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_atm_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback to destroy 'atm' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_atm_destroy_cb(csap_p csap, unsigned int layer);


/**
 * Callback for confirm template PDU with ATM CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_atm_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Callback for confirm pattern PDU with ATM CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_atm_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Callback to release PDU with ATM layer private data.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_atm_release_pdu_cb(csap_p csap, unsigned int layer,
                                   void *opaque);


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_atm_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);


extern te_errno tad_atm_match_pre_cb(csap_p              csap,
                                     unsigned int        layer,
                                     tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_atm_match_post_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_atm_match_do_cb(csap_p           csap,
                                    unsigned int     layer,
                                    const asn_value *ptrn_pdu,
                                    void            *ptrn_opaque,
                                    tad_recv_pkt    *meta_pkt,
                                    tad_pkt         *pdu,
                                    tad_pkt         *sdu);


/**
 * Callback to initialize 'aal5' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_aal5_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback to destroy 'aal5' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_aal5_destroy_cb(csap_p csap, unsigned int layer);


/**
 * Callback for confirm template PDU with ATM CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_aal5_confirm_tmpl_cb(csap_p         csap,
                                         unsigned int   layer,
                                         asn_value     *layer_pdu,
                                         void         **p_opaque);

/**
 * Callback for confirm pattern PDU with ATM CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_aal5_confirm_ptrn_cb(csap_p         csap,
                                         unsigned int   layer,
                                         asn_value     *layer_pdu,
                                         void         **p_opaque);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_aal5_gen_bin_cb(csap_p                csap,
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
extern te_errno tad_aal5_match_bin_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);


enum {
    CRC32_INIT = 0xffffffff, /**< Init CRC32 value */
};

/**
 * Calculate CRC32 hash value for new portion of data,
 * using ready value for previous data.
 * If there are no previous data, pass 0xffffffff as 'previous_value'.
 *
 * @param previous_value        Ready CRC32 value for previous data block.
 * @param next_pkt              Pointer to new portion of data.
 * @param next_len              Length of new portion of data.
 *
 * @return updated CRC32 value or zero if NULL pointer passed.
 */
extern uint32_t calculate_crc32(uint32_t previous_value,
                                uint8_t *next_pkt,
                                size_t   next_len);

#endif /* !__TE_TAD_ATM_IMPL_H__ */
