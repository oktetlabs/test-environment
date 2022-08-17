/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD CLI
 *
 * Traffic Application Domain Command Handler.
 * CLI CSAP support description structures.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD CLI"

#include "te_config.h"

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
