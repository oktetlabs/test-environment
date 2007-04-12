/** @file
 * @brief IPv6 Demo Test Suite
 *
 * Common includes and definitions.
 *
 * Copyright (C) 2007 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __IPV6_DEMO_TS_H__
#define __IPV6_DEMO_TS_H__

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_bufs.h"
#include "logger_api.h"
#include "te_sleep.h"
#include "tapi_jmp.h"
#include "tapi_sockaddr.h"
#include "tapi_rpc.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_env.h"
#include "tapi_test_log.h"
#include "tapi_rpc_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__IPV6_DEMO_TS_H__ */
