/** @file
 * @brief Proteos, TAD ISCSI protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for ISCSI protocol. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h" 

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_iscsi.h"

/* ISCSI-Message definitions */
static asn_named_entry_t _ndn_iscsi_message_ne_array [] = 
{
    { "param",   &asn_base_integer_s, {PRIVATE, NDN_TAG_ISCSI_PARAM} },
};

asn_type ndn_iscsi_message_s =
{
    "ISCSI-Message", {PRIVATE, NDN_TAD_ISCSI}, SEQUENCE,
    sizeof(_ndn_iscsi_message_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_iscsi_message_ne_array}
};

const asn_type *ndn_iscsi_message = &ndn_iscsi_message_s;


/* ISCSI-CSAP definitions */
static asn_named_entry_t _ndn_iscsi_csap_ne_array [] = 
{
    { "socket", &asn_base_int16_s, {PRIVATE, NDN_TAG_ISCSI_SOCKET} },
};

asn_type ndn_iscsi_csap_s =
{
    "ISCSI-CSAP", {PRIVATE, NDN_TAD_ISCSI}, SEQUENCE, 
    sizeof(_ndn_iscsi_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_iscsi_csap_ne_array}
};

const asn_type *ndn_iscsi_csap = &ndn_iscsi_csap_s;


