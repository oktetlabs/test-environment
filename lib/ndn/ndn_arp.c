/** @file
 * @brief ARP NDN
 *
 * Definitions of ASN.1 types for NDN for Ethernet Address Resolution
 * protocol (RFC 826). 
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if 0
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_defs.h"

#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_arp.h"
#include "logger_api.h"


static asn_named_entry_t _ndn_arp_header_ne_array[] = 
{
    { "hw-type",        &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ARP_HW_TYPE } },
    { "proto-type",     &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ARP_PROTO } },
    { "hw-size",        &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ARP_HW_SIZE } },
    { "proto-size",     &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_ARP_PROTO_SIZE } },
    { "opcode",         &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_ARP_OPCODE } },
    { "snd-hw-addr",    &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_ARP_SND_HW_ADDR } },
    { "snd-proto-addr", &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_ARP_SND_PROTO_ADDR } },
    { "tgt-hw-addr",    &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_ARP_TGT_HW_ADDR } },
    { "tgt-proto-addr", &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_ARP_TGT_PROTO_ADDR } },
};

static asn_type ndn_arp_header_s =
{
    "ARP-Header", { PRIVATE, 100 /* FIXME */}, SEQUENCE, 
    sizeof(_ndn_arp_header_ne_array) / sizeof(asn_named_entry_t),
    {_ndn_arp_header_ne_array}
};

const asn_type * const ndn_arp_header = &ndn_arp_header_s;



asn_type ndn_arp_csap_s =
{
    "ARP-CSAP", { PRIVATE, 101 /* FIXME */ }, SEQUENCE, 
    sizeof(_ndn_arp_header_ne_array) / sizeof(asn_named_entry_t),
    {_ndn_arp_header_ne_array}
};

const asn_type * const ndn_arp_csap = &ndn_arp_csap_s;

#if 0

/* See description in ndn_arp.h */
te_errno
ndn_arp_packet_to_plain(const asn_value *pkt, 
                        ndn_arp_header_plain *arp_header)
{
    int rc;
    int val;
    size_t len;

    len = ETH_ALEN;
    rc = asn_read_value_field(pkt, arp_header->dst_addr, &len, 
                              "dst-addr.#plain"); 
    len = ETH_ALEN;
    if (rc == 0) 
        rc = asn_read_value_field(pkt, arp_header->src_addr, &len, 
                                  "src-addr.#plain"); 
    len = 2;
    if (rc == 0) 
        rc = asn_read_value_field(pkt, &arp_header->arp_type_len, &len, 
                                  "arp-type.#plain"); 
    len = sizeof(val);
    if (rc == 0) 
        rc = asn_read_value_field(pkt, &val, &len, "cfi"); 

    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        arp_header->is_tagged = 0;
        return 0;
    }
    else
        arp_header->is_tagged = 1;
    
    if (rc) 
        return rc;

    arp_header->cfi = val;

    len = sizeof(arp_header->priority);
    rc = asn_read_value_field(pkt, &arp_header->priority, &len, 
                              "priority.#plain"); 

    len = sizeof(arp_header->vlan_id);
    if (rc == 0)
        rc = asn_read_value_field(pkt, &arp_header->vlan_id, &len, 
                                  "vlan-id.#plain"); 

    return rc;
}


/* See description in ndn_arp.h */
asn_value *
ndn_arp_plain_to_packet(const ndn_arp_header_plain *arp_header)
{ 
    asn_value  *asn_arp_hdr;
    te_errno    rc;

    if (arp_header == NULL)
        return NULL;

    asn_arp_hdr = asn_init_value(ndn_arp_header);
    if (asn_arp_hdr == NULL)
        return NULL;

    rc = asn_write_int32(asn_arp_hdr, arp_header->hw_type,
                         "hw-type.#plain");
    if (rc == 0)
        rc = asn_write_int32(asn_arp_hdr, arp_header->proto_type,
                             "proto-type.#plain");
    if (rc == 0)
        rc = asn_write_int32(asn_arp_hdr, arp_header->hw_size,
                             "hw-size.#plain");
    if (rc == 0)
        rc = asn_write_int32(asn_arp_hdr, arp_header->proto_size,
                             "proto-size.#plain");
    if (rc == 0)
        rc = asn_write_int32(asn_arp_hdr, arp_header->opcode,
                             "opcode.#plain");
    if (rc == 0)
        rc = asn_write_value_field(asn_arp_hdr,
                                   arp_header->snd_hw_addr,
                                   arp_header->hw_size,
                                   "snd-hw-addr.#plain");
    if (rc == 0)
        rc = asn_write_value_field(asn_arp_hdr,
                                   arp_header->snd_proto_addr,
                                   arp_header->proto_size,
                                   "snd-proto-addr.#plain");
    if (rc == 0)
        rc = asn_write_value_field(asn_arp_hdr,
                                   arp_header->tgt_hw_addr,
                                   arp_header->hw_size,
                                   "tgt-hw-addr.#plain");
    if (rc == 0)
        rc = asn_write_value_field(asn_arp_hdr,
                                   arp_header->tgt_proto_addr,
                                   arp_header->proto_size,
                                   "tgt-proto-addr.#plain");

    if (rc != 0)
    {
        asn_free_value(asn_arp_hdr);
        return NULL; 
    }

    return asn_arp_hdr;
}
#endif
