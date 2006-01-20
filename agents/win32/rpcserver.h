/** @file
 * @brief Windows Test Agent
 *
 * Standard OS includes.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2006 Level5 Networks Corp.
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */


#ifndef __TA_WIN32_RPCSERVER_H__
#define __TA_WIN32_RPCSERVER_H__

/* Include necessary headers */
INCLUDE(winsock2.h)
INCLUDE(winerror.h)
INCLUDE(mswsock.h)
INCLUDE(ws2tcpip.h)
INCLUDE(mstcpip.h)
INCLUDE(stdio.h)
INCLUDE(stdarg.h)
INCLUDE(stdlib.h)
INCLUDE(string.h)
INCLUDE(time.h)
INCLUDE(assert.h)

/* Prevent including of these headers by #include directive */

/* Native Windows headers */
#define _WINSOCK2_H
#define _WINERROR_H
#define _MSWSOCK_H
#define _WS2TCPIP_H
#define _MSTCPIP_H

/* Standard C headers */ 
#define _STDIO_H_
#define _STDARG_H
#define _STDLIB_H_
#define _STRING_H_
#define __ERRNO_H__

/* UNIX-specific headers */
#define _PTHREAD_H
#define _SYS_TYPES_H
#define _SIGNAL_H_
#define _SCHED_H
#define __TE_RPC_SYS_RESOURCE_H__
#define __TE_RPC_SYS_WAIT_H__
#define __TE_SHELL_CMD_H__


/* We cannot use CYGWIN configure output */
#define __TE_CONFIG_H__

/** Define our own features */
#define HAVE_STDARG_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

/* The size of a `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of a `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of a `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of a `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T 4

/* The size of a `socklen_t', as computed by sizeof. */
#define SIZEOF_SOCKLEN_T 4

/* The size of a `te_log_id', as computed by sizeof. */
#define SIZEOF_TE_LOG_ID 4

/* The size of a `te_log_level', as computed by sizeof. */
#define SIZEOF_TE_LOG_LEVEL 2

/* The size of a `te_log_nfl', as computed by sizeof. */
#define SIZEOF_TE_LOG_NFL 2

/* The size of a `te_log_seqno', as computed by sizeof. */
#define SIZEOF_TE_LOG_SEQNO 4

/* The size of a `te_log_ts_sec', as computed by sizeof. */
#define SIZEOF_TE_LOG_TS_SEC 4

/* The size of a `te_log_ts_usec', as computed by sizeof. */
#define SIZEOF_TE_LOG_TS_USEC 4

/* The size of a `te_log_version', as computed by sizeof. */
#define SIZEOF_TE_LOG_VERSION 1

/* The size of a `void *', as computed by sizeof. */
#define SIZEOF_VOID_P 4

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1
 
typedef int ssize_t;
typedef int pid_t;

#define inline
#define __const
#undef __stdcall 

#define int8_t     INT8
#define int16_t    INT16
#define int32_t    INT32
#define int64_t    INT64
#define uint8_t    UINT8
#define uint16_t   UINT16
#define uint32_t   UINT32
#define uint64_t   UINT64

#include "te_defs.h"

#endif
