/** @file
 * @brief TAD Socket
 *
 * Traffic Application Domain Command Handler.
 * Socket CSAP support description structures.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TAD Socket"

#include "te_config.h"

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_socket_impl.h"


static csap_spt_type_t socket_csap_spt =
{
    .proto               = "socket",
    .unregister_cb       = NULL,

    .init_cb             = NULL,
    .destroy_cb          = NULL,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_socket_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_socket_gen_bin_cb,
    .release_tmpl_cb     = NULL,

    .confirm_ptrn_cb     = NULL,
    .match_pre_cb        = NULL,
    .match_do_cb         = tad_socket_match_bin_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = NULL,
    .match_free_cb       = NULL,
    .release_ptrn_cb     = NULL,

    .generate_pattern_cb = NULL,

    .rw_init_cb          = tad_socket_rw_init_cb,
    .rw_destroy_cb       = tad_socket_rw_destroy_cb,

    .prepare_send_cb     = NULL,
    .write_cb            = tad_socket_write_cb,
    .shutdown_send_cb    = NULL,

    .prepare_recv_cb     = NULL,
    .read_cb             = tad_socket_read_cb,
    .shutdown_recv_cb    = NULL,

    .write_read_cb       = tad_common_write_read_cb,
};


/**
 * Register Socket CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Status code.
 */
te_errno
csap_support_socket_register(void)
{
    return csap_spt_add(&socket_csap_spt);
}
