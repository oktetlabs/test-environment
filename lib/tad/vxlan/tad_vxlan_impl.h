/** @file
 * @brief TAD VxLAN
 *
 * Traffic Application Domain Command Handler
 * VxLAN CSAP implementaion internal definitions
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAD_VXLAN_IMPL_H__
#define __TE_TAD_VXLAN_IMPL_H__

#include "config.h"

#include "te_defs.h"
#include "te_errno.h"

#include "asn_usr.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize 'vxlan' CSAP layer protocol-specific data
 * (the function complies with 'csap_layer_init_cb_t' prototype)
 */
extern te_errno tad_vxlan_init_cb(csap_p       csap,
                                  unsigned int layer_idx);

/**
 * Teardown 'vxlan' CSAP layer protocol-specific data
 * (the function complies with 'csap_layer_destroy_cb_t' prototype)
 */
extern te_errno tad_vxlan_destroy_cb(csap_p       csap,
                                     unsigned int layer_idx);

/**
 * Teardown VxLAN data prepared by confirm callback or packet match
 * (the function complies with 'csap_layer_release_opaque_cb_t' prototype)
 */
extern void tad_vxlan_release_pdu_cb(csap_p        csap,
                                     unsigned int  layer_idx,
                                     void         *opaque);

/**
 * Confirm template PDU with respect to VxLAN CSAP parameters and possibilities
 * (the function complies with 'csap_layer_confirm_pdu_cb_t' prototype)
 */
extern te_errno tad_vxlan_confirm_tmpl_cb(csap_p         csap,
                                          unsigned int   layer_idx,
                                          asn_value     *layer_pdu,
                                          void         **p_opaque);

/**
 * Generate VxLAN binary data
 * (the function complies with 'csap_layer_generate_pkts_cb_t' prototype)
 */
extern te_errno tad_vxlan_gen_bin_cb(csap_p                csap,
                                     unsigned int          layer_idx,
                                     const asn_value      *tmpl_pdu,
                                     void                 *opaque,
                                     const tad_tmpl_arg_t *args,
                                     size_t                arg_num,
                                     tad_pkts             *sdus,
                                     tad_pkts             *pdus);

/**
 * Confirm pattern PDU with respect to VxLAN CSAP parameters and possibilities
 * (the function complies with 'csap_layer_confirm_pdu_cb_t' prototype)
 */
extern te_errno tad_vxlan_confirm_ptrn_cb(csap_p         csap,
                                          unsigned int   layer_idx,
                                          asn_value     *layer_pdu,
                                          void         **p_opaque);

/**
 * Generate a meta packet VxLAN NDS per a packet received (if need be)
 * (the function complies with 'csap_layer_match_post_cb_t' prototype)
 */
extern te_errno tad_vxlan_match_post_cb(csap_p              csap,
                                        unsigned int        layer_idx,
                                        tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Parse a packet received and match it against the pattern's VxLAN PDU
 * (the function complies with 'csap_layer_match_do_cb_t' prototype)
 */
extern te_errno tad_vxlan_match_do_cb(csap_p           csap,
                                      unsigned int     layer_idx,
                                      const asn_value *ptrn_pdu,
                                      void            *ptrn_opaque,
                                      tad_recv_pkt    *meta_pkt,
                                      tad_pkt         *pdu,
                                      tad_pkt         *sdu);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_VXLAN_IMPL_H__ */
