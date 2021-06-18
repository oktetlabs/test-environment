/** @file
 * @brief BPF program
 *
 * BPF program to delay packets.
 *
 * This program attaches to the interface and delays packets.
 * How much packets to wait before sending delayed packet is
 * specified in control map.
 *
 * Copyright (C) 2021 OKTET Labs. All rights reserved.
 *
 * @author Sergey Nikitin <Sergey.Nikitin@oktetlabs.ru>
 */

dnl Declare frame size constants needed for delaying a frame.
dnl $1 - The frame size to delay.
dnl $2 - The chunk size.
define(`te_frame_size_declare',
`
`#'define FRAME_SAVE_CHUNK_SIZE         $2
`#'define FRAME_SAVE_SIZE               $1
`#'define FRAME_SAVE_CHUNKS_NUM         eval($1 / $2)
`#'define FRAME_SAVE_LAST_CHUNK_SIZE    eval($1 % $2)
'
)

#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include "bpf_stim_helpers.h"

te_frame_size_declare(frame_size, chunk_size)

/**
 * Key to access 'ctrl' map field to read an interface index to which
 * delayed packet is sent.
 */
#define CT_BPF_DELAY_IFINDEX_KEY 0

/** Key to access 'ctrl' map field to read number of frames to delay. */
#define CT_BPF_DELAY_NUMPKT_KEY  1

/**
 * Key to access boolean map field containing flag whether to
 * use @c BPF_F_INGRESS flag.
 */
#define CT_BPF_DELAY_INGRESS_KEY 3

/**
 * Check return value of a @p call and return if is not equal to zero.
 */
#define CHECK_RC(call) \
    do {                    \
        if ((call) != 0)    \
            return -1;      \
    } while (0)

/**
 * Map for a flag, signaling about whether
 * a frame is saved now or not.
 */
struct bpf_map SEC("maps") m_flag = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = 1,
};

/**
 * Map for a flag, signaling about whether bpf_clone_redirect() was
 * called. It is used in egress interface case.
 */
struct bpf_map SEC("maps") m_cloned = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = 1,
};

/**
 * Map for storing a packet. Each entry is a packet chunk
 * with size @ref FRAME_SAVE_CHUNK_SIZE.
 */
struct bpf_map SEC("maps") pktbuf = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = FRAME_SAVE_CHUNK_SIZE,
    .max_entries = FRAME_SAVE_CHUNKS_NUM + 1,
};

/**
 * Control map.
 * Contents:
 * 0 - interface index
 * 1 - number of frames to delay
 * 2 - unused
 * 3 - whether to use BPF_F_INGRESS flag.
 */
struct bpf_map SEC("maps") ctrl = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = 4,
};

/**
 * Load a frame chunk from BPF context to @ref pktbuf map.
 *
 * @param skb   BPF skb context.
 * @param key   Key value of the @ref pktbuf map.
 * @param len   Length of data to load.
 *
 * @return status code, 0 - success, -1 - error.
 */
static inline int
tc_delay_load_chunk(struct __sk_buff *skb, __u32 key, __u32 len)
{
    void *buf;
    __u32 offset = key * FRAME_SAVE_CHUNK_SIZE;

    buf = bpf_map_lookup_elem(&pktbuf, &key);
    if (buf == NULL)
        return -1;

    if (bpf_skb_load_bytes(skb, offset, buf, len) != 0)
        return -1;

    return 0;
}

/**
 * Store a frame chunk to BPF context from @ref pktbuf map.
 *
 * @param skb   BPF skb context.
 * @param key   Key value of the @ref pktbuf map.
 * @param len   Length of data to store.
 *
 * @return status code, 0 - success, -1 - error.
 */
static inline int
tc_delay_store_chunk(struct __sk_buff *skb, __u32 key, __u32 len)
{
    void *buf;
    __u32 offset = key * FRAME_SAVE_CHUNK_SIZE;
    __u64 flags = BPF_F_RECOMPUTE_CSUM | BPF_F_INVALIDATE_HASH;

    buf = bpf_map_lookup_elem(&pktbuf, &key);
    if (buf == NULL)
        return -1;

    if (bpf_skb_store_bytes(skb, offset, buf, len, flags) != 0)
        return -1;

    return 0;
}

/**
 * Load a frame with size according to @p size from @p skb context
 * to @ref pktbuf map.
 *
 * @param skb   BPF context.
 *
 * @return Status code - 0 on success, or a negative value in case of failure.
 */
static inline int
load_frame(struct __sk_buff *skb)
{
    int i;

    for (i = 0; i < FRAME_SAVE_CHUNKS_NUM; ++i)
        CHECK_RC(tc_delay_load_chunk(skb, i, FRAME_SAVE_CHUNK_SIZE));

    CHECK_RC(tc_delay_load_chunk(skb, i, FRAME_SAVE_LAST_CHUNK_SIZE));

    return 0;
}

/**
 * Store a frame with size according to @p size to @p skb context
 * from @ref pktbuf map.
 *
 * @param skb   BPF context.
 *
 * @return Status code - 0 on success, or a negative value in case of failure.
 */
static inline int
store_frame(struct __sk_buff *skb)
{
    int i;

    for (i = 0; i < FRAME_SAVE_CHUNKS_NUM; ++i)
        CHECK_RC(tc_delay_store_chunk(skb, i, FRAME_SAVE_CHUNK_SIZE));

    CHECK_RC(tc_delay_store_chunk(skb, i, FRAME_SAVE_LAST_CHUNK_SIZE));

    return 0;
}

/**
 * The entry point to the `delay` bpf program.
 */
SEC("classifier")
int
tc_delay(struct __sk_buff *skb)
{
    __u32   key = 0;
    __u32  *ifindex = NULL;
    __u32  *delay = NULL;
    __u32  *flag = NULL;
    __u32  *cloned = NULL;

    cloned = bpf_map_lookup_elem(&m_cloned, &key);
    flag = bpf_map_lookup_elem(&m_flag, &key);

    /*
     * If the condition is true, we caught just cloned frame while the
     * bpf_clone_redirect() is still executing.
     * In such case we should pass it and exit the program immediately.
     */
    if (cloned != NULL && *cloned != 0)
    {
        printk("Caught cloned frame. Exiting.\n");
        return TC_ACT_OK;
    }

    key = CT_BPF_DELAY_NUMPKT_KEY;
    delay = bpf_map_lookup_elem(&ctrl, &key);

    if (flag == NULL || delay == NULL)
        return TC_ACT_OK;

    /* The case when the BPF program has nothing to do. */
    if (*delay == 0 && *flag == 0)
    {
        printk("do nothing\n");
        return TC_ACT_OK;
    }

    key = CT_BPF_DELAY_IFINDEX_KEY;
    ifindex = bpf_map_lookup_elem(&ctrl, &key);

    if (ifindex == NULL)
        return TC_ACT_OK;

    if (*delay != 0 && *flag == 0)
    {
        /*
         * A user has updated the delay counter. Save current packet,
         * do not send it.
         */

        if (skb->len != FRAME_SAVE_SIZE)
        {
            printk("Ignoring frame with size %u\n", skb->len);
            return TC_ACT_OK;
        }

        printk("save packet\n");

        if (load_frame(skb) == 0)
        {
            *flag = 1;
            *delay -= 1;
            return TC_ACT_SHOT;
        }
    }
    else if (*delay != 0 && *flag != 0)
    {
        /*
         * We have saved packet, but it is not time to send it. Just update
         * the delay counter.
         */
        printk("wait, delay--\n");
        *delay -= 1;
    }
    else if (*delay == 0 && *flag != 0)
    {
        __u32 *ingress;
        __u64 flags = 0;

        /* We have saved packet, and it is time to send it. */
        printk("send delayed packet\n");

        key = CT_BPF_DELAY_INGRESS_KEY;
        ingress = bpf_map_lookup_elem(&ctrl, &key);

        if (ingress && *ingress != 0)
            flags = BPF_F_INGRESS;

        /*
         * Send the current frame without changes. This frame will be sent
         * before the restored one.
         * Set 'cloned' flag to determine the case when the BPF program is
         * called at bpf_clone_redirect() execution time (regular case for
         * egress interface).
         */
        if (cloned != NULL)
            *cloned = 1;
        bpf_clone_redirect(skb, *ifindex, flags);
        if (cloned != NULL)
            *cloned = 0;

        /* Update bpf context with saved frame. */
        bpf_skb_change_tail(skb, FRAME_SAVE_SIZE, 0);
        if (store_frame(skb) == 0)
        {
            /* Send the delayed frame. */
            *flag = 0;
            return bpf_redirect(*ifindex, flags);
        }
        else
        {
            return TC_ACT_SHOT;
        }
    }

    return TC_ACT_OK;
}

#if TC_DEBUG
char _license[] SEC("license") = "GPL";
#endif /* TC_DEBUG */
