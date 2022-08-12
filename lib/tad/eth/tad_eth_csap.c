/** @file
 * @brief TAD Ethernet
 *
 * Traffic Application Domain Command Handler.
 * Ethernet CSAP support description structures.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#define TE_LGR_USER     "TAD Ethernet"

#include "te_config.h"

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_eth_impl.h"


static csap_spt_type_t eth_csap_spt =
{
    .proto               = "eth",
    .unregister_cb       = NULL,

    .init_cb             = tad_eth_init_cb,
    .destroy_cb          = tad_eth_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_eth_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_eth_gen_bin_cb,
    .release_tmpl_cb     = tad_eth_release_pdu_cb,

    .confirm_ptrn_cb     = tad_eth_confirm_ptrn_cb,
    .match_pre_cb        = tad_eth_match_pre_cb,
    .match_do_cb         = tad_eth_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_eth_match_post_cb,
    .match_free_cb       = tad_eth_release_pdu_cb,
    .release_ptrn_cb     = tad_eth_release_pdu_cb,

    .generate_pattern_cb = NULL,

    .rw_init_cb          = tad_eth_rw_init_cb,
    .rw_destroy_cb       = tad_eth_rw_destroy_cb,

    .prepare_send_cb     = tad_eth_prepare_send,
    .write_cb            = tad_eth_write_cb,
    .shutdown_send_cb    = tad_eth_shutdown_send,

    .prepare_recv_cb     = tad_eth_prepare_recv,
    .read_cb             = tad_eth_read_cb,
    .shutdown_recv_cb    = tad_eth_shutdown_recv,

    .write_read_cb       = tad_common_write_read_cb,
};


/**
 * Register Ethernet CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Status code.
 */
te_errno
csap_support_eth_register(void)
{
    return csap_spt_add(&eth_csap_spt);
}
