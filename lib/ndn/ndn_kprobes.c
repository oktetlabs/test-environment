/** @file
 * @brief KPROBES NDN
 *
 * Definitions of ASN.1 types for NDN for Kprobes.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Aleksandr Platonov <Aleksandr.Platonov@oktetlabs.ru>
 *
 * $Id:$
 */

#if defined(__linux__)

#include "te_config.h"

#include "te_defs.h"
#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn.h"
#include "ndn_kprobes.h"

typedef enum {
    NDN_KPROBES_FUNCTION,
    NDN_KPROBES_ACTION,
    NDN_KPROBES_INTERCOUNT,
    NDN_KPROBES_RETVAL,
    NDN_KPROBES_BLOCKTIMEOUT
} ndn_kprobes_scenario_item_tags_t;

typedef enum {
    NDN_KPROBES_SCENARIO_ITEM
} ndn_kprobes_scenario_tags_t;

typedef enum {
    NDN_KPROBES_SCENARIO
} ndn_kprobes_scenarios_tags_t;

typedef enum {
    NDN_KPROBES_SCENARIOS,
    NDN_KPROBES_EXPRESULT,
} ndn_kprobes_scenarios_sequence_tags_t;

typedef enum {
    NDN_KPROBES_PACKET
} ndn_kprobes_packet_tags_t;

static asn_named_entry_t _ndn_kprobes_scenario_item_ne_array[] = {
    { "function", &asn_base_charstring_s,  {PRIVATE, NDN_KPROBES_FUNCTION} },
    { "action", &asn_base_charstring_s,  {PRIVATE, NDN_KPROBES_ACTION} },
    { "interceptcount", &asn_base_integer_s,  {PRIVATE, NDN_KPROBES_INTERCOUNT} },
    { "retval", &asn_base_integer_s, {PRIVATE, NDN_KPROBES_RETVAL} },
    { "blocktimeout", &asn_base_integer_s,  {PRIVATE, NDN_KPROBES_BLOCKTIMEOUT} }
};

static asn_type ndn_kprobes_scenario_item_s = {
    "kprobes-scenario_item", {PRIVATE, NDN_KPROBES_SCENARIO_ITEM}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_kprobes_scenario_item_ne_array),
    {_ndn_kprobes_scenario_item_ne_array}
};

const asn_type * const ndn_kprobes_scenario_item = &ndn_kprobes_scenario_item_s;

static asn_type ndn_kprobes_scenario_s = {
    "kprobes-scenario", {PRIVATE, NDN_KPROBES_SCENARIO},
    SEQUENCE_OF, 0, {subtype: &ndn_kprobes_scenario_item_s}
};

const asn_type * const ndn_kprobes_scenario =
                            &ndn_kprobes_scenario_s;

static asn_type ndn_kprobes_scenarios_s = {
    "kprobes-scenarios", {PRIVATE, NDN_KPROBES_SCENARIOS},
    SEQUENCE_OF, 0, {subtype: &ndn_kprobes_scenario_s}
};

static asn_named_entry_t _ndn_kprobes_packet_ne_array[] = {
    { "scenarios", &ndn_kprobes_scenarios_s, {PRIVATE, NDN_KPROBES_SCENARIOS} },
    { "expresult", &asn_base_charstring_s, {PRIVATE, NDN_KPROBES_EXPRESULT} }
};

asn_type ndn_kprobes_packet_s = {
    "Kprobes-Packet", {PRIVATE, NDN_KPROBES_PACKET}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_kprobes_packet_ne_array),
    {_ndn_kprobes_packet_ne_array}
};

asn_type * ndn_kprobes_packet = &ndn_kprobes_packet_s;

typedef struct kprobes_map_s
{
    char *id;
    int   code;
} kprobes_map_t;

static kprobes_map_t kprobes_expresult_map[] = {
    {"KPROBES_LOAD_FAIL", KPROBES_FAULTS_DRV_LOAD_FAIL},
    {"KPROBES_IF_CREATE_FAIL", KPROBES_FAULTS_IF_CREATE_FAIL},
    {"KPROBES_NO_FAIL", KPROBES_FAULTS_NO_FAIL},
    {NULL, 0}
};

static kprobes_map_t kprobes_action_map[] = {
    {"fail", TE_KPROBES_ACTION_FAIL},
    {"skip", TE_KPROBES_ACTION_SKIP},
    {"block", TE_KPROBES_ACTION_BLOCK},
    {"unblock", TE_KPROBES_ACTION_UNBLOCK}
};

static int
kprobes_map_code(kprobes_map_t *table, const char *id)
{
    int val;
    char *tail;

    for (; table->id != NULL; table++)
    {
        if (strcmp(table->id, id) == 0)
            return table->code;
    }
    val = strtol(id, &tail, 0);
    if (*tail != '\0')
        return -1;
    return val;
}

/**
 * Function parses the kprobes info asn string, fills given
 * array of kprobes_info_t structures.
 *
 * @param kprobes_info_str     Kprobes info in asn string representation
 * @param expresult            (OUT) Expected driver load result 
 *                             from kprobes info
 * @param kprobes_info         (OUT) Array of structures, 
 *                             which represents kptobes info
 * @param number_of_structures (OUT)Number of structures in kprobes_info array
 *
 * @return 0 on success
 */
int
ndn_kprobes_parse_info(const char *kprobes_info_str, int *expresult, 
                       kprobes_info_t **kprobes_info, 
                       int *number_of_structures)
{
    asn_value *kprobes_info_asn = NULL;
    int        rc;
    int        s_parsed;    
    int        i;
    int        j;
    char       request[KPROBES_MAX_FUNC_NAME] = {0};
    char      *action_str = NULL;
    char      *function_name = NULL;
    char      *expresult_str = NULL;
    
    *number_of_structures = 0;

    /* string2asn */
    if ((rc = asn_parse_value_text(kprobes_info_str, ndn_kprobes_packet,
                                   &kprobes_info_asn, &s_parsed)) != 0)
        return rc;
    
    /* While we can get function name */
    for (i = 0; ; i++)
    {
        for (j = 0; ; j++, (*number_of_structures)++)
        {
            *kprobes_info =
                (kprobes_info_t*)realloc((void *)*kprobes_info,
                                         sizeof(kprobes_info_t) * 
                                            ((*number_of_structures) + 1));
            /* Get function name */
            sprintf(request, "scenarios.%d.%d.function", i, j);
            if (asn_read_string(kprobes_info_asn, &function_name, request))
                break;
            strcpy((*kprobes_info)[*number_of_structures].function_name,
                   function_name);
            
            /* Get action for function */
            sprintf(request, "scenarios.%d.%d.action", i, j);
            if ((rc = asn_read_string(kprobes_info_asn, &action_str, request))
                != 0)
                return rc;

            (*kprobes_info)[*number_of_structures].action = 
                kprobes_map_code(kprobes_action_map, action_str);
        
            /* Get intercept count */
            sprintf(request, "scenarios.%d.%d.interceptcount", i, j);
            if ((rc = asn_read_int32(kprobes_info_asn,
                                     &((*kprobes_info)[*number_of_structures].
                                        intercept_count),
                                     request)) != 0)
                return rc;
            
            /* Get retval if needed */
            if ((*kprobes_info)[*number_of_structures].action ==
                TE_KPROBES_ACTION_FAIL)
            {
                sprintf(request, "scenarios.%d.%d.retval", i, j);
                if ((rc = asn_read_int32(kprobes_info_asn,
                                         &((*kprobes_info)
                                            [*number_of_structures].
                                                retval),
                                         request)) != 0)
                    return rc;
            }
            else
                (*kprobes_info)[*number_of_structures].retval = 0;
        
            /* Get block timeout if needed */
            if ((*kprobes_info)[*number_of_structures].action == 
                TE_KPROBES_ACTION_BLOCK)
            {
                sprintf(request, "scenarios.%d.%d.blocktimeout", i, j);
                if ((rc = asn_read_int32(kprobes_info_asn,
                                   &((*kprobes_info)[*number_of_structures].
                                       block_timeout),
                                   request)) != 0)
                    return rc;
            }
            else
                (*kprobes_info)[*number_of_structures].block_timeout = 0;
           (*kprobes_info)[*number_of_structures].scenario_index = i + 1;
        }
        if (j == 0)
            break;
    }
    /* Get expected drivers load result */
    if ((rc = asn_read_string(kprobes_info_asn, &expresult_str, "expresult"))
        != 0)
        return rc;

    *expresult = kprobes_map_code(kprobes_expresult_map, expresult_str);
    
    return 0;
}
#endif /* defined (__linux__) */

