/** @file
 * @brief RPC client API for RTE mbuf CSAP layer
 *
 * RPC client API to be used for traffic matching and conversion
 * between ASN.1 and RTE mbuf(s) representation
 * (implementation)
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#include "te_config.h"

#include "tapi_rpc_internal.h"
#include "tapi_rpc_rte.h"
#include "tapi_rpc_rte_mbuf_ndn.h"
#include "tapi_mem.h"
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
    TAPI_RPC_LOG(rpcs, rte_mk_mbuf_from_template, "\n%s\n" RPC_PTR_FMT,
                 "%s, " NEG_ERRNO_FMT, in.template, RPC_PTR_VAL(in.mp),
                 rpc_rte_mbufs2str(tlbp, out.mbufs.mbufs_val,
                                   out.mbufs.mbufs_len, rpcs),
                 NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    *mbufs = tapi_memdup(out.mbufs.mbufs_val,
                         out.mbufs.mbufs_len * sizeof(**mbufs));

    *count = out.mbufs.mbufs_len;

    RETVAL_ZERO_INT(rte_mk_mbuf_from_template, out.retval);
}

int
rpc_rte_mbuf_match_pattern(rcf_rpc_server     *rpcs,
                           const asn_value    *pattern,
                           rpc_rte_mbuf_p     *mbufs,
                           unsigned int        count,
                           asn_value        ***packets,
                           unsigned int       *matched)
{
    tarpc_rte_mbuf_match_pattern_in     in;
    tarpc_rte_mbuf_match_pattern_out    out;
    size_t                              pattern_len;
    te_log_buf                         *tlbp;
    unsigned int                        i;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (matched == NULL)
        TEST_FAIL("Location for the number of matching packets cannot be NULL");

    pattern_len = asn_count_txt_len(pattern, 0) + 1;
    in.pattern = tapi_calloc(1, pattern_len);

    if (asn_sprint_value(pattern, in.pattern, pattern_len, 0) <= 0)
        TEST_FAIL("Failed to prepare textual representation of ASN.1 pattern");

    in.mbufs.mbufs_len = count;
    in.mbufs.mbufs_val = tapi_memdup(mbufs, count * sizeof(*mbufs));

    in.return_matching_pkts = (tarpc_bool)(packets != NULL);

    rcf_rpc_call(rpcs, "rte_mbuf_match_pattern", &in, &out);

    tlbp = te_log_buf_alloc();
    TAPI_RPC_LOG(rpcs, rte_mbuf_match_pattern,
                 "\n%s\n%s", "%u, " NEG_ERRNO_FMT, in.pattern,
                 rpc_rte_mbufs2str(tlbp, in.mbufs.mbufs_val,
                                   in.mbufs.mbufs_len, rpcs),
                 out.matched, NEG_ERRNO_ARGS(out.retval));
    te_log_buf_free(tlbp);

    if (out.retval == 0)
        *matched = out.matched;

    if (out.retval == 0 && packets != NULL)
    {
        asn_value **asn_pkts;
        int         num_symbols_parsed;
        te_errno    rc;

        asn_pkts = *packets = tapi_calloc(out.matched, sizeof(**packets));

        for (i = 0; i < out.matched; ++i)
        {
            rc = asn_parse_value_text(out.packets.packets_val[i].str,
                                      ndn_raw_packet, &asn_pkts[i],
                                      &num_symbols_parsed);

            if (rc != 0)
            {
                do {
                    asn_free_value(asn_pkts[i]);
                } while (i-- > 0);
                free(asn_pkts);
                *packets = NULL;
                TEST_FAIL("Failed to parse textual representation of "
                          "matching packets; rc = %d", rc);
            }
        }
    }

    RETVAL_ZERO_INT(rte_mbuf_match_pattern, out.retval);
}
