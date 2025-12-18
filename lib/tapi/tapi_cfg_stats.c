/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to access network statistics via configurator.
 *
 * Implementation of API to configure network statistics.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI CFG stats"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "te_string.h"
#include "logger_api.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_stats.h"


#ifdef U64_FMT
#undef U64_FMT
#endif

#ifdef _U64_FMT
#undef _U64_FMT
#endif

#ifdef I64_FMT
#undef I64_FMT
#endif

#ifdef _I64_FMT
#undef _I64_FMT
#endif

#define U64_FMT "%" TE_PRINTF_64 "u"
#define I64_FMT "%" TE_PRINTF_64 "u"

#define _U64_FMT " " U64_FMT
#define _I64_FMT " " I64_FMT


/* See description in tapi_cfg_stats.h */
te_errno
tapi_cfg_stats_if_stats_get(const char          *ta,
                            const char          *ifname,
                            tapi_cfg_if_stats   *stats)
{
    te_errno            rc;

    VERB("%s(ta=%s, ifname=%s, stats=%x) started",
         __FUNCTION__, ta, ifname, stats);

    if (stats == NULL)
    {
        ERROR("%s(): stats=NULL!", __FUNCTION__);
        return TE_OS_RC(TE_TAPI, EINVAL);
    }

    memset(stats, 0, sizeof(tapi_cfg_if_stats));

    VERB("%s(): stats=%x", __FUNCTION__, stats);

    /*
     * Synchronize configuration trees and get assigned interfaces
     */

    VERB("Try to sync stats");

    rc = cfg_synchronize_fmt(true, "/agent:%s/interface:%s/stats:",
                             ta, ifname);
    if (rc != 0)
        return rc;

    VERB("Get stats counters");

#define TAPI_CFG_STATS_IF_COUNTER_GET(_counter_) \
    do                                                      \
    {                                                       \
        VERB("IF_COUNTER_GET(%s)", #_counter_);             \
        rc = cfg_get_instance_uint64_fmt(&stats->_counter_, \
                 "/agent:%s/interface:%s/stats:/%s:",       \
                 ta, ifname, #_counter_);                   \
        if (rc != 0)                                        \
        {                                                   \
            ERROR("Failed to get %s counter for "           \
                  "interface %s on %s Test Agent: %r",      \
                  #_counter_, ifname, ta, rc);              \
            return rc;                                      \
        }                                                   \
    } while (0)

    TAPI_CFG_STATS_IF_COUNTER_GET(in_octets);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_ucast_pkts);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_nucast_pkts);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_discards);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_errors);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_unknown_protos);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_octets);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_ucast_pkts);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_nucast_pkts);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_discards);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_errors);

#undef TAPI_CFG_STATS_IF_COUNTER_GET

    return 0;
}


/* See description in tapi_cfg_stats.h */
te_errno
tapi_cfg_stats_net_stats_get(const char          *ta,
                             tapi_cfg_net_stats  *stats)
{
    te_errno            rc;

    VERB("%s(ta=%s) started", __FUNCTION__, ta);

    if (stats == NULL)
    {
        ERROR("%s(): stats=NULL!", __FUNCTION__);
        return TE_OS_RC(TE_TAPI, EINVAL);
    }

    memset(stats, 0, sizeof(tapi_cfg_net_stats));

    /*
     * Synchronize configuration trees and get assigned interfaces
     */

    VERB("Try to sync stats");

    rc = cfg_synchronize_fmt(true, "/agent:%s/stats:", ta);
    if (rc != 0)
        return rc;

    VERB("Get stats counters");

#define TAPI_CFG_STATS_IPV4_COUNTER_GET(_counter_) \
    do                                                          \
    {                                                           \
        VERB("IPV4_COUNTER_GET(%s)", #_counter_);               \
        rc = cfg_get_instance_uint64_fmt(                       \
                                  &stats->ipv4._counter_,       \
                                  "/agent:%s/stats:/ipv4_%s:",  \
                                  ta, #_counter_);              \
        if (rc != 0)                                            \
        {                                                       \
            ERROR("Failed to get ipv4_%s counter from %s "      \
                  "Test Agent: %r", #_counter_, ta, rc);        \
            return rc;                                          \
        }                                                       \
    } while (0)

    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_recvs);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_hdr_errs);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_addr_errs);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(forw_dgrams);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_unknown_protos);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_discards);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_delivers);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(out_requests);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(out_discards);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(out_no_routes);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(reasm_timeout);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(reasm_reqds);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(reasm_oks);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(reasm_fails);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(frag_oks);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(frag_fails);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(frag_creates);

#undef TAPI_CFG_STATS_IPV4_COUNTER_GET

#define TAPI_CFG_STATS_ICMP_COUNTER_GET(_counter_) \
    do                                                          \
    {                                                           \
        VERB("ICMP_COUNTER_GET(%s)", #_counter_);               \
        rc = cfg_get_instance_uint64_fmt(                       \
                                  &stats->icmp._counter_,       \
                                  "/agent:%s/stats:/icmp_%s:",  \
                                  ta, #_counter_);              \
        if (rc != 0)                                            \
        {                                                       \
            ERROR("Failed to get icmp_%s counter from %s "      \
                  "Test Agent: %r", #_counter_, ta, rc);        \
            return rc;                                          \
        }                                                       \
    } while (0)

    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_msgs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_errs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_dest_unreachs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_time_excds);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_parm_probs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_src_quenchs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_redirects);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_echos);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_echo_reps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_timestamps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_timestamp_reps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_addr_masks);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_addr_mask_reps);

    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_msgs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_errs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_dest_unreachs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_time_excds);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_parm_probs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_src_quenchs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_redirects);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_echos);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_echo_reps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_timestamps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_timestamp_reps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_addr_masks);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_addr_mask_reps);

#undef TAPI_CFG_STATS_ICMP_COUNTER_GET

    return 0;
}

static void
tapi_cfg_stats_if_stats_print_with_descr_va(const tapi_cfg_if_stats *stats,
                                            bool print_zeros,
                                            const char *descr_fmt, va_list ap)
{
    te_string buf = TE_STRING_INIT;

    te_string_append_va(&buf, descr_fmt, ap);

#define PRINT_STAT(_name) \
    do {                                                    \
        if (print_zeros || stats->_name != 0)                \
            te_string_append(&buf, "\n  %s : " U64_FMT,     \
                             #_name, stats->_name);         \
    } while (0)

    PRINT_STAT(in_octets);
    PRINT_STAT(in_ucast_pkts);
    PRINT_STAT(in_nucast_pkts);
    PRINT_STAT(in_discards);
    PRINT_STAT(in_errors);
    PRINT_STAT(in_unknown_protos);
    PRINT_STAT(out_octets);
    PRINT_STAT(out_ucast_pkts);
    PRINT_STAT(out_nucast_pkts);
    PRINT_STAT(out_discards);
    PRINT_STAT(out_errors);

#undef PRINT_STAT

    RING("%s", te_string_value(&buf));

    te_string_free(&buf);
}

static void
tapi_cfg_stats_if_stats_print_with_descr(const tapi_cfg_if_stats *stats,
                                          const char *descr_fmt, ...)
{
    va_list ap;

    va_start(ap, descr_fmt);
    tapi_cfg_stats_if_stats_print_with_descr_va(stats, true, descr_fmt, ap);
    va_end(ap);
}

/* See description in tapi_cfg_stats.h */
te_errno
tapi_cfg_stats_if_stats_print(const char          *ta,
                              const char          *ifname,
                              tapi_cfg_if_stats   *stats)
{
    if (stats == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    tapi_cfg_stats_if_stats_print_with_descr(stats,
        "Network statistics for interface %s on Test Agent %s:", ifname, ta);

    return 0;
}

static void
tapi_cfg_if_stats_diff(tapi_cfg_if_stats *diff,
                       const tapi_cfg_if_stats *stats,
                       const tapi_cfg_if_stats *prev)
{
    /* It is a bit dirty, but I have no time to restructure stats to be array */
    const size_t n_stats = sizeof(*diff) / sizeof(uint64_t);
    const uint64_t *stats_u64 = (const uint64_t *)stats;
    const uint64_t *prev_u64 = (const uint64_t *)prev;
    uint64_t *diff_u64 = (uint64_t *)diff;
    size_t i;

    for (i = 0; i < n_stats; ++i)
        diff_u64[i] = stats_u64[i] - prev_u64[i];
}

te_errno
tapi_cfg_stats_if_stats_print_diff(const tapi_cfg_if_stats *stats,
                                   const tapi_cfg_if_stats *prev,
                                   const char *descr_fmt, ...)
{
    const tapi_cfg_if_stats *diff = stats;
    tapi_cfg_if_stats diff_buf;
    va_list ap;

    if (stats == NULL || descr_fmt == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if (prev != NULL)
    {
        tapi_cfg_if_stats_diff(&diff_buf, stats, prev);
        diff = &diff_buf;
    }

    va_start(ap, descr_fmt);
    tapi_cfg_stats_if_stats_print_with_descr_va(diff, false, descr_fmt, ap);
    va_end(ap);

    return 0;
}

static void
tapi_cfg_stats_net_stats_print_with_descr_va(const tapi_cfg_net_stats *stats,
                                             bool print_zeros,
                                             const char *descr_fmt, va_list ap)
{
    te_string buf = TE_STRING_INIT;

    te_string_append_va(&buf, descr_fmt, ap);

    te_string_append(&buf, "\nIPv4:");

#define PRINT_STAT(_name) \
    do {                                                    \
        if (print_zeros || stats->ipv4._name != 0)           \
            te_string_append(&buf, "\n  %s : " U64_FMT,     \
                             #_name, stats->ipv4._name);    \
    } while (0)

    PRINT_STAT(in_recvs);
    PRINT_STAT(in_hdr_errs);
    PRINT_STAT(in_addr_errs);
    PRINT_STAT(forw_dgrams);
    PRINT_STAT(in_unknown_protos);
    PRINT_STAT(in_discards);
    PRINT_STAT(in_delivers);
    PRINT_STAT(out_requests);
    PRINT_STAT(out_discards);
    PRINT_STAT(out_no_routes);
    PRINT_STAT(reasm_timeout);
    PRINT_STAT(reasm_reqds);
    PRINT_STAT(reasm_oks);
    PRINT_STAT(reasm_fails);
    PRINT_STAT(frag_oks);
    PRINT_STAT(frag_fails);
    PRINT_STAT(frag_creates);

#undef PRINT_STAT

    te_string_append(&buf, "\nICMP:");

#define PRINT_STAT(_name) \
    do {                                                    \
        if (print_zeros || stats->icmp._name != 0)           \
            te_string_append(&buf, "\n  %s : " U64_FMT,     \
                             #_name, stats->icmp._name);    \
    } while (0)

    PRINT_STAT(in_msgs);
    PRINT_STAT(in_errs);
    PRINT_STAT(in_dest_unreachs);
    PRINT_STAT(in_time_excds);
    PRINT_STAT(in_parm_probs);
    PRINT_STAT(in_src_quenchs);
    PRINT_STAT(in_redirects);
    PRINT_STAT(in_echos);
    PRINT_STAT(in_echo_reps);
    PRINT_STAT(in_timestamps);
    PRINT_STAT(in_timestamp_reps);
    PRINT_STAT(in_addr_masks);
    PRINT_STAT(in_addr_mask_reps);
    PRINT_STAT(out_msgs);
    PRINT_STAT(out_errs);
    PRINT_STAT(out_dest_unreachs);
    PRINT_STAT(out_time_excds);
    PRINT_STAT(out_parm_probs);
    PRINT_STAT(out_src_quenchs);
    PRINT_STAT(out_redirects);
    PRINT_STAT(out_echos);
    PRINT_STAT(out_echo_reps);
    PRINT_STAT(out_timestamps);
    PRINT_STAT(out_timestamp_reps);
    PRINT_STAT(out_addr_masks);
    PRINT_STAT(out_addr_mask_reps);

#undef PRINT_STAT

    RING("%s", te_string_value(&buf));

    te_string_free(&buf);
}

static void
tapi_cfg_stats_net_stats_print_with_descr(const tapi_cfg_net_stats *stats,
                                          const char *descr_fmt, ...)
{
    va_list ap;

    va_start(ap, descr_fmt);
    tapi_cfg_stats_net_stats_print_with_descr_va(stats, true, descr_fmt, ap);
    va_end(ap);
}

/* See description in tapi_cfg_stats.h */
te_errno
tapi_cfg_stats_net_stats_print(const char *ta, tapi_cfg_net_stats *stats)
{
    if (stats == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    tapi_cfg_stats_net_stats_print_with_descr(stats,
        "Network statistics for Test Agent %s:", ta);

    return 0;
}

static void
tapi_cfg_net_stats_diff(tapi_cfg_net_stats *diff,
                        const tapi_cfg_net_stats *stats,
                        const tapi_cfg_net_stats *prev)
{
    /* It is a bit dirty, but I have no time to restructure stats to be array */
    const size_t n_stats = sizeof(*diff) / sizeof(uint64_t);
    const uint64_t *stats_u64 = (const uint64_t *)stats;
    const uint64_t *prev_u64 = (const uint64_t *)prev;
    uint64_t *diff_u64 = (uint64_t *)diff;
    size_t i;

    for (i = 0; i < n_stats; ++i)
        diff_u64[i] = stats_u64[i] - prev_u64[i];
}

te_errno
tapi_cfg_stats_net_stats_print_diff(const tapi_cfg_net_stats *stats,
                                    const tapi_cfg_net_stats *prev,
                                    const char *descr_fmt, ...)
{
    const tapi_cfg_net_stats *diff = stats;
    tapi_cfg_net_stats diff_buf;
    va_list ap;

    if (stats == NULL || descr_fmt == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if (prev != NULL)
    {
        tapi_cfg_net_stats_diff(&diff_buf, stats, prev);
        diff = &diff_buf;
    }

    va_start(ap, descr_fmt);
    tapi_cfg_stats_net_stats_print_with_descr_va(diff, false, descr_fmt, ap);
    va_end(ap);

    return 0;
}
