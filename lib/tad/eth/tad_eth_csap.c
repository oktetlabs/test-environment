/** @file
 * @brief TAD: Ethernet
 *
 * Traffic Application Domain Command Handler Ethernet CSAP support
 * description structures. 
 *
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
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#include <string.h>

#include "tad_eth_impl.h"


csap_layer_neighbour_list_t eth_nbr_list = 
{
    NULL,
    NULL, 
    eth_single_init_cb,
    eth_single_destroy_cb,
};

csap_spt_type_t eth_csap_spt = 
{
    "eth",
    eth_confirm_pdu_cb,
    eth_gen_bin_cb,
    eth_match_bin_cb,
    eth_gen_pattern_cb,

    &eth_nbr_list,
};


/**
 * Register Ethernet CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return zero on success or error code.
 */ 
int
csap_support_eth_register(void)
{ 
    return add_csap_spt(&eth_csap_spt);
}
