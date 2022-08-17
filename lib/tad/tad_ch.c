/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Command Handler
 *
 * Traffic Application Domain Command Handler.
 * Definition of routines provided for Portable Command Handler.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD CH"

#include "te_config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <pthread.h>

#include "logger_api.h"
#include "logger_ta_fast.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "te_defs.h"

#ifndef TAD_DUMMY

#include "ndn.h"

#include "tad_types.h"
#include "csap_id.h"
#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "tad_send_recv.h"
#include "tad_agent_csap.h"
#include "tad_reply_rcf.h"
#include "tad_api.h"


#define SEND_ANSWER(_fmt...) \
    do {                                                                   \
        te_errno _rc;                                                      \
                                                                           \
        if ((size_t)snprintf(cbuf + answer_plen, buflen - answer_plen,     \
                             _fmt) >= (buflen - answer_plen))              \
        {                                                                  \
            VERB("answer is truncated\n");                                 \
        }                                                                  \
        RCF_CH_LOCK;                                                       \
        _rc = rcf_comm_agent_reply(rcfc, cbuf, strlen(cbuf) + 1);          \
        RCF_CH_UNLOCK;                                                     \
        if (_rc)                                                           \
            fprintf(stderr, "rc from rcf_comm_agent_reply: 0x%X\n", _rc);  \
    } while (0)

#endif


/**
 * Check TAD initialization. It is intended to be called from RCF
 * Command Handler routines only. If initialization is not done or
 * failed, the macro calls return with -1 (no support).
 */
#define TAD_CHECK_INIT \
    do {                            \
        if (!tad_is_initialized)    \
            return -1;              \
    } while (0)


static te_bool tad_is_initialized = FALSE;


/* See description in rcf_ch_api.h */
te_errno
rcf_ch_tad_init(void)
{
    if (tad_is_initialized)
        return TE_RC(TE_TAD_CH, TE_EALREADY);

    /*
     * Set initialized flag in any case, if error occur it will be
     * unsed, by shutdown routine.
     */
    tad_is_initialized = TRUE;

#ifdef TAD_DUMMY

    return TE_RC(TE_TAD_CH, TE_ENOSYS);

#else /* !TAD_DUMMY */

#define CHECK_RC(_expr) \
    do {                                            \
        te_errno rc_ = (_expr);                     \
                                                    \
        if (rc_ != 0)                               \
        {                                           \
            ERROR("%s failed: %r",  #_expr, rc_);   \
            return rc_;                             \
        }                                           \
    } while (0)

    csap_id_init();
    CHECK_RC(csap_spt_init());

#ifdef WITH_ATM
    extern te_errno csap_support_atm_register(void);
    CHECK_RC(csap_support_atm_register());
#endif
#ifdef WITH_ETH
    extern te_errno csap_support_eth_register(void);
    CHECK_RC(csap_support_eth_register());
#endif
#ifdef WITH_ARP
    extern te_errno csap_support_arp_register(void);
    CHECK_RC(csap_support_arp_register());
#endif
#ifdef WITH_IPSTACK
    extern te_errno csap_support_ipstack_register(void);
    CHECK_RC(csap_support_ipstack_register());
#endif
#ifdef WITH_IGMP
    extern te_errno csap_support_igmp_register(void);
    CHECK_RC(csap_support_igmp_register());
#endif
#ifdef WITH_SNMP
    extern te_errno csap_support_snmp_register(void);
    CHECK_RC(csap_support_snmp_register());
#endif
#ifdef WITH_CLI
    extern te_errno csap_support_cli_register(void);
    CHECK_RC(csap_support_cli_register());
#endif
#ifdef WITH_DHCP
    extern te_errno csap_support_dhcp_register(void);
    extern te_errno csap_support_dhcp6_register(void);
    CHECK_RC(csap_support_dhcp_register());
    CHECK_RC(csap_support_dhcp6_register());
#endif
#ifdef WITH_BRIDGE
    extern te_errno csap_support_bridge_register(void);
    CHECK_RC(csap_support_bridge_register());
#endif
#ifdef WITH_PCAP
    extern te_errno csap_support_pcap_register(void);
    CHECK_RC(csap_support_pcap_register());
#endif
#ifdef WITH_ISCSI
    extern te_errno csap_support_iscsi_register(void);
    CHECK_RC(csap_support_iscsi_register());
#endif
#ifdef WITH_SOCKET
    extern te_errno csap_support_socket_register(void);
    CHECK_RC(csap_support_socket_register());
#endif
#ifdef WITH_PPP
    extern te_errno csap_support_ppp_register(void);
    CHECK_RC(csap_support_ppp_register());
#endif
#ifdef WITH_RTE_MBUF
    extern te_errno csap_support_rte_mbuf_register(void);
    CHECK_RC(csap_support_rte_mbuf_register());
#endif
#ifdef WITH_VXLAN
    extern te_errno csap_support_vxlan_register(void);
    CHECK_RC(csap_support_vxlan_register());
#endif
#ifdef WITH_GENEVE
    extern te_errno csap_support_geneve_register(void);
    CHECK_RC(csap_support_geneve_register());
#endif
#ifdef WITH_GRE
    extern te_errno csap_support_gre_register(void);
    CHECK_RC(csap_support_gre_register());
#endif

#ifdef WITH_CS
    CHECK_RC(tad_agent_csap_init());
#endif

#undef CHECK_RC

    return 0;

#endif /* !TAD_DUMMY */
}

/* See description in rcf_ch_api.h */
te_errno
rcf_ch_tad_shutdown(void)
{
    te_errno    rc = 0;

    if (!tad_is_initialized)
        return TE_RC(TE_TAD_CH, TE_EINVAL);

    /* The function continues shutdown even in the case of failures */
    tad_is_initialized = FALSE;

#ifndef TAD_DUMMY
#if defined(WITH_CS)
    tad_agent_csap_fini();
#endif
    csap_spt_destroy();
    csap_id_destroy();
#endif

    return rc;
}

#ifndef TAD_DUMMY

/**
 * Safe compare two strings. Almost equivalent to standard "strcmp", but
 * correct work if one or both arguments are NULL.
 * If both arguments are empty strings or NULL (in any combination), they
 * are considered as equal and zero is returned.
 *
 * @param l        first string
 * @param r        second string
 *
 * @return same value as strcmp.
 */
static inline int
strcmp_imp(const char *l, const char *r)
{
    if (l == NULL)
    {
        if ((r == NULL) || (*r == '\0'))
            return 0;
        else
            return -1;
    }
    if (r == NULL)
    {
        if (*l == '\0')
            return 0;
        else
            return 1;
    }

    return strcmp(l, r);
}

/* See the description in tad_api.h */
te_errno
tad_csap_create(const char *stack, const char *spec_str, csap_p *new_csap_p)
{
    csap_p           new_csap;
    unsigned int     layer;
    int              syms;
    te_errno         rc;
    const asn_value *csap_layers;
    csap_spt_type_p  csap_spt_descr;
    int32_t          i32_tmp;

    if (!tad_is_initialized)
    {
        rc = TE_ETADNOINIT;
        goto fail_tad_no_init;
    }

    rc = csap_create(stack, &new_csap);
    if (rc != 0)
    {
        ERROR("CSAP '%s' creation internal error %r", stack, rc);
        goto fail_csap_create;
    }

    INFO("CSAP '%s' created, new id: %u", stack, new_csap->id);

    if (spec_str == NULL)
    {
        ERROR("Missing attached NDS with CSAP parameters");
        goto exit;
    }

    rc = asn_parse_value_text(spec_str, ndn_csap_spec, &new_csap->nds, &syms);
    if (rc != 0)
    {
        ERROR("CSAP NDS parse error sym=%d: %r", syms, rc);
        goto exit;
    }
    assert(new_csap->nds != NULL);

    /* 'stop-latency-timeout-ms' parameter processing */
    rc = asn_read_int32(new_csap->nds, &i32_tmp,
                        "params.stop-latency-timeout-ms");
    if (rc == 0)
    {
        new_csap->stop_latency_timeout = TE_MS2US(i32_tmp);
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        /* Unspecified, keep the default value */
    }
    else
    {
        ERROR("Failed to read 'stop-latency-timeout-ms' from CSAP "
              "NDS: %r", rc);
        goto exit;
    }

    /* 'receive-timeout-ms' parameter processing */
    rc = asn_read_int32(new_csap->nds, &i32_tmp,
                        "params.receive-timeout-ms");
    if (rc == 0)
    {
        new_csap->recv_timeout = TE_MS2US(i32_tmp);
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        /* Unspecified, keep the default value */
    }
    else
    {
        ERROR("Failed to read 'receive-timeout-ms' from CSAP NDS: %r", rc);
        goto exit;
    }

    /* Get layers specification */
    rc = asn_get_child_value(new_csap->nds, &csap_layers,
                             PRIVATE, NDN_CSAP_LAYERS);
    if (rc != 0)
    {
        ERROR("Failed to get CSAP layers: %r", rc);
        goto exit;
    }
    assert(csap_layers != NULL);

    /*
     * Get CSAP specification parameters for each layer
     */
    for (layer = 0; layer < new_csap->depth; ++layer)
    {
        rc = asn_get_indexed(csap_layers, &new_csap->layers[layer].nds,
                             layer, NULL);
        if (rc != 0)
        {
            ERROR("Get %u layer generic PDU from CSAP NDS failed: %r",
                  layer, rc);
            goto exit;
        }
        rc = asn_get_choice_value(new_csap->layers[layer].nds,
                                  &new_csap->layers[layer].nds,
                                  NULL, NULL);
        if (rc != 0)
        {
            ERROR("Get choice on %u layer from generic PDU in CSAP NDS "
                  "failed: %r", layer, rc);
            goto exit;
        }
    }

    /*
     * Initialize read/write layer (the lowest) af first.
     */
    new_csap->rw_layer = new_csap->depth - 1;
    csap_spt_descr =
        csap_get_proto_support(new_csap, csap_get_rw_layer(new_csap));
    if (csap_spt_descr->rw_init_cb == NULL)
    {
        ERROR("The lowest CSAP layer '%s' does not have read/write "
              "initialization routine", csap_spt_descr->proto);
        rc = TE_RC(TE_TAD_CH, TE_EPROTONOSUPPORT);
        goto exit;
    }
    else if ((rc = csap_spt_descr->rw_init_cb(new_csap)) != 0)
    {
        ERROR("Initialization of the lowest layer '%s' to read/write "
              "failed: %r", csap_spt_descr->proto, rc);
        goto exit;
    }

    /*
     * Initialize CSAP layers from lower to upper
     */
    for (layer = new_csap->depth; layer-- > 0; )
    {
        csap_spt_descr = csap_get_proto_support(new_csap, layer);

        if ((csap_spt_descr->init_cb != NULL) &&
            (rc = csap_spt_descr->init_cb(new_csap, layer)) != 0)
        {
            ERROR(CSAP_LOG_FMT "Initialization of layer #%u '%s' "
                  "failed: %r", CSAP_LOG_ARGS(new_csap), layer,
                  csap_spt_descr->proto, rc);
            break;
        }
    }

exit:
    if (rc != 0)
        (void)csap_destroy(new_csap->id);
    else
        *new_csap_p = new_csap;

fail_csap_create:
fail_tad_no_init:
    return rc;
}

/**
 * Wait for exclusive use of the CSAP.
 *
 * @param csap          CSAP instance
 *
 * @return Status code.
 */
static inline te_errno
csap_wait_exclusive_use(csap_p csap)
{
    te_errno    rc = 0;
    int         ret;

    CSAP_LOCK(csap);
    while (csap->ref > 1)
    {
        ret = pthread_cond_wait(&csap->event, &csap->lock);
        if (ret != 0)
        {
            rc = TE_OS_RC(TE_TAD_CH, ret);
            assert(TE_RC_GET_ERROR(rc) != TE_ENOENT);
            ERROR("%s(): pthread_cond_wait() failed: %r",
                  __FUNCTION__, rc);
            break;
        }
    }
    CSAP_UNLOCK(csap);

    return rc;
}

/* See the description in tad_api.h */
te_errno
tad_csap_destroy(csap_p csap)
{
    csap_spt_type_p csap_spt_descr;

    unsigned int layer;
    te_errno     rc;

    VERB("%s: " CSAP_LOG_FMT, __FUNCTION__, CSAP_LOG_ARGS(csap));

    rc = csap_command(csap, TAD_OP_DESTROY);
    if (rc != 0)
    {
        return rc;
    }

    if (~csap->state & CSAP_STATE_IDLE)
    {
        rc = csap_wait(csap, CSAP_STATE_DONE);
        if (rc != 0)
        {
            /*
             * It is better to keep CSAP open rather than get
             * segmentation fault because of invalid destruction.
             */
            return rc;
        }
    }

    /*
     * If we get exclude use after destroy command, it is guaranteed
     * that noone will start to use it again.
     */
    rc = csap_wait_exclusive_use(csap);
    if (rc != 0)
    {
        /*
         * It is better to keep CSAP open rather than get
         * segmentation fault because of invalid destruction.
         */
        return rc;
    }

    /* CSAP should be either IDLE or DONE */
    assert(csap->state & (CSAP_STATE_IDLE | CSAP_STATE_DONE));

    INFO(CSAP_LOG_FMT "Starting destruction", CSAP_LOG_ARGS(csap));

    csap_spt_descr = csap_get_proto_support(csap, csap_get_rw_layer(csap));
    if (csap_spt_descr->rw_destroy_cb != NULL &&
        (rc = csap_spt_descr->rw_destroy_cb(csap)) != 0)
    {
        ERROR(CSAP_LOG_FMT "Destruction of the read/write layer #%u '%s' "
              "failed: %r", CSAP_LOG_ARGS(csap), csap_get_rw_layer(csap),
              csap_spt_descr->proto, rc);
    }

    for (layer = 0; layer < csap->depth; layer++)
    {
        csap_spt_descr = csap_get_proto_support(csap, layer);
        if (csap_spt_descr->destroy_cb != NULL)
        {
            rc = csap_spt_descr->destroy_cb(csap, layer);
            if (rc != 0)
            {
                ERROR(CSAP_LOG_FMT "Destruction of the layer #%u '%s' "
                      "failed: %r", CSAP_LOG_ARGS(csap), layer,
                      csap_spt_descr->proto, rc);
            }
        }
    }
    csap_destroy(csap->id);

    return rc;
}

/* See the description in tad_agent_csap.h */
te_errno
tad_csap_destroy_by_id(csap_handle_t csap_id)
{
    csap_p  csap;

    VERB("%s: CSAP %u\n", __FUNCTION__, csap_id);

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        return TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX);
    }

    return tad_csap_destroy(csap);
}

#endif /* ndef TAD_DUMMY */

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_create(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen,
                   const char *stack, const char *params)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("CSAP create: stack <%s> params <%s>\n", stack, params);
    return -1;
#else
    te_errno    rc;
    csap_p      new_csap;

    UNUSED(cmdlen);
    UNUSED(params);

    TAD_CHECK_INIT;

    rc = tad_csap_create(stack, (const char *)ba, &new_csap);
    if (rc != 0)
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
    else
        SEND_ANSWER("0 %u", new_csap->id);

    return 0;
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_destroy(struct rcf_comm_connection *rcfc,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    csap_handle_t csap_id)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("CSAP destroy: CSAP %u\n", csap_id);
    return -1;
#else
    TAD_CHECK_INIT;

    VERB("%s(CSAP %u), answer prefix %s", __FUNCTION__, csap_id,  cbuf);
    cbuf[answer_plen] = '\0';

    SEND_ANSWER("%u", tad_csap_destroy_by_id(csap_id));

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_start(struct rcf_comm_connection *rcfc,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    const uint8_t *ba, size_t cmdlen,
                    csap_handle_t csap_id, te_bool postponed)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("TRSEND start: CSAP %u %s\n",
         csap_id, postponed ? "postponed" : "");
    return -1;
#else
    int                 rc;
    csap_p              csap;
    tad_reply_context   reply_ctx;

    UNUSED(cmdlen);

    VERB("%s(CSAP %u)", __FUNCTION__, csap_id);
    cbuf[answer_plen] = '\0';

    TAD_CHECK_INIT;

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("CSAP %u does not exist", csap_id);
        rc = TE_ETADCSAPNOTEX;
        goto send_answer_no_csap;
    }

    rc = tad_reply_rcf_init(&reply_ctx, rcfc, cbuf, answer_plen);
    if (rc != 0)
        goto send_answer_fail_reply_ctx_init;

    rc = tad_send_start_prepare(csap, (const char *)ba, postponed,
                                &reply_ctx);
    if (rc != 0)
        goto send_answer_fail_prepare;

    rc = tad_pthread_create(NULL, tad_send_thread, csap);
    if (rc != 0)
        goto send_answer_fail_thread;

    /* Send context has its own copy */
    tad_reply_cleanup(&reply_ctx);

    return 0;

send_answer_fail_thread:
    (void)tad_send_release(csap, csap_get_send_context(csap));
    (void)csap_command(csap, TAD_OP_IDLE);
send_answer_fail_prepare:
    tad_reply_cleanup(&reply_ctx);
send_answer_fail_reply_ctx_init:
send_answer_no_csap:
    SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_stop(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap_id)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRSEND stop CSAP %u\n", csap_id);
    return -1;
#else
    te_errno        rc;
    csap_p          csap;
    unsigned int    sent_pkts = 0;

    TAD_CHECK_INIT;

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("CSAP %u does not exist", csap_id);
        rc = TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX);
    }
    else
    {
        rc = tad_send_stop(csap, &sent_pkts);
    }

    SEND_ANSWER("%u %u", rc, sent_pkts);

    return 0;
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_start(struct rcf_comm_connection *rcfc,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    const uint8_t *ba, size_t cmdlen, csap_handle_t csap_id,
                    unsigned int num, unsigned int timeout,
                    unsigned int flags)
{
#ifndef TAD_DUMMY
    csap_p              csap;
    te_errno            rc;
    tad_reply_context   reply_ctx;
#endif

    INFO("%s: csap %u, num %u, timeout %u ms, flags=%x", __FUNCTION__,
         csap_id, num, timeout, flags);

#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(csap_id);
    UNUSED(num);
    UNUSED(flags);
    UNUSED(timeout);

    return -1;
#else

    UNUSED(cmdlen);

    TAD_CHECK_INIT;

    if ((csap = csap_find(csap_id)) == NULL)
    {
        ERROR("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        rc = TE_ETADCSAPNOTEX;
        goto send_answer_no_csap;
    }

    rc = tad_reply_rcf_init(&reply_ctx, rcfc, cbuf, answer_plen);
    if (rc != 0)
        goto send_answer_fail_reply_ctx_init;

    rc = tad_recv_start_prepare(csap, (const char *)ba, num, timeout, flags,
                                &reply_ctx);
    if (rc != 0)
        goto send_answer_fail_prepare;

    rc = tad_pthread_create(NULL, tad_recv_thread, csap);
    if (rc != 0)
        goto send_answer_fail_thread;

    /* Receive context has its own copy */
    tad_reply_cleanup(&reply_ctx);

    return 0;

send_answer_fail_thread:
    (void)tad_recv_release(csap, csap_get_recv_context(csap));
    (void)csap_command(csap, TAD_OP_IDLE);
send_answer_fail_prepare:
    tad_reply_cleanup(&reply_ctx);
send_answer_fail_reply_ctx_init:
send_answer_no_csap:
    SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
    return 0;
#endif
}


#ifndef TAD_DUMMY
/**
 * Generic implementation of trrecv_stop/wait/get.
 *
 * @param rcfc          RCF communication handle
 * @param cbuf          Buffer with request
 * @param answer_plen   Answer prefix length
 * @param csap_id       CSAP ID
 * @param op            Operation (stop/wait/get)
 *
 * @return Status code.
 */
static te_errno
tad_trrecv_op(rcf_comm_connection *rcfc,
              char *cbuf, size_t buflen, size_t answer_plen,
              csap_handle_t csap_id, tad_traffic_op_t op)
{
    csap_p              csap;
    te_errno            rc;
    tad_reply_context   reply_ctx;

    VERB("%s: CSAP %u OP %u", __FUNCTION__, csap_id, op);

    TAD_CHECK_INIT;

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        rc = TE_ETADCSAPNOTEX;
        goto send_answer_no_csap;
    }

    rc = csap_command(csap, op);
    if (rc != 0)
        goto send_answer_fail_command;

    rc = tad_reply_rcf_init(&reply_ctx, rcfc, cbuf, answer_plen);
    if (rc != 0)
        goto send_answer_fail_reply_ctx_init;

    rc = tad_recv_op_enqueue(csap, op, &reply_ctx);
    if (rc != 0)
    {
        /* Keep set flags, there is only one way now - destroy */
        goto send_answer_fail_enqueue;
    }

    /* Receive operation context has its own copy */
    tad_reply_cleanup(&reply_ctx);

    return 0;

send_answer_fail_enqueue:
    tad_reply_cleanup(&reply_ctx);
send_answer_fail_reply_ctx_init:
send_answer_fail_command:
send_answer_no_csap:
    SEND_ANSWER("%u 0", TE_RC(TE_TAD_CH, rc));
    return 0;
}
#endif

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_stop(rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap_id)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRRECV stop CSAP %u\n", csap_id);
    return -1;
#else
    return tad_trrecv_op(rcfc, cbuf, buflen, answer_plen, csap_id,
                         TAD_OP_STOP);
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_wait(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap_id)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(csap_id);

    VERB("%s: CSAP %u", __FUNCTION__, csap_id);

    return -1;
#else
    return tad_trrecv_op(rcfc, cbuf, buflen, answer_plen, csap_id,
                         TAD_OP_WAIT);
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_get(struct rcf_comm_connection *rcfc,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap_id)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRRECV get CSAP %u\n", csap_id);
    return -1;
#else
    return tad_trrecv_op(rcfc, cbuf, buflen, answer_plen, csap_id,
                         TAD_OP_GET);
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_recv(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen, csap_handle_t csap_id,
                   unsigned int timeout, unsigned int flags)
{
#ifndef TAD_DUMMY
    csap_p              csap;
    te_errno            rc;
    int                 syms;
    asn_value          *tmpl = NULL;
    asn_value          *ptrn = NULL;
    tad_reply_context   reply_ctx;
#endif

    INFO("%s: csap %u, timeout %u ms, flags=%x", __FUNCTION__,
         csap_id, timeout, flags);

#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    return -1;
#else

    UNUSED(cmdlen);

    TAD_CHECK_INIT;

    if ((csap = csap_find(csap_id)) == NULL)
    {
        ERROR("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        rc = TE_ETADCSAPNOTEX;
        goto send_answer_no_csap;
    }

    rc = csap_command(csap, TAD_OP_SEND_RECV);
    if (rc != 0)
        goto send_answer_fail_command;

    if (ba == NULL)
    {
        ERROR(CSAP_LOG_FMT "No NDS attached to traffic send/receive start "
              "command", CSAP_LOG_ARGS(csap));
        rc = TE_ETADMISSNDS;
        goto send_answer_no_ba;
    }

    rc = asn_parse_value_text((const char *)ba, ndn_traffic_template,
                              &tmpl, &syms);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Parse error in attached NDS on symbol %d: %r",
              CSAP_LOG_ARGS(csap), syms, rc);
        goto send_answer_bad_ba;
    }

    CSAP_LOCK(csap);

    if (flags & RCF_CH_TRRECV_PACKETS)
        csap->state |= CSAP_STATE_RESULTS;

    /*
     * Set results flag in the case of mismatch receive to enable
     * processing in post match callbacks which fill in packet NDS.
     * Also the flag enables purge of packets queue on stop.
     */
    if (flags & RCF_CH_TRRECV_MISMATCH)
        csap->state |= CSAP_STATE_RESULTS | CSAP_STATE_RECV_MISMATCH;

    if ((csap->state & CSAP_STATE_RESULTS) &&
        (flags & RCF_CH_TRRECV_PACKETS_NO_PAYLOAD))
        csap->state |= CSAP_STATE_PACKETS_NO_PAYLOAD;

    csap->first_pkt = tad_tv_zero;
    csap->last_pkt  = tad_tv_zero;

    CSAP_UNLOCK(csap);

    rc = tad_reply_rcf_init(&reply_ctx, rcfc, cbuf, answer_plen);
    if (rc != 0)
        goto send_answer_fail_reply_ctx_init;

    rc = tad_send_prepare(csap, tmpl, &reply_ctx);
    if (rc != 0)
        goto send_answer_fail_send_prepare;

    if (csap_get_send_context(csap)->tmpl_data.n_units != 1)
    {
        ERROR(CSAP_LOG_FMT "Invalid number of units in send/recv template",
              CSAP_LOG_ARGS(csap));
        rc = TE_ETADWRONGNDS;
        goto send_answer_bad_tmpl;
    }

    rc = tad_send_recv_generate_pattern(csap, tmpl, &ptrn);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to generate pattern by template: %r",
              CSAP_LOG_ARGS(csap), rc);
        goto send_answer_bad_tmpl;
    }

    rc = tad_recv_prepare(csap, ptrn, 1, timeout, &reply_ctx);
    if (rc != 0)
        goto send_answer_fail_recv_prepare;

    /*
     * Start threads in reverse order.
     */

    /* Emulate 'trrecv_wait' operation */
    rc = tad_recv_op_enqueue(csap, TAD_OP_WAIT, &reply_ctx);
    if (rc != 0)
        goto send_answer_fail_wait;

    /*
     * Don't care about Receiver context any more, since 'trrecv_wait'
     * operation is responsible for destruction. However, now it is
     * necessary to care about stop of 'trrecv_wait' operation.
     */

    if ((rc = tad_pthread_create(NULL, tad_recv_thread, csap)) != 0)
    {
        /* Set status of the Receiver */
        csap_get_recv_context(csap)->status = rc;
        /* We can release Sender context, since it has not been started */
        (void)tad_send_release(csap, csap_get_send_context(csap));
        /* Unblock 'trrecv_wait' operation */
        (void)csap_command(csap, TAD_OP_RECV_DONE);
    }
    else if ((rc = tad_pthread_create(NULL, tad_send_thread, csap)) != 0)
    {
        /* Set status of the Sender */
        csap_get_send_context(csap)->status = rc;
        /* We can release Sender context, since it has not been started */
        (void)tad_send_release(csap, csap_get_send_context(csap));
        /* Notify Receiver that send has been finished */
        (void)csap_command(csap, TAD_OP_SEND_DONE);
    }

    /* Send/receive contexts have its own copies */
    tad_reply_cleanup(&reply_ctx);

    /*
     * Emulated 'trrecv_wait' is responsible for send of answer and
     * transition to IDLE state.
     */
    return 0;

send_answer_fail_wait:
    (void)tad_recv_release(csap, csap_get_recv_context(csap));
send_answer_fail_recv_prepare:
send_answer_bad_tmpl:
    (void)tad_send_release(csap, csap_get_send_context(csap));
send_answer_fail_send_prepare:
    tad_reply_cleanup(&reply_ctx);
send_answer_fail_reply_ctx_init:
send_answer_bad_ba:
send_answer_no_ba:
    (void)csap_command(csap, TAD_OP_IDLE);
send_answer_fail_command:
send_answer_no_csap:
    SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
    return 0;
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trpoll(struct rcf_comm_connection *rcfc,
              char *cbuf, size_t buflen, size_t answer_plen,
              csap_handle_t csap_id, unsigned int timeout)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(csap_id);
    UNUSED(timeout);

    return -1;
#else
    csap_p              csap;
    te_errno            rc;
    tad_reply_context   reply_ctx;

    TAD_CHECK_INIT;

    /*
     * Answers are send with 0 in poll ID, since no request to be
     * canceled.
     */

    if ((csap = csap_find(csap_id)) == NULL)
    {
        ERROR("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        rc = TE_ETADCSAPNOTEX;
        goto send_answer_no_csap;
    }

    if (csap->state == CSAP_STATE_IDLE)
    {
        /* Just created CSAP */
        rc = TE_ETADCSAPSTATE;
    }
    else if (csap->state & CSAP_STATE_DONE)
    {
        /* send and/or receive is done */
        rc = 0;
    }
    else
    {
        /* send and/or receive is in progress */
        rc = TE_ETIMEDOUT;
    }

    if ((rc != TE_ETIMEDOUT) || (timeout == 0))
        goto send_answer_bad_state;

    rc = tad_reply_rcf_init(&reply_ctx, rcfc, cbuf, answer_plen);
    if (rc != 0)
        goto send_answer_fail_reply_ctx_init;

    rc = tad_poll_enqueue(csap, timeout, &reply_ctx);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to enqueue poll request: %r",
              CSAP_LOG_ARGS(csap));
        goto send_answer_fail_enqueue;
    }

    /* Poll context has its own copy */
    tad_reply_cleanup(&reply_ctx);

    return 0;

send_answer_fail_enqueue:
    tad_reply_cleanup(&reply_ctx);
send_answer_fail_reply_ctx_init:
send_answer_bad_state:
send_answer_no_csap:
    SEND_ANSWER("%u 0", TE_RC(TE_TAD_CH, rc));
    return 0;
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trpoll_cancel(struct rcf_comm_connection *rcfc,
                     char *cbuf, size_t buflen, size_t answer_plen,
                     csap_handle_t csap_id, unsigned int poll_id)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(csap_id);
    UNUSED(poll_id);

    return -1;
#else
    csap_p              csap;
    tad_poll_context   *p;
    te_errno            rc = TE_ENOENT;
    int                 ret;

    TAD_CHECK_INIT;

    if ((csap = csap_find(csap_id)) == NULL)
    {
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_LOCK(csap);
    LIST_FOREACH(p, &csap->poll_ops, links)
    {
        if (p->id == poll_id)
        {
            ret = pthread_cancel(p->thread);
            if (ret != 0)
            {
                rc = te_rc_os2te(ret);
            }
            else
            {
                rc = 0;
            }
            break;
        }
    }
    CSAP_UNLOCK(csap);

    SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
    return 0;

#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_param(struct rcf_comm_connection *rcfc,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap_id, const char *param)
{
#ifdef TAD_DUMMY
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("CSAP param: CSAP %u param <%s>\n", csap_id, param);
    return -1;
#else
    csap_p              csap = csap_find(csap_id);
    csap_layer_get_param_cb_t get_param_cb;
    int                 layer = csap == NULL ?
                                0 : csap->rw_layer;

    TAD_CHECK_INIT;
    VERB("CSAP param: CSAP %u param <%s>\n", csap_id, param);
    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
    }
    else if (strcmp(param, CSAP_PARAM_STATUS) == 0)
    {
        tad_csap_status_t status = 0;

        CSAP_LOCK(csap);
        if (csap->state & CSAP_STATE_IDLE)
        {
            status = CSAP_IDLE;
        }
        else if (csap->state & CSAP_STATE_DONE)
        {
            te_errno    rc;

            if (csap->state & CSAP_STATE_RECV)
                rc = csap_get_recv_context(csap)->status;
            else
                rc = csap_get_send_context(csap)->status;

            if (rc != 0)
                status = CSAP_ERROR;
            else
                status = CSAP_COMPLETED;
        }
        else
            status = CSAP_BUSY;

        F_VERB("CSAP get_param, state 0x%x, status %d\n",
               csap->state, (int)status);
        CSAP_UNLOCK(csap);

        SEND_ANSWER("0 %d", (int)status);
    }
    else if (strcmp(param, CSAP_PARAM_NO_MATCH_PKTS) == 0)
    {
        unsigned int no_match_pkts;

        no_match_pkts = csap_get_recv_context(csap)->no_match_pkts;

        VERB("CSAP get_param, get number of unmatched pkts %u\n",
             no_match_pkts);
        SEND_ANSWER("0 %u", no_match_pkts);
    }
    else if (strcmp(param, CSAP_PARAM_FIRST_PACKET_TIME) == 0)
    {
        VERB("CSAP get_param, get first pkt, %u.%u\n",
                                (uint32_t)csap->first_pkt.tv_sec,
                                (uint32_t)csap->first_pkt.tv_usec);
        SEND_ANSWER("0 %u.%u",  (uint32_t)csap->first_pkt.tv_sec,
                                (uint32_t)csap->first_pkt.tv_usec);
    }
    else if (strcmp(param, CSAP_PARAM_LAST_PACKET_TIME) == 0)
    {
        VERB("CSAP get_param, get last pkt, %u.%u\n",
                                (uint32_t)csap->last_pkt.tv_sec,
                                (uint32_t)csap->last_pkt.tv_usec);
        SEND_ANSWER("0 %u.%u",  (uint32_t)csap->last_pkt.tv_sec,
                                (uint32_t)csap->last_pkt.tv_usec);
    }
    else if ((get_param_cb =
                  csap_get_proto_support(csap, layer)->get_param_cb)
             == NULL)
    {
        VERB("CSAP does not support get_param\n");
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_EOPNOTSUPP));
    }
    else
    {
        char *param_value = get_param_cb(csap, layer, param);
        if (param_value)
        {
            VERB("got value: <%s>\n", param_value);
            SEND_ANSWER("0 %s", param_value);
            free(param_value);
        }
        else
        {
            VERB("CSAP return error for get_param\n");
            SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_EOPNOTSUPP));
        }
    }

    return 0;
#endif
}
