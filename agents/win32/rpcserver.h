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
INCLUDE(stdio.h)

/* Prevent including of these headers by #include directive */
#define _WINSOCK2_H
#define _WINERROR_H
#define _MSWSOCK_H
#define _WS2TCPIP_H
#define _STDIO_H_

#endif
