/** @file
 * @brief TAD Frame
 *
 * Traffic Application Domain Command Handler.
 * Frame layer support description structures. 
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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

#define TE_LGR_USER     "TAD Frame"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_frame_impl.h"


static csap_spt_type_t frame_csap_spt = 
{
    proto               : "frame",
    unregister_cb       : NULL,

    init_cb             : NULL,
    destroy_cb          : NULL,
    get_param_cb        : NULL,

    confirm_tmpl_cb     : NULL,
    generate_pkts_cb    : tad_frame_gen_bin_cb,
    release_tmpl_cb     : NULL,

    confirm_ptrn_cb     : tad_frame_confirm_ptrn_cb,
    match_pre_cb        : NULL,
    match_do_cb         : tad_frame_match_do_cb,
    match_done_cb       : NULL,
    match_post_cb       : NULL,
    match_free_cb       : NULL,
    release_ptrn_cb     : tad_frame_release_ptrn_cb,

    generate_pattern_cb : NULL,

    CSAP_SUPPORT_NO_RW,
};


/**
 * Register 'frame' layer callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Status code.
 */ 
te_errno
csap_support_frame_register(void)
{ 
    return csap_spt_add(&frame_csap_spt);
}
