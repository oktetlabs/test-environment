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

#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include "bpf_stim_helpers.h"

/**
 * Supported frame sizes that the program can delay.
 * The same enum is defined in a test.
 */
typedef enum ct_stim_del_frame
{
    CT_DELAY_FRAME_SIZE_UNSPEC,  /**< Unspecified */
    CT_DELAY_FRAME_SIZE_1514,    /**< 1514 bytes - the size corresponds
                                      to MTU 1500. */
} ct_stim_del_frame;

/**
 * Size of a packet chunk stored in the map "pktbuf" per entry.
 */
#define FRAME_SAVE_CHUNK_SIZE 256

/**
 * List of supported frame sizes.
 *
 * Used definitions:
 * FRAME_SAVE_SIZE_<size> - full size of ethernet frame.
 * FRAME_SAVE_CHUNKS_NUM_<size> - number of chunks (@ref FRAME_SAVE_CHUNK_SIZE)
 * to store a frame with this size in a map @ref pktbuf.
 * FRAME_SAVE_LAST_CHUNK_SIZE_<size> - size of last chunk, calculated as:
 * frame_size - ((chunks_num - 1) * chunk_size)
 */

/** Definitions for 1514 bytes frames. */
#define FRAME_SAVE_SIZE_1514            1514
#define FRAME_SAVE_CHUNKS_NUM_1514      6
#define FRAME_SAVE_LAST_CHUNK_SIZE_1514 234

/**
 * Number of entries for map to be able to store
 * the packet with max size.
 */
#define FRAME_SAVE_CHUNKS_NUM_MAX   FRAME_SAVE_CHUNKS_NUM_1514

/**
 * Key to access 'ctrl' map field to read an interface index to which
 * delayed packet is sent.
 */
#define CT_BPF_DELAY_IFINDEX_KEY 0

/** Key to access 'ctrl' map field to read number of frames to delay. */
#define CT_BPF_DELAY_NUMPKT_KEY  1

/** Key to access 'ctrl' map field to read size of frame to save. */
#define CT_BPF_DELAY_SIZE_KEY    2

/**
 * Key to access boolean map field containing flag whether to
 * use @c BPF_F_INGRESS flag.
 */
#define CT_BPF_DELAY_INGRESS_KEY 3

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
    .max_entries = FRAME_SAVE_CHUNKS_NUM_MAX,
};

/**
 * Control map.
 * Contents:
 * 0 - interface index
 * 1 - number of frames to delay
 * 2 - size of a frame to delay (@ref ct_stim_del_frame)
 * 3 - whether to use BPF_F_INGRESS flag.
 */
struct bpf_map SEC("maps") ctrl = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u32),
    .max_entries = 4,
};

/**
 * Macro for loading a packet chunk from BPF context
 * to @ref pktbuf map.
 *
 * @note The @b bpf_skb_load_bytes helper needs its last parameter (@p chunk_size)
 *       to be known at compile time.
 *
 * @param key           Key value.
 * @param chunk_size    Size of chunk in bytes.
 */
#define LOAD_CHUNK(key, chunk_size) \
    do {                                                                    \
        void    *__buf;                                                     \
        __u32    __key = key;                                               \
                                                                            \
        __buf = bpf_map_lookup_elem(&pktbuf, &__key);                       \
        if (__buf == NULL)                                                  \
            return -1;                                                      \
                                                                            \
        if (bpf_skb_load_bytes(skb, __key * FRAME_SAVE_CHUNK_SIZE, __buf,   \
                               chunk_size) != 0)                            \
        {                                                                   \
            return -1;                                                      \
        }                                                                   \
    } while (0)

/**
 * Macro for storing a packet chunk to BPF context
 * from @ref pktbuf map.
 *
 * @note The @b bpf_skb_load_bytes helper needs its last parameter (@p chunk_size)
 *       to be known at compile time.
 *
 * @param key           Key value.
 * @param chunk_size    Size of chunk in bytes.
 */
#define STORE_CHUNK(key, chunk_size) \
    do {                                                                    \
        void    *__buf;                                                     \
        __u32    __key = key;                                               \
        __u64    __flags = BPF_F_RECOMPUTE_CSUM | BPF_F_INVALIDATE_HASH;    \
                                                                            \
        __buf = bpf_map_lookup_elem(&pktbuf, &__key);                       \
        if (__buf == NULL)                                                  \
            return -1;                                                      \
                                                                            \
        if (bpf_skb_store_bytes(skb, __key * FRAME_SAVE_CHUNK_SIZE, __buf,  \
                                chunk_size, __flags) != 0)                  \
        {                                                                   \
            return -1;                                                      \
        }                                                                   \
    } while (0)

/**
 * Macro defining a function for storing a frame of specified size.
 * The function prototype is store_frame_<size>(struct __sk_buff *skb)
 *
 * @note The @b bpf_skb_load_bytes helper needs its last parameter (@p chunk_size)
 *       to be known at compile time.
 *
 * @param size  The frame size.
 */
#define STORE_FUNC_IMPL(size) \
    static inline int                                           \
    store_frame_##size(struct __sk_buff *skb)                   \
    {                                                           \
        int i;                                                  \
                                                                \
        for (i = 0; i < FRAME_SAVE_CHUNKS_NUM_##size - 1; ++i)  \
        {                                                       \
            STORE_CHUNK(i, FRAME_SAVE_CHUNK_SIZE);              \
        }                                                       \
                                                                \
        STORE_CHUNK(i, FRAME_SAVE_LAST_CHUNK_SIZE_##size);      \
                                                                \
        return 0;                                               \
    }

/**
 * Macro defining a function for loading a frame of specified size.
 * The function prototype is load_frame_<size>(struct __sk_buff *skb)
 *
 * @param size  The frame size.
 */
#define LOAD_FUNC_IMPL(size) \
    static inline int                                           \
    load_frame_##size(struct __sk_buff *skb)                    \
    {                                                           \
        int i;                                                  \
                                                                \
        for (i = 0; i < FRAME_SAVE_CHUNKS_NUM_##size - 1; ++i)  \
        {                                                       \
            LOAD_CHUNK(i, FRAME_SAVE_CHUNK_SIZE);               \
        }                                                       \
                                                                \
        LOAD_CHUNK(i, FRAME_SAVE_LAST_CHUNK_SIZE_##size);       \
                                                                \
        return 0;                                               \
    }

/* load/store functions for a 1514 bytes packet. */
LOAD_FUNC_IMPL(1514)
STORE_FUNC_IMPL(1514)

/**
 * Load a frame with size according to @p size from @p skb context
 * to @ref pktbuf map.
 *
 * @param skb   BPF context.
 * @param size  Size.
 *
 * @return Status code - 0 on success, or a negative value in case of failure.
 */
static inline int
load_frame(struct __sk_buff *skb, ct_stim_del_frame size)
{
    switch (size)
    {
        case CT_DELAY_FRAME_SIZE_1514:
            return load_frame_1514(skb);

        default:
            return -1;
    }
}

/**
 * Store a frame with size according to @p size to @p skb context
 * from @ref pktbuf map.
 *
 * @param skb   BPF context.
 * @param size  Size.
 *
 * @return Status code - 0 on success, or a negative value in case of failure.
 */
static inline int
store_frame(struct __sk_buff *skb, ct_stim_del_frame size)
{
    switch (size)
    {
        case CT_DELAY_FRAME_SIZE_1514:
            return store_frame_1514(skb);

        default:
            return -1;
    }
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
    __u32   saved_framelen = 0;
    __u32  *cloned = NULL;

    ct_stim_del_frame *delay_frame_size = NULL;

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

    key = CT_BPF_DELAY_SIZE_KEY;
    delay_frame_size = bpf_map_lookup_elem(&ctrl, &key);

    if (ifindex == NULL || delay_frame_size == NULL)
        return TC_ACT_OK;

    switch (*delay_frame_size)
    {
        case CT_DELAY_FRAME_SIZE_1514:
            saved_framelen = FRAME_SAVE_SIZE_1514;
            break;

        default:
            printk("Unsupported frame size value\n");
            return TC_ACT_OK;
    }

    if (*delay != 0 && *flag == 0)
    {
        /*
         * A user has updated the delay counter. Save current packet,
         * do not send it.
         */

        if (skb->len != saved_framelen)
        {
            printk("Ignoring frame with size %u\n", skb->len);
            return TC_ACT_OK;
        }

        printk("save packet\n");

        if (load_frame(skb, *delay_frame_size) == 0)
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
        bpf_skb_change_tail(skb, saved_framelen, 0);
        if (store_frame(skb, *delay_frame_size) == 0)
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
