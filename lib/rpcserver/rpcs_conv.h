/** @file
 * @brief API for converting various types in RPC calls
 *
 * API used to convert between TARPC types and native types.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __RPCSERVER_RPCS_CONV_H__
#define __RPCSERVER_RPCS_CONV_H__

#include "rpc_server.h"

/**
 * Fill array of iovec structures with data from array of tarpc_iovec
 * structures.
 *
 * @param tarpc_iov     Array of tarpc_iovec structures.
 * @param iov           Array of iovec structures.
 * @param count         Number of elements in the arrays.
 * @param may_change    If @c TRUE, INIT_CHECKED_ARG() should allow
 *                      change of data in iovecs, otherwise it should
 *                      not. The first option is meant for readv()-like
 *                      calls, the second - for writev()-like ones.
 * @param arglist       Pointer to list of RPC call arguments which
 *                      are checked after target function call (to ensure
 *                      that the target function changes only what it is
 *                      supposed to).
 */
extern void rpcs_iovec_tarpc2h(const struct tarpc_iovec *tarpc_iov,
                               struct iovec *iov, size_t count,
                               te_bool may_change,
                               checked_arg_list *arglist);

#endif /* __RPCSERVER_RPCS_CONV_H__ */
