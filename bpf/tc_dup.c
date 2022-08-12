/** @file
 * @brief BPF program
 *
 * BPF program to duplicate packets.
 *
 * This program attaches to the interface and duplacates packets.
 * The number of packets is specified in control map.
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 */

#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include "bpf_stim_helpers.h"

/**
 * RX count map.
 * The map contents:
 * 0 - couner of RX packets.
 */
struct bpf_map SEC("maps") rxcnt = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = 1,
};

/**
 * Control map.
 * The map contents:
 * 0 - number of copies to made.
 * 1 - interface index to redirect packet to.
 * 2 - whether to use BPF_F_INGRESS flag.
 */
struct bpf_map SEC("maps") ctrl = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = 3,
};

/**
 * Increase the counter of RX packets.
 */
static inline void
count_pkt(void)
{
    __u32  key = 0;
    __u32 *count;

    count = bpf_map_lookup_elem(&rxcnt, &key);
    if (count)
        *count += 1;
}

/**
 * The entry point to the `duplicate` bpf program.
 */
SEC("classifier")
int
tc_dup(struct __sk_buff *skb)
{
    __u32   key = 0;
    __u32  *copies;
    __u32  *ifindex = 0;

    copies = bpf_map_lookup_elem(&ctrl, &key);
    if (copies && *copies > 0)
    {
        __u32 *ingress;
        __u64 flags = 0;

        key = 1;
        ifindex = bpf_map_lookup_elem(&ctrl, &key);

        key = 2;
        ingress = bpf_map_lookup_elem(&ctrl, &key);

        if (!ifindex || *ifindex == 0)
            return TC_ACT_OK;

        if (ingress && *ingress != 0)
            flags = BPF_F_INGRESS;

        count_pkt();
        *copies -= 1;
        bpf_clone_redirect(skb, *ifindex, flags);
    }

    return TC_ACT_OK;
}
