/** @file
 * @brief TAD iSCSI
 *
 * Traffic Application Domain Command Handler.
 * iSCSI CSAP support description structures.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD iSCSI"

#include "te_config.h"

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_iscsi_impl.h"


static csap_spt_type_t iscsi_csap_spt =
{
    .proto               = "iscsi",
    .unregister_cb       = NULL,

    .init_cb             = tad_iscsi_init_cb,
    .destroy_cb          = tad_iscsi_destroy_cb,
    .get_param_cb        = tad_iscsi_get_param_cb,

    .confirm_tmpl_cb     = NULL,
    .generate_pkts_cb    = tad_iscsi_gen_bin_cb,
    .release_tmpl_cb     = NULL,

    .confirm_ptrn_cb     = NULL,
    .match_pre_cb        = NULL,
    .match_do_cb         = tad_iscsi_match_bin_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = NULL,
    .match_free_cb       = NULL,
    .release_ptrn_cb     = NULL,

    .generate_pattern_cb = tad_iscsi_gen_pattern_cb,

    .rw_init_cb          = tad_iscsi_rw_init_cb,
    .rw_destroy_cb       = tad_iscsi_rw_destroy_cb,

    .prepare_send_cb     = NULL,
    .write_cb            = tad_iscsi_write_cb,
    .shutdown_send_cb    = NULL,

    .prepare_recv_cb     = NULL,
    .read_cb             = tad_iscsi_read_cb,
    .shutdown_recv_cb    = NULL,

    .write_read_cb       = NULL,
};


/**
 * Register iSCSI CSAP callbacks and support structures in
 * TAD Command Handler.
 *
 * @return Status code
 */
te_errno
csap_support_iscsi_register(void)
{
    return csap_spt_add(&iscsi_csap_spt);
}
