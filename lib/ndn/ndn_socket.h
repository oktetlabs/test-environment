/** @file
 * @brief NDN Socket
 *
 * Declarations of ASN.1 types for NDN for sockets.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
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

extern asn_type * ndn_socket_message;
extern asn_type * ndn_socket_csap;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_SOCKET_H__ */
