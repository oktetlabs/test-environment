/** @file
 * @brief CLI TAD
 *
 * Traffic Application Domain Command Handler
 * CLI CSAP support description structures. 
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD CLI CSAP"

#include "te_config.h"

#include "tad_cli_impl.h"


static csap_layer_neighbour_list_t cli_nbr_list = 
{
    NULL,
    NULL, 

    tad_cli_single_init_cb,
    tad_cli_single_destroy_cb,
};

static csap_spt_type_t cli_csap_spt = 
{
    "cli",

    tad_cli_confirm_pdu_cb,
    tad_cli_gen_bin_cb,
    tad_cli_match_bin_cb,
    tad_cli_gen_pattern_cb,

    &cli_nbr_list
};


/**
 * Register CLI CSAP callbacks and support structures in TAD Command Handler.
 *
 * @return Zero on success or error code
 */ 
te_errno
csap_support_cli_register(void)
{ 
    return add_csap_spt(&cli_csap_spt);
}
