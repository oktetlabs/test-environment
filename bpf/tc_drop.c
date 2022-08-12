/** @file
 * @brief BPF program
 *
 * BPF program to drop packets.
 *
 * This program attaches to the interface and drops packets.
 * The number of packets is specified in control map.
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 * @author Sergey Nikitin <Sergey.Nikitin@oktetlabs.ru>
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
 * 0 - Number of packets to drop.
 */
struct bpf_map SEC("maps") ctrl = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = 1,
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
 * The entry point to the  `drop` bpf program.
 */
SEC("classifier")
int
tc_drop(struct __sk_buff *skb)
{
    __u32   key = 0;
    __u32  *value;

    value = bpf_map_lookup_elem(&ctrl, &key);
    if (value && *value > 0)
    {
        *value -= 1;
        count_pkt();
        return TC_ACT_SHOT;
    }

    return TC_ACT_OK;
}
