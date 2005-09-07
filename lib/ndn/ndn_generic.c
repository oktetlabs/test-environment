/** @file
 * @brief ASN.1 library
 *
 * Definitions of general NDN ASN.1 types
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

/* for hton* functions */
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#define TE_LGR_USER "NDN"

#include "te_defs.h"
#include "te_errno.h" 

#include "asn_impl.h"
#include "ndn.h"
#include "ndn_internal.h"
#include "logger_api.h"

/*
Defined in SNMPv2-SMI:
IpAddress ::=
    [APPLICATION 0]
        IMPLICIT OCTET STRING (SIZE (4))
*/
asn_type ndn_ip_address_s = 
{ "IpAddress", {APPLICATION, 0}, OCT_STRING, 4, {0}};

const asn_type * const ndn_ip_address = &ndn_ip_address_s;



/* 
 * Interval 
 */ 

static asn_named_entry_t _ndn_interval_ne_array[] = 
{
    { "b", &asn_base_integer_s, {PRIVATE, NDN_INTERVALS_BEGIN} },
    { "e", &asn_base_integer_s, {PRIVATE, NDN_INTERVALS_END} }
};

asn_type ndn_interval_static = 
{ "Interval", {PRIVATE, NDN_DU_INTERVALS}, SEQUENCE, 
    sizeof(_ndn_interval_ne_array) / sizeof(_ndn_interval_ne_array[0]),
    {_ndn_interval_ne_array} 
};

const asn_type *const ndn_interval = &ndn_interval_static;



asn_type ndn_data_unit_ints_s = 
{   "DATA-UNIT-intervals", {PRIVATE, NDN_DU_INTERVALS}, SEQUENCE_OF, 0,
    {subtype: &ndn_interval_static} 
};

const asn_type * const  ndn_interval_sequence = &ndn_data_unit_ints_s;




asn_type ndn_data_unit_enum_s = 
{ "DATA-UNIT-enum", {PRIVATE, NDN_DU_ENUM}, SET_OF, 0,
   {subtype: &asn_base_integer_s} 
};



static asn_named_entry_t _ndn_data_unit_mask_ne_array[] = 
{
    { "v", &asn_base_octstring_s, {PRIVATE, NDN_MASK_VALUE} },
    { "m", &asn_base_octstring_s, {PRIVATE, NDN_MASK_PATTERN} },
    { "exact-len", &asn_base_boolean_s, {PRIVATE, NDN_MASK_EXACT_LEN} }
}; 

asn_type ndn_data_unit_mask_s = 
{ 
    "DATA-UNIT-mask", {PRIVATE, NDN_DU_MASK}, SEQUENCE, 
    sizeof(_ndn_data_unit_mask_ne_array)/
        sizeof(_ndn_data_unit_mask_ne_array[0]), 
    {_ndn_data_unit_mask_ne_array} 
};



static asn_named_entry_t _ndn_data_unit_env_ne_array[] = 
{
    { "name", &asn_base_charstring_s, {PRIVATE, NDN_ENV_NAME} },
    { "type", &asn_base_enum_s, {PRIVATE, NDN_ENV_TYPE} }
}; 

asn_type ndn_data_unit_env_s = 
{ "DATA-UNIT-env", {PRIVATE, NDN_DU_ENV}, SEQUENCE, 
  sizeof(_ndn_data_unit_env_ne_array) /
      sizeof(_ndn_data_unit_env_ne_array[0]), 
  {_ndn_data_unit_env_ne_array} 
};




asn_type ndn_octet_string6_s = 
{  "OCTET STRING (SIZE (6))", {UNIVERSAL, 4}, OCT_STRING, 6, {0}};

const asn_type * const  ndn_octet_string6 = &ndn_octet_string6_s;



NDN_DATA_UNIT_TYPE(int32,         asn_base_integer_s,    INTEGER);
NDN_DATA_UNIT_TYPE(int4,          asn_base_int4_s,       INTEGER(0..15));
NDN_DATA_UNIT_TYPE(int8,          asn_base_int8_s,       INTEGER(0..255));
NDN_DATA_UNIT_TYPE(int16,         asn_base_int16_s,      INTEGER(0..65535));
NDN_DATA_UNIT_TYPE(octet_string,  asn_base_octstring_s,  OCTET STRING);
NDN_DATA_UNIT_TYPE(octet_string6, ndn_octet_string6_s,   OCTET STRING (6));
NDN_DATA_UNIT_TYPE(char_string,   asn_base_charstring_s, UniversalString );
NDN_DATA_UNIT_TYPE(ip_address,    ndn_ip_address_s,      IpAddress );
NDN_DATA_UNIT_TYPE(objid,         asn_base_objid_s,      OBJECT IDENTIFIER);



static asn_named_entry_t _ndn_pld_stream_ne_array[] = 
{
    { "function", &asn_base_charstring_s, {PRIVATE, NDN_PLD_STR_FUNC} },
    { "offset",   &ndn_data_unit_int32_s, {PRIVATE, NDN_PLD_STR_OFF} },
    { "length",   &ndn_data_unit_int32_s, {PRIVATE, NDN_PLD_STR_LEN} },
};

asn_type ndn_pld_stream_s =
{ "Payload-Stream", {PRIVATE, NDN_PLD_STREAM}, SEQUENCE, 
  sizeof(_ndn_pld_stream_ne_array) / sizeof(_ndn_pld_stream_ne_array[0]), 
  {_ndn_pld_stream_ne_array} 
};


static asn_named_entry_t _ndn_payload_ne_array[] = 
{
    { "bytes",    &asn_base_octstring_s,  {PRIVATE, NDN_PLD_BYTES} },
    { "mask",     &ndn_data_unit_mask_s,  {PRIVATE, NDN_PLD_MASK} },
    { "function", &asn_base_charstring_s, {PRIVATE, NDN_PLD_FUNC} },
    { "filename", &asn_base_charstring_s, {PRIVATE, NDN_PLD_FILE} },
    { "length",   &asn_base_integer_s,    {PRIVATE, NDN_PLD_LEN} },
    { "stream",   &ndn_pld_stream_s,      {PRIVATE, NDN_PLD_STREAM} },
}; 

asn_type ndn_payload_s =
{ "Payload", {PRIVATE, NDN_TMPL_PAYLOAD}, CHOICE, 
  sizeof(_ndn_payload_ne_array) / sizeof(_ndn_payload_ne_array[0]), 
  {_ndn_payload_ne_array} 
};

const asn_type * const ndn_payload = &ndn_payload_s;



static asn_type ndn_csap_spec_s =
{ "CSAP-spec", {PRIVATE, NDN_CSAP_SPEC}, SEQUENCE_OF, 0,
    {subtype: &ndn_generic_csap_level_s} };

const asn_type * const ndn_csap_spec = &ndn_csap_spec_s;





static asn_type ndn_integer_seq_s = 
{ 
    "SEQENCE OF INTEGER", {PRIVATE, NDN_ITER_INTS}, SEQUENCE_OF, 0,
    {subtype: &asn_base_integer_s} 
};

static asn_type ndn_integer_seq_assoc_s = 
{ 
    "SEQENCE OF INTEGER", {PRIVATE, NDN_ITER_INTS_ASSOC}, SEQUENCE_OF, 0,
    {subtype: &asn_base_integer_s} 
};

static asn_type ndn_chstring_seq_s = 
{ 
    "SEQENCE OF UniversalString",
    {PRIVATE, NDN_ITER_STRINGS}, SEQUENCE_OF, 0,
    {subtype: &asn_base_charstring_s} 
};


static asn_named_entry_t _ndn_template_parameter_simple_for_ne_array[] =
{
    { "begin",  &asn_base_integer_s, {PRIVATE, NDN_FOR_BEGIN} },
    { "end",    &asn_base_integer_s, {PRIVATE, NDN_FOR_END} },
    { "step",   &asn_base_integer_s, {PRIVATE, NDN_FOR_STEP} }
}; 

static asn_type ndn_template_parameter_simple_for_s = 
{ 
    "Templ-Param-simple-for", {PRIVATE, NDN_ITER_FOR}, SEQUENCE, 
    sizeof(_ndn_template_parameter_simple_for_ne_array) /
        sizeof(_ndn_template_parameter_simple_for_ne_array[0]), 
    {_ndn_template_parameter_simple_for_ne_array} 
};


static asn_named_entry_t _ndn_template_parameter_ne_array[] = 
{
    { "ints",       &ndn_integer_seq_s, {PRIVATE, NDN_ITER_INTS} },
    { "ints-assoc", &ndn_integer_seq_assoc_s,
        {PRIVATE, NDN_ITER_INTS_ASSOC} },
    { "strings",    &ndn_chstring_seq_s, {PRIVATE, NDN_ITER_STRINGS} },
    { "simple-for", &ndn_template_parameter_simple_for_s,
        {PRIVATE, NDN_ITER_FOR} },
}; 

asn_type ndn_template_parameter_s =
{
    "Template-Parameter", {PRIVATE, NDN_TMPL_ARGS}, CHOICE,
    sizeof(_ndn_template_parameter_ne_array) /
        sizeof(_ndn_template_parameter_ne_array[0]), 
    {_ndn_template_parameter_ne_array}
};

const asn_type * const ndn_template_parameter = &ndn_template_parameter_s;





static asn_type ndn_template_parameter_sequence_s = 
{ "SEQENCE OF Template-Parameter", {PRIVATE, NDN_TMPL_ARGS}, 
  SEQUENCE_OF, 0, {subtype: &ndn_template_parameter_s} 
};
const asn_type * const ndn_template_params_seq = 
                            &ndn_template_parameter_sequence_s;

static asn_type ndn_generic_pdu_sequence_s = 
{ "Generic-PDU-sequence", {PRIVATE, NDN_TMPL_PDUS}, 
  SEQUENCE_OF, 0, {subtype: &ndn_generic_pdu_s} 
};

const asn_type * const ndn_generic_pdu_sequence = 
                            &ndn_generic_pdu_sequence_s;





static asn_named_entry_t _ndn_traffic_template_ne_array[] = 
{
    { "arg-sets",  &ndn_template_parameter_sequence_s,
        {PRIVATE, NDN_TMPL_ARGS} },
    { "delays",    &ndn_data_unit_int32_s,
        {PRIVATE, NDN_TMPL_DELAYS} },
    { "pdus",      &ndn_generic_pdu_sequence_s,
        {PRIVATE, NDN_TMPL_PDUS} },
    { "payload",   &ndn_payload_s,
        {PRIVATE, NDN_TMPL_PAYLOAD} },
}; 

asn_type ndn_traffic_template_s =
{ "Traffic-Template", {PRIVATE, NDN_TRAFFIC_TEMPLATE}, SEQUENCE, 
   sizeof(_ndn_traffic_template_ne_array) /
       sizeof(_ndn_traffic_template_ne_array[0]), 
   {_ndn_traffic_template_ne_array}
};

const asn_type * const  ndn_traffic_template = &ndn_traffic_template_s;

/*
Packet-Action ::= CHOICE {
    echo        NULL,
    forward     INTEGER,
    function    UniversalString,
    file        UniversalString
}
*/

static asn_named_entry_t _ndn_packet_action_ne_array[] = 
{
    { "echo",    &asn_base_null_s, {PRIVATE, NDN_ACT_ECHO} },
    { "forward", &asn_base_integer_s, {PRIVATE, NDN_ACT_FORWARD} },
    { "function",&asn_base_charstring_s, {PRIVATE, NDN_ACT_FUNCTION} },
    { "file",    &asn_base_charstring_s, {PRIVATE, NDN_ACT_FILE} },
}; 

asn_type ndn_packet_action_s =
{ "Payload", {PRIVATE, 2}, CHOICE, 
  sizeof(_ndn_packet_action_ne_array) /
      sizeof(_ndn_packet_action_ne_array[0]), 
  {_ndn_packet_action_ne_array} 
};

const asn_type * const  ndn_packet_action = &ndn_packet_action_s;


/*

Traffic-Pattern-Unit ::= SEQUENCE {
    pdus        SEQUENCE OF Generic-PDU,
    payload     Payload OPTIONAL,
    action      Packet-Action OPTIONAL,
}
*/


static asn_named_entry_t _ndn_traffic_pattern_unit_ne_array[] = 
{
    { "pdus",      &ndn_generic_pdu_sequence_s, {PRIVATE, NDN_PU_PDUS} },
    { "payload",   &ndn_payload_s,       {PRIVATE, NDN_PU_PAYLOAD} },
    { "action",    &ndn_packet_action_s, {PRIVATE, NDN_PU_ACTION} },
}; 

asn_type ndn_traffic_pattern_unit_s =
{ "Traffic-Pattern-Unit", {PRIVATE, NDN_TRAFFIC_PATTERN_UNIT}, SEQUENCE, 
   sizeof(_ndn_traffic_pattern_unit_ne_array) /
       sizeof(_ndn_traffic_pattern_unit_ne_array[0]),
  {_ndn_traffic_pattern_unit_ne_array}
};

const asn_type * const  ndn_traffic_pattern_unit = 
                            &ndn_traffic_pattern_unit_s;


/*

Traffic-Pattern ::= SEQUENCE OF Traffic-Pattern-Unit

*/

asn_type ndn_traffic_pattern_s =
{ "Traffic-Pattern", {PRIVATE, NDN_TRAFFIC_PATTERN}, SEQUENCE_OF, 0,
  {subtype: &ndn_traffic_pattern_unit_s}
};

const asn_type * const  ndn_traffic_pattern = &ndn_traffic_pattern_s;



/* 
NDN-TimeStamp ::= SEQUENCE {
    seconds INTEGER, -- seconds since Unix epoch
    micro-seconds INTEGER
}
 */
static asn_named_entry_t _ndn_time_stamp_ne_array[] = 
{
    { "seconds",       &asn_base_integer_s, {PRIVATE, NDN_TIME_SEC} },
    { "micro-seconds", &asn_base_integer_s, {PRIVATE, NDN_TIME_MCS} },
}; 

asn_type ndn_time_stamp_s =
{ "NDN-TimeStamp", {PRIVATE, NDN_PKT_TIMESTAMP}, SEQUENCE, 
   sizeof(_ndn_time_stamp_ne_array) / sizeof(_ndn_time_stamp_ne_array[0]),
  {_ndn_time_stamp_ne_array}

};

const asn_type * const  ndn_time_stamp = &ndn_time_stamp_s;


/*
Raw-Packet ::= SEQUENCE -- values of this type are passed from CSAP to test
{
    received    NDN-TimeStamp,
    pdus        SEQUENCE (SIZE (1..max-pdus)) OF Generic-PDU,
    payload     Payload OPTIONAL
} 
*/


static asn_named_entry_t _ndn_raw_packet_ne_array[] = 
{
    { "received",  &ndn_time_stamp_s, {PRIVATE, NDN_PKT_TIMESTAMP} },
    { "pdus",      &ndn_generic_pdu_sequence_s, {PRIVATE, NDN_PKT_PDUS} },
    { "payload",   &ndn_payload_s,    {PRIVATE, NDN_PKT_PAYLOAD} },
}; 

asn_type ndn_raw_packet_s =
{ "Raw-Packet", {PRIVATE, NDN_TRAFFIC_PACKET}, SEQUENCE, 
   sizeof(_ndn_raw_packet_ne_array) / sizeof(_ndn_raw_packet_ne_array[0]), 
  {_ndn_raw_packet_ne_array}

};

const asn_type * const  ndn_raw_packet = &ndn_raw_packet_s;




/* See description in ndn.h */ 
int
ndn_match_mask(const asn_value *mask_pat, uint8_t *data, size_t d_len)
{
    int exact_len_index, pat_index, val_index;
    int rc = 0;

    te_bool exact_len;

    size_t           mask_len;
    size_t           cmp_len;

    const asn_value *leaf_m = NULL;
    const asn_value *leaf_v = NULL;

    const uint8_t *data_m;
    const uint8_t *data_v;
    const uint8_t *m, *d, *p;

    if (mask_pat->asn_type != &ndn_data_unit_mask_s)
    {
        ERROR("%s(): wrong asn type of masḵ_pat", __FUNCTION__);
        return TE_EASNWRONGTYPE;
    }

    rc = asn_child_tag_index(&ndn_data_unit_mask_s,
                             PRIVATE, NDN_MASK_VALUE, &val_index);
    if (rc == 0)
        rc = asn_child_tag_index(&ndn_data_unit_mask_s,
                                 PRIVATE, NDN_MASK_PATTERN, &pat_index);
    if (rc == 0)
        rc = asn_child_tag_index(&ndn_data_unit_mask_s,
                                 PRIVATE, NDN_MASK_EXACT_LEN,
                                 &exact_len_index);
    if (rc != 0)
    {
        ERROR("%s(): get indexes of leafs in mask type fails %r",
              __FUNCTION__, rc);
        return TE_EASNGENERAL;
    }

    if ((leaf_m = mask_pat->data.array[pat_index]) == NULL ||
        (leaf_v = mask_pat->data.array[val_index]) == NULL)
    {
        ERROR("%s(): no sufficient data to match with mask", __FUNCTION__);
        return TE_EASNINCOMPLVAL;
    }

    cmp_len = mask_len = leaf_m->len;

    exact_len = ((mask_pat->data.array[exact_len_index]->data.integer == 0)
                    ? FALSE : TRUE);

    if (exact_len && mask_len != d_len)
    {
        VERB("%s(): mask_len %d not equal d_len %d",
             __FUNCTION__, mask_len, d_len);
        return TE_ETADNOTMATCH;
    }

    if (d_len < mask_len)
        cmp_len = d_len;

    if ((data_m = leaf_m->data.other) == NULL || 
        (data_v = leaf_v->data.other) == NULL)
    {
        ERROR("%s(): no sufficient data to match with mask", __FUNCTION__);
        return TE_EASNINCOMPLVAL;
    }

    for (d = data, m = data_m, p = data_v;
         cmp_len > 0; 
         d++, p++, m++, cmp_len--)
        if ((*d & *m) != (*p & *m))
        { 
            rc = TE_ETADNOTMATCH;
            break;
        }
    return rc;
}

/* See description in ndn.h */ 
int 
ndn_match_data_units(const asn_value *pattern, asn_value *pkt_pdu,
                     uint8_t *data, size_t d_len, const char *label)
{
    int               field_index;
    asn_syntax        plain_syntax;
    const asn_value  *du_ch_val = NULL;
    const asn_value  *du_val = NULL;
    const asn_type   *du_type;
    const asn_type   *du_sub_type;
    asn_tag_class     t_class;
    uint16_t          t_val = NDN_DU_UNDEF;
    const uint8_t *pat_data;
    size_t pat_d_len;

    int      rc = 0;
    uint32_t user_int = 0;

    if (pattern == NULL || data == NULL || label == NULL)
        return TE_EWRONGPTR; 

    rc = asn_impl_named_subvalue_index(pattern->asn_type, label,
                                       &field_index);
    if (rc != 0)
    {
        ERROR("%s(): find field %s index failed %r",
              __FUNCTION__, label, rc);
        return rc;
    }

    du_ch_val = pattern->data.array[field_index];
    if(du_ch_val == NULL && pkt_pdu == NULL)
        /* data matches, no value should be saved into packet */
        return 0; 

    if (du_ch_val != NULL)
    {
        rc = asn_get_choice_value(du_ch_val, &du_val, &t_class, &t_val);
        if (rc != 0)
            return rc;
    } 

    /* 
     * since rc from 'asn_impl_named_subvalue_index' was zero,
     * we may be sure that named_entries field here is correct;
     * get first subtype in Data-Unit, that is "plain" allways.
     */
    du_type = pattern->asn_type->sp.named_entries[field_index].type;

    if (du_type->sp.named_entries == NULL || 
        (du_sub_type = du_type->sp.named_entries[0].type) == NULL)
    {
        ERROR("%s: Wrong type of subleaf passed", __FUNCTION__);
        return TE_EASNGENERAL;
    }

    plain_syntax = du_sub_type->syntax;

    switch (plain_syntax)
    {
        case SYNTAX_UNDEFINED:
            ERROR("error getting syntax\n");
        case INTEGER:
        case ENUMERATED:
            switch (d_len)
            {
                case 2: 
                    user_int = ntohs(*((uint16_t *)data));
                    break;
                case 4: 
                    user_int = ntohl(*((uint32_t *)data));
                    break;
                case 8: 
                    return TE_EOPNOTSUPP;
                default:
                    user_int = *((uint8_t *)data);
            }
            break;
        default: 
            break; /* No any preliminary actions needed. */
    }

    switch (t_val)
    {
        case NDN_DU_UNDEF:
            /* do nothing, data is matched */
            break;

        case NDN_DU_PLAIN:
            switch (plain_syntax)
            {
                case INTEGER:
                case ENUMERATED:
                    if ((uint32_t)du_val->data.integer != user_int)
                        rc = TE_ETADNOTMATCH;
                    break;

                case BIT_STRING:
                    rc = TE_EOPNOTSUPP; /* TODO */
                    break;

                case OCT_STRING:
                case CHAR_STRING: 
                    pat_data = du_val->data.other;
                    pat_d_len = du_val->len;

                    if (pat_d_len != d_len || 
                        memcmp(data, pat_data, d_len) != 0)
                        rc = TE_ETADNOTMATCH;
                    break;

                default:
                    WARN("%s, compare with plain of syntax %d"
                         " unsupported yet",
                         __FUNCTION__, plain_syntax);
                    break;
            }
            break;

        case NDN_DU_MASK:
            rc = ndn_match_mask(du_val, data, d_len);
            break;
        case NDN_DU_SCRIPT:
        case NDN_DU_ENUM:
            WARN("%s, DATA-UNIT tag %d unsupported", 
                 __FUNCTION__, (int)t_val);
            rc = TE_EOPNOTSUPP;
            break;

        case NDN_DU_INTERVALS:
            {
                const asn_value *interval;
                const asn_value *i_begin;
                const asn_value *i_end;
                unsigned int i; 

                if (plain_syntax != INTEGER &&
                    plain_syntax != ENUMERATED)
                {
                    ERROR("%s(): intervals pattern may be applied "
                          "only with integer plain syntax", __FUNCTION__);
                    return TE_ETADWRONGNDS;
                }

                if (du_val->asn_type != ndn_interval_sequence)
                {
                    ERROR("%s(): Wrong type of interval choice leaf",
                          __FUNCTION__);
                    return TE_ETADWRONGNDS;
                }

                rc = TE_ETADNOTMATCH;
                for (i = 0; i < du_val->len; i++)
                {
                    interval = du_val->data.array[i];
                    if (interval == NULL)
                        continue;
                    /* 
                     * hardcode offsets of 'b' and 'e' fields in
                     * Interval type
                     */
                    if ((i_begin = interval->data.array[0]) == NULL ||
                        (i_end   = interval->data.array[1]) == NULL)
                    {
                        WARN("%s(): wrong begin or end in interval");
                        continue; 
                    } 

                    if ((unsigned)i_begin->data.integer <= user_int &&
                        (unsigned)i_end->data.integer >= user_int)
                    {
                        rc = 0;
                        break;
                    }
                }
            }
            break;

        case NDN_DU_ENV:
        case NDN_DU_FUNC:
        default:
            WARN("%s, this Data-Unit fields may not present for match",
                 __FUNCTION__);
            rc = TE_EOPNOTSUPP;
            break;
    }

    if (rc == 0 && pkt_pdu)
    {
        char labels_buffer[200];

        strcpy(labels_buffer, label);
        strcat(labels_buffer, ".#plain");

        switch (plain_syntax)
        {
            case INTEGER:
            case ENUMERATED:
                rc = asn_write_int32(pkt_pdu, user_int, labels_buffer);
                break;

            case OCT_STRING:
            case CHAR_STRING:
                rc = asn_write_value_field(pkt_pdu, data, d_len, 
                                           labels_buffer);
                break;

            default:
                WARN("unsupported yet");
        }
    }
    return rc;

}


/**
 * Get timestamp from recieved Raw-Packet
 *
 * @param packet ASN-value of Raw-Packet type.
 * @param ts     location for timestamp (OUT).
 *
 * @return status code
 */
int
ndn_get_timestamp(const asn_value *packet, struct timeval *ts)
{ 
    int rc;
    size_t len;

    len = sizeof(ts->tv_sec);
    rc = asn_read_value_field(packet, &ts->tv_sec, &len, 
                              "received.seconds");

    len = sizeof(ts->tv_usec);
    if (rc == 0)
        rc = asn_read_value_field(packet, &ts->tv_usec, &len, 
                                  "received.micro-seconds");
    return rc;
}

/* Type of operation which will be done with Data-Unit field */
typedef enum {
    NDN_DU_RO,
    NDN_DU_WR,
} ndn_du_get_oper_t;

/**
 * Get valid pointer to Data-Unit PDU subvalue. If there is no
 * such subvalue in passed PDU, creates it. 
 * 
 * @param pdu     ASN value with NDN PDU
 * @param tag     ASN tag of Data-Unit subvalue
 * @param du_leaf place for Data-Unit subvalue (OUT)
 *
 * @return status code
 */
static int
ndn_get_du_field(asn_value *pdu, ndn_du_get_oper_t oper,
                 uint16_t tag, asn_value **du_leaf)
{
    int rc = 0;

    if (du_leaf == NULL)
        return TE_EWRONGPTR;

    rc = asn_get_child_value(pdu, (const asn_value **)du_leaf,
                             PRIVATE, tag);
    if (oper == NDN_DU_WR)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        { 
            const asn_type *du_type;
            rc = asn_get_child_type(asn_get_type(pdu), &du_type, 
                                    PRIVATE, tag);
            if (rc != 0) 
            {
                ERROR("%s(): get child type failed %x", __FUNCTION__, rc);
                return rc; 
            }

            *du_leaf = asn_init_value(du_type);
            rc = asn_put_child_value(pdu, *du_leaf, PRIVATE, tag);
            if (rc != 0)
            {
                ERROR("%s(): put child value failed %x", __FUNCTION__, rc);
                asn_free_value(*du_leaf);
                *du_leaf = NULL;
                return rc;
            }
        }
        else if (rc == 0) /* free Data-Unit choice - DU get for write */
            rc = asn_free_child_value(*du_leaf, PRIVATE, 0);
    }
    return rc;
}


/* see description in ndn.h */
int
ndn_du_write_plain_int(asn_value *pdu, uint16_t tag, int32_t value)
{
    asn_value *du_leaf;
    int rc;

    if ((rc = ndn_get_du_field(pdu, NDN_DU_WR, tag, &du_leaf)) != 0)
        return rc;


    return asn_write_int32(du_leaf, value, "#plain");
}

/* see description in ndn.h */
int
ndn_du_read_plain_int(const asn_value *pdu, uint16_t tag, int32_t *value)
{
    asn_value *du_leaf;
    int rc;

    if ((rc = ndn_get_du_field((asn_value *)pdu, NDN_DU_RO, tag, &du_leaf))
        != 0)
        return rc; 

    return asn_read_int32(du_leaf, value, "#plain");
}


/* see description in ndn.h */
int
ndn_du_write_plain_string(asn_value *pdu, uint16_t tag, const char *value)
{
    asn_value *du_leaf;
    int rc;

    if ((rc = ndn_get_du_field(pdu, NDN_DU_WR, tag, &du_leaf)) != 0)
        return rc;


    return asn_write_string(du_leaf, value, "#plain");
}

/* see description in ndn.h */
int
ndn_du_read_plain_string(const asn_value *pdu, uint16_t tag, char **value)
{
    asn_value *du_leaf;
    int rc;

    if ((rc = ndn_get_du_field((asn_value *)pdu, NDN_DU_RO, tag, &du_leaf))
        != 0)
        return rc; 

    return asn_read_string(du_leaf, value, "#plain");
}


/* see description in ndn.h */
int
ndn_du_write_plain_oct(asn_value *pdu, uint16_t tag,
                       const uint8_t *value, size_t len) 
{
    asn_value *du_leaf;
    int rc;

    if ((rc = ndn_get_du_field(pdu, NDN_DU_WR, tag, &du_leaf)) != 0)
        return rc; 

    return asn_write_value_field(du_leaf, value, len, "#plain");
}

/* see description in ndn.h */
int
ndn_du_read_plain_oct(const asn_value *pdu, uint16_t tag,
                      uint8_t *value, size_t *len) 
{
    asn_value *du_leaf;
    int rc;

    if ((rc = ndn_get_du_field((asn_value *)pdu, NDN_DU_RO, tag, &du_leaf))
        != 0)
        return rc; 

    return asn_read_value_field(du_leaf, value, len, "#plain");
}
