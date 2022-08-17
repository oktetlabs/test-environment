/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief NDN Socket
 *
 * Definitions of ASN.1 types for NDN for sockets.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "te_defs.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_socket.h"


static asn_named_entry_t _ndn_socket_message_ne_array[] = {
    { "type-of-service", &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_SOCKET_TOS } },
    { "time-to-live",    &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_SOCKET_TTL } },
    { "src-addr",        &ndn_data_unit_ip_address_s,
      { PRIVATE, NDN_TAG_SOCKET_SRC_ADDR } },
    { "dst-addr",        &ndn_data_unit_ip_address_s,
      { PRIVATE, NDN_TAG_SOCKET_DST_ADDR } },
    { "src-port",        &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_SOCKET_SRC_PORT } },
    { "dst-port",        &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_SOCKET_DST_PORT } },
    { "file-descr",      &asn_base_integer_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_FD } },
};

asn_type ndn_socket_message_s = {
    "Socket-Message", { PRIVATE, 100 }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_socket_message_ne_array),
    { _ndn_socket_message_ne_array }
};

asn_type * ndn_socket_message = &ndn_socket_message_s;



static asn_named_entry_t _ndn_socket_type_ne_array[] = {
    { "file-descr", &asn_base_integer_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_FD } },
    { "udp",        &asn_base_null_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_UDP } },
    { "tcp-server", &asn_base_null_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_TCP_SERVER } },
    { "tcp-client", &asn_base_null_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_TCP_CLIENT } },
};

asn_type ndn_socket_type_s = {
    "TCP-CSAP", { PRIVATE, NDN_TAG_SOCKET_TYPE }, CHOICE,
    TE_ARRAY_LEN(_ndn_socket_type_ne_array),
    { _ndn_socket_type_ne_array }
};


static asn_named_entry_t _ndn_socket_csap_ne_array[] = {
    { "type",            &ndn_socket_type_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE } },
    { "type-of-service", &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_SOCKET_TOS } },
    { "time-to-live",    &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_SOCKET_TTL } },
    { "local-addr",      &ndn_data_unit_ip_address_s,
      { PRIVATE, NDN_TAG_SOCKET_LOCAL_ADDR } },
    { "remote-addr",     &ndn_data_unit_ip_address_s,
      { PRIVATE, NDN_TAG_SOCKET_REMOTE_ADDR } },
    { "local-port",      &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_SOCKET_LOCAL_PORT } },
    { "remote-port",     &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_SOCKET_REMOTE_PORT } },
};

asn_type ndn_socket_csap_s = {
    "Socket-CSAP", { PRIVATE, 101 }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_socket_csap_ne_array),
    { _ndn_socket_csap_ne_array }
};

asn_type * ndn_socket_csap = &ndn_socket_csap_s;
