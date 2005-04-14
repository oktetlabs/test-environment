/** @file
 * @brief TAD: Ethernet with Lib PCAP filtering
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#include <string.h>

#include "tad_pcap_impl.h"


csap_layer_neighbour_list_t pcap_nbr_list = 
{
    NULL,
    NULL, 
    pcap_single_init_cb,
    pcap_single_destroy_cb,
};

csap_spt_type_t eth_csap_spt = 
{
    "pcap",
    pcap_confirm_pdu_cb,
    pcap_gen_bin_cb,
    pcap_match_bin_cb,
    pcap_gen_pattern_cb,

    &pcap_nbr_list,
};


/**
 * Register Ethernet-PCAP CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return zero on success or error code.
 */ 
int
csap_support_pcap_register(void)
{ 
    return add_csap_spt(&pcap_csap_spt);
}
