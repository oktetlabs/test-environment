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

#define TE_LGR_USER "NDN/Forw"

#include "te_defs.h"
#include "te_errno.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_forw.h"

#include "logger_api.h"


/*
Forwarder-Delay-Cont ::= SEQUENCE {
    delay-min [1] INTEGER,
    delay-max [2] INTEGER
} 
*/

static asn_named_entry_t _ndn_forw_delay_cont_ne_array [] = 
{
    { "delay-min",   &asn_base_integer_s, {PRIVATE, 1}},
    { "delay-max",   &asn_base_integer_s, {PRIVATE, 2}},
};

asn_type ndn_forw_delay_cont_s =
{
    "Forwarder-Delay-Cont", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_forw_delay_cont_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_forw_delay_cont_ne_array}
};

const asn_type * const ndn_forw_delay_cont = &ndn_forw_delay_cont_s;

/* 
Discret-Pair ::= SEQUENCE {
    prob        [1] INTEGER(1..100), 
    delay       [2] INTEGER -- in microseconds
} 
*/

static asn_named_entry_t _ndn_forw_discr_pair_array [] = 
{
    { "prob",    &asn_base_int8_s, {PRIVATE, 1}},
    { "delay",   &asn_base_integer_s, {PRIVATE, 2}},
};

asn_type ndn_forw_discr_pair_s =
{
    "Discret-Pair", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_forw_discr_pair_array)/sizeof(asn_named_entry_t),
    {_ndn_forw_discr_pair_array}
};

const asn_type * const ndn_forw_discr_pair = &ndn_forw_discr_pair_s;

/*
Forwarder-Delay-Discrete ::= SEQUENCE OF Discret-Pair;
*/

asn_type ndn_forw_delay_discr_s =
{
    "Forwarder-Delay-Discrete", {PRIVATE, 102}, SEQUENCE_OF, 
    0, {subtype: &ndn_forw_discr_pair_s}
};

/*
Forwarder-Action-Delay-Params ::= CHOICE {
    cont  [1] Forwarder-Delay-Cont,
    discr [2] Forwarder-Delay-Discrete
} 
*/

enum {
    FTASK_DELAY_CONTINOUS = 1,
    FTASK_DELAY_DISCRETE = 2,
};

static asn_named_entry_t _ndn_forw_delay_array [] = 
{
    { "cont",  &ndn_forw_delay_cont_s,  {PRIVATE, FTASK_DELAY_CONTINOUS}},
    { "discr", &ndn_forw_delay_discr_s, {PRIVATE, FTASK_DELAY_DISCRETE}},
};

asn_type ndn_forw_delay_s =
{
    "Discret-Pair", {PRIVATE, 101}, CHOICE, 
    sizeof(_ndn_forw_delay_array)/sizeof(asn_named_entry_t),
    {_ndn_forw_delay_array}
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
{ 
    {"disabled", FORW_REORDER_DISABLED}, 
    {"random",   FORW_REORDER_RANDOM}, 
    {"reversed", FORW_REORDER_REVERSED}, 
};
static asn_type ndn_forw_reorder_type_s = {
    "Forw-Delay-Type", {APPLICATION, 15}, ENUMERATED,
    sizeof(_ndn_reorder_type_enum_entries)/sizeof(asn_enum_entry_t), 
    {enum_entries: _ndn_reorder_type_enum_entries}
};


static asn_named_entry_t _ndn_forw_reorder_ne_array [] = 
{
    { "type",          &ndn_forw_reorder_type_s, {PRIVATE, 1}},
    { "timeout",       &ndn_data_unit_int32_s, {PRIVATE, 1}},
    { "reorder-size",  &ndn_data_unit_int16_s, {PRIVATE, 1}},
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
    random-rate      [0] INTEGER(0..100),
    pattern-mask     [1] BIT STRING
} 
*/ 

static asn_named_entry_t _ndn_forw_drop_ne_array [] = 
{
    { "random-rate",  &asn_base_int8_s, {PRIVATE, 0}},
    { "pattern-mask", &asn_base_bitstring_s, {PRIVATE, 1}},
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
    { "id",       &asn_base_charstring_s, {PRIVATE, 1}},
    { "delay",    &ndn_forw_delay_s, {PRIVATE, 1} },
    { "reorder",  &ndn_forw_reorder_s, {PRIVATE, 1} },
    { "drop",     &ndn_forw_drop_s, {PRIVATE, 1} },
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

    const char *choice_ptr;

    if (val == NULL || forw_delay == NULL) 
        return EINVAL;

    choice_ptr = asn_get_choice_ptr(val);

    if (strcmp(choice_ptr, "cont") == 0)
    {
        d_len = sizeof(forw_delay->min);
        rc = asn_read_value_field(val, &(forw_delay->min), &d_len, 
                                  "#cont.delay-min"); 
        if (rc)
            return rc;

        d_len = sizeof(forw_delay->max);
        rc = asn_read_value_field(val, &(forw_delay->max), &d_len, 
                                  "#cont.delay-max"); 
        if (rc)
            return rc;

        if (forw_delay->max == forw_delay->min)
            forw_delay->type = FORW_DELAY_CONSTANT;
        else 
            forw_delay->type = FORW_DELAY_RAND_CONT;
    }
    else if (strcmp(choice_ptr, "discr") == 0)
    {
        char label[50];
        char *pl;
        int i;
        int ar_len;

        ar_len = asn_get_length(val, "#discr"); 

        forw_delay->type = FORW_DELAY_RAND_DISCR;

        if (ar_len > DELAY_DISCR_MAX)
            ar_len = DELAY_DISCR_MAX;

        forw_delay->n_pairs = ar_len;

        for (i = 0; i < ar_len; i++)
        { 
            pl = label;
            pl += snprintf(pl, sizeof(label), "#discr.%d", i);
            strcpy(pl, ".prob");
            d_len = sizeof(forw_delay->discr[i].prob);
            rc = asn_read_value_field(val, &(forw_delay->discr[i].prob),
                                      &d_len, label);
            if (rc != 0)
                break;

            strcpy(pl, ".delay");
            d_len = sizeof(forw_delay->discr[i].delay);
            rc = asn_read_value_field(val, &(forw_delay->discr[i].delay),
                                      &d_len, label);
            if (rc != 0)
                break;
        }
    }

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
ndn_forw_reorder_to_plain(const asn_value *val,
                          ndn_forw_reorder_t *forw_reorder)
{
    int rc = 0;
    int d_len; 

    if (val == NULL || forw_reorder == NULL) 
        return EINVAL;

    d_len = sizeof (forw_reorder->type);
    rc = asn_read_value_field(val, &forw_reorder->type, &d_len, "type");
    if (rc)
        return rc;

    d_len = sizeof (forw_reorder->timeout);
    rc = asn_read_value_field(val, &forw_reorder->timeout, &d_len, 
                              "timeout");
    if (rc)
        return rc;

    d_len = sizeof (forw_reorder->r_size);
    rc = asn_read_value_field(val, &forw_reorder->r_size, &d_len, 
                              "reorder-size"); 
    VERB("%s: reorder: type %d, timeout %d, size %d", __FUNCTION__,
         forw_reorder->type, forw_reorder->timeout, forw_reorder->r_size);
    return rc;
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

    if (rc) return rc;

    if (strcmp(drop_label, "random-rate") == 0)
    {
        forw_drop->type = FORW_DROP_RANDOM;
        d_len = sizeof(forw_drop->rate);
        rc = asn_read_value_field(val, &forw_drop->rate, &d_len, 
                                  "drop.#random-rate"); 
    }
    else
    {
        forw_drop->type = FORW_DROP_PATTERN;
        forw_drop->mask_len = d_len = 
                        asn_get_length(val, "drop.#pattern-mask"); 

        forw_drop->pattern_mask = calloc ((d_len >> 3) + 1, 1);

        rc = asn_read_value_field(val, forw_drop->pattern_mask, &d_len, 
                                 "drop");
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
    VERB("%s: length of id %d", __FUNCTION__, id_len);

    if (id_len <= 0)
        return EASNGENERAL;

    d_len = id_len + 1;
    if ((forw_action->id = malloc(d_len)) == NULL )
        return ENOMEM;

    rc = asn_read_value_field(val, forw_action->id, &d_len, "id");
    if (rc) return rc;

    forw_action->id[id_len] = '\0';
    VERB("%s: got id: %s", __FUNCTION__, forw_action->id);

    rc = asn_impl_find_subvalue(val, "delay", &subval); 
    VERB("%s: get delay: %X", __FUNCTION__, rc); 
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
    VERB("%s: get reorder: %X", __FUNCTION__, rc); 
    if (rc == 0) 
    {
        rc = ndn_forw_reorder_to_plain(subval, &(forw_action->reorder));
        VERB("%s: reorder to plain: %X", __FUNCTION__, rc); 
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
ndn_forw_action_plain_to_asn(const ndn_forw_action_plain *forw_action,
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
        if (rc != 0)
            break;
                            
        switch (forw_action->drop.type)
        {
            case FORW_DROP_DISABLED:
                break; /* do nothing */
            case FORW_DROP_RANDOM:
                rc = asn_write_value_field(val, &forw_action->drop.rate,
                                           sizeof(forw_action->drop.rate),
                                           "drop.#random-rate");
                break;
            case FORW_DROP_PATTERN:
                rc = asn_write_value_field(val, 
                                           forw_action->drop.pattern_mask,
                                           forw_action->drop.mask_len,
                                           "drop.#pattern-mask");
                break;
        } 
        if (rc)
            break;

        if (forw_action->reorder.type != FORW_REORDER_DISABLED)
        {
            rc = asn_write_value_field(val, &forw_action->reorder.type,
                                       sizeof(forw_action->reorder.type),
                                       "reorder.type");
            if (rc != 0)
                break;
            rc = asn_write_value_field(val, &forw_action->reorder.timeout,
                                       sizeof(forw_action->reorder.timeout),
                                       "reorder.timeout.#plain");
            if (rc != 0)
                break;
            rc = asn_write_value_field(val, &forw_action->reorder.r_size,
                                       sizeof(forw_action->reorder.r_size),
                                       "reorder.reorder-size.#plain");
            if (rc != 0)
                break;
        }

        switch (forw_action->delay.type)
        {
            case FORW_DELAY_DISABLED:
                break; /* nothing to do */

            case FORW_DELAY_CONSTANT:
                /* explicetly discard 'const' qualifier */
                ((ndn_forw_action_plain *)forw_action)->delay.max =
                    forw_action->delay.min;
                /* fall through */
            case FORW_DELAY_RAND_CONT:
                rc = asn_write_value_field(val, &forw_action->delay.min,
                                           sizeof(forw_action->delay.min),
                                           "delay.#cont.delay-min");
                if (rc != 0) 
                    break;
                rc = asn_write_value_field(val, &forw_action->delay.max,
                                           sizeof(forw_action->delay.max),
                                           "delay.#cont.delay-max");
                break;
            case FORW_DELAY_RAND_DISCR:
                {
                    int i;
                    int ar_len; 
                    asn_value *delay_discr;

                    rc = asn_parse_value_text("discr:{}", ndn_forw_delay,
                                              &delay_discr, &ar_len);
                    VERB("parse delay discr rc %X, syms %d", rc, ar_len);
                    if (rc != 0)
                        break;


                    ar_len = forw_action->delay.n_pairs;

                    for (i = 0; i < ar_len && rc == 0; i++)
                    { 
                        asn_value *pair_val = 
                            asn_init_value(ndn_forw_discr_pair);

                        rc = asn_write_value_field
                                (pair_val,  
                                 &forw_action->delay.discr[i].prob,
                                 sizeof(forw_action->delay.discr[i].prob),
                                 "prob");
                        if (rc != 0)
                            break;

                        rc = asn_write_value_field
                                (pair_val,
                                 &forw_action->delay.discr[i].delay,
                                 sizeof(forw_action->delay.discr[i].delay),
                                 "delay");
                        if (rc != 0)
                            break;

                        rc = asn_insert_indexed(delay_discr, 
                                                pair_val, i, "#discr");
                        VERB("%s: insert discr.pair N %d to delay, rc: %X",
                             __FUNCTION__, i, rc);
                        asn_free_value(pair_val);
                    }
                    if (rc != 0)
                        break;
                    rc = asn_write_component_value(val, delay_discr,
                                                   "delay"); 
                    VERB("%s: write delay_discr to ftask action, rc: %X",
                         __FUNCTION__, rc);
                    asn_free_value(delay_discr);
                }
                break;
            default:
                rc = EINVAL;
                ERROR("unsupported type of delay: %d", 
                      forw_action->delay.type);
                break;
        }
        if (rc != 0) 
            break;
        
    } while (0);

    if (rc!= 0)
    {
        ERROR("%s failed %X", __FUNCTION__, rc);
        asn_free_value(val);
    }
    else
        *result = val;

    return rc;
}

