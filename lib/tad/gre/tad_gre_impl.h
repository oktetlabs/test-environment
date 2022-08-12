/** @file
 * @brief TAD GRE
 *
 * Traffic Application Domain Command Handler
 * GRE CSAP implementaion internal definitions
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAD_GRE_IMPL_H__
#define __TE_TAD_GRE_IMPL_H__


#include "te_defs.h"
#include "te_errno.h"

#include "asn_usr.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize 'gre' CSAP layer protocol-specific data
 * (the function complies with 'csap_layer_init_cb_t' prototype)
 */
extern te_errno tad_gre_init_cb(csap_p       csap,
                                unsigned int layer_idx);

/**
 * Teardown 'gre' CSAP layer protocol-specific data
 * (the function complies with 'csap_layer_destroy_cb_t' prototype)
 */
extern te_errno tad_gre_destroy_cb(csap_p       csap,
                                   unsigned int layer_idx);

/**
 * Teardown GRE data prepared by confirm callback or packet match
 * (the function complies with 'csap_layer_release_opaque_cb_t' prototype)
 */
extern void tad_gre_release_pdu_cb(csap_p        csap,
                                   unsigned int  layer_idx,
                                   void         *opaque);

/**
 * Confirm template PDU with respect to GRE CSAP parameters and possibilities
 * (the function complies with 'csap_layer_confirm_pdu_cb_t' prototype)
 */
extern te_errno tad_gre_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer_idx,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Generate GRE binary data
 * (the function complies with 'csap_layer_generate_pkts_cb_t' prototype)
 */
extern te_errno tad_gre_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer_idx,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);

/**
 * Confirm pattern PDU with respect to GRE CSAP parameters and possibilities
 * (the function complies with 'csap_layer_confirm_pdu_cb_t' prototype)
 */
extern te_errno tad_gre_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer_idx,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Generate a meta packet GRE NDS per a packet received (if need be)
 * (the function complies with 'csap_layer_match_post_cb_t' prototype)
 */
extern te_errno tad_gre_match_post_cb(csap_p              csap,
                                      unsigned int        layer_idx,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Parse a packet received and match it against the pattern's GRE PDU
 * (the function complies with 'csap_layer_match_do_cb_t' prototype)
 */
extern te_errno tad_gre_match_do_cb(csap_p           csap,
                                    unsigned int     layer_idx,
                                    const asn_value *ptrn_pdu,
                                    void            *ptrn_opaque,
                                    tad_recv_pkt    *meta_pkt,
                                    tad_pkt         *pdu,
                                    tad_pkt         *sdu);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_GRE_IMPL_H__ */
