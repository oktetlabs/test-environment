/** @file
 * @brief Test Environment
 *
 * Common definitions for RPC test suite.
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Evgeniy Vladimirovich Bobkov <kavri@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __RPC_SUITE_H__
#define __RPC_SUITE_H__

#include "te_config.h"

//#define _GNU_SOURCE     1

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_bufs.h"
#include "logger_api.h"
#include "tapi_jmp.h"
#include "tapi_bufs.h"
#include "tapi_sockaddr.h"
#include "tapi_rpc.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_env.h"

#define TEST_START_VARS TEST_START_ENV_VARS
#define TEST_START_SPECIFIC TEST_START_ENV
#define TEST_END_SPECIFIC TEST_END_ENV

#include "tapi_test.h"

#endif /* __RPC_SUITE_H__ */
