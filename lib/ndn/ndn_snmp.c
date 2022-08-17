/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for file protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_snmp.h"

#if HAVE_NET_SNMP_DEFINITIONS_H
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/definitions.h>
#elif HAVE_UCD_SNMP_SNMP_IMPL_H
#include <sys/types.h>
#include <ucd-snmp/asn1.h>
#include<ucd-snmp/snmp_impl.h>
#else
#error This module cannot be compiled without UCD- or NET-SNMP library
#endif




asn_type ndn_snmp_obj_syntax_integer_s = {
    "INTEGER",
    {APPLICATION, NDN_SNMP_OBJSYN_INT},
    INTEGER, 1,{NULL}
};

asn_type ndn_snmp_obj_syntax_string_s = {
    "OCTET STRING",
    {APPLICATION, NDN_SNMP_OBJSYN_STR},
    OCT_STRING, 0,{NULL}
};

asn_type ndn_snmp_obj_syntax_objid_s = {
    "OBJECT IDENTIFIER",
    {APPLICATION, NDN_SNMP_OBJSYN_OID},
    OID, 0,{NULL}
};

asn_type ndn_snmp_obj_syntax_ipaddr_s = {
    "IpAddress",
    {APPLICATION, NDN_SNMP_OBJSYN_IPADDR},
    OCT_STRING, 4,{NULL}
};

asn_type ndn_snmp_obj_syntax_counter_s = {
    "Counter32",
    {APPLICATION, NDN_SNMP_OBJSYN_COUNTER},
    INTEGER, 0,{NULL}
};

asn_type ndn_snmp_obj_syntax_timeticks_s = {
    "TimeTicks",
    {APPLICATION, NDN_SNMP_OBJSYN_TIMETICKS},
    INTEGER, 0,{NULL}
};

asn_type ndn_snmp_obj_syntax_arbitrary_s = {
    "Opaque",
    {APPLICATION, NDN_SNMP_OBJSYN_ARB},
    OCT_STRING, 0,{NULL}
};

asn_type ndn_snmp_obj_syntax_big_counter_s = {
    "Counter64",
    {APPLICATION, NDN_SNMP_OBJSYN_BIGCOUNTER},
    LONG_INT, 64,{NULL}
};

asn_type ndn_snmp_obj_syntax_unsigned_s = {
    "Unsigned32",
    {APPLICATION, NDN_SNMP_OBJSYN_UINT},
    INTEGER , 0,{NULL}
};


const asn_type * const ndn_snmp_obj_syntax_integer    =
                      &ndn_snmp_obj_syntax_integer_s;
const asn_type * const ndn_snmp_obj_syntax_string     =
                      &ndn_snmp_obj_syntax_string_s;
const asn_type * const ndn_snmp_obj_syntax_objid      =
                      &ndn_snmp_obj_syntax_objid_s;
const asn_type * const ndn_snmp_obj_syntax_ipaddr     =
                      &ndn_snmp_obj_syntax_ipaddr_s;
const asn_type * const ndn_snmp_obj_syntax_counter    =
                      &ndn_snmp_obj_syntax_counter_s;
const asn_type * const ndn_snmp_obj_syntax_timeticks  =
                      &ndn_snmp_obj_syntax_timeticks_s;
const asn_type * const ndn_snmp_obj_syntax_arbitrary  =
                      &ndn_snmp_obj_syntax_arbitrary_s;
const asn_type * const ndn_snmp_obj_syntax_big_counter=
                      &ndn_snmp_obj_syntax_big_counter_s;
const asn_type * const ndn_snmp_obj_syntax_unsigned   =
                      &ndn_snmp_obj_syntax_unsigned_s;


asn_enum_entry_t _ndn_snmp_error_status_enum_entries[] = {
    {"noError", 0},
    {"tooBig", 1},
    {"noSuchName", 2},
    {"badValue", 3},
    {"readOnly", 4},
    {"noAccess", 6},
    {"wrongType", 7},
    {"wrongLength", 8},
    {"wrongEncoding", 9},
    {"wrongValue", 10},
    {"noCreation", 11},
    {"inconsistentValue", 12},
    {"resourceUnavailable", 13},
    {"commitFailed", 14},
    {"undoFailed", 15},
    {"authorizationError", 16},
    {"notWritable", 17},
    {"inconsistentName", 18}
};

asn_type ndn_snmp_error_status_s = {
    "SnmpErrorStatus",
    {UNIVERSAL, 10},
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_snmp_error_status_enum_entries),
    {enum_entries: _ndn_snmp_error_status_enum_entries}
};

asn_type * ndn_snmp_error_status = &ndn_snmp_error_status_s;


NDN_DATA_UNIT_TYPE(snmp_errstat, ndn_snmp_error_status_s, SnmpErrorStatus);

asn_enum_entry_t _ndn_snmp_message_type_enum_entries[] = {
    {"get",     NDN_SNMP_MSG_GET},
    {"get-next",NDN_SNMP_MSG_GETNEXT},
    {"response",NDN_SNMP_MSG_RESPONSE},
    {"set",     NDN_SNMP_MSG_SET},
    {"trap1",   NDN_SNMP_MSG_TRAP1},
    {"trap2",   NDN_SNMP_MSG_TRAP2},
    {"get-bulk",NDN_SNMP_MSG_GETBULK},
    {"inform",  NDN_SNMP_MSG_INFORM},
};

asn_type ndn_snmp_message_type_s = {
    "SnmpMessageType",
    {UNIVERSAL, 10},
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_snmp_message_type_enum_entries),
    {enum_entries: _ndn_snmp_message_type_enum_entries}
};

const asn_type * const ndn_snmp_message_type = &ndn_snmp_message_type_s;


NDN_DATA_UNIT_TYPE(snmp_msgtype, ndn_snmp_message_type_s, SnmpMessageType);


static asn_named_entry_t _ndn_snmp_simple_ne_array[] = {
    { "integer-value",   &ndn_snmp_obj_syntax_integer_s,
        {APPLICATION, NDN_SNMP_OBJSYN_INT} },
    { "string-value",    &ndn_snmp_obj_syntax_string_s,
        {APPLICATION, NDN_SNMP_OBJSYN_STR} },
    { "objectID-value",  &ndn_snmp_obj_syntax_objid_s,
        {APPLICATION, NDN_SNMP_OBJSYN_OID} },
};

asn_type ndn_snmp_simple_s = {
    "SimpleSyntax", {APPLICATION, 1}, CHOICE,
    TE_ARRAY_LEN(_ndn_snmp_simple_ne_array),
    {_ndn_snmp_simple_ne_array}
};

const asn_type * const ndn_snmp_simple = &ndn_snmp_simple_s;



static asn_named_entry_t _ndn_snmp_appl_ne_array [] = {
    {"ipAddress-value",  &ndn_snmp_obj_syntax_ipaddr_s,
        {APPLICATION, NDN_SNMP_OBJSYN_IPADDR} },
    {"counter-value",    &ndn_snmp_obj_syntax_counter_s,
        {APPLICATION, NDN_SNMP_OBJSYN_COUNTER} },
    {"timeticks-value",  &ndn_snmp_obj_syntax_timeticks_s,
        {APPLICATION, NDN_SNMP_OBJSYN_TIMETICKS} },
    {"arbitrary-value",  &ndn_snmp_obj_syntax_arbitrary_s,
        {APPLICATION, NDN_SNMP_OBJSYN_ARB} },
    {"big-counter-value",&ndn_snmp_obj_syntax_big_counter_s,
        {APPLICATION, NDN_SNMP_OBJSYN_BIGCOUNTER} },
    {"unsigned-value",   &ndn_snmp_obj_syntax_unsigned_s,
        {APPLICATION, NDN_SNMP_OBJSYN_UINT} },
};

asn_type ndn_snmp_appl_s = {
    "ApplicationSyntax", {APPLICATION, 1}, CHOICE,
    TE_ARRAY_LEN(_ndn_snmp_appl_ne_array),
    {_ndn_snmp_appl_ne_array}
};

const asn_type * const ndn_snmp_appl = &ndn_snmp_appl_s;




static asn_named_entry_t _ndn_snmp_object_syntax_ne_array [] = {
    { "simple",            &ndn_snmp_simple_s, {PRIVATE, 1} },
    { "application-wide" , &ndn_snmp_appl_s, {PRIVATE, 1} }
};

asn_type ndn_snmp_object_syntax_s = {
    "ObjectSyntax", {APPLICATION, 1}, CHOICE,
    TE_ARRAY_LEN(_ndn_snmp_object_syntax_ne_array),
    {_ndn_snmp_object_syntax_ne_array}
};

const asn_type * const ndn_snmp_object_syntax = &ndn_snmp_object_syntax_s;


NDN_DATA_UNIT_TYPE(object_syntax, ndn_snmp_object_syntax_s, ObjectSyntax);


asn_type snmp_no_such_object_s = {
    "noSuchObject", {CONTEXT_SPECIFIC, 0}, PR_ASN_NULL, 0, {NULL}
};

asn_type snmp_no_such_instance_s = {
    "noSuchInstance", {CONTEXT_SPECIFIC, 1}, PR_ASN_NULL, 0, {NULL}
};

asn_type snmp_end_of_mib_view_s = {
    "endOfMibView", {CONTEXT_SPECIFIC, 2}, PR_ASN_NULL, 0, {NULL}
};


/* This is very simple temporary specification of VarBind!!! */
static asn_named_entry_t _ndn_snmp_var_bind_ne_array[] = {
    { "name",           &ndn_data_unit_objid_s, {PRIVATE, 1} },
    { "value",          &ndn_data_unit_object_syntax_s, {PRIVATE, 1} },
    { "noSuchObject",   &snmp_no_such_object_s, {PRIVATE, 1} },
    { "noSuchInstance", &snmp_no_such_instance_s, {PRIVATE, 1} },
    { "endOfMibView",   &snmp_end_of_mib_view_s, {PRIVATE, 1} }
};

asn_type ndn_snmp_var_bind_s = {
    "VarBind", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_snmp_var_bind_ne_array),
    {_ndn_snmp_var_bind_ne_array}
};

asn_type * ndn_snmp_var_bind = &ndn_snmp_var_bind_s;


asn_type ndn_snmp_var_bind_seq_s = {
    "SEQUENCE OF VarBind", {APPLICATION, 200},
    SEQUENCE_OF, 0, {subtype: &ndn_snmp_var_bind_s}
};

asn_type * ndn_snmp_var_bind_seq = &ndn_snmp_var_bind_seq_s;



static asn_named_entry_t _ndn_snmp_message_ne_array[] = {
    { "type",       &ndn_data_unit_snmp_msgtype_s, {PRIVATE, 1} },
    { "community",  &ndn_data_unit_char_string_s, {PRIVATE, 1} },
    { "repeats",    &ndn_data_unit_int32_s, {PRIVATE, 1} },
    { "request-id", &ndn_data_unit_int32_s, {PRIVATE, 1} },
    { "err-status", &ndn_data_unit_snmp_errstat_s, {PRIVATE, 1} },
    { "err-index",  &ndn_data_unit_int32_s, {PRIVATE, 1} },
    { "enterprise", &ndn_data_unit_objid_s, {PRIVATE, 1} },
    { "gen-trap",   &ndn_data_unit_int32_s, {PRIVATE, 1} },
    { "spec-trap",  &ndn_data_unit_int32_s, {PRIVATE, 1} },
    { "agent-addr", &ndn_data_unit_ip_address_s, {PRIVATE, 1} },
    { "variable-bindings", &ndn_snmp_var_bind_seq_s, {PRIVATE, 1} }
};

asn_type ndn_snmp_message_s = {
    "SNMP-Message", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_snmp_message_ne_array),
    {_ndn_snmp_message_ne_array}
};

asn_type * ndn_snmp_message = &ndn_snmp_message_s;



/** SNMPv3 USM Security level */
static asn_enum_entry_t _ndn_snmp_security_level_enum_entries[] = {
    {"noAuth",      NDN_SNMP_SEC_LEVEL_NOAUTH},
    {"authNoPriv",  NDN_SNMP_SEC_LEVEL_AUTHNOPRIV},
    {"authPriv",    NDN_SNMP_SEC_LEVEL_AUTHPRIV},
};
asn_type ndn_snmp_security_level_s = {
    "SNMP-Security-Level",
    {UNIVERSAL, 10},
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_snmp_security_level_enum_entries),
    {enum_entries: _ndn_snmp_security_level_enum_entries}
};
const asn_type * const ndn_snmp_security_level = &ndn_snmp_security_level_s;


/** SNMPv3 USM Authentication protocol */
static asn_enum_entry_t _ndn_snmp_auth_proto_enum_entries[] = {
    {"md5",  NDN_SNMP_AUTH_PROTO_MD5},
    {"sha",  NDN_SNMP_AUTH_PROTO_SHA},
};

asn_type ndn_snmp_auth_proto_s = {
    "SNMP-USM-AuthProtocol",
    {UNIVERSAL, 10},
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_snmp_auth_proto_enum_entries),
    {enum_entries: _ndn_snmp_auth_proto_enum_entries}
};

const asn_type * const ndn_snmp_auth_proto = &ndn_snmp_auth_proto_s;


/** SNMPv3 USM Privacy protocol */
static asn_enum_entry_t _ndn_snmp_priv_proto_enum_entries[] = {
    {"des",  NDN_SNMP_PRIV_PROTO_DES},
    {"aes",  NDN_SNMP_PRIV_PROTO_AES},
};

asn_type ndn_snmp_priv_proto_s = {
    "SNMP-USM-PrivProtocol",
    {UNIVERSAL, 10},
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_snmp_priv_proto_enum_entries),
    {enum_entries: _ndn_snmp_priv_proto_enum_entries}
};


/** SNMP User-based security model */
static asn_named_entry_t _ndn_snmp_security_usm_ne_array[] = {
    {"name",           &asn_base_charstring_s, {PRIVATE, 1}},
    {"level",          &ndn_snmp_security_level_s,  {PRIVATE, 1}},
    {"auth-protocol",  &ndn_snmp_auth_proto_s, {PRIVATE, 1}},
    {"auth-pass",      &asn_base_charstring_s, {PRIVATE, 1}},
    {"priv-protocol",  &ndn_snmp_priv_proto_s, {PRIVATE, 1}},
    {"priv-pass",      &asn_base_charstring_s, {PRIVATE, 1}},
};

asn_type ndn_snmp_security_usm_s = {
    "SNMP-Security-USM", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_snmp_security_usm_ne_array),
    {_ndn_snmp_security_usm_ne_array}
};
const asn_type * ndn_snmp_security_usm = &ndn_snmp_security_usm_s;


/** SNMP v2c community-based security model */
static asn_named_entry_t _ndn_snmp_security_v2c_ne_array[] = {
    {"community",      &asn_base_charstring_s, {PRIVATE, 1}},
};
asn_type ndn_snmp_security_v2c_s = {
    "SNMP-Security-v2c", {PRIVATE, 100}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_snmp_security_v2c_ne_array),
    {_ndn_snmp_security_v2c_ne_array}
};
const asn_type * ndn_snmp_security_v2c = &ndn_snmp_security_v2c_s;


/** SNMP Security model */
static asn_named_entry_t _ndn_snmp_security_ne_array[] = {
    {"v2c",  &ndn_snmp_security_v2c_s, {PRIVATE, 1}},
    {"usm" , &ndn_snmp_security_usm_s, {PRIVATE, 1}}
};
asn_type ndn_snmp_security_s = {
    "SNMP-Security", {APPLICATION, 1}, CHOICE,
    TE_ARRAY_LEN(_ndn_snmp_security_ne_array),
    {_ndn_snmp_security_ne_array}
};
const asn_type * const ndn_snmp_security = &ndn_snmp_security_s;


/** SNMP CSAP parameters */
static asn_named_entry_t _ndn_snmp_csap_ne_array[] = {
    { "version",       &ndn_data_unit_int8_s, {PRIVATE, 1} },
    { "remote-port",   &ndn_data_unit_int16_s, {PRIVATE, 1} },
    { "local-port",    &ndn_data_unit_int16_s, {PRIVATE, 1} },
    { "security",      &ndn_snmp_security_s, {PRIVATE, 1} },
    { "timeout",       &ndn_data_unit_int32_s, {PRIVATE, 1} },
    { "snmp-agent",    &ndn_data_unit_char_string_s, {PRIVATE, 1} },
};

asn_type ndn_snmp_csap_s = {
    "SNMP-CSAP", {PRIVATE, 101}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_snmp_csap_ne_array),
    {_ndn_snmp_csap_ne_array}
};

asn_type * ndn_snmp_csap = &ndn_snmp_csap_s;

int snmp_asn_syntaxes[] = {
    ASN_INTEGER,
    ASN_OCTET_STR,
    ASN_OBJECT_ID,
    ASN_IPADDRESS,
    ASN_UNSIGNED,
    ASN_TIMETICKS,
    ASN_OCTET_STR,
#ifdef OPAQUE_SPECIAL_TYPES
    ASN_OPAQUE_U64,
#else
    ASN_OCTET_STR,
#endif
    ASN_UNSIGNED
};

/* See the description in ndn_snmp.h */
const char *
ndn_snmp_msg_type_h2str(ndn_snmp_msg_t msg_type)
{
    static char buf[255];

#define NDN_SNMP_MSG_H2STR(msg_) \
        case NDN_SNMP_MSG_ ## msg_: return #msg_

    switch (msg_type)
    {
        NDN_SNMP_MSG_H2STR(GET);
        NDN_SNMP_MSG_H2STR(GETNEXT);
        NDN_SNMP_MSG_H2STR(RESPONSE);
        NDN_SNMP_MSG_H2STR(SET);
        NDN_SNMP_MSG_H2STR(TRAP1);
        NDN_SNMP_MSG_H2STR(TRAP2);
        NDN_SNMP_MSG_H2STR(GETBULK);
        NDN_SNMP_MSG_H2STR(INFORM);
    }
#undef NDN_SNMP_MSG_H2STR

    snprintf(buf, sizeof(buf), "UNKNOWN (%d)", msg_type);
    return buf;
}
