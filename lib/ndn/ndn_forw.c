/** @file
 * @brief Test Environment TAD, Forwarder, NDN.
 *
 * Definitions of ASN.1 types for NDN for Forwarder module. 
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "te_defs.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_forw.h"

#include "logger_api.h"


/*
Forwarder-Action-Delay-Params ::= SEQUENCE {
    delay-min [1] DATA-UNIT {INTEGER},
    delay-max [2] DATA-UNIT {INTEGER}
} 
*/

static asn_named_entry_t _ndn_forw_delay_ne_array [] = 
{
    { "delay-min",   &ndn_data_unit_int32_s},
    { "delay-max",   &ndn_data_unit_int32_s},
};

asn_type ndn_forw_delay_s =
{
    "Forwarder-Action-Delay-Params", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_forw_delay_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_forw_delay_ne_array}
};

const asn_type * const ndn_forw_delay = &ndn_forw_delay_s;


/*
Forwarder-Action-Reorder-Params ::= SEQUENCE {
    type            [0] INTEGER {disabled(0), random(1), reversed(2)},
    timeout         [1] DATA-UNIT {INTEGER},
    reorder-size    [2] DATA-UNIT {INTEGER}
}
*/
static asn_enum_entry_t _ndn_reorder_type_enum_entries[] = 
{ {"disabled", 0}, {"random", 1}, {"reversed",   2}, };
static asn_type ndn_forw_reorder_type_s = {
    "Forw-Delay-Type", {APPLICATION, 15}, ENUMERATED,
    sizeof(_ndn_reorder_type_enum_entries)/sizeof(asn_enum_entry_t), 
    {_ndn_reorder_type_enum_entries}
};


static asn_named_entry_t _ndn_forw_reorder_ne_array [] = 
{
    { "type",          &ndn_forw_reorder_type_s},
    { "timeout",       &ndn_data_unit_int32_s},
    { "reorder-size",  &ndn_data_unit_int16_s},
};

asn_type ndn_forw_reorder_s =
{
    "Forwarder-Action-Reorder-Params", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_forw_reorder_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_forw_reorder_ne_array}
};

const asn_type * const ndn_forw_reorder = &ndn_forw_reorder_s;


/*
Forwarder-Action-Drop-Params ::= CHOICE {
    random-rate      [0] INTEGER(1..100),
    pattern-mask     [1] BIT STRING
} 
*/ 

static asn_named_entry_t _ndn_forw_drop_ne_array [] = 
{
    { "random-rate",  &asn_base_int8_s},
    { "pattern-mask", &asn_base_bitstring_s},
};

asn_type ndn_forw_drop_s =
{
    "Forwarder-Action-Drop-Params", {PRIVATE, 100}, CHOICE, 
    sizeof(_ndn_forw_drop_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_forw_drop_ne_array}
};

const asn_type * const ndn_forw_drop = &ndn_forw_drop_s;



/*
Forwarder-Action ::= SEQUENCE {
    id        [0] UniversalString,
    delay     [1] Forwarder-Action-Delay-Params OPTIONAL,
    reorder   [3] Forwarder-Action-Reorder-Params OPTIONAL,
    drop      [4] Forwarder-Action-Drop-Params OPTIONAL
}

*/

static asn_named_entry_t _ndn_forw_action_ne_array [] = 
{
    { "id",       &asn_base_charstring_s},
    { "delay",    &ndn_forw_delay_s },
    { "reorder",  &ndn_forw_reorder_s },
    { "drop",     &ndn_forw_drop_s },
};

asn_type ndn_forw_action_s =
{
    "Forwarder-Action", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_forw_action_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_forw_action_ne_array}
};

const asn_type * const ndn_forw_action = &ndn_forw_action_s;








/*******************************************************
 *                    Utilities 
 *******************************************************/

/** 
 * Convert Forwarder-Action ASN value to plain C structrue. 
 * 
 * @param val           ASN value of type Forwarder-Action-Delay-Params
 * @param forw_dealy    converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 

int 
ndn_forw_delay_to_plain(const asn_value *val, ndn_forw_delay_t *forw_delay)
{
    int rc = 0;
    int d_len; 

    if (val == NULL || forw_delay == NULL) 
        return EINVAL;

    d_len = sizeof (forw_delay->delay_min);
    rc = asn_read_value_field(val, &(forw_delay->delay_min), &d_len, 
                                    "delay_min.#plain"); 
    if (rc)
        return rc;

    d_len = sizeof (forw_delay->delay_max);
    rc = asn_read_value_field(val, &(forw_delay->delay_max), &d_len, 
                                    "delay_max.#plain"); 
    if (rc)
        return rc;

    if (forw_delay->delay_max == forw_delay->delay_min)
        forw_delay->type = FORW_DELAY_CONSTANT;
    else 
        forw_delay->type = FORW_DELAY_RANDOM;

    return rc;
}


/** 
 * Convert Forwarder-Action ASN value to plain C structrue. 
 * 
 * @param val           ASN value of type Forwarder-Action-Reorder-Params
 * @param forw_reorder    converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 

int 
ndn_forw_reorder_to_plain(const asn_value *val, ndn_forw_reorder_t *forw_reorder)
{
    UNUSED(val);
    UNUSED(forw_reorder);

    return EASNINCOMPLVAL; /* unsupported, imitate absent */
}

/** 
 * Convert Forwarder-Action ASN value to plain C structrue. 
 * 
 * @param val          ASN value of type Forwarder-Action-Params
 * @param forw_drop    converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 

int 
ndn_forw_drop_to_plain(const asn_value *val, ndn_forw_drop_t *forw_drop)
{
    char drop_label[40]; 
    size_t d_len;
    int rc;

    rc = asn_get_choice(val, "drop", drop_label, sizeof(drop_label));
    RING("%s: get choice: %X", __FUNCTION__, rc); 

    if (rc) return rc;

    if (strcmp(drop_label, "random-rate") == 0)
    {
        forw_drop->type = FORW_DROP_RANDOM;
        d_len = sizeof(forw_drop->rate);
        rc = asn_read_value_field(val, &forw_drop->rate, &d_len, 
                                  "drop.#random-rate");

    RING("%s: get random-rate: %X", __FUNCTION__, rc); 
    }
    else
    {
        forw_drop->type = FORW_DROP_PATTERN;
        forw_drop->mask_len = d_len = 
                        asn_get_length(val, "drop.#pattern-mask"); 

        forw_drop->pattern_mask = calloc ((d_len >> 3) + 1, 1);

        rc = asn_read_value_field(val, forw_drop->pattern_mask, &d_len, 
                                 "drop");

    RING("%s: get pattern-mask: %X", __FUNCTION__, rc); 
    } 

    return rc; 
}

/** 
 * Convert Forwarder-Action ASN value to plain C structrue. 
 * 
 * @param val           ASN value of type  
 * @param forw_action   converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 
int 
ndn_forw_action_asn_to_plain(const asn_value *val, 
                               ndn_forw_action_plain *forw_action)
{
    int rc = 0; 
    int id_len, d_len;

    const asn_value *subval;

    if (val == NULL || forw_action == NULL) 
        return EINVAL;

    id_len = asn_get_length(val, "id");
    RING("%s: length of id %d", __FUNCTION__, id_len);

    if (id_len <= 0)
        return EASNGENERAL;

    d_len = id_len + 1;
    if ((forw_action->id = malloc(d_len)) == NULL )
        return ENOMEM;

    rc = asn_read_value_field(val, forw_action->id, &d_len, "id");
    if (rc) return rc;

    forw_action->id[id_len] = '\0';
    RING("%s: got id: %s", __FUNCTION__, forw_action->id);

    rc = asn_get_subvalue(val, &subval, "delay");
    RING("%s: get delay: %X", __FUNCTION__, rc); 
    if (rc == 0) 
    {
        rc = ndn_forw_delay_to_plain(subval, &(forw_action->delay));
        if (rc) return rc;
    }
    else if (rc == EASNINCOMPLVAL)
    {
        forw_action->delay.type = 0; /* uninitalized, default */
        rc = 0;
    }
    else 
        return rc;



    rc = asn_get_subvalue(val, &subval, "reorder");
    RING("%s: get reorder: %X", __FUNCTION__, rc); 
    if (rc == 0) 
    {
        rc = ndn_forw_reorder_to_plain(subval, &(forw_action->reorder));
        if (rc) return rc;
    }
    else if (rc == EASNINCOMPLVAL)
    {
        forw_action->reorder.type = 0; /* uninitalized, default */
        rc = 0;
    }
    else 
        return rc;


    rc = ndn_forw_drop_to_plain(val, &(forw_action->drop));
    if (rc == EASNINCOMPLVAL)
    {
        forw_action->drop.type = 0; /* uninitalized, default */
        rc = 0;
    }
    else 
        return rc; 

    return 0;
}



/** 
 * Convert plain C structrue to Forwarder-Action ASN value. 
 * 
 * @param forw_action   converted structure.
 * @param result        location for pointer to ASN value of type (OUT)
 *
 * @return zero on success or error code.
 */ 
int 
ndn_forw_action_plain_to_asn(ndn_forw_action_plain *forw_action,
                             asn_value **result)
{
    int rc = 0;
    asn_value *val = NULL;

    if (result == NULL || forw_action == NULL || forw_action->id == NULL)
        return ETEWRONGPTR;

    do { 
        val = asn_init_value(ndn_forw_action);

        rc = asn_write_value_field(val, 
                        forw_action->id, strlen (forw_action->id), "id"); 
        if (rc) break;
                            
        switch (forw_action->drop.type)
        {
        case FORW_DROP_DISABLED:
            break; /* do nothing */
        case FORW_DROP_RANDOM:
            rc = asn_write_value_field(val, 
                                    &forw_action->drop.rate,
                                    sizeof(forw_action->drop.rate),
                                    "drop.#random-rate");
            break;
        case FORW_DROP_PATTERN:
            rc = ETENOSUPP; /* TODO */
        } 
        if (rc) break;
        
        /* TODO delay and reorder */
    } while (0);

    if (rc)
    {
        ERROR("%s failed %X", __FUNCTION__, rc);
        asn_free_value(val);
    }
    else
        *result = val;

    return rc;
}

