/** @file
 * @brief Test API - RPC
 *
 * Declarations of RPC calls and auxiliary functions related
 * to time.
 *
 *
 * Copyright (C) 2003-2022 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_TIME_H__
#define __TE_TAPI_RPC_TIME_H__

#include "te_config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "rcf_rpc.h"
#include "tarpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert 'struct timeval' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv     pointer to 'struct timeval'
 *
 * @return null-terminated string
 */
extern const char *tarpc_timeval2str(const struct tarpc_timeval *tv);

/**
 * Convert 'struct timespec' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv     pointer to 'struct timespec'
 *
 * @return null-terminated string
 */
extern const char *timespec2str(const struct timespec *tv);

/**
 * Convert 'struct tarpc_timespec' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv     pointer to 'struct tarpc_timespec'
 *
 * @return null-terminated string
 */
extern const char *tarpc_timespec2str(const struct tarpc_timespec *tv);

/**
 * Convert 'struct tarpc_hwtstamp_config' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param hw_cfg     pointer to 'struct tarpc_hwtstamp_config'
 *
 * @return null-terminated string
 */
extern const char *tarpc_hwtstamp_config2str(
                        const tarpc_hwtstamp_config *hw_cfg);

/**
 * Get current time.
 *
 * @param rpcs    RPC server
 * @param tv      Where to save returned time
 * @param tz      Timezone (usually @c NULL)
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_gettimeofday(rcf_rpc_server *rpcs,
                            tarpc_timeval *tv, tarpc_timezone *tz);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RPC_TIME_H__ */
