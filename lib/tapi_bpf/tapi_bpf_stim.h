/** @file
 * @brief Test API to control network stimuli.
 *
 * API to control network stimuli.
 *
 * Copyright (C) 2021 OKTET Labs. All rights reserved.
 *
 * @author Sergey Nikitin <Sergey.Nikitin@oktetlabs.ru>
 */

#ifndef __TAPI_BPF_STIM_H__
#define __TAPI_BPF_STIM_H__

#include "te_queue.h"
#include "te_errno.h"
#include "logger_api.h"
#include "tapi_rpc.h"
#include "tapi_bpf.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Stimulus types. */
typedef enum tapi_bpf_stim_type
{
    TAPI_BPF_STIM_STIMULUS_NONE = 0x0,
    TAPI_BPF_STIM_STIMULUS_DROP = 0x1,
    TAPI_BPF_STIM_STIMULUS_DUPLICATE = 0x2,
    TAPI_BPF_STIM_STIMULUS_DELAY = 0x4,
    TAPI_BPF_STIM_STIMULUS_SLOW_START = 0x8,
} tapi_bpf_stim_type;

/** Internal BPF context. */
typedef struct tapi_bpf_stim_ctx {
    LIST_ENTRY(tapi_bpf_stim_ctx) next;

    unsigned int        bpf_id;
    char               *bpf_path;
    tapi_bpf_link_point link_type;
    tapi_bpf_stim_type  stim_type;
} tapi_bpf_stim_ctx;

/**
 * BPF context list type.
 */
typedef LIST_HEAD(tapi_bpf_stim_ctx_list, tapi_bpf_stim_ctx)
        tapi_bpf_stim_ctx_list;

/** Congestion stimuli BPF handle type. */
typedef struct tapi_bpf_stim_hdl {
    const char  *ta;        /**< Agent where BPF is loaded. */
    const char  *ifname;    /**< Interface where BPF program is linked. */
    int          ifindex;   /**< Index of the interface. */

    tapi_bpf_stim_ctx_list bpf_ctxs; /**< List of BPF context instances for
                                          each loaded stimulus.*/
} tapi_bpf_stim_hdl;

/**
 * List of values allowed for parameter of type @ref tapi_bpf_stim_type.
 */
#define TAPI_BPF_STIM_STIMULUS \
    { "none", TAPI_BPF_STIM_STIMULUS_NONE }, \
    { "drop", TAPI_BPF_STIM_STIMULUS_DROP }, \
    { "duplicate", TAPI_BPF_STIM_STIMULUS_DUPLICATE }, \
    { "delay", TAPI_BPF_STIM_STIMULUS_DELAY }, \
    { "slow_start", TAPI_BPF_STIM_STIMULUS_SLOW_START }

/**
 * Get value of @ref tapi_bpf_stim_type parameter.
 *
 * @param _var_name     Test parameter name.
 */
#define TEST_GET_CT_STIMULUS_PARAM(_var_name) \
    TEST_GET_ENUM_PARAM(_var_name, TAPI_BPF_STIM_STIMULUS)

/**
 * Load specific BPF program according to stimulus type @p type,
 * enable clsact qdisc and link the program to tc attach point
 * on interface @p ifname, according to @p egress argument.
 *
 * @note The function jumps to cleanup in case of error.
 *
 * @param[in]  pco      RPC server.
 * @param[in]  ifname   Interface to link the program to.
 * @param[in]  type     Type of the stimulus.
 * @param[in]  egress   Link stimulus BPF program to tc egress if @c TRUE,
 *                      otherwise link to tc ingress.
 * @param[out] handle   BPF stimulus handle.
 */
extern void tapi_bpf_stim_init(rcf_rpc_server *pco,
                               const char *ifname,
                               unsigned int type,
                               te_bool egress,
                               tapi_bpf_stim_hdl **handle);

/**
 * Unlink and unload BPF stimulus program, disable clsact qdisc.
 *
 * @param handle    BPF stimulus handle.
 */
extern void tapi_bpf_stim_del(tapi_bpf_stim_hdl *handle);

/**
 * Activate "drop" stimulus. Drop next @p num packets.
 *
 * @param handle    BPF stimulus handle.
 * @param num       Number of packets to drop.
 *
 * @return Status code
 */
extern te_errno tapi_bpf_stim_drop(tapi_bpf_stim_hdl *handle,
                                   unsigned int num);

/**
 * Activate "duplicate" stimulus. Duplicate next packet @p num times.
 *
 * @param handle    BPF stimulus handle.
 * @param int       Number of copies to made.
 *
 * @return Status code
 */
extern te_errno tapi_bpf_stim_dup(tapi_bpf_stim_hdl *handle,
                                  unsigned int num);

/**
 * Activate "delay" stimulus. Delay next packet and send it after
 * @p num frames.
 *
 * @param handle        BPF stimulus handle.
 * @param num           How much packets to wait before sending delayed packet.
 *
 * @return Status code
 */
extern te_errno tapi_bpf_stim_delay(tapi_bpf_stim_hdl *handle,
                                    unsigned int num);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_BPF_STIM_H__ */
