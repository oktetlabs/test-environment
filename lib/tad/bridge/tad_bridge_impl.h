/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Bridge/STP
 *
 * Traffic Application Domain Command Handler.
 * Ethernet Bridge/STP PDU CSAP implementaion internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_BRIDGE_IMPL_H__
#define __TE_TAD_BRIDGE_IMPL_H__


#include "te_errno.h"

#include "asn_usr.h"
#include "ndn_bridge.h"
#include "logger_api.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ethernet CSAP specific data
 */
struct bridge_csap_specific_data;
typedef struct bridge_csap_specific_data *bridge_csap_specific_data_p;

typedef struct bridge_csap_specific_data {

} bridge_csap_specific_data_t;


/**
 * Callback for init 'bridge' CSAP layer over 'eth' in stack.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_bridge_eth_init_cb(csap_p           csap,
                                       unsigned int     layer,
                                       const asn_value *csap_nds);

/**
 * Callback for destroy 'bridge' CSAP layer over 'eth' in stack.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_bridge_eth_destroy_cb(csap_p       csap,
                                          unsigned int layer);

/**
 * Callback for confirm template PDU with ehternet CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_bridge_confirm_tmpl_cb(csap_p         csap,
                                           unsigned int   layer,
                                           asn_value     *layer_pdu,
                                           void         **p_opaque);

/**
 * Callback for confirm pattern PDU with ehternet CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_bridge_confirm_ptrn_cb(csap_p         csap,
                                           unsigned int   layer,
                                           asn_value     *layer_pdu,
                                           void         **p_opaque);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_bridge_gen_bin_cb(csap_p                csap,
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
extern te_errno tad_bridge_match_bin_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);

/**
 * Callback for generating pattern to filter
 * just one response to the packet which will be sent by this CSAP
 * according to this template.
 *
 * The function complies with csap_layer_gen_pattern_cb_t prototype.
 */
extern te_errno tad_bridge_gen_pattern_cb(csap_p            csap,
                                          unsigned int      layer,
                                          const asn_value  *tmpl_pdu,
                                          asn_value       **ptrn_pdu);


/**
 * Free all memory allocated by eth csap specific data.
 *
 * @param csap_data     Poiner to structure
 * @param is_complete   If not 0 the final free() will be called
 *                      on passed pointer
 */
extern void free_bridge_csap_data(bridge_csap_specific_data_p spec_data,
                                  char is_colmplete);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_BRIDGE_IMPL_H__ */
