/** @file
 * @brief TAD RTE mbuf
 *
 * Traffic Application Domain Command Handler
 * RTE mbuf CSAP support description structures
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#define TE_LGR_USER     "TAD RTE mbuf"

#include "te_config.h"

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_rte_mbuf_impl.h"

static csap_spt_type_t rte_mbuf_csap_spt =
{
    .proto               = "rtembuf",
    .unregister_cb       = NULL,

    .init_cb             = NULL,
    .destroy_cb          = NULL,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = NULL,
    .generate_pkts_cb    = tad_rte_mbuf_gen_bin_cb,
    .release_tmpl_cb     = NULL,

    .confirm_ptrn_cb     = NULL,
    .match_pre_cb        = NULL,
    .match_do_cb         = tad_rte_mbuf_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_rte_mbuf_match_post_cb,
    .match_free_cb       = NULL,
    .release_ptrn_cb     = NULL,

    .generate_pattern_cb = NULL,

    .rw_init_cb          = tad_rte_mbuf_rw_init_cb,
    .rw_destroy_cb       = tad_rte_mbuf_rw_destroy_cb,

    .prepare_send_cb     = NULL,
    .write_cb            = tad_rte_mbuf_write_cb,
    .shutdown_send_cb    = NULL,

    .prepare_recv_cb     = NULL,
    .read_cb             = tad_rte_mbuf_read_cb,
    .shutdown_recv_cb    = NULL,

    .write_read_cb       = tad_common_write_read_cb,
};

/**
 * Register RTE mbuf CSAP callbacks in TAD Command Handler
 *
 * @return Status code
 */
te_errno
csap_support_rte_mbuf_register(void)
{
    return csap_spt_add(&rte_mbuf_csap_spt);
}
