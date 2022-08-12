/** @file
 * @brief TAD DHCP
 *
 * Traffic Application Domain Command Handler.
 * DHCP CSAP support description structures.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TAD DHCP"

#include "te_config.h"

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "tad_dhcp_impl.h"

static csap_spt_type_t dhcp_csap_spt =
{
    .proto               = "dhcp",
    .unregister_cb       = NULL,

    .init_cb             = tad_dhcp_init_cb,
    .destroy_cb          = tad_dhcp_destroy_cb,
    .get_param_cb        = tad_dhcp_get_param_cb,

    .confirm_tmpl_cb     = tad_dhcp_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_dhcp_gen_bin_cb,
    .release_tmpl_cb     = tad_dhcp_release_pdu_cb,

    .confirm_ptrn_cb     = tad_dhcp_confirm_ptrn_cb,
    .match_pre_cb        = tad_dhcp_match_pre_cb,
    .match_do_cb         = tad_dhcp_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_dhcp_match_post_cb,
    .match_free_cb       = tad_dhcp_release_pdu_cb,
    .release_ptrn_cb     = tad_dhcp_release_pdu_cb,

    .generate_pattern_cb = tad_dhcp_gen_pattern_cb,

    .rw_init_cb          = tad_dhcp_rw_init_cb,
    .rw_destroy_cb       = tad_dhcp_rw_destroy_cb,

    .prepare_send_cb     = NULL,
    .write_cb            = tad_dhcp_write_cb,
    .shutdown_send_cb    = NULL,

    .prepare_recv_cb     = NULL,
    .read_cb             = tad_dhcp_read_cb,
    .shutdown_recv_cb    = NULL,

    .write_read_cb       = tad_common_write_read_cb,
};

static csap_spt_type_t dhcp6_csap_spt =
{
    .proto               = "dhcp6",
    .unregister_cb       = NULL,

    .init_cb             = tad_dhcp6_init_cb,
    .destroy_cb          = tad_dhcp_destroy_cb,
    .get_param_cb        = tad_dhcp_get_param_cb,

    .confirm_tmpl_cb     = tad_dhcp6_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_dhcp6_gen_bin_cb,
    .release_tmpl_cb     = tad_dhcp_release_pdu_cb,

    .confirm_ptrn_cb     = tad_dhcp_confirm_ptrn_cb,
    .match_pre_cb        = tad_dhcp_match_pre_cb,
    .match_do_cb         = tad_dhcp_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_dhcp6_match_post_cb,
    .match_free_cb       = tad_dhcp_release_pdu_cb,
    .release_ptrn_cb     = tad_dhcp_release_pdu_cb,

    .generate_pattern_cb = tad_dhcp6_gen_pattern_cb,

    .rw_init_cb          = tad_dhcp6_rw_init_cb,
    .rw_destroy_cb       = tad_dhcp_rw_destroy_cb,

    .prepare_send_cb     = NULL,
    .write_cb            = tad_dhcp6_write_cb,
    .shutdown_send_cb    = NULL,

    .prepare_recv_cb     = NULL,
    .read_cb             = tad_dhcp_read_cb,
    .shutdown_recv_cb    = NULL,

    .write_read_cb       = tad_common_write_read_cb,
};

/**
 * Register dhcp CSAP callbacks and support structures in
 * TAD Command Handler.
 *
 * @return Status code
 */
te_errno
csap_support_dhcp_register(void)
{
    return csap_spt_add(&dhcp_csap_spt);
}

/**
 * Register dhcp6 CSAP callbacks and support structures in
 * TAD Command Handler.
 *
 * @return Status code
 */
te_errno
csap_support_dhcp6_register(void)
{
    return csap_spt_add(&dhcp6_csap_spt);
}
