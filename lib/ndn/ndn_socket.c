/** @file
 * @brief NDN Socket
 *
 * Definitions of ASN.1 types for NDN for sockets. 
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

#include "te_config.h" 

#include "te_defs.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_socket.h"


static asn_named_entry_t _ndn_socket_message_ne_array[] = 
{
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

asn_type ndn_socket_message_s =
{
    "Socket-Message", { PRIVATE, 100 }, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_socket_message_ne_array),
    { _ndn_socket_message_ne_array }
};

asn_type_p ndn_socket_message = &ndn_socket_message_s;



static asn_named_entry_t _ndn_socket_type_ne_array[] = 
{
    { "file-descr", &asn_base_integer_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_FD } },
    { "udp",        &asn_base_null_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_UDP } },
    { "tcp-server", &asn_base_null_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_TCP_SERVER } },
    { "tcp-client", &asn_base_null_s,
      { PRIVATE, NDN_TAG_SOCKET_TYPE_TCP_CLIENT } },
};

asn_type ndn_socket_type_s =
{
    "TCP-CSAP", { PRIVATE, NDN_TAG_SOCKET_TYPE }, CHOICE,
    TE_ARRAY_LEN(_ndn_socket_type_ne_array),
    { _ndn_socket_type_ne_array }
};


static asn_named_entry_t _ndn_socket_csap_ne_array[] = 
{
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

asn_type ndn_socket_csap_s =
{
    "Socket-CSAP", { PRIVATE, 101 }, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_socket_csap_ne_array),
    { _ndn_socket_csap_ne_array }
};

asn_type_p ndn_socket_csap = &ndn_socket_csap_s;
