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
#include <stdio.h>

#include "te_defs.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_forw.h"




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
        forw_delay->type = FORW_DELAY_RANDROM;

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
 * @param val           ASN value of type Forwarder-Action-Drop-Params
 * @param forw_drop    converted structure (OUT).
 *
 * @return zero on success or error code.
 */ 

int 
ndn_forw_drop_to_plain(const asn_value *val, ndn_forw_drop_t *forw_drop)
{
    char drop_label[40]; 
    size_t d_len;

    rc = asn_get_choice(val, "", drop_label, sizeof(drop_label));
    if (rc) return rc;

    if (strcmp(drop_label, "random-rate") == 0)
    {
        forw_drop->type = FORW_DROP_RANDROM;
        d_len = sizeof(forw_drop->rate);
        rc = asn_read_value_field(val, "#random-rate", 
                                  &forw_drop->rate, &d_len);
    }
    else
    {
        forw_drop->type = FORW_DROP_PATTERN;
        forw_drop->mask_len = d_len = asn_get_length(val, "#pattern-mask"); 

        forw_drop->pattern_mask = calloc ((d_len >> 3) + 1, 1);

        rc = asn_read_value_field(val, forw_drop->pattern_mask, &d_len, "");
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
ndn_forw_action_to_plain(const asn_value *val, 
                               ndn_forw_action_plain *forw_action)
{
    int rc = 0; 
    int id_len, d_len;

    const asn_value *subval;

    if (val == NULL || forw_action == NULL) 
        return EINVAL;

    id_len = asn_get_length(val, "id");
    if (id_len <= 0)
        return EASNGENERAL;

    d_len = id_len + 1;
    if ((forw_action->id = malloc(d_len)) == NULL )
        return ENOMEM;

    rc = asn_read_value_field(val, forw_action->id, &d_len, "id");
    if (rc) return rc;

    forw_action->id[id_len] = '\0';

    rc = asn_get_subvalue(val, &subval, "delay");
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


    rc = asn_get_subvalue(val, &subval, "drop");
    if (rc == 0) 
    {
        rc = ndn_forw_drop_to_plain(subval, &(forw_action->drop));
        if (rc) return rc;
    }
    else if (rc == EASNINCOMPLVAL)
    {
        forw_action->drop.type = 0; /* uninitalized, default */
        rc = 0;
    }
    else 
        return rc; 

    return 0;
}



