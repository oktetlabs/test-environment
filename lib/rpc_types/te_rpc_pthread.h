/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from pthread.h.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPC_PTHREAD_H__
#define __TE_RPC_PTHREAD_H__

#include "te_rpc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Thread cancelability states.
 */
typedef enum rpc_pthread_cancelstate {
    RPC_PTHREAD_CANCEL_STATE_UNKNOWN,    /**< Cancelability state unknown to RPC server */
    RPC_PTHREAD_CANCEL_ENABLE,           /**< The thread is cancelable */
    RPC_PTHREAD_CANCEL_DISABLE,          /**< The thread is not cancelable */
} rpc_pthread_cancelstate;

/** Convert RPC thread cancel state to string */
extern const char * pthread_cancelstate_rpc2str(rpc_pthread_cancelstate state);

/** Convert RPC thread cancel state to native thread cancel state */
extern int pthread_cancelstate_rpc2h(rpc_pthread_cancelstate state);

/** Convert native thread cancel state to RPC thread cancel state */
extern rpc_pthread_cancelstate pthread_cancelstate_h2rpc(int state);

/**
 * Thread cancelability types.
 */
typedef enum rpc_pthread_canceltype {
    RPC_PTHREAD_CANCEL_TYPE_UNKNOWN,        /**< Cancelability type unknown to RPC server */
    RPC_PTHREAD_CANCEL_DEFERRED,            /**< A cancellation request is deferred until
                                                 the thread next calls a function that
                                                 is a cancellation point. */
    RPC_PTHREAD_CANCEL_ASYNCHRONOUS,        /**< The thread can be canceled at any time */
} rpc_pthread_canceltype;

/** Convert RPC thread cancel type to string */
extern const char * pthread_canceltype_rpc2str(rpc_pthread_canceltype type);

/** Convert RPC thread cancel type to native thread cancel type */
extern int pthread_canceltype_rpc2h(rpc_pthread_canceltype type);

/** Convert native thread cancel type to RPC thread cancel type */
extern rpc_pthread_canceltype pthread_canceltype_h2rpc(int type);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_PTHREAD_H__ */