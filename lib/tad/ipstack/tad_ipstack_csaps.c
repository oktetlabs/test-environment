/** @file
 * @brief IP Stack TAD
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP support description structures. 
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include "tad_ipstack_impl.h"

/*
 * IPv4
 */

static csap_layer_neighbour_list_t ip4_nbr_eth = 
{
    NULL, 
    "eth",

    tad_ip4_eth_init_cb,
    tad_ip4_eth_destroy_cb,
};

static csap_layer_neighbour_list_t ip4_nbr_single = 
{
    &ip4_nbr_eth, 
    NULL,

    tad_ip4_single_init_cb,
    tad_ip4_single_destroy_cb,
};

static csap_spt_type_t ip4_csap_spt = 
{
    "ip4",

    NULL,
    NULL,
    tad_ip4_confirm_pdu_cb,
    tad_ip4_gen_bin_cb,
    tad_ip4_match_bin_cb,
    tad_ip4_gen_pattern_cb,

    &ip4_nbr_single
};


/*
 * UDP
 */

static csap_layer_neighbour_list_t udp_nbr_ip4 = 
{
    NULL, 
    "ip4",

    tad_udp_ip4_init_cb,
    tad_udp_ip4_destroy_cb,
};

/* It seems, there should not be UDP CSAP without any layer under it. */
#if 0 
static csap_layer_neighbour_list_t udp_nbr_single = 
{
    &udp_nbr_ip4, 
    NULL,

    tad_udp_single_init_cb,
    tad_udp_single_destroy_cb,
};
#endif

static csap_spt_type_t udp_csap_spt = 
{
    "udp",

    NULL,
    NULL,
    tad_udp_confirm_pdu_cb,
    tad_udp_gen_bin_cb,
    tad_udp_match_bin_cb,
    tad_udp_gen_pattern_cb,

    &udp_nbr_ip4
};

/*
 * TCP
 */

static csap_layer_neighbour_list_t tcp_nbr_ip4 = 
{
    NULL, /* next */
    "ip4",

    tad_tcp_ip4_init_cb,
    tad_tcp_ip4_destroy_cb,
};

static csap_spt_type_t tcp_csap_spt = 
{
    "tcp",

    NULL,
    NULL,
    tad_tcp_confirm_pdu_cb,
    tad_tcp_gen_bin_cb,
    tad_tcp_match_bin_cb,
    tad_tcp_gen_pattern_cb,

    &tcp_nbr_ip4
};


/**
 * Register ipstack CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Zero on success or error code.
 */ 
te_errno
csap_support_ipstack_register(void)
{ 
    te_errno rc;

    rc = add_csap_spt(&ip4_csap_spt);
    if (rc != 0) 
        return rc;

    rc = add_csap_spt(&tcp_csap_spt);
    if (rc != 0)
        return rc;

    return add_csap_spt(&udp_csap_spt);
}
