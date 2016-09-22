/** @file
 * @brief Windows support
 *
 * Standard OS includes for libraries and applications built by native
 * MS Windows compiler.
 *
 * This file should be used only on pre-processing stage.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * $Id: rpcserver.h 23087 2006-01-20 15:33:00Z helen $
 */


#ifndef __TE_WIN_H__
#define __TE_WIN_H__

/* Include necessary headers */
INCLUDE(winsock2.h)
INCLUDE(winerror.h)
INCLUDE(mswsock.h)
INCLUDE(ws2tcpip.h)
INCLUDE(mstcpip.h)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
INCLUDE(tcpioctl.h)
INCLUDE(stdio.h)
INCLUDE(stdarg.h)
INCLUDE(stdlib.h)
INCLUDE(string.h)
INCLUDE(time.h)
INCLUDE(assert.h)
INCLUDE(limits.h)

/* Prevent including of these headers by #include directive */

/* Native Windows headers */
#define _WINSOCK2_H
#define _WINERROR_H
#define _MSWSOCK_H
#define _WS2TCPIP_H
#define _MSTCPIP_H

/* Standard C headers */
#define _STDIO_H_
#define _STDIO_H
#define _STDARG_H
#define _STDLIB_H_
#define _STDLIB_H
#define _STRING_H_
#define _STRING_H
#define __ERRNO_H__

/* UNIX-specific headers */
#define _PTHREAD_H
#define _SYS_TYPES_H
#define _SIGNAL_H_
#define _SCHED_H

/* We cannot use CYGWIN configure output */
#define __TE_CONFIG_H__

/** Define our own features */
#define HAVE_STDARG_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define STDC_HEADERS 1

#include "te_win_sizeof.h"
#include "te_win_defs.h"

#endif
