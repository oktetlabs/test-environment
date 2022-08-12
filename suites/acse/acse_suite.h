/** @file
 * @brief ACSE Test Suite
 *
 * Common definitions for ACSE test suite.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __ACSE_SUITE_H__
#define __ACSE_SUITE_H__

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
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
#include "tapi_sockaddr.h"
#include "tapi_rpc.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_env.h"

#if 0
#define TEST_START_VARS TEST_START_ENV_VARS
#define TEST_START_SPECIFIC TEST_START_ENV
#define TEST_END_SPECIFIC TEST_END_ENV
#else
#define TEST_START_VARS
#define TEST_START_SPECIFIC
#define TEST_END_SPECIFIC
#endif

#include "tapi_test.h"
#include "tapi_acse.h"

#endif /* __ACSE_SUITE_H__ */
