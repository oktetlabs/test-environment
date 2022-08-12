/** @file
 * @brief TAD PPP
 *
 * Traffic Application Domain Command Handler.
 * PPP CSAP support description structures.
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TAD PPP"

#include "te_config.h"

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

