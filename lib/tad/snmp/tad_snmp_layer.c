/** @file
 * @brief TAD SNMP
 *
 * Traffic Application Domain Command Handler.
 * SNMP CSAP implementaion, layer-related callbacks.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD SNMP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#undef SNMPDEBUG
#include "tad_snmp_impl.h"

#include "logger_api.h"
#include "logger_ta_fast.h"

#include <string.h>
#include "te_stdint.h"


/* See description in tad_snmp_impl.h */
te_errno
tad_snmp_gen_bin_cb(csap_p csap, unsigned int layer,
                    const asn_value *tmpl_pdu, void *opaque,
                    const tad_tmpl_arg_t *args, size_t arg_num, 
                    tad_pkts *sdus, tad_pkts *pdus)
{
    int rc; 
    int operation;
    int ucd_snmp_op;
    int num_var_bind;
    int i;

    size_t operation_len = sizeof(int); 

    struct snmp_pdu *pdu;
    const asn_value *var_bind_list;

    UNUSED(csap);
    UNUSED(opaque);
    UNUSED(args);
    UNUSED(arg_num);

    VERB("%s, layer %d", __FUNCTION__, layer);

    rc = asn_read_value_field(tmpl_pdu, &operation, &operation_len, "type");
    if (rc != 0)
        return rc;

    VERB("%s, operation %d", __FUNCTION__, operation);
    switch (operation)
    {
        case NDN_SNMP_MSG_GET:     ucd_snmp_op = SNMP_MSG_GET;     break;
        case NDN_SNMP_MSG_GETNEXT: ucd_snmp_op = SNMP_MSG_GETNEXT; break;
        case NDN_SNMP_MSG_GETBULK: ucd_snmp_op = SNMP_MSG_GETBULK; break;
        case NDN_SNMP_MSG_SET:     ucd_snmp_op = SNMP_MSG_SET;     break;
        case NDN_SNMP_MSG_TRAP1:   ucd_snmp_op = SNMP_MSG_TRAP;    break; 
        case NDN_SNMP_MSG_TRAP2:   ucd_snmp_op = SNMP_MSG_TRAP2;   break;
        case NDN_SNMP_MSG_INFORM:  ucd_snmp_op = SNMP_MSG_INFORM;  break;
        default: 
            return TE_ETADWRONGNDS;
    } 
    pdu = snmp_pdu_create(ucd_snmp_op);
    VERB("%s, snmp pdu created 0x%x", __FUNCTION__, pdu);


    if (operation == NDN_SNMP_MSG_GETBULK) 
    {
        int repeats;
        size_t r_len = sizeof(repeats);

        rc = asn_read_value_field(tmpl_pdu, &repeats, &r_len, "repeats");
        if (rc != 0) 
            pdu->max_repetitions = SNMP_CSAP_DEF_REPEATS;
        else 
            pdu->max_repetitions = repeats;
#ifdef SNMPDEBUG
        printf ("GETBULK on TA, repeats: %d\n", pdu->max_repetitions);
#endif
        pdu->non_repeaters = 0;
    }

    rc = asn_get_subvalue(tmpl_pdu, &var_bind_list, "variable-bindings");
    if (rc != 0)
    {
        ERROR("%s(): get subvalue 'variable-bindings' list failed %r", 
              __FUNCTION__, rc);
        return rc;
    }

    num_var_bind = asn_get_length(var_bind_list, ""); 

    for (i = 0; i < num_var_bind; i++)
    {
        asn_value *var_bind;
        oid              oid[MAX_OID_LEN];
        size_t           oid_len = MAX_OID_LEN;

        if ((rc = asn_get_indexed(var_bind_list, &var_bind, i, NULL)) != 0) 
        {
            ERROR("Cannot get VarBind %d from PDU, rc %r", i, rc);
            break;
        }

        rc = asn_read_value_field(var_bind, oid, &oid_len, "name");
        if (rc != 0)
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
            case NDN_SNMP_MSG_INFORM:
            {
                const char  *val_name;
                asn_value_p  value;
                uint8_t      buffer[1000];
                size_t       d_len = sizeof(buffer);


                rc = asn_read_component_value(var_bind, &value, "value");
                if (rc != 0) break; 

                rc = asn_read_value_field(value, buffer, &d_len, "");
                if (rc != 0) break; 

                val_name = asn_get_name(value);
                if (val_name == NULL)
                { 
                    rc = TE_EASNGENERAL;
                    break;
                } 
                snmp_pdu_add_variable(pdu, oid, oid_len, 
                                      snmp_asn_syntaxes[asn_get_tag(value)],
                                      buffer, d_len );
            }
        } 
        if (rc != 0) break;
    }

    if (rc == 0)
    {
        tad_pkts_move(pdus, sdus);
        rc = tad_pkts_add_new_seg(pdus, TRUE, pdu, sizeof(*pdu),
                                  tad_snmp_free_pdu);
    }

    if (rc != 0)
        snmp_free_pdu(pdu);

    VERB("%s rc %r", __FUNCTION__, rc);

    return rc;
}


/* See description in tad_snmp_impl.h */
te_errno
tad_snmp_match_bin_cb(csap_p           csap,
                      unsigned int     layer,
                      const asn_value *ptrn_pdu,
                      void            *ptrn_opaque,
                      tad_recv_pkt    *meta_pkt,
                      tad_pkt         *pdu,
                      tad_pkt         *sdu)
{ 
    struct snmp_pdu        *my_pdu;
    struct variable_list   *vars;
    asn_value              *snmp_msg = NULL;
    asn_value              *vb_seq = NULL;
    te_errno                rc;
    int                     type;

    UNUSED(ptrn_opaque);
    UNUSED(sdu);

    assert(tad_pkt_seg_num(pdu) == 1);
    assert(tad_pkt_first_seg(pdu) != NULL);
    assert(tad_pkt_first_seg(pdu)->data_len == sizeof(*my_pdu));
    my_pdu = tad_pkt_first_seg(pdu)->data_ptr;
    assert(my_pdu != NULL);

    if (csap->state & CSAP_STATE_RESULTS)
    {
        meta_pkt->layers[layer].nds = snmp_msg =
            asn_init_value(ndn_snmp_message);
        vb_seq = asn_init_value(ndn_snmp_var_bind_seq);
    }
    else if (ptrn_pdu == NULL)
    {
        return 0;
    }

    VERB("%s, layer %d, my_pdu 0x%x, my_pdu command: <%d>", 
         __FUNCTION__, layer, my_pdu, my_pdu->command);

    switch (my_pdu->command)
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

        case SNMP_MSG_INFORM:
            type = NDN_SNMP_MSG_INFORM;
            break;

        default:
            RING("%s(): UNKNOWN PDU command %d ",
                  __FUNCTION__, my_pdu->command);
            return TE_ETADNOTMATCH;
    }

#define CHECK_FIELD(asn_label_, data_, size_) \
    do {                                                        \
        rc = ndn_match_data_units(ptrn_pdu, snmp_msg,           \
                                  (uint8_t *)data_, size_,      \
                                  asn_label_);                  \
        if (rc != 0)                                            \
        {                                                       \
            F_VERB("%s: field %s not match, rc %r",             \
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

    if (my_pdu->community != NULL)
        CHECK_FIELD("community", my_pdu->community, my_pdu->community_len + 1);

    CHECK_INT_FIELD("request-id", my_pdu->reqid);
    CHECK_INT_FIELD("err-status", my_pdu->errstat);
    CHECK_INT_FIELD("err-index", my_pdu->errindex); 

    if (my_pdu->errstat || my_pdu->errindex)
        RING("in %s, errstat %d, errindex %d",
                __FUNCTION__, my_pdu->errstat, my_pdu->errindex);

    if (type == NDN_SNMP_MSG_TRAP1)
    {
        CHECK_FIELD("enterprise", my_pdu->enterprise, my_pdu->enterprise_length);
        CHECK_INT_FIELD("gen-trap", my_pdu->trap_type); 
        CHECK_INT_FIELD("spec-trap", my_pdu->specific_type); 
        CHECK_FIELD("agent-addr", my_pdu->agent_addr, sizeof(my_pdu->agent_addr));
    }

#undef CHECK_INT_FIELD
#undef CHECK_FIELD

    do { /* Match VarBinds */
        const asn_value *pat_vb_list;
        int              pat_vb_num, i;

        rc = asn_get_subvalue(ptrn_pdu, &pat_vb_list,
                              "variable-bindings");
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            rc = 0;
            break;
        }
        else if (rc != 0) /* may not happen in normal situation */
        {
            ERROR("SNMP match: get var-binds from pattern fails %r", rc);
            break;
        }

        pat_vb_num = asn_get_length(pat_vb_list, ""); 
        VERB("%s: number of varbinds in pattern %d",
             __FUNCTION__, pat_vb_num);

        for (i = 0; i < pat_vb_num; i++)
        { 
            asn_value *pat_var_bind;
            const asn_value *pat_vb_value;
            const oid       *pat_oid;
            const uint8_t   *pat_vb_val_data;
            size_t           pat_oid_len;
            asn_syntax       pat_value_syntax;

            rc = asn_get_indexed(pat_vb_list, &pat_var_bind, i, NULL);
            if (rc != 0)
            {
                WARN("SNMP match: get of var bind pattern fails %r", rc);
                break;
            } 

            rc = asn_get_field_data(pat_var_bind, &pat_oid,
                                    "name.#plain");
            if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
            {
                /* match OID other then plain patterns not supported yet */
                VERB("SNMP VB match, no name in varbind");
                rc = 0;
                continue;
            }
            pat_oid_len = asn_get_length(pat_var_bind, "name.#plain"); 

            for (vars = my_pdu->variables;
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
                rc = TE_ETADNOTMATCH;
                break;
            }

            rc = asn_get_subvalue(pat_var_bind, &pat_vb_value, "value.#plain");

            if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
            {
                VERB("There is no value in vb pattern, value matches.");
                rc = 0; /* value matches - no pattern for it */
                continue;
            }
            else if (TE_RC_GET_ERROR(rc) == TE_EASNOTHERCHOICE)
            {
                rc = TE_EOPNOTSUPP; /* value matches - math is not implemented */
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
                ERROR("Unexpected error getting pat vb value data: %r",
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
                        rc = TE_ETADNOTMATCH;
                        break;
                    }
                    VERB("SNMP VB match, got int val %d, pat %d", 
                         *(vars->val.integer), *((int *)pat_vb_val_data));
                    if (*((int *)pat_vb_val_data) != 
                        *(vars->val.integer))
                        rc = TE_ETADNOTMATCH;
                    break;

                case ASN_IPADDRESS: 
                case ASN_OCTET_STR: 
                case ASN_OBJECT_ID: 
                    if (vars->type == ASN_OBJECT_ID)
                    {
                        if (pat_value_syntax != OID)
                            rc = TE_ETADNOTMATCH;
                    }
                    else if (pat_value_syntax != OCT_STRING &&
                             pat_value_syntax != CHAR_STRING)
                        rc = TE_ETADNOTMATCH;

                    if (rc != 0)
                    {
                        VERB("SNMP VB match, got octet str syntax, rc %r",
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
                            rc = TE_ETADNOTMATCH;
                            break;
                        }

                        if (memcmp(pat_vb_val_data, vars->val.string, 
                                   vb_data_len) != 0)
                        {
#if 0
                            char buf[1000], *pt = buf;
                            int  i;

                            rc = TE_ETADNOTMATCH;
                            pt += sprintf(pt, "got data: ");
                            for (i = 0; i < vb_data_len; i++)
                                pt += sprintf(pt, "%02x ", vars->val.string[i]);
                            pt += sprintf(pt, " pat data: ");
                            for (i = 0; i < vb_data_len; i++)
                                pt += sprintf(pt, "%02x ", pat_vb_val_data[i]);
                            VERB("SNMP VB, fail: %s", buf);
#endif
                            rc = TE_ETADNOTMATCH;
                        }
                    }
                    VERB("SNMP VB match, values compare rc %r", rc);
                    break;

                default:;
            }
        }

        if (rc != 0)
            break;

    } while(0); /* match on varbinds finished */

    /* fill varbinds into parsed packet */
    for (vars = my_pdu->variables;
         rc == 0 && snmp_msg != NULL && vars != NULL;
         vars = vars->next_variable)
    {
        asn_value_p var_bind = NULL;
        char        os_choice[100]; 

        VERB("BEGIN of LOOP rc %r", rc);

        var_bind = asn_init_value(ndn_snmp_var_bind);
        asn_write_value_field(var_bind, vars->name, vars->name_length, 
                              "name.#plain");

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

        if (rc != 0)
            break;
    } /* end of loop fill of varbinds */

    if (rc == 0 && snmp_msg != NULL)
    {
        assert(vb_seq != NULL);
        rc = asn_write_component_value(snmp_msg, vb_seq,
                                       "variable-bindings");
    }

    asn_free_value(vb_seq);

    return rc;
}

/* See description in tad_snmp_impl.h */
te_errno
tad_snmp_gen_pattern_cb(csap_p csap, unsigned int layer,
                        const asn_value *tmpl_pdu, 
                        asn_value_p *ptrn_pdu)
{ 
    UNUSED(tmpl_pdu);

    *ptrn_pdu = asn_init_value(ndn_snmp_message);
    VERB("%s callback, CSAP # %d, layer %d",
         __FUNCTION__, csap->id, layer); 
    return 0;
}

#if 0
/**
 * Action function for replying to SNMPv2 InformRequest-PDU
 *
 * @param csap    CSAP Descriptor structure.
 * @param usr_param     User-supplied optional parameter.
 * @param pkt           Packet data (in the form of snmp_pdu structure).
 * @param pkt_len       Length of packet data.
 *
 * @return Status code.
 */
te_errno
tad_snmp_inform_response(csap_p csap, const char *usr_param,
                         const uint8_t *pkt, size_t pkt_len)
{
    struct snmp_pdu     *pdu = (struct snmp_pdu *)pkt;
    struct snmp_pdu     *reply;
    
    UNUSED(usr_param);

    if (pkt_len < sizeof(struct snmp_pdu))
    {
        WARN("%s: too small packet data supplied: %d bytes, %d required",
             __FUNCTION__, pkt_len, sizeof(struct snmp_pdu));
    }
    if (pdu->command != SNMP_MSG_INFORM)
    {
        WARN("%s: call for non-InformRequest SNMP PDU", __FUNCTION__);
        return 0;
    }

    reply = snmp_clone_pdu(pdu);
    if (reply == NULL)
    {
        ERROR("%s: cannot allocate memory for SNMP Response PDU",
              __FUNCTION__);
        return TE_ENOMEM;
    }
    reply->command = SNMP_MSG_RESPONSE;
    reply->errstat = 0;
    reply->errindex = 0;

    if (csap->write_cb(csap, (char *)reply, sizeof(*reply)) < 0)
    {
        ERROR("%s: failed sending SNMP Response PDU", __FUNCTION__);
        snmp_free_pdu(reply);
        return TE_RC(TE_TAD_CSAP, TE_ETADLOWER);
    }
    VERB("%s: SNMP Response on InformRequest is sent", __FUNCTION__);

    return 0;
}
#endif
