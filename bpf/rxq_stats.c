/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief BPF program to get RX queues statistics
 *
 * BPF program to count packets received via each RX queue.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#include <linux/bpf.h>

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

#include "te_bpf_helpers.h"

#define MAX_QUEUES 128

/* Map to store per-queue numbers of received packets */
TE_BPF_MAP_DEF(queue_stats, BPF_MAP_TYPE_HASH, MAX_QUEUES,
               __u32, __u64);

/* Map to store parameters of packets which should be counted */
TE_BPF_MAP_DEF(params, BPF_MAP_TYPE_HASH, 1,
               __u32, te_bpf_rxq_stats_params);

SEC("prog")
int
rxq_stats(struct xdp_md *ctx)
{
    te_xdp_frame frame = TE_XDP_FRAME_INIT(ctx);

    __u32 key;
    __u64 *count;
    __u64 new_count;

    te_bpf_rxq_stats_params *prms = NULL;

    key = 0;
    prms = bpf_map_lookup_elem(&params, &key);
    if (prms == NULL || prms->enabled == 0)
        return XDP_PASS;

    if (ctx->rx_queue_index < MAX_QUEUES)
    {
        if (te_xdp_parse_eth_frame(&frame) < 0)
            return XDP_PASS;

        if (!te_xdp_frame_match_ip_tcpudp(&frame, &prms->filter))
            return XDP_PASS;

        key = ctx->rx_queue_index;

        count = bpf_map_lookup_elem(&queue_stats, &key);
        if (count != NULL)
            new_count = *count + 1;
        else
            new_count = 1;

        bpf_map_update_elem(&queue_stats, &key, &new_count, BPF_ANY);
    }

    return XDP_PASS;
}
