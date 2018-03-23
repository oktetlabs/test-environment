/** @file
 * @brief TAD Bridge/STP
 *
 * Traffic Application Domain Command Handler.
 * Ethernet Bridge/STP CSAP support description structures.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Bridge"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_bridge_impl.h"


static csap_spt_type_t bridge_csap_spt =
{
    .proto               = "bridge",
    .unregister_cb       = NULL,

    .init_cb             = NULL,
    .destroy_cb          = NULL,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_bridge_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_bridge_gen_bin_cb,
    .release_tmpl_cb     = NULL,

    .confirm_ptrn_cb     = tad_bridge_confirm_ptrn_cb,
    .match_pre_cb        = NULL,
    .match_do_cb         = tad_bridge_match_bin_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = NULL,
    .match_free_cb       = NULL,
    .release_ptrn_cb     = NULL,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};


/**
 * Register Bridge/STP CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Status code.
 */
te_errno
csap_support_bridge_register(void)
{
    return csap_spt_add(&bridge_csap_spt);
}
