/** @file
 * @brief Performance Test API to iperf tool routines
 *
 * Test API to control 'iperf' tool.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __IPERF_H__
#define __IPERF_H__

#include "tapi_performance.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize iperf server context with certain methods.
 *
 * @param server            Server tool context.
 */
extern void iperf_server_init(tapi_perf_server *server);

/**
 * Initialize iperf client context with certain methods.
 *
 * @param client            Client tool context.
 */
extern void iperf_client_init(tapi_perf_client *client);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __IPERF_H__ */
