/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for Ethernet protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "ndn_llc.h"


asn_type ndn_eth_address_s = {
    "Ethernet-Address",
    {PRIVATE, 500},
    OCT_STRING,
    6,
    {NULL}
};

const asn_type * const ndn_eth_address = &ndn_eth_address_s;

NDN_DATA_UNIT_WITH_RANGE_TYPE(eth_address, ndn_eth_address_s, Ethernet-Address);



static asn_named_entry_t _ndn_tag_header_ne_array[] = {

    /* Tag Control Information (TCI) */
    /** TCI priority field */
    { "priority", &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_PRIO } },
    /** TCI Cannonical Format Identifier (CFI) */
    { "cfi", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_CFI } },
    /** TCI Virtual LAN (VLAN) identifier */
    { "vlan-id", &ndn_data_unit_int12_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_VID } },

    /* Embedded RIF (E-RIF) */
    /* Route control (RC) */
    /** E-RIF RC Routing Type */
    { "e-rif-rc-rt", &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_RT } },
    /** E-RIF RC Length */
    { "e-rif-rc-lth", &ndn_data_unit_int5_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_LTH } },
    /** E-RIF RC Direction */
    { "e-rif-rc-d", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_D } },
    /** E-RIF RC Largest Frame */
    { "e-rif-rc-lf", &ndn_data_unit_int6_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_LF } },
    /** E-RIF RC Non-Canonical Format Identifier */
    { "e-rif-rc-ncfi", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_ERIF_RC_NCFI } },
    /** E-RIF Route Descriptors */
    { "e-rif-rd", &ndn_data_unit_octet_string_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_ERIF_RD } },
};

asn_type ndn_vlan_tag_header_s = {
    "IEEE-Std-802.1Q-Tag-Header", {PRIVATE, NDN_TAG_VLAN_TAG_HEADER},
    SEQUENCE, TE_ARRAY_LEN(_ndn_tag_header_ne_array),
    { _ndn_tag_header_ne_array }
};

const asn_type * const ndn_vlan_tag_header = &ndn_vlan_tag_header_s;

static asn_named_entry_t _ndn_vlan_header_ne_array[] = {
    /** Tag protocol identifier field (TPID) */
    { "tpid", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_VLAN_HEADER_TPID } },

    /* Tag Control Information (TCI) */
    /** Priority code point field (PCP) */
    { "pcp", &ndn_data_unit_int3_s,
      { PRIVATE, NDN_TAG_VLAN_HEADER_PCP } },
    /** Drop eligible indicator field (DEI) */
    { "dei", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_VLAN_HEADER_DEI } },
    /** VLAN identifier field (VID) */
    { "vid", &ndn_data_unit_int12_s,
      { PRIVATE, NDN_TAG_VLAN_HEADER_VID } },
};

asn_type ndn_vlan_header_s = {
    "IEEE-Std-802.1Q-Header", {PRIVATE, NDN_TAG_VLAN_HEADER},
    SEQUENCE, TE_ARRAY_LEN(_ndn_vlan_header_ne_array),
    { _ndn_vlan_header_ne_array }
};

const asn_type * const ndn_vlan_header = &ndn_vlan_header_s;

static asn_named_entry_t _ndn_vlan_double_tag_header_ne_array[] = {
    { "outer", &ndn_vlan_header_s,
      { PRIVATE, NDN_TAG_VLAN_HEADER_OUTER } },
    { "inner", &ndn_vlan_header_s,
      { PRIVATE, NDN_TAG_VLAN_HEADER_INNER } },
};

asn_type ndn_vlan_double_tag_header_s = {
    "IEEE-Std-802.1QinQ-Header", {PRIVATE, NDN_TAG_VLAN_DOUBLE_TAG_HEADER},
    SEQUENCE, TE_ARRAY_LEN(_ndn_vlan_double_tag_header_ne_array),
    { _ndn_vlan_double_tag_header_ne_array }
};

const asn_type * const ndn_vlan_double_tag_header = &ndn_vlan_double_tag_header_s;

static asn_named_entry_t _ndn_tagged_ne_array[] = {
    { "untagged", &asn_base_null_s,
      { PRIVATE, NDN_TAG_ETH_UNTAGGED } },  /**< Frame is not tagged */
    { "tagged", &ndn_vlan_tag_header_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER } },   /**< Frame is tagged */
    { "double-tagged", &ndn_vlan_double_tag_header_s,
      { PRIVATE, NDN_TAG_VLAN_DOUBLE_TAGGED } },   /**< Frame is double-tagged */
};

static asn_type ndn_vlan_tagged_s = {
    "IEEE-Std-802.1Q-Tagged", { PRIVATE, 0 }, CHOICE,
    TE_ARRAY_LEN(_ndn_tagged_ne_array),
    { _ndn_tagged_ne_array }
};

const asn_type * const ndn_vlan_tagged = &ndn_vlan_tagged_s;


static asn_named_entry_t _ndn_802_3_encap_ne_array[] = {
    /** Ethernet v2 encapsulation: Type interpretation of Length/Type */
    { "ethernet2", &asn_base_null_s,
      { PRIVATE, NDN_TAG_ETHERNET2 } },
    /** LLC encapsulation: Length interpretation of Length/Type */
    { "llc", &ndn_llc_header_s,
      { PRIVATE, NDN_TAG_LLC_HEADER } },
};

static asn_type ndn_802_3_encap_s = {
    "IEEE-Std-802.3-Encapsulation", { PRIVATE, 100 }, CHOICE,
    TE_ARRAY_LEN(_ndn_802_3_encap_ne_array),
    { _ndn_802_3_encap_ne_array }
};


static asn_named_entry_t ndn_802_3_header_ne_array[] = {
    /** 48-bit destination address */
    { "dst-addr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_802_3_DST } },
    /** 48-bit source address */
    { "src-addr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_802_3_SRC } },
    /** Is frame tagged or not? */
    { "tagged", &ndn_vlan_tagged_s,
      { PRIVATE, NDN_TAG_VLAN_TAGGED } },
    /** Length/Type field exactly */
    { "length-type", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_802_3_LENGTH_TYPE } },
    /** Interpretation of Length/Type field */
    { "encap", &ndn_802_3_encap_s,
      { PRIVATE, NDN_TAG_802_3_ENCAP } },
    /** EtherType (regardless encapsulation) */
    { "ether-type", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_802_3_ETHER_TYPE } },
};

asn_type ndn_eth_header_s = {
    "IEEE-Std-802.3-Header", { PRIVATE, 0 }, SEQUENCE,
    TE_ARRAY_LEN(ndn_802_3_header_ne_array),
    { ndn_802_3_header_ne_array }
};

const asn_type * const ndn_eth_header = &ndn_eth_header_s;



static asn_named_entry_t _ndn_eth_csap_ne_array [] = {
    { "device-id", &ndn_data_unit_char_string_s,
      { PRIVATE, NDN_TAG_ETH_DEVICE } },
    { "receive-mode", &asn_base_integer_s,
      { PRIVATE, NDN_TAG_ETH_RECV_MODE } },
    { "local-addr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_ETH_LOCAL } },
    { "remote-addr", &ndn_data_unit_eth_address_s,
      { PRIVATE, NDN_TAG_ETH_REMOTE } },
    { "ether-type", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_802_3_ETHER_TYPE} },
    { "priority", &ndn_data_unit_int8_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_PRIO } },
    { "vlan-id", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_VLAN_TAG_HEADER_VID } },
};

asn_type ndn_eth_csap_s = {
    "Ethernet-CSAP", {PRIVATE, 101}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_eth_csap_ne_array),
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
                                  "tagged.#tagged.cfi.#plain");

    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        eth_header->is_tagged = FALSE;
        return 0;
    }
    else
        eth_header->is_tagged = TRUE;

    if (rc)
        return rc;

    eth_header->cfi = val;

    len = sizeof(eth_header->priority);
    rc = asn_read_value_field(pkt, &eth_header->priority, &len,
                              "tagged.#tagged.priority.#plain");

    len = sizeof(eth_header->vlan_id);
    if (rc == 0)
        rc = asn_read_value_field(pkt, &eth_header->vlan_id, &len,
                                  "tagged.#tagged.vlan-id.#plain");

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
                             "tagged.#tagged.cfi.#plain");

        if (rc == 0)
            rc = asn_write_int32(asn_eth_hdr, eth_header->priority,
                                 "tagged.#tagged.priority.#plain");
        if (rc == 0)
            rc = asn_write_int32(asn_eth_hdr, eth_header->vlan_id,
                                 "tagged.#tagged.vlan-id.#plain");
    }

    if (rc)
    {
        asn_free_value(asn_eth_hdr);
        return NULL;
    }

    return asn_eth_hdr;

}
