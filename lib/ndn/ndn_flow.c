/** @file
 * @brief Traffic flow processing, NDN.
 *
 * Definitions of ASN.1 types for NDN for traffic flow processing.
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TE_LGR_USER "NDN/Flow"

#include "asn_impl.h"
#include "ndn.h"
#include "ndn_internal.h"
#include "ndn_base.h"
#include "ndn_flow.h"

/**
 * Constants for ASN tags of NDN messages
 */
typedef enum {
    NDN_FLOW_EP_TA = 11111,
    NDN_FLOW_EP_ID,
    NDN_FLOW_EP_NAME,
    NDN_FLOW_EP_DESCR,
    NDN_FLOW_EP_CSAP,
    NDN_FLOW_EP,
    NDN_FLOW_EP_SEQ,

    NDN_FLOW_PDU_ID,
    NDN_FLOW_PDU_NAME,
    NDN_FLOW_PDU_SRC,
    NDN_FLOW_PDU_DST,
    NDN_FLOW_PDU_SEND,
    NDN_FLOW_PDU_RECV,
    NDN_FLOW_PDU_COUNT,
    NDN_FLOW_PDU_PLEN,
    NDN_FLOW_PDU,
    NDN_FLOW_PDU_SEQUENCE,

    NDN_FLOW_ENDPOINTS,
    NDN_FLOW_TRAFFIC,
    NDN_FLOW,
} ndn_flowe_tags_t;

static asn_named_entry_t _ndn_flow_ep_ne_array [] = {
    { "id", &asn_base_integer_s, {PRIVATE, NDN_FLOW_EP_ID } },
    { "name", NDN_BASE_STRING, {PRIVATE, NDN_FLOW_EP_NAME } },
    { "description", NDN_BASE_STRING, {PRIVATE, NDN_FLOW_EP_DESCR } },
    { "ta", NDN_BASE_STRING, { PRIVATE, NDN_FLOW_EP_TA } },
    { "layers", &ndn_csap_layers_s, { PRIVATE, NDN_FLOW_EP_CSAP } },
};

asn_type ndn_flow_ep_s = {
    "QoS-Flow-Endpoint", { PRIVATE, NDN_FLOW_EP }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_flow_ep_ne_array),
    {_ndn_flow_ep_ne_array}
};

const asn_type * const ndn_flow_ep = &ndn_flow_ep_s;

/* Endpoints sequence */
asn_type ndn_flow_ep_seq_s = {
    "QoS-Flow-Endpoints-Seq", {PRIVATE, NDN_FLOW_EP_SEQ}, SEQUENCE_OF, 0,
    {subtype: &ndn_flow_ep_s}
};

const asn_type * const ndn_flow_ep_seq = &ndn_flow_ep_seq_s;

static asn_named_entry_t _ndn_flow_pdu_ne_array [] = {
    { "id", &asn_base_integer_s, {PRIVATE, NDN_FLOW_PDU_ID } },
    { "name", NDN_BASE_STRING, {PRIVATE, NDN_FLOW_PDU_NAME } },
    { "src", NDN_BASE_STRING, {PRIVATE, NDN_FLOW_PDU_SRC } },
    { "dst", NDN_BASE_STRING, {PRIVATE, NDN_FLOW_PDU_DST } },
    { "send", &ndn_generic_pdu_sequence_s, {PRIVATE, NDN_FLOW_PDU_SEND } },
    { "recv", &ndn_generic_pdu_sequence_s, {PRIVATE, NDN_FLOW_PDU_RECV } },
    { "count", &ndn_data_unit_int16_s, {PRIVATE, NDN_FLOW_PDU_COUNT } },
    { "plen", &ndn_data_unit_int16_s,
        {PRIVATE, NDN_FLOW_PDU_PLEN } }
};

asn_type ndn_flow_pdu_s = {
    "QoS-Flow-PDU", { PRIVATE, NDN_FLOW_PDU }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_flow_pdu_ne_array),
    {_ndn_flow_pdu_ne_array}
};

const asn_type * const ndn_flow_pdu = &ndn_flow_pdu_s;

asn_type ndn_flow_pdu_sequence_s = {
    "QoS-Flow-PDU-Sequence", { PRIVATE, NDN_FLOW_PDU_SEQUENCE },
    SEQUENCE_OF, 0, {subtype: &ndn_flow_pdu_s}
};

const asn_type * const ndn_flow_pdu_sequence =
    &ndn_flow_pdu_sequence_s;

static asn_named_entry_t _ndn_flow_ne_array [] = {
    { "endpoint", &ndn_flow_ep_seq_s, { PRIVATE, NDN_FLOW_ENDPOINTS } },
    { "traffic", &ndn_flow_pdu_sequence_s,
      { PRIVATE, NDN_FLOW_TRAFFIC } }
};

asn_type ndn_flow_s = {
    "QoS-Flow", { PRIVATE, NDN_FLOW }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_flow_ne_array),
    {_ndn_flow_ne_array}
};

const asn_type * const ndn_flow = &ndn_flow_s;


