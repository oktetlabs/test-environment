/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * SNMP protocol implementaion internal declarations.
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
 * @author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#define LGR_USER "tapi_snmp"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "te_stdint.h"

#if HAVE_NET_SNMP_DEFINITIONS_H
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/definitions.h> 
#include <net-snmp/mib_api.h> 
#endif

#include "asn_impl.h"
#include "ndn_snmp.h"
#include "rcf_api.h"

#include "tapi_snmp.h"
#include "conf_api.h"
#include "logger_api.h"


/* save or not tmp ndn files */
#define DEBUG 0


const char*
print_oid (const tapi_snmp_oid_t *oid )
{
    static char buf[256];
    char *p = buf; 
    unsigned i;

    if (oid == NULL)
        strncpy(buf, "<null oid>", sizeof(buf));
    for(i =0; i < oid->length; i ++)
        p += sprintf (p, ".%d", (int)oid->id[i]);
    return buf;
}


int 
tapi_snmp_copy_varbind(tapi_snmp_varbind_t *dst, const tapi_snmp_varbind_t *src)
{
    int d_len;

    if (dst == NULL || src == NULL) 
        return TE_RC(TE_TAPI, ETEWRONGPTR); 

    memcpy (dst, src, sizeof(*dst));
    d_len = src->v_len;
    switch(src->type)
    {
        case TAPI_SNMP_OTHER:
        case TAPI_SNMP_INTEGER:
        case TAPI_SNMP_IPADDRESS:
        case TAPI_SNMP_COUNTER:
        case TAPI_SNMP_UNSIGNED:
        case TAPI_SNMP_TIMETICKS:
        case TAPI_SNMP_ENDOFMIB:
        case TAPI_SNMP_NOSUCHOBJ:
        case TAPI_SNMP_NOSUCHINS:
            /* do nothing - no memory allocated for storage. */
            break;

        case TAPI_SNMP_OBJECT_ID:
            d_len = sizeof (tapi_snmp_oid_t); /* fall through */
        case TAPI_SNMP_OCTET_STR:
            if ((dst->oct_string = malloc (d_len)) == NULL)
                return TE_RC(TE_TAPI, ENOMEM); 

            memcpy (dst->oct_string, src->oct_string, d_len);
            break;
    }

    return 0;
}

/* See description in tapi_snmp.h */
int 
tapi_snmp_is_sub_oid(const tapi_snmp_oid_t *tree, const tapi_snmp_oid_t *node)
{
    unsigned i;
    if (tree == NULL || node == NULL || tree->length > node->length) 
        return 0; 
    for (i = 0; i < tree->length && i < MAX_OID_LEN; i++)
        if (tree->id[i] != node->id[i]) return 0;
    return 1;
}

/* See description in tapi_snmp.h */
int 
tapi_snmp_cat_oid(tapi_snmp_oid_t *base, const tapi_snmp_oid_t *suffix)
{
    unsigned i, b_i;
    if (base == NULL || suffix == NULL)
        return TE_RC(TE_TAPI, ETEWRONGPTR); 
    
    for (i = 0, b_i = base->length; 
         i < suffix->length && b_i < MAX_OID_LEN; 
         i ++, b_i ++)
    {
        base->id[b_i] = suffix->id[i];
    }
    base->length += suffix->length;
    return 0;
}


/* See description in tapi_snmp.h */
void
tapi_snmp_free_varbind(tapi_snmp_varbind_t *varbind)
{
    if (varbind->type == TAPI_SNMP_OCTET_STR)
    {
        if(varbind->oct_string) 
            free(varbind->oct_string);
        varbind->oct_string = NULL;
    }
    else if (varbind->type == TAPI_SNMP_OBJECT_ID)
    {
        if(varbind->obj_id) 
            free(varbind->obj_id);
        varbind->obj_id = NULL;
    }
}

/* See description in tapi_snmp.h */
void
tapi_snmp_free_message(tapi_snmp_message_t *snmp_message)
{
    unsigned int i;

    if (snmp_message == NULL) 
        return; 

    if (snmp_message->vars)
    {
        for(i = 0; i < snmp_message->num_var_binds; i++)
        {
            switch(snmp_message->vars[i].type)
            {
                case TAPI_SNMP_INTEGER:
                case TAPI_SNMP_IPADDRESS:
                case TAPI_SNMP_COUNTER:
                case TAPI_SNMP_UNSIGNED:
                case TAPI_SNMP_TIMETICKS:
                case TAPI_SNMP_ENDOFMIB:
                    /* do nothing - no memory allocated for storage. */
                    break;

                case TAPI_SNMP_OCTET_STR:
                case TAPI_SNMP_OBJECT_ID:
                    if (snmp_message->vars[i].oct_string)
                        free (snmp_message->vars[i].oct_string);
                    break;
                default:
                    /* do nothing - no memory allocated for storage. */
                    break;

            }
        }
        free(snmp_message->vars);
    } 
}


/* See description in tapi_snmp.h */
int
tapi_snmp_packet_to_plain(asn_value *pkt, tapi_snmp_message_t *snmp_message)
{
    int rc;
    unsigned i, len;

    len = sizeof (snmp_message->type);
    rc = asn_read_value_field(pkt, &snmp_message->type, &len, "type");
    if (rc) 
        return TE_RC(TE_TAPI, rc);

    len = sizeof (snmp_message->err_status);
    rc = asn_read_value_field(pkt, &snmp_message->err_status, &len,
                                "err-status");
    if (rc) 
        return TE_RC(TE_TAPI, rc);


    len = sizeof (snmp_message->err_index);
    rc = asn_read_value_field(pkt, &snmp_message->err_index, &len, "err-index");
    if (rc) 
        return TE_RC(TE_TAPI, rc);

    VERB("in %s, errstat %d, errindex %d", __FUNCTION__, 
            snmp_message->err_status, snmp_message->err_index);

    if (snmp_message->type == NDN_SNMP_MSG_TRAP1)
    {

        len = snmp_message->enterprise.length = 
                                    asn_get_length(pkt, "enterprise");
        if (len > (int)sizeof(snmp_message->enterprise.id) / 
                       sizeof(snmp_message->enterprise.id[0]))
        {
            return TE_RC(TE_TAPI, ETESMALLBUF);
        }

        rc = asn_read_value_field(pkt, &(snmp_message->enterprise.id),
                                    &len, "enterprise"); 

        len = sizeof (snmp_message->gen_trap);
        rc = asn_read_value_field(pkt, &snmp_message->gen_trap, 
                                &len, "gen-trap");
        if (rc) 
            return TE_RC(TE_TAPI, rc);

        len = sizeof (snmp_message->spec_trap);
        rc = asn_read_value_field(pkt, &snmp_message->spec_trap, 
                                &len, "spec-trap");
        if (rc) 
            return TE_RC(TE_TAPI, rc);

        len = sizeof (snmp_message->agent_addr);
        rc = asn_read_value_field(pkt, &snmp_message->agent_addr, 
                                &len, "agent-addr");
        if (rc) 
            return TE_RC(TE_TAPI, rc);
    }

    snmp_message->num_var_binds = asn_get_length(pkt, "variable-bindings");
    snmp_message->vars = malloc(snmp_message->num_var_binds * 
                                sizeof(tapi_snmp_varbind_t));
    if(snmp_message->vars == NULL)
        return TE_RC(TE_TAPI, ENOMEM);

    for (i = 0; i < snmp_message->num_var_binds; i++)
    {
        asn_value *var_bind = asn_read_indexed(pkt, i, "variable-bindings");
#define CL_MAX 40
        char choice_label[CL_MAX];

        if (var_bind == NULL)
        {
            fprintf(stderr, "SNMP msg to C struct: var_bind = NULL\n");
            return TE_RC(TE_TAPI, EASNGENERAL); 
        }

        len = snmp_message->vars[i].name.length = 
                                    asn_get_length(var_bind, "name.#plain");
        if (len > (int)sizeof(snmp_message->vars[i].name.id) / 
                       sizeof(snmp_message->vars[i].name.id[0]))
        {
            return TE_RC(TE_TAPI, ETESMALLBUF);
        }

        rc = asn_read_value_field (var_bind, &(snmp_message->vars[i].name.id),
                                    &len, "name.#plain"); 
        VERB ("%s; var N %d ,oid %s", __FUNCTION__, i, print_oid(&(snmp_message->vars[i].name)));

        if (rc == 0)
            rc = asn_get_choice(var_bind, "value.#plain", choice_label, CL_MAX);

        if (rc == EASNINCOMPLVAL)
        { /* Some of SNMP errors occure */
            
            if ((rc = asn_read_value_field(var_bind, 0, 0, "endOfMibView")) == 0)
            {
                snmp_message->vars[i].type = TAPI_SNMP_ENDOFMIB;
            }    
            else if ((rc = asn_read_value_field(var_bind, 0, 0,
                                                "noSuchObject")) == 0)
                snmp_message->vars[i].type = TAPI_SNMP_NOSUCHOBJ;
            else if ((rc = asn_read_value_field(var_bind, 0, 0,
                                                "noSuchInstance")) == 0)
                snmp_message->vars[i].type = TAPI_SNMP_NOSUCHINS; 
            VERB ("read SNMP error fields return 0x%x\n", rc);    

            if (rc == 0)
            {
                snmp_message->vars[i].v_len   = 0;
                snmp_message->vars[i].integer = 0;
                continue;
            }
        }

        if (rc == 0 && strcmp(choice_label, "simple") == 0)
        {
            rc = asn_get_choice(var_bind, "value.#plain.#simple", 
                                        choice_label, CL_MAX);
            if (rc == 0)
            {
                if (strcmp(choice_label, "integer-value") == 0)
                {
                    int value;
                    len = sizeof(value);
                    rc = asn_read_value_field (var_bind, &value, &len, 
                                        "value.#plain.#simple.#integer-value");
                    snmp_message->vars[i].type = TAPI_SNMP_INTEGER;
                    snmp_message->vars[i].v_len = 0;
                    snmp_message->vars[i].integer = value;
                }
                else if (strcmp(choice_label, "string-value") == 0)
                {
                    len = asn_get_length(var_bind, 
                                        "value.#plain.#simple.#string-value");
                    snmp_message->vars[i].oct_string = malloc(len);
                    snmp_message->vars[i].type = TAPI_SNMP_OCTET_STR;
                    snmp_message->vars[i].v_len = len;
                    rc = asn_read_value_field (var_bind, 
                                   snmp_message->vars[i].oct_string, 
                                   &len, "value.#plain.#simple.#string-value");
                }
                else if (strcmp(choice_label, "objectID-value") == 0)
                {
                    len = asn_get_length(var_bind, 
                                        "value.#plain.#simple.#objectID-value");
                    snmp_message->vars[i].obj_id = 
                                                malloc(sizeof(tapi_snmp_oid_t));
                    snmp_message->vars[i].type = TAPI_SNMP_OBJECT_ID;
                    snmp_message->vars[i].v_len = len;
                    rc = asn_read_value_field (var_bind, 
                               snmp_message->vars[i].obj_id->id, 
                               &len, "value.#plain.#simple.#objectID-value");
                }
                else
                {
                    fprintf (stderr, "SNMP msg to C struct: "
                                "unexpected choice in simple: %s \n",
                                choice_label);
                    rc = EASNGENERAL;
                }
            }

        }
        else if (rc == 0 && strcmp(choice_label, "application-wide") == 0)
		{
            rc = asn_get_choice(var_bind, "value.#plain.#application-wide", 
                                        choice_label, CL_MAX);

            if (strcmp(choice_label, "ipAddress-value") == 0)
            {
                snmp_message->vars[i].type = TAPI_SNMP_IPADDRESS;
            }
            else if (strcmp(choice_label, "unsigned-value") == 0)
            {
                snmp_message->vars[i].type = TAPI_SNMP_UNSIGNED;
            } 
            else if (strcmp(choice_label, "counter-value") == 0)
            {
                snmp_message->vars[i].type = TAPI_SNMP_COUNTER;
            } 
            else if (strcmp(choice_label, "timeticks-value") == 0)
            {
                snmp_message->vars[i].type = TAPI_SNMP_TIMETICKS;
            }
            else 
            {
                fprintf (stderr, "SNMP msg to C struct: "
                            "unexpected choice in application-wide: %s \n",
                            choice_label);
                rc = EASNGENERAL;
            }

            len = sizeof(int);
            snmp_message->vars[i].v_len = len;
            if (rc == 0)
                rc = asn_read_value_field (var_bind, 
                                       &snmp_message->vars[i].integer, &len, 
                                       "value.#plain.#application-wide"); 
        }
        asn_free_value(var_bind);
    }
    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_snmp.h */
int 
tapi_snmp_csap_create(const char *ta, int sid, const char *snmp_agent, 
                        const char *community, int snmp_version, int *csap_id)
{
    return tapi_snmp_gen_csap_create(ta, sid, snmp_agent,
                                community, snmp_version, 0, 0,  -1, csap_id);
#if 0
    int rc;
    char tmp_name[100];
    FILE *f;

    strcpy(tmp_name, "/tmp/te_snmp_csap_create.XXXXXX"); 
    mktemp(tmp_name);
#if (DEBUG)
    VERB("tmp file: %s\n", tmp_name);
#endif
    f = fopen (tmp_name, "w+");
    if (f == NULL)
        return TE_RC(TE_TAPI, errno); /* return system errno */

    fprintf(f, "{ snmp:{ community plain:\"%s\", "
             " version plain:%d, "
             " snmp-agent plain:\"%s\"}}\n",
             community, 
             snmp_version - 1, /* convert "human"->ASN SNMP version */
             snmp_agent);
    fclose(f);


    rc = rcf_ta_csap_create(ta, sid, "snmp", tmp_name, csap_id); 

#if !(DEBUG)
    unlink(tmp_name);
#endif

    return rc;
#endif
}

/* See description in tapi_snmp.h */
int 
tapi_snmp_gen_csap_create(const char *ta, int sid, const char *snmp_agent, 
                          const char *community, int snmp_version, 
                          uint16_t rem_port, uint16_t loc_port,
                          int timeout, int *csap_id)
{
    int rc;
    char tmp_name[100];
    FILE *f;

    strcpy(tmp_name, "/tmp/te_snmp_csap_create.XXXXXX"); 
    mktemp(tmp_name);
#if DEBUG
    VERB("tmp file: %s\n", tmp_name);
#endif
    f = fopen (tmp_name, "w+");
    if (f == NULL)
        return TE_RC(TE_TAPI, errno); /* return system errno */

    fprintf(f, "{ snmp:{ version plain:%d ",
             snmp_version - 1 /* convert "human"->ASN SNMP version */
             );

    if (rem_port)
        fprintf(f, ", remote-port plain:%d ", rem_port);

    if (loc_port)
        fprintf(f, ", local-port plain:%d ", loc_port);

    if (community)
        fprintf(f, ", community plain:\"%s\" ", community);

    if (timeout >= 0)
        fprintf(f, ", timeout plain:%d ", timeout);

    if (snmp_agent)
        fprintf(f, ", snmp-agent plain:\"%s\" ", snmp_agent);

    fprintf(f,"}}\n");

    fclose(f);


    rc = rcf_ta_csap_create(ta, sid, "snmp", tmp_name, csap_id); 

#if !(DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}



void
tapi_snmp_pkt_handler(char *fn, void *p)
{ 
    int rc, s_parsed;
    asn_value_p packet, snmp_message;

#if DEBUG
    VERB("%s, file: %s\n", __FUNCTION__, fn);
#endif

    if (p == NULL)
    {
        fprintf (stderr, "NULL data pointer in tapi_snmp_pkt_handler!\n");
        return;
    }
    rc = asn_parse_dvalue_in_file(fn, ndn_raw_packet, &packet, &s_parsed);
    VERB("SNMP pkt handler, parse file rc: %x, syms: %d\n", rc, s_parsed);
#if DEBUG
    if (rc == 0)
    {
        struct timeval timestamp;
        rc = ndn_get_timestamp(packet, &timestamp);
        VERB("got timestamp, rc: %x, sec: %d, mcs: %d.\n", 
                rc, (int)timestamp.tv_sec, (int)timestamp.tv_usec); 
        asn_save_to_file(packet, "/tmp/got_file");
        rc = 0;
    }
#endif

    if (rc == 0)
    {
        tapi_snmp_message_t *plain_msg = (tapi_snmp_message_t *)p;

        VERB("parse SNMP file OK!\n");

        snmp_message = asn_read_indexed (packet, 0, "pdus");
        rc = tapi_snmp_packet_to_plain(snmp_message, plain_msg);

        VERB("packet to plain rc %d\n", rc);

        if (plain_msg->num_var_binds == 0) /* abnormal situation in SNMP message */
            plain_msg->err_status = rc;
    } 
}






static int 
tapi_snmp_msg_head(FILE *f, ndn_snmp_msg_t msg_type)
{
    if (f == NULL)
        return EINVAL;

    fprintf(f, "{pdus{snmp:{type ");
    switch (msg_type)
    {
        case NDN_SNMP_MSG_GET:      fprintf(f,"get, ");      break;
        case NDN_SNMP_MSG_GETNEXT:  fprintf(f,"get-next, "); break;
        case NDN_SNMP_MSG_GETBULK:  
            fprintf(f,"get-bulk, repeats plain:0, "); 
            break;

        case NDN_SNMP_MSG_SET:      fprintf(f,"set, ");      break;
        default: 
            return EINVAL;
    }
    fprintf(f,"variable-bindings {");

    return 0;
}

static int
tapi_snmp_msg_var_bind(FILE *f, const tapi_snmp_varbind_t *var_bind)
{
    unsigned int i;

    if (f == NULL)
        return EINVAL;

    fprintf(f,"{name plain:{");

    for (i = 0; i < var_bind->name.length; i ++)
    {
        fprintf(f, "%lu ", var_bind->name.id[i]);
    }
    fprintf (f, "}"); /* close for OID */

    if (var_bind->type != TAPI_SNMP_OTHER)
    {
        fprintf (f, ", value plain:");
        switch (var_bind->type)
        {
            case TAPI_SNMP_INTEGER:
                fprintf (f, "simple:integer-value:%d", var_bind->integer); 
                break;

            case TAPI_SNMP_OCTET_STR:
                fprintf (f, "simple:string-value:'"); 
                for(i = 0; i < var_bind->v_len; i++)
                    fprintf (f, "%02x ", var_bind->oct_string[i]); 
                fprintf (f, "'H"); 
                break;

            case TAPI_SNMP_OBJECT_ID:
                fprintf (f, "simple:objectID-value:{"); 
                for (i = 0; i < var_bind->v_len; i++)
                    fprintf(f, "%lu ", var_bind->obj_id->id[i]
                            );
                fprintf (f, "}"); 
                break;

            case TAPI_SNMP_IPADDRESS:
                fprintf (f, "application-wide:ipAddress-value:"); 
                for(i = 0; i < var_bind->v_len; i++)
                    fprintf (f, "%02x ", var_bind->oct_string[i]); 
                fprintf (f, "'H"); 
                break;

            case TAPI_SNMP_COUNTER:
                fprintf (f, "application-wide:counter-value:%d", 
                                                var_bind->integer); 
                break;
            case TAPI_SNMP_TIMETICKS:
                fprintf (f, "application-wide:timeticks-value:%d", 
                                                var_bind->integer); 
                break;
            case TAPI_SNMP_UNSIGNED:
                fprintf (f, "application-wide:unsigned-value:%u", 
                                                var_bind->integer); 
                break;
            default:
                return ETENOSUPP;
        } 
    }
    fprintf (f, "}"); /* close for var-bind */

    return 0;
}

static int
tapi_snmp_msg_tail(FILE *f)
{ 
    if (f == NULL) return EINVAL;
    fprintf (f, "}}}}\n"); 
    return 0;
}

/**
 * Internal common procedure for SNMP operations. 
 */
static int 
tapi_snmp_operation (const char *ta, int sid, int csap_id, 
                     const tapi_snmp_oid_t *val_oid, ndn_snmp_msg_t msg_type, 
                     tapi_snmp_vartypes_t var_type, 
                     size_t dlen, const void *data, 
                     tapi_snmp_message_t *msg)
{
    FILE         *f;
    unsigned int  timeout;
    char          tmp_name[100];
    int           rc, num;
    tapi_snmp_varbind_t var_bind;

    strcpy(tmp_name, "/tmp/te_snmp_op.XXXXXX"); 
    mktemp(tmp_name);
#if DEBUG
    VERB ("tmp file: %s\n", tmp_name);
#endif
    f = fopen (tmp_name, "w+");
    if (f == NULL)
        return TE_RC(TE_TAPI, errno); /* return system errno */


    var_bind.name = *val_oid;

    if (msg_type == NDN_SNMP_MSG_SET)
    {
        var_bind.type = var_type;
        var_bind.v_len = dlen;
        switch(var_type)
        {
            case TAPI_SNMP_OBJECT_ID:
            case TAPI_SNMP_OCTET_STR:
            case TAPI_SNMP_IPADDRESS:
                var_bind.oct_string = data;
                break;
            default:
                var_bind.integer = *((int*)data);
        }
    }
    else
        var_bind.type = TAPI_SNMP_OTHER;

    rc = tapi_snmp_msg_head(f, msg_type);

    if (rc == 0)
        rc = tapi_snmp_msg_var_bind(f, &var_bind);


    if (rc == 0)
        rc = tapi_snmp_msg_tail(f);

    fclose(f);

    if (rc == 0)
    {
        memset (msg, 0, sizeof (*msg)); 
        num = 1;
        timeout = 5000; /** @todo Fix me */

        rc = rcf_ta_trsend_recv(ta, sid, csap_id, tmp_name, 
                                tapi_snmp_pkt_handler, msg, timeout, &num); 

    }
#if !(DEBUG)
    unlink(tmp_name);
#endif 
    
    return TE_RC(TE_TAPI, rc);
}


/* See description in tapi_snmp.h */
int 
tapi_snmp_set(const char *ta, int sid, int csap_id, 
                         const tapi_snmp_varbind_t *var_binds, 
                         size_t num_vars, int *errstat, int *errindex)
{
    FILE         *f;
    unsigned int  timeout, i;
    char          tmp_name[100];
    int           rc, num;
    tapi_snmp_message_t msg;

    strcpy(tmp_name, "/tmp/te_snmp_op.XXXXXX"); 
    mktemp(tmp_name);
#if DEBUG
    VERB ("tmp file: %s\n", tmp_name);
#endif
    f = fopen (tmp_name, "w+");
    if (f == NULL)
        return TE_RC(TE_TAPI, errno); /* return system errno */ 

    rc = tapi_snmp_msg_head(f, NDN_SNMP_MSG_SET);

    for (i = 0; (i < num_vars) && (rc == 0); i ++)
    {
        if (i > 0)
            fprintf(f, ", ");
        rc = tapi_snmp_msg_var_bind(f, var_binds + i); 
    }

    if (rc == 0)
        rc = tapi_snmp_msg_tail(f);

    fclose(f);
    VERB("file %s written, rc %d", tmp_name, rc);

    if (rc == 0)
    {
        memset (&msg, 0, sizeof(msg)); 
        num = 1;
        timeout = 5000; /** @todo Fix me */

        rc = rcf_ta_trsend_recv(ta, sid, csap_id, tmp_name, 
                                tapi_snmp_pkt_handler, &msg, timeout, &num); 

    }
    if (rc == 0)
    {
        if (msg.num_var_binds) /* this is real response from Test Agent*/
        {
            if (errstat)
            {
                *errstat = msg.err_status;
                *errindex = msg.err_index;
              
                RING("in %s, errstat %d, errindex %d", __FUNCTION__, 
                                msg.err_status, msg.err_index);
            }
            tapi_snmp_free_message(&msg);
        }
        else 
            rc = msg.err_status;
    } 
#if !(DEBUG)
    unlink(tmp_name);
#endif 

    return TE_RC(TE_TAPI, rc);
}


struct tapi_vb_list {
    const tapi_snmp_varbind_t *vb;
    struct tapi_vb_list *next;
};


int 
tapi_snmp_set_row(const char *ta, int sid, int csap_id, 
                 int *errstat, int *errindex,
                 const tapi_snmp_oid_t *common_index, ...)
{
    va_list ap;
    int num_vars = 0;
    int i;
    struct tapi_vb_list *vbl_head = NULL; 
    const tapi_snmp_varbind_t *vb;
    tapi_snmp_varbind_t *vb_array;

    va_start(ap, common_index);
    do {
        vb = va_arg(ap, const tapi_snmp_varbind_t *);
        if (vb)
        {
            struct tapi_vb_list *new_vbl = calloc(1, sizeof(struct tapi_vb_list));
            new_vbl->next = vbl_head;
            new_vbl->vb = vb;
            num_vars ++;
        }
    } while (vb);

    if (!num_vars)
        return 0; /* ??? */

    vb_array = calloc(num_vars, sizeof(tapi_snmp_varbind_t)); 

    i = num_vars; 
    do {
        struct tapi_vb_list *tail = vbl_head->next;
        i--; 
        vb_array[i] = *(vbl_head->vb);
        free(vbl_head);
        vbl_head = tail;
        if (common_index)
            tapi_snmp_cat_oid(&(vb_array[i].name), common_index);
    } while (i > 0); 

    return tapi_snmp_set(ta, sid, csap_id, vb_array, num_vars, errstat, errindex);
}


/* See description in tapi_snmp.h */
int
tapi_snmp_set_integer(const char *ta, int sid, int csap_id, 
                      const tapi_snmp_oid_t *oid, int value, int *errstat)
{
    int local_value = value;
    tapi_snmp_message_t msg;
    int rc;

    rc = tapi_snmp_operation(ta, sid, csap_id, oid, 
                             NDN_SNMP_MSG_SET, TAPI_SNMP_INTEGER, 
                             sizeof (local_value), &local_value, &msg); 
    if (rc == 0)
    {
        if (msg.num_var_binds) /* this is real response from Test Agent*/
        {
            if (errstat)
                *errstat = msg.err_status;
            tapi_snmp_free_message(&msg);
        }
        else 
            rc = TE_RC(TE_TAPI, msg.err_status);
    } 
    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_set_octetstring(const char *ta, int sid, int csap_id,
                    const tapi_snmp_oid_t *oid,
                    const unsigned char *value, size_t size, int *errstat)
{
    tapi_snmp_message_t msg;
    int rc;

    rc = tapi_snmp_operation(ta, sid, csap_id, oid, 
                             NDN_SNMP_MSG_SET, TAPI_SNMP_OCTET_STR, 
                             size, value, &msg); 
    if (rc == 0)
    {
        if (msg.num_var_binds) /* this is real response from Test Agent*/
        {
            if (errstat)
                *errstat = msg.err_status;
            tapi_snmp_free_message(&msg);
        }
        else 
            rc = TE_RC(TE_TAPI, msg.err_status);
    } 
    return rc; 
}


/* See description in tapi_snmp.h */
int
tapi_snmp_set_string(const char *ta, int sid, int csap_id, 
                 const tapi_snmp_oid_t *oid, const char *value, int *errstat)
{
    size_t len = strlen(value);
    return tapi_snmp_set_octetstring(ta, sid, csap_id, oid, 
                        (const unsigned char *)value, len, errstat);
}


/* See description in tapi_snmp.h */
int
tapi_snmp_get(const char *ta, int sid, int csap_id, const tapi_snmp_oid_t *v_oid, 
              tapi_snmp_get_type_t next, tapi_snmp_varbind_t *varbind)
{
    tapi_snmp_message_t msg;
    int rc;

    rc = tapi_snmp_operation(ta, sid, csap_id, v_oid, 
             next == TAPI_SNMP_EXACT ? NDN_SNMP_MSG_GET : NDN_SNMP_MSG_GETNEXT,
             0, 0, NULL, &msg); 
    if (rc == 0)
    {
        if (msg.num_var_binds) /* this is real response from Test Agent*/
        {
            tapi_snmp_copy_varbind(varbind, msg.vars);
            tapi_snmp_free_message(&msg);
        }
        else 
            rc = TE_RC(TE_TAPI, msg.err_status);
    }

    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_getbulk(const char *ta, int sid, int csap_id, 
                  const tapi_snmp_oid_t *v_oid, 
                  int *num, tapi_snmp_varbind_t *varbind)
{
    tapi_snmp_message_t msg;
    int rc;

    rc = tapi_snmp_operation(ta, sid, csap_id, v_oid, 
              NDN_SNMP_MSG_GETBULK, 0, *num, NULL, &msg); 

    if (rc == 0)
    {
        if ((unsigned int)(*num) > msg.num_var_binds)
            *num = msg.num_var_binds;

        if (msg.num_var_binds) /* this is real response from Test Agent*/
        {
            int i;

            for (i = 0; i < *num; i++)
            {
                tapi_snmp_copy_varbind(&varbind[i], &msg.vars[i]);
                VERB ("GETBULK, variable: %s", print_oid(&(varbind[i].name)));
            }
            tapi_snmp_free_message(&msg);
        }
        else 
            rc = TE_RC(TE_TAPI, msg.err_status);
    } 
    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_walk(const char *ta, int sid, int csap_id, 
               const tapi_snmp_oid_t *oid, void *userdata,
               walk_callback callback)
{
    int rc;
    tapi_snmp_varbind_t vb;
    tapi_snmp_oid_t base_oid, next_oid;

    if (ta == NULL || oid == NULL)
        return TE_RC(TE_TAPI, ETEWRONGPTR); 

    next_oid = base_oid = *oid;
    VERB("%s for oid %s", __FUNCTION__, print_oid(oid));
    
    while (1) 
    {
        rc = tapi_snmp_get(ta, sid, csap_id, &next_oid, TAPI_SNMP_NEXT, &vb);
        if (vb.type == TAPI_SNMP_ENDOFMIB)
        {
	    VERB("walk is over");
            break; /* walk is finished */ 
        }    
        if (rc)
            return rc;

        next_oid = vb.name;

        VERB("walk go on, oid %s", print_oid(&next_oid));

        if (tapi_snmp_is_sub_oid(&base_oid, &vb.name))
        {

            rc = callback(&vb, userdata);

            tapi_snmp_free_varbind(&vb);
	    VERB("user callback in walk return %x", rc);

            if (rc)
                return rc;
        }
        else
            break; /* walk is finished */
    } 

    return 0;
}


struct tapi_snmp_column_list_t {
    struct tapi_snmp_column_list_t *next; /**< pointer to the next in list */ 
    tapi_snmp_varbind_t              vb;
};

static int 
tapi_snmp_column_list_callback(const tapi_snmp_varbind_t *varbind, 
                              void *userdata)
{
    struct tapi_snmp_column_list_t *ti_list, *l_p;
    ti_list = (struct tapi_snmp_column_list_t *)userdata; 

    /* go to the end of list */
    for (l_p = ti_list; l_p->next; l_p = l_p->next);

    l_p->next = (struct tapi_snmp_column_list_t *)
                        calloc(1, sizeof(struct tapi_snmp_column_list_t));

    tapi_snmp_copy_varbind(&(l_p->next->vb), varbind);
    VERB ("%s, got reply with OID: %s", 
            __FUNCTION__, print_oid(&(varbind->name)));

    return 0;
}

/**
 * Convert variable binding value to the plain presentation in table row.
 * Allocates memory for value, presentation depends on type:  
 *   - Octet strings (except IP address!) presented in special structure 
 *     tapi_snmp_oct_string_t; 
 *   - OID - in usual library type * tapi_snmp_oid_t; 
 *   - IP address - in array of 4 bytes, 
 *   - integer types - in natural for build architecture 'int'. 
 * Types are checked by variabel OID (taken from varbind) and preloaded MIB, 
 * if not MIB entry found for this OID, no check performed and value is 
 * converted according to type specified in varbind. 
 *
 * @param vb    variable binding to be converted.
 *
 * @return pointer to the allocated memory with converted data or NULL on error.
 */
static void *
tapi_snmp_vb_to_mem (const tapi_snmp_varbind_t *vb)
{
    struct tree *var_mib_object;
    
    if (vb == NULL) 
        return NULL;

    var_mib_object = get_tree(vb->name.id, vb->name.length, get_tree_head());

    switch (vb->type)
    {
        case TAPI_SNMP_OCTET_STR:
            if (var_mib_object && var_mib_object->type != TYPE_OCTETSTR)
                return NULL; 
            {
                tapi_snmp_oct_string_t *ret_val = 
                    calloc (1, sizeof (tapi_snmp_oct_string_t) + vb->v_len + 1);
                ret_val->len = vb->v_len;
                memcpy (ret_val->data, vb->oct_string, vb->v_len); 
                return (void *)ret_val;
            }
            break;
        case TAPI_SNMP_IPADDRESS:
            if (var_mib_object && var_mib_object->type != TYPE_IPADDR)
                return NULL; 
            {
                uint8_t *ret_val = calloc (1,  4);
                memcpy (ret_val, vb->oct_string, 4); 
                return (void *)ret_val;
            }
            break;
        case TAPI_SNMP_OBJECT_ID:
            if (var_mib_object && var_mib_object->type != TYPE_OBJID)
                return NULL; 
            {
                tapi_snmp_oid_t *ret_val = 
                    calloc (1,  sizeof (tapi_snmp_oid_t));
                memcpy (ret_val, vb->obj_id, sizeof (tapi_snmp_oid_t)); 
                return (void *)ret_val;
            }
            break;
        default: /* All other types, which are different kinds of integers. */
            {
                int *ret_val = malloc(sizeof(int));
                *ret_val = vb->integer;
                return (void *)ret_val;
            } 
    }
    return NULL;
}

/* See description in tapi_snmp.h */
int 
tapi_snmp_get_table(const char *ta, int sid, int csap_id, 
                    tapi_snmp_oid_t *table_oid, int *num, void **result)
{
    struct tapi_snmp_column_list_t ti_list;
    struct tapi_snmp_column_list_t *index_l_en;
    struct tree *entry_node; 
    int table_width = 0;
    int table_height = 0;
    int rc = 0; 
    tapi_snmp_oid_t entry; /* table Entry OID */

    if (ta == NULL || table_oid == NULL || num == NULL || result == NULL)
        return ETEWRONGPTR;

    memcpy(&entry, table_oid, sizeof(entry));

    VERB("GET TABLE called for oid %s", print_oid(&entry)); 

    entry_node = get_tree(entry.id, entry.length, get_tree_head());

    if (entry_node == NULL)
    {
        WARN("no entry node found!\n");
        return TE_RC(TE_TAPI, EINVAL);
    }

    VERB("find MIB node <%s> with last subid %d\n", 
                     entry_node->label, entry_node->subid);

    /* fall down in MIB tree to the table Entry node or leaf. */

    while (entry_node->indexes == NULL && entry_node->child_list != NULL)
    {
        entry_node = entry_node->child_list;
        if (entry.length == MAX_OID_LEN)
            return TE_RC(TE_TAPI, ENOBUFS);
        entry.id[entry.length] = entry_node->subid;
        entry.length++;
    }
    VERB("find Table entry node <%s> with last subid %d\n", 
                     entry_node->label, entry_node->subid);

    if (entry_node->indexes)
    {
        struct index_list *t_index;
        struct tree *index_node;
        struct tree *leaf;
        int column_found = 0;

        for (t_index = entry_node->indexes; t_index; 
             t_index = t_index->next)
        {
            index_node = find_node(t_index->ilabel, entry_node);
            if (index_node == NULL)
            {
                INFO("strange, node for index not found\n");
                break;
            }
            if (index_node->access == MIB_ACCESS_READONLY ||
                index_node->access == MIB_ACCESS_READWRITE )
                break;
            else
                index_node = NULL;
        }

        for (leaf = entry_node->child_list; leaf; leaf = leaf->next_peer)
        {
            if ((unsigned int)table_width < leaf->subid)
                table_width = leaf->subid;
        }

        if (index_node)
        {
            entry.id[entry.length] = index_node->subid;
            entry.length++;
        }
        else /* all index nodes are non-accessible, find any element */
        { 
            if (entry_node->child_list == NULL)
            {
                /* strange, node with indexes without children! */
                return TE_RC(TE_TAPI, 1);
            }

            for (entry_node = entry_node->child_list; entry_node; 
                 entry_node = entry_node->next_peer)
            {
                if (!column_found && 
                    ( entry_node->access == MIB_ACCESS_READONLY ||
                      entry_node->access == MIB_ACCESS_READWRITE )
                   )
                {
                    entry.id[entry.length] = entry_node->subid;
                    entry.length++;
                    VERB("find accessible entry <%s> with last subid %d\n", 
                             entry_node->label, entry_node->subid);
                    column_found = 1;
                }
            }
        } 
    } 
    else 
        table_width = 1;

    memset (&ti_list, 0, sizeof(ti_list));

    VERB("in gettable, now walk on %s", print_oid(&entry));

    /* Now 'entry' contains OID of table column. */
    rc = tapi_snmp_walk(ta, sid, csap_id, &entry,  &ti_list,
                        tapi_snmp_column_list_callback); 

    if (rc) 
        return rc;
    
    table_height = 0;
    for (index_l_en = ti_list.next; index_l_en; index_l_en = index_l_en->next)
        table_height ++; 

    *num = table_height;

    INFO("table width: %d, height: %d\n", table_width, table_height);
    if (table_height == 0) 
        return 0;
    
    *result = calloc (table_height, table_width * sizeof(void*));
    if (*result == NULL)
        return TE_RC(TE_TAPI, ENOMEM);
    
    if (table_width == 1)
    { /* no more SNMP operations need, only one column should be get */ 
        int i = 0;
        void **res_table = (void **) *result;

        for (index_l_en = ti_list.next, i = 0; 
             index_l_en; index_l_en = index_l_en->next, i++) 
        {
            res_table[i] = tapi_snmp_vb_to_mem(&index_l_en->vb);
        } 
    }
    else
    {
        int ti_start = entry.length; /**< table index start - 
                                          position of first index suboid */ 
        void **res_table = (void **) *result;

        int table_cardinality = table_width * table_height;
        int rest_varbinds = table_cardinality;
        int got_varbinds = 0;
        tapi_snmp_oid_t begin_of_portion;
        tapi_snmp_varbind_t *vb = calloc (table_cardinality, 
                                          sizeof (tapi_snmp_varbind_t));


        VERB("res_table: %p\n", res_table);
        entry.length --; 
        begin_of_portion = entry;
            /* to make 'entry' be OID of table Entry node */

        while (rest_varbinds)
        {
            int vb_num = rest_varbinds;

            rc = tapi_snmp_getbulk(ta, sid, csap_id, &begin_of_portion, 
                                    &vb_num, vb + got_varbinds);

            VERB ("table getbulk return %x, got %d vbs for oid %s\n  ", 
                    rc, vb_num, print_oid(&(begin_of_portion)));
            if (rc) break; 
            rest_varbinds -= vb_num;
            got_varbinds  += vb_num;

            if (vb[got_varbinds - 1].type == TAPI_SNMP_ENDOFMIB)
            {
                table_cardinality = got_varbinds - 1;
                /* last varbind is only endOfMibView. */
                break;
            }
            begin_of_portion = vb[got_varbinds - 1].name;
        }
        VERB("table cardinality, bulk got %d varbinds.\n", 
                    table_cardinality);

        /* ISSUE: fill result entry according to the indexes got 
           in the last Get-Bulk; currently processed only indexes, which 
           are found in previously got index-list. 
           So, result will not contain new table rows which are added on 
           SNMP Agent between walk over "index" column and table bulk. */

        if (rc == 0)
        {
            int i; 
            int ti_len = vb[0].name.length - ti_start;
                    /* table index length - number of index suboids.*/

            int row_num;

            VERB("table entry oid: %s, ti_len %d", print_oid(&entry), ti_len);

            for (i = 0; i < table_cardinality; i ++)
            {
                int table_offset;

                VERB("try to add varbind with oid %s", print_oid(&(vb[i].name))); 

                if (!tapi_snmp_is_sub_oid(&entry, &vb[i].name))
                {
                    continue;
                }

                for (index_l_en = ti_list.next, row_num = 0; index_l_en; 
                     index_l_en = index_l_en->next, row_num ++)
                {
                    if (memcmp (&(vb[i].name.id[ti_start]), 
                                &(index_l_en->vb.name.id[ti_start]),
                                ti_len * sizeof(oid)
                               ) == 0 )
                        break;
                }
                VERB("found index_l_en: %p\n", index_l_en); 
                if (index_l_en == NULL) 
                    continue; /* just skip this varbind, for which we cannot 
                                 find index... */ 
                table_offset = row_num * table_width + 
                                      (vb[i].name.id[ti_start - 1] - 1) ;
    
                res_table[table_offset] = tapi_snmp_vb_to_mem(&vb[i]);
                VERB("table offset:%d, ptr: %p\n", table_offset, res_table[table_offset]);
            }
        } 
    } 

    return rc; 
} 

/* See description in tapi_snmp.h */
int 
tapi_snmp_get_table_dimension(tapi_snmp_oid_t *table_oid, int *dimension)
{
    struct tree *entry_node; 
    int rc = 0; 
    tapi_snmp_oid_t entry; /* table Entry OID */
    struct index_list *t_index;

    if (table_oid == NULL)
        return ETEWRONGPTR;

    *dimension = 0;

    memcpy(&entry, table_oid, sizeof(entry));

    entry_node = get_tree(entry.id, entry.length, get_tree_head());

    if (entry_node == NULL)
    {
        WARN("no entry node found!\n");
        return TE_RC(TE_TAPI, EINVAL);
    }

    /* fall down in MIB tree to the table Entry node or leaf. */

    while (entry_node->indexes == NULL && entry_node->child_list != NULL)
    {
        entry_node = entry_node->child_list;
        if (entry.length == MAX_OID_LEN)
            return TE_RC(TE_TAPI, ENOBUFS);
        entry.id[entry.length] = entry_node->subid;
        entry.length++;
    }
    if (entry_node->indexes == NULL)
    {
        VERB("Very strange, no indices for table %s\n", print_oid(table_oid));
	return rc;
    }	    

    for (t_index = entry_node->indexes; t_index; t_index = t_index->next)
        (*dimension)++;

    return 0;
}

static tapi_snmp_vartypes_t net_snmp_convert[] = 
{
    /* TYPE_OTHER          0*/ TAPI_SNMP_OTHER,   
    /* TYPE_OBJID          1*/ TAPI_SNMP_OBJECT_ID,
    /* TYPE_OCTETSTR       2*/ TAPI_SNMP_OCTET_STR,
    /* TYPE_INTEGER        3*/ TAPI_SNMP_INTEGER,
    /* TYPE_NETADDR        4*/ TAPI_SNMP_OCTET_STR,
    /* TYPE_IPADDR         5*/ TAPI_SNMP_IPADDRESS,
    /* TYPE_COUNTER        6*/ TAPI_SNMP_COUNTER,
    /* TYPE_GAUGE          7*/ TAPI_SNMP_INTEGER,
    /* TYPE_TIMETICKS      8*/ TAPI_SNMP_TIMETICKS,
    /* TYPE_OPAQUE         9*/ TAPI_SNMP_OCTET_STR,
    /* TYPE_NULL           10*/ TAPI_SNMP_OTHER,
    /* TYPE_COUNTER64      11*/ TAPI_SNMP_INTEGER,
    /* TYPE_BITSTRING      12*/ TAPI_SNMP_OCTET_STR,
    /* TYPE_NSAPADDRESS    13*/ TAPI_SNMP_OCTET_STR,
    /* TYPE_UINTEGER       14*/ TAPI_SNMP_UNSIGNED,
    /* TYPE_UNSIGNED32     15*/ TAPI_SNMP_UNSIGNED,
    /* TYPE_INTEGER32      16*/ TAPI_SNMP_INTEGER,
    /* 17 */                    TAPI_SNMP_OTHER, 
    /* 18 */                    TAPI_SNMP_OTHER,
    /* 19 */                    TAPI_SNMP_OTHER,
    /* TYPE_TRAPTYPE       20*/ TAPI_SNMP_OTHER,
    /* TYPE_NOTIFTYPE      21*/ TAPI_SNMP_OTHER,
    /* TYPE_OBJGROUP       22*/ TAPI_SNMP_OTHER,
    /* TYPE_NOTIFGROUP     23*/ TAPI_SNMP_OTHER,
    /* TYPE_MODID          24*/ TAPI_SNMP_OTHER,
    /* TYPE_AGENTCAP       25*/ TAPI_SNMP_OTHER,
    /* TYPE_MODCOMP        26*/ TAPI_SNMP_OTHER
}; 


/* See description in tapi_snmp.h */
int
tapi_snmp_get_syntax(tapi_snmp_oid_t *oid, tapi_snmp_vartypes_t *type)
{
    struct tree *entry_node;

    if (oid == NULL)
        return ETEWRONGPTR;

    entry_node = get_tree(oid->id, oid->length, get_tree_head());

    if (entry_node == NULL)
    {
        WARN("no entry node found!\n");
        return TE_RC(TE_TAPI, EINVAL);
    }

    VERB("%s, label %s, syntax %d", __FUNCTION__, entry_node->label, entry_node->type);

    *type = net_snmp_convert[entry_node->type];
    return 0;
}


/* See description in tapi_snmp.h */
int 
tapi_snmp_get_table_columns(tapi_snmp_oid_t *table_oid, tapi_snmp_var_access **columns)
{
    struct tree *entry_node; 
    int rc = 0; 
    tapi_snmp_oid_t entry; /* table Entry OID */
    tapi_snmp_var_access *columns_p;

    if (table_oid == NULL)
        return ETEWRONGPTR;

    *columns = NULL;

    memcpy(&entry, table_oid, sizeof(entry));

    entry_node = get_tree(entry.id, entry.length, get_tree_head());

    if (entry_node == NULL)
    {
        WARN("no entry node found!\n");
        return TE_RC(TE_TAPI, EINVAL);
    }

    /* fall down in MIB tree to the table Entry node or leaf. */

    while (entry_node->indexes == NULL && entry_node->child_list != NULL)
    {
        entry_node = entry_node->child_list;
        if (entry.length == MAX_OID_LEN)
            return TE_RC(TE_TAPI, ENOBUFS);
        entry.id[entry.length] = entry_node->subid;
        entry.length++;
    }
    if (entry_node->indexes == NULL)
    {
        VERB("Very strange, cannot find entry for table %s\n", print_oid(table_oid));
	return rc;
    }	    

    if (entry_node->child_list == NULL)
    {
        /* strange, node with indexes without children! */
        return TE_RC(TE_TAPI, 1);
    }
    VERB("Table leaves:   \n");
    for (entry_node = entry_node->child_list; entry_node; entry_node = entry_node->next_peer)
    {
        columns_p = (tapi_snmp_var_access *)calloc(1, sizeof(tapi_snmp_var_access));
	if (columns_p == NULL)
	    return TE_RC(TE_TAPI, ENOMEM);

	strcpy(columns_p->label, entry_node->label);
	rc = tapi_snmp_make_oid(columns_p->label, &(columns_p->oid));
	if (rc)
	    return TE_RC(TE_TAPI, rc);	
	columns_p->access = entry_node->access;
	VERB("    %s, %s", columns_p->label, print_oid(&(columns_p->oid)));
	columns_p->next = *columns;
	*columns = columns_p;
    } 	
    return 0;
}	

/* See description in tapi_snmp.h */
int
tapi_snmp_get_ipaddr(const char *ta, int sid, int csap_id,
                     const tapi_snmp_oid_t *oid, void *addr)
{
    tapi_snmp_varbind_t varbind;
    int                 rc;

    rc = tapi_snmp_get(ta, sid, csap_id, oid, TAPI_SNMP_EXACT, &varbind);
    if (rc != 0)
        return rc;

    if (varbind.type != TAPI_SNMP_OCTET_STR /* TAPI_SNMP_IPADDRESS */)
    {
        tapi_snmp_free_varbind(&varbind);
        return EINVAL; /** @todo Cheange it to something like ETESNMPWRONGTYPE */
    }

    if (varbind.v_len != 4)
    {
        tapi_snmp_free_varbind(&varbind);
        return EINVAL;
    }

    memcpy(addr, varbind.oct_string, 4);

    return 0;
}

/**
 * Parse DateAndTime encoded in octet string in accordance with SNMPv2-TC
 *
 * @param ptr              Pointer to octet string with UTC time
 * @param len              Buffer length
 * @param time_val         Location for converted time
 * @param offset_from_utc  Location for signed offset from UTC octet
 *                         in minutes
 *
 * @return  Status of the operation
 *
 * @retval INVAL   Length or ecoding of the octet string is wrong
 * @retval 0       Success
 */
static int
ParseDateAndTime(void *ptr, int len, time_t *time_val, int *offset_from_utc)
{
    int       rc = EINVAL;
    uint8_t  *p_time = ptr;
    uint16_t  year;
    uint8_t   direction_from_utc;
    int       hours_from_utc;
    int       minutes_from_utc;
    struct tm tm_time;

    if (len != 8 && len != 11)
        return EINVAL;

    do {
        /* Get year */
        memcpy(&year, p_time, sizeof(year));
        tm_time.tm_year = ntohs(year);
        if (tm_time.tm_year < 1970)
            break;
        /* tm_year - the number of years since 1900 */
        tm_time.tm_year -= 1900;
        p_time += sizeof(year);
        /* Get month, day, hours, seconds and minutes */
        tm_time.tm_mon = *p_time++;
        if (tm_time.tm_mon < 1 || tm_time.tm_mon > 12)
            break;
        tm_time.tm_mday = *p_time++;
        if (tm_time.tm_mday < 1 || tm_time.tm_mday > 31)
            break;
        tm_time.tm_hour = *p_time++;
        if (tm_time.tm_hour < 0 || tm_time.tm_hour > 23)
            break;
        tm_time.tm_min = *p_time++;
        if (tm_time.tm_min < 0 || tm_time.tm_min > 59)
            break;
        tm_time.tm_sec = *p_time++;
        if (tm_time.tm_sec < 0 || tm_time.tm_sec > 59)
            break;
        /* Skip deci-seconds */
        if (*p_time++ > 9)
            break;
        /* Get offset information, if it's provided */
        if (len == 11)
        {
            direction_from_utc = *p_time++;
            if (direction_from_utc != '+' && direction_from_utc != '-')
                break;
            hours_from_utc = *p_time++;
            if (hours_from_utc < 0 || hours_from_utc > 11)
                break;
            minutes_from_utc = *p_time;
            if (minutes_from_utc < 0 || minutes_from_utc > 59)
                break;

            /* It seems it's all right with offset from UTC */
            *offset_from_utc = ((direction_from_utc == '+') ? 1 : -1) *
                (hours_from_utc * 60 + minutes_from_utc);
        }
        else
        {
            *offset_from_utc = 0;
        }
        *time_val = mktime(&tm_time);
        rc = 0;
    } while (0);

    return rc;
}

int
tapi_snmp_get_date_and_time(const char *ta, int sid, int csap_id,
                            const tapi_snmp_oid_t *oid, time_t *val,
                            int *offset_from_utc)
{
    int rc;
    uint8_t buf[11];
    size_t  buf_len = sizeof(buf);

    if ((rc = tapi_snmp_get_oct_string(ta, sid, csap_id, oid, 
                                       buf, &buf_len)) != 0)
        return rc;

    rc = ParseDateAndTime(buf, buf_len, val, offset_from_utc);
    return rc;
}

int
tapi_snmp_get_integer(const char *ta, int sid, int csap_id,
                      const tapi_snmp_oid_t *oid, int *val)
{
    tapi_snmp_varbind_t varbind;
    int                 rc;

    rc = tapi_snmp_get(ta, sid, csap_id, oid, TAPI_SNMP_EXACT, &varbind);
    if (rc)
        return rc;

    switch (varbind.type) 
    {
        case TAPI_SNMP_INTEGER:
            *val = varbind.integer;
            break;
        case TAPI_SNMP_NOSUCHOBJ:
        case TAPI_SNMP_NOSUCHINS:
        case TAPI_SNMP_ENDOFMIB:
            rc = varbind.type;
            break;
        default:
            VERB("got variable has different type: %d\n", (int)varbind.type);
            tapi_snmp_free_varbind(&varbind);
            rc = EINVAL; /** @todo Cheange it to something like ETESNMPWRONGTYPE */
    } 
    return TE_RC(TE_TAPI, rc);
}

int
tapi_snmp_get_string(const char *ta, int sid, int csap_id,
                     const tapi_snmp_oid_t *oid, char *buf, size_t buf_size)
{
    int rc;

    buf_size--;
    rc = tapi_snmp_get_oct_string(ta, sid, csap_id, oid, buf, &buf_size);
    if (rc == 0)
    {
        buf[buf_size] = '\0';
    }

    return rc;
}

int
tapi_snmp_get_oct_string(const char *ta, int sid, int csap_id,
                         const tapi_snmp_oid_t *oid,
                         void *buf, size_t *buf_size)
{
    tapi_snmp_varbind_t varbind;
    int                 rc;

    rc = tapi_snmp_get(ta, sid, csap_id, oid, TAPI_SNMP_EXACT, &varbind);
    if (rc != 0)
        return rc;

    if (varbind.type != TAPI_SNMP_OCTET_STR)
    {
        tapi_snmp_free_varbind(&varbind);
        return EINVAL; /** @todo Cheange it to something like ETESNMPWRONGTYPE */
    }

    if (varbind.v_len > *buf_size)
    {
        return ETESMALLBUF;
    }
    if (varbind.v_len > 0)
    {
        memcpy(buf, varbind.oct_string, varbind.v_len);
    }
    *buf_size = varbind.v_len;

    return 0;
}

int
tapi_snmp_get_objid(const char *ta, int sid, int csap_id,
                    const tapi_snmp_oid_t *oid, tapi_snmp_oid_t *ret_oid)
{
    tapi_snmp_varbind_t varbind;
    int                 rc;

    rc = tapi_snmp_get(ta, sid, csap_id, oid, TAPI_SNMP_EXACT, &varbind);
    if (rc != 0)
        return rc;

    if (varbind.type != TAPI_SNMP_OBJECT_ID)
    {
        tapi_snmp_free_varbind(&varbind);
        return EINVAL; /** @todo Cheange it to something like ETESNMPWRONGTYPE */
    }

    *ret_oid = *varbind.obj_id;

    return 0;
}


static int snmp_lib_initialized = 0;

/* See description in tapi_snmp.h */
int
tapi_snmp_load_mib_with_path(const char *dir_path, const char *mib_file)
{
    char *full_path;
    int   dir_path_len;
    int   mib_file_len;

    if (!snmp_lib_initialized)
    {
        init_snmp("");
        snmp_lib_initialized = 1;
    }
    dir_path_len = strlen(dir_path);
    mib_file_len = strlen(mib_file);
    if ((full_path = malloc(dir_path_len + mib_file_len + 2 + 3)) == NULL)
        return ENOMEM;

    memcpy(full_path, dir_path, dir_path_len);
    full_path[dir_path_len] = '/';
    memcpy(full_path + dir_path_len + 1, mib_file, mib_file_len + 1);
    strcat(full_path, ".my");

    if (read_mib(full_path) == NULL)
        return ENOENT;

    free(full_path);

    return 0;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_load_cfg_mibs(const char *dir_path)
{
    const char * const  mibs_ptrn = "/snmp:/mibs:/load:*";
    unsigned int        num;
    cfg_handle         *set = NULL;
    char               *mib_name;
    int                 rc = 0;
    unsigned int        i;

    if (!snmp_lib_initialized)
    {
        init_snmp("");
        snmp_lib_initialized = 1;
    }
    
    if ((rc = cfg_find_pattern(mibs_ptrn, &num, &set)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == ENOENT)
        {
            WARN("There is no MIB entries specified in configurator.conf");
            return 0;
        }
        else
        {
            ERROR("Failed to find by pattern '%s' in Configurator, %X",
                  mibs_ptrn, rc);
            return rc;
        }
    }

    if (num == 0)
    {
        WARN("There is no MIB entries specified in configurator.conf");
        goto cleanup;
    }

    for (i = 0; i < num; i++)
    {
        if ((rc = cfg_get_inst_name(set[i], &mib_name)) != 0)
        {
            ERROR("Failed to get instance name by handle 0x%x, %X",
                  set[i], rc);
            goto cleanup;
        }

        if ((rc = tapi_snmp_load_mib_with_path(dir_path, mib_name)) != 0)
            WARN("Loading %s MIB fails", mib_name);
        else
            INFO("%s MIB has been sucessfully loaded", mib_name);

        free(mib_name);
    }

cleanup:

    free(set);
    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_make_oid(const char *oid_str, tapi_snmp_oid_t *bin_oid)
{
    bin_oid->length = sizeof(bin_oid->id) / sizeof(bin_oid->id[0]);

    if (!snmp_lib_initialized)
    {
        init_snmp("");
        snmp_lib_initialized = 1;
    }

    /* Check if we have the same length of an element of OID */
    if (sizeof(bin_oid) == sizeof(bin_oid->id[0]))
    {
        if (snmp_parse_oid(oid_str, (oid *)bin_oid->id,
                           &(bin_oid->length)) == NULL)
        {
            return ENOENT;
        }
    }
    else if (sizeof(bin_oid->id[0]) > sizeof(oid))
    {
        oid name[bin_oid->length];
        unsigned i;
        
        if (snmp_parse_oid(oid_str, name, &bin_oid->length) == NULL)
        {
            return ENOENT;
        }
        for (i = 0; i < bin_oid->length; i++)
        {
            bin_oid->id[i] = name[i];
        }
    }
    else
    {
        /* Size of SUBID in NET-SNMP library is more than in tapi_snmp_oid_t */
        /* assert(0); */
    }

    return 0;
}

struct tapi_pkt_handler_data
{
    tapi_snmp_trap_callback user_callback;
    void                    *user_data; 
};


static void
tapi_snmp_trap_handler(char *fn, void *user_param)
{
    struct tapi_pkt_handler_data *i_data = 
        (struct tapi_pkt_handler_data *)user_param; 
    tapi_snmp_message_t plain_msg;

    int rc, s_parsed;
    asn_value_p packet, snmp_message;

    rc = asn_parse_dvalue_in_file(fn, ndn_raw_packet, &packet, &s_parsed);
    VERB("SNMP pkt handler, parse file rc: %x, syms: %d\n", rc, s_parsed);

#if DEBUG
    if (rc == 0)
    {
        struct timeval timestamp;
        rc = ndn_get_timestamp(packet, &timestamp);
        VERB("got timestamp, rc: %x, sec: %d, mcs: %d.\n", 
                rc, (int)timestamp.tv_sec, (int)timestamp.tv_usec); 
        asn_save_to_file(packet, "/tmp/got_file");
        rc = 0;
    }
#endif

    if (rc)
    {
        WARN("error in %s: %X\n", __FUNCTION__, rc);
        return;
    }

    VERB("parse SNMP file OK!\n");

    snmp_message = asn_read_indexed (packet, 0, "pdus");
    rc = tapi_snmp_packet_to_plain(snmp_message, &plain_msg);
    VERB("packet to plain rc %d\n", rc);
    asn_free_value(packet);
    asn_free_value(snmp_message);

    i_data->user_callback(&plain_msg, i_data->user_data);

    tapi_snmp_free_message(&plain_msg);

    return;
}


/* See description in tapi_snmp.h */
int 
tapi_snmp_trap_recv_start(const char *ta_name, int sid,
                         int snmp_csap, const asn_value *pattern,
                         tapi_snmp_trap_callback cb, void *cb_data,
                         unsigned int timeout, int num)
{
    int    rc;
    char   tmp_name[100];
    struct tapi_pkt_handler_data *i_data;

    if (ta_name == NULL || pattern == NULL)
        return TE_RC(TE_TAPI, EINVAL);

    if ((i_data = malloc(sizeof(*i_data))) == NULL)
        return TE_RC(TE_TAPI, ENOMEM);

    i_data->user_callback = cb;
    i_data->user_data = cb_data;

    strcpy(tmp_name, "/tmp/te_eth_trrecv.XXXXXX"); 
    mkstemp(tmp_name);
    rc = asn_save_to_file(pattern, tmp_name);
    if (rc)
    {
        free(i_data);
        return TE_RC(TE_TAPI, rc);
    }

    rc = rcf_ta_trrecv_start(ta_name, sid, snmp_csap, tmp_name,
                             (cb != NULL) ? tapi_snmp_trap_handler : NULL,
                             (cb != NULL) ? (void *) i_data : NULL,
                             timeout, num);
    if (rc != 0)
    {
        ERROR("%s() failed(0x%x) on TA %s:%d CSAP %d "
              "file %s", __FUNCTION__, rc, ta_name, sid, snmp_csap, tmp_name);
    }

#if !(TAPI_DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_make_table_field_instance(const char *oid_str, tapi_snmp_oid_t *bin_oid, ...)
{
    int                 rc = 0;
    int                 table_dimension;
    va_list             index_list;
    
    va_start(index_list, bin_oid);

    if ((rc = tapi_snmp_make_oid(oid_str, bin_oid)) != 0)
        return rc;

    /* To get table OID cut two last indexes */

    if (bin_oid->length <= 2)
        return TE_RC(TE_TAPI, EINVAL);

    bin_oid->length -= 2;

    if ((rc = tapi_snmp_get_table_dimension(bin_oid, &table_dimension)) != 0)
        return rc;

    /* Return back two last indexes of OID */

    bin_oid->length += 2;

    for (; table_dimension > 0; table_dimension--)
    {
        bin_oid->id[bin_oid->length++] = va_arg(index_list, int);
    }

    va_end(index_list);
    
    return rc;
}

