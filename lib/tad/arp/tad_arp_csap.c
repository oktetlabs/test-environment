/** @file
 * @brief ARP TAD
 *
 * Traffic Application Domain Command Handler
 * ARP CSAP support description structures. 
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

#define TE_LGR_USER     "TAD ARP CSAP"

#include "tad_csap_support.h"

#include "tad_arp_impl.h"


static csap_layer_neighbour_list_t arp_nbr_eth = 
{
    NULL, 
    "eth",

    tad_arp_eth_init_cb,
    tad_arp_eth_destroy_cb,
};

static csap_spt_type_t arp_csap_spt = 
{
    "arp",

    NULL,
    NULL,
    tad_arp_confirm_pdu_cb,
    tad_arp_gen_bin_cb,
    tad_arp_match_bin_cb,
    tad_arp_gen_pattern_cb,

    &arp_nbr_eth
};



/**
 * Register ARP CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Zero on success or error code.
 */ 
te_errno
csap_support_arp_register(void)
{ 
    return add_csap_spt(&arp_csap_spt);
}
