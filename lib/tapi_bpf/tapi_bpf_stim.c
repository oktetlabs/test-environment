/** @file
 * @brief Test API to control network stimuli.
 *
 * Implementation of API to control network stimuli.
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 */

#include "te_config.h"

#include "tapi_bpf_stim.h"
#include "tapi_bpf.h"
#include "tapi_cfg_qdisc.h"
#include "tapi_test.h"
#include "conf_api.h"
#include "te_queue.h"
#include "tapi_mem.h"

/**
 * @defgroup tapi_bpf_stim_common_map Common map names to control BPF stimulus
 *                                    programs. All BPF programs provide this
 *                                    interface.
 * @{
 */
/**
 * Control map, containing 32bit integer values.
 * Number of elements may vary depending on stimulus type.
 */
#define TAPI_BPF_STIM_CTRL_MAP_NAME    "ctrl"
 /** Processed packets counter. */
#define TAPI_BPF_STIM_RXCNT_MAP_NAME   "rxcnt"
/**@} <!-- END tapi_bpf_stim_common_map --> */

/**
 * @defgroup tapi_bpf_stim_drop_cd Drop stimulus BPF control data.
 *
 * @{
 */
/** Program name. */
#define TAPI_BPF_STIM_DROP_PROG_NAME   "tc_drop"
 /** Key to access the number of packets to drop. */
#define TAPI_BPF_STIM_DROP_NUM_PKT_KEY 0
/**@} <!-- END tapi_bpf_stim_drop_cd --> */

/**
 * @defgroup tapi_bpf_stim_dup_cd Duplicate stimulus BPF control data.
 *
 * @{
 */
/** Program name. */
#define TAPI_BPF_STIM_DUP_PROG_NAME      "tc_dup"
 /** Key to access map field to write number of copies to made. */
#define TAPI_BPF_STIM_DUP_NUM_COPIES_KEY 0
/**
 * Key to access map field to write an interface index
 * to which packet copies are sent.
 */
#define TAPI_BPF_STIM_DUP_IFINDEX_KEY    1
/**
 * Key to access boolean map field containing flag whether to use
 * @c BPF_F_INGRESS flag in @b bpf_clone_redirect().
 */
#define TAPI_BPF_STIM_DUP_INGRESS_KEY 2
/**@} <!-- END tapi_bpf_stim_dup_cd --> */

/**
 * @defgroup tapi_bpf_stim_delay_cd Delay stimulus BPF control data.
 *
 * @{
 */
/**< Program name. */
#define TAPI_BPF_STIM_DELAY_PROG_NAME   "tc_delay"
/**
 * Key to access map field to write an interface index
 * to which delayed packet is  sent.
 */
#define TAPI_BPF_STIM_DELAY_IFINDEX_KEY 0
/** Key to access map field to write number of frames to delay. */
#define TAPI_BPF_STIM_DELAY_NUMPKT_KEY  1

/**
 * Key to access boolean map field containing flag whether to use
 * @c BPF_F_INGRESS flag in @b bpf_redirect().
 */
#define TAPI_BPF_STIM_DELAY_INGRESS_KEY 2

/**@} <!-- END tapi_bpf_stim_delay_cd --> */

/**
 * Allocate and initialize BPF context, load BPF object to kernel,
 * link BPF program to TC.
 *
 * @param handle        Congestion stimulus BPF handle.
 * @param prog_name     BPF program name.
 * @param type          Stimulus type.
 * @param egress        Link BPF program to TC egress if @c TRUE, otherwise
 *                      link to TC ingress.
 */
static void
tapi_bpf_stim_ctx_create(tapi_bpf_stim_hdl *handle, const char *prog_name,
                         tapi_bpf_stim_type type, te_bool egress)
{

    tapi_bpf_stim_ctx *bpf_ctx = tapi_calloc(1, sizeof(*bpf_ctx));

    LIST_INSERT_HEAD(&handle->bpf_ctxs, bpf_ctx, next);

    bpf_ctx->link_type = egress ? TAPI_BPF_LINK_TC_EGRESS :
                                  TAPI_BPF_LINK_TC_INGRESS;
    bpf_ctx->stim_type = type;
    bpf_ctx->bpf_path = tapi_bpf_build_bpf_obj_path(handle->ta, prog_name);

    CHECK_RC(tapi_bpf_obj_init(handle->ta, bpf_ctx->bpf_path,
                               TAPI_BPF_PROG_TYPE_SCHED_CLS,
                               &bpf_ctx->bpf_id));
    CHECK_RC(tapi_bpf_prog_name_check(handle->ta, bpf_ctx->bpf_id, prog_name));
    CHECK_RC(tapi_bpf_map_name_check(handle->ta, bpf_ctx->bpf_id,
                                     TAPI_BPF_STIM_CTRL_MAP_NAME));

    /*
     * Some stimulus may not have a 'rxcnt' map, so do not fail
     * if it is not loaded.
     */
    tapi_bpf_map_name_check(handle->ta, bpf_ctx->bpf_id,
                            TAPI_BPF_STIM_RXCNT_MAP_NAME);

    CHECK_RC(tapi_bpf_map_set_writable(handle->ta, bpf_ctx->bpf_id,
                                       TAPI_BPF_STIM_CTRL_MAP_NAME));
    CHECK_RC(tapi_bpf_prog_link(handle->ta, handle->ifname, bpf_ctx->bpf_id,
                                bpf_ctx->link_type, prog_name));
}

/**
 * Write a value to control map.
 *
 * @param ta        Agent to which BPF program is loaded.
 * @param bpfid     BPF ID.
 * @param key       Map key to write.
 * @param value     Value to write to the map.
 *
 * @return Status code.
 */
static te_errno
tapi_bpf_stim_ctrl_write(const char *ta, unsigned int bpfid, unsigned int key,
                         unsigned int value)
{
    return tapi_bpf_map_update_kvpair(ta, bpfid, TAPI_BPF_STIM_CTRL_MAP_NAME,
                                      (uint8_t *)&key, sizeof(key),
                                      (uint8_t *)&value, sizeof(value));
}

/* See description in ts_congestion.h */
void
tapi_bpf_stim_init(rcf_rpc_server *pco, const char *ifname,
                   unsigned int type, te_bool egress,
                   tapi_bpf_stim_hdl **handle)
{
    tapi_bpf_stim_hdl *hdl = NULL;

    if (type == TAPI_BPF_STIM_STIMULUS_NONE)
        return;

    if (handle == NULL)
        TEST_FAIL("%s(): handle == NULL", __FUNCTION__);

    hdl = tapi_calloc(1, sizeof(*hdl));
    hdl->ta = pco->ta;
    hdl->ifname = ifname;
    LIST_INIT(&hdl->bpf_ctxs);

    *handle = hdl;

    CHECK_RC(cfg_get_instance_int_fmt(&hdl->ifindex,
                                      "/agent:%s/interface:%s/index:", hdl->ta,
                                      hdl->ifname));

    CHECK_RC(tapi_cfg_qdisc_set_kind(hdl->ta, hdl->ifname,
                                     TAPI_CFG_QDISC_KIND_CLSACT));
    CHECK_RC(tapi_cfg_qdisc_enable(hdl->ta, hdl->ifname));

    switch (type)
    {
        case TAPI_BPF_STIM_STIMULUS_DROP:
            tapi_bpf_stim_ctx_create(hdl, TAPI_BPF_STIM_DROP_PROG_NAME,
                                     TAPI_BPF_STIM_STIMULUS_DROP, egress);
            break;
        case TAPI_BPF_STIM_STIMULUS_DUPLICATE:
            tapi_bpf_stim_ctx_create(hdl, TAPI_BPF_STIM_DUP_PROG_NAME,
                                     TAPI_BPF_STIM_STIMULUS_DUPLICATE, egress);
            break;
        case TAPI_BPF_STIM_STIMULUS_DELAY:
            tapi_bpf_stim_ctx_create(hdl, TAPI_BPF_STIM_DELAY_PROG_NAME,
                                     TAPI_BPF_STIM_STIMULUS_DELAY, egress);
            break;
        default:
            TEST_FAIL("%s(): unsupported BPF stimulus", __FUNCTION__);
    }
}

/* See description in tapi_bpf_stim.h */
void
tapi_bpf_stim_del(tapi_bpf_stim_hdl *handle)
{
    tapi_bpf_stim_ctx *item;
    tapi_bpf_stim_ctx *tmp;

    if (handle == NULL)
        return;

    LIST_FOREACH_SAFE(item,
                      &handle->bpf_ctxs,
                      next, tmp)
    {
        tapi_bpf_prog_unlink(handle->ta, handle->ifname, item->link_type);
        tapi_bpf_obj_fini(handle->ta, item->bpf_id);
        LIST_REMOVE(item, next);
        free(item);
    }

    tapi_cfg_qdisc_disable(handle->ta, handle->ifname);

    free(handle);
}

/* See description in tapi_bpf_stim.h */
te_errno
tapi_bpf_stim_drop(tapi_bpf_stim_hdl *handle, unsigned int num)
{
    tapi_bpf_stim_ctx *bpf_ctx;

    LIST_FOREACH(bpf_ctx, &handle->bpf_ctxs, next)
    {
        if (bpf_ctx->stim_type == TAPI_BPF_STIM_STIMULUS_DROP)
        {
            return tapi_bpf_stim_ctrl_write(handle->ta, bpf_ctx->bpf_id,
                                            TAPI_BPF_STIM_DROP_NUM_PKT_KEY,
                                            num);
        }
    }

    ERROR("BPF drop stimulus was not found");
    return TE_EFAIL;
}

/* See description in tapi_bpf_stim.h */
te_errno
tapi_bpf_stim_dup(tapi_bpf_stim_hdl *handle, unsigned int num)
{
    tapi_bpf_stim_ctx *bpf_ctx;

    LIST_FOREACH(bpf_ctx, &handle->bpf_ctxs, next)
    {
        if (bpf_ctx->stim_type == TAPI_BPF_STIM_STIMULUS_DUPLICATE)
        {
            te_errno rc = 0;

            rc = tapi_bpf_stim_ctrl_write(handle->ta, bpf_ctx->bpf_id,
                                          TAPI_BPF_STIM_DUP_IFINDEX_KEY,
                                          handle->ifindex);
            if (rc != 0)
                return rc;

            if (bpf_ctx->link_type == TAPI_BPF_LINK_TC_INGRESS)
            {
                rc = tapi_bpf_stim_ctrl_write(handle->ta, bpf_ctx->bpf_id,
                                              TAPI_BPF_STIM_DUP_INGRESS_KEY,
                                              TRUE);
                if (rc != 0)
                    return rc;
            }

            return tapi_bpf_stim_ctrl_write(handle->ta, bpf_ctx->bpf_id,
                                            TAPI_BPF_STIM_DUP_NUM_COPIES_KEY,
                                            num);
        }
    }

    ERROR("BPF duplicate stimulus was not found");
    return TE_EFAIL;
}

/* See description in tapi_bpf_stim.h */
te_errno
tapi_bpf_stim_delay(tapi_bpf_stim_hdl *handle, unsigned int num)
{
    tapi_bpf_stim_ctx *bpf_ctx;

    LIST_FOREACH(bpf_ctx, &handle->bpf_ctxs, next)
    {
        if (bpf_ctx->stim_type == TAPI_BPF_STIM_STIMULUS_DELAY)
        {
            te_errno rc = 0;

            rc = tapi_bpf_stim_ctrl_write(handle->ta, bpf_ctx->bpf_id,
                                          TAPI_BPF_STIM_DELAY_IFINDEX_KEY,
                                          handle->ifindex);
            if (rc != 0)
                return rc;

            if (bpf_ctx->link_type == TAPI_BPF_LINK_TC_INGRESS)
            {
                rc = tapi_bpf_stim_ctrl_write(handle->ta, bpf_ctx->bpf_id,
                                              TAPI_BPF_STIM_DELAY_INGRESS_KEY,
                                              TRUE);
                if (rc != 0)
                    return rc;
            }

            return tapi_bpf_stim_ctrl_write(handle->ta, bpf_ctx->bpf_id,
                                            TAPI_BPF_STIM_DELAY_NUMPKT_KEY,
                                            num);
        }
    }

    ERROR("BPF delay stimulus was not found");
    return TE_EFAIL;
}
