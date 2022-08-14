/** @file
 * @brief TCP states API library
 *
 * TCP states API library - internal definitions.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_TCP_STATES_INTERNAL_H__
#define __TAPI_TCP_STATES_INTERNAL_H__

#include "te_config.h"

#include "netinet/tcp.h"
#include <sys/time.h>
#include "tapi_tcp.h"
#include "tapi_tcp_states.h"

/**
 * Wait for changing of TCP state not due to timeout at most
 * MAX_CHANGE_TIMEOUT milliseconds.
 */
#define MAX_CHANGE_TIMEOUT 10000

/**
 * Get time of start before entering infinite loop.
 *
 * @param name_     Unique name used in auxiliary variables.
 */
#define INFINITE_LOOP_BEGIN(name_) \
    struct timeval tv_start_ ## name_;          \
    struct timeval tv_cur_ ## name_;            \
                                                \
    gettimeofday(&tv_start_ ## name_, NULL)

/**
 * Check whether infinite loop runs too long, break if timeout
 * is reached.
 *
 * @param name_     Unique name used in auxiliary variables.
 * @param timeout_  Timeout in milliseconds.
 */
#define INFINITE_LOOP_BREAK(name_, timeout_) \
    gettimeofday(&tv_cur_ ## name_, NULL);                    \
    if (TIMEVAL_SUB(tv_cur_ ## name_, tv_start_ ## name_) >   \
            TE_MS2US(timeout_))                               \
        break

/**
 * Sleep 2 sec when we don't know whether TCP state can change instantly
 * or not (no definite timeout, just packets retransmitting after
 * forwarding unlock).
 */
#define SLEEP_MSEC 2000

/**
 * Wait for change of TCP state of socket on the IUT side
 *
 * @param ss        Pointer to TSA session structure
 * @param timeout   Maximum time to wait (in milliseconds)
 *
 * @return Status code.
 */
extern te_errno iut_wait_change_gen(tsa_session *ss, int timeout);

/**
 * Wait at most 2 * MSL time for change of TCP state of socket
 * on the IUT side
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
extern te_errno iut_wait_change(tsa_session *ss);

/**
 * Wait for changes in connectivity if necessary.
 *
 * @param ss    Pointer to TSA session structure of current working session
 */
extern void wait_connectivity_changes(tsa_session *ss);

enum {
TSA_IUT = 0, /**< Open socket on the IUT side
                  when calling tsa_sock_create(). */
TSA_TST      /**< Open socket on the TST side when
                  calling tsa_sock_create(). */
};

/**
 * Create new socket (if it was not created previously),
 * set needed options, @b bind() it.
 *
 * @param ss            Pointer to TSA session structure
 * @param rpc_selector  Determine RPC server on which socket must
 *                      be created (that on IUT or on TST side)
 *
 * @return Status code.
 */
extern te_errno tsa_sock_create(tsa_session *ss, int rpc_selector);

/**
 * Set pointers in tsa_handlers structure to socket mode specific
 * TCP state change handlers.
 *
 * @param handlers        Pointer to tsa_handlers structure.
 */
extern void tsa_set_sock_handlers(tsa_handlers *handlers);

/**
 * Set pointers in tsa_handlers structure to CSAP mode specific
 * TCP state change handlers.
 *
 * @param handlers        Pointer to tsa_handlers structure.
 */
extern void tsa_set_csap_handlers(tsa_handlers *handlers);

/** Default listen backlog value. */
#define TSA_BACKLOG_DEF 1

#endif /* !__TAPI_TCP_STATES_INTERNAL_H__ */
