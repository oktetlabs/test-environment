/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Dummy FILE protocol implementaion, layer-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#undef SNMPDEBUG
#include "tad_snmp_impl.h"

#define TE_LGR_USER     "TAD SNMP"
#include "logger_ta.h"

#include <string.h>
#include "te_stdint.h"

void
tad_snmp_free_pdu(void *ptr)
{
    snmp_free_pdu((struct snmp_pdu *)ptr);
}

/**
 * Callback for read parameter value of "snmp" CSAP.
 *
 * @param csap_id       identifier of CSAP.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
char* snmp_get_param_cb(int csap_id, int level, const char *param)
{
    UNUSED(csap_id);
    UNUSED(level);
    UNUSED(param);
    return NULL;
}

/**
 * Callback for confirm PDU with 'snmp' CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int
snmp_confirm_pdu_cb(int csap_id, int layer, asn_value_p tmpl_pdu)
{
    UNUSED(csap_id);
    UNUSED(layer);
    UNUSED(tmpl_pdu);
    VERB("%s, csap %d, layer %d", __FUNCTION__, csap_id, layer);
    return 0;
}

/**
 * Callback for generate binary data to be sent to media.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU. 
 * @param up_payload    pointer to data which is already generated for upper 
 *                      layers and is payload for this protocol level. 
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one element, 
 *                      but need in fragmentation sometimes may occur. 
 *                      Of cause, on up level only one PDU is passed, 
 *                      but upper layer (if any present) may perform 
 *                      fragmentation, and current layer may have possibility 
 *                      to de-fragment payload.
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
int
snmp_gen_bin_cb(int csap_id, int layer, const asn_value *tmpl_pdu,
                const tad_tmpl_arg_t *args, size_t  arg_num, 
                csap_pkts_p up_payload, csap_pkts_p pkts)
{
    int rc; 
    int operation;
    int operation_len = sizeof(int);
    int ucd_snmp_op;
    int num_var_bind;
    int i;

    struct snmp_pdu *pdu;

    UNUSED(args);
    UNUSED(arg_num);
    UNUSED(csap_id);
    UNUSED(up_payload); 

    VERB("%s, layer %d", __FUNCTION__, layer);

    memset(pkts, 0, sizeof(*pkts));

    rc = asn_read_value_field(tmpl_pdu, &operation, &operation_len, "type");
    if (rc) return rc;

    VERB("%s, operation %d", __FUNCTION__, operation);
    switch (operation)
    {
        case NDN_SNMP_MSG_GET:     ucd_snmp_op = SNMP_MSG_GET;     break;
        case NDN_SNMP_MSG_GETNEXT: ucd_snmp_op = SNMP_MSG_GETNEXT; break;
        case NDN_SNMP_MSG_GETBULK: ucd_snmp_op = SNMP_MSG_GETBULK; break;
        case NDN_SNMP_MSG_SET:     ucd_snmp_op = SNMP_MSG_SET;     break;
        case NDN_SNMP_MSG_TRAP1:   ucd_snmp_op = SNMP_MSG_TRAP;    break; 
        case NDN_SNMP_MSG_TRAP2:   ucd_snmp_op = SNMP_MSG_TRAP2;   break; 
        default: 
            return ETADWRONGNDS;
    } 
    pdu = snmp_pdu_create(ucd_snmp_op);
    VERB("%s, snmp pdu created 0x%x", __FUNCTION__, pdu);


    if (operation == NDN_SNMP_MSG_GETBULK) 
    {
        int repeats;
        int r_len = sizeof(repeats);

        rc = asn_read_value_field(tmpl_pdu, &repeats, &r_len, "repeats");
        if (rc) 
            pdu->max_repetitions = SNMP_CSAP_DEF_REPEATS;
        else 
            pdu->max_repetitions = repeats;
#ifdef SNMPDEBUG
        printf ("GETBULK on TA, repeats: %d\n", pdu->max_repetitions);
#endif
        pdu->non_repeaters = 0;
    }

    num_var_bind = asn_get_length(tmpl_pdu, "variable-bindings"); 
    for (i = 0; i < num_var_bind; i++)
    {
        asn_value_p var_bind = asn_read_indexed(tmpl_pdu, i, "variable-bindings");
        oid         oid[MAX_OID_LEN];
        int         oid_len = MAX_OID_LEN;
        
        if (var_bind == NULL)
        {
            ERROR("Cannot get VarBind %d from PDU", i);
            rc = EASNGENERAL;
            break;
        }

        rc = asn_read_value_field(var_bind, oid, &oid_len, "name");
        if (rc)
            break;

        switch (operation)
        {
            case NDN_SNMP_MSG_GET:     
            case NDN_SNMP_MSG_GETNEXT:
            case NDN_SNMP_MSG_GETBULK:
                if (snmp_add_null_var(pdu, oid, oid_len) == NULL)
                {
                    ERROR("Cannot add OID into PDU %d", operation);
                }
                break;

            case NDN_SNMP_MSG_SET:   
            case NDN_SNMP_MSG_TRAP1: 
            case NDN_SNMP_MSG_TRAP2: 
            {
                const char* val_name;
                asn_value_p value;
                uint8_t buffer[1000];
                int d_len = sizeof(buffer);


                rc = asn_read_component_value(var_bind, &value, "value");
                if (rc) break; 

                rc = asn_read_value_field(value, buffer, &d_len, "");
                if (rc) break; 

                val_name = asn_get_name(value);
                if (val_name == NULL)
                { 
                    rc = EASNGENERAL;
                    break;
                } 
                snmp_pdu_add_variable ( pdu, oid, oid_len, 
                                        snmp_asn_syntaxes[asn_get_tag(value)], 
                                        buffer, d_len );
            }
        } 
        if (rc) break;
    }


    if (rc)
        snmp_free_pdu(pdu);
    else
    {
        pkts->next = 0;
        pkts->data = pdu;
        pkts->len  = sizeof(*pdu);
        pkts->free_data_cb = tad_snmp_free_pdu;
    }
    VERB("%s rc %X", __FUNCTION__, rc);

    return rc;
}


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   pattern NDS 
 * @param pkt           recevied packet
 * @param payload       rest upper layer payload, if any exists. (OUT)
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int snmp_match_bin_cb(int csap_id, int layer, const asn_value *pattern_pdu,
                      const csap_pkts *pkt, csap_pkts *payload,
                      asn_value *parsed_packet )
{ 
    int type;
    int rc;

    struct snmp_pdu      *pdu = (struct snmp_pdu *)pkt->data;
    struct variable_list *vars;

    asn_value *snmp_msg = NULL;
    asn_value *vb_seq = NULL;

    UNUSED(csap_id);

    if (parsed_packet != NULL)
    {
        snmp_msg = asn_init_value(ndn_snmp_message);
        vb_seq = asn_init_value(ndn_snmp_var_bind_seq);
    }

    if (parsed_packet == NULL && pattern_pdu == NULL)
        return 0;

    /* never use buffer for upper payload. */
    if (payload != NULL)
        memset(payload, 0, sizeof (csap_pkts));

    VERB("%s, layer %d, pdu 0x%x, pdu command: <%d>", 
         __FUNCTION__, layer, pdu, pdu->command);

    switch (pdu->command)
    {
        case SNMP_MSG_GET:
            type = NDN_SNMP_MSG_GET;
            break;

        case SNMP_MSG_GETNEXT:
            type = NDN_SNMP_MSG_GETNEXT;
            break;

        case SNMP_MSG_RESPONSE:
            type = NDN_SNMP_MSG_RESPONSE;
            break;

        case SNMP_MSG_SET:
            type = NDN_SNMP_MSG_SET;
            break;

        case SNMP_MSG_TRAP:
            type = NDN_SNMP_MSG_TRAP1;
            break;

        case SNMP_MSG_TRAP2:
            type = NDN_SNMP_MSG_TRAP2;
            break;

        case SNMP_MSG_GETBULK:
            type = NDN_SNMP_MSG_GETBULK;
            break;

        default:
            RING("%s(): UNKNOWN PDU command %d ",
                  __FUNCTION__, pdu->command);
            return ETADNOTMATCH;
    }

#define CHECK_FIELD(asn_label_, data_, size_) \
    do {                                                        \
        rc = ndn_match_data_units(pattern_pdu, snmp_msg,        \
                                  (uint8_t *)data_, size_,      \
                                  asn_label_);                  \
        if (rc != 0)                                            \
        {                                                       \
            F_VERB("%s: field %s not match, rc %X",             \
                    __FUNCTION__, asn_label_, rc);              \
            return rc;                                          \
        }                                                       \
    } while(0)

#define CHECK_INT_FIELD(asn_label_, value_) \
    do {                                                        \
        uint32_t net_ordered = htonl((uint32_t)value_);         \
                                                                \
        CHECK_FIELD(asn_label_, &net_ordered,                   \
                    sizeof(net_ordered));                       \
    } while(0)

    CHECK_INT_FIELD("type", type); 
    CHECK_FIELD("community", pdu->community, pdu->community_len + 1); 
    CHECK_INT_FIELD("request-id", pdu->reqid);
    CHECK_INT_FIELD("err-status", pdu->errstat);
    CHECK_INT_FIELD("err-index", pdu->errindex); 

    if (pdu->errstat || pdu->errindex)
        RING("in %s, errstat %d, errindex %d",
                __FUNCTION__, pdu->errstat, pdu->errindex);

    if (type == NDN_SNMP_MSG_TRAP1)
    {
        CHECK_FIELD("enterprise", pdu->enterprise, pdu->enterprise_length);
        CHECK_INT_FIELD("gen-trap", pdu->trap_type); 
        CHECK_INT_FIELD("spec-trap", pdu->specific_type); 
        CHECK_FIELD("agent-addr", pdu->agent_addr, sizeof(pdu->agent_addr));
    }

#undef CHECK_INT_FIELD
#undef CHECK_FIELD

    if (parsed_packet != NULL)
    {
        rc = asn_write_component_value(parsed_packet, snmp_msg, "#snmp"); 
        if (rc)
            ERROR("%s, write SNMP message to packet fails %X\n", 
                  __FUNCTION__, rc);
    } 

    do { /* Match VarBinds */
        const asn_value *pat_vb_list;
        int              pat_vb_num, i;

        rc = asn_get_subvalue(pattern_pdu, &pat_vb_list,
                              "variable-bindings");
        if (rc == EASNINCOMPLVAL)
        {
            rc = 0;
            break;
        }
        else if (rc != 0) /* may not happen in normal situation */
        {
            ERROR("SNMP match: get var-binds from pattern fails %X", rc);
            break;
        }

        pat_vb_num = asn_get_length(pat_vb_list, ""); 
        VERB("%s: number of varbinds in pattern %d",
             __FUNCTION__, pat_vb_num);

        for (i = 0; i < pat_vb_num; i++)
        { 
            const asn_value *pat_var_bind;
            const asn_value *pat_vb_value;
            const oid       *pat_oid;
            const uint8_t   *pat_vb_val_data;
            size_t           pat_oid_len;
            asn_syntax       pat_value_syntax;

            rc = asn_get_indexed(pat_vb_list, &pat_var_bind, i);
            if (rc != 0)
            {
                WARN("SNMP match: get of var bind pattern fails %X", rc);
                break;
            } 

            rc = asn_get_field_data(pat_var_bind, 
                                    (const uint8_t **)&pat_oid,
                                    "name.#plain");
            if (rc == EASNINCOMPLVAL)
            {
                /* match OID other then plain patterns not supported yet */
                VERB("SNMP VB match, no name in varbind");
                rc = 0;
                continue;
            }
            pat_oid_len = asn_get_length(pat_var_bind, "name.#plain"); 

            for (vars = pdu->variables;
                 vars != NULL;
                 vars = vars->next_variable)
            {
                VERB("try to match varbind of type %d", 
                     vars->type);
                if (pat_oid_len == vars->name_length && 
                    memcmp(pat_oid, vars->name,
                           pat_oid_len * sizeof(oid)) == 0)
                    break;
            }

            if (vars == NULL) /* No matching varbind found */
            {
                VERB("no varbind found for pat num %d", i);
                rc = ETADNOTMATCH;
                break;
            }

            rc = asn_get_subvalue(pat_var_bind, &pat_vb_value, "value.#plain");

            if (rc == EASNINCOMPLVAL)
            {
                VERB("There is no value in vb pattern, value matches.");
                rc = 0; /* value matches - no pattern for it */
                continue;
            }
            else if (rc == EASNOTHERCHOICE)
            {
                rc = ETENOSUPP; /* value matches - math is not implemented */
                WARN("SNMP match: unsupported choice"
                     " in varbind value pattern");
                break;
            }
            else if (rc != 0)
                break;

            pat_value_syntax = asn_get_syntax(pat_vb_value, ""); 

            rc = asn_get_field_data(pat_vb_value, &pat_vb_val_data, "");
            if (rc != 0)
            {
                ERROR("Unexpected error getting pat vb value data: %X",
                      rc);
                break;
            }
            VERB("pattern value ASN syntax %d, got SNMP varbind type %d",
                 pat_value_syntax, vars->type);

            switch (vars->type)
            {
                case ASN_INTEGER:   
                case ASN_COUNTER:  
                case ASN_UNSIGNED:  
                case ASN_TIMETICKS: 
                    if (pat_value_syntax != INTEGER &&
                        pat_value_syntax != ENUMERATED)
                    {
                        VERB("SNMP VB match, got int syntax, not match");
                        rc = ETADNOTMATCH;
                        break;
                    }
                    VERB("SNMP VB match, got int val %d, pat %d", 
                         *(vars->val.integer), *((int *)pat_vb_val_data));
                    if (*((int *)pat_vb_val_data) != 
                        *(vars->val.integer))
                        rc = ETADNOTMATCH;
                    break;

                case ASN_IPADDRESS: 
                case ASN_OCTET_STR: 
                case ASN_OBJECT_ID: 
                    if (vars->type == ASN_OBJECT_ID)
                    {
                        if (pat_value_syntax != OID)
                            rc = ETADNOTMATCH;
                    }
                    else if (pat_value_syntax != OCT_STRING &&
                             pat_value_syntax != CHAR_STRING)
                        rc = ETADNOTMATCH;

                    if (rc != 0)
                    {
                        VERB("SNMP VB match, got octet str syntax, rc %X",
                             rc);
                    } 
                    else 
                    {
                        size_t vb_data_len;

                        vb_data_len = asn_get_length(pat_vb_value, "");

                        if (pat_value_syntax == OID)
                            vb_data_len *= 4; 

                        if (vb_data_len != vars->val_len)
                        {
                            VERB("SNMP VB match, length not match"
                                 "got len %d, pat len %d", 
                                 vars->val_len, vb_data_len);
                            rc = ETADNOTMATCH;
                            break;
                        }

                        if (memcmp(pat_vb_val_data, vars->val.string, 
                                   vb_data_len) != 0)
                        {
#if 0
                            char buf[1000], *pt = buf;
                            int  i;

                            rc = ETADNOTMATCH;
                            pt += sprintf(pt, "got data: ");
                            for (i = 0; i < vb_data_len; i++)
                                pt += sprintf(pt, "%02x ", vars->val.string[i]);
                            pt += sprintf(pt, " pat data: ");
                            for (i = 0; i < vb_data_len; i++)
                                pt += sprintf(pt, "%02x ", pat_vb_val_data[i]);
                            VERB("SNMP VB, fail: %s", buf);
#endif
                            rc = ETADNOTMATCH;
                        }
                    }
                    VERB("SNMP VB match, values compare rc %X", rc);
                    break;

                default:;
            }
        }

        if (rc != 0)
            break;

    } while(0); /* match on varbinds finished */

    /* fill varbinds into parsed packet */
    for (vars = pdu->variables;
         rc == 0 && vars != NULL;
         vars = vars->next_variable)
    {
        asn_value_p var_bind = NULL;
        char        os_choice[100]; 

        VERB("BEGIN of LOOP rc %X", rc);

        if (parsed_packet != NULL)
        {
            var_bind = asn_init_value(ndn_snmp_var_bind);
            asn_write_value_field(var_bind, vars->name, vars->name_length, 
                                  "name.#plain");
        }

        if (parsed_packet == NULL)
            continue;

        VERB(" SNMP MATCH: vars type: %d\n", vars->type);

        strcpy (os_choice, "value.#plain.");
        switch (vars->type)
        {
            case ASN_INTEGER:   
                strcat(os_choice, "#simple.#integer-value");
                break;
            case ASN_OCTET_STR: 
                strcat(os_choice, "#simple.#string-value");
                break;
            case ASN_OBJECT_ID: 
                strcat(os_choice, "#simple.#objectID-value");
                break;

            case ASN_IPADDRESS: 
                strcat(os_choice, "#application-wide.#ipAddress-value");
                break;
            case ASN_COUNTER:  
                strcat(os_choice, "#application-wide.#counter-value");
                break;
            case ASN_UNSIGNED:  
                strcat(os_choice, "#application-wide.#unsigned-value");
                break;
            case ASN_TIMETICKS: 
                strcat(os_choice, "#application-wide.#timeticks-value");
                break;
#if 0
            case ASN_OCTET_STR: 
                strcat(os_choice, "#application-wide.#arbitrary-value");
                break;
#ifdef OPAQUE_SPECIAL_TYPES 
            case ASN_OPAQUE_U64: 
#else
            case ASN_OCTET_STR:
#endif
                                
                strcat(os_choice, "#application-wide.#big-counter-value");
                break;
            case ASN_UNSIGNED:  
                strcat(os_choice, "#application-wide.#unsigned-value");
                break;
#endif
            case SNMP_NOSUCHOBJECT: 
                strcpy(os_choice, "noSuchObject"); /* overwrite "value..." */
                break;
            case SNMP_NOSUCHINSTANCE: 
                strcpy(os_choice, "noSuchInstance"); /* overwrite "value..." */
                break;
            case SNMP_ENDOFMIBVIEW: 
                strcpy(os_choice, "endOfMibView"); /* overwrite "value..." */
                break;
            default:
                asn_free_value(var_bind);
                return 1;
        }
        VERB("in SNMP MATCH, rc before varbind value write: %x", rc);
        VERB("in SNMP MATCH, try to write for label: %s", os_choice);

        if (rc != 0)
            break;

        rc = asn_write_value_field(var_bind,
                                   vars->val.string,
                                   (vars->type != ASN_OBJECT_ID) ?
                                       vars->val_len :
                                       vars->val_len / sizeof(oid),
                                   os_choice);
        
        VERB("in SNMP MATCH, rc from varbind value write: %x", rc);
        if (rc != 0)
            break;

        rc = asn_insert_indexed(vb_seq, var_bind, -1, "");

        VERB("in SNMP MATCH, rc from varbind insert: %x", rc);

        asn_free_value(var_bind);

        if (rc != 0)
            break;
    } /* end of loop fill of varbinds */

    if (rc == 0 && parsed_packet != NULL)
        rc = asn_write_component_value(parsed_packet, vb_seq,
                                       "#snmp.variable-bindings");

#ifdef SNMPDEBUG
    printf ("in SNMP MATCH, rc from vb_seq insert: %x\n", rc);
#endif

    asn_free_value(vb_seq);

    return rc;
}

/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according 
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return zero on success or error code.
 */
int snmp_gen_pattern_cb(int csap_id, int layer, const asn_value *tmpl_pdu, 
                        asn_value_p *pattern_pdu)
{ 
    UNUSED(tmpl_pdu);

    *pattern_pdu = asn_init_value(ndn_snmp_message);
    VERB("%s callback, CSAP # %d, layer %d", __FUNCTION__, csap_id, layer); 
    return 0;
}


