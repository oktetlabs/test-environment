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
    te_bool indication; /**< ATM-user-to-ATM-user indication */
} tad_atm_cell_ctrl_data;


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
 * Callback for confirm PDU with ATM CSAP parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_aal5_confirm_pdu_cb(csap_p         csap,
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
                                      const asn_value *pattern_pdu,
                                      const csap_pkts *pkt,
                                      csap_pkts       *payload,
                                      asn_value       *parsed_packet);


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
