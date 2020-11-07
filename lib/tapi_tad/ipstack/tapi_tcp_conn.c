/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAPI TCP connection"

#include "te_config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif


#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/ether.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "te_queue.h"
#include "te_dbuf.h"
#include "rcf_api.h"
#include "conf_api.h"

#include "logger_api.h"
#include "te_sleep.h"
#include "te_time.h"

#include "tapi_tcp.h"
#include "tapi_tad.h"
#include "tapi_eth.h"
#include "tapi_arp.h"
#include "tapi_cfg.h"

#include "ndn_ipstack.h"
#include "ndn_eth.h"

/** Maximum TCP window size. */
#define MAX_TCP_WINDOW 65535

/** Default TCP window size. */
#define DEF_TCP_WINDOW MAX_TCP_WINDOW

#ifndef IFNAME_SIZE
#define IFNAME_SIZE 256
#endif

/*
 * ==================================================================
 * ------------------- TCP connections handlers ---------------------
 * ==================================================================
 */


/**
 * Structure for storage of received TCP messages, which
 * are not asked yet by TAPI user.
 */
typedef struct tapi_tcp_msg_queue_t {
    CIRCLEQ_ENTRY(tapi_tcp_msg_queue_t) link;   /**< Queue link. */

    uint8_t *data;          /**< Payload. */
    size_t   len;           /**< Payload length. */

    tapi_tcp_pos_t seqn;    /**< TCP SEQN. */
    tapi_tcp_pos_t ackn;    /**< TCP ACKN. */

    te_bool unexp_seqn;     /**< Was TCP SEQN unexpected? */

    uint8_t flags;          /**< TCP flags. */

} tapi_tcp_msg_queue_t;

/**
 * Structure for descriptor of TCP connection, handled by TAPI and TAD
 */
typedef struct tapi_tcp_connection_t {
    CIRCLEQ_ENTRY(tapi_tcp_connection_t) link;

    tapi_tcp_handler_t id;

    const char    *agt;
    int            arp_sid;
    int            rcv_sid;
    int            snd_sid;
    csap_handle_t  arp_csap;
    csap_handle_t  rcv_csap;
    csap_handle_t  snd_csap;

    char loc_iface[IFNAME_SIZE];

    uint8_t loc_mac[ETHER_ADDR_LEN];
    uint8_t rem_mac[ETHER_ADDR_LEN];

    struct sockaddr_storage loc_addr;
    struct sockaddr_storage rem_addr;

    te_tad_protocols_t ip_proto;

    int window;

    tapi_tcp_pos_t seq_got;
    tapi_tcp_pos_t ack_got;
    size_t         last_len_got;
    size_t         last_win_got;
    tapi_tcp_pos_t peer_isn;

    te_bool        ack_flag_got;
    te_bool        fin_got;
    te_bool        reset_got;

    tapi_tcp_pos_t seq_sent;
    tapi_tcp_pos_t ack_sent;
    tapi_tcp_pos_t our_isn;
    size_t         last_len_sent;

    te_bool got_exp_seqn; /**< Set to @c TRUE if a packet with
                               expected SEQN was got; used by
                               conn_wait_msg() and can be reset to
                               @c FALSE by it */

    te_bool        enabled_ts;        /**< Whether TCP timestamp is
                                           enabled. */
    te_bool        dst_enabled_ts;    /**< Whether peer enabled TCP
                                           timestamp. */
    uint32_t       last_ts_got;       /**< The last TCP timestamp value
                                           got from the peer. */
    uint32_t       ts_to_echo;        /**< Value to be echoed in TCP
                                           timestamp echo-reply field
                                           the next time. */
    uint32_t       last_ts_echo_got;  /**< The last TCP timestamp echo-reply
                                           value got from the peer. */
    uint32_t       last_ts;           /**< Last computed TCP timestamp. */
    uint32_t       last_ts_sent;      /**< Last TCP timestamp sent to
                                           peer. */
    te_bool        upd_ts_echo_sent;  /**< Whether last_ts_echo_sent field
                                           should be updated. */
    uint32_t       last_ts_echo_sent; /**< TCP timestamp echo-reply value
                                           sent in the last packet. */
    uint32_t       ts_start_value;    /**< Start value for TCP timestamp. */
    struct timeval ts_start_time;     /**< Moment of time when TCP timestamp
                                           timer started (timestamp value
                                           is increased by number of ms
                                           since this time). */
    te_bool        ts_timer_started;  /**< Will be set to @c TRUE once
                                           TCP timestamp timer starts
                                           (which happens after setting
                                            timestamp the first time). */

    CIRCLEQ_HEAD(tapi_tcp_msg_queue_head, tapi_tcp_msg_queue_t)
        *messages;
} tapi_tcp_connection_t;

/*
 * Declarations of local static methods, descriptions see
 * at their definitions.
 */

static inline tapi_tcp_pos_t conn_next_seq(tapi_tcp_connection_t *
                                           conn_descr);

static inline tapi_tcp_pos_t conn_next_ack(tapi_tcp_connection_t *
                                           conn_descr);
static inline int conn_update_sent_seq(tapi_tcp_connection_t *conn_descr,
                                       size_t new_sent_len);

static inline int conn_send_syn(tapi_tcp_connection_t *conn_descr);

static int conn_send_ack(tapi_tcp_connection_t *conn_descr,
                         tapi_tcp_pos_t ackn);

static void tcp_conn_pkt_handler(const char *pkt_file, void *user_param);

/* global fields */
CIRCLEQ_HEAD(tapi_tcp_conn_list_head, tapi_tcp_connection_t)
    *conns_root = NULL;


static inline void
tapi_tcp_conns_db_init(void)
{
    if (conns_root == NULL)
    {
        conns_root = calloc(1, sizeof(*conns_root));
        CIRCLEQ_INIT(conns_root);
    }
}



/**
 * Find TCP connection descriptor by handle
 *
 * @param handler       identifier of TCP connecion
 *
 * @return pointer to found descriptor or NULL if not found.
 */
static inline tapi_tcp_connection_t *
tapi_tcp_find_conn(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn;

    tapi_tcp_conns_db_init();

    for (conn = conns_root->cqh_first; conn != (void *) conns_root;
         conn = (void *) conn->link.cqe_next)
    {
        if (conn->id == handler)
            return conn;
    }
    return NULL;
}

/**
 * Add new TCP connection descriptor into databse and assign handler to it.
 * Descriptor structure should be already filled with TCP-related info.
 *
 * @param descr         pointer to descriptor to be inserted
 *
 * @return status code
 */
/* see description in tapi_tcp.h */
static inline int
tapi_tcp_insert_conn(tapi_tcp_connection_t *descr)
{
    tapi_tcp_conns_db_init();

    if (conns_root->cqh_last == (void *)conns_root)
        descr->id = 1;
    else
        descr->id = conns_root->cqh_last->id + 1;

    CIRCLEQ_INSERT_TAIL(conns_root, descr, link);

    return 0;
}


/**
 * Clear first (oldest) TCP message in queue, if it present.
 */
static inline void
tapi_tcp_clear_msg(tapi_tcp_connection_t *conn_descr)
{
    tapi_tcp_msg_queue_t *msg;

    if (conn_descr != NULL &&
        conn_descr->messages != NULL &&
        (msg = conn_descr->messages->cqh_first) !=
            (void *)conn_descr->messages)
    {
        VERB("%s() clear msg: seq %u, ack %u, len %d, flags %r",
             __FUNCTION__, msg->seqn, msg->ackn, msg->len,
             (int)msg->flags);
        free(msg->data);
        CIRCLEQ_REMOVE(conn_descr->messages, msg, link);
        free(msg);
    }
}

#ifdef ARP_IN_INIT_CON
static int destroy_arp_session(tapi_tcp_connection_t *conn_descr);
#endif

/**
 * Remove TCP connection descriptor from data base
 *
 * @param conn_descr    pointer to descriptor
 *
 * @return status code
 */
static inline int
tapi_tcp_destroy_conn_descr(tapi_tcp_connection_t *conn_descr)
{
    te_errno        rc;
    unsigned int    num;

    tapi_tcp_conns_db_init();

    if (conn_descr == NULL || conn_descr == (void *)conns_root)
        return 0;

    if (conn_descr->rcv_csap != CSAP_INVALID_HANDLE)
    {
        rc = rcf_ta_trrecv_stop(conn_descr->agt, conn_descr->rcv_sid,
                                conn_descr->rcv_csap,
                                tcp_conn_pkt_handler, conn_descr,
                                &num);
        if (rc != 0)
        {
            WARN("%s(conn %d): rcv CSAP %u on agt %s stop failed %r",
                 __FUNCTION__, conn_descr->id, conn_descr->rcv_csap,
                 conn_descr->agt, rc);
        }

        rc = rcf_ta_csap_destroy(conn_descr->agt, conn_descr->rcv_sid,
                                 conn_descr->rcv_csap);
        if (rc != 0)
        {
            WARN("%s(conn %d): rcv CSAP %u on agt %s destroy failed %r",
                 __FUNCTION__, conn_descr->id, conn_descr->rcv_csap,
                 conn_descr->agt, rc);
        }
        else
            INFO("%s(conn %d): rcv CSAP %u on agt %s destroyed",
                 __FUNCTION__, conn_descr->id, conn_descr->rcv_csap,
                 conn_descr->agt);
    }

    if (conn_descr->snd_csap != CSAP_INVALID_HANDLE)
    {
        rc = rcf_ta_csap_destroy(conn_descr->agt, conn_descr->snd_sid,
                                 conn_descr->snd_csap);
        if (rc != 0)
        {
            WARN("%s(conn %d): snd CSAP %u on agt %s destroy failed %r",
                 __FUNCTION__, conn_descr->id, conn_descr->snd_csap,
                 conn_descr->agt, rc);
        }
        else
            INFO("%s(conn %d): snd CSAP %u on agt %s destroyed",
                 __FUNCTION__, conn_descr->id, conn_descr->snd_csap,
                 conn_descr->agt);
    }

    if (conn_descr->rcv_csap != CSAP_INVALID_HANDLE ||
        conn_descr->snd_csap != CSAP_INVALID_HANDLE)
    {
        if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/csap:*",
                                      conn_descr->agt)) != 0)
            ERROR("%s(): cfg_synchronize_fmt(/agent:%s/csap:*) failed: %r",
                  __FUNCTION__, conn_descr->agt, rc);
    }

    #ifdef ARP_IN_INIT_CON
    destroy_arp_session(conn_descr);
    #endif

    if (conn_descr->messages != NULL)
        while (conn_descr->messages->cqh_first !=
               (void *)conn_descr->messages)
            tapi_tcp_clear_msg(conn_descr);

    free(conn_descr->messages);

    CIRCLEQ_REMOVE(conns_root, conn_descr, link);
    INFO("%s(conn %d) finished", __FUNCTION__, conn_descr->id);
    free(conn_descr);
    return 0;
}

/**
 * Wait for any packet in this connection until timeout is expired.
 *
 * @param [in]  conn_descr  Pointer to TAPI connection descriptor
 * @param [in]  timeout     Timeout in milliseconds
 * @param [out] duration    How much time in milliseconds it took to get the
 *                          packet, @c NULL can be used to ignore.
 *
 * @return Zero is returned on success (one or more messages got),
 *         otherwise - TE errno.
 */
static te_errno
conn_wait_packet(tapi_tcp_connection_t *conn_descr, unsigned int timeout,
                 unsigned int *duration)
{
    te_errno        rc;
    unsigned int    num = 0;
    unsigned int    sub = 0;
    struct timeval  tv1;
    struct timeval  tv2;
    te_bool tr_op_log = rcf_tr_op_log_get();

    if (conn_descr == NULL)
    {
        ERROR("%s: connection descriptor is NULL", __FUNCTION__);
        return TE_EINVAL;
    }

    RING("Waiting for a packet on the CSAP %u (%s:%d) timeout is %u "
         "milliseconds", conn_descr->rcv_csap, conn_descr->agt,
         conn_descr->rcv_sid, timeout);
    rcf_tr_op_log(FALSE);

    gettimeofday(&tv1, NULL);
    while (num == 0)
    {
        rc = rcf_ta_trrecv_get(conn_descr->agt, conn_descr->rcv_sid,
                               conn_descr->rcv_csap,
                               tcp_conn_pkt_handler, conn_descr, &num);
        if (rc != 0)
        {
            ERROR("%s: rcf_ta_trrecv_get() failed", __FUNCTION__);
            rcf_tr_op_log(tr_op_log);
            return rc;
        }

        gettimeofday(&tv2, NULL);
        sub = TIMEVAL_SUB(tv2, tv1) / 1000;
        if (sub >= timeout)
            break;

        if (num > 0)
            break;

        usleep(1000);
    }

    rcf_tr_op_log(tr_op_log);
    RING("The CSAP %u (%s:%d) got %u packets", conn_descr->rcv_csap,
         conn_descr->agt, conn_descr->rcv_sid, num);

    if (duration != NULL)
        *duration = sub;

    if (num == 0)
        return TE_RC(TE_TAPI, TE_ETIMEDOUT);

    return 0;
}

/**
 * Wait for new message in this connection ignoring retransmits and
 * out-of-order packets until timeout is expired.
 *
 * @param conn_descr    pointer to TAPI connection descriptor
 * @param timeout       timeout in milliseconds
 *
 * @return zero on success (one or more messages got), TE errno otherwise.
 */
static te_errno
conn_wait_msg(tapi_tcp_connection_t *conn_descr, unsigned int timeout)
{
    te_errno        rc;
    unsigned int    duration;

    conn_descr->got_exp_seqn = FALSE;

    /* Poll for packets in the loop during the @p timeout, ignore
     * retransmits. */
    while (TRUE)
    {
        rc = conn_wait_packet(conn_descr, timeout, &duration);
        if (rc != 0)
        {
            VERB("%s(): failed to get packet", __FUNCTION__);
            return rc;
        }

        if (conn_descr->got_exp_seqn)
            break;

        if (timeout <= duration)
        {
            VERB("%s: no new messages received", __FUNCTION__);
            return TE_RC(TE_TAPI, TE_ETIMEDOUT);
        }
        timeout -= duration;

        WARN("A packet with unexpected sequence number has been "
             "received, probably it is a retransmit - ignore it");
    }

    return 0;
}

int
tapi_tcp_wait_msg(tapi_tcp_handler_t handler, int timeout)
{
    tapi_tcp_connection_t *conn_descr;

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    /* It is simply wrapper for external use of static inline method. */
    return conn_wait_msg(conn_descr, timeout);
}



static inline tapi_tcp_msg_queue_t *
conn_get_oldest_msg(tapi_tcp_connection_t *conn_descr)
{
    if (conn_descr == NULL)
    {
        ERROR("%s: Connection descriptor is NULL", __FUNCTION__);
        return NULL;
    }

    if (conn_descr->messages == NULL)
    {
        ERROR("%s: Connection descriptor messages is NULL", __FUNCTION__);
        return NULL;
    }

    if (conn_descr->messages->cqh_first ==
            (void *)conn_descr->messages)
        return NULL;

    return conn_descr->messages->cqh_first;
}

/**
 * Create TCP packet ASN template.
 *
 * @param conn_descr    Connection descriptor.
 * @param seqn          TCP SEQN.
 * @param ackn          TCP ACKN.
 * @param syn_flag      Whether to set SYN flag.
 * @param ack_flag      Whether to set ACK flag.
 * @param data          Pointer to payload data or NULL.
 * @param pld_len       Payload length.
 * @param tmpl          Location for pointer to ASN value (OUT).
 *
 * @return Status code.
 */
static int
create_tcp_template(tapi_tcp_connection_t *conn_descr,
                    tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                    te_bool syn_flag, te_bool ack_flag,
                    uint8_t *data, size_t pld_len,
                    asn_value **tmpl)
{
    int rc;

    rc = tapi_tcp_template(conn_descr->ip_proto == TE_PROTO_IP6,
                           seqn, ackn, syn_flag, ack_flag, data,
                           pld_len, tmpl);
    if (rc != 0)
        return rc;

    rc = asn_write_int32(*tmpl, conn_descr->window,
                         "pdus.0.#tcp.win-size.#plain");
    if (rc != 0)
    {
        ERROR("%s(): failed to set TCP win-size, %r",
              __FUNCTION__, rc);
        asn_free_value(*tmpl);
        *tmpl = NULL;
        return rc;
    }

    return 0;
}

/**
 * Get current value of TCP timestamp.
 *
 * @param conn_descr      Connection descriptor.
 * @param ts              Where to save timestamp value.
 *
 * @return Status code.
 */
static te_errno
get_current_ts(tapi_tcp_connection_t *conn_descr, uint32_t *ts)
{
    struct timeval  tv;
    te_errno        rc;

    if (!conn_descr->ts_timer_started)
    {
        *ts = conn_descr->ts_start_value;
        return 0;
    }

    rc = te_gettimeofday(&tv, NULL);
    if (rc != 0)
        return rc;

    *ts = TE_US2MS(TIMEVAL_SUB(tv, conn_descr->ts_start_time)) +
          conn_descr->ts_start_value;
    return 0;
}

/**
 * Set TCP timestamp in a packet template.
 *
 * @param conn_descr      Connection descriptor.
 * @param pkt             Packet template.
 * @param syn_recvd       Whether @c SYN is already received.
 * @param ack             Whether @c ACK flag is set.
 * @param update_echo     If @c TRUE, timestamp echo-reply
 *                        should be updated.
 *
 * @return Status code.
 */
static te_errno
set_timestamp(tapi_tcp_connection_t *conn_descr, asn_value *pkt,
              te_bool syn_recvd, te_bool ack, te_bool update_echo)
{
    uint32_t  ts;
    te_errno  rc;
    uint32_t  ts_echo;

    conn_descr->upd_ts_echo_sent = FALSE;

    if (!conn_descr->enabled_ts)
        return 0;
    if (!conn_descr->dst_enabled_ts && syn_recvd)
        return 0;

    rc = get_current_ts(conn_descr, &ts);
    if (rc != 0)
        return rc;

    conn_descr->last_ts = ts;

    if (ack)
    {
        if (update_echo)
        {
            ts_echo = conn_descr->ts_to_echo;
            conn_descr->upd_ts_echo_sent = TRUE;
        }
        else
        {
            ts_echo = conn_descr->last_ts_echo_sent;
        }
    }
    else
    {
        ts_echo = 0;
    }

    return tapi_tcp_set_ts_opt(pkt, ts, ts_echo);
}

/**
 * Update last_ts_sent/last_ts_echo_sent fields after sending a packet.
 *
 * @param conn_descr      Connection descriptor.
 */
static void
update_last_ts(tapi_tcp_connection_t *conn_descr)
{
    te_errno          rc;
    struct timeval    tv;

    if (conn_descr->upd_ts_echo_sent)
    {
        conn_descr->last_ts_echo_sent = conn_descr->ts_to_echo;
        conn_descr->upd_ts_echo_sent = FALSE;
    }

    conn_descr->last_ts_sent = conn_descr->last_ts;

    if (!conn_descr->ts_timer_started)
    {
        rc = te_gettimeofday(&tv, NULL);
        if (rc == 0)
        {
            memcpy(&conn_descr->ts_start_time, &tv, sizeof(tv));
            conn_descr->ts_timer_started = TRUE;
        }
    }
}

/**
 * send SYN corresponding with connection descriptor.
 * If SYN was sent already, seq_sent will re-write, and SYN re-sent.
 */
static inline int
conn_send_syn(tapi_tcp_connection_t *conn_descr)
{
    asn_value *syn_template = NULL;
    int rc = 0;

#define CHECK_ERROR(msg_...)\
    do {                           \
        if (rc != 0)               \
        {                          \
            ERROR(msg_);           \
            goto cleanup;          \
        }                          \
    } while (0)

    conn_descr->seq_sent = conn_descr->our_isn;
    conn_descr->last_len_sent = 0;

    rc = create_tcp_template(conn_descr, conn_descr->our_isn, 0,
                             TRUE, FALSE,
                             NULL, 0, &syn_template);
    CHECK_ERROR("%s(): make syn template failed, rc %r",
                __FUNCTION__, rc);

    rc = set_timestamp(conn_descr, syn_template, FALSE, FALSE, FALSE);
    CHECK_ERROR("%s(): failed to set TCP timestamp, rc %r",
                __FUNCTION__, rc);

    rc = tapi_tad_trsend_start(conn_descr->agt, conn_descr->snd_sid,
                               conn_descr->snd_csap,
                               syn_template, RCF_MODE_BLOCKING);

    CHECK_ERROR("%s(): send SYN failed, rc %r",
                __FUNCTION__, rc);
    conn_update_sent_seq(conn_descr, 1);

    update_last_ts(conn_descr);

#undef CHECK_ERROR
cleanup:

    asn_free_value(syn_template);
    return rc;
}

/* handler for data, see description of 'rcf_pkt_handler' type */
static void
tcp_conn_pkt_handler(const char *pkt_file, void *user_param)
{
    tapi_tcp_connection_t *conn_descr = (tapi_tcp_connection_t *)user_param;
    asn_value             *tcp_message = NULL;
    const asn_value       *tcp_pdu;
    const asn_value       *ip_pdu;
    const asn_value       *val, *subval;
    tapi_tcp_msg_queue_t  *pkt;

    int         rc,
                syms;
    uint8_t     flags;
    int32_t     pdu_field;
    size_t      pld_len = 0;
    int         data_len;

    tapi_tcp_pos_t seq_got,
                   ack_got;
    te_tad_protocols_t ip_proto;

    /*
     * This handler DOES NOT check that got packet related to
     * connection, which descriptor passed as user data.
     */

    if (pkt_file == NULL || user_param == NULL)
    {
        WARN("%s(): receive strange arguments", __FUNCTION__);
        return;
    }
    ip_proto = conn_descr->ip_proto;
    if (ip_proto != TE_PROTO_IP4 && ip_proto != TE_PROTO_IP6)
    {
        WARN("%s(): bad IP protocol", __FUNCTION__);
        return;
    }
#define CHECK_ERROR(msg_) \
    do {                                                        \
        if (rc != 0)                                            \
        {                                                       \
            asn_free_value(tcp_message);                        \
            ERROR("%s(id %d): %s, rc %r", __FUNCTION__,       \
                  conn_descr->id, msg_, rc);                    \
            return;                                             \
        }                                                       \
    } while (0)

    rc = asn_parse_dvalue_in_file(pkt_file, ndn_raw_packet,
                                  &tcp_message, &syms);
    if (rc != 0)
    {
        ERROR("%s(): cannot parse message file: %r, sym %d",
              __FUNCTION__, rc, syms);
        return;
    }
    val = tcp_message;
    rc = asn_get_child_value(val, &subval, PRIVATE, NDN_PKT_PDUS);
    CHECK_ERROR("get pdus error");

    val = subval;

    rc = asn_get_indexed(val, (asn_value **)&subval, 1, NULL);
    CHECK_ERROR("get IP gen pdu error");
    rc = asn_get_choice_value(subval, (asn_value **)&ip_pdu, NULL, NULL);
    CHECK_ERROR("get IP special choice error");

    if (ip_proto == TE_PROTO_IP4)
    {
        rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_LEN, &pdu_field);
        CHECK_ERROR("read IPv4 total length error");
        data_len = pdu_field;
        rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_HLEN, &pdu_field);
        CHECK_ERROR("read IPv4 header length error");
        data_len -= (pdu_field << 2);
    }
    else
    {
        rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP6_LEN, &pdu_field);
        CHECK_ERROR("read IPv6 payload length error");
        data_len = pdu_field;
    }

    rc = asn_get_indexed(val, (asn_value **)&subval, 0, NULL);
    CHECK_ERROR("get TCP gen pdu error");
    rc = asn_get_choice_value(subval, (asn_value **)&tcp_pdu, NULL, NULL);
    CHECK_ERROR("get TCP special choice error");

    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_HLEN, &pdu_field);
    CHECK_ERROR("read TCP header length error");
    data_len -= (pdu_field << 2);

    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_FLAGS, &pdu_field);
    CHECK_ERROR("read TCP flag error");
    flags = pdu_field;

    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_SEQN, &pdu_field);
    CHECK_ERROR("read TCP seqn error");
    seq_got = pdu_field;

    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_ACKN, &pdu_field);
    CHECK_ERROR("read TCP ackn error");
    ack_got = pdu_field;

    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_WINDOW, &pdu_field);
    CHECK_ERROR("read TCP window error");
    conn_descr->last_win_got = pdu_field;

    pkt = calloc(1, sizeof(*pkt));

    if (conn_descr->seq_got + conn_descr->last_len_got == seq_got ||
        (conn_descr->peer_isn == 0 && (flags & TCP_SYN_FLAG)) ||
        /* SYN-SENT -> SYN-RECV, peer sends SYN, we send SYN instead
         * of acking, peer responds with SYN-ACK. */
        (conn_descr->seq_got == conn_descr->peer_isn &&
         (flags & TCP_SYN_FLAG) && (flags & TCP_ACK_FLAG)))
    {
        conn_descr->last_len_got = 0;
        conn_descr->seq_got = seq_got;

        if (flags & TCP_SYN_FLAG) /* this is SYN or SYN-ACK */
        {
            conn_descr->peer_isn = seq_got;
            conn_descr->last_len_got = pkt->len = 1;
        }

        if (flags & TCP_ACK_FLAG)
        {
            conn_descr->ack_got = ack_got;
            conn_descr->ack_flag_got = TRUE;
        }

        if (flags & TCP_FIN_FLAG)
        {
            conn_descr->fin_got = TRUE;
            conn_descr->last_len_got = pkt->len = 1;
        }

        if (data_len > 0)
            conn_descr->last_len_got = data_len;

        pkt->unexp_seqn = FALSE;
        conn_descr->got_exp_seqn = TRUE;
    }
    else
    {
        pkt->unexp_seqn = TRUE;
    }

    if (flags & TCP_RST_FLAG)
        conn_descr->reset_got = TRUE;

    if (conn_descr->enabled_ts)
    {
        uint32_t ts_got;
        uint32_t ts_echo_got;

        rc = tapi_tcp_get_ts_opt(tcp_pdu, &ts_got, &ts_echo_got);
        if (rc == 0)
        {
            conn_descr->last_ts_got = ts_got;
            conn_descr->last_ts_echo_got = ts_echo_got;
            /*
             * In ts_to_echo is stored value to be sent in
             * timestamp echo-reply field of the next packet.
             * So if there is nothing new to acknowledge, peer
             * timestamp is ignored here. See algorithm on page
             * 15 of RFC 1323.
             */
            if (flags & TCP_SYN_FLAG)
            {
                conn_descr->ts_to_echo = ts_got;
                conn_descr->dst_enabled_ts = TRUE;
            }
            else if (tapi_tcp_compare_seqn(conn_descr->ack_sent,
                                           seq_got) >= 0 &&
                     tapi_tcp_compare_seqn(conn_descr->ack_sent,
                                           seq_got +
                                           conn_descr->last_len_got) < 0)
            {
                conn_descr->ts_to_echo = ts_got;
            }
        }
        else if (conn_descr->dst_enabled_ts)
        {
            ERROR("Failed to get TCP timestamp from incoming packet: %r",
                  rc);
        }
    }

    if (conn_descr->messages == NULL)
    {
        conn_descr->messages =
            calloc(1, sizeof(*(conn_descr->messages)));
        CIRCLEQ_INIT(conn_descr->messages);
    }

    if (data_len > 0)
    {
        pkt->len = data_len;

        pkt->data = malloc(pld_len = data_len);
        rc = asn_read_value_field(tcp_message, pkt->data, &pld_len,
                                  "payload.#bytes");
        CHECK_ERROR("read TCP payload error");
        if ((int)pld_len < data_len)
        {
            WARN("Truncated TCP/IPv4 packet is received");
        }
    }

    pkt->flags = flags;
    pkt->seqn  = seq_got;
    pkt->ackn  = ack_got;

    CIRCLEQ_INSERT_TAIL(conn_descr->messages, pkt, link);

#undef CHECK_ERROR
    asn_free_value(tcp_message);

    INFO("%s(conn %d): seq got %u; len %d; ack %u, flags 0x%X",
         __FUNCTION__, conn_descr->id, seq_got,
         data_len, ack_got, flags);
}


#ifdef ARP_IN_INIT_CON

static
uint8_t broadcast_mac[] = {0xff,0xff,0xff,0xff,0xff,0xff};

static int
create_arp_session(tapi_tcp_connection_t *conn_descr,
                   asn_value *arp_pattern,
                   const struct sockaddr *local_addr,
                   const char *local_iface,
                   const uint8_t *local_mac,
                   const uint8_t *remote_mac,
                   te_bool  use_native_mac)
{
    int rc = 0;
    int arp_sid;
    char arp_reply_method[100];
    csap_handle_t arp_csap = CSAP_INVALID_HANDLE;
    uint16_t trafic_param;
    size_t   func_len;

    struct sockaddr_in const *const local_in_addr  = SIN(local_addr);

#define CHECK_ERROR(msg_...)       \
    do {                             \
        if (rc != 0)                 \
        {                          \
            ERROR(msg_);           \
            goto proc_exit;          \
        }                          \
    } while (0)


    rc = rcf_ta_create_session(conn_descr->agt, &arp_sid);
    CHECK_ERROR("%s(); create snd session failed %r",
                __FUNCTION__, rc);

    trafic_param = 1;
    rc = tapi_arp_add_pdu_eth_ip4(&arp_pattern, TRUE,
                                  &trafic_param, remote_mac, NULL, NULL,
                                  (uint8_t *)&(local_in_addr->sin_addr));
    CHECK_ERROR("%s(); create arp pattern fails %r", __FUNCTION__, rc);
    rc = tapi_eth_add_pdu(&arp_pattern, NULL, TRUE,
                          broadcast_mac, remote_mac, NULL,
                          TE_BOOL3_ANY /* tagged/untagged */,
                          TE_BOOL3_ANY /* Ethernet2/LLC */);

    CHECK_ERROR("%s(); create arp/eth pattern fails %r",
                __FUNCTION__, rc);

    func_len = snprintf(arp_reply_method, sizeof(arp_reply_method),
                  "tad_eth_arp_reply:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                  local_mac[0], local_mac[1],
                  local_mac[2], local_mac[3],
                  local_mac[4], local_mac[5]);

    rc = asn_write_value_field(arp_pattern, arp_reply_method, func_len + 1,
                               "0.actions.0.#function");
    CHECK_ERROR("%s(): write arp reply method name failed %r",
                __FUNCTION__, rc);


    trafic_param = ETH_P_ARP;

    rc = tapi_arp_eth_csap_create_ip4(conn_descr->agt, arp_sid, local_iface,
                                      use_native_mac ?
                                      TAD_ETH_RECV_HOST |
                                      TAD_ETH_RECV_BCAST
                                      : TAD_ETH_RECV_DEF,
                                      remote_mac, NULL, &arp_csap);
    CHECK_ERROR("%s(): create arp csap fails %r", __FUNCTION__, rc);

    INFO("%s(): created arp csap: %d", __FUNCTION__, arp_csap);


    conn_descr->arp_csap = arp_csap;
    conn_descr->arp_sid = arp_sid;

    #undef CHECK_ERROR

proc_exit:
    return rc;
}


static int
destroy_arp_session(tapi_tcp_connection_t *conn_descr)
{
    int rc = 0;
    unsigned int num;
    if (conn_descr->arp_csap != CSAP_INVALID_HANDLE)
    {
        rc = rcf_ta_trrecv_stop(conn_descr->agt, conn_descr->arp_sid,
                                conn_descr->arp_csap,
                                tcp_conn_pkt_handler, conn_descr,
                                &num);
        if (rc != 0)
        {
            WARN("%s(id %d): arp CSAP %u on agt %s stop failed %r",
                 __FUNCTION__, conn_descr->id, conn_descr->arp_csap,
                 conn_descr->agt, rc);
        }

        rc = rcf_ta_csap_destroy(conn_descr->agt, conn_descr->arp_sid,
                                 conn_descr->arp_csap);
        if (rc != 0)
        {
            WARN("%s(id %d): arp CSAP %u on agt %s destroy failed %r",
                 __FUNCTION__, conn_descr->id, conn_descr->arp_csap,
                 conn_descr->agt, rc);
        }
        else
            INFO("%s(conn %d): arp CSAP %u on agt %s destroyed",
                 __FUNCTION__, conn_descr->id, conn_descr->arp_csap,
                 conn_descr->agt);

        if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/csap:*",
                                      conn_descr->agt)) != 0)
            ERROR("%s(): cfg_synchronize_fmt(/agent:%s/csap:*) failed: %r",
                  __FUNCTION__, conn_descr->agt, rc);
    }

    return rc;
}

#endif

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_create_conn(const char *agt,
                     const struct sockaddr *local_addr,
                     const struct sockaddr *remote_addr,
                     const char *local_iface,
                     const uint8_t *local_mac,
                     const uint8_t *remote_mac,
                     int window,
                     tapi_tcp_handler_t *handler)
{
    int rc;
    int syms;
    int rcv_sid;
    int snd_sid;

    te_bool  use_native_mac = FALSE;
    uint8_t  native_local_mac[6];
    size_t   mac_len = sizeof(native_local_mac);

    asn_value *syn_pattern = NULL;
    asn_value *arp_pattern = NULL;

    csap_handle_t rcv_csap = CSAP_INVALID_HANDLE;
    csap_handle_t snd_csap = CSAP_INVALID_HANDLE;

    tapi_tcp_connection_t *conn_descr = NULL;
    sa_family_t            sa_family;

    tapi_tcp_conns_db_init();

    if (agt == NULL || local_addr == NULL || remote_addr == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    sa_family = local_addr->sa_family;
    if ((sa_family != AF_INET && sa_family != AF_INET6)
        || sa_family != remote_addr->sa_family)
    {
        ERROR("Invalid local and/or remote address value");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /*
     * TODO: Make automatic investigation of local interface and
     * MAC addresses.
     */
    if (local_iface == NULL || local_mac == NULL || remote_mac == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    if (window == TAPI_TCP_DEF_WINDOW)
    {
        window = DEF_TCP_WINDOW;
    }
    else if (window == TAPI_TCP_ZERO_WINDOW)
    {
        window = 0;
    }
    else if (window < 0 || window > MAX_TCP_WINDOW)
    {
        ERROR("Invalid TCP window size %d was specified", window);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

#define CHECK_ERROR(msg_...)\
    do {                           \
        if (rc != 0)                 \
        {                          \
            ERROR(msg_);           \
            goto cleanup;          \
        }                          \
    } while (0)
    rc = tapi_cfg_get_hwaddr(agt, local_iface, native_local_mac, &mac_len);
    if (rc != 0)
    {
        WARN("%s(); get local native MAC failed %r",
             __FUNCTION__, rc);
        use_native_mac = FALSE;
    }
    else
    {
        use_native_mac = !memcmp(native_local_mac, local_mac, mac_len);
    }
    if (use_native_mac)
        RING("%s(): use native MAC on interface, may be side effects",
             __FUNCTION__);

    rc = rcf_ta_create_session(agt, &rcv_sid);
    CHECK_ERROR("%s(); create rcv session failed %r",
                __FUNCTION__, rc);

    rc = rcf_ta_create_session(agt, &snd_sid);
    CHECK_ERROR("%s(); create snd session failed %r",
                __FUNCTION__, rc);

    if (sa_family == AF_INET)
    {
        struct sockaddr_in const *const local_in_addr  = SIN(local_addr);
        struct sockaddr_in const *const remote_in_addr = SIN(remote_addr);

        rc = tapi_tcp_ip4_eth_csap_create(agt, rcv_sid, local_iface,
                                          use_native_mac ?
                                              TAD_ETH_RECV_HOST
                                            : TAD_ETH_RECV_DEF,
                                          local_mac, remote_mac,
                                          local_in_addr->sin_addr.s_addr,
                                          remote_in_addr->sin_addr.s_addr,
                                          local_in_addr->sin_port,
                                          remote_in_addr->sin_port,
                                          &rcv_csap);
        CHECK_ERROR("%s(): rcv csap create failed %r",
                    __FUNCTION__, rc);

        rc = tapi_tcp_ip4_eth_csap_create(agt, snd_sid, local_iface,
                                          TAD_ETH_RECV_HOST,
                                          local_mac, remote_mac,
                                          local_in_addr->sin_addr.s_addr,
                                          remote_in_addr->sin_addr.s_addr,
                                          local_in_addr->sin_port,
                                          remote_in_addr->sin_port,
                                          &snd_csap);
        CHECK_ERROR("%s(): snd csap create failed %r",
                    __FUNCTION__, rc);
    }
    else
    {
        struct sockaddr_in6 const *const local_in_addr  = SIN6(local_addr);
        struct sockaddr_in6 const *const remote_in_addr = SIN6(remote_addr);

        rc = tapi_tcp_ip6_eth_csap_create(agt, rcv_sid, local_iface,
                                          use_native_mac ?
                                              TAD_ETH_RECV_HOST
                                            : TAD_ETH_RECV_DEF,
                                          local_mac, remote_mac,
                                          local_in_addr->sin6_addr.s6_addr,
                                          remote_in_addr->sin6_addr.s6_addr,
                                          local_in_addr->sin6_port,
                                          remote_in_addr->sin6_port,
                                          &rcv_csap);
        CHECK_ERROR("%s(): rcv csap create failed %r",
                    __FUNCTION__, rc);

        rc = tapi_tcp_ip6_eth_csap_create(agt, snd_sid, local_iface,
                                          TAD_ETH_RECV_HOST,
                                          local_mac, remote_mac,
                                          local_in_addr->sin6_addr.s6_addr,
                                          remote_in_addr->sin6_addr.s6_addr,
                                          local_in_addr->sin6_port,
                                          remote_in_addr->sin6_port,
                                          &snd_csap);
        CHECK_ERROR("%s(): snd csap create failed %r",
                    __FUNCTION__, rc);

    }

    conn_descr = calloc(1, sizeof(*conn_descr));

    conn_descr->rcv_csap = rcv_csap;
    conn_descr->rcv_sid = rcv_sid;
    conn_descr->snd_csap = snd_csap;
    conn_descr->snd_sid = snd_sid;
    conn_descr->agt = agt;
    conn_descr->our_isn = rand();
    conn_descr->window = window;

    if (sa_family == AF_INET)
    {
        conn_descr->arp_csap = CSAP_INVALID_HANDLE;
#ifdef ARP_IN_INIT_CON
        rc = create_arp_session(conn_descr, arp_pattern, local_addr,
                                local_iface, local_mac,
                                remote_mac, use_native_mac);
        CHECK_ERROR("%s: fail to create arp session %r", __FUNCTION__, rc);
#endif
        conn_descr->ip_proto = TE_PROTO_IP4;
    }
    else
    {
        conn_descr->arp_csap = CSAP_INVALID_HANDLE;
        conn_descr->ip_proto = TE_PROTO_IP6;
    }

    tapi_tcp_insert_conn(conn_descr);
    *handler = conn_descr->id;

    INFO("%s(): init TCP connection started, id %d, our ISN %u",
         __FUNCTION__, conn_descr->id, conn_descr->our_isn);

    if (sa_family == AF_INET)
    {
        /* make template for this connection */
        rc = asn_parse_value_text("{{pdus {tcp:{}, ip4:{}, eth:{}}}}",
                                  ndn_traffic_pattern,
                                  &syn_pattern, &syms);
        CHECK_ERROR("%s(): parse pattern failed, rc %r, sym %d",
                    __FUNCTION__, rc, syms);

#ifdef ARP_IN_INIT_CON

        /* start catch our ARP */
        rc = tapi_tad_trrecv_start(agt, conn_descr->arp_sid,
                                   conn_descr->arp_csap, arp_pattern,
                                   TAD_TIMEOUT_INF, 0, RCF_TRRECV_COUNT);
        CHECK_ERROR("%s(): failed for arp_csap %r", __FUNCTION__, rc);

#endif
    }
    else
    {
        rc = asn_parse_value_text("{{pdus {tcp:{}, ip6:{}, eth:{}}}}",
                                  ndn_traffic_pattern,
                                  &syn_pattern, &syms);
        CHECK_ERROR("%s(): parse pattern failed, rc %r, sym %d",
                    __FUNCTION__, rc, syms);
    }

    rc = tapi_tad_trrecv_start(agt, conn_descr->rcv_sid,
                               conn_descr->rcv_csap, syn_pattern,
                               TAD_TIMEOUT_INF, 0, RCF_TRRECV_PACKETS);
    CHECK_ERROR("%s(): failed for rcv_csap %r", __FUNCTION__, rc);

#undef CHECK_ERROR

cleanup:
    if (rc != 0)
    {
        tapi_tcp_destroy_conn_descr(conn_descr);
        *handler = 0;
    }
    if (sa_family == AF_INET)
        asn_free_value(arp_pattern);
    asn_free_value(syn_pattern);
    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_start_conn(tapi_tcp_handler_t handler,
                    tapi_tcp_mode_t mode)
{
    tapi_tcp_connection_t *conn_descr;

    if (mode != TAPI_TCP_CLIENT)
        return 0;

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return conn_send_syn(conn_descr);
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_init_connection(const char *agt, tapi_tcp_mode_t mode,
                         const struct sockaddr *local_addr,
                         const struct sockaddr *remote_addr,
                         const char *local_iface,
                         const uint8_t *local_mac,
                         const uint8_t *remote_mac,
                         int window, tapi_tcp_handler_t *handler)
{
    te_errno rc;

    rc = tapi_tcp_create_conn(agt, local_addr, remote_addr,
                              local_iface, local_mac, remote_mac,
                              window, handler);
    if (rc != 0)
        return rc;

    return tapi_tcp_start_conn(*handler, mode);
}

/* See description in tapi_tcp.h */
int
tapi_tcp_wait_open(tapi_tcp_handler_t handler, int timeout)
{
    int rc;

    te_bool    is_server = FALSE;

    asn_value *syn_ack_template = NULL;

    tapi_tcp_connection_t *conn_descr;
    tapi_tcp_msg_queue_t  *msg = NULL;

#define CHECK_ERROR(msg_...)\
    do {                                        \
        if (rc != 0)                            \
        {                                       \
            ERROR(msg_);                        \
            asn_free_value(syn_ack_template);   \
            goto cleanup;                       \
        }                                       \
    } while (0)


    tapi_tcp_conns_db_init();
    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
    {
        ERROR("%s(): failed to find connection descriptor",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (conn_descr->seq_sent == 0) /* have no SYN sent yet */
    {
        is_server = TRUE;
        conn_descr->seq_sent = conn_descr->our_isn;
    }

    /* Wait for SYN or SYN-ACK if it is not received yet. */
    if (conn_descr->peer_isn == 0)
    {
        rc = conn_wait_msg(conn_descr, timeout);
        if (rc == TE_ETIMEDOUT && !is_server)
        {
            INFO("%s(): re-send SYN", __FUNCTION__);
            conn_send_syn(conn_descr);
            rc = conn_wait_msg(conn_descr, timeout);

            if (rc == TE_ETIMEDOUT)
            {
                INFO("%s(): re-send SYN again", __FUNCTION__);
                conn_send_syn(conn_descr);
                rc = conn_wait_msg(conn_descr, timeout);
            }
        }

        CHECK_ERROR("%s(): wait for SYN of SYN-ACK failed, rc %r",
                    __FUNCTION__, rc);
    }

    msg = conn_get_oldest_msg(conn_descr);
    if (msg == NULL || conn_descr->peer_isn == 0)
    {
        ERROR("%s(id %d): get SYN of SYN-ACK from queue failed",
                __FUNCTION__, conn_descr->id);
        rc = TE_ETIMEDOUT;
        goto cleanup;
    }

    /* Send ACK or SYN-ACK*/

    conn_descr->ack_sent = conn_next_ack(conn_descr);
    rc = create_tcp_template(conn_descr, conn_next_seq(conn_descr),
                             conn_descr->ack_sent,
                             is_server,
                             TRUE, NULL, 0, &syn_ack_template);
    CHECK_ERROR("%s(): make SYN-ACK template failed, rc %r",
                __FUNCTION__, rc);

    rc = set_timestamp(conn_descr, syn_ack_template, TRUE, TRUE, TRUE);
    CHECK_ERROR("%s(): failed to set timestamp in SYN-ACK, rc %r",
                __FUNCTION__, rc);

    rc = tapi_tad_trsend_start(conn_descr->agt,
                               conn_descr->snd_sid, conn_descr->snd_csap,
                               syn_ack_template, RCF_MODE_BLOCKING);

    CHECK_ERROR("%s(): send ACK or SYN-ACK failed, rc %r",
                __FUNCTION__, rc);

    asn_free_value(syn_ack_template);
    syn_ack_template = NULL;

    update_last_ts(conn_descr);

    if (is_server)
        conn_update_sent_seq(conn_descr, 1);

    if (!conn_descr->ack_flag_got ||
        conn_descr->ack_got != conn_descr->our_isn + 1)
    {
        /* wait for ACK */
        rc = conn_wait_msg(conn_descr, timeout);

        CHECK_ERROR("%s(): wait for ACK failed, rc %r",
                    __FUNCTION__, rc);
        tapi_tcp_clear_msg(conn_descr);
    }

    /* check if we got ACK for our SYN */
    if (!conn_descr->ack_flag_got ||
        conn_descr->ack_got != conn_descr->our_isn + 1)
    {
        ERROR("%s(id %d): ACK for our SYN dont got",
                __FUNCTION__, conn_descr->id);
        rc = TE_ETIMEDOUT;
        goto cleanup;
    }

    tapi_tcp_clear_msg(conn_descr);

    return 0;
#undef CHECK_ERROR

cleanup:
    ERROR("%s() failed", __FUNCTION__);
    tapi_tcp_destroy_conn_descr(conn_descr);
    return TE_RC(TE_TAPI, rc);
}

static int
tapi_tcp_send_fin_gen(tapi_tcp_handler_t handler, int timeout,
                      te_bool fin_ack)
{
    tapi_tcp_connection_t *conn_descr;
    asn_value             *fin_template = NULL;
    tapi_tcp_pos_t         new_ackn;

    unsigned int    num;
    te_errno        rc;
    uint8_t         flags;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    /* try to get messages and peer FIN, if they were sent */
    rcf_ta_trrecv_get(conn_descr->agt, conn_descr->rcv_sid,
                      conn_descr->rcv_csap,
                      tcp_conn_pkt_handler, conn_descr, &num);

    if (fin_ack)
        new_ackn = conn_next_ack(conn_descr);
    else
        new_ackn = conn_descr->ack_sent;

    INFO("%s(conn %d) new ack %u", __FUNCTION__, handler, new_ackn);
    rc = create_tcp_template(conn_descr, conn_next_seq(conn_descr), new_ackn,
                             FALSE, TRUE,
                             NULL, 0, &fin_template);
    if (rc != 0)
    {
        ERROR("%s(): failed to create FIN template, %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    flags = TCP_FIN_FLAG | TCP_ACK_FLAG;
    rc = asn_write_int32(fin_template, flags, "pdus.0.#tcp.flags.#plain");
    if (rc != 0)
    {
        ERROR("%s(): set fin flag failed %r",
              __FUNCTION__, rc);
        asn_free_value(fin_template);
        return TE_RC(TE_TAPI, rc);
    }

    rc = set_timestamp(conn_descr, fin_template, TRUE, TRUE,
                       !(new_ackn == conn_descr->ack_sent));
    if (rc != 0)
    {
        ERROR("%s(): failed to set TCP timestamp, rc %r",
              __FUNCTION__, rc);
        asn_free_value(fin_template);
        return TE_RC(TE_TAPI, rc);
    }

    rc = tapi_tad_trsend_start(conn_descr->agt, conn_descr->snd_sid,
                               conn_descr->snd_csap,
                               fin_template, RCF_MODE_BLOCKING);
    asn_free_value(fin_template);
    if (rc != 0)
    {
        ERROR("%s(): send FIN failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    update_last_ts(conn_descr);

#if FIN_ACK
    conn_descr->ack_sent = new_ackn;
#endif
    conn_update_sent_seq(conn_descr, 1);

    INFO("fin sent");
    rcf_ta_trrecv_get(conn_descr->agt, conn_descr->rcv_sid,
                      conn_descr->rcv_csap,
                      tcp_conn_pkt_handler, conn_descr, &num);
    if (conn_descr->ack_got != conn_descr->seq_sent + 1)
    {
        if (conn_descr->reset_got)
        {
            INFO("%s(conn %d) got reset", __FUNCTION__, handler);
        }
        else
        {
            conn_wait_msg(conn_descr, timeout);
            if (conn_descr->ack_got != conn_descr->seq_sent + 1)
            {
                WARN("%s(conn %d): wait ACK for our FIN timed out",
                     __FUNCTION__, handler);
                return TE_RC(TE_TAPI, TE_ETIMEDOUT);
            }
        }
    }

    return 0;
}

int
tapi_tcp_send_fin(tapi_tcp_handler_t handler, int timeout)
{
    return tapi_tcp_send_fin_gen(handler, timeout, FALSE);
}

int
tapi_tcp_send_fin_ack(tapi_tcp_handler_t handler, int timeout)
{
    return tapi_tcp_send_fin_gen(handler, timeout, TRUE);
}

int
tapi_tcp_send_rst(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;
    asn_value             *rst_template = NULL;
    tapi_tcp_pos_t         new_ackn;

    int     rc;
    uint8_t flags;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((new_ackn = conn_descr->ack_sent) == 0)
    {
        /* There was no any ACK sent yet, dont need save new ack
         *  connection will be broken by this method. */
        new_ackn = conn_descr->peer_isn + 1;
    }

    INFO("%s(conn %d) seq %d, new ack %u",
         __FUNCTION__, handler, conn_next_seq(conn_descr), new_ackn);

    rc = create_tcp_template(conn_descr, conn_next_seq(conn_descr),
                             new_ackn, FALSE, TRUE, NULL, 0,
                             &rst_template);
    if (rc != 0)
    {
        ERROR("%s(): failed to create RST template, %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    flags = TCP_RST_FLAG | TCP_ACK_FLAG;
    rc = asn_write_int32(rst_template, flags, "pdus.0.#tcp.flags.#plain");
    if (rc != 0)
    {
        ERROR("%s(): set RST flag failed %r",
              __FUNCTION__, rc);
        asn_free_value(rst_template);
        return TE_RC(TE_TAPI, rc);
    }

    rc = tapi_tad_trsend_start(conn_descr->agt, conn_descr->snd_sid,
                               conn_descr->snd_csap,
                               rst_template, RCF_MODE_BLOCKING);
    asn_free_value(rst_template);
    if (rc != 0)
    {
        ERROR("%s(): send RST failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }
    return 0;
}

int
tapi_tcp_destroy_connection(tapi_tcp_handler_t handler)
{
    int rc = 0;
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = tapi_tcp_destroy_conn_descr(conn_descr);

    if (rc != 0)
        WARN("$s(conn %d) destroy connection failed %r",
             __FUNCTION__, handler, rc);

    return rc;
}

int
tapi_tcp_send_template(tapi_tcp_handler_t handler,
                       const asn_value *template,
                       rcf_call_mode_t blk_mode)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();
    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return tapi_tad_trsend_start(conn_descr->agt, conn_descr->snd_sid,
                                 conn_descr->snd_csap,
                                 template, blk_mode);
}

int
tapi_tcp_send_msg(tapi_tcp_handler_t handler, uint8_t *payload, size_t len,
                  tapi_tcp_protocol_mode_t seq_mode,
                  tapi_tcp_pos_t seqn,
                  tapi_tcp_protocol_mode_t ack_mode,
                  tapi_tcp_pos_t ackn,
                  tapi_ip_frag_spec *frags, size_t frag_num)
{
    tapi_tcp_connection_t *conn_descr;

    asn_value *msg_template = NULL;
    asn_value *ip_pdu;
    te_bool    ipv4;
    int        rc;

    tapi_tcp_pos_t new_seq = 0;
    tapi_tcp_pos_t new_ack = 0;
    te_bool        ack_flag = FALSE;

    tapi_tcp_conns_db_init();
    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if (conn_descr->ip_proto == TE_PROTO_IP6)
        ipv4 = FALSE;
    else
        ipv4 = TRUE;

    switch (seq_mode)
    {
        case TAPI_TCP_AUTO:
            new_seq = conn_next_seq(conn_descr);
            break;

        case TAPI_TCP_EXPLICIT:
            new_seq = seqn;
            break;

        default:
            return TE_EINVAL;
    }

    switch (ack_mode)
    {
        case TAPI_TCP_EXPLICIT:
            new_ack = ackn;
            ack_flag = TRUE;
            break;

        case TAPI_TCP_QUIET:
            new_ack = 0;
            ack_flag = FALSE;
            break;

        case TAPI_TCP_AUTO:
            /* make very simple ack: last sent one. */
            new_ack = conn_descr->ack_sent;
            ack_flag = TRUE;
            break;

        default:
            return TE_EINVAL;
    }

    rc = create_tcp_template(conn_descr, new_seq, new_ack,
                             FALSE, ack_flag, payload, len,
                             &msg_template);
    if (rc != 0)
    {
        ERROR("%s: make msg template error %r", __FUNCTION__, rc);
        return rc;
    }

    rc = set_timestamp(conn_descr, msg_template, TRUE, ack_flag,
                       (tapi_tcp_compare_seqn(new_ack,
                                              conn_descr->ack_sent) > 0));
    if (rc != 0)
    {
        ERROR("%s(): failed to set TCP timestamp, rc %r",
              __FUNCTION__, rc);
        asn_free_value(msg_template);
        return TE_RC(TE_TAPI, rc);
    }

    if (frags != NULL)
    {
        ip_pdu = asn_find_descendant(msg_template, &rc, "pdus.1.#ip%d",
                                     (ipv4 ? 4 : 6));
        if (ip_pdu == NULL)
        {
            ERROR("Failed to get IP PDU from template: %r", rc);
            asn_free_value(msg_template);
            return rc;
        }
        rc = tapi_ip_pdu_tmpl_fragments(NULL, &ip_pdu, ipv4,
                                        frags, frag_num);
        if (rc != 0)
        {
            ERROR("Failed to add fragments specification in IPv4 PDU "
                  "template: %r", rc);
            asn_free_value(msg_template);
            return rc;
        }
    }

    rc = tapi_tad_trsend_start(conn_descr->agt, conn_descr->snd_sid,
                               conn_descr->snd_csap,
                               msg_template, RCF_MODE_BLOCKING);
    asn_free_value(msg_template);
    if (rc != 0)
    {
        ERROR("%s: send msg %r", __FUNCTION__, rc);
    }
    else
    {
        INFO("%s(conn %d) sent msg %d bytes, %u seq, %u ack",
             __FUNCTION__, handler, len, new_seq, new_ack);
        if (ack_flag)
            conn_descr->ack_sent = new_ack;

        if (seq_mode == TAPI_TCP_AUTO)
            conn_update_sent_seq(conn_descr, len);

        update_last_ts(conn_descr);
    }
    return rc;
}

/**
 * Get next TCP message in queue (ignored messages will be
 * released).
 *
 * @param conn_descr        TCP connection descriptor.
 * @param timeout           Timeout (milliseconds).
 * @param no_unexp_seqn     Ignore packets with unexpected TCP SEQN.
 *
 * @return TCP message or NULL.
 */
static tapi_tcp_msg_queue_t *
conn_get_next_msg(tapi_tcp_connection_t *conn_descr,
                  int timeout, te_bool no_unexp_seqn)
{
    tapi_tcp_msg_queue_t  *msg = NULL;

    te_bool wait_done = FALSE;

    while (TRUE)
    {
        msg = conn_get_oldest_msg(conn_descr);
        if (msg == NULL)
        {
            if (wait_done)
                break;

            if (no_unexp_seqn)
                conn_wait_msg(conn_descr, timeout);
            else
                conn_wait_packet(conn_descr, timeout, NULL);

            wait_done = TRUE;
            continue;
        }

        if (no_unexp_seqn && msg->unexp_seqn)
            tapi_tcp_clear_msg(conn_descr);
        else
            return msg;
    }

    return NULL;
}

/* See description in tapi_tcp.h */
int
tapi_tcp_recv_msg_gen(tapi_tcp_handler_t handler, int timeout,
                      tapi_tcp_protocol_mode_t ack_mode,
                      uint8_t *buffer, size_t *len,
                      tapi_tcp_pos_t *seqn_got,
                      tapi_tcp_pos_t *ackn_got,
                      uint8_t *flags, te_bool no_unexp_seqn)
{
    tapi_tcp_msg_queue_t  *msg;
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    msg = conn_get_next_msg(conn_descr, timeout, no_unexp_seqn);

    if (msg != NULL)
    {
        if (msg->data == NULL || buffer == NULL)
        {
            if (len != NULL)
                *len = 0;
        }
        else if (*len >= msg->len)
        {
            memcpy(buffer, msg->data, msg->len);
            *len = msg->len;
        }
        else
        {
            ERROR("TCP message has %"TE_PRINTF_SIZE_T"u bytes, but "
                  "supplied buffer has size of only "
                  "%"TE_PRINTF_SIZE_T"u bytes",
                  msg->len, *len);
            return TE_RC(TE_TAPI, TE_ENOBUFS);
        }

        if (seqn_got != NULL)
            *seqn_got = msg->seqn;
        if (ackn_got != NULL)
            *ackn_got = msg->ackn;
        if (flags != NULL)
            *flags = msg->flags;

        INFO("%s(conn %d): msg with seq %u, ack %u, len %d, flags 0x%X",
             __FUNCTION__, handler,
             msg->seqn, msg->ackn, msg->len, msg->flags);
        if (ack_mode == TAPI_TCP_AUTO)
        {
            if (msg->len == 0)
                INFO("%s(conn %d): do not send ACK to msg with zero len",
                     __FUNCTION__, handler);
            else
                conn_send_ack(conn_descr, msg->seqn + msg->len);
        }
        tapi_tcp_clear_msg(conn_descr);
    }
    else
    {
        WARN("%s(id %d) no message got", __FUNCTION__, handler);
        return TE_RC(TE_TAPI, TE_ETIMEDOUT);
    }

    return 0;
}

/* See description in tapi_tcp.h */
int
tapi_tcp_recv_msg(tapi_tcp_handler_t handler, int timeout,
                  tapi_tcp_protocol_mode_t ack_mode,
                  uint8_t *buffer, size_t *len,
                  tapi_tcp_pos_t *seqn_got,
                  tapi_tcp_pos_t *ackn_got,
                  uint8_t *flags)
{
    return tapi_tcp_recv_msg_gen(handler, timeout, ack_mode,
                                 buffer, len, seqn_got, ackn_got, flags,
                                 FALSE);
}

/* See description in tapi_tcp.h */
int
tapi_tcp_recv_data(tapi_tcp_handler_t handler, int time2wait,
                   tapi_tcp_protocol_mode_t ack_mode,
                   te_dbuf *data)
{
    tapi_tcp_msg_queue_t  *msg = NULL;
    tapi_tcp_connection_t *conn_descr = NULL;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    while (TRUE)
    {
        msg = conn_get_next_msg(conn_descr, time2wait, TRUE);
        if (msg == NULL)
            break;

        if (msg->data != NULL)
            te_dbuf_append(data, msg->data, msg->len);

        if (ack_mode == TAPI_TCP_AUTO && msg->len > 0)
            conn_send_ack(conn_descr, msg->seqn + msg->len);

        tapi_tcp_clear_msg(conn_descr);
    }

    return 0;
}

/**
 * Send ACK.
 *
 * @param conn_descr    TCP connection descriptor.
 * @param ackn          ACK number.
 *
 * @return Status code.
 */
static int
conn_send_ack(tapi_tcp_connection_t *conn_descr, tapi_tcp_pos_t ackn)
{
    asn_value *ack_template = NULL;
    int        rc;

    rc = create_tcp_template(conn_descr, conn_next_seq(conn_descr), ackn,
                             FALSE, TRUE, NULL, 0, &ack_template);
    if (rc != 0)
    {
        ERROR("%s: make ACK template error %r", __FUNCTION__, rc);
        return rc;
    }

    rc = set_timestamp(conn_descr, ack_template, TRUE, TRUE,
                       (tapi_tcp_compare_seqn(ackn,
                                              conn_descr->ack_sent) > 0));
    if (rc != 0)
    {
        ERROR("%s(): failed to set TCP timestamp, rc %r",
              __FUNCTION__, rc);
        asn_free_value(ack_template);
        return TE_RC(TE_TAPI, rc);
    }

    rc = tapi_tad_trsend_start(conn_descr->agt, conn_descr->snd_sid,
                               conn_descr->snd_csap,
                               ack_template, RCF_MODE_BLOCKING);
    asn_free_value(ack_template);
    if (rc != 0)
    {
        ERROR("%s: send ACK %r", __FUNCTION__, rc);
    }
    else
    {
        conn_descr->ack_sent = ackn;

        update_last_ts(conn_descr);
    }

    return rc;
}

int
tapi_tcp_send_ack(tapi_tcp_handler_t handler, tapi_tcp_pos_t ackn)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return conn_send_ack(conn_descr, ackn);
}

/* See description in tapi_tcp.h */
int
tapi_tcp_ack_all(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t  *conn_descr;
    tapi_tcp_pos_t          next_ackn;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    next_ackn = conn_next_ack(conn_descr);

    return conn_send_ack(conn_descr, next_ackn);
}

size_t
tapi_tcp_last_win_got(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->last_win_got;
}

te_bool
tapi_tcp_fin_got(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->fin_got;
}

te_bool
tapi_tcp_rst_got(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->reset_got;
}

/* See description in tapi_tcp.h */
tapi_tcp_pos_t
tapi_tcp_first_seqn_got(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->peer_isn;
}

tapi_tcp_pos_t
tapi_tcp_last_seqn_got(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->seq_got;
}


tapi_tcp_pos_t
tapi_tcp_last_ackn_got(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->ack_got;
}

/* See description in tapi_tcp.h */
tapi_tcp_pos_t
tapi_tcp_first_seqn_sent(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->our_isn;
}

tapi_tcp_pos_t
tapi_tcp_last_seqn_sent(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->seq_sent;
}


tapi_tcp_pos_t
tapi_tcp_last_ackn_sent(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return 0;

    return conn_descr->ack_sent;
}

tapi_tcp_pos_t
tapi_tcp_next_seqn(tapi_tcp_handler_t handler)
{
    tapi_tcp_conns_db_init();
    return conn_next_seq(tapi_tcp_find_conn(handler));
}

tapi_tcp_pos_t
tapi_tcp_next_ackn(tapi_tcp_handler_t handler)
{
    tapi_tcp_conns_db_init();
    return conn_next_ack(tapi_tcp_find_conn(handler));
}


static inline tapi_tcp_pos_t
conn_next_seq(tapi_tcp_connection_t *conn_descr)
{
    if (conn_descr == NULL)
        return 0;

    return conn_descr->seq_sent + conn_descr->last_len_sent;
}

static inline tapi_tcp_pos_t
conn_next_ack(tapi_tcp_connection_t *conn_descr)
{
    if (conn_descr == NULL)
        return 0;

    INFO("%s(conn %d) seq got %u; last len got = %d;",
         __FUNCTION__, conn_descr->id,
         conn_descr->seq_got, conn_descr->last_len_got);

    return conn_descr->seq_got + conn_descr->last_len_got;
}

static inline int
conn_update_sent_seq(tapi_tcp_connection_t *conn_descr,
                     size_t new_sent_len)
{
    if (conn_descr == NULL)
        return 0;

    conn_descr->seq_sent += conn_descr->last_len_sent;
    conn_descr->last_len_sent = new_sent_len;

    VERB("%s() last seq sent %u, new sent len %d",
         __FUNCTION__, conn_descr->seq_sent,
         conn_descr->last_len_sent);

    return 0;
}

int
tapi_tcp_update_sent_seq(tapi_tcp_handler_t handler, size_t new_sent_len)
{
    tapi_tcp_conns_db_init();
    return conn_update_sent_seq(tapi_tcp_find_conn(handler), new_sent_len);
}

static inline int
conn_update_sent_ack(tapi_tcp_connection_t *conn_descr, size_t ack)
{
    if (conn_descr == NULL)
        return 0;

    conn_descr->ack_sent = ack;

    VERB("%s() last ack sent %u", __FUNCTION__, conn_descr->ack_sent,
         conn_descr->ack_sent);

    return 0;
}

int
tapi_tcp_update_sent_ack(tapi_tcp_handler_t handler, size_t ack)
{
    tapi_tcp_conns_db_init();
    return conn_update_sent_ack(tapi_tcp_find_conn(handler), ack);
}

/* See description in tapi_tcp.h */
int
tapi_tcp_get_window(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
    {
        ERROR("TCP connection cannot be found");
        return -1;
    }

    return conn_descr->window;
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_set_window(tapi_tcp_handler_t handler, int window)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();

    if (window < 0 || window > MAX_TCP_WINDOW)
    {
        ERROR("Invalid TCP window size");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
    {
        ERROR("TCP connection cannot be found");
        return TE_RC(TE_TAPI, TE_ENOENT);
    }

    conn_descr->window = window;
    return 0;
}

int
tapi_tcp_conn_template(tapi_tcp_handler_t handler,
                       uint8_t *payload, size_t len, asn_value **tmpl)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_pos_t ackn;

    tapi_tcp_conns_db_init();
    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    ackn = conn_descr->ack_sent;

    return create_tcp_template(conn_descr, conn_next_seq(conn_descr),
                               ackn, FALSE, (ackn != 0),
                               payload, len, tmpl);
}

csap_handle_t
tapi_tcp_conn_snd_csap(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();
    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return conn_descr->snd_csap;
}

csap_handle_t
tapi_tcp_conn_rcv_csap(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;

    tapi_tcp_conns_db_init();
    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return conn_descr->rcv_csap;
}

int
tapi_tcp_wait_packet(tapi_tcp_handler_t handler, int timeout)
{
    tapi_tcp_connection_t *conn_descr;

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    /* It is simply wrapper for external use of static inline method. */
    return conn_wait_packet(conn_descr, timeout, NULL);
}

int
tapi_tcp_get_packets(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn_descr;
    unsigned int num = 0;
    te_errno rc;

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = rcf_ta_trrecv_get(conn_descr->agt, conn_descr->rcv_sid,
                           conn_descr->rcv_csap,
                           tcp_conn_pkt_handler, conn_descr, &num);
    if (rc != 0)
    {
        ERROR("%s: rcf_ta_trrecv_get() failed", __FUNCTION__);
        return -1;
    }

    return num;
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_conn_enable_ts(tapi_tcp_handler_t handler,
                        te_bool enable,
                        uint32_t start_value)
{
    tapi_tcp_connection_t  *conn_descr;

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    conn_descr->enabled_ts = enable;
    conn_descr->ts_start_value = start_value;

    return 0;
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_conn_get_ts(tapi_tcp_handler_t handler,
                     te_bool *enabled,
                     te_bool *dst_enabled,
                     uint32_t *ts_value,
                     uint32_t *last_ts_sent,
                     uint32_t *last_ts_got,
                     uint32_t *ts_to_echo,
                     uint32_t *last_ts_echo_sent,
                     uint32_t *last_ts_echo_got)
{
    tapi_tcp_connection_t  *conn_descr;

    if ((conn_descr = tapi_tcp_find_conn(handler)) == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if (enabled != NULL)
        *enabled = conn_descr->enabled_ts;

    if (conn_descr->enabled_ts)
    {
        if (dst_enabled != NULL)
            *dst_enabled = conn_descr->dst_enabled_ts;

        if (last_ts_sent != NULL)
            *last_ts_sent = conn_descr->last_ts_sent;

        if (last_ts_got != NULL)
            *last_ts_got = conn_descr->last_ts_got;

        if (ts_to_echo != NULL)
            *ts_to_echo = conn_descr->ts_to_echo;

        if (last_ts_echo_sent != NULL)
            *last_ts_echo_sent = conn_descr->last_ts_echo_sent;

        if (last_ts_echo_got != NULL)
            *last_ts_echo_got = conn_descr->last_ts_echo_got;

        if (ts_value != NULL)
            return get_current_ts(conn_descr, ts_value);
    }

    return 0;
}
