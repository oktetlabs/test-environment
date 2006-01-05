/** @file
 * @brief TAD ATM
 *
 * Traffic Application Domain Command Handler.
 * ATM CSAP support internal declarations.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TAD_ATM_IMPL_H__
#define __TE_TAD_ATM_IMPL_H__ 

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h"

#include "tad_csap_support.h"
#include "tad_csap_inst.h"


/**
 * ATM CSAP specific data
 */
typedef struct atm_csap_specific_data {
    uint16_t    hw_type;
    uint16_t    proto_type;
    uint8_t     hw_size;
    uint8_t     proto_size;
} atm_csap_specific_data_t;


/**
 * Callback to initialize 'atm' CSAP layer if over 'eth' in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */
extern te_errno tad_atm_eth_init_cb(csap_p           csap,
                                    unsigned int     layer,
                                    const asn_value *csap_nds);

/**
 * Callback to destroy 'atm' CSAP layer if over 'eth' in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */
extern te_errno tad_atm_eth_destroy_cb(csap_p       csap,
                                       unsigned int layer);


/**
 * Callback for read parameter value of ATM CSAP.
 *
 * The function complies with csap_layer_get_param_cb_t prototype.
 */
extern char *tad_atm_get_param_cb(csap_p        csap,
                                  unsigned int  layer,
                                  const char   *param);

/**
 * Callback for confirm PDU with ATM CSAP parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_atm_confirm_pdu_cb(csap_p         csap,
                                       unsigned int   layer, 
                                       asn_value     *layer_pdu,
                                       void         **p_opaque); 

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

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_atm_match_bin_cb(csap_p           csap,
                                     unsigned int     layer,
                                     const asn_value *pattern_pdu,
                                     const csap_pkts *pkt,
                                     csap_pkts       *payload,
                                     asn_value       *parsed_packet);

#endif /* !__TE_TAD_ATM_IMPL_H__ */
