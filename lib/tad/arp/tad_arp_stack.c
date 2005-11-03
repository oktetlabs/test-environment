/** @file
 * @brief ARP TAD
 *
 * Traffic Application Domain Command Handler
 * ARP CSAP stack-related callbacks.
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

#define TE_LGR_USER     "TAD ARP stack"

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"
#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_arp_impl.h"


/* See description in tad_arp_impl.h */
te_errno
tad_arp_eth_init_cb(int csap_id, const asn_value *csap_nds,
                    unsigned int layer)
{
    F_ENTRY("(%d:%u) nds=%p", csap_id, layer, (void *)csap_nds);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_eth_destroy_cb(int csap_id, unsigned int layer)
{
    F_ENTRY("(%d:%u)", csap_id, layer);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}
