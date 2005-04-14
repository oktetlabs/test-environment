/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for Ethernet protocol. 
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "te_errno.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_pcap.h"


/**
 * PCAP filter (matching string) definition
 */
static asn_named_entry_t _ndn_pcap_filter_ne_array [] = 
{
    { "filter", &ndn_data_unit_char_string_s, {PRIVATE, 1}},
    { "bpf-id", &asn_base_integer_s, {PRIVATE, 2}},
};

asn_type ndn_pcap_filter_s =
{
    "PCAP-Filter", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_pcap_filter_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_pcap_filter_ne_array}
};

const asn_type * const ndn_pcap_filter = &ndn_pcap_filter_s;


/**
 * PCAP CSAP definition
 */
static asn_named_entry_t _ndn_pcap_csap_ne_array [] = 
{
    { "device-id",     &ndn_data_unit_char_string_s, {PRIVATE, 1} },
    { "device-type",   &ndn_data_unit_char_string_s, {PRIVATE, 2} },
    { "receive-mode",  &asn_base_integer_s, {PRIVATE, 3} },
};

asn_type ndn_eth_csap_s =
{
    "PCAP-CSAP", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_pcap_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_pcap_csap_ne_array}
};

const asn_type * const ndn_pcap_csap = &ndn_pcap_csap_s;

