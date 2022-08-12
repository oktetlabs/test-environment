/** @file
 * @brief TAD ARP
 *
 * Traffic Application Domain Command Handler.
 * ARP CSAP support internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TAD_ARP_IMPL_H__
#define __TE_TAD_ARP_IMPL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h"

#include "tad_csap_support.h"
#include "tad_csap_inst.h"


/**
 * Callback to initialize 'arp' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_arp_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback to destroy 'arp' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_arp_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback for confirm template PDU with ARP CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_arp_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Callback for confirm pattern PDU with ARP CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_arp_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_arp_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);


extern te_errno tad_arp_match_pre_cb(csap_p              csap,
                                     unsigned int        layer,
                                     tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_arp_match_post_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_arp_match_do_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);

#endif /* !__TE_TAD_ARP_IMPL_H__ */
