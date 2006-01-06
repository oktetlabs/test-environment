/** @file
 * @brief NDN Socket
 *
 * Declarations of ASN.1 types for NDN for sockets. 
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: ndn_ipstack.h 16769 2005-07-29 04:21:03Z konst $
 */ 

#ifndef __TE_NDN_SOCKET_H__
#define __TE_NDN_SOCKET_H__

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NDN_TAG_SOCKET_TOS,
    NDN_TAG_SOCKET_TTL,
    NDN_TAG_SOCKET_SRC_ADDR,
    NDN_TAG_SOCKET_DST_ADDR,
    NDN_TAG_SOCKET_LOCAL_ADDR,
    NDN_TAG_SOCKET_REMOTE_ADDR,
    NDN_TAG_SOCKET_SRC_PORT,
    NDN_TAG_SOCKET_DST_PORT,
    NDN_TAG_SOCKET_LOCAL_PORT,
    NDN_TAG_SOCKET_REMOTE_PORT,
    NDN_TAG_SOCKET_TYPE,
    NDN_TAG_SOCKET_TYPE_FD,
    NDN_TAG_SOCKET_TYPE_UDP,
    NDN_TAG_SOCKET_TYPE_TCP_SERVER,
    NDN_TAG_SOCKET_TYPE_TCP_CLIENT,
} ndn_socket_tags_t;

extern asn_type_p ndn_socket_message;
extern asn_type_p ndn_socket_csap; 

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_SOCKET_H__ */
