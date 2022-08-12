/** @file
 * @brief Proteos, TAD RTE mbuf, NDN
 *
 * Definitions of ASN.1 types for NDN of RTE mbuf pseudo-protocol
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "tad_common.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_rte_mbuf.h"

asn_type ndn_rte_mbuf_pdu_s = {
    "RTE_mbuf-dummy-pdu", { PRIVATE, 0 }, SEQUENCE, 0, {NULL}
};

const asn_type * const ndn_rte_mbuf_pdu = &ndn_rte_mbuf_pdu_s;

static asn_named_entry_t _ndn_rte_mbuf_csap_ne_array [] = {
    { "pkt-ring", &ndn_data_unit_char_string_s,
      { PRIVATE, NDN_TAG_RTE_MBUF_RING } },
    { "pkt-pool", &ndn_data_unit_char_string_s,
      { PRIVATE, NDN_TAG_RTE_MBUF_POOL } },
};

asn_type ndn_rte_mbuf_csap_s = {
    "RTE_mbuf-CSAP", {PRIVATE, TE_PROTO_RTE_MBUF}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_rte_mbuf_csap_ne_array),
    {_ndn_rte_mbuf_csap_ne_array}
};

const asn_type * const ndn_rte_mbuf_csap = &ndn_rte_mbuf_csap_s;
