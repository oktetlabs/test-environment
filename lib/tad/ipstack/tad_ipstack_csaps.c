/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * IP Stack CSAPs support description structures.
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD IP Stack"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_ipstack_impl.h"


/**
 * IPv4
 */
static csap_spt_type_t ip4_csap_spt = 
{
    .proto               = "ip4",
    .unregister_cb       = NULL,

    .init_cb             = tad_ip4_init_cb,
    .destroy_cb          = tad_ip4_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_ip4_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_ip4_gen_bin_cb,
    .release_tmpl_cb     = tad_ip4_release_pdu_cb,

    .confirm_ptrn_cb     = tad_ip4_confirm_ptrn_cb,
    .match_pre_cb        = tad_ip4_match_pre_cb,
    .match_do_cb         = tad_ip4_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_ip4_match_post_cb,
    .match_free_cb       = tad_ip4_release_pdu_cb,
    .release_ptrn_cb     = tad_ip4_release_pdu_cb,

    .generate_pattern_cb = NULL,

    .rw_init_cb          = tad_ip4_rw_init_cb,
    .rw_destroy_cb       = tad_ip4_rw_destroy_cb,

    .prepare_send_cb     = NULL,
    .write_cb            = tad_ip4_write_cb,
    .shutdown_send_cb    = NULL,
    
    .prepare_recv_cb     = NULL,
    .read_cb             = tad_ip4_read_cb,
    .shutdown_recv_cb    = NULL,

    .write_read_cb       = tad_common_write_read_cb,
};

/**
 * IPv6
 */
static csap_spt_type_t ip6_csap_spt =
{
    .proto               = "ip6",
    .unregister_cb       = NULL,

    .init_cb             = tad_ip6_init_cb,
    .destroy_cb          = tad_ip6_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_ip6_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_ip6_gen_bin_cb,
    .release_tmpl_cb     = tad_ip6_release_pdu_cb,

    .confirm_ptrn_cb     = tad_ip6_confirm_ptrn_cb,
    .match_pre_cb        = tad_ip6_match_pre_cb,
    .match_do_cb         = tad_ip6_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_ip6_match_post_cb,
    .match_free_cb       = tad_ip6_release_pdu_cb,
    .release_ptrn_cb     = tad_ip6_release_pdu_cb,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};

/**
 * ICMPv4
 */
static csap_spt_type_t icmp4_csap_spt = 
{
    .proto               = "icmp4",
    .unregister_cb       = NULL,

    .init_cb             = tad_icmp4_init_cb,
    .destroy_cb          = tad_icmp4_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_icmp4_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_icmp4_gen_bin_cb,
    .release_tmpl_cb     = tad_icmp4_release_pdu_cb,

    .confirm_ptrn_cb     = tad_icmp4_confirm_ptrn_cb,
    .match_pre_cb        = tad_icmp4_match_pre_cb,
    .match_do_cb         = tad_icmp4_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_icmp4_match_post_cb,
    .match_free_cb       = tad_icmp4_release_pdu_cb,
    .release_ptrn_cb     = tad_icmp4_release_pdu_cb,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};

/**
 * ICMPv6
 */
static csap_spt_type_t icmp6_csap_spt = 
{
    .proto               = "icmp6",
    .unregister_cb       = NULL,

    .init_cb             = tad_icmp6_init_cb,
    .destroy_cb          = tad_icmp6_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_icmp6_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_icmp6_gen_bin_cb,
    .release_tmpl_cb     = tad_icmp6_release_pdu_cb,

    .confirm_ptrn_cb     = tad_icmp6_confirm_ptrn_cb,
    .match_pre_cb        = tad_icmp6_match_pre_cb,
    .match_do_cb         = tad_icmp6_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_icmp6_match_post_cb,
    .match_free_cb       = tad_icmp6_release_pdu_cb,
    .release_ptrn_cb     = tad_icmp6_release_pdu_cb,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};

/**
 * UDP
 */
static csap_spt_type_t udp_csap_spt = 
{
    .proto               = "udp",
    .unregister_cb       = NULL,

    .init_cb             = tad_udp_init_cb,
    .destroy_cb          = tad_udp_destroy_cb,
    .get_param_cb        = NULL,

    .confirm_tmpl_cb     = tad_udp_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_udp_gen_bin_cb,
    .release_tmpl_cb     = tad_udp_release_pdu_cb,

    .confirm_ptrn_cb     = tad_udp_confirm_ptrn_cb,
    .match_pre_cb        = tad_udp_match_pre_cb,
    .match_do_cb         = tad_udp_match_do_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = tad_udp_match_post_cb,
    .match_free_cb       = tad_udp_release_pdu_cb,
    .release_ptrn_cb     = tad_udp_release_pdu_cb,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};

/**
 * TCP
 */
static csap_spt_type_t tcp_csap_spt = 
{
    .proto               = "tcp",
    .unregister_cb       = NULL,

    .init_cb             = tad_tcp_init_cb,
    .destroy_cb          = tad_tcp_destroy_cb,
    .get_param_cb        = tad_tcp_get_param_cb,

    .confirm_tmpl_cb     = tad_tcp_confirm_tmpl_cb,
    .generate_pkts_cb    = tad_tcp_gen_bin_cb,
    .release_tmpl_cb     = NULL,

    .confirm_ptrn_cb     = tad_tcp_confirm_ptrn_cb,
    .match_pre_cb        = NULL,
    .match_do_cb         = tad_tcp_match_bin_cb,
    .match_done_cb       = NULL,
    .match_post_cb       = NULL,
    .match_free_cb       = NULL,
    .release_ptrn_cb     = NULL,

    .generate_pattern_cb = NULL,

    CSAP_SUPPORT_NO_RW,
};


/**
 * Register IP stack CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Status code.
 */
te_errno
csap_support_ipstack_register(void)
{
    te_errno rc;

    rc = csap_spt_add(&ip4_csap_spt);
    if (rc != 0)
        return rc;

    rc = csap_spt_add(&icmp4_csap_spt);
    if (rc != 0)
        return rc;

    rc = csap_spt_add(&udp_csap_spt);
    if (rc != 0)
        return rc;

    rc = csap_spt_add(&ip6_csap_spt);
    if (rc != 0)
        return rc;

    rc = csap_spt_add(&icmp6_csap_spt);
    if (rc != 0)
        return rc;

    return csap_spt_add(&tcp_csap_spt);
}
