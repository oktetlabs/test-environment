/** @file
 * @brief Performance Test API to iperf3 tool routines
 *
 * Test API to control 'iperf3' tool.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __IPERF3_H__
#define __IPERF3_H__

#include "tapi_performance.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize iperf3 server context with certain methods.
 *
 * @param server            Server tool context.
 */
extern void iperf3_server_init(tapi_perf_server *server);

/**
 * Initialize iperf3 client context with certain methods.
 *
 * @param client            Client tool context.
 */
extern void iperf3_client_init(tapi_perf_client *client);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __IPERF3_H__ */
