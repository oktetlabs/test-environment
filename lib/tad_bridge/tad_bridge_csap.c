/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP support description structures. 
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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "tad_bridge_impl.h"


csap_layer_neighbour_list_t bridge_nbr_list = 
{
    "eth",
    NULL, 
    bridge_eth_init_cb,
    bridge_eth_destroy_cb,
};

csap_spt_type_t bridge_csap_spt = 
{
    "bridge",
    bridge_confirm_pdu_cb,
    bridge_gen_bin_cb,
    bridge_match_bin_cb,
    bridge_gen_pattern_cb,

    &bridge_nbr_list
};

/**
 * Register bridgeernet CSAP callbacks and support structures in TAD Command Handler.
 *
 * @return zero on success or error code.
 */ 
int csap_support_bridge_register (void)
{ 
    return add_csap_spt(&bridge_csap_spt);
}

