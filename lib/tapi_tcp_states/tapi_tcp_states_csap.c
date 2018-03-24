/** @file
 * @brief TCP states API library
 *
 * TCP states API library - implementation of handlers used to change
 * TCP state, for the case when a CSAP is used on a peer.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "te_sleep.h"
#include "tapi_rpc_socket.h"
#include "tapi_rpc_unistd.h"

#include "tapi_tcp_states.h"
#include "tapi_tcp_states_internal.h"

/**
 * Action to be done when SYN must be sent from socket on the IUT side
 * according to TCP specification.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
iut_syn_csap_handler(tsa_session *ss)
{
    te_errno rc;

    if (ss->state.csap.csap_tst_s != -1)
    {
        rc = tapi_tcp_destroy_connection(
                          ss->state.csap.csap_tst_s);

        if (rc != 0)
        {
            ERROR("%s(): destroying of CSAP connection failed",
                  __FUNCTION__);
            return rc;
        }
        ss->state.csap.csap_tst_s = -1;
    }

    rc = tapi_tcp_init_connection(ss->config.pco_tst->ta,
                                  TAPI_TCP_SERVER,
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
    ss->state.tst_wait_connect = TRUE;

    RPC_AWAIT_ERROR(ss->config.pco_iut);
    if (rpc_connect(ss->config.pco_iut, ss->state.iut_s,
                    ss->config.tst_addr) != 0 &&
        RPC_ERRNO(ss->config.pco_iut) != RPC_EINPROGRESS)
        return RPC_ERRNO(ss->config.pco_iut);

    ss->state.iut_wait_connect = TRUE;

    rc = tsa_update_cur_state(ss);
    if (rc != 0)
        return rc;

    if (tsa_state_cur(ss) == RPC_TCP_CLOSE)
        rc = iut_wait_change_gen(ss, MAX_CHANGE_TIMEOUT);

    return rc;
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
tst_syn_csap_handler(tsa_session *ss)
{
    te_errno rc = 0;

    if (ss->state.csap.csap_tst_s != -1)
    {
        rc = tapi_tcp_destroy_connection(
                          ss->state.csap.csap_tst_s);
        if (rc != 0)
        {
            ERROR("%s(): destroying of CSAP connection failed",
                  __FUNCTION__);
            return rc;
        }
        ss->state.csap.csap_tst_s = -1;
    }

    rc = tapi_tcp_init_connection(ss->config.pco_tst->ta,
                                TAPI_TCP_CLIENT,
                                ss->config.tst_addr,
                                ss->config.iut_addr,
                                ss->config.tst_if->if_name,
                                ss->config.alien_link_addr,
                                ss->state.tst_type == TSA_TST_CSAP ?
                                ss->config.iut_link_addr :
                                ss->config.gw_tst_link_addr,
                                0, &ss->state.csap.csap_tst_s);

    ss->state.tst_wait_connect = TRUE;
    return rc;
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
iut_syn_ack_csap_handler(tsa_session *ss)
{
    te_errno rc = 0;

    /* Listener socket does not change its status when it sends SYN-ACK. */
    if (!(tsa_state_from(ss) == RPC_TCP_LISTEN &&
          tsa_state_to(ss) == RPC_TCP_SYN_RECV &&
          tsa_state_cur(ss) == RPC_TCP_LISTEN))
        rc = iut_wait_change_gen(ss, MAX_CHANGE_TIMEOUT);

    return rc;
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
tst_syn_ack_csap_handler(tsa_session *ss)
{
    te_errno rc = 0;

    rc = tapi_tcp_wait_open(ss->state.csap.csap_tst_s,
                            MAX_CHANGE_TIMEOUT);
    if (rc != 0)
        ss->state.csap.csap_tst_s = -1;
    ss->state.tst_wait_connect = FALSE;

    RPC_AWAIT_ERROR(ss->config.pco_iut);
    if (rpc_connect(ss->config.pco_iut, ss->state.iut_s,
                    ss->config.tst_addr) != 0 &&
        RPC_ERRNO(ss->config.pco_iut) != RPC_EALREADY &&
        RPC_ERRNO(ss->config.pco_iut) != RPC_EINPROGRESS)
        rc = RPC_ERRNO(ss->config.pco_iut);
    ss->state.iut_wait_connect = FALSE;

    return rc;
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
iut_ack_csap_handler(tsa_session *ss)
{
    return iut_wait_change_gen(ss, MAX_CHANGE_TIMEOUT);
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
tst_ack_csap_handler(tsa_session *ss)
{
    te_errno rc = 0;

    if (ss->state.tst_wait_connect == TRUE)
    {
        rc = tapi_tcp_wait_open(ss->state.csap.csap_tst_s,
                                MAX_CHANGE_TIMEOUT);

        if (rc != 0)
        {
            ss->state.csap.csap_tst_s = -1;
            return rc;
        }
        ss->state.tst_wait_connect = FALSE;

        if (tsa_state_cur(ss) == RPC_TCP_LISTEN)
        {
            ss->state.iut_s_aux = ss->state.iut_s;

            INFINITE_LOOP_BEGIN(loop);
            while (TRUE)
            {
                RPC_AWAIT_ERROR(ss->config.pco_iut);
                ss->state.iut_s = rpc_accept(ss->config.pco_iut,
                                             ss->state.iut_s_aux,
                                             NULL, NULL);
                if (ss->state.iut_s >= 0)
                {
                    break;
                }
                else if (RPC_ERRNO(ss->config.pco_iut) != RPC_EAGAIN)
                {
                    ERROR("%s(): accept returned unexpected errno %r",
                          __FUNCTION__, RPC_ERRNO(ss->config.pco_iut));
                    break;
                }

                INFINITE_LOOP_BREAK(loop, MAX_CHANGE_TIMEOUT);
            }

            if (ss->state.iut_s < 0)
            {
                rc = RPC_ERRNO(ss->config.pco_iut);
                if (rc == RPC_EAGAIN)
                    rc = TE_RC(TE_TAPI, TE_ETIMEDOUT);

                return rc;
            }
        }
        else
        {
            RING("Waiting for connect() call termination on "
                 "IUT side");
            RPC_AWAIT_ERROR(ss->config.pco_iut);
            if (rpc_connect(ss->config.pco_iut, ss->state.iut_s,
                            ss->config.tst_addr) != 0 &&
                RPC_ERRNO(ss->config.pco_iut) != RPC_EALREADY)
                rc = RPC_ERRNO(ss->config.pco_iut);
            ss->state.iut_wait_connect = FALSE;
        }
    }
    else
    {
        rc = tapi_tcp_wait_msg(ss->state.csap.csap_tst_s,
                               SLEEP_MSEC);
        if (rc != 0 && rc != TE_RC(TE_TAPI, TE_ETIMEDOUT))
            return rc;

        rc = tapi_tcp_send_ack(
                  ss->state.csap.csap_tst_s,
                  tapi_tcp_next_ackn(
                        ss->state.csap.csap_tst_s));
        if (rc != 0)
            return rc;
    }

    return iut_wait_change_gen(ss, MAX_CHANGE_TIMEOUT);
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
iut_fin_csap_handler(tsa_session *ss)
{
    te_errno rc = 0;

    RPC_AWAIT_ERROR(ss->config.pco_iut);
    if (rpc_shutdown(ss->config.pco_iut, ss->state.iut_s,
                     RPC_SHUT_WR) < 0)
        rc = RPC_ERRNO(ss->config.pco_iut);

    return rc;
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
tst_fin_csap_handler(tsa_session *ss)
{
    return tapi_tcp_send_fin(ss->state.csap.csap_tst_s,
                             MAX_CHANGE_TIMEOUT);
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
tst_fin_ack_csap_handler(tsa_session *ss)
{
    te_errno rc = 0;

    rc = tapi_tcp_wait_msg(ss->state.csap.csap_tst_s,
                           SLEEP_MSEC);
    if (rc != 0 && rc != TE_RC(TE_TAPI, TE_ETIMEDOUT))
        return rc;

    return tapi_tcp_send_fin_ack(ss->state.csap.csap_tst_s,
                                 MAX_CHANGE_TIMEOUT);
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
tst_rst_csap_handler(tsa_session *ss)
{
    te_errno rc;

    /* We send packet with RST set having correct
     * sequence number and acknowledging all received
     * by TESTER previously */

    rc = tapi_tcp_wait_msg(ss->state.csap.csap_tst_s,
                           SLEEP_MSEC);
    if (rc != 0 && rc != TE_RC(TE_TAPI, TE_ETIMEDOUT))
        return rc;

    rc = tapi_tcp_send_rst(ss->state.csap.csap_tst_s);
    if (rc != 0)
        return rc;

    return iut_wait_change_gen(ss, MAX_CHANGE_TIMEOUT);
}

/**
 * Action to be done when socket on the IUT side should become listening
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
static te_errno
iut_listen_csap_handler(tsa_session *ss)
{
    te_errno rc = 0;

    RPC_AWAIT_ERROR(ss->config.pco_iut);
    if (rpc_listen(ss->config.pco_iut, ss->state.iut_s,
                   TSA_BACKLOG_DEF) < 0)
        rc = RPC_ERRNO(ss->config.pco_iut);

    return rc;
}

/* See description in tapi_tcp_states_internal.h */
void
tsa_set_csap_handlers(tsa_handlers *handlers)
{
    memset(handlers, 0, sizeof(*handlers));

    handlers->iut_syn = &iut_syn_csap_handler;
    handlers->tst_syn = &tst_syn_csap_handler;
    handlers->iut_syn_ack = &iut_syn_ack_csap_handler;
    handlers->tst_syn_ack = &tst_syn_ack_csap_handler;
    handlers->iut_ack = &iut_ack_csap_handler;
    handlers->tst_ack = &tst_ack_csap_handler;
    handlers->iut_fin = &iut_fin_csap_handler;
    handlers->tst_fin = &tst_fin_csap_handler;
    handlers->tst_fin_ack = &tst_fin_ack_csap_handler;
    handlers->tst_rst = &tst_rst_csap_handler;
    handlers->iut_listen = &iut_listen_csap_handler;
}
