/** @file
 * @brief Performance Test API to iperf3 tool routines
 *
 * Test API to control 'iperf3' tool.
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
