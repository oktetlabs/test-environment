/** @file
 * @brief iSCSI TAD
 *
 * Traffic Application Domain Command Handler
 * iSCSI CSAP support description structures. 
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD iSCSI CSAP"

#include "te_config.h"

#include "tad_iscsi_impl.h"


static csap_layer_neighbour_list_t iscsi_nbr_list = 
{
    NULL,
    NULL, 

    tad_iscsi_single_init_cb,
    tad_iscsi_single_destroy_cb,
};

static csap_spt_type_t iscsi_csap_spt = 
{
    "iscsi",

    NULL,
    NULL,
    tad_iscsi_confirm_pdu_cb,
    tad_iscsi_gen_bin_cb,
    tad_iscsi_match_bin_cb,
    tad_iscsi_gen_pattern_cb,

    &iscsi_nbr_list
};


/**
 * Register iSCSI CSAP callbacks and support structures in
 * TAD Command Handler.
 *
 * @return Zero on success or error code
 */ 
te_errno
csap_support_iscsi_register(void)
{ 
    return add_csap_spt(&iscsi_csap_spt);
}
