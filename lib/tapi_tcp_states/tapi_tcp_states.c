/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TCP states API library
 *
 * TCP states API library - generic functions implementation.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "te_sleep.h"
#include "te_sockaddr.h"
#include "tapi_test.h"
#include "tapi_rpc_socket.h"
#include "tapi_rpc_unistd.h"

#include "tapi_tcp_states.h"
#include "te_ethernet.h"
#include "tapi_ip4.h"
#include "ndn_ipstack.h"
#include "ndn_eth.h"
#include "tapi_route_gw.h"
#include "tapi_tcp_states_internal.h"

/**
 * Word used to signify using of timeout in TCP states
 * transition sequence
 */
#define timeout_word "timeout"

/**
 * Word used to signify using of packet with RST flag sending
 * in TCP states transition sequence
 */
#define rst_word "reset"

/** TCP maximum segment lifetime in milliseconds */
#define MAX_MSL 120000

/**
 * Sleep SLEEP_INT msec repeatedly waiting for TCP state
 * change after timeout.
 */
#define SLEEP_INT 100

/** Maximum length of string containing name of TCP state */
#define TCP_STATE_STR_LEN 20

/** Delimeters used in string representation of TCP states sequence. */
#define TSA_DELIMETERS " \t\r\n,;:->"

/**
 * Paths used for achieving of a given initial state
 * when testing sequence doesn't begin in TCP_CLOSE.
 * "Active" paths are through active TCP connection establishing,
 * "passive" ones are through passive establishing;
 * where they make no sense - they defined for simplicity of
 * related functions.
 */

#define TCP_CLOSE_PATH RPC_TCP_CLOSE
#define TCP_CLOSE_ACTIVE_PATH TCP_CLOSE_PATH
#define TCP_CLOSE_PASSIVE_PATH TCP_CLOSE_PATH

#define TCP_LISTEN_PATH \
    TCP_CLOSE_PATH, RPC_TCP_LISTEN
#define TCP_LISTEN_ACTIVE_PATH TCP_LISTEN_PATH
#define TCP_LISTEN_PASSIVE_PATH TCP_LISTEN_PATH

#define TCP_SYN_SENT_PATH \
    TCP_CLOSE_PATH, RPC_TCP_SYN_SENT
#define TCP_SYN_SENT_ACTIVE_PATH TCP_SYN_SENT_PATH
#define TCP_SYN_SENT_PASSIVE_PATH TCP_SYN_SENT_PATH

/*
 * SYN_RECV can be achieved both via LISTEN and via SYN_SENT
 * (simultaneous open with connect() from both ends, in real
 * world this happens rarely).
 * However when it is achieved via LISTEN, it is not directly
 * observable since listening socket itself does not change
 * its state to SYN_RECV and accept() returns only socket
 * which has already moved from SYN_RECV to ESTABLISHED.
 */
#define TCP_SYN_RECV_ACTIVE_PATH TCP_SYN_SENT_PATH, RPC_TCP_SYN_RECV
#define TCP_SYN_RECV_PASSIVE_PATH TCP_LISTEN_PATH, RPC_TCP_SYN_RECV
#define TCP_SYN_RECV_PATH TCP_SYN_RECV_ACTIVE_PATH

#define TCP_ESTABLISHED_PATH \
    TCP_SYN_SENT_PATH, RPC_TCP_ESTABLISHED
#define TCP_ESTABLISHED_ACTIVE_PATH \
    TCP_ESTABLISHED_PATH
#define TCP_ESTABLISHED_PASSIVE_PATH \
    TCP_LISTEN_PATH, RPC_TCP_SYN_RECV, RPC_TCP_ESTABLISHED

#define TCP_FIN_WAIT1_PATH \
    TCP_ESTABLISHED_PATH, RPC_TCP_FIN_WAIT1
#define TCP_FIN_WAIT1_PASSIVE_PATH \
    TCP_ESTABLISHED_PASSIVE_PATH, RPC_TCP_FIN_WAIT1
#define TCP_FIN_WAIT1_ACTIVE_PATH \
    TCP_ESTABLISHED_ACTIVE_PATH, RPC_TCP_FIN_WAIT1

#define TCP_CLOSE_WAIT_PATH \
    TCP_ESTABLISHED_PATH, RPC_TCP_CLOSE_WAIT
#define TCP_CLOSE_WAIT_PASSIVE_PATH \
    TCP_ESTABLISHED_PASSIVE_PATH, RPC_TCP_CLOSE_WAIT
#define TCP_CLOSE_WAIT_ACTIVE_PATH \
    TCP_ESTABLISHED_ACTIVE_PATH, RPC_TCP_CLOSE_WAIT

#define TCP_LAST_ACK_PATH \
    TCP_CLOSE_WAIT_PATH, RPC_TCP_LAST_ACK
#define TCP_LAST_ACK_PASSIVE_PATH \
    TCP_CLOSE_WAIT_PASSIVE_PATH, RPC_TCP_LAST_ACK
#define TCP_LAST_ACK_ACTIVE_PATH \
    TCP_CLOSE_WAIT_ACTIVE_PATH, RPC_TCP_LAST_ACK

#define TCP_FIN_WAIT2_PATH \
    TCP_FIN_WAIT1_PATH, RPC_TCP_FIN_WAIT2
#define TCP_FIN_WAIT2_PASSIVE_PATH \
    TCP_FIN_WAIT1_PASSIVE_PATH, RPC_TCP_FIN_WAIT2
#define TCP_FIN_WAIT2_ACTIVE_PATH \
    TCP_FIN_WAIT1_ACTIVE_PATH, RPC_TCP_FIN_WAIT2

#define TCP_CLOSING_PATH \
    TCP_FIN_WAIT1_PATH, RPC_TCP_CLOSING
#define TCP_CLOSING_PASSIVE_PATH \
    TCP_FIN_WAIT1_PASSIVE_PATH, RPC_TCP_CLOSING
#define TCP_CLOSING_ACTIVE_PATH \
    TCP_FIN_WAIT1_ACTIVE_PATH, RPC_TCP_CLOSING

#define TCP_TIME_WAIT_PATH \
    TCP_FIN_WAIT2_PATH, RPC_TCP_TIME_WAIT
#define TCP_TIME_WAIT_PASSIVE_PATH \
    TCP_FIN_WAIT2_PASSIVE_PATH, RPC_TCP_TIME_WAIT
#define TCP_TIME_WAIT_ACTIVE_PATH \
    TCP_FIN_WAIT2_ACTIVE_PATH, RPC_TCP_TIME_WAIT

static te_errno iut_syn(tsa_session *ss);
static te_errno tst_syn(tsa_session *ss);
static te_errno iut_syn_ack(tsa_session *ss);
static te_errno tst_syn_ack(tsa_session *ss);
static te_errno iut_ack(tsa_session *ss);
static te_errno tst_ack(tsa_session *ss);
static te_errno iut_fin(tsa_session *ss);
static te_errno tst_fin(tsa_session *ss);
static te_errno tst_fin_ack(tsa_session *ss);
static te_errno iut_close(tsa_session *ss);
static te_errno tst_rst(tsa_session *ss);
static te_errno iut_listen(tsa_session *ss);

/** Table of TCP states transitions according to TCP specification */
static tcp_move_action tcp_moves[] = {
    {RPC_TCP_CLOSE, RPC_TCP_LISTEN, NULL, iut_listen},
    {RPC_TCP_LISTEN, RPC_TCP_SYN_SENT, NULL, iut_syn},
    {RPC_TCP_LISTEN, RPC_TCP_SYN_RECV, tst_syn, iut_syn_ack},
    {RPC_TCP_CLOSE, RPC_TCP_SYN_SENT, NULL, iut_syn},
    {RPC_TCP_SYN_SENT, RPC_TCP_SYN_RECV, tst_syn, iut_syn_ack},
    {RPC_TCP_SYN_SENT, RPC_TCP_ESTABLISHED, tst_syn_ack, iut_ack},
    {RPC_TCP_SYN_SENT, RPC_TCP_CLOSE, NULL, iut_close},
    {RPC_TCP_SYN_SENT, RPC_TCP_CLOSE, NULL, iut_wait_change},
    {RPC_TCP_SYN_RECV, RPC_TCP_LISTEN, tst_rst, NULL},
    {RPC_TCP_SYN_RECV, RPC_TCP_ESTABLISHED, tst_ack, NULL},
    {RPC_TCP_SYN_RECV, RPC_TCP_FIN_WAIT1, NULL, iut_fin},
    {RPC_TCP_ESTABLISHED, RPC_TCP_FIN_WAIT1, NULL, iut_fin},
    {RPC_TCP_ESTABLISHED, RPC_TCP_CLOSE_WAIT, tst_fin, iut_ack},
    {RPC_TCP_CLOSE_WAIT, RPC_TCP_LAST_ACK, NULL, iut_fin},
    {RPC_TCP_LAST_ACK, RPC_TCP_CLOSE, tst_ack, NULL},
    {RPC_TCP_FIN_WAIT1, RPC_TCP_FIN_WAIT2, tst_ack, NULL},
    {RPC_TCP_FIN_WAIT1, RPC_TCP_CLOSING, tst_fin, iut_ack},
    {RPC_TCP_FIN_WAIT1, RPC_TCP_TIME_WAIT, tst_fin_ack, iut_ack},
    {RPC_TCP_FIN_WAIT2, RPC_TCP_TIME_WAIT, tst_fin, iut_ack},
    {RPC_TCP_CLOSING, RPC_TCP_TIME_WAIT, tst_ack, NULL},
    {RPC_TCP_TIME_WAIT, RPC_TCP_CLOSE, NULL, iut_wait_change},
    {RPC_TCP_UNKNOWN, RPC_TCP_UNKNOWN, NULL, NULL}
};

/**
 * Get string representation of a given move action.
 *
 * @param act       Action function
 *
 * @return
 *      Pointer to char array containing string representation
 *      of @p act.
 */
const char *
action2str(int (*act)(tsa_session *ss))
{
#define FUNC2STR(act_, name_) \
    if ((act_) == (name_)) \
        return #name_ \

    FUNC2STR(act, iut_ack);
    FUNC2STR(act, iut_close);
    FUNC2STR(act, iut_fin);
    FUNC2STR(act, iut_listen);
    FUNC2STR(act, iut_syn);
    FUNC2STR(act, iut_syn_ack);
    FUNC2STR(act, iut_wait_change);
    FUNC2STR(act, tst_ack);
    FUNC2STR(act, tst_fin);
    FUNC2STR(act, tst_fin_ack);
    FUNC2STR(act, tst_rst);
    FUNC2STR(act, tst_syn);
    FUNC2STR(act, tst_syn_ack);

    return "<UNKNOWN ACTION>";
#undef FUNC2STR
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_state_init(tsa_session *ss, tsa_tst_type tst_type)
{
    memset(ss, 0, sizeof(tsa_session));

    ss->state.tst_type = tst_type;

    if (tst_type == TSA_TST_SOCKET)
        tsa_set_sock_handlers(&ss->state.move_handlers);
    else
        tsa_set_csap_handlers(&ss->state.move_handlers);

    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_iut_set(tsa_session *ss, rcf_rpc_server *pco_iut,
            const struct if_nameindex *iut_if,
            const struct sockaddr *iut_addr)
{
    char        oid[RCF_MAX_ID];
    te_errno    rc;

    ss->config.pco_iut = pco_iut;
    ss->config.iut_if = iut_if;
    ss->config.iut_addr = iut_addr;

    if (ss->state.tst_type != TSA_TST_SOCKET)
    {
        ss->config.iut_link_addr = calloc(ETHER_ADDR_LEN, 1);

        snprintf(oid, RCF_MAX_ID, "/agent:%s/interface:%s",
                 ss->config.pco_iut->ta,
                 ss->config.iut_if->if_name);

        rc = tapi_cfg_base_if_get_mac(oid,
                                      (uint8_t *)
                                        ss->config.iut_link_addr);
        if (rc != 0)
        {
            ERROR("Cannot get ethernet IUT address");
            return rc;
        }
    }

    ss->state.iut_s_aux = -1;
    ss->state.iut_s = -1;
    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_tst_set(tsa_session *ss, rcf_rpc_server *pco_tst,
            const struct if_nameindex *tst_if,
            const struct sockaddr *tst_addr,
            const void *alien_link_addr)
{
    te_errno rc;

    ss->config.pco_tst = pco_tst;
    ss->config.tst_if = tst_if;
    ss->config.tst_addr = tst_addr;
    ss->config.alien_link_addr = alien_link_addr;

    if (ss->state.tst_type == TSA_TST_SOCKET)
    {
        ss->state.sock.tst_s_aux = -1;
        ss->state.sock.tst_s = -1;
    }
    else
        ss->state.csap.csap_tst_s = -1;

    if (ss->state.tst_type == TSA_TST_CSAP)
    {
        rc = tsa_break_iut_tst_conn(ss);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/* See description in tapi_tcp_states_internal.h */
void
wait_connectivity_changes(tsa_session *ss)
{
    if (ss->config.flags & TSA_NO_CONNECTIVITY_CHANGE)
        return;
    if (!(ss->config.flags & TSA_NO_CFG_WAIT_CHANGES))
       CFG_WAIT_CHANGES;
}

/* See the tapi_tcp_states.h file for the description. */
void
tsa_gw_preconf(tsa_session *ss, te_bool preconfigured)
{
    ss->config.gw_preconf = preconfigured;
}

/**
 * Configure network connection via gateway.
 *
 * @param ss        Pointer to tsa_session structure with filled
 *                  fields related to gateway.
 *
 * @return Status code.
 */
static te_errno
configure_gateway(tsa_session *ss)
{
    rpc_socket_addr_family  family;
    te_errno                rc;

    family = addr_family_h2rpc(ss->config.iut_addr->sa_family);

    /*
     * Add route on 'pco_iut': 'tst_addr' via gateway
     * 'gw_iut_addr'
     */
    rc = tapi_cfg_add_route_via_gw(
            ss->config.pco_iut->ta,
            addr_family_rpc2h(family),
            te_sockaddr_get_netaddr(ss->config.tst_addr),
            te_netaddr_get_size(addr_family_rpc2h(family)) * 8,
            te_sockaddr_get_netaddr(ss->config.gw_iut_addr));
    if (rc != 0)
        return rc;

    /*
     * We need to add IPv6 neighbors entries manually because there are cases
     * when Linux can not re-resolve FAILED entries for gateway routes.
     * See bug 9774.
     */
    if (family == RPC_AF_INET6)
    {
        CHECK_NZ_RETURN(tapi_update_arp(ss->config.pco_iut->ta,
                                        ss->config.iut_if->if_name,
                                        ss->config.pco_gw->ta,
                                        ss->config.gw_iut_if->if_name,
                                        ss->config.gw_iut_addr, NULL, FALSE));
        CHECK_NZ_RETURN(tapi_update_arp(ss->config.pco_gw->ta,
                                        ss->config.gw_iut_if->if_name,
                                        ss->config.pco_iut->ta,
                                        ss->config.iut_if->if_name,
                                        ss->config.iut_addr, NULL, FALSE));
    }

    ss->state.sock.route_dst_added = TRUE;

    /*
     * Add route on 'pco_tst': 'iut_addr' via gateway
     * 'gw_tst_addr'
     */
    rc = tapi_cfg_add_route_via_gw(
            ss->config.pco_tst->ta,
            addr_family_rpc2h(family),
            te_sockaddr_get_netaddr(ss->config.iut_addr),
            te_netaddr_get_size(addr_family_rpc2h(
                family)) * 8,
            te_sockaddr_get_netaddr(ss->config.gw_tst_addr));
    if (rc != 0)
        return rc;

    ss->state.sock.route_src_added = TRUE;

    /* Turn on forwarding on router host */
    if (family == RPC_AF_INET)
    {
        CHECK_NZ_RETURN(tapi_cfg_base_ipv4_fw_enabled(
                                            ss->config.pco_gw->ta,
                                            &ss->state.sock.ipv4_fw));
        CHECK_NZ_RETURN(tapi_cfg_base_ipv4_fw(ss->config.pco_gw->ta, TRUE));
        ss->state.sock.ipv4_fw_enabled = TRUE;
    }
    else if (family == RPC_AF_INET6)
    {
        CHECK_NZ_RETURN(tapi_cfg_base_ipv6_fw_enabled(
                                            ss->config.pco_gw->ta,
                                            &ss->state.sock.ipv6_fw));
        CHECK_NZ_RETURN(tapi_cfg_base_ipv6_fw(ss->config.pco_gw->ta, TRUE));
        ss->state.sock.ipv6_fw_enabled = TRUE;
    }

    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_gw_set(tsa_session *ss, rcf_rpc_server *pco_gw,
           const struct sockaddr *gw_iut_addr,
           const struct sockaddr *gw_tst_addr,
           const struct if_nameindex *gw_iut_if,
           const struct if_nameindex *gw_tst_if,
           const void *alien_link_addr)
{
    te_errno rc;

    if (ss->state.tst_type != TSA_TST_SOCKET &&
        ss->state.tst_type != TSA_TST_GW_CSAP)
    {
        ERROR("%s: Invalid tsa tester type (%d) for this call",
              __FUNCTION__, ss->state.tst_type);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    ss->config.pco_gw = pco_gw;
    ss->config.gw_iut_addr = gw_iut_addr;
    ss->config.gw_tst_addr = gw_tst_addr;
    ss->config.gw_iut_if = gw_iut_if;
    ss->config.gw_tst_if = gw_tst_if;
    ss->config.alien_link_addr = alien_link_addr;


    if (ss->state.tst_type == TSA_TST_GW_CSAP)
    {
        char oid[RCF_MAX_ID];

        if (ss->config.gw_tst_link_addr != NULL)
        {
            ERROR("Gateway link address is already specified");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
        ss->config.gw_tst_link_addr = calloc(ETHER_ADDR_LEN, 1);

        snprintf(oid, RCF_MAX_ID, "/agent:%s/interface:%s",
                 ss->config.pco_gw->ta,
                 ss->config.gw_tst_if->if_name);

        rc = tapi_cfg_base_if_get_mac(oid,
                                      (uint8_t *)
                                        ss->config.gw_tst_link_addr);

        if (rc != 0)
        {
            ERROR("Cannot get ethernet address of gateway tester "
                  "interface");
            return rc;
        }

        rc = tsa_break_iut_tst_conn(ss);
        if (rc != 0)
            return rc;
    }

    if (!ss->config.gw_preconf)
    {
        rc = configure_gateway(ss);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/* See description in tapi_tcp_states_internal.h */
te_errno
tsa_sock_create(tsa_session *ss, int rpc_selector)
{
    tarpc_linger           ling_optval;
    int                    opt_val;
    rcf_rpc_server        *pco;
    int                   *s;
    const struct sockaddr *addr;
    int                    fdflags;
    rpc_socket_addr_family family;

    family = addr_family_h2rpc(ss->config.iut_addr->sa_family);

    switch (rpc_selector)
    {
        case TSA_TST:
            pco = ss->config.pco_tst;
            s = &(ss->state.sock.tst_s);
            addr = ss->config.tst_addr;
            break;

        case TSA_IUT:
            pco = ss->config.pco_iut;
            s = &(ss->state.iut_s);
            addr = ss->config.iut_addr;
            break;

        default:
            return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (*s == -1)
    {
        RPC_AWAIT_ERROR(pco);
        *s = rpc_socket(pco, family,
                        RPC_SOCK_STREAM, RPC_PROTO_DEF);
        if (*s < 0)
            return RPC_ERRNO(pco);
    }

    if (rpc_selector == TSA_TST)
    {
        /*
         * Always set zero linger on Tester socket to
         * allow to generate RST from it if required.
         */
        ling_optval.l_onoff = 1;
        ling_optval.l_linger = 0;
        RPC_AWAIT_ERROR(pco);
        if (rpc_setsockopt(pco, *s, RPC_SO_LINGER,
                           &ling_optval) < 0)
            return RPC_ERRNO(pco);
    }

    if (ss->config.flags & TSA_TST_USE_REUSEADDR)
    {
        opt_val = 1;
        RPC_AWAIT_ERROR(pco);
        if (rpc_setsockopt(pco, *s,
                           RPC_SO_REUSEADDR, &opt_val) < 0)
            return RPC_ERRNO(pco);
    }

    RPC_AWAIT_ERROR(pco);
    if (rpc_bind(pco, *s, addr) < 0)
        return RPC_ERRNO(pco);

    if (rpc_selector == TSA_IUT)
    {
        RPC_AWAIT_ERROR(ss->config.pco_iut);
        fdflags = rpc_fcntl(ss->config.pco_iut,
                            ss->state.iut_s,
                            RPC_F_GETFL,
                            0);
        if (fdflags < 0)
            return RPC_ERRNO(ss->config.pco_iut);

        fdflags |= RPC_O_NONBLOCK;

        RPC_AWAIT_ERROR(ss->config.pco_iut);
        if (rpc_fcntl(ss->config.pco_iut,
                      ss->state.iut_s,
                      RPC_F_SETFL,
                      fdflags) < 0)
            return RPC_ERRNO(ss->config.pco_iut);
    }
    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_create_session(tsa_session *ss, uint32_t flags)
{
    te_errno rc = 0;

    ss->config.flags = flags;

    if (ss->state.tst_type == TSA_TST_SOCKET)
    {
        rc = rcf_ta_create_session(ss->config.pco_tst->ta,
                                   &ss->state.sock.sid);
        if (rc != 0)
            return rc;

        memset(&ss->state.sock.rst_hack_c, 0,
               sizeof(ss->state.sock.rst_hack_c));
        ss->state.sock.rst_hack_c.loc_ip_addr = SIN(ss->config.tst_addr)->
                                                        sin_addr.s_addr;
        ss->state.sock.rst_hack_c.rem_ip_addr = SIN(ss->config.iut_addr)->
                                                        sin_addr.s_addr;
        rc = tapi_tcp_reset_hack_init(ss->config.pco_tst->ta,
                                      ss->state.sock.sid,
                                      ss->config.tst_if->if_name, TRUE,
                                      &ss->state.sock.rst_hack_c);
        if (rc != 0)
            return rc;

        ss->state.state_cur = RPC_TCP_UNKNOWN;
    }

    rc = tsa_sock_create(ss, TSA_IUT);
    if (rc != 0)
        return rc;

    if (ss->state.tst_type == TSA_TST_SOCKET)
    {
        rc = tsa_sock_create(ss, TSA_TST);
        if (rc != 0)
            return rc;

        if (!(ss->config.flags & TSA_ESTABLISH_PASSIVE))
        {
            RPC_AWAIT_ERROR(ss->config.pco_tst);
            if (rpc_listen(ss->config.pco_tst, ss->state.sock.tst_s,
                           TSA_BACKLOG_DEF) < 0)
                return RPC_ERRNO(ss->config.pco_tst);
        }
    }
    else
    {
        rc = tapi_tcp_create_conn(ss->config.pco_tst->ta,
                                  ss->config.tst_addr,
                                  ss->config.iut_addr,
                                  ss->config.tst_if->if_name,
                                  ss->config.alien_link_addr,
                                  ss->state.tst_type == TSA_TST_CSAP ?
                                      ss->config.iut_link_addr :
                                      ss->config.gw_tst_link_addr,
                                  0, &ss->state.csap.csap_tst_s);
        if (rc != 0)
            return rc;
    }

    rc = tsa_update_cur_state(ss);
    if (rc != 0)
        return rc;

    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_destroy_session(tsa_session *ss)
{
    te_errno                rc = 0;
    te_errno                rc_aux = 0;
    rpc_socket_addr_family  family = RPC_AF_UNKNOWN;

    if (ss->config.iut_addr != NULL)
        family = addr_family_h2rpc(ss->config.iut_addr->sa_family);

    if (ss->state.iut_alien_arp_added)
    {
        if ((rc_aux = tsa_repair_tst_iut_conn(ss)) != 0)
            rc = rc_aux;
    }
    if (ss->state.tst_alien_arp_added)
    {
        if ((rc_aux = tsa_repair_iut_tst_conn(ss)) != 0)
            rc = rc_aux;
    }
    wait_connectivity_changes(ss);

    if (ss->state.iut_wait_connect)
    {
        if (ss->state.tst_wait_connect &&
            ss->state.tst_type != TSA_TST_SOCKET)
        {
            if (tapi_tcp_wait_open(ss->state.csap.csap_tst_s,
                                   MAX_CHANGE_TIMEOUT) != 0)
                ss->state.csap.csap_tst_s = -1;
            ss->state.tst_wait_connect = FALSE;
        }

        RPC_AWAIT_ERROR(ss->config.pco_iut);
        rpc_connect(ss->config.pco_iut, ss->state.iut_s,
                    ss->config.tst_addr);

        ss->state.iut_wait_connect = FALSE;
    }

    if (ss->state.tst_wait_connect &&
        ss->state.tst_type == TSA_TST_SOCKET)
    {
        ss->config.pco_tst->op = RCF_RPC_WAIT;
        RPC_AWAIT_ERROR(ss->config.pco_tst);
        rpc_connect(ss->config.pco_tst, ss->state.sock.tst_s,
                    ss->config.tst_addr);

        ss->state.tst_wait_connect = FALSE;
    }

    if (ss->config.pco_iut != NULL)
    {
        if (ss->state.iut_s_aux != -1)
        {
            RPC_AWAIT_ERROR(ss->config.pco_iut);
            if (rpc_close(ss->config.pco_iut, ss->state.iut_s_aux) != 0)
                rc = RPC_ERRNO(ss->config.pco_iut);
        }

        if (ss->state.iut_s != -1)
        {
            RPC_AWAIT_ERROR(ss->config.pco_iut);
            if (rpc_close(ss->config.pco_iut, ss->state.iut_s) != 0)
                rc = RPC_ERRNO(ss->config.pco_iut);
        }
    }

    if (ss->state.tst_type == TSA_TST_SOCKET)
    {
        if (ss->config.pco_tst != NULL)
        {
            if (ss->state.sock.tst_s != -1)
            {
                RPC_AWAIT_ERROR(ss->config.pco_tst);
                if (rpc_close(ss->config.pco_tst,
                              ss->state.sock.tst_s) != 0)
                    rc = RPC_ERRNO(ss->config.pco_tst);
            }

            if (ss->state.sock.tst_s_aux != -1)
            {
                RPC_AWAIT_ERROR(ss->config.pco_tst);
                if (rpc_close(ss->config.pco_tst,
                              ss->state.sock.tst_s_aux) != 0)
                    rc = RPC_ERRNO(ss->config.pco_tst);
            }
        }
    }

    if (ss->state.tst_type != TSA_TST_SOCKET)
    {
        if (ss->state.csap.csap_tst_s != -1)
        {
            rc_aux = tapi_tcp_destroy_connection(ss->state.csap.csap_tst_s);
            if (rc_aux != 0)
            {
                ERROR("Destroying of CSAP connection failed");
                rc = rc_aux;
            }
        }
        ss->state.csap.csap_tst_s = -1;
    }

    ss->state.iut_s = -1;
    ss->state.iut_s_aux = -1;

    if (ss->state.tst_type == TSA_TST_SOCKET)
    {
        ss->state.sock.tst_s = -1;
        ss->state.sock.tst_s_aux = -1;

        if (ss->state.sock.route_dst_added)
        {
            rc_aux = tapi_cfg_del_route_via_gw(
                        ss->config.pco_iut->ta,
                        addr_family_rpc2h(family),
                        te_sockaddr_get_netaddr(ss->config.tst_addr),
                        te_netaddr_get_size(addr_family_rpc2h(
                                                        family)) * 8,
                        te_sockaddr_get_netaddr(ss->config.gw_iut_addr));
            if (rc_aux != 0)
                rc = rc_aux;
        }

        if (ss->state.sock.route_src_added)
        {
            rc_aux = tapi_cfg_del_route_via_gw(
                        ss->config.pco_tst->ta,
                        addr_family_rpc2h(family),
                        te_sockaddr_get_netaddr(ss->config.iut_addr),
                        te_netaddr_get_size(addr_family_rpc2h(
                                                        family)) * 8,
                        te_sockaddr_get_netaddr(ss->config.gw_tst_addr));
            if (rc_aux != 0)
                rc = rc_aux;
        }

        if (ss->state.sock.ipv4_fw_enabled)
        {
            rc_aux = tapi_cfg_base_ipv4_fw(ss->config.pco_gw->ta,
                                           ss->state.sock.ipv4_fw);
            if (rc_aux != 0)
                rc = rc_aux;
        }

        if (ss->state.sock.ipv6_fw_enabled)
        {
            rc_aux = tapi_cfg_base_ipv6_fw(ss->config.pco_gw->ta,
                                           ss->state.sock.ipv6_fw);
            if (rc_aux != 0)
                rc = rc_aux;
        }
    }
    CFG_WAIT_CHANGES;

    if (ss->state.tst_type == TSA_TST_SOCKET)
    {
        rc_aux = tapi_tcp_reset_hack_clear(ss->config.pco_tst->ta,
                                           ss->state.sock.sid,
                                           &ss->state.sock.rst_hack_c);
        if (rc_aux != 0)
            rc = rc_aux;
    }

    free((void *)ss->config.gw_tst_link_addr);
    free((void *)ss->config.iut_link_addr);

    return rc;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_set_start_tcp_state(tsa_session *ss, rpc_tcp_state state,
                        rpc_tcp_state stop_state,
                        uint32_t flags)
{
#define STATE2PATH(tsa_ss_, tcp_state_) \
    case RPC_ ## tcp_state_:                                \
        if (flags & TSA_ESTABLISH_PASSIVE)                  \
            rc = tsa_do_moves((tsa_ss_), stop_state, flags, \
                              tcp_state_ ## _PASSIVE_PATH,  \
                              RPC_TCP_UNKNOWN);             \
        else                                                \
            rc = tsa_do_moves((tsa_ss_), stop_state, flags, \
                              tcp_state_ ## _ACTIVE_PATH,   \
                              RPC_TCP_UNKNOWN);             \
        break

    te_errno    rc = 0;

    switch(state)
    {
        STATE2PATH(ss, TCP_ESTABLISHED);
        STATE2PATH(ss, TCP_SYN_SENT);
        STATE2PATH(ss, TCP_SYN_RECV);
        STATE2PATH(ss, TCP_FIN_WAIT1);
        STATE2PATH(ss, TCP_FIN_WAIT2);
        STATE2PATH(ss, TCP_TIME_WAIT);
        STATE2PATH(ss, TCP_CLOSE_WAIT);
        STATE2PATH(ss, TCP_LAST_ACK);
        STATE2PATH(ss, TCP_LISTEN);
        STATE2PATH(ss, TCP_CLOSING);

        case RPC_TCP_CLOSE:
            rc = 0;
            break;

        default:
            assert(0);
    }

    return rc;
#undef STATE2PATH
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_break_tst_iut_conn(tsa_session *ss)
{
    te_errno rc = 0;

    if (ss->config.flags & TSA_NO_CONNECTIVITY_CHANGE)
        return 0;

    if (ss->state.iut_alien_arp_added)
        return 0;

    if (ss->state.tst_type == TSA_TST_SOCKET ||
        ss->state.tst_type == TSA_TST_GW_CSAP)
    {
        rc = tapi_update_arp(ss->config.pco_tst->ta,
                             ss->config.tst_if->if_name,
                             NULL, NULL, ss->config.gw_tst_addr,
                             ss->config.alien_link_addr, TRUE);
    }
    else
    {
        rc = tapi_update_arp(ss->config.pco_tst->ta,
                             ss->config.tst_if->if_name,
                             NULL, NULL, ss->config.iut_addr,
                             ss->config.alien_link_addr, TRUE);
    }

    if (rc != 0)
        return rc;

    ss->state.iut_alien_arp_added = TRUE;
    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_break_iut_tst_conn(tsa_session *ss)
{
    te_errno rc = 0;

    if (ss->config.flags & TSA_NO_CONNECTIVITY_CHANGE)
        return 0;

    if (ss->state.tst_alien_arp_added)
        return 0;

    if (ss->state.tst_type == TSA_TST_SOCKET ||
        ss->state.tst_type == TSA_TST_GW_CSAP)
    {
        rc = tapi_update_arp(ss->config.pco_gw->ta,
                             ss->config.gw_tst_if->if_name,
                             NULL, NULL, ss->config.tst_addr,
                             ss->config.alien_link_addr, TRUE);
    }
    else
    {
        rc = tapi_update_arp(ss->config.pco_iut->ta,
                             ss->config.iut_if->if_name,
                             NULL, NULL, ss->config.tst_addr,
                             ss->config.alien_link_addr, TRUE);
    }

    if (rc != 0)
        return rc;

    ss->state.tst_alien_arp_added = TRUE;
    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_repair_tst_iut_conn(tsa_session *ss)
{
    te_errno rc = 0;

    if (ss->config.flags & TSA_NO_CONNECTIVITY_CHANGE)
        return 0;

    if (!(ss->state.iut_alien_arp_added))
        return 0;

    if (ss->state.tst_type == TSA_TST_SOCKET ||
        ss->state.tst_type == TSA_TST_GW_CSAP)
    {
        rc = tapi_update_arp(ss->config.pco_tst->ta,
                             ss->config.tst_if->if_name,
                             ss->config.pco_gw->ta,
                             ss->config.gw_tst_if->if_name,
                             ss->config.gw_tst_addr, NULL, FALSE);
    }
    else
    {
        rc = tapi_update_arp(ss->config.pco_tst->ta,
                             ss->config.tst_if->if_name,
                             ss->config.pco_iut->ta,
                             ss->config.iut_if->if_name,
                             ss->config.iut_addr, NULL, FALSE);
    }

    if (rc != 0)
        return rc;

    ss->state.iut_alien_arp_added = FALSE;

    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_repair_iut_tst_conn(tsa_session *ss)
{
    te_errno rc = 0;

    if (ss->config.flags & TSA_NO_CONNECTIVITY_CHANGE)
        return 0;

    if (!(ss->state.tst_alien_arp_added))
        return 0;

    if (ss->state.tst_type == TSA_TST_SOCKET ||
        ss->state.tst_type == TSA_TST_GW_CSAP)
    {
        rc = tapi_update_arp(ss->config.pco_gw->ta,
                             ss->config.gw_tst_if->if_name,
                             ss->config.pco_tst->ta,
                             ss->config.tst_if->if_name,
                             ss->config.tst_addr, NULL, FALSE);
    }
    else
    {
        rc = tapi_update_arp(ss->config.pco_iut->ta,
                             ss->config.iut_if->if_name,
                             ss->config.pco_tst->ta,
                             ss->config.tst_if->if_name,
                             ss->config.tst_addr, NULL, FALSE);
    }

    if (rc != 0)
        return rc;

    ss->state.tst_alien_arp_added = FALSE;

    return 0;
}

/**
 * Find actions needed to be done in order to move from one TCP state
 * to another one.
 *
 * @param state_from    Initial TCP state
 * @param state_to      TCP state to move to
 * @param flags         Flags determining whether to use transition
 *                      via timeout or RST if available
 *
 * @return
 *     Pointer to tcp_move_action struct containing pointers to action
 *     functions.
 */
static tcp_move_action *
get_tcp_move(rpc_tcp_state state_from, rpc_tcp_state state_to,
             uint32_t flags)
{
    int i;

    struct tcp_move_action *act = NULL;

    for (i = 0; tcp_moves[i].state_from != RPC_TCP_UNKNOWN; i++)
    {
        if (tcp_moves[i].state_from == state_from &&
            tcp_moves[i].state_to == state_to)
        {
            act = &(tcp_moves[i]);

            /*
             * If we should use timeout if possible we stop searching
             * when we find @b iut_wait_change(). If we shouldn't
             * use timeout if possible we stop searching when we find some
             * action other than @b iut_wait_change. Otherwise we continue
             * searching.
             * The same considerations are applicable to sending RST.
             */
            if ((flags & TSA_ACT_TIMEOUT) &&
                tcp_moves[i].iut_act == iut_wait_change)
                break;

            if ((flags & TSA_ACT_RST) &&
                tcp_moves[i].tst_act == tst_rst)
                break;

            if (!(flags & TSA_ACT_TIMEOUT) &&
                tcp_moves[i].iut_act != iut_wait_change &&
                !(flags & TSA_ACT_RST) &&
                tcp_moves[i].tst_act != tst_rst)
                break;
        }
    }

    return act;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_do_tcp_move(tsa_session *ss, rpc_tcp_state state_from,
                rpc_tcp_state state_to, uint32_t flags)
{
    struct timeval tv_before;
    struct timeval tv_after;

    struct tcp_move_action *act = NULL;

    te_errno rc = 0;

    ss->state.timeout_used = flags & TSA_ACT_TIMEOUT;
    ss->state.state_from = state_from;
    ss->state.state_to = state_to;
    ss->state.elapsed_time = 0;

    if (state_from == state_to)
        return 0;

    /*
     * Note that state_from may not be equal to real
     * current TCP state. For example, when we cannot
     * use forwarding breaking to see non-stable
     * TCP states, we nevertheless perform
     * the same actions as if we saw them.
     */
    RING("Performing actions to move from %s to %s",
         tcp_state_rpc2str(state_from),
         tcp_state_rpc2str(state_to));

    if (flags & TSA_ACT_TIMEOUT)
        gettimeofday(&tv_before, NULL);

    act = get_tcp_move(state_from, state_to, flags);

    if (act == NULL)
    {
        VERB("Actions for transition from %s to %s wasn't found",
             tcp_state_rpc2str(state_from),
             tcp_state_rpc2str(state_to));

        return TE_RC(TE_TAPI, TE_ENOENT);
    }

    if (act->tst_act != NULL)
    {
        rc = act->tst_act(ss);
        if (rc != 0)
        {
            VERB("Action %s failed in transition from %s to %s",
                 action2str(act->tst_act),
                 tcp_state_rpc2str(state_from),
                 tcp_state_rpc2str(state_to));
            return rc;
        }
    }

    if (act->iut_act != NULL)
    {
        rc = act->iut_act(ss);
        if (rc != 0)
        {
            VERB("Action %s failed in transition from %s to %s",
                 action2str(act->iut_act),
                 tcp_state_rpc2str(state_from),
                 tcp_state_rpc2str(state_to));
            return rc;
        }
    }

    if (flags & TSA_ACT_TIMEOUT)
    {
        gettimeofday(&tv_after, NULL);
        ss->state.elapsed_time = (tv_after.tv_sec - tv_before.tv_sec)
                                  * 1000 + (tv_after.tv_usec -
                                            tv_before.tv_usec) / 1000;
    }

    if (state_to != tsa_state_cur(ss))
    {
        rc = tsa_update_cur_state(ss);
        if (rc != 0)
            return rc;
    }

    if (tsa_state_cur(ss) != state_to &&
        !(flags & TSA_MOVE_IGNORE_ERR))
        return TE_RC(TE_TAPI, TE_EFAIL);

    return 0;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_do_moves(tsa_session *ss, rpc_tcp_state stop_state,
             uint32_t flags, ...)
{
    va_list       argptr;
    rpc_tcp_state tcp_next = RPC_TCP_UNKNOWN;
    rpc_tcp_state tcp_cur;
    rpc_tcp_state tcp_init;
    te_errno      rc = 0;

    va_start(argptr, flags);

    tcp_cur = va_arg(argptr, rpc_tcp_state);
    tcp_init = tsa_state_cur(ss);

    /* If the first state of path specified in arguments
     * and initial TCP state are not the same - try to
     * pass TCP states up to initial one (in which socket
     * is now) */
    while (tcp_cur != tcp_init &&
           tcp_cur != RPC_TCP_UNKNOWN)
        tcp_cur = va_arg(argptr, rpc_tcp_state);

    if (tcp_cur == RPC_TCP_UNKNOWN)
        return TE_RC(TE_TAPI, TE_EINVAL);

    do {
        tcp_next = va_arg(argptr, rpc_tcp_state);

        if (tcp_next == RPC_TCP_UNKNOWN)
            break;

        rc = tsa_do_tcp_move(ss, tcp_cur,
                             tcp_next, flags);
        if (rc != 0)
            break;

        tcp_cur = tcp_next;
        if (tcp_cur == stop_state)
        {
            rc = TSA_ESTOP;
            break;
        }
    } while (1);

    va_end(argptr);

    return rc;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_do_moves_str(tsa_session *ss,
                 rpc_tcp_state init_state,
                 rpc_tcp_state stop_state,
                 uint32_t flags, const char *s)
{
    const char     *p = NULL;
    const char     *q = NULL;
    char            buff[TCP_STATE_STR_LEN];
    rpc_tcp_state   next_state = RPC_TCP_CLOSE;
    rpc_tcp_state   prev_state = RPC_TCP_CLOSE;
    uint32_t        move_flags = flags;
    te_bool         first_state = TRUE;
    te_errno        rc = 0;

    const char *delims = TSA_DELIMETERS;

    if (init_state != RPC_TCP_UNKNOWN)
        prev_state = init_state;
    else
        prev_state = tsa_state_cur(ss);

    ss->state.rem_path = s;

    VERB("tsa_do_moves_str() call, pco_tst %p, pco_iut %p, "
         "transition sequence %s, initial state %s", ss->config.pco_tst,
         ss->config.pco_iut, s, tcp_state_rpc2str(prev_state));

    if (tsa_state_cur(ss) == stop_state)
        return TSA_ESTOP;

    for (p = s; ; p++)
    {
        if (*p != '\0' && strchr(delims, *p) == NULL)
        {
            if (q == NULL)
                q = p;
        }
        else if (q != NULL)
        {
            strncpy(buff, q, (int) (p - q));
            buff[(int) (p - q)] = '\0';

            ss->state.rem_path = p;

            if (strncmp(buff, timeout_word, TCP_STATE_STR_LEN) == 0)
                move_flags |= TSA_ACT_TIMEOUT;
            else if (strncmp(buff, rst_word, TCP_STATE_STR_LEN) == 0)
                move_flags |= TSA_ACT_RST;
            else
            {
                next_state = tcp_state_str2rpc(buff);
                if (next_state == RPC_TCP_UNKNOWN)
                {
                    ss->state.state_to = RPC_TCP_UNKNOWN;
                    rc = TE_RC(TE_TAPI, TE_EFAIL);
                    goto exit;
                }

                /*
                 * If we start from RPC_TCP_CLOSE and there
                 * is no direct transition from it to the next
                 * state in the table, we get intermediate states from
                 * paths defined at the beginning of the file.
                 */
                if (first_state &&
                    get_tcp_move(prev_state, next_state, 0) == NULL &&
                    prev_state != next_state)
                {
                    rc = tsa_set_start_tcp_state(
                                ss, next_state, stop_state,
                                flags | ((flags & TSA_MOVE_IGNORE_START_ERR) ?
                                         TSA_MOVE_IGNORE_ERR : 0));
                    if (rc != 0)
                    {
                        if (stop_state != next_state)
                            ss->state.rem_path = s;
                        goto exit;
                    }
                }
                else
                {
                    rc = tsa_do_tcp_move(ss, prev_state,
                                         next_state, move_flags);

                    if (rc > 0 ||
                        (tsa_state_cur(ss) != next_state &&
                         !(flags & TSA_MOVE_IGNORE_ERR)))
                    {
                        if (rc == 0)
                            rc = TE_RC(TE_TAPI, TE_EFAIL);

                        goto exit;
                    }
                }

                first_state = FALSE;

                if (tsa_state_cur(ss) == stop_state &&
                    next_state == stop_state)
                {
                    rc = TSA_ESTOP;
                    goto exit;
                }

                move_flags = flags;
                prev_state = next_state;
            }

            q = NULL;
        }

        if (*p == '\0')
            break;
    }

exit:

    return rc;
}

/**
 * Action to be done when SYN must be sent from socket on the IUT side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
iut_syn(tsa_session *ss)
{
    return ss->state.move_handlers.iut_syn(ss);
}

/**
 * Action to be done when SYN must be sent from socket on the TST side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
tst_syn(tsa_session *ss)
{
    return ss->state.move_handlers.tst_syn(ss);
}

/**
 * Action to be done when SYN-ACK must be sent from socket on the IUT side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
iut_syn_ack(tsa_session *ss)
{
    return ss->state.move_handlers.iut_syn_ack(ss);
}

/**
 * Action to be done when SYN-ACK must be sent from socket on the TST side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
tst_syn_ack(tsa_session *ss)
{
    return ss->state.move_handlers.tst_syn_ack(ss);
}

/**
 * Action to be done when ACK must be sent from socket on the IUT side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
iut_ack(tsa_session *ss)
{
    return ss->state.move_handlers.iut_ack(ss);
}

/**
 * Action to be done when ACK must be sent from socket on the TST side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
tst_ack(tsa_session *ss)
{
    return ss->state.move_handlers.tst_ack(ss);
}

/**
 * Action to be done when FIN must be sent from socket on the IUT side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
iut_fin(tsa_session *ss)
{
    return ss->state.move_handlers.iut_fin(ss);
}

/**
 * Action to be done when FIN must be sent from socket on the TST side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
tst_fin(tsa_session *ss)
{
    return ss->state.move_handlers.tst_fin(ss);
}

/**
 * Action to be done when FIN-ACK (i.e. packet with FIN flag and
 * ACK on previously received FIN simultaneously)
 * must be sent from socket on the TST side according to TCP
 * specification. This can be performed by CSAP only.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
tst_fin_ack(tsa_session *ss)
{
    return ss->state.move_handlers.tst_fin_ack(ss);
}

/**
 * Action to be done when appication should be closed on the IUT side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
iut_close(tsa_session *ss)
{
    return iut_fin(ss);
}

/**
 * Action to be done when RST must be sent from socket on the TST side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
tst_rst(tsa_session *ss)
{
    return ss->state.move_handlers.tst_rst(ss);
}

/**
 * Action to be done when socket on the IUT side should become listening
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
iut_listen(tsa_session *ss)
{
    return ss->state.move_handlers.iut_listen(ss);
}

/* See description in tapi_tcp_states_internal.h */
te_errno
iut_wait_change_gen(tsa_session *ss, int timeout)
{
    int i;
    int step = SLEEP_INT;

    te_errno rc = 0;

    rpc_tcp_state state_cur;

    rc = tsa_update_cur_state(ss);
    if (rc != 0)
        return rc;

    state_cur = tsa_state_cur(ss);

    if (state_cur != tsa_state_from(ss))
        return 0;

    RING("Wait untill TCP state of IUT socket will change");
    for(i = 0; i <= timeout; i += step)
    {
        MSLEEP(step);
        rc = tsa_update_cur_state(ss);
        if (rc != 0)
            return rc;

        if (tsa_state_cur(ss) != state_cur ||
            tsa_state_cur(ss) == tsa_state_to(ss))
            break;

        step *= 2;
        if (step > timeout - i)
            step = (timeout - i) > 0 ? timeout - i : 1;
    }

    if (i > timeout)
        return TE_RC(TE_TAPI, TE_ETIMEDOUT);

    return 0;
}

/* See description in tapi_tcp_states_internal.h */
te_errno
iut_wait_change(tsa_session *ss)
{
    te_errno rc = 0;

    rc = iut_wait_change_gen(ss, 2 * MAX_MSL);

    if (rc != 0)
        RING("TCP state of IUT socket was not changed "
             "after waiting for 2 * MAX_MSL time");

    return rc;
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_update_cur_state(tsa_session *ss)
{
    int                 rc = -1;
    struct rpc_tcp_info info;

    memset(&info, 0, sizeof(info));
    RPC_AWAIT_ERROR(ss->config.pco_iut);
    rc = rpc_getsockopt(ss->config.pco_iut, ss->state.iut_s,
                        RPC_TCP_INFO, &info);

    if (rc == 0)
    {
        ss->state.state_cur = info.tcpi_state;
        return 0;
    }
    else
    {
        return RPC_ERRNO(ss->config.pco_iut);
    }
}

/* See the tapi_tcp_states.h file for the description. */
te_errno
tsa_tst_send_rst(tsa_session *ss)
{
    ss->state.state_to = RPC_TCP_CLOSE;
    return tst_rst(ss);
}

/* See the tapi_tcp_states.h file for the description. */
void
tsa_packet_handler(const char *packet, void *user_param)
{
    asn_value             *tcp_message = NULL;
    const asn_value       *tcp_pdu;
    const asn_value       *subval;
    const asn_value       *val;
    tsa_packets_counter   *ctx = (tsa_packets_counter *)user_param;

    int32_t pdu_field;
    int     syms;
    int     rc;

    ctx->count++;

    if (packet == NULL || user_param == NULL)
    {
        ERROR("Packet handler received bad arguments");
        return;
    }

    rc = asn_parse_dvalue_in_file(packet, ndn_raw_packet,
                                  &tcp_message, &syms);
    if (rc != 0)
    {
        ERROR("Cannot parse message file: %r, sym %d", rc, syms);
        return;
    }

#define CHECK_ERROR(msg_) \
do {                                                             \
    if (rc != 0)                                                 \
    {                                                            \
        asn_free_value(tcp_message);                             \
        ERROR("packet %d: %s, rc %r", ctx->count, msg_, rc);     \
        return;                                                  \
    }                                                            \
} while (0)

    val = tcp_message;
    rc = asn_get_child_value(val, &subval, PRIVATE, NDN_PKT_PDUS);
    CHECK_ERROR("get pdus error");

    val = subval;
    rc = asn_get_indexed(val, (asn_value **)&subval, 0, NULL);
    CHECK_ERROR("get TCP gen pdu error");
    rc = asn_get_choice_value(subval, (asn_value **)&tcp_pdu, NULL, NULL);
    CHECK_ERROR("get TCP special choice error");

    rc = ndn_du_read_plain_int(tcp_pdu, NDN_TAG_TCP_FLAGS, &pdu_field);
    CHECK_ERROR("read TCP flag error");

    switch (pdu_field)
    {
        case TCP_ACK_FLAG:
            ctx->ack++;
            VERB("ACK %d", ctx->ack);
            break;

        case TCP_SYN_FLAG:
            ctx->syn++;
            VERB("SYN %d", ctx->syn);
            break;

        case TCP_ACK_FLAG | TCP_SYN_FLAG:
            ctx->syn_ack++;
            VERB("SYN-ACK %d", ctx->syn_ack);
            break;

        case TCP_ACK_FLAG | TCP_PSH_FLAG:
            ctx->push_ack++;
            VERB("PSH-ACK %d", ctx->push_ack);
            break;

        case TCP_ACK_FLAG | TCP_FIN_FLAG:
            ctx->fin_ack++;
            VERB("FIN-ACK %d", ctx->fin_ack);
            break;

        case TCP_ACK_FLAG | TCP_FIN_FLAG | TCP_PSH_FLAG:
            ctx->push_fin_ack++;
            VERB("PSH-FIN-ACK %d", ctx->push_fin_ack);
            break;

        case TCP_ACK_FLAG | TCP_RST_FLAG:
            ctx->rst_ack++;
            VERB("RST-ACK %d", ctx->rst_ack);
            break;

        case TCP_RST_FLAG:
            ctx->rst++;
            VERB("RST %d", ctx->rst);
            break;

        default:
            ctx->other++;
    }

    asn_free_value(tcp_message);
}

void
tsa_print_packet_stats(tsa_packets_counter *ctx)
{
    char buf[1024];
    int offt = 0;
    int rc;

#define PUSH_LINE(_line...) \
do {                                                                \
    if ((rc = snprintf(buf + offt, sizeof(buf) - offt, _line)) < 0) \
        TEST_FAIL("Failed writing to packet stats buffer");         \
    offt += rc;                                                     \
} while (0)

    PUSH_LINE("ACK %d\n", ctx->ack);
    PUSH_LINE("SYN %d\n", ctx->syn);
    PUSH_LINE("SYN-ACK %d\n", ctx->syn_ack);
    PUSH_LINE("PSH-ACK %d\n", ctx->push_ack);
    PUSH_LINE("FIN-ACK %d\n", ctx->fin_ack);
    PUSH_LINE("PSH-FIN-ACK %d\n", ctx->push_fin_ack);
    PUSH_LINE("RST-ACK %d\n", ctx->rst_ack);
    PUSH_LINE("RST %d", ctx->rst);

#undef PUSH_LINE

    RING("Captured packet stats:\n%s", buf);
}
