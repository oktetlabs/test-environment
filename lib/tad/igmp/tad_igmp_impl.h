/** @file
 * @brief TAD IGMP version 2 layer
 *
 * Traffic Application Domain Command Handler.
 * IGMP version 2 PDU CSAP implementaion internal declarations.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Alexander Kukuta <kam@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef __TE_TAD_IGMP_IMPL_H__
#define __TE_TAD_IGMP_IMPL_H__

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"

#include "asn_usr.h"
#include "ndn_igmp.h"
#include "logger_api.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
 * IGMPv2 CSAP specific data
 */
struct igmp_csap_specific_data;

typedef struct igmp_csap_specific_data *igmp_csap_specific_data_p;

typedef struct igmp_csap_specific_data {
    uint8_t version;
} igmp_csap_specific_data_t;


/**
 * Callback for init 'igmp' CSAP layer
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_igmp_init_cb(csap_p           csap,
                                 unsigned int     layer);

/**
 * Callback for destroy 'igmp' CSAP layer
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_igmp_destroy_cb(csap_p       csap,
                                    unsigned int layer);

/**
 * Callback for confirm template PDU with IGMPv2 CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_igmp_confirm_tmpl_cb(csap_p         csap,
                                         unsigned int   layer,
                                         asn_value     *layer_pdu,
                                         void         **p_opaque);

/**
 * Callback for confirm pattern PDU with IGMPv2 CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_igmp_confirm_ptrn_cb(csap_p         csap,
                                         unsigned int   layer,
                                         asn_value     *layer_pdu,
                                         void         **p_opaque);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_igmp_gen_bin_cb(csap_p                csap,
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
extern te_errno tad_igmp_match_do_cb(csap_p           csap,
                                     unsigned int     layer,
                                     const asn_value *ptrn_pdu,
                                     void            *ptrn_opaque,
                                     tad_recv_pkt    *meta_pkt,
                                     tad_pkt         *pdu,
                                     tad_pkt         *sdu);

extern te_errno tad_igmp_match_pre_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_igmp_match_post_cb(csap_p              csap,
                                       unsigned int        layer,
                                       tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for generating pattern to filter
 * just one response to the packet which will be sent by this CSAP
 * according to this template.
 *
 * The function complies with csap_layer_gen_pattern_cb_t prototype.
 */
extern te_errno tad_igmp_gen_pattern_cb(csap_p            csap,
                                        unsigned int      layer,
                                        const asn_value  *tmpl_pdu,
                                        asn_value       **ptrn_pdu);


/**
 * Callback to release data prepared by confirm callback or packet match.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_igmp_release_pdu_cb(csap_p csap, unsigned int layer,
                                    void *opaque);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_IGMP_IMPL_H__ */
