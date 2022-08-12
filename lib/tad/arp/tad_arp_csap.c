/** @file
 * @brief TAD ARP
 *
 * Traffic Application Domain Command Handler.
 * ARP CSAP support description structures.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#define TE_LGR_USER     "TAD ARP"

#include "te_config.h"

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_arp_impl.h"


static csap_spt_type_t arp_csap_spt =
{
    .proto               = "arp",
    .unregister_cb       = NULL,

    .init_cb             = tad_arp_init_cb,
    .destroy_cb          = tad_arp_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_arp_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_arp_gen_bin_cb,
    .release_tmpl_cb     = NULL,

    .confirm_ptrn_cb     = tad_arp_confirm_ptrn_cb,
    .match_pre_cb        = tad_arp_match_pre_cb,
    .match_do_cb         = tad_arp_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_arp_match_post_cb,
    .match_free_cb       = NULL,
    .release_ptrn_cb     = NULL,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};


/**
 * Register ARP CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Status code.
 */
te_errno
csap_support_arp_register(void)
{
    return csap_spt_add(&arp_csap_spt);
}
