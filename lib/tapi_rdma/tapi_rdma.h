/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2024 Advanced Micro Devices, Inc. */
/** @file
 * @brief Generic Test API to interact with RDMA links
 *
 * @defgroup tapi_rdma Test API to interact with RDMA links
 * @ingroup te_ts_tapi
 * @{
 *
 * Generic API to interact with RDMA links.
 */

#ifndef __TAPI_RDMA_H__
#define __TAPI_RDMA_H__

#include "te_errno.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tapi_rdma_link_stat {
    char    *name;
    int64_t  value;
} tapi_rdma_link_stat_t;

typedef struct tapi_rdma_link_stats {
    tapi_rdma_link_stat_t *stats;
    unsigned int           size;
} tapi_rdma_link_stats_t;

/**
 * Collect statistics reported by an RDMA link.
 *
 * @param[in]  rpcs         RPC server.
 * @param[in]  link         Name of the RDMA link.
 * @param[out] stats        Where to store the statistics array.
 *
 * @return Status code.
 */
extern te_errno tapi_rdma_link_get_stats(rcf_rpc_server *rpcs, const char *link,
                                         tapi_rdma_link_stats_t **stats);

/**
 * Compare two sets of statistics.
 *
 * @param[in]  old_stats    Old statistics array.
 * @param[in]  new_stats    New statistics array.
 * @param[out] diff_stats   Where to store the statistics diff array.
 *
 * @return Status code.
 */
extern te_errno tapi_rdma_link_diff_stats(const tapi_rdma_link_stats_t *old_stats,
                                          const tapi_rdma_link_stats_t *new_stats,
                                          tapi_rdma_link_stats_t **diff_stats);

/**
 * Log statistics whose name contains a given substring. Use the log level
 * provided by the caller.
 *
 * @param stats             Statistics array.
 * @param description       Description line to be logged before the stats.
 * @param pattern           Substring that must be in the name.
 * @param non_empty         Only log if there is something to log.
 * @param log_level         Log level to use.
 */
extern void tapi_rdma_link_log_stats(tapi_rdma_link_stats_t *stats,
                                     const char *description,
                                     const char *pattern,
                                     bool non_empty,
                                     te_log_level log_level);

/**
 * Free the memory occupied by an array of RDMA link statistics.
 *
 * @param stats           Statistics array.
 */
extern void tapi_rdma_link_free_stats(tapi_rdma_link_stats_t *stats);

/**@} <!-- END tapi_rdma --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_RDMA_H__ */
