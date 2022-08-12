/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for IEEE Std 802.2 LLC protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#include "te_config.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_llc.h"


static asn_named_entry_t _ndn_snap_header_ne_array[] = {
    { "oui", &ndn_data_unit_int24_s, { PRIVATE, NDN_TAG_SNAP_OUI } },
    { "pid", &ndn_data_unit_int16_s, { PRIVATE, NDN_TAG_SNAP_PID } },
};

asn_type ndn_snap_header_s = {
    "IEEE-Std-802-SNAP-Header", { PRIVATE, 0 }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_snap_header_ne_array),
    { _ndn_snap_header_ne_array }
};

const asn_type * const ndn_snap_header = &ndn_snap_header_s;


static asn_named_entry_t _ndn_llc_header_ne_array[] = {
    { "i-g", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_LLC_DSAP_IG } },   /**< Individual/group DSAP */
    { "dsap", &ndn_data_unit_int7_s,
      { PRIVATE, NDN_TAG_LLC_DSAP } },      /**< DSAP address */
    { "c-r", &ndn_data_unit_int1_s,
      { PRIVATE, NDN_TAG_LLC_SSAP_CR } },   /**< Command/response */
    { "ssap", &ndn_data_unit_int7_s,
      { PRIVATE, NDN_TAG_LLC_SSAP } },      /**< SSAP address */
    { "ctl", &ndn_data_unit_int16_s,
      { PRIVATE, NDN_TAG_LLC_CTL } },       /**< Control */

    { "snap", &ndn_snap_header_s,
      { PRIVATE, NDN_TAG_LLC_SNAP_HEADER } },   /**< SNAP sublayer header */
};

asn_type ndn_llc_header_s = {
    "IEEE-Std-802.2-LLC-Header", {PRIVATE, 102}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_llc_header_ne_array),
    { _ndn_llc_header_ne_array }
};

const asn_type * const ndn_llc_header = &ndn_llc_header_s;
