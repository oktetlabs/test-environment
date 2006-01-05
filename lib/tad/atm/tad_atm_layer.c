/** @file
 * @brief TAD ATM
 *
 * Traffic Application Domain Command Handler.
 * ATM CSAP layer-related callbacks.
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

#define TE_LGR_USER     "TAD ATM"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"
#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_atm_impl.h"



/* See description in tad_atm_impl.h */
te_errno
tad_atm_init_cb(csap_p csap, unsigned int layer)
{
    return 0;
}

/* See description in tad_atm_impl.h */
te_errno
tad_atm_eth_destroy_cb(csap_p csap, unsigned int layer)
{
    return 0;
}


/* See description in tad_atm_impl.h */
te_errno
tad_atm_confirm_pdu_cb(csap_p csap, unsigned int  layer, 
                       asn_value *layer_pdu, void **p_opaque)
{
    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}

/* See description in tad_atm_impl.h */
te_errno
tad_atm_gen_bin_cb(csap_p                csap,
                   unsigned int          layer,
                   const asn_value      *tmpl_pdu,
                   void                 *opaque,
                   const tad_tmpl_arg_t *args,
                   size_t                arg_num,
                   tad_pkts             *sdus,
                   tad_pkts             *pdus)
{
    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}

/* See description in tad_atm_impl.h */
te_errno
tad_atm_match_bin_cb(csap_p           csap,
                     unsigned int     layer,
                     const asn_value *pattern_pdu,
                     const csap_pkts *pkt,
                     csap_pkts       *payload,
                     asn_value       *parsed_packet)
{
    F_ENTRY("(%d:%u) pattern_pdu=%p pkt=%p payload=%p parsed_packet=%p",
            csap->id, layer, (void *)pattern_pdu, pkt, payload,
            (void *)parsed_packet);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}
