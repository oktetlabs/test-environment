/** @file
 * @brief TAD IGMP version 2 layer
 *
 * Traffic Application Domain Command Handler.
 * IGMPv2 CSAP support description structures.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Alexander Kukuta <kam@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TAD IGMPv2"

#include "te_config.h"

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_igmp_impl.h"


static csap_spt_type_t igmp_csap_spt =
{
    .proto               = "igmp",
    .unregister_cb       = NULL,

    .init_cb             = tad_igmp_init_cb,
    .destroy_cb          = tad_igmp_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_igmp_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_igmp_gen_bin_cb,
    .release_tmpl_cb     = tad_igmp_release_pdu_cb,

    .confirm_ptrn_cb     = tad_igmp_confirm_ptrn_cb,
    .match_pre_cb        = tad_igmp_match_pre_cb,
    .match_do_cb         = tad_igmp_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_igmp_match_post_cb,
    .match_free_cb       = tad_igmp_release_pdu_cb,
    .release_ptrn_cb     = tad_igmp_release_pdu_cb,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};


/**
 * Register IGMPv2 CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Status code.
 */
te_errno
csap_support_igmp_register(void)
{
    INFO("Register IGMP TAD layer");
    return csap_spt_add(&igmp_csap_spt);
}
