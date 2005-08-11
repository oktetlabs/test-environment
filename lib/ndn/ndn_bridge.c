/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for Ethernet protocol. 
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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_errno.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_bridge.h"




static asn_named_entry_t _ndn_bpdu_config_ne_array [] = 
{
    { "flags",          &ndn_data_unit_int8_s,
        { PRIVATE, NDN_BRIDGE_FLAGS }},
    { "root-id",        &ndn_data_unit_octet_string_s ,
        { PRIVATE, NDN_BRIDGE_ROOT_ID }},
    { "root-path-cost", &ndn_data_unit_int32_s, 
        { PRIVATE, NDN_BRIDGE_PATH_COST }},
    { "bridge-id",      &ndn_data_unit_octet_string_s,
        { PRIVATE, NDN_BRIDGE_BRIDGE_ID }},
    { "port-id",        &ndn_data_unit_int16_s, 
        { PRIVATE, NDN_BRIDGE_PORT_ID }},
    { "message-age",    &ndn_data_unit_int16_s, 
        { PRIVATE, NDN_BRIDGE_MESSAGE_AGE }},
    { "max-age",        &ndn_data_unit_int16_s, 
        { PRIVATE, NDN_BRIDGE_MAX_AGE }},
    { "hello-time",     &ndn_data_unit_int16_s, 
        { PRIVATE, NDN_BRIDGE_HELLO_TIME }},
    { "forward-delay",  &ndn_data_unit_int16_s, 
        { PRIVATE, NDN_BRIDGE_FORWARD_DELAY }},
};

asn_type ndn_bpdu_config_s =
{
    "BPDU-Content-Config", {PRIVATE, NDN_BRIDGE_CFG}, SEQUENCE, 
    sizeof(_ndn_bpdu_config_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_bpdu_config_ne_array}
};



static asn_named_entry_t _ndn_bpdu_content_ne_array [] = 
{
    { "cfg",   &ndn_bpdu_config_s , { PRIVATE, NDN_BRIDGE_CFG }},
    { "tcn",   &asn_base_null_s , { PRIVATE, NDN_BRIDGE_TCN }},
};

asn_type ndn_bpdu_content_s =
{
    "BPDU-Content", {APPLICATION, NDN_BRIDGE_CONTENT}, CHOICE, 
    sizeof(_ndn_bpdu_content_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_bpdu_content_ne_array}
}; 




static asn_named_entry_t _ndn_bridge_pdu_ne_array [] = 
{
    { "proto-id",  &ndn_data_unit_int16_s ,
        { PRIVATE, NDN_BRIDGE_PROTO_ID }},
    { "version-id",&ndn_data_unit_int8_s , 
        { PRIVATE, NDN_BRIDGE_VERSION_ID }},
    { "bpdu-type", &ndn_data_unit_int8_s, 
        { PRIVATE, NDN_BRIDGE_BPDU_TYPE }},
    { "content",   &ndn_bpdu_content_s, 
        { PRIVATE, NDN_BRIDGE_CONTENT }},
};

asn_type ndn_bridge_pdu_s =
{
    "Bridge-PDU", {PRIVATE, NDN_TAD_BRIDGE}, SEQUENCE, 
    sizeof(_ndn_bridge_pdu_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_bridge_pdu_ne_array}
};

const asn_type * const ndn_bridge_pdu = &ndn_bridge_pdu_s;








static asn_named_entry_t _ndn_bridge_csap_ne_array [] = 
{
    { "proto-id",  &ndn_data_unit_int16_s ,
        { PRIVATE, NDN_BRIDGE_PROTO_ID }},
    { "version-id",&ndn_data_unit_int8_s , 
        { PRIVATE, NDN_BRIDGE_VERSION_ID }},
    { "bpdu-type", &ndn_data_unit_int8_s, 
        { PRIVATE, NDN_BRIDGE_BPDU_TYPE }},
    { "content",   &ndn_bpdu_content_s, 
        { PRIVATE, NDN_BRIDGE_CONTENT }},
};

asn_type ndn_bridge_csap_s =
{
    "Bridge-CSAP", {PRIVATE, NDN_TAD_BRIDGE}, SEQUENCE, 
    sizeof(_ndn_bridge_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_bridge_csap_ne_array}
};

const asn_type * const ndn_bridge_csap = &ndn_bridge_csap_s;


/** 
 * Convert Bridge STP PDU ASN value to plain C structure. 
 * 
 * @param pkt           - ASN value of type Bridge-PDU or Generic-PDU with 
 *                        choice "eth". 
 * @param bpdu          - converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 
int 
ndn_bpdu_asn_to_plain(const asn_value *pkt, ndn_stp_bpdu_t *bpdu) 
{
    int      rc = 0;
    size_t   len;
    uint16_t proto_id = 0;

    /* @todo insert normal log, but currently this is impossible,
     * because this library linked to both TA and tests. */
    len = sizeof(proto_id);
    rc = asn_read_value_field (pkt, &proto_id, &len, "proto-id.#plain"); 

    /* STP PDU marked by zero Protocol Identifier, according with 
     * IEEE 802.1D, item 9.3.2 */
    if (rc == 0 && proto_id != 0) 
    {
        return TE_EINVAL;
    }
    rc = 0;

#define READ_BPDU_FIELD(fld, label) \
    do if (rc == 0)                                                       \
    {                                                                     \
        len = sizeof(bpdu->fld);                                          \
        rc = asn_read_value_field(pkt, &bpdu->fld, &len, label ".#plain");\
    } while (0)

    READ_BPDU_FIELD(version, "version-id" ); 
    READ_BPDU_FIELD(bpdu_type, "bpdu-type" );

#define READ_BPDU_CFG_FIELD(fld, label) \
    do {                                                                \
        if (rc == 0)                                                    \
        {                                                               \
            len = sizeof(bpdu->cfg.fld);                                \
            rc = asn_read_value_field(pkt, &bpdu->cfg.fld, &len,        \
                                      "content.#cfg." label ".#plain"); \
        }                                                               \
    } while (0)

    READ_BPDU_CFG_FIELD(flags, "flags" ); 
    if (rc == TE_EASNINCOMPLVAL || rc == TE_EASNOTHERCHOICE)
    {
        return 0;
    }

    if (rc == 0)
    {
        len = sizeof(bpdu->cfg.root_id);
        rc = asn_read_value_field(pkt, bpdu->cfg.root_id, &len, 
                                  "content.#cfg.root-id.#plain");
    }

    READ_BPDU_CFG_FIELD(root_path_cost, "root-path-cost" );
    if (rc == 0)
    {
        len = sizeof(bpdu->cfg.bridge_id);
        rc = asn_read_value_field(pkt, bpdu->cfg.bridge_id, &len, 
                                  "content.#cfg.bridge-id.#plain");
    }

    READ_BPDU_CFG_FIELD(port_id,    "port-id" );
    READ_BPDU_CFG_FIELD(msg_age,    "message-age" );
    READ_BPDU_CFG_FIELD(max_age,    "max-age" );
    READ_BPDU_CFG_FIELD(hello_time, "hello-time" );
    READ_BPDU_CFG_FIELD(fwd_delay,  "forward-delay" ); 

#undef READ_BPDU_FIELD
#undef READ_BPDU_CFG_FIELD

    return rc;
}


/** 
 * Convert plain C structure with Bridge STP PDU to respective ASN value. 
 * 
 * @param bpdu          - structure to be converted.
 *
 * @retval      pointer to created ASN value on success.
 * @retval      NULL on memory allocation error.
 */ 
asn_value * 
ndn_bpdu_plain_to_asn(const ndn_stp_bpdu_t *bpdu)
{
    asn_value *new_value;
    int rc = 0;

    new_value = asn_init_value(ndn_bridge_pdu);
    if (new_value == NULL)
        return NULL;

#define WRITE_BPDU_FIELD(fld_, label_) \
    do if (rc == 0)                                                     \
    {                                                                   \
        rc = asn_write_value_field(new_value, &bpdu-> fld_ ,            \
                                   sizeof(bpdu->fld_),label_ ".#plain");\
    } while (0)

    WRITE_BPDU_FIELD(version, "version-id" ); 
    WRITE_BPDU_FIELD(bpdu_type, "bpdu-type" );


    if (bpdu->bpdu_type == STP_BPDU_TCN_TYPE)
    {
        rc = asn_write_value_field (new_value, NULL, 0, "content.#tcn");
        if (rc)
        {
            asn_free_value(new_value);
            return NULL;
        }
        return new_value;
    }
    else if (bpdu->bpdu_type != STP_BPDU_CFG_TYPE)
    {
        asn_free_value(new_value);
        return NULL; 
    }



#define WRITE_BPDU_CFG_FIELD(fld_, label_) \
    do                                                                    \
    {                                                                     \
        if (rc == 0)                                                      \
            rc = asn_write_value_field(new_value, &bpdu->cfg.             \
                                       fld_, sizeof(bpdu->cfg.fld_),      \
                                       "content.#cfg." label_ ".#plain"); \
    } while (0)

    WRITE_BPDU_CFG_FIELD(flags, "flags" ); 

    if (rc == 0)
    {
        rc = asn_write_value_field(new_value, bpdu->cfg.root_id, 
                                   sizeof(bpdu->cfg.root_id), 
                                   "content.#cfg.root-id.#plain");
    }

    WRITE_BPDU_CFG_FIELD(root_path_cost, "root-path-cost" );
    if (rc == 0)
    {
        int len = sizeof(bpdu->cfg.bridge_id);
        rc = asn_write_value_field(new_value, bpdu->cfg.bridge_id, len, 
                                   "content.#cfg.bridge-id.#plain");
    }


    WRITE_BPDU_CFG_FIELD(port_id, "port-id" );
    WRITE_BPDU_CFG_FIELD(msg_age, "message-age" );
    WRITE_BPDU_CFG_FIELD(max_age, "max-age" );
    WRITE_BPDU_CFG_FIELD(hello_time, "hello-time" );
    WRITE_BPDU_CFG_FIELD(fwd_delay, "forward-delay" ); 

#undef WRITE_BPDU_FIELD
#undef WRITE_BPDU_CFG_FIELD
    if (rc)
    {
        asn_free_value(new_value);
        return NULL;
    }
    return new_value;

}
