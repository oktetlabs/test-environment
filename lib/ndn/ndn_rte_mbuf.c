/** @file
 * @brief Proteos, TAD RTE mbuf, NDN
 *
 * Definitions of ASN.1 types for NDN of RTE mbuf pseudo-protocol
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
