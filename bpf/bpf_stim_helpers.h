/** @file
 * @brief BPF defenition
 *
 * Auxilliary functions and structures for BPF programs.
 *
 * Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __BPF_STIM_HELPERS_H__
#define __BPF_STIM_HELPERS_H__

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

/**
 * Enable/disable debug mode - print debug messages.
 * The output is located in /sys/kernel/debug/tracing/trace
 */
#ifndef TC_DEBUG
#define TC_DEBUG 0
#endif /* TC_DEBUG */

/**
 * Macro to declare BPF helper functions.
 */
#define BPF_HELPER_DECL(_rettype, _name, ...) \
    static _rettype (*bpf_##_name)(__VA_ARGS__) = (void *) BPF_FUNC_##_name

/* helper macro to place programs, maps, license */
#define SEC(NAME) __attribute__((section(NAME), used))

/* Structure of maps used in BPF programs */
struct bpf_map {
    unsigned int type;
    unsigned int key_size;
    unsigned int value_size;
    unsigned int max_entries;
    unsigned int map_flags;
    unsigned int inner_map_idx;
};

/* Kernel functions to work with maps */
BPF_HELPER_DECL(void *, map_lookup_elem, void *map, void *key);

BPF_HELPER_DECL(int, redirect, int ifindex, int flags);
BPF_HELPER_DECL(int, clone_redirect, struct __sk_buff *skb,
                                     __u32 ifindex,
                                     __u64 flags);
BPF_HELPER_DECL(int, skb_load_bytes, const struct __sk_buff *skb, __u32 offset,
                                     void *to, __u32 len);
BPF_HELPER_DECL(int, skb_store_bytes, const struct __sk_buff *skb, __u32 offset,
                                      const void *from, __u32 len, __u64 flags);
BPF_HELPER_DECL(int, skb_change_tail, const struct __sk_buff *skb, __u32 len,
                                      __u64 flags);

#if TC_DEBUG
BPF_HELPER_DECL(int, trace_printk, const char *fmt, __u32 fmt_size, ...);

#define printk(fmt, ...)                                \
        ({                                              \
            char ____fmt[] = fmt;                       \
            bpf_trace_printk(____fmt, sizeof(____fmt),  \
                     ##__VA_ARGS__);                    \
        })
#else
#define printk(...)
#endif /* TC_DEBUG */

#endif /* !__BPF_STIM_HELPERS_H__ */
