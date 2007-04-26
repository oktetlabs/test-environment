/** @file
 * @brief TAD CLI
 *
 * Traffic Application Domain Command Handler.
 * CLI CSAP support description structures. 
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD CLI"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_cli_impl.h"


static csap_spt_type_t cli_csap_spt = 
{
    .proto               = "cli",
    .unregister_cb       = NULL,

    .init_cb             = NULL,
    .destroy_cb          = NULL,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = NULL,
    .generate_pkts_cb    = tad_cli_gen_bin_cb,
    .release_tmpl_cb     = NULL,

    .confirm_ptrn_cb     = NULL,
    .match_pre_cb        = NULL,
    .match_do_cb         = tad_cli_match_bin_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = NULL,
    .match_free_cb       = NULL,
    .release_ptrn_cb     = NULL,

    .generate_pattern_cb = tad_cli_gen_pattern_cb,

    .rw_init_cb          = tad_cli_rw_init_cb,
    .rw_destroy_cb       = tad_cli_rw_destroy_cb,

    .prepare_send_cb     = NULL,
    .write_cb            = tad_cli_write_cb,
    .shutdown_send_cb    = NULL,
    
    .prepare_recv_cb     = NULL,
    .read_cb             = tad_cli_read_cb,
    .shutdown_recv_cb    = NULL,

    .write_read_cb       = tad_cli_write_read_cb,
};


/**
 * Register CLI CSAP callbacks and support structures in TAD Command Handler.
 *
 * @return Status code
 */ 
te_errno
csap_support_cli_register(void)
{ 
    return csap_spt_add(&cli_csap_spt);
}
