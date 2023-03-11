/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC client API for RTE mbuf CSAP layer
 *
 * RPC client API to be used for traffic matching and conversion
 * between ASN.1 and RTE mbuf(s) representation
 * (implementation)
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "conf_api.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte.h"
#include "tapi_rpc_rte_mbuf_ndn.h"
#include "tapi_mem.h"
#include "tapi_test.h"
#include "te_errno.h"
#include "log_bufs.h"
#include "rpcc_dpdk.h"
#include "tapi_test_log.h"
#include "tapi_rpc_rte_mbuf.h"
#include "ndn.h"

#include "tarpc.h"

int
rpc_rte_mk_mbuf_from_template(rcf_rpc_server   *rpcs,
                              const asn_value  *template,
                              rpc_rte_mempool_p mp,
                              rpc_rte_mbuf_p  **mbufs,
                              unsigned int     *count)
{
    tarpc_rte_mk_mbuf_from_template_in  in;
    tarpc_rte_mk_mbuf_from_template_out out;
    size_t                              template_len;
    te_log_buf                         *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    template_len = asn_count_txt_len(template, 0) + 1;
    in.template = tapi_calloc(1, template_len);

    if (asn_sprint_value(template, in.template, template_len, 0) <= 0)
        TEST_FAIL("Failed to prepare textual representation of ASN.1 template");

    in.mp = (tarpc_rte_mempool)mp;

    rcf_rpc_call(rpcs, "rte_mk_mbuf_from_template", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_NEG_ERRNO(rte_mk_mbuf_from_template, out.retval);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_mk_mbuf_from_template, "\n%s,\n" RPC_PTR_FMT,
                 "%s, " NEG_ERRNO_FMT, in.template, RPC_PTR_VAL(in.mp),
                 rpc_rte_mbufs2str(tlbp, out.mbufs.mbufs_val,
                                   out.mbufs.mbufs_len, rpcs),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    free(in.template);

    *mbufs = tapi_memdup(out.mbufs.mbufs_val,
                         out.mbufs.mbufs_len * sizeof(**mbufs));

    *count = out.mbufs.mbufs_len;

    RETVAL_ZERO_INT(rte_mk_mbuf_from_template, out.retval);
}

static int
tapi_rte_mbuf_match_pattern_call(rcf_rpc_server *rpcs,
                                 char *ptrn,
                                 rpc_rte_mbuf_p *mbufs,
                                 unsigned int n_mbufs,
                                 te_bool seq_match,
                                 te_bool need_pkts,
                                 asn_value ***packets,
                                 unsigned int *n_packets)
{
    tarpc_rte_mbuf_match_pattern_in in = {
        .pattern = ptrn,
        .mbufs.mbufs_len = n_mbufs,
        .mbufs.mbufs_val = mbufs,
        .return_matching_pkts = (tarpc_bool)need_pkts,
        .seq_match = (tarpc_bool)seq_match,
    };
    tarpc_rte_mbuf_match_pattern_out out = {};
    te_log_buf *tlbp;

    *packets = NULL;
    *n_packets = 0;

    rcf_rpc_call(rpcs, "rte_mbuf_match_pattern", &in, &out);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_mbuf_match_pattern,
                 "%s\n%s,\n%s", "%u, " NEG_ERRNO_FMT,
                 (in.seq_match == TRUE) ? "(sequence matching)" : "",
                 in.pattern,
                 rpc_rte_mbufs2str(tlbp, in.mbufs.mbufs_val,
                                   in.mbufs.mbufs_len, rpcs),
                 out.matched, NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    if (out.retval == 0)
        *n_packets = out.matched;

    if (out.retval == 0 && need_pkts)
    {
        asn_value **pkts = tapi_malloc(out.matched * sizeof(*pkts));
        unsigned int i;
        int n_parsed;
        te_errno rc;

        for (i = 0; i < out.matched; ++i)
        {
            rc = asn_parse_value_text(out.packets.packets_val[i].str,
                                      ndn_raw_packet, pkts + i, &n_parsed);
            if (rc != 0)
            {
                TEST_FAIL("Failed to parse textual representation of "
                          "matching packets; rc = %d", rc);
            }
        }

        *packets = pkts;
    }

    RETVAL_ZERO_INT(rte_mbuf_match_pattern, out.retval);
}

static int
tapi_rte_mbuf_match_pattern(rcf_rpc_server *rpcs,
                            const asn_value *pattern,
                            te_bool seq_match,
                            rpc_rte_mbuf_p *mbufs,
                            unsigned int count,
                            asn_value ***packets,
                            unsigned int *matched)
{
/*
 * The maximum number of mempool buffers processed at a time.
 */
#define MBUFS_PER_CALL 0x1000

    char *ptrn;
    unsigned int ptrn_len;
    te_bool need_pkts = packets != NULL;
    unsigned int mbufs_len = 0;
    asn_value **pktbuf = NULL;
    unsigned int pktbuf_size = 0;
    unsigned int pktbuf_len = 0;

    if (matched == NULL)
        TEST_FAIL("Location for the number of matching packets cannot be NULL");

    ptrn_len = asn_count_txt_len(pattern, 0) + 1;
    ptrn = tapi_malloc(ptrn_len);
    if (asn_sprint_value(pattern, ptrn, ptrn_len, 0) <= 0)
        TEST_FAIL("Failed to prepare textual representation of ASN.1 pattern");

    do {
        unsigned int mbufs_num = MIN(count - mbufs_len, MBUFS_PER_CALL);
        asn_value **pkts;
        unsigned int pkts_len;

        tapi_rte_mbuf_match_pattern_call(rpcs, ptrn,
                                         mbufs + mbufs_len, mbufs_num,
                                         seq_match, need_pkts,
                                         &pkts, &pkts_len);

        if (need_pkts)
        {
            if (pktbuf_len + pkts_len > pktbuf_size)
            {
                pktbuf_size += MAX(count, pkts_len);
                pktbuf = tapi_realloc(pktbuf, pktbuf_size * sizeof(*pktbuf));
            }

            memcpy(pktbuf + pktbuf_len, pkts, pkts_len * sizeof(*pkts));
            free(pkts);
        }

        pktbuf_len += pkts_len;
        mbufs_len += mbufs_num;
    } while (mbufs_len < count);

    *matched = pktbuf_len;
    if (need_pkts)
        *packets = pktbuf;

    free(ptrn);

    return 0;

#undef MBUFS_PER_CALL
}

int
rpc_rte_mbuf_match_pattern(rcf_rpc_server     *rpcs,
                           const asn_value    *pattern,
                           rpc_rte_mbuf_p     *mbufs,
                           unsigned int        count,
                           asn_value        ***packets,
                           unsigned int       *matched)
{
    return tapi_rte_mbuf_match_pattern(rpcs, pattern, FALSE, mbufs,
                                       count, packets, matched);
}

int
tapi_rte_mbuf_match_pattern_seq(rcf_rpc_server    *rpcs,
                                const asn_value   *pattern,
                                rpc_rte_mbuf_p    *mbufs,
                                unsigned int       count,
                                asn_value       ***packets,
                                unsigned int      *matched)
{
    return tapi_rte_mbuf_match_pattern(rpcs, pattern, TRUE, mbufs,
                                       count, packets, matched);
}

int
rpc_rte_mbuf_match_tx_rx_pre(rcf_rpc_server *rpcs,
                             rpc_rte_mbuf_p  m)
{
    tarpc_rte_mbuf_match_tx_rx_pre_in  in;
    tarpc_rte_mbuf_match_tx_rx_pre_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m = (tarpc_rte_mbuf)m;

    rcf_rpc_call(rpcs, "rte_mbuf_match_tx_rx_pre", &in, &out);

    TAPI_RPC_LOG(rpcs, rte_mbuf_match_tx_rx_pre, RPC_PTR_FMT, NEG_ERRNO_FMT,
                 RPC_PTR_VAL(in.m), NEG_ERRNO_ARGS(out.retval));

    RETVAL_ZERO_INT(rte_mbuf_match_tx_rx_pre, out.retval);
}

static const char *
tarpc_rte_mbuf_ol_status2str(enum tarpc_rte_mbuf_ol_status ol_status)
{
    switch (ol_status)
    {
        case TARPC_RTE_MBUF_OL_NA:
            return "NA";
            break;
        case TARPC_RTE_MBUF_OL_DONE:
            return "DONE";
            break;
        case TARPC_RTE_MBUF_OL_NOT_DONE:
            return "NOT_DONE";
            break;
        default:
            return "__CORRUPTED";
            break;
    }
}

static const char *
rpc_rte_mbuf_match_tx_rx_match_status_str(
        enum tarpc_rte_mbuf_match_tx_rx_status match_status)
{
#define STATUS_STR(status) \
    case TARPC_RTE_MBUF_MATCH_TX_RX_##status: \
        return #status;

    switch (match_status)
    {
        STATUS_STR(MATCHED)
        STATUS_STR(VLAN_MISMATCH)
        STATUS_STR(UNEXPECTED_PACKET)
        STATUS_STR(LESS_DATA)
        STATUS_STR(INCONISTENT_TSO_OFFSET)
        STATUS_STR(PAYLOAD_MISMATCH)
        STATUS_STR(HEADER_MISMATCH)
        default:
            return "NA";
    }

#undef STATUS_STR
}

static void
rpc_rte_mbuf_match_tx_rx_mismatch_verdict(
        const tarpc_rte_mbuf_match_tx_rx_out *out)
{
    switch (out->match_status)
    {
        case TARPC_RTE_MBUF_MATCH_TX_RX_MATCHED:
            break;
        case TARPC_RTE_MBUF_MATCH_TX_RX_VLAN_MISMATCH:
            ERROR_VERDICT("Packet #%u has mismatched VLAN ID",
                         out->match_idx);
            break;
        case TARPC_RTE_MBUF_MATCH_TX_RX_UNEXPECTED_PACKET:
            ERROR_VERDICT("Packet #%u is not expected",
                         out->match_idx);
            break;
        case TARPC_RTE_MBUF_MATCH_TX_RX_LESS_DATA:
            ERROR_VERDICT("Not enough data to match packet #%u",
                         out->match_idx);
            break;
        case TARPC_RTE_MBUF_MATCH_TX_RX_INCONISTENT_TSO_OFFSET:
            ERROR_VERDICT("Packet #%u has inconsistent TSO cutoff offset",
                         out->match_idx);
            break;
        case TARPC_RTE_MBUF_MATCH_TX_RX_PAYLOAD_MISMATCH:
            ERROR_VERDICT("Packet #%u has mismatched payload",
                         out->match_idx);
            break;
        case TARPC_RTE_MBUF_MATCH_TX_RX_HEADER_MISMATCH:
            ERROR_VERDICT("Packet #%u has mismatched header",
                         out->match_idx);
            break;
        default:
            ERROR_VERDICT("Failed to match packets for an unexpected reason");
            break;
    }
}

int
rpc_rte_mbuf_match_tx_rx(rcf_rpc_server               *rpcs,
                         rpc_rte_mbuf_p                m_tx,
                         rpc_rte_mbuf_p               *rx_burst,
                         unsigned int                  nb_rx,
                         struct tarpc_rte_mbuf_report *reportp)
{
    tarpc_rte_mbuf_match_tx_rx_in   in;
    tarpc_rte_mbuf_match_tx_rx_out  out;
    te_log_buf                     *tlbp;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.m_tx = (tarpc_rte_mbuf)m_tx;
    in.rx_burst.rx_burst_val = tapi_memdup(rx_burst, nb_rx * sizeof(*rx_burst));
    in.rx_burst.rx_burst_len = nb_rx;

    CHECK_RC(cfg_get_instance_string_fmt(&in.tso_ip_id_inc_algo,
                                         "/local:/dpdk:/tso_ip_id_inc_algo:"));

    rcf_rpc_call(rpcs, "rte_mbuf_match_tx_rx", &in, &out);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_mbuf_match_tx_rx,
                 "m_tx = " RPC_PTR_FMT "; rx_burst = %s; "
                 "tso_ip_id_inc_algo = %s", "offloads = { "
                 "vlan = %s; outer_ip_cksum = %s; outer_udp_cksum = %s; "
                 "innermost_ip_cksum = %s; innermost_l4_cksum = %s }; "
                 "tso_cutoff_barrier = %u; "
                 "match_status = %s; match_idx = %u; "
                 NEG_ERRNO_FMT, RPC_PTR_VAL(in.m_tx),
                 rpc_rte_mbufs2str(tlbp, in.rx_burst.rx_burst_val,
                                   in.rx_burst.rx_burst_len, rpcs),
                 (in.tso_ip_id_inc_algo[0] != '\0') ?
                     in.tso_ip_id_inc_algo : "default",
                 tarpc_rte_mbuf_ol_status2str(out.report.ol_vlan),
                 tarpc_rte_mbuf_ol_status2str(out.report.ol_outer_ip_cksum),
                 tarpc_rte_mbuf_ol_status2str(out.report.ol_outer_udp_cksum),
                 tarpc_rte_mbuf_ol_status2str(out.report.ol_innermost_ip_cksum),
                 tarpc_rte_mbuf_ol_status2str(out.report.ol_innermost_l4_cksum),
                 out.report.tso_cutoff_barrier,
                 rpc_rte_mbuf_match_tx_rx_match_status_str(out.match_status),
                 out.match_idx, NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    rpc_rte_mbuf_match_tx_rx_mismatch_verdict(&out);

    free(in.rx_burst.rx_burst_val);
    free(in.tso_ip_id_inc_algo);

    if (reportp != NULL)
        memcpy(reportp, &out.report, sizeof(*reportp));

    RETVAL_ZERO_INT(rte_mbuf_match_tx_rx, out.retval);
}
