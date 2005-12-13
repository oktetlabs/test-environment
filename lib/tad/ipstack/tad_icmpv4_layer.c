/** @file
 * @brief IP Stack TAD
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP layer-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "tad_ipstack_impl.h"


/* See description in tad_ipstack_impl.h */
char *
icmp4_get_param_cb(csap_p csap_descr, unsigned int layer, const char *param)
{
    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(param);
    return NULL;
}


/* See description in tad_ipstack_impl.h */
int 
icmp4_confirm_pdu_cb(csap_p csap_descr, unsigned int layer,
                     asn_value_p layer_pdu)
{ 
    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(layer_pdu);
    return 0;
}


/* See description in tad_ipstack_impl.h */
te_errno
icmp4_gen_bin_cb(csap_p csap_descr, unsigned int layer, const asn_value *tmpl_pdu,
                 const tad_tmpl_arg_t *args, size_t arg_num, 
                 const csap_pkts_p  up_payload, csap_pkts_p pkts)
{ 
    UNUSED(up_payload);
    UNUSED(tmpl_pdu); 
    UNUSED(pkts); 
    UNUSED(args); 
    UNUSED(arg_num); 
    UNUSED(csap_descr); 
    UNUSED(layer); 
    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}


/* See description in tad_ipstack_impl.h */
int
icmp4_match_bin_cb(csap_p csap_descr, unsigned int layer,
                   const asn_value *pattern_pdu,
                   const csap_pkts *pkt, csap_pkts *payload, 
                   asn_value_p  parsed_packet )
{ 
    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(pkt);
    UNUSED(payload);
    UNUSED(pattern_pdu);
    UNUSED(parsed_packet);

    return TE_EOPNOTSUPP;
}


/* See description in tad_ipstack_impl.h */
int
icmp4_gen_pattern_cb(csap_p csap_descr, unsigned int layer,
                     const asn_value *tmpl_pdu, asn_value_p *pattern_pdu)
{
    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(tmpl_pdu); 
    UNUSED(pattern_pdu); 
    return TE_EOPNOTSUPP;
}
