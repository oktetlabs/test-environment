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

#include <string.h>
#include "tad_ipstack_impl.h"

/*
 * IPv4
 */

csap_layer_neighbour_list_t ip4_nbr_eth = 
{
    "eth",
    NULL, 
    ip4_eth_init_cb,
    ip4_eth_destroy_cb,
};

csap_layer_neighbour_list_t ip4_nbr_single = 
{
    NULL,
    &ip4_nbr_eth, 
    ip4_single_init_cb,
    ip4_single_destroy_cb,
};

csap_spt_type_t ip4_csap_spt = 
{
    "ip4",
    ip4_confirm_pdu_cb,
    ip4_gen_bin_cb,
    ip4_match_bin_cb,
    ip4_gen_pattern_cb,

    &ip4_nbr_single
};


/*
 * UDP
 */

csap_layer_neighbour_list_t udp_nbr_ip4 = 
{
    "ip4",
    NULL, 
    udp_ip4_init_cb,
    udp_ip4_destroy_cb,
};

/* It seems, there should not be UDP CSAP without any layer under it. */
#if 0 
csap_layer_neighbour_list_t udp_nbr_single = 
{
    NULL,
    &udp_nbr_ip4, 
    udp_single_init_cb,
    udp_single_destroy_cb,
};
#endif

csap_spt_type_t udp_csap_spt = 
{
    "udp",
    udp_confirm_pdu_cb,
    udp_gen_bin_cb,
    udp_match_bin_cb,
    udp_gen_pattern_cb,

    &udp_nbr_ip4
};

/**
 * Register ipstack CSAP callbacks and support structures in TAD Command Handler.
 *
 * @return zero on success or error code.
 */ 
int csap_support_ipstack_register (void)
{ 
    int rc;
    rc = add_csap_spt(&ip4_csap_spt);
    if (rc) 
        return rc;

    return add_csap_spt(&udp_csap_spt);
}

