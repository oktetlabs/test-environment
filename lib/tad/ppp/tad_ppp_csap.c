/** @file
 * @brief TAD PPP
 *
 * Traffic Application Domain Command Handler.
 * PPP CSAP support description structures. 
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

#define TE_LGR_USER     "TAD PPP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_ppp_impl.h"



static csap_spt_type_t ppp_csap_spt =
{
    .proto               = "ppp",
    .unregister_cb       = NULL,

    .init_cb             = tad_ppp_init_cb,
    .destroy_cb          = tad_ppp_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_ppp_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_ppp_gen_bin_cb,
    .release_tmpl_cb     = tad_ppp_release_pdu_cb,

    .confirm_ptrn_cb     = tad_ppp_confirm_ptrn_cb,
    .match_pre_cb        = tad_ppp_match_pre_cb,
    .match_do_cb         = tad_ppp_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_ppp_match_post_cb,
    .match_free_cb       = tad_ppp_release_pdu_cb,
    .release_ptrn_cb     = tad_ppp_release_pdu_cb,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};

static csap_spt_type_t pppoe_csap_spt =
{
    .proto               = "pppoe",
    .unregister_cb       = NULL,

    .init_cb             = tad_pppoe_init_cb,
    .destroy_cb          = tad_pppoe_destroy_cb,
#if 0
    .get_param_cb        = tad_pppoe_get_param_cb,
#endif
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_pppoe_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_pppoe_gen_bin_cb,
    .release_tmpl_cb     = tad_pppoe_release_pdu_cb,

    .confirm_ptrn_cb     = tad_pppoe_confirm_ptrn_cb,
    .match_pre_cb        = tad_pppoe_match_pre_cb,
    .match_do_cb         = tad_pppoe_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_pppoe_match_post_cb,
    .match_free_cb       = tad_pppoe_release_pdu_cb,
    .release_ptrn_cb     = tad_pppoe_release_pdu_cb,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};

/**
 * Register PPP & PPPoE CSAPs callbacks and support structures in
 * TAD Command Handler.
 *
 * @return Status code
 */
te_errno
csap_support_ppp_register(void)
{
    te_errno rc;

    rc = csap_spt_add(&ppp_csap_spt);
    if (rc != 0)
        return rc;

    return csap_spt_add(&pppoe_csap_spt);
}

