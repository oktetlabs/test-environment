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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* for hton* functions */
#include <netinet/in.h>

#include <string.h>

#include "te_defs.h"
#include "te_errno.h" 

#include "asn_impl.h"
#include "ndn.h"
#include "ndn_internal.h"
#include "logger_api.h"

/*
IpAddress ::= OCTET STRING (SIZE(4)) 
*/
asn_type ndn_ip_address_s = 
{ "IpAddress", {APPLICATION, 1}, OCT_STRING, 4, {0}};

const asn_type * const ndn_ip_address = &ndn_ip_address_s;



/* 
 * Interval 
 */ 

static asn_named_entry_t _ndn_interval_ne_array [] = 
{
    { "b", &asn_base_integer_s, {PRIVATE, 1} },
    { "e", &asn_base_integer_s, {PRIVATE, 1} }
};

 asn_type ndn_interval_static = 
{ "Interval", {PRIVATE, 1}, SEQUENCE, 2, {_ndn_interval_ne_array} };

const asn_type * const  ndn_interval = &ndn_interval_static;



asn_type ndn_data_unit_ints_s = 
{   "DATA-UNIT-intervals", {PRIVATE, 2}, SEQUENCE_OF, 0,
    {&ndn_interval_static} 
};

const asn_type * const  ndn_interval_sequence = &ndn_data_unit_ints_s;




asn_type ndn_data_unit_enum_s = 
{ "DATA-UNIT-enum", {PRIVATE, 2}, SET_OF, 1,
   {&asn_base_integer_s} 
};



static asn_named_entry_t _ndn_data_unit_mask_ne_array [] = 
{
    { "v", &asn_base_octstring_s, {PRIVATE, 1} },
    { "m", &asn_base_octstring_s, {PRIVATE, 1} },
    { "free-len", &asn_base_null_s, {PRIVATE, 1} }
}; 

asn_type ndn_data_unit_mask_s = 
{ 
    "DATA-UNIT-mask", {PRIVATE, 2}, SEQUENCE, 3, 
    {_ndn_data_unit_mask_ne_array} 
};



static asn_named_entry_t _ndn_data_unit_env_ne_array [] = 
{
    { "name", &asn_base_charstring_s, {PRIVATE, 1} },
    { "type", &asn_base_enum_s, {PRIVATE, 1} }
}; 

 asn_type ndn_data_unit_env_s = 
{ "DATA-UNIT-env", {PRIVATE, 2}, SEQUENCE, 
  sizeof(_ndn_data_unit_env_ne_array)/sizeof(asn_named_entry_t), 
  {_ndn_data_unit_env_ne_array} 
};






asn_type asn_base_int4_s = 
{ "INTEGER (0..15)", {UNIVERSAL, 2}, INTEGER, 4, {0}};

const asn_type * const  asn_base_int4 = &asn_base_int4_s;


asn_type asn_base_int8_s = 
{ "INTEGER (0..255)", {UNIVERSAL, 2}, INTEGER, 8, {0}};

const asn_type * const  asn_base_int8 = &asn_base_int8_s;


asn_type asn_base_int16_s = 
{ "INTEGER (0..65535)", {UNIVERSAL, 2}, INTEGER, 16, {0}};

const asn_type * const  asn_base_int16 = &asn_base_int16_s;


asn_type ndn_octet_string6_s = 
{  "OCTET STRING (SIZE (6))", {UNIVERSAL, 4}, OCT_STRING, 6, {0}};

const asn_type * const  ndn_octet_string6 = &ndn_octet_string6_s;



NDN_DATA_UNIT_TYPE (int32,         asn_base_integer_s,    INTEGER)
NDN_DATA_UNIT_TYPE (int4,          asn_base_int4_s,       INTEGER(0..15))
NDN_DATA_UNIT_TYPE (int8,          asn_base_int8_s,       INTEGER(0..255))
NDN_DATA_UNIT_TYPE (int16,         asn_base_int16_s,      INTEGER(0..65535))
NDN_DATA_UNIT_TYPE (octet_string,  asn_base_octstring_s,  OCTET STRING)
NDN_DATA_UNIT_TYPE (octet_string6, ndn_octet_string6_s,   OCTET STRING (6))
NDN_DATA_UNIT_TYPE (char_string,   asn_base_charstring_s, UniversalString )
NDN_DATA_UNIT_TYPE (ip_address,    ndn_ip_address_s,      IpAddress )
NDN_DATA_UNIT_TYPE (objid,         asn_base_objid_s,      OBJECT IDENTIFIER)





static asn_named_entry_t _ndn_payload_ne_array [] = 
{
    { "bytes",    &asn_base_octstring_s, {PRIVATE, 1} },
    { "mask",     &ndn_data_unit_mask_s, {PRIVATE, 1} },
    { "function", &asn_base_charstring_s, {PRIVATE, 1} },
    { "filename", &asn_base_charstring_s, {PRIVATE, 1} },
    { "length",   &asn_base_integer_s, {PRIVATE, 1} }
}; 

asn_type ndn_payload_s =
{ "Payload", {PRIVATE, 2}, CHOICE, 
  sizeof(_ndn_payload_ne_array)/sizeof(asn_named_entry_t), 
  {_ndn_payload_ne_array} 
};

const asn_type * const  ndn_payload = &ndn_payload_s;



static asn_type ndn_csap_spec_s =
{ "CSAP-spec", {PRIVATE, 2}, SEQUENCE_OF, 0, {&ndn_generic_csap_level_s} };

const asn_type * const  ndn_csap_spec = &ndn_csap_spec_s;





static asn_type ndn_integer_seq_s = 
{ 
    "SEQENCE OF INTEGER", {PRIVATE, 20}, SEQUENCE_OF, 0,
    {&asn_base_integer_s} 
};

static asn_type ndn_chstring_seq_s = 
{ 
    "SEQENCE OF UniversalString", {PRIVATE, 20}, SEQUENCE_OF, 0,
    {&asn_base_charstring_s} 
};


static asn_named_entry_t _ndn_template_parameter_simple_for_ne_array [] =
{
    { "begin",  &asn_base_integer_s, {PRIVATE, 1} },
    { "end",    &asn_base_integer_s, {PRIVATE, 1} },
    { "step",   &asn_base_integer_s, {PRIVATE, 1} }
}; 

static asn_type ndn_template_parameter_simple_for_s = 
{ 
    "Templ-Param-simple-for", {PRIVATE, 20}, SEQUENCE, 
    sizeof(_ndn_template_parameter_simple_for_ne_array)/
        sizeof(asn_named_entry_t), 
    {_ndn_template_parameter_simple_for_ne_array} 
};


static asn_named_entry_t _ndn_template_parameter_ne_array [] = 
{
    { "ints",       &ndn_integer_seq_s, {PRIVATE, 1} },
    { "strings",    &ndn_chstring_seq_s, {PRIVATE, 1} },
    { "simple-for", &ndn_template_parameter_simple_for_s, {PRIVATE, 1} },
}; 

asn_type ndn_template_parameter_s =
{
    "Template-Parameter", {PRIVATE, 20}, CHOICE,
    sizeof(_ndn_template_parameter_ne_array)/sizeof(asn_named_entry_t), 
    {_ndn_template_parameter_ne_array}
};

const asn_type * const ndn_template_parameter = &ndn_template_parameter_s;





static asn_type ndn_template_parameter_sequence_s = 
{ "SEQENCE OF Template-Parameter", {PRIVATE, 20}, 
  SEQUENCE_OF, 1, {&ndn_template_parameter_s} 
};

static asn_type ndn_generic_pdu_sequence_s = 
{ "Generic-PDU-sequence", {PRIVATE, 20}, 
  SEQUENCE_OF, 1, {&ndn_generic_pdu_s} 
};

const asn_type * const ndn_generic_pdu_sequence = 
                            &ndn_generic_pdu_sequence_s;





static asn_named_entry_t _ndn_traffic_template_ne_array [] = 
{
    { "arg-sets",  &ndn_template_parameter_sequence_s, {PRIVATE, 1} },
    { "delays",    &ndn_data_unit_int32_s, {PRIVATE, 1} },
    { "pdus",      &ndn_generic_pdu_sequence_s, {PRIVATE, 1} },
    { "payload",   &ndn_payload_s, {PRIVATE, 1} },
}; 

asn_type ndn_traffic_template_s =
{ "Traffic-Template", {PRIVATE, 22}, SEQUENCE, 
   sizeof(_ndn_traffic_template_ne_array)/sizeof(asn_named_entry_t), 
   {_ndn_traffic_template_ne_array}
};

const asn_type * const  ndn_traffic_template = &ndn_traffic_template_s;

/*
Packet-Action ::= CHOICE {
    echo        NULL,
    function    UniversalString,
    file        UniversalString
}
*/

static asn_named_entry_t _ndn_packet_action_ne_array [] = 
{
    { "echo",    &asn_base_null_s, {PRIVATE, 1} },
    { "function",&asn_base_charstring_s, {PRIVATE, 1} },
    { "file",    &asn_base_charstring_s, {PRIVATE, 1} },
}; 

asn_type ndn_packet_action_s =
{ "Payload", {PRIVATE, 2}, CHOICE, 
  sizeof(_ndn_packet_action_ne_array)/sizeof(asn_named_entry_t), 
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


static asn_named_entry_t _ndn_traffic_pattern_unit_ne_array [] = 
{
    { "pdus",      &ndn_generic_pdu_sequence_s, {PRIVATE, 1} },
    { "payload",   &ndn_payload_s, {PRIVATE, 1} },
    { "action",    &ndn_packet_action_s, {PRIVATE, 1} },
}; 

asn_type ndn_traffic_pattern_unit_s =
{ "Traffic-Pattern-Unit", {PRIVATE, 20}, SEQUENCE, 
   sizeof(_ndn_traffic_pattern_unit_ne_array)/sizeof(asn_named_entry_t),
  {_ndn_traffic_pattern_unit_ne_array}
};

const asn_type * const  ndn_traffic_pattern_unit = 
                            &ndn_traffic_pattern_unit_s;


/*

Traffic-Pattern ::= SEQUENCE OF Traffic-Pattern-Unit

*/

asn_type ndn_traffic_pattern_s =
{ "Traffic-Pattern", {PRIVATE, 30}, SEQUENCE_OF, 1,
  {&ndn_traffic_pattern_unit_s}
};

const asn_type * const  ndn_traffic_pattern = &ndn_traffic_pattern_s;



/* 
NDN-TimeStamp ::= SEQUENCE {
    seconds INTEGER, -- seconds since Unix epoch
    micro-seconds INTEGER
}
 */
static asn_named_entry_t _ndn_time_stamp_ne_array [] = 
{
    { "seconds",       &asn_base_integer_s, {PRIVATE, 1} },
    { "micro-seconds", &asn_base_integer_s, {PRIVATE, 1} },
}; 

asn_type ndn_time_stamp_s =
{ "NDN-TimeStamp", {PRIVATE, 20}, SEQUENCE, 
   sizeof(_ndn_time_stamp_ne_array)/sizeof(asn_named_entry_t), 
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


static asn_named_entry_t _ndn_raw_packet_ne_array [] = 
{
    { "received",  &ndn_time_stamp_s, {PRIVATE, 1} },
    { "pdus",      &ndn_generic_pdu_sequence_s, {PRIVATE, 1} },
    { "payload",   &ndn_payload_s, {PRIVATE, 1} },
}; 

asn_type ndn_raw_packet_s =
{ "Raw-Packet", {PRIVATE, 20}, SEQUENCE, 
   sizeof(_ndn_raw_packet_ne_array)/sizeof(asn_named_entry_t), 
  {_ndn_raw_packet_ne_array}

};

const asn_type * const  ndn_raw_packet = &ndn_raw_packet_s;





/* See description in ndn.h */ 
int 
ndn_match_data_units(const asn_value *pattern, asn_value *pkt_pdu,
                     uint8_t *data, size_t d_len, const char *label)
{
    asn_syntax        plain_syntax;
    const asn_value  *du_val;
    const asn_type   *val_type;
    const asn_type   *du_type;
    const char       *choice_ptr = NULL;

    int      rc = 0;
    uint32_t user_int;

    if (pattern == NULL || data == NULL || label == NULL)
        return ETEWRONGPTR; 

    rc = asn_impl_find_subvalue(pattern, label, &du_val);
    if (rc && rc != EASNINCOMPLVAL)
        return rc;

    if (rc == 0)
        choice_ptr = asn_get_choice_ptr(du_val);

    if (choice_ptr == NULL && pkt_pdu == NULL)
        return 0; /* data matches, no value should be saved into packet */

    val_type = asn_get_type(pattern);
    rc = asn_impl_find_subtype(val_type, label, &du_type);
    if (rc)
        return rc;

    plain_syntax = du_type->sp.named_entries[0].type->syntax;

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
                    return ETENOSUPP;
                default:
                    user_int = *((uint8_t *)data);
            }
            break;
        default: 
            break; /* No any preliminary actions needed. */
    }

    if (choice_ptr == NULL) 
    {
        /* do nothing, data is matched */
        rc = 0;
    }
    else if (strcmp(choice_ptr, "plain") == 0)
    {
        const uint8_t *pat_data;
        size_t pat_d_len;

        rc = asn_get_field_data(du_val, &pat_data, "#plain"); 
        if (rc) 
            return rc;

        switch (plain_syntax)
        {
            case INTEGER:
            case ENUMERATED:
            {
                uint32_t pat_int = *((uint32_t *)pat_data);
                if (pat_int != user_int)
                    rc = ETADNOTMATCH;
            }
            break;
            case BIT_STRING:
                rc = ETENOSUPP; /* TODO */
                break;
            case OCT_STRING:
            case CHAR_STRING: 
                pat_d_len = asn_get_length(du_val, "");
                if (pat_d_len != d_len || 
                    memcmp(data, pat_data, d_len) != 0)
                    rc = ETADNOTMATCH;
                break;
            default:
                WARN("%s, compare with plain of syntax %d unsupported yet",
                     __FUNCTION__, plain_syntax);
                break;
        }
    }
    else if (strcmp(choice_ptr, "mask") == 0)
    { 
        const asn_value *mask_val;
        size_t           mask_len;

        const uint8_t *mask_data;
        const uint8_t *pat_data;
        const uint8_t *m, *d, *p;


        rc = asn_get_subvalue(du_val, &mask_val, "");
        if (rc)
            return rc; 
        mask_len = asn_get_length(mask_val, "m");
        if (mask_len != d_len)
            return ETADNOTMATCH;

        rc = asn_get_field_data(mask_val, &mask_data, "m");
        if (rc == 0)
            rc = asn_get_field_data(mask_val, &pat_data, "v");
        if (rc)
            return rc; 
        for (d = data, m = mask_data, p = pat_data;
             mask_len > 0; 
             d++, p++, m++)
            if ((*d & *m) != (*p & *m))
            {
                rc = ETADNOTMATCH;
                break;
            }
    }
    else
    {
        WARN("%s, DATA-UNIT choice %s unsupported", 
             __FUNCTION__, choice_ptr);
        rc = ETENOSUPP;
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
                rc = asn_write_value_field(pkt_pdu, &user_int,
                                           sizeof(user_int), labels_buffer);
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
 * return zero on success or appropriate error code otherwise.
 */
int
ndn_get_timestamp(const asn_value *packet, struct timeval *ts)
{ 
    int rc, len;

    len = sizeof(ts->tv_sec);
    rc = asn_read_value_field(packet, &ts->tv_sec, &len, 
                              "received.seconds");

    len = sizeof(ts->tv_usec);
    if (rc == 0)
        rc = asn_read_value_field(packet, &ts->tv_usec, &len, 
                                  "received.micro-seconds");
    return rc;
}

