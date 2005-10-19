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
 * Author: Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#define TE_LGR_USER     "TAD DHCP CSAP"

#include "config.h"

#include <string.h>

#include "logger_api.h"
#include "tad_dhcp_impl.h"


csap_layer_neighbour_list_t dhcp_nbr_list = 
{
    NULL,
    NULL, 
    dhcp_single_init_cb,
    dhcp_single_destroy_cb,
};

csap_spt_type_t dhcp_csap_spt = 
{
    "dhcp",
    dhcp_confirm_pdu_cb,
    dhcp_gen_bin_cb,
    dhcp_match_bin_cb,
    dhcp_gen_pattern_cb,

    &dhcp_nbr_list
};

/**
 * Register dhcpernet CSAP callbacks and support structures in
 * TAD Command Handler.
 *
 * @return zero on success or error code.
 */ 
int
csap_support_dhcp_register(void)
{ 
    VERB("match=%x", dhcp_csap_spt.match_cb);
    return add_csap_spt(&dhcp_csap_spt);
}

