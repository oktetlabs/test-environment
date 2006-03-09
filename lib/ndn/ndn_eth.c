/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for Ethernet protocol. 
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS 
 * in the root directory of the distribution).
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
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


static asn_named_entry_t _ndn_eth_tagged_ne_array[] =
{
    { "priority", &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_ETH_PRIO } },
    { "cfi", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ETH_CFI } },
    { "vlan-id", &ndn_data_unit_int12_s,
      { PRIVATE, NDN_TAG_ETH_VLAN_ID } },

    { "e-rif-rc-rt", &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_ETH_ERIF_RC_RT } },
    { "e-rif-rc-lth", &ndn_data_unit_int5_s,
      { PRIVATE, NDN_TAG_ETH_ERIF_RC_LTH } },
    { "e-rif-rc-d", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ETH_ERIF_RC_D } },
    { "e-rif-rc-lf", &ndn_data_unit_int6_s,
      { PRIVATE, NDN_TAG_ETH_ERIF_RC_LF } },
    { "e-rif-rc-ncfi", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_ETH_ERIF_RC_NCFI } },
    { "e-rif-rd", &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_ETH_ERIF_RD } },
};

asn_type ndn_eth_tagged_s =
{
    "Ethernet-Tagged", {PRIVATE, 100}, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_eth_tagged_ne_array),
    { _ndn_eth_tagged_ne_array }
};

const asn_type * const ndn_eth_tagged = &ndn_eth_tagged_s;


static asn_named_entry_t _ndn_eth_snap_ne_array[] =
{
    { "snap-oui", &ndn_data_unit_int24_s,
      { PRIVATE, NDN_TAG_ETH_SNAP_OUI } },
    { "snap-pid", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ETH_SNAP_PID } },
};

asn_type ndn_eth_snap_s =
{
    "Ethernet-SNAP", {PRIVATE, 100}, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_eth_snap_ne_array),
    { _ndn_eth_snap_ne_array }
};

const asn_type * const ndn_eth_snap = &ndn_eth_snap_s;


static asn_named_entry_t _ndn_eth_frame_type_ne_array[] =
{
    { "untagged", &asn_base_null_s,
      { PRIVATE, NDN_TAG_ETH_UNTAGGED } },
    { "tagged", &ndn_eth_tagged_s,
      { PRIVATE, NDN_TAG_ETH_TAGGED } },
    { "snap", &ndn_eth_snap_s,
      { PRIVATE, NDN_TAG_ETH_SNAP } },
};

asn_type ndn_eth_frame_type_s =
{
    "Eth-Frame-Type", {PRIVATE, 100}, CHOICE, 
    TE_ARRAY_LEN(_ndn_eth_frame_type_ne_array),
    { _ndn_eth_frame_type_ne_array }
};

const asn_type * const ndn_eth_frame_type = &ndn_eth_frame_type_s;


static asn_named_entry_t _ndn_eth_header_ne_array[] = 
{
    { "src-addr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_ETH_SRC } },
    { "dst-addr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_ETH_DST } },
    { "length-type", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ETH_LENGTH_TYPE } },
    { "frame-type", &ndn_eth_frame_type_s,
      { PRIVATE, NDN_TAG_ETH_FRAME_TYPE } },
};

asn_type ndn_eth_header_s =
{
    "Ethernet-Header", {PRIVATE, 100}, SEQUENCE, 
    TE_ARRAY_LEN(_ndn_eth_header_ne_array),
    { _ndn_eth_header_ne_array }
};

const asn_type * const ndn_eth_header = &ndn_eth_header_s;


static asn_named_entry_t _ndn_eth_csap_ne_array [] = 
{
    { "device-id", &ndn_data_unit_char_string_s,
      { PRIVATE, NDN_TAG_ETH_DEVICE } },
    { "receive-mode", &asn_base_integer_s,
      { PRIVATE, NDN_TAG_ETH_RECV_MODE } },
    { "local-addr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_ETH_LOCAL } },
    { "remote-addr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_ETH_REMOTE } },
    { "eth-type", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ETH_LENGTH_TYPE } },
    { "priority", &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ETH_PRIO } },
    { "vlan-id", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ETH_VLAN_ID } },
};

asn_type ndn_eth_csap_s =
{
    "Ethernet-CSAP", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_eth_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_eth_csap_ne_array}
};

const asn_type * const ndn_eth_csap = &ndn_eth_csap_s;


/* See description in ndn_eth.h */
int 
ndn_eth_packet_to_plain(const asn_value *pkt, 
                        ndn_eth_header_plain *eth_header)
{
    int rc;
    int val;
    size_t len;

    len = ETHER_ADDR_LEN;
    rc = asn_read_value_field(pkt, eth_header->dst_addr, &len, 
                              "dst-addr.#plain"); 
    len = ETHER_ADDR_LEN;
    if (rc == 0) 
        rc = asn_read_value_field(pkt, eth_header->src_addr, &len, 
                                  "src-addr.#plain"); 
    len = 2;
    if (rc == 0) 
        rc = asn_read_value_field(pkt, &eth_header->len_type, &len, 
                                  "length-type.#plain"); 
    len = sizeof(val);
    if (rc == 0) 
        rc = asn_read_value_field(pkt, &val, &len,
                                  "frame-type.#tagged.cfi.#plain"); 

    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
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
                              "frame-type.#tagged.priority.#plain"); 

    len = sizeof(eth_header->vlan_id);
    if (rc == 0)
        rc = asn_read_value_field(pkt, &eth_header->vlan_id, &len, 
                                  "frame-type.#tagged.vlan-id.#plain"); 

    return rc;
}


/* See description in ndn_eth.h */
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
        rc = asn_write_int32(asn_eth_hdr, eth_header->len_type,
                             "length-type.#plain"); 
    if (rc == 0 && eth_header->is_tagged)
    {
        rc = asn_write_int32(asn_eth_hdr, eth_header->cfi,
                             "frame-type.#tagged.cfi.#plain");

        if (rc == 0)
            rc = asn_write_int32(asn_eth_hdr, eth_header->priority,
                                 "frame-type.#tagged.priority.#plain"); 
        if (rc == 0)
            rc = asn_write_int32(asn_eth_hdr, eth_header->vlan_id, 
                                 "frame-type.#tagged.vlan-id.#plain"); 
    }

    if (rc)
    {
        asn_free_value(asn_eth_hdr);
        return NULL; 
    }

    return asn_eth_hdr;

}
