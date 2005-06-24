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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
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
#include "ndn_eth.h"



asn_type ndn_eth_address_s =
{
    "Ethernet-Address", 
    {PRIVATE, 500}, 
    OCT_STRING, 
    6, 
    {NULL}
};

const asn_type * const ndn_eth_address = &ndn_eth_address_s;

NDN_DATA_UNIT_TYPE(eth_address, ndn_eth_address_s, Ethernet-Address);


/* 
VLAN-extended Ethernet header, field 
CFI 

CFI-Mode ::= INTEGER {false(0), true(1)}
 */
static asn_enum_entry_t _ndn_vlan_cfi_enum_entries[] = 
{
    {"false", 0},
    {"true", 1},
};

static asn_type ndn_vlan_cfi_s = {
    "VLAN-CFI-mode", {APPLICATION, 15}, ENUMERATED,
    sizeof(_ndn_vlan_cfi_enum_entries)/sizeof(asn_enum_entry_t), 
    {enum_entries: _ndn_vlan_cfi_enum_entries}
};



static asn_named_entry_t _ndn_eth_header_ne_array [] = 
{
    { "src-addr", &ndn_data_unit_eth_address_s,
        {PRIVATE, NDN_TAG_ETH_SRC} },
    { "dst-addr", &ndn_data_unit_eth_address_s,
        {PRIVATE, NDN_TAG_ETH_DST} },
    { "eth-type", &ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_ETH_TYPE_LEN} },
    { "cfi",      &ndn_vlan_cfi_s,
        {PRIVATE, NDN_TAG_ETH_CFI} },
    { "priority", &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_ETH_PRIO} },
    { "vlan-id",  &ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_ETH_VLAN_ID} },
};

asn_type ndn_eth_header_s =
{
    "Ethernet-Header", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_eth_header_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_eth_header_ne_array}
};

const asn_type * const ndn_eth_header = &ndn_eth_header_s;








static asn_named_entry_t _ndn_eth_csap_ne_array [] = 
{
    { "device-id",   &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_TAG_ETH_DEVICE} },
    { "receive-mode",&asn_base_integer_s,
        {PRIVATE, NDN_TAG_ETH_RECV_MODE} },
    { "local-addr",  &ndn_data_unit_eth_address_s,
        {PRIVATE, NDN_TAG_ETH_LOCAL} },
    { "remote-addr", &ndn_data_unit_eth_address_s,
        {PRIVATE, NDN_TAG_ETH_REMOTE} },
    { "eth-type",    &ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_ETH_TYPE_LEN} },
    { "cfi",         &ndn_vlan_cfi_s,
        {PRIVATE, NDN_TAG_ETH_CFI} },
    { "priority",    &ndn_data_unit_int8_s,
        {PRIVATE, NDN_TAG_ETH_PRIO} },
    { "vlan-id",     &ndn_data_unit_int16_s,
        {PRIVATE, NDN_TAG_ETH_VLAN_ID} },
};

asn_type ndn_eth_csap_s =
{
    "Ethernet-CSAP", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_eth_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_eth_csap_ne_array}
};

const asn_type * const ndn_eth_csap = &ndn_eth_csap_s;


/** 
 * Convert Ehternet-Header ASN value to plain C structrue. 
 * 
 * @param pkt           ASN value of type Ethernet Header or Generic-PDU 
 *                      with choice "eth". 
 * @param eth_header    converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 
int 
ndn_eth_packet_to_plain(const asn_value *pkt, 
                        ndn_eth_header_plain *eth_header)
{
    int rc;
    int val;
    size_t len;

    len = ETH_ALEN;
    rc = asn_read_value_field(pkt, eth_header->dst_addr, &len, 
                              "dst-addr.#plain"); 
    len = ETH_ALEN;
    if (rc == 0) 
        rc = asn_read_value_field(pkt, eth_header->src_addr, &len, 
                                  "src-addr.#plain"); 
    len = 2;
    if (rc == 0) 
        rc = asn_read_value_field(pkt, &eth_header->eth_type_len, &len, 
                                  "eth-type.#plain"); 
    len = sizeof(val);
    if (rc == 0) 
        rc = asn_read_value_field(pkt, &val, &len, "cfi"); 

    if (rc == EASNINCOMPLVAL)
    {
        eth_header->is_tagged = 0;
        return 0;
    }
    else
        eth_header->is_tagged = 1;
    
    if (rc) 
        return rc;

    eth_header->cfi = val;

    len = sizeof(eth_header->priority);
    rc = asn_read_value_field(pkt, &eth_header->priority, &len, 
                              "priority.#plain"); 

    len = sizeof(eth_header->vlan_id);
    if (rc == 0)
        rc = asn_read_value_field(pkt, &eth_header->vlan_id, &len, 
                                  "vlan-id.#plain"); 

    return rc;
}


/** 
 * Convert plain C structure to Ehternet-Header ASN value. 
 * 
 * @param eth_header    structure to be converted. 
 *
 * @return pointer to new ASN value object or NULL.
 */ 
asn_value *
ndn_eth_plain_to_packet(const ndn_eth_header_plain *eth_header)
{ 
    asn_value *asn_eth_hdr;
    int rc;

    if (eth_header == NULL)
        return NULL;

    asn_eth_hdr = asn_init_value(ndn_eth_header);
    if (asn_eth_hdr == NULL)
        return NULL;

    rc = asn_write_value_field(asn_eth_hdr, eth_header->dst_addr, 
                               sizeof(eth_header->dst_addr) , 
                               "dst-addr.#plain");
    if (rc == 0) 
        rc = asn_write_value_field(asn_eth_hdr, eth_header->src_addr, 
                                   sizeof(eth_header->src_addr) , 
                                   "src-addr.#plain");

    if (rc == 0) 
        rc = asn_write_value_field(asn_eth_hdr, &eth_header->eth_type_len,
                                   sizeof(eth_header->eth_type_len), 
                                   "eth-type.#plain"); 
    if (rc == 0 && eth_header->is_tagged)
    {
        rc = asn_write_value_field(asn_eth_hdr, &eth_header->cfi, 
                                   sizeof(eth_header->cfi), 
                                   "cfi.#plain"); 

        if (rc == 0)
            rc = asn_write_value_field(asn_eth_hdr, &eth_header->priority,
                                       sizeof(eth_header->priority), 
                                       "priority.#plain"); 
        if (rc == 0)
            rc = asn_write_value_field(asn_eth_hdr, &eth_header->vlan_id, 
                                       sizeof(eth_header->vlan_id), 
                                       "vlan-id.#plain"); 
    }

    if (rc)
    {
        asn_free_value(asn_eth_hdr);
        return NULL; 
    }

    return asn_eth_hdr;
}


