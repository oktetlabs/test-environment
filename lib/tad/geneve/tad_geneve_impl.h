/** @file
 * @brief TAD Geneve
 *
 * Traffic Application Domain Command Handler
 * Geneve CSAP implementaion internal definitions
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAD_GENEVE_IMPL_H__
#define __TE_TAD_GENEVE_IMPL_H__

#include "config.h"

#include "te_defs.h"
#include "te_errno.h"

#include "asn_usr.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize 'geneve' CSAP layer protocol-specific data
 * (the function complies with 'csap_layer_init_cb_t' prototype)
 */
extern te_errno tad_geneve_init_cb(csap_p       csap,
                                   unsigned int layer_idx);

/**
 * Teardown 'geneve' CSAP layer protocol-specific data
 * (the function complies with 'csap_layer_destroy_cb_t' prototype)
 */
extern te_errno tad_geneve_destroy_cb(csap_p       csap,
                                      unsigned int layer_idx);

/**
 * Teardown Geneve data prepared by confirm callback or packet match
 * (the function complies with 'csap_layer_release_opaque_cb_t' prototype)
 */
extern void tad_geneve_release_pdu_cb(csap_p        csap,
                                      unsigned int  layer_idx,
                                      void         *opaque);


/**
 * Confirm template PDU with respect to Geneve CSAP parameters and possibilities
 * (the function complies with 'csap_layer_confirm_pdu_cb_t' prototype)
 */
extern te_errno tad_geneve_confirm_tmpl_cb(csap_p         csap,
                                           unsigned int   layer_idx,
                                           asn_value     *layer_pdu,
                                           void         **p_opaque);

/**
 * Generate Geneve binary data
 * (the function complies with 'csap_layer_generate_pkts_cb_t' prototype)
 */
extern te_errno tad_geneve_gen_bin_cb(csap_p                csap,
                                      unsigned int          layer_idx,
                                      const asn_value      *tmpl_pdu,
                                      void                 *opaque,
                                      const tad_tmpl_arg_t *args,
                                      size_t                arg_num,
                                      tad_pkts             *sdus,
                                      tad_pkts             *pdus);

/**
 * Confirm pattern PDU with respect to Geneve CSAP parameters and possibilities
 * (the function complies with 'csap_layer_confirm_pdu_cb_t' prototype)
 */
extern te_errno tad_geneve_confirm_ptrn_cb(csap_p         csap,
                                           unsigned int   layer_idx,
                                           asn_value     *layer_pdu,
                                           void         **p_opaque);

/**
 * Generate a meta packet Geneve NDS per a packet received (if need be)
 * (the function complies with 'csap_layer_match_post_cb_t' prototype)
 */
extern te_errno tad_geneve_match_post_cb(csap_p              csap,
                                         unsigned int        layer_idx,
                                         tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Parse a packet received and match it against the pattern's Geneve PDU
 * (the function complies with 'csap_layer_match_do_cb_t' prototype)
 */
extern te_errno tad_geneve_match_do_cb(csap_p           csap,
                                       unsigned int     layer_idx,
                                       const asn_value *ptrn_pdu,
                                       void            *ptrn_opaque,
                                       tad_recv_pkt    *meta_pkt,
                                       tad_pkt         *pdu,
                                       tad_pkt         *sdu);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_GENEVE_IMPL_H__ */
