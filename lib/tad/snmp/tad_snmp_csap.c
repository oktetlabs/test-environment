/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Dummy FILE protocol implementaion, CSAP support description structures. 
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

#include "tad_snmp_impl.h"

#define TE_LGR_USER     "TAD SNMP"
#include "logger_ta.h"

#include <string.h>


csap_layer_neighbour_list_t snmp_nbr_list = 
{
    NULL,
    NULL, 
    snmp_single_init_cb,
    snmp_single_destroy_cb,
};

csap_spt_type_t snmp_csap_spt = 
{
    "snmp",

    snmp_confirm_pdu_cb,
    snmp_gen_bin_cb,
    snmp_match_bin_cb,
    snmp_gen_pattern_cb,

    &snmp_nbr_list
};

/**
 * Register 'snmp' CSAP callbacks and support structures in TAD Command Handler.
 *
 * @return zero on success or error code.
 */ 
int csap_support_snmp_register (void)
{ 
    return add_csap_spt(&snmp_csap_spt);
}

