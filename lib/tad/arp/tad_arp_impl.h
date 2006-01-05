/** @file
 * @brief TAD ARP
 *
 * Traffic Application Domain Command Handler.
 * ARP CSAP support internal declarations.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
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
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */
extern te_errno tad_arp_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback for confirm PDU with ARP CSAP parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_arp_confirm_pdu_cb(csap_p         csap,
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

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_arp_match_bin_cb(csap_p           csap,
                                     unsigned int     layer,
                                     const asn_value *pattern_pdu,
                                     const csap_pkts *pkt,
                                     csap_pkts       *payload,
                                     asn_value       *parsed_packet);

#endif /* !__TE_TAD_ARP_IMPL_H__ */
