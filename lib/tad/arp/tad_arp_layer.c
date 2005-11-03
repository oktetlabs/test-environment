/** @file
 * @brief ARP TAD
 *
 * Traffic Application Domain Command Handler
 * ARP CSAP layer-related callbacks.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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

#define TE_LGR_USER     "TAD ARP layer"

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"
#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_arp_impl.h"


/* See description in tad_arp_impl.h */
char *
tad_arp_get_param_cb(csap_p csap_descr, unsigned int  layer,
                     const char *param)
{
    assert(csap_descr != NULL);
    ENTRY("(%d:%u) param=%s", csap_descr->id, layer, param);

    return NULL;
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_confirm_pdu_cb(int csap_id, unsigned int  layer, 
                       asn_value *traffic_pdu)
{
    F_ENTRY("(%d:%u) traffic_pdu=%p", csap_id, layer, (void *)traffic_pdu);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_gen_bin_cb(csap_p                csap_descr,
                   unsigned int          layer,
                   const asn_value      *tmpl_pdu,
                   const tad_tmpl_arg_t *args,
                   size_t                arg_num,
                   csap_pkts_p           up_payload,
                   csap_pkts_p           pkts)
{
    assert(csap_descr != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u up_payload=%p pkts=%p",
            csap_descr->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, up_payload, pkts);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_match_bin_cb(int              csap_id,
                     unsigned int     layer,
                     const asn_value *pattern_pdu,
                     const csap_pkts *pkt,
                     csap_pkts       *payload,
                     asn_value       *parsed_packet)
{
    F_ENTRY("(%d:%u) pattern_pdu=%p pkt=%p payload=%p parsed_packet=%p",
            csap_id, layer, (void *)pattern_pdu, pkt, payload,
            (void *)parsed_packet);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_gen_pattern_cb(int csap_id, unsigned int layer,
                       const asn_value *tmpl_pdu, asn_value **pattern_pdu)
{
    F_ENTRY("(%d:%u) tmpl_pdu=%p pattern_pdu=%p",
            csap_id, layer, (void *)tmpl_pdu, pattern_pdu);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}
