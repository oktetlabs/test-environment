/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for Ethernet-PCAP protocol.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
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
static asn_named_entry_t _ndn_pcap_filter_ne_array[] = {
    { "filter", &ndn_data_unit_char_string_s, {PRIVATE, 1}},
    { "filter-id", &asn_base_integer_s, {PRIVATE, 2}},
    { "bpf-id", &asn_base_integer_s, {PRIVATE, 3}},
};

asn_type ndn_pcap_filter_s = {
    "PCAP-Filter", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_pcap_filter_ne_array),
    {_ndn_pcap_filter_ne_array}
};

const asn_type * const ndn_pcap_filter = &ndn_pcap_filter_s;


/**
 * PCAP CSAP definition
 */
static asn_named_entry_t _ndn_pcap_csap_ne_array[] = {
    { "ifname",         &ndn_data_unit_char_string_s, {PRIVATE, 1} },
    { "iftype",         &asn_base_integer_s, {PRIVATE, 2} },
    { "receive-mode",   &asn_base_integer_s, {PRIVATE, 3} },
};

asn_type ndn_pcap_csap_s = {
    "PCAP-CSAP", {PRIVATE, 101}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_pcap_csap_ne_array),
    {_ndn_pcap_csap_ne_array}
};

const asn_type * const ndn_pcap_csap = &ndn_pcap_csap_s;
