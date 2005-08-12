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

#define TE_LGR_USER     "TAPI SNMP"

#include "config.h"

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(x) \
    do {                                         \
        ERROR("ASSERT(" #x ") at %s:%s:%d",      \
              __FILE__, __FUNCTION__, __LINE__); \
    } while (0)
#endif

#include "te_stdint.h"

#if HAVE_NET_SNMP_DEFINITIONS_H
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/definitions.h>
#include <net-snmp/mib_api.h>
#include <net-snmp/varbind_api.h> /* For snmp_pdu_create() */
#endif

#include "te_defs.h" 
#include "asn_impl.h"
#include "ndn_snmp.h"
#include "rcf_api.h"

#include "tapi_snmp.h"
#include "conf_api.h"
#include "logger_api.h"

#include "tapi_bufs.h"

/* save or not tmp ndn files */
#undef DEBUG

#define TAPI_SNMP_LOG_FLUSH(buf_) \
    do {                                    \
        RING("%s", tapi_log_buf_get(buf_)); \
        tapi_log_buf_free(buf_);            \
    } while (0)

static int tapi_snmp_get_object_type(const tapi_snmp_oid_t *oid,
                                     enum snmp_obj_type *obj_type);

/**
 * Function is used for start logging SNMP operation
 *
 * @param log_buf   log buffer to append
 * @param msg_type  SNMP message type
 */
static void
tapi_snmp_log_op_start(tapi_log_buf *log_buf, ndn_snmp_msg_t msg_type)
{
    tapi_log_buf_append(log_buf, "SNMP %s: {\n",
                        ndn_snmp_msg_type_h2str(msg_type));
}

/**
 * Function is used for end logging SNMP operation
 *
 * @param log_buf     log buffer to append
 * @param rc          TE return code for this SNMP operation
 * @param err_status  SNMP error status of this operation
 * @param err_index   SNMP error index of the operation
 */
static void
tapi_snmp_log_op_end(tapi_log_buf *log_buf, int rc,
                     int err_status, int err_index)
{
    tapi_log_buf_append(log_buf,
            "} TAPI RESULT: %r, SNMP RESULT: %s, ERR INDEX: %d",
            rc, snmp_error_h2str(err_status), err_index);
}

/**
 * Function is used for logging SNMP VarBind name
 *
 * @param log_buf  log buffer to append
 * @param oid      OID od the VarBind
 */
static void
tapi_snmp_log_vb_name(tapi_log_buf *log_buf, const tapi_snmp_oid_t *oid)
{
    enum snmp_obj_type obj_type = SNMP_OBJ_UNKNOWN;

    tapi_snmp_get_object_type(oid, &obj_type);

    tapi_log_buf_append(log_buf, "\t%s (%s): ",
                        print_oid(oid),
                        snmp_obj_type_h2str(obj_type));
}


/* See description in tapi_snmp.h */
const char*
print_oid(const tapi_snmp_oid_t *oid)
{
#define TAPI_SNMP_OID_BUF_SIZE (1024 * 2)
#define TAPI_SNMP_OID_BUF_NUM (4)

    static char     buf[TAPI_SNMP_OID_BUF_NUM][TAPI_SNMP_OID_BUF_SIZE];
    static unsigned buf_cur = 0;

    buf_cur++;
    buf_cur %= TAPI_SNMP_OID_BUF_NUM;

    if (oid == NULL)
    {
        snprintf(buf[buf_cur],
                 TAPI_SNMP_OID_BUF_SIZE, "<null oid>");
    }
    else
    {
        if (snprint_objid(buf[buf_cur],
                          TAPI_SNMP_OID_BUF_SIZE,
                          oid->id, oid->length) < 0)
        {
            snprintf(buf[buf_cur],
                     TAPI_SNMP_OID_BUF_SIZE, "snprint_objid() failed");
        }
    }

#undef TAPI_SNMP_OID_BUF_SIZE
#undef TAPI_SNMP_OID_BUF_NUM

    return buf[buf_cur];
}

/* See description in tapi_snmp.h */
const char *
tapi_snmp_print_oct_str(const void *data, size_t len)
{
#define TAPI_SNMP_OCT_STR_BUF_SIZE (1024)
#define TAPI_SNMP_OCT_STR_BUF_NUM  (4)

    static char     *buf[TAPI_SNMP_OCT_STR_BUF_NUM] = {};
    static size_t    buf_len[TAPI_SNMP_OCT_STR_BUF_NUM] = {};
    static unsigned  buf_cur = 0;
    size_t           req_len;
    size_t           i;

    if (len == 0)
        return "<EMPTY STRING>";

    buf_cur++;
    buf_cur %= TAPI_SNMP_OCT_STR_BUF_NUM;

    /* (Re)allocate a buffer */
    req_len = 3 * len; /* two characters per byte and one for space & '\0'*/
    while (buf_len[buf_cur] < req_len)
    {
        char *ptr = buf[buf_cur];

        buf_len[buf_cur] += TAPI_SNMP_OCT_STR_BUF_SIZE;
        if ((buf[buf_cur] = realloc(ptr, buf_len[buf_cur])) == NULL)
        {
            ERROR("%s:%s:%d: Cannot allocate memory",
                  __FILE__, __FUNCTION__, __LINE__);

            buf_len[buf_cur] -= TAPI_SNMP_OCT_STR_BUF_SIZE;
            buf[buf_cur] = ptr;
            return "<TE_ENOMEM>";
        }
    }

    for (i = 0; i < len; i++)
    {
        snprintf(buf[buf_cur] + (3 * i), buf_len[buf_cur] - (3 * i),
                 "%02X ", ((uint8_t *)data)[i]);
    }
    assert(len >= 1);

    /*
     * Overwrite trailing space with '\0', knowing that, actually,
     * we still have trailing '\0' after that space.
     */
    *(buf[buf_cur] + (3 * i) - 1) = '\0';

#undef TAPI_SNMP_OCT_STR_BUF_SIZE
#undef TAPI_SNMP_OCT_STR_BUF_NUM

    return buf[buf_cur];
}

int
tapi_snmp_mib_entry_oid(struct tree *entry, tapi_snmp_oid_t *res_oid)
{
    int rc = 0;
    if (entry == NULL || res_oid == NULL)
        return TE_EWRONGPTR;

    if (entry->parent == NULL || entry->parent == entry)
        res_oid->length = 0;
    else
        rc = tapi_snmp_mib_entry_oid(entry->parent, res_oid);

    if (rc)
        return rc;

    res_oid->id[res_oid->length] = entry->subid;
    res_oid->length++;

    return 0;
}

int
tapi_snmp_copy_varbind(tapi_snmp_varbind_t *dst,
                       const tapi_snmp_varbind_t *src)
{
    int d_len;

    if (dst == NULL || src == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    memcpy(dst, src, sizeof(*dst));
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
            d_len = sizeof(tapi_snmp_oid_t); /* fall through */
        case TAPI_SNMP_OCTET_STR:
            if ((dst->oct_string = malloc(d_len)) == NULL)
                return TE_RC(TE_TAPI, TE_ENOMEM);

            memcpy(dst->oct_string, src->oct_string, d_len);
            break;
    }

    return 0;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_is_sub_oid(const tapi_snmp_oid_t *tree,
                     const tapi_snmp_oid_t *node)
{
    unsigned i;

    if (tree == NULL || node == NULL || tree->length > node->length)
        return 0;

    for (i = 0; i < tree->length && i < MAX_OID_LEN; i++)
        if (tree->id[i] != node->id[i])
            return 0;

    return 1;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_cat_oid(tapi_snmp_oid_t *base, const tapi_snmp_oid_t *suffix)
{
    unsigned int i;
    unsigned int b_i;

    if (base == NULL || suffix == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

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
        if (varbind->oct_string)
            free(varbind->oct_string);
        varbind->oct_string = NULL;
    }
    else if (varbind->type == TAPI_SNMP_OBJECT_ID)
    {
        if (varbind->obj_id)
            free(varbind->obj_id);
        varbind->obj_id = NULL;
    }
}

/* See description in tapi_snmp.h */
int
tapi_snmp_find_vb(const tapi_snmp_varbind_t *var_binds, size_t num,
                  const tapi_snmp_oid_t *oid,
                  const tapi_snmp_varbind_t **vb, size_t *pos)
{
    size_t i;
    
    if (var_binds == NULL || oid == NULL || vb == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    for (i = 0; i < num; i++)
    {
        if (tapi_snmp_cmp_oid(&(var_binds[i].name), oid) == 0)
        {
            *vb = var_binds + i;

            if (pos != NULL)
                *pos = i;

            return 0;
        }
    }

    return TE_RC(TE_TAPI, TE_ENOENT);
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
                        free(snmp_message->vars[i].oct_string);
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
    int          rc;
    size_t       len;
    unsigned int i;

    memset(snmp_message, 0, sizeof(*snmp_message));
    asn_save_to_file(pkt, "/tmp/te_snmp_pkt.asn");

    len = sizeof(snmp_message->type);
    rc = asn_read_value_field(pkt, &snmp_message->type, &len, "type");
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    len = sizeof(snmp_message->err_status);
    rc = asn_read_value_field(pkt, &snmp_message->err_status, &len,
                              "err-status");
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    len = sizeof(snmp_message->err_index);
    rc = asn_read_value_field(pkt, &snmp_message->err_index, &len,
                              "err-index");
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    VERB("%s(): errstat %d, errindex %d", __FUNCTION__,
         snmp_message->err_status, snmp_message->err_index);

    if (snmp_message->type == NDN_SNMP_MSG_TRAP1)
    {

        len = snmp_message->enterprise.length =
                                    asn_get_length(pkt, "enterprise");
        if (len > (int)sizeof(snmp_message->enterprise.id) /
                       sizeof(snmp_message->enterprise.id[0]))
        {
            return TE_RC(TE_TAPI, TE_ESMALLBUF);
        }

        rc = asn_read_value_field(pkt, &(snmp_message->enterprise.id),
                                  &len, "enterprise");

        len = sizeof(snmp_message->gen_trap);
        rc = asn_read_value_field(pkt, &snmp_message->gen_trap,
                                  &len, "gen-trap");
        if (rc != 0)
            return TE_RC(TE_TAPI, rc);

        len = sizeof(snmp_message->spec_trap);
        rc = asn_read_value_field(pkt, &snmp_message->spec_trap,
                                  &len, "spec-trap");
        if (rc != 0)
            return TE_RC(TE_TAPI, rc);

        len = sizeof(snmp_message->agent_addr);
        rc = asn_read_value_field(pkt, &snmp_message->agent_addr,
                                  &len, "agent-addr");
        if (rc != 0)
            return TE_RC(TE_TAPI, rc);
    }

    snmp_message->num_var_binds = asn_get_length(pkt, "variable-bindings");
    snmp_message->vars = calloc(snmp_message->num_var_binds,
                                sizeof(tapi_snmp_varbind_t));
    if (snmp_message->vars == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    for (i = 0; i < snmp_message->num_var_binds; i++)
    {
        asn_value *var_bind = asn_read_indexed(pkt, i, "variable-bindings");
#define CL_MAX 40
        char choice_label[CL_MAX];

        if (var_bind == NULL)
        {
            fprintf(stderr, "SNMP msg to C struct: var_bind = NULL\n");
            return TE_RC(TE_TAPI, TE_EASNGENERAL);
        }

        len = snmp_message->vars[i].name.length =
                                    asn_get_length(var_bind, "name.#plain");
        if (len > (int)sizeof(snmp_message->vars[i].name.id) /
                       sizeof(snmp_message->vars[i].name.id[0]))
        {
            return TE_RC(TE_TAPI, TE_ESMALLBUF);
        }

        rc = asn_read_value_field(var_bind,
                                  &(snmp_message->vars[i].name.id),
                                  &len, "name.#plain");
        if (rc != 0)
            return TE_RC(TE_TAPI, rc);

        VERB("%s(): var N %d, oid %s", __FUNCTION__, i,
             print_oid(&(snmp_message->vars[i].name)));

        rc = asn_get_choice(var_bind, "value.#plain", choice_label, CL_MAX);

        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            /* Some of SNMP errors occure */
            if ((rc = asn_read_value_field(var_bind, 0, 0,
                                           "endOfMibView")) == 0)
            {
                snmp_message->vars[i].type = TAPI_SNMP_ENDOFMIB;
            }
            else if ((rc = asn_read_value_field(var_bind, 0, 0,
                                                "noSuchObject")) == 0)
            {
                snmp_message->vars[i].type = TAPI_SNMP_NOSUCHOBJ;
            }
            else if ((rc = asn_read_value_field(var_bind, 0, 0,
                                                "noSuchInstance")) == 0)
            {
                snmp_message->vars[i].type = TAPI_SNMP_NOSUCHINS;
            }

            VERB("read SNMP error fields return %r\n", rc);

            if (rc == 0)
            {
                snmp_message->vars[i].v_len   = 0;
                snmp_message->vars[i].integer = 0;
                continue;
            }
        }

        if (rc != 0)
            return TE_RC(TE_TAPI, rc);

        if (strcmp(choice_label, "simple") == 0)
        {
            rc = asn_get_choice(var_bind, "value.#plain.#simple",
                                choice_label, CL_MAX);
            if (rc != 0)
                return TE_RC(TE_TAPI, rc);

            if (strcmp(choice_label, "integer-value") == 0)
            {
                int value;

                len = sizeof(value);
                rc = asn_read_value_field(var_bind, &value, &len,
                         "value.#plain.#simple.#integer-value");
                if (rc != 0)
                    return TE_RC(TE_TAPI, rc);

                snmp_message->vars[i].type = TAPI_SNMP_INTEGER;
                snmp_message->vars[i].v_len =
                    sizeof(snmp_message->vars[i].integer);
                snmp_message->vars[i].integer = value;
            }
            else if (strcmp(choice_label, "string-value") == 0)
            {
                len = asn_get_length(var_bind,
                          "value.#plain.#simple.#string-value");

                snmp_message->vars[i].oct_string = malloc(len);
                if (snmp_message->vars[i].oct_string == NULL)
                    return TE_RC(TE_TAPI, TE_ENOMEM);

                snmp_message->vars[i].type = TAPI_SNMP_OCTET_STR;
                snmp_message->vars[i].v_len = len;
                rc = asn_read_value_field(var_bind,
                         snmp_message->vars[i].oct_string,
                         &len, "value.#plain.#simple.#string-value");
            }
            else if (strcmp(choice_label, "objectID-value") == 0)
            {
                len = asn_get_length(var_bind,
                          "value.#plain.#simple.#objectID-value");

                snmp_message->vars[i].obj_id =
                        malloc(sizeof(tapi_snmp_oid_t));
                if (snmp_message->vars[i].obj_id == NULL)
                    return TE_RC(TE_TAPI, TE_ENOMEM);

                snmp_message->vars[i].type = TAPI_SNMP_OBJECT_ID;
                snmp_message->vars[i].v_len = len;
                snmp_message->vars[i].obj_id->length = len;
                rc = asn_read_value_field(var_bind,
                             snmp_message->vars[i].obj_id->id,
                             &len, "value.#plain.#simple.#objectID-value");
            }
            else
            {
                fprintf(stderr, "%s(): SNMP msg to C struct - "
                        "unexpected choice in simple: %s\n",
                        __FUNCTION__, choice_label);
                rc = TE_EASNGENERAL;

                assert(0);
            }
        }
        else if (strcmp(choice_label, "application-wide") == 0)
        {
            rc = asn_get_choice(var_bind, "value.#plain.#application-wide",
                                choice_label, CL_MAX);
            if (rc != 0)
                return TE_RC(TE_TAPI, rc);

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
                fprintf(stderr, "SNMP msg to C struct: "
                        "unexpected choice in application-wide: %s\n",
                        choice_label);
                rc = TE_EASNGENERAL;
                
                assert(0);

            }

            if (rc == 0)
            {
                len = sizeof(int);
                snmp_message->vars[i].v_len = len;

                rc = asn_read_value_field(var_bind,
                         &snmp_message->vars[i].integer, &len,
                         "value.#plain.#application-wide");
            }
        }

        if (rc != 0)
            return TE_RC(TE_TAPI, rc);

        asn_free_value(var_bind);
    }
    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_snmp.h */
int
tapi_snmp_csap_create(const char *ta, int sid, const char *snmp_agent,
                      const char *community, 
                      tapi_snmp_version_t snmp_version, int *csap_id)
{
    tapi_snmp_security_t security;

    memset(&security, 0, sizeof(security));
    security.model = TAPI_SNMP_SEC_MODEL_V2C;
    security.community = community;

    return tapi_snmp_gen_csap_create(ta, sid, snmp_agent, &security,
                                     snmp_version, 0, 0,  -1, csap_id);
}

/**
 * Convert SNMP protocol version in TAPI SNMP notation 
 * to protocol version NET-SNMP Library notation 
 *
 * @param version   SNMP protocol version in TAPI
 *
 * @return Protocol version in NET-SNMP, or -1 for unknown input value
 */
static int
tapi_snmp_version_to_netsnmp_version(tapi_snmp_version_t version)
{
    switch (version)
    {
        case TAPI_SNMP_VERSION_1:
            return SNMP_VERSION_1;

        case TAPI_SNMP_VERSION_2c:
            return SNMP_VERSION_2c;

        case TAPI_SNMP_VERSION_3:
            return SNMP_VERSION_3;

        default:
            ERROR("Unknown SNMP version %d is requested!", version);
    }
    return -1;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_gen_csap_create(const char *ta, int sid, const char *snmp_agent,
                          tapi_snmp_security_t *security,
                          tapi_snmp_version_t snmp_version,
                          uint16_t rem_port, uint16_t loc_port,
                          int timeout, int *csap_id)
{
    int   rc;
    char  tmp_name[] = "/tmp/te_snmp_csap_create.XXXXXX";
    FILE *f;

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

#if DEBUG
    VERB("tmp file: %s\n", tmp_name);
#endif
    f = fopen(tmp_name, "w+");
    if (f == NULL)
        return TE_OS_RC(TE_TAPI, errno); /* return system errno */

    fprintf(f, "{ snmp:{ version plain:%d ",
             tapi_snmp_version_to_netsnmp_version(snmp_version)
             );

    if (rem_port)
        fprintf(f, ", remote-port plain:%d ", rem_port);

    if (loc_port)
        fprintf(f, ", local-port plain:%d ", loc_port);

    if (security != NULL)
    {
        fprintf(f, ", security ");
        switch (security->model)
        {
            case TAPI_SNMP_SEC_MODEL_V2C:
                fprintf(f, "v2c:{community \"%s\"}", security->community);
                break;

            case TAPI_SNMP_SEC_MODEL_USM:
                fprintf(f, "usm:{name \"%s\"", security->name);
                switch (security->level)
                {
                    case TAPI_SNMP_SEC_LEVEL_AUTHPRIV:
                        fprintf(f, ", level authPriv");
                        break;

                    case TAPI_SNMP_SEC_LEVEL_AUTHNOPRIV:
                        fprintf(f, ", level authNoPriv");
                        break;

                    default:
                        fprintf(f, ", level noAuth");
                        break;
                }
                switch (security->auth_proto)
                {
                    case TAPI_SNMP_AUTH_PROTO_MD5:
                        fprintf(f, ", auth-protocol md5");
                        break;

                    case TAPI_SNMP_AUTH_PROTO_SHA:
                        fprintf(f, ", auth-protocol sha");
                        break;

                    default:
                        assert(0);
                        break;
                }
                if (security->auth_pass != NULL)
                    fprintf(f, ", auth-pass \"%s\"", security->auth_pass);

                switch (security->priv_proto)
                {
                    case TAPI_SNMP_PRIV_PROTO_DES:
                        fprintf(f, ", priv-protocol des");
                        break;

                    case TAPI_SNMP_PRIV_PROTO_AES:
                        fprintf(f, ", priv-protocol aes");
                        break;

                    default:
                        assert(0);
                        break;
                }
                if (security->priv_pass != NULL)
                    fprintf(f, ", priv-pass \"%s\"", security->priv_pass);

                fprintf(f, "}");
                break;

            default:
                ERROR("%s: unknown security model %d", security->model);
                assert(0);
                break;
        }
    }

    if (timeout >= 0)
        fprintf(f, ", timeout plain:%d ", timeout);

    if (snmp_agent)
        fprintf(f, ", snmp-agent plain:\"%s\" ", snmp_agent);

    fprintf(f,"}}\n");

    fclose(f);
    
    rc = rcf_ta_csap_create(ta, sid, "snmp", tmp_name, csap_id);

    INFO("Create SNMP CSAP %tf with status %r", tmp_name, rc);

#if !(DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}



void
tapi_snmp_pkt_handler(char *fn, void *p)
{
    int         rc;
    int         s_parsed;
    asn_value_p packet;
    asn_value_p snmp_message;

#if DEBUG
    VERB("%s, file: %s\n", __FUNCTION__, fn);
#endif

    if (p == NULL)
    {
        fprintf(stderr, "NULL data pointer in tapi_snmp_pkt_handler!\n");
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

        snmp_message = asn_read_indexed(packet, 0, "pdus");
        rc = tapi_snmp_packet_to_plain(snmp_message, plain_msg);

        VERB("packet to plain rc %x\n", rc);

        /* abnormal situation in SNMP message */
        if (plain_msg->num_var_binds == 0)
            plain_msg->err_status = rc;
    }
}






static int
tapi_snmp_msg_head(FILE *f, ndn_snmp_msg_t msg_type, int reps)
{
    if (f == NULL)
        return TE_EINVAL;

    fprintf(f, "{pdus{snmp:{type plain:");
    switch (msg_type)
    {
        case NDN_SNMP_MSG_GET:
            fprintf(f,"get, ");
            break;

        case NDN_SNMP_MSG_GETNEXT:
            fprintf(f,"get-next, ");
            break;

        case NDN_SNMP_MSG_GETBULK:
            fprintf(f,"get-bulk, repeats plain: %d, ", reps);
            break;

        case NDN_SNMP_MSG_SET:
            fprintf(f,"set, ");
            break;

        default:
            return TE_EINVAL;
    }
    fprintf(f,"variable-bindings {");

    return 0;
}

static int
tapi_snmp_msg_var_bind(FILE *f, const tapi_snmp_varbind_t *var_bind)
{
    unsigned int i;

    if (f == NULL)
        return TE_EINVAL;

    fprintf(f,"{name plain:{");

    if (var_bind->name.length > MAX_OID_LEN)
    {
        ERROR("Too long OID length: %d, max: %d",
              var_bind->name.length, MAX_OID_LEN);
        return TE_RC(TE_TAPI, TE_ENAMETOOLONG);
    }

    for (i = 0; i < var_bind->name.length; i ++)
    {
        fprintf(f, "%lu ", var_bind->name.id[i]);
    }
    fprintf(f, "}"); /* close for OID */

    if (var_bind->type != TAPI_SNMP_OTHER)
    {
        fprintf(f, ", value plain:");
        switch (var_bind->type)
        {
            case TAPI_SNMP_INTEGER:
                fprintf(f, "simple:integer-value:%d", var_bind->integer);
                break;

            case TAPI_SNMP_OCTET_STR:
                fprintf(f, "simple:string-value:'");
                for(i = 0; i < var_bind->v_len; i++)
                    fprintf(f, "%02x ", var_bind->oct_string[i]);
                fprintf(f, "'H");
                break;

            case TAPI_SNMP_OBJECT_ID:
                fprintf(f, "simple:objectID-value:{");
                for (i = 0; i < var_bind->v_len; i++)
                    fprintf(f, "%lu ", var_bind->obj_id->id[i]);
                fprintf(f, "}");
                break;

            case TAPI_SNMP_IPADDRESS:
                fprintf(f, "application-wide:ipAddress-value:'");
                for(i = 0; i < 4; i++)
                    fprintf(f, "%02x ",
                            ((uint8_t *)&(var_bind->integer))[i]);
                fprintf(f, "'H");
                break;

            case TAPI_SNMP_COUNTER:
                fprintf(f, "application-wide:counter-value:%d",
                        var_bind->integer);
                break;
            case TAPI_SNMP_TIMETICKS:
                fprintf(f, "application-wide:timeticks-value:%d",
                        var_bind->integer);
                break;
            case TAPI_SNMP_UNSIGNED:
                fprintf(f, "application-wide:unsigned-value:%u",
                        var_bind->integer);
                break;
            default:
                return TE_EOPNOTSUPP;
        }
    }
    fprintf(f, "}"); /* close for var-bind */

    return 0;
}

static int
tapi_snmp_msg_tail(FILE *f)
{
    if (f == NULL)
        return TE_EINVAL;

    fprintf(f, "}}}}\n");

    return 0;
}


/**
 * Internal common procedure for SNMP operations.
 */
static int
tapi_snmp_operation(const char *ta, int sid, int csap_id,
                    const tapi_snmp_oid_t *val_oid,
                    ndn_snmp_msg_t msg_type,
                    tapi_snmp_vartypes_t var_type,
                    size_t dlen, const void *data,
                    tapi_snmp_message_t *msg)
{
    FILE                *f;
    unsigned int         timeout;
    char                 tmp_name[] = "/tmp/te_snmp_op.XXXXXX";
    int                  rc, num;
    tapi_snmp_varbind_t  var_bind;

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

#if DEBUG
    VERB("tmp file: %s\n", tmp_name);
#endif
    f = fopen(tmp_name, "w+");
    if (f == NULL)
        return TE_OS_RC(TE_TAPI, errno); /* return system errno */

    var_bind.name = *val_oid;

    if (msg_type == NDN_SNMP_MSG_SET)
    {
        var_bind.type = var_type;
        var_bind.v_len = dlen;

        switch(var_type)
        {
            case TAPI_SNMP_OBJECT_ID:
            case TAPI_SNMP_OCTET_STR:
                var_bind.oct_string = (char *)data;
                break;

            default:
                var_bind.integer = *((int*)data);
                break;
        }
    }
    else
        var_bind.type = TAPI_SNMP_OTHER;

    rc = tapi_snmp_msg_head(f, msg_type, dlen);

    if (rc == 0)
        rc = tapi_snmp_msg_var_bind(f, &var_bind);


    if (rc == 0)
        rc = tapi_snmp_msg_tail(f);

    if (rc)
        WARN("%s: prepare NDS file error, rc %r", __FUNCTION__, rc);

    fclose(f);

    if (rc == 0)
    {
        memset(msg, 0, sizeof(*msg));
        num = 1;
        timeout = 5000; /** @todo Fix me */

        rc = rcf_ta_trsend_recv(ta, sid, csap_id, tmp_name,
                                tapi_snmp_pkt_handler, msg, timeout, &num);

        if (rc != 0)
            ERROR("rcf_ta_trsend_recv rc %r", rc);
    }
#if !(DEBUG)
    unlink(tmp_name);
#endif

    return TE_RC(TE_TAPI, rc);
}

typedef enum {
    ROW_PAR_INT,
    ROW_PAR_OS,
    ROW_PAR_OBJID
} row_par_type;

typedef struct tapi_get_row_par_list {
    row_par_type        type;
    int                *int_place;
    tapi_snmp_oid_t    *objid_place;
    unsigned char     **oct_str_place;
    size_t             *oct_str_len_place;

    struct tapi_get_row_par_list *next;
} tapi_get_row_par_list_t;


int
tapi_snmp_get_row(const char *ta, int sid, int csap_id,
                  int *errstatus, int *errindex,
                  const tapi_snmp_oid_t *common_index, ...)
{
    va_list       ap;
    int           num_vars = 0;
    int           i;
    FILE         *f;
    unsigned int  timeout;
    char          tmp_name[] = "/tmp/te_snmp_get_row.XXXXXX";
    int           rc, num;
    tapi_snmp_message_t msg;
    tapi_get_row_par_list_t *gp_head = NULL;
    tapi_get_row_par_list_t *get_par;

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

#if DEBUG
    VERB("tmp file: %s\n", tmp_name);
#endif
    f = fopen(tmp_name, "w+");
    if (f == NULL)
        return TE_OS_RC(TE_TAPI, errno); /* return system errno */


    rc = tapi_snmp_msg_head(f, NDN_SNMP_MSG_GET, 0);


    va_start(ap, common_index);
    while (1)
    {
        tapi_snmp_varbind_t  var_bind;
        tapi_snmp_vartypes_t syntax;
        char                 *oid_name = va_arg(ap, char *);

        if (!oid_name)
            break;

        if ((rc = tapi_snmp_make_oid(oid_name, &var_bind.name)) != 0)
            return TE_RC(TE_TAPI, rc);

        VERB("%s: var #%d, label %s, got oid %s\n", __FUNCTION__,
             num_vars, oid_name, print_oid(&var_bind.name));

        if ((rc = tapi_snmp_get_syntax(&var_bind.name, &syntax)) != 0)
            return TE_RC(TE_TAPI, rc);

        tapi_snmp_cat_oid(&var_bind.name, common_index);

        var_bind.type = TAPI_SNMP_OTHER; /* value is not need for GET */
        rc = tapi_snmp_msg_var_bind(f, &var_bind);
        if (rc) break;

        get_par = calloc(1, sizeof(*get_par));

        switch (syntax) {
            case TAPI_SNMP_OTHER:
            case TAPI_SNMP_INTEGER:
            case TAPI_SNMP_IPADDRESS:
            case TAPI_SNMP_COUNTER:
            case TAPI_SNMP_UNSIGNED:
            case TAPI_SNMP_TIMETICKS:
                get_par->type = ROW_PAR_INT;
                get_par->int_place = va_arg(ap, int*);
                break;

            case TAPI_SNMP_OCTET_STR:
                get_par->type = ROW_PAR_OS;
                get_par->oct_str_place = va_arg(ap, unsigned char **);
                get_par->oct_str_len_place = va_arg(ap, size_t*);
                break;

            case TAPI_SNMP_OBJECT_ID:
                get_par->type = ROW_PAR_INT;
                get_par->objid_place = va_arg(ap, tapi_snmp_oid_t *);
                break;

            default:
                ERROR("%s : unexpected syntax %d", __FUNCTION__, syntax);
                rc = TE_EINVAL;
                break;
        }

        get_par->next = gp_head;
        gp_head = get_par;
        num_vars++;
    };
    va_end(ap);

    if (rc == 0)
        rc = tapi_snmp_msg_tail(f);

    fclose(f);

    if (rc)
    {
        ERROR("%s : prepare in failed, rc %r", __FUNCTION__, rc);
        goto clean_up;
    }

    if (!num_vars) /* return 0??? */
        goto clean_up;

    VERB("in %s: num_vars %d\n", __FUNCTION__, num_vars);

    memset(&msg, 0, sizeof(msg));
    timeout = 5000; /** @todo Fix me. */
    rc = rcf_ta_trsend_recv(ta, sid, csap_id, tmp_name,
                            tapi_snmp_pkt_handler, &msg, timeout, &num);

    if (rc)
    {
        WARN("rcf_ta_trsend_recv rc %r", rc);
        goto clean_up;
    }

    if (msg.num_var_binds) /* this is real response from Test Agent*/
    {
        if ((unsigned int)(num_vars) != msg.num_var_binds)
        {
            ERROR("Wrong number of gor var_binds: %d", msg.num_var_binds);
            rc = TE_EFAULT;
            goto clean_up;
        }

        i = num_vars;
        get_par = gp_head;
        do
        {
            unsigned char *buf;
            size_t len;

            i--;
            len = msg.vars[i].v_len;
            switch(get_par->type)
            {
                case ROW_PAR_INT:
                    *(get_par->int_place) = msg.vars[i].integer;
                    break;
                case ROW_PAR_OS:
                    buf = *(get_par->oct_str_place) = calloc(1, len+1);
                    memcpy(buf, msg.vars[i].oct_string, len);
                    break;
                case ROW_PAR_OBJID:
                    *(get_par->objid_place) = *(msg.vars[i].obj_id);
            }
            VERB("GET_ROW, variable: %s", print_oid(&(msg.vars[i].name)));
        } while (i != 0);

        tapi_snmp_free_message(&msg);

    }
    else
    {
        if (errstatus)
            *errstatus = msg.err_status;
        if (errindex)
            *errindex  = msg.err_index;
    }

clean_up:

#if !(DEBUG)
    unlink(tmp_name);
#endif

    while (gp_head)
    {
        get_par = gp_head->next;
        free(gp_head);
        gp_head = get_par;
    }

    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_snmp.h */
int
tapi_snmp_set_vbs(const char *ta, int sid, int csap_id,
                  const tapi_snmp_varbind_t *var_binds,
                  size_t num_vars, int *errstat, int *errindex)
{
    FILE         *f;
    unsigned int  timeout, i;
    char          tmp_name[] = "/tmp/te_snmp_set.XXXXXX";
    int           rc, num;
    tapi_snmp_message_t msg;

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

#if DEBUG
    VERB("tmp file: %s\n", tmp_name);
#endif
    if ((f = fopen(tmp_name, "w+")) == NULL)
    {
        unlink(tmp_name);
        return TE_OS_RC(TE_TAPI, errno);
    }

    rc = tapi_snmp_msg_head(f, NDN_SNMP_MSG_SET, 0);

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
        memset(&msg, 0, sizeof(msg));
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

                INFO("in %s, errstat %d, errindex %d", __FUNCTION__,
                     msg.err_status, msg.err_index);
            }
            tapi_snmp_free_message(&msg);
        }
        else /* abnormal situation, msg is not correct SNMP response,
                err_status simply used for passing error code. */
            rc = msg.err_status;
    }
#if !(DEBUG)
    unlink(tmp_name);
#endif

    return TE_RC(TE_TAPI, rc);
}

struct tapi_vb_list {
    tapi_snmp_varbind_t *vb;
    struct tapi_vb_list *next;
};




static int
tapi_snmp_get_object_type(const tapi_snmp_oid_t *oid,
                          enum snmp_obj_type *obj_type)
{
    struct tree *entry_node;

    if (obj_type == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    entry_node = get_tree(oid->id, oid->length, get_tree_head());
    if (entry_node == NULL)
    {
        *obj_type = SNMP_OBJ_UNKNOWN;
        return 0;
    }
    if (entry_node->indexes != NULL)
    {
        *obj_type = SNMP_OBJ_TBL_ENTRY;
        return 0;
    }

    /* Try to check if it is a table field by going up to two nodes */
    if (entry_node->parent == NULL || entry_node->parent->parent == NULL)
    {
        *obj_type = SNMP_OBJ_SCALAR;
        return 0;
    }
    if (entry_node->parent->indexes != NULL)
    {
        *obj_type = SNMP_OBJ_TBL_FIELD;
        return 0;
    }
    if (entry_node->child_list != NULL &&
        entry_node->child_list->indexes != NULL)
    {
        *obj_type = SNMP_OBJ_TBL;
        return 0;
    }

    *obj_type = SNMP_OBJ_SCALAR;

    return 0;
}


static int
tapi_snmp_set_gen(const char *ta, int sid, int csap_id,
                  int *errstat, int *errindex,
                  const tapi_snmp_oid_t *common_index, va_list ap)
{
    int                   num_vars = 0;
    int                   i;
    int                   rc;
    struct tapi_vb_list  *vbl;
    struct tapi_vb_list  *vbl_head = NULL;
    tapi_snmp_varbind_t  *vb_array;
    tapi_snmp_varbind_t  *vb;
    tapi_snmp_oid_t       oid;
    tapi_snmp_vartypes_t  syntax;
    tapi_log_buf         *log_buf = tapi_log_buf_alloc();

    tapi_snmp_log_op_start(log_buf, NDN_SNMP_MSG_SET);

    *errstat = 0;
    *errindex = 0;

    while (1)
    {
        char *oid_name = va_arg(ap, char *);

        if (oid_name == NULL)
        {
            /* End Of Data mark */
            break;
        }
        tapi_log_buf_append(log_buf, "\t%s", oid_name);

        vb = calloc(1, sizeof(tapi_snmp_varbind_t));

        if ((rc = tapi_snmp_make_oid(oid_name, &oid)) != 0)
        {
            ERROR("Cannot parse %s OID", oid_name);
            tapi_log_buf_free(log_buf);
            return TE_RC(TE_TAPI, rc);
        }

        if ((rc = tapi_snmp_get_syntax(&oid, &syntax)) != 0)
        {
            ERROR("Cannot get syntax of %s OID", oid_name);
            tapi_log_buf_free(log_buf);
            return TE_RC(TE_TAPI, rc);
        }

        if (common_index == NULL)
        {
            enum snmp_obj_type obj_type;

            if ((rc = tapi_snmp_get_object_type(&oid, &obj_type)) != 0)
            {
                ERROR("Cannot get type of %s object", oid_name);
                tapi_log_buf_free(log_buf);
                return TE_RC(TE_TAPI, rc);
            }
            switch (obj_type)
            {
                case SNMP_OBJ_SCALAR:
                    /* Scalar object - just append zero */
                    if ((oid.length + 1) >= MAX_OID_LEN)
                    {
                        ERROR("Object %s has too long OID", oid_name);
                        tapi_log_buf_free(log_buf);
                        return TE_RC(TE_TAPI, TE_EFAULT);
                    }
                    tapi_snmp_append_oid(&oid, 1, 0);

                    tapi_log_buf_append(log_buf, ".0");
                    break;

                case SNMP_OBJ_TBL_FIELD:
                {
                    tapi_snmp_oid_t *tbl_index;

                    /* Table object - apend index */
                    tbl_index = va_arg(ap, tapi_snmp_oid_t *);
                    tapi_snmp_cat_oid(&oid, tbl_index);

                    tapi_log_buf_append(log_buf, "%s",
                                        print_oid(tbl_index));
                    break;
                }

                default:
                    ERROR("It is not allowed to pass objects other than "
                          "table fields and scalars");
                    tapi_log_buf_free(log_buf);
                    return TE_RC(TE_TAPI, TE_EFAULT);
            }
            tapi_log_buf_append(log_buf, " (%s) : ",
                                snmp_obj_type_h2str(obj_type));
        }
        else
        {
            tapi_log_buf_append(log_buf, "%s (%s) : ",
                                print_oid(common_index),
                                snmp_obj_type_h2str(SNMP_OBJ_TBL_FIELD));
        }

        vb->type = syntax;
        vb->name = oid;

        switch (syntax)
        {
            case TAPI_SNMP_OTHER:
            case TAPI_SNMP_INTEGER:
            case TAPI_SNMP_IPADDRESS:
            case TAPI_SNMP_COUNTER:
            case TAPI_SNMP_UNSIGNED:
            case TAPI_SNMP_TIMETICKS:
                vb->integer = va_arg(ap, int);
                tapi_log_buf_append(log_buf, "%d", vb->integer);
                break;

            case TAPI_SNMP_OCTET_STR:
                vb->oct_string = va_arg(ap, unsigned char *);
                vb->v_len = va_arg(ap, int);

                if (vb->v_len == 0)
                    tapi_log_buf_append(log_buf, "NULL");

                for (i = 0; i < (int)vb->v_len; i++)
                {
                    tapi_log_buf_append(log_buf, "%02X ",
                                        vb->oct_string[i]);
                }

                break;

            case TAPI_SNMP_OBJECT_ID:
                vb->obj_id = va_arg(ap, tapi_snmp_oid_t *);
                vb->v_len = vb->obj_id->length;
                
                tapi_log_buf_append(log_buf, "%s", print_oid(vb->obj_id));
                break;

            default:
                ERROR("%s unexpected syntax %d", __FUNCTION__, syntax);
                tapi_log_buf_free(log_buf);
                return TE_RC(TE_TAPI, TE_EFAULT);
        }
        tapi_log_buf_append(log_buf, "\n");

        vbl = calloc(1, sizeof(struct tapi_vb_list));
        vbl->next = vbl_head;
        vbl->vb = vb;
        vbl_head = vbl;
        num_vars++;
    };

    if (num_vars == 0)
    {
        WARN("No one varbind specified for the SET operation");
        /* @todo Probably we should issue SET op in this case too ? */
        tapi_log_buf_free(log_buf);
        return 0;
    }

    vb_array = calloc(num_vars, sizeof(tapi_snmp_varbind_t));

    VERB("in %s: num_vars %d\n", __FUNCTION__, num_vars);

    for (i = (num_vars - 1); i >= 0; i--)
    {
        vbl = vbl_head;
        vbl_head = vbl_head->next;

        vb_array[i] = *(vbl->vb);

        if (common_index)
            tapi_snmp_cat_oid(&(vb_array[i].name), common_index);

        free(vbl->vb);
        free(vbl);
    }

    rc = tapi_snmp_set_vbs(ta, sid, csap_id, vb_array, num_vars,
                           errstat, errindex);

    tapi_snmp_log_op_end(log_buf, rc, *errstat, *errindex);

    RING("%s", tapi_log_buf_get(log_buf));
    tapi_log_buf_free(log_buf);

    return rc;
}

int
tapi_snmp_set_row(const char *ta, int sid, int csap_id,
                  int *errstat, int *errindex,
                  const tapi_snmp_oid_t *common_index, ...)
{
    int     rc;
    va_list ap;

    va_start(ap, common_index);
    rc = tapi_snmp_set_gen(ta, sid, csap_id, errstat, errindex,
                           common_index, ap);
    va_end(ap);

    return rc;
}

int
tapi_snmp_set(const char *ta, int sid, int csap_id,
              int *errstat, int *errindex, ...)
{
    int     rc;
    va_list ap;

    va_start(ap, errindex);
    rc = tapi_snmp_set_gen(ta, sid, csap_id, errstat, errindex,
                           NULL, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_set_integer(const char *ta, int sid, int csap_id,
                      const tapi_snmp_oid_t *oid, int value, int *errstat)
{
    int                  local_value = value;
    tapi_snmp_message_t  msg;
    int                  rc;
    tapi_log_buf        *log_buf = tapi_log_buf_alloc();

    tapi_snmp_log_op_start(log_buf, NDN_SNMP_MSG_SET);
    tapi_snmp_log_vb_name(log_buf, oid);
    tapi_log_buf_append(log_buf, "%d\n", value);

    msg.err_status = SNMP_ERR_NOERROR;
    rc = tapi_snmp_operation(ta, sid, csap_id, oid,
                             NDN_SNMP_MSG_SET, TAPI_SNMP_INTEGER,
                             sizeof(local_value), &local_value, &msg);

    tapi_snmp_log_op_end(log_buf, rc, msg.err_status, 0);
    TAPI_SNMP_LOG_FLUSH(log_buf);

    if (rc == 0)
    {
        if (msg.num_var_binds != 0)
        {
            /* this is real response from Test Agent*/
            if (errstat)
                *errstat = msg.err_status;
            tapi_snmp_free_message(&msg);
        }
        else /* abnormal situation, msg is not correct SNMP response,
                err_status simply used for passing error code. */
            rc = TE_RC(TE_TAPI, msg.err_status);
    }

    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_set_unsigned(const char *ta, int sid, int csap_id,
                       const tapi_snmp_oid_t *oid,
                       unsigned int value, int *errstat)
{
    unsigned int         local_value = value;
    tapi_snmp_message_t  msg;
    int                  rc;
    tapi_log_buf        *log_buf = tapi_log_buf_alloc();

    tapi_snmp_log_op_start(log_buf, NDN_SNMP_MSG_SET);
    tapi_snmp_log_vb_name(log_buf, oid);
    tapi_log_buf_append(log_buf, "%u\n", value);

    msg.err_status = SNMP_ERR_NOERROR;
    rc = tapi_snmp_operation(ta, sid, csap_id, oid,
                             NDN_SNMP_MSG_SET, TAPI_SNMP_UNSIGNED,
                             sizeof(local_value), &local_value, &msg);

    tapi_snmp_log_op_end(log_buf, rc, msg.err_status, 0);
    TAPI_SNMP_LOG_FLUSH(log_buf);

    if (rc == 0)
    {
        if (msg.num_var_binds) 
        {
            /* this is real response from Test Agent*/
            if (errstat)
                *errstat = msg.err_status;
            tapi_snmp_free_message(&msg);
        }
        else
        {
            /* 
             * abnormal situation, msg is not correct SNMP response,
             * err_status simply used for passing error code. 
             */
            rc = TE_RC(TE_TAPI, msg.err_status);
        }
    }
 
    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_set_octetstring(const char *ta, int sid, int csap_id,
                          const tapi_snmp_oid_t *oid,
                          const uint8_t *value, size_t size, int *errstat)
{
    tapi_snmp_message_t  msg;
    int                  rc;
    unsigned int         i;
    tapi_log_buf        *log_buf = tapi_log_buf_alloc();

    tapi_snmp_log_op_start(log_buf, NDN_SNMP_MSG_SET);
    tapi_snmp_log_vb_name(log_buf, oid);

    for (i = 0; i < size; i++)
    {
        tapi_log_buf_append(log_buf, "%02x ", value[i]);
    }
    tapi_log_buf_append(log_buf, "%s\n", size != 0 ? "" : "NULL");

    msg.err_status = SNMP_ERR_NOERROR;
    rc = tapi_snmp_operation(ta, sid, csap_id, oid,
                             NDN_SNMP_MSG_SET, TAPI_SNMP_OCTET_STR,
                             size, value, &msg);

    tapi_snmp_log_op_end(log_buf, rc, msg.err_status, 0);
    TAPI_SNMP_LOG_FLUSH(log_buf);

    if (rc == 0)
    {
        if (msg.num_var_binds) /* this is real response from Test Agent*/
        {
            if (errstat)
                *errstat = msg.err_status;
            tapi_snmp_free_message(&msg);
        }
        else /* abnormal situation, msg is not correct SNMP response,
                err_status simply used for passing error code. */
            rc = TE_RC(TE_TAPI, msg.err_status);
    }
 
    return rc;
}


/* See description in tapi_snmp.h */
int
tapi_snmp_set_string(const char *ta, int sid, int csap_id,
                     const tapi_snmp_oid_t *oid, const char *value,
                     int *errstat)
{
    size_t len = strlen(value);
    return tapi_snmp_set_octetstring(ta, sid, csap_id, oid,
                        (const unsigned char *)value, len, errstat);
}


/* See description in tapi_snmp.h */
int
tapi_snmp_get(const char *ta, int sid, int csap_id,
              const tapi_snmp_oid_t *v_oid, tapi_snmp_get_type_t next,
              tapi_snmp_varbind_t *varbind, int *errstatus)
{
    tapi_snmp_message_t msg;
    int rc;

    rc = tapi_snmp_operation(ta, sid, csap_id, v_oid,
             next == TAPI_SNMP_EXACT ? NDN_SNMP_MSG_GET : 
                                       NDN_SNMP_MSG_GETNEXT,
             0, 0, NULL, &msg);
    if (rc == 0)
    {
        if (msg.num_var_binds) /* this is real response from Test Agent*/
        {
            tapi_snmp_copy_varbind(varbind, msg.vars);
            tapi_snmp_free_message(&msg);
        }

        if (errstatus)
            *errstatus = msg.err_status;
    }

    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_getbulk(const char *ta, int sid, int csap_id,
                  const tapi_snmp_oid_t *v_oid,
                  int *num, tapi_snmp_varbind_t *varbind, int *errstatus)
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
/*
                VERB("GETBULK, variable: %s",
                     print_oid(&(varbind[i].name)));
*/
            }
            tapi_snmp_free_message(&msg);
        }
        else if (errstatus)
            *errstatus = msg.err_status;
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
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    next_oid = base_oid = *oid;
    VERB("%s for oid %s", __FUNCTION__, print_oid(oid));

    while (1)
    {
        rc = tapi_snmp_get(ta, sid, csap_id, &next_oid, TAPI_SNMP_NEXT,
                           &vb, NULL);
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
    struct tapi_snmp_column_list_t *next; /**< Next in list */
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
    VERB("%s, got reply with OID: %s",
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
 * @param vb    Variable binding to be converted.
 *
 * @return Pointer to the allocated memory with converted data or
 *         NULL on error.
 */
static void *
tapi_snmp_vb_to_mem (const tapi_snmp_varbind_t *vb)
{
    struct tree *var_mib_object;

    if (vb == NULL)
        return NULL;

    var_mib_object = get_tree(vb->name.id, vb->name.length,
                              get_tree_head());

    switch (vb->type)
    {
        case TAPI_SNMP_OCTET_STR:
            if (var_mib_object && var_mib_object->type != TYPE_OCTETSTR)
                return NULL;
            {
                tapi_snmp_oct_string_t *ret_val =
                    calloc(1, sizeof(tapi_snmp_oct_string_t) + 
                              vb->v_len + 1);
                ret_val->len = vb->v_len;
                memcpy(ret_val->data, vb->oct_string, vb->v_len);
                return (void *)ret_val;
            }
            break;
        case TAPI_SNMP_IPADDRESS:
            if (var_mib_object && var_mib_object->type != TYPE_IPADDR)
                return NULL;
            {
                uint8_t *ret_val = calloc (1,  4);
                memcpy(ret_val, &vb->integer, 4);
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

        /* All other types, which are different kinds of integers */
        default:
        {
            int *ret_val = malloc(sizeof(int));

            *ret_val = vb->integer;
            return (void *)ret_val;
        }
    }
    return NULL;
}

/* Checks if SNMP mib entry access is readable */
static inline int
check_access_readable(int access)
{
    switch (access)
    {
        case MIB_ACCESS_READONLY:
        case MIB_ACCESS_READWRITE:
        case MIB_ACCESS_CREATE:
            return 1;
    }
    return 0;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_get_table(const char *ta, int sid, int csap_id,
                    tapi_snmp_oid_t *table_oid, int *num, void **result)
{
    struct tapi_snmp_column_list_t ti_list;
    struct tapi_snmp_column_list_t *index_l_en;
    struct tree *entry_node;
    int table_width = 0;  /* Really this is maximum sub-id of leaf 
                             in Entry */
    int num_columns = 0;  /* Quantity of leafs in Entry */
    int table_height = 0;
    int rc = 0;
    tapi_snmp_oid_t entry; /* table Entry OID */

    if (ta == NULL || table_oid == NULL || num == NULL || result == NULL)
        return TE_EWRONGPTR;

    memcpy(&entry, table_oid, sizeof(entry));

    VERB("GET TABLE called for oid %s", print_oid(&entry));

    entry_node = get_tree(entry.id, entry.length, get_tree_head());

    if (entry_node == NULL)
    {
        WARN("no entry node found!\n");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    VERB("find MIB node <%s> with last subid %d\n",
                     entry_node->label, entry_node->subid);

    /* fall down in MIB tree to the table Entry node or leaf. */

    while (entry_node->indexes == NULL && entry_node->child_list != NULL)
    {
        entry_node = entry_node->child_list;
        if (entry.length == MAX_OID_LEN)
            return TE_RC(TE_TAPI, TE_ENOBUFS);

        tapi_snmp_append_oid(&entry, 1, entry_node->subid);
    }
    VERB("find Table entry node <%s> with last subid %d\n",
                     entry_node->label, entry_node->subid);

    if (entry_node->indexes)
    {
        struct index_list *t_index;
        struct tree *index_node;
        struct tree *leaf;

        /* Try to find readable index column in table */
        for (t_index = entry_node->indexes, index_node = NULL; 
             t_index != NULL;
             t_index = t_index->next, index_node = NULL)
        {
            tapi_snmp_oid_t index_oid;

            index_node = find_node(t_index->ilabel, entry_node);
            if (index_node == NULL)
            {
                RING("strange, node for index point not found\n");
                break;
            }

            tapi_snmp_mib_entry_oid(index_node, &index_oid);

            if (!tapi_snmp_is_sub_oid(&entry, &index_oid))
            {
                INFO("Index entry <%s> is not in the table",
                     t_index->ilabel);
                continue;
            }

            if (check_access_readable(index_node->access))
            {
                INFO("Find readable column <%s> with access %d",
                        index_node->label, index_node->access);
                break;
            }
        }

        for (leaf = entry_node->child_list; leaf; leaf = leaf->next_peer)
        {
            if (check_access_readable(leaf->access))
                num_columns++;
            if ((unsigned int)table_width < leaf->subid)
                table_width = leaf->subid;
        }

        if (index_node)
        {
            tapi_snmp_append_oid(&entry, 1, entry_node->subid);
        }
        else /* all index columns are non-accessible, find any element */
        {
            tapi_snmp_varbind_t  vb;
            struct tree         *tbl_field;
            oid                  subid;

            INFO("Try to find any readable column");
            if (entry_node->child_list == NULL)
            {
                WARN("Node in MIB with indexes without children");
                return TE_RC(TE_TAPI, TE_ENOENT);
            }

            rc = tapi_snmp_get(ta, sid, csap_id, &entry, TAPI_SNMP_NEXT,
                               &vb, NULL);
            if (rc != 0)
            {
                ERROR("%s: get next to find first column fails %r", 
                      __FUNCTION__, rc);
                return TE_RC(TE_TAPI, rc);
            } 

            if (!tapi_snmp_is_sub_oid(&entry, &vb.name))
            {
                RING("%s: get-next obtain OID '%s' => table is EMPTY",
                     __FUNCTION__, print_oid(&entry));
                *num = 0;
                return 0;
            }
            INFO("%s: get-next on entry got %s", 
                 __FUNCTION__, print_oid(&vb.name));
            
            VERB("Check if we deal with read-create Table");
            
            {
                int tmp = vb.name.length;
                
                VERB("VB OID %s, SubID = %d", print_oid(&vb.name),
                     vb.name.id[entry.length]);
                vb.name.length = entry.length + 1;
                VERB("which is %s", print_oid(&vb.name));
                vb.name.length = tmp;
            }

            /* Check if it is read-create table */
            subid = vb.name.id[entry.length];
            for (tbl_field = entry_node->child_list;
                 tbl_field != NULL;
                 tbl_field = tbl_field->next_peer)
            {
                VERB("Check if %s object ours", tbl_field->label);
                if (tbl_field->subid == subid)
                {
                    VERB("Yes, with access = %d", tbl_field->access);

                    /* We found object in MIB tree for returned leaf */
                    if (tbl_field->access == TAPI_SNMP_MIB_ACCESS_CREATE)
                    {
                        /*
                         * In read-create tables some fields can be
                         * empty in some lines so that we need to
                         * go to the last field of the table row, where we
                         * will get RowStatus field, which is present in
                         * each row.
                         *
                         * In NET-SNMP tree data structure this entry 
                         * is the first in 'child_list' list, i.e.
                         * nodes are in reverse order.
                         */

                        /*
                         * tbl_field now point to RowStatus field of 
                         * the table, so let's define the number of
                         * rows in the table by using this field.
                         */
                        tapi_snmp_append_oid(&entry, 1,
                                             entry_node->child_list->subid);
                        break;
                    }
                }
            }

            if (tbl_field == NULL)
            {
                /*
                 * This is not read-create Table, so we can use any 
                 * field to define the number of rows in the table
                 */
                tapi_snmp_append_oid(&entry, 1, vb.name.id[entry.length]);
            }
        }
    }
    else
        table_width = 1;

    memset (&ti_list, 0, sizeof(ti_list));

    INFO("in gettable, now walk on %s", print_oid(&entry));

    /* Now 'entry' contains OID of table column. */
    rc = tapi_snmp_walk(ta, sid, csap_id, &entry,  &ti_list,
                        tapi_snmp_column_list_callback);

    if (rc != 0)
        return rc;

    table_height = 0;
    for (index_l_en = ti_list.next;
         index_l_en;
         index_l_en = index_l_en->next)
    {
        table_height++;
    }

    *num = table_height;

    INFO("table width: %d, height: %d; number of readable columns %d\n",
          table_width, table_height, num_columns);
    if (table_height == 0)
        return 0;

    *result = calloc(table_height, (table_width + 1) * sizeof(void *));
    if (*result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    if (table_width == 1)
    {
        /* No more SNMP operations need, only one column should be got */
        void **res_table = (void **)*result;
        int    i = 0;

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

        int table_cardinality = num_columns * table_height;
        int rest_varbinds = table_cardinality;
        int got_varbinds = 0;
        tapi_snmp_oid_t begin_of_portion;
        tapi_snmp_varbind_t *vb = calloc(table_cardinality,
                                         sizeof(tapi_snmp_varbind_t));


        VERB("res_table: %x", res_table);
        entry.length --;
        begin_of_portion = entry;
        /* to make 'entry' be OID of table Entry node */

        while (rest_varbinds)
        {
            int vb_num = rest_varbinds;

            rc = tapi_snmp_getbulk(ta, sid, csap_id, &begin_of_portion,
                                    &vb_num, vb + got_varbinds, NULL);

            VERB("Table getbulk return %r, asked for %d, got %d vbs "
                 "for oid %s", rc, rest_varbinds, vb_num,
                 print_oid(&(begin_of_portion)));

            if (rc) break;

            if (vb_num == 0)
            {
                rc = TE_EFAULT;
                WARN("GETBULK got zero variables!");
                break;
            }

            rest_varbinds -= vb_num;
            got_varbinds  += vb_num;

            if (vb[got_varbinds - 1].type == TAPI_SNMP_ENDOFMIB)
            {
                table_cardinality = got_varbinds - 1;
                /* last varbind is only endOfMibView. */
                break;
            }
            begin_of_portion = vb[got_varbinds - 1].name;

            VERB("prepare next bulk to oid %s",
                 print_oid(&begin_of_portion));
        }
        INFO("table cardinality, bulk got %d varbinds.",
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

            VERB("table entry oid: %s, ti_len %d",
                 print_oid(&entry), ti_len);

            for (i = 0; i < table_cardinality; i ++)
            {
                int table_offset;


                VERB("try to add varbind with oid %s",
                     print_oid(&(vb[i].name)));

                if (!tapi_snmp_is_sub_oid(&entry, &vb[i].name))
                {
                    continue;
                }

                for (index_l_en = ti_list.next, row_num = 0; index_l_en;
                     index_l_en = index_l_en->next, row_num ++)
                {
                    if (memcmp(&(vb[i].name.id[ti_start]),
                               &(index_l_en->vb.name.id[ti_start]),
                               ti_len * sizeof(oid)
                              ) == 0 )
                        break;
                }

                VERB("found index_l_en: %x\n", index_l_en);

                if (index_l_en == NULL)
                    continue; /* just skip this varbind, for which we cannot
                                 find index... */
                table_offset = row_num * (table_width + 1);

                if (res_table[table_offset] == NULL)
                {
                    /* index field not empty */
                    tapi_snmp_oid_t *index_suffix =
                        calloc(1, sizeof(tapi_snmp_oid_t));

                    memcpy(index_suffix->id, &(vb[i].name.id[ti_start]),
                                ti_len * sizeof(oid) );
                    index_suffix->length = ti_len;
                    res_table[table_offset] = index_suffix;

                    VERB("add index_suffix for row %d:  %s",
                         row_num, print_oid(index_suffix));
                }

                table_offset += (vb[i].name.id[ti_start - 1] ) ;

                res_table[table_offset] = tapi_snmp_vb_to_mem(&vb[i]);
                VERB("Table offset:%d, ptr: %x\n",
                     table_offset, res_table[table_offset]);
            }
        }
    }

    return rc;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_get_table_dimension(tapi_snmp_oid_t *table_oid, int *dimension)
{
    struct tree       *entry_node;
    tapi_snmp_oid_t    entry; /* table Entry OID */
    struct index_list *t_index;

    if (table_oid == NULL)
        return TE_EWRONGPTR;

    *dimension = 0;

    memcpy(&entry, table_oid, sizeof(entry));

    entry_node = get_tree(entry.id, entry.length, get_tree_head());
    if (entry_node == NULL)
    {
        WARN("no entry node found!\n");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (entry_node->indexes == NULL && entry_node->child_list == NULL)
    {
        /* it is an scalar Object */
        return 0;
    }

    /* fall down in MIB tree to the table Entry node or leaf. */
    while (entry_node->indexes == NULL && entry_node->child_list != NULL)
    {
        entry_node = entry_node->child_list;
        if (entry.length == MAX_OID_LEN)
            return TE_RC(TE_TAPI, TE_ENOBUFS);

        tapi_snmp_append_oid(&entry, 1, entry_node->subid);
    }
    if (entry_node->indexes == NULL)
    {
        VERB("Very strange, no indices for table %s\n",
             print_oid(table_oid));
        return TE_EFAULT;
    }

    for (t_index = entry_node->indexes; t_index; t_index = t_index->next)
        (*dimension)++;

    return 0;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_make_table_index(tapi_snmp_oid_t *tbl,
                           tapi_snmp_oid_t *index, ...)
{
    int dimension;
    int i = 0;
    int rc;

    va_list list;

    rc = tapi_snmp_get_table_dimension(tbl, &dimension);
    if (rc)
    {
        return rc;
    }

    va_start(list, index);
    index->length = dimension;
    while (i < dimension)
    {
        index->id[i++] = va_arg(list, oid);
    }
    va_end(list);
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
        return TE_EWRONGPTR;

    entry_node = get_tree(oid->id, oid->length, get_tree_head());

    if (entry_node == NULL)
    {
        WARN("no entry node found!\n");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    VERB("%s(): label %s, syntax %d", __FUNCTION__,
         entry_node->label, entry_node->type);

    *type = net_snmp_convert[entry_node->type];
    return 0;
}


/* See description in tapi_snmp.h */
int
tapi_snmp_get_table_columns(tapi_snmp_oid_t *table_oid,
                            tapi_snmp_var_access **columns)
{
    struct tree *entry_node;
    int rc = 0;
    tapi_snmp_oid_t entry; /* table Entry OID */
    tapi_snmp_var_access *columns_p;

    if (table_oid == NULL)
        return TE_EWRONGPTR;

    *columns = NULL;

    memcpy(&entry, table_oid, sizeof(entry));

    entry_node = get_tree(entry.id, entry.length, get_tree_head());

    if (entry_node == NULL)
    {
        WARN("no entry node found!\n");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* fall down in MIB tree to the table Entry node or leaf. */

    while (entry_node->indexes == NULL && entry_node->child_list != NULL)
    {
        entry_node = entry_node->child_list;
        if (entry.length == MAX_OID_LEN)
            return TE_RC(TE_TAPI, TE_ENOBUFS);
        tapi_snmp_append_oid(&entry, 1, entry_node->subid);
    }
    if (entry_node->indexes == NULL)
    {
        VERB("Very strange, cannot find entry for table %s\n",
             print_oid(table_oid));
       return rc;
    }

    if (entry_node->child_list == NULL)
    {
        /* strange, node with indexes without children! */
        return TE_RC(TE_TAPI, 1);
    }
    VERB("Table leaves:   \n");
    for (entry_node = entry_node->child_list;
         entry_node;
         entry_node = entry_node->next_peer)
    {
        columns_p = (tapi_snmp_var_access *)calloc(1,
                                              sizeof(tapi_snmp_var_access));
       if (columns_p == NULL)
           return TE_RC(TE_TAPI, TE_ENOMEM);

       strcpy(columns_p->label, entry_node->label);
       rc = tapi_snmp_make_oid(columns_p->label, &(columns_p->oid));
       if (rc)
           return TE_RC(TE_TAPI, rc);
       columns_p->access = entry_node->access;
       columns_p->status = entry_node->status;
       columns_p->subid = entry_node->subid;
       VERB("    %s, %s", columns_p->label, print_oid(&(columns_p->oid)));
       columns_p->next = *columns;
       *columns = columns_p;
    }
    return 0;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_get_ipaddr(const char *ta, int sid, int csap_id,
                     const tapi_snmp_oid_t *oid, void *addr, int *errstatus)
{
    tapi_snmp_varbind_t  varbind;
    int                  rc;
    tapi_log_buf        *log_buf;


    if (oid == NULL || addr == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    log_buf = tapi_log_buf_alloc();
    tapi_snmp_log_op_start(log_buf, NDN_SNMP_MSG_GET);
    tapi_snmp_log_vb_name(log_buf, oid);

    rc = tapi_snmp_get(ta, sid, csap_id, oid, TAPI_SNMP_EXACT,
                       &varbind, errstatus);
    if (rc != 0)
    {
        tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
        TAPI_SNMP_LOG_FLUSH(log_buf);

        return rc;
    }

    if (varbind.v_len != 4)
    {
        tapi_log_buf_append(log_buf, "-> LEN (%d) - EXPECTED 4\n",
                        varbind.v_len);
        ERROR("%s: expected IP address, but length is %d", 
              __FUNCTION__, varbind.v_len);
        rc = TE_EINVAL;
    }
    else
    { 
        switch (varbind.type)
        {
            case TAPI_SNMP_OCTET_STR:
                memcpy(addr, varbind.oct_string, 4);
                break;

            case TAPI_SNMP_IPADDRESS:
                memcpy(addr, &(varbind.integer), 4);
                break;

            default:
                tapi_log_buf_append(log_buf, "-> %s - EXPECTED %s or %s\n",
                        tapi_snmp_val_type_h2str(varbind.type),
                        tapi_snmp_val_type_h2str(TAPI_SNMP_OCTET_STR),
                        tapi_snmp_val_type_h2str(TAPI_SNMP_IPADDRESS));

                WARN("%s(): Got variable expected to be %s or %s, "
                     "but it is %s",
                     tapi_snmp_val_type_h2str(TAPI_SNMP_OCTET_STR),
                     tapi_snmp_val_type_h2str(TAPI_SNMP_IPADDRESS),
                     tapi_snmp_val_type_h2str(varbind.type));

                /** @todo Cheange it to something like ETESNMPWRONGTYPE */
                rc = TE_EINVAL;
        }
    }
    if (rc == 0)
    {
        tapi_log_buf_append(log_buf, "-> %d.%d.%d.%d\n",
                            ((uint8_t *)addr)[0],
                            ((uint8_t *)addr)[1],
                            ((uint8_t *)addr)[2],
                            ((uint8_t *)addr)[3]);
    }
    
    tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
    TAPI_SNMP_LOG_FLUSH(log_buf);

    tapi_snmp_free_varbind(&varbind);

    return TE_RC(TE_TAPI, rc);
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
    int       rc = TE_EINVAL;
    uint8_t  *p_time = ptr;
    uint16_t  year;
    uint8_t   direction_from_utc;
    int       hours_from_utc;
    int       minutes_from_utc;
    struct tm tm_time;

    if (len != 8 && len != 11)
        return TE_EINVAL;

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
                            int *offset_from_utc, int *errstatus)
{
    int rc;
    uint8_t buf[11];
    size_t  buf_len = sizeof(buf);

    if ((rc = tapi_snmp_get_oct_string(ta, sid, csap_id, oid,
                                       buf, &buf_len, errstatus)) != 0)
        return rc;

    rc = ParseDateAndTime(buf, buf_len, val, offset_from_utc);
    return rc;
}

int
tapi_snmp_get_integer(const char *ta, int sid, int csap_id,
                      const tapi_snmp_oid_t *oid, int *val, int *errstatus)
{
    tapi_snmp_varbind_t  varbind;
    int                  rc;
    tapi_log_buf        *log_buf = tapi_log_buf_alloc();

    tapi_snmp_log_op_start(log_buf, NDN_SNMP_MSG_GET);
    tapi_snmp_log_vb_name(log_buf, oid);

    rc = tapi_snmp_get(ta, sid, csap_id, oid,
                       TAPI_SNMP_EXACT, &varbind, errstatus);
    if (rc != 0)
    {
        tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
        TAPI_SNMP_LOG_FLUSH(log_buf);
        return rc;
    }

    switch (varbind.type)
    {
        case TAPI_SNMP_INTEGER:
        case TAPI_SNMP_COUNTER:
        case TAPI_SNMP_UNSIGNED:
        case TAPI_SNMP_TIMETICKS:
            *val = varbind.integer;
            tapi_log_buf_append(log_buf, "-> %d\n", *val);
            break;

        case TAPI_SNMP_NOSUCHOBJ:
        case TAPI_SNMP_NOSUCHINS:
        case TAPI_SNMP_ENDOFMIB:
            rc = varbind.type;
            tapi_log_buf_append(log_buf, "-> %s\n",
                                tapi_snmp_val_type_h2str(varbind.type));
            break;

        default:
            tapi_log_buf_append(log_buf, "-> %s - EXPECTED %s\n",
                    tapi_snmp_val_type_h2str(varbind.type),
                    tapi_snmp_val_type_h2str(TAPI_SNMP_INTEGER));

            WARN("%s(): Got variable expected to be INTEGER, "
                 "but it is %s", tapi_snmp_val_type_h2str(varbind.type));

            tapi_snmp_free_varbind(&varbind);
            /** @todo Cheange it to something like ETESNMPWRONGTYPE */
            rc = TE_EINVAL;
            break;
    }

    tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
    TAPI_SNMP_LOG_FLUSH(log_buf);

    return TE_RC(TE_TAPI, rc);
}

int
tapi_snmp_get_string(const char *ta, int sid, int csap_id,
                     const tapi_snmp_oid_t *oid,
                     char *buf, size_t buf_size, int *errstatus)
{
    int rc;

    if (buf_size < 1)
        return TE_RC(TE_TAPI, TE_ESMALLBUF);

    buf_size--;
    rc = tapi_snmp_get_oct_string(ta, sid, csap_id, oid,
                                  buf, &buf_size, errstatus);
    if (rc == 0)
    {
        buf[buf_size] = '\0';
    }

    return rc;
}

int
tapi_snmp_get_oct_string(const char *ta, int sid, int csap_id,
                         const tapi_snmp_oid_t *oid,
                         void *buf, size_t *buf_size,
                         int *errstatus)
{
    tapi_snmp_varbind_t  varbind;
    int                  rc;
    unsigned int         i;
    tapi_log_buf        *log_buf = tapi_log_buf_alloc();

    tapi_snmp_log_op_start(log_buf, NDN_SNMP_MSG_GET);
    tapi_snmp_log_vb_name(log_buf, oid);

    rc = tapi_snmp_get(ta, sid, csap_id, oid, TAPI_SNMP_EXACT, &varbind,
                       errstatus);
    if (rc != 0)
    {
        tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
        TAPI_SNMP_LOG_FLUSH(log_buf);
        return rc;
    }

    if (varbind.type != TAPI_SNMP_OCTET_STR)
    {
        tapi_log_buf_append(log_buf, "-> %s - EXPECTED %s\n",
                tapi_snmp_val_type_h2str(varbind.type),
                tapi_snmp_val_type_h2str(TAPI_SNMP_OCTET_STR));
        TAPI_SNMP_LOG_FLUSH(log_buf);

        WARN("%s(): Got variable expected to be OCTET STRING, "
                 "but it is %s", tapi_snmp_val_type_h2str(varbind.type));

        tapi_snmp_free_varbind(&varbind);
        /** @todo Cheange it to something like ETESNMPWRONGTYPE */
        return TE_EINVAL;
    }
    
    for (i = 0; i < varbind.v_len; i++)
    {
        tapi_log_buf_append(log_buf, "%02x ", varbind.oct_string[i]);
    }
    tapi_log_buf_append(log_buf, "%s\n", varbind.v_len != 0 ? "" : "NULL");
    tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
    TAPI_SNMP_LOG_FLUSH(log_buf);

    if (varbind.v_len > *buf_size)
    {
        return TE_ESMALLBUF;
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
                    const tapi_snmp_oid_t *oid, tapi_snmp_oid_t *ret_oid,
                    int *errstatus)
{
    tapi_snmp_varbind_t  varbind;
    int                  rc;
    tapi_log_buf        *log_buf = tapi_log_buf_alloc();

    tapi_snmp_log_op_start(log_buf, NDN_SNMP_MSG_GET);
    tapi_snmp_log_vb_name(log_buf, oid);

    rc = tapi_snmp_get(ta, sid, csap_id, oid,
                       TAPI_SNMP_EXACT, &varbind, errstatus);
    if (rc != 0)
    {
        tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
        TAPI_SNMP_LOG_FLUSH(log_buf);
        return rc;
    }

    if (varbind.type != TAPI_SNMP_OBJECT_ID)
    {
        tapi_log_buf_append(log_buf, "-> %s - EXPECTED %s\n",
                tapi_snmp_val_type_h2str(varbind.type),
                tapi_snmp_val_type_h2str(TAPI_SNMP_OBJECT_ID));
        tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
        TAPI_SNMP_LOG_FLUSH(log_buf);

        tapi_snmp_free_varbind(&varbind);
        /** @todo Change it to something like ETESNMPWRONGTYPE */
        return TE_EINVAL;
    }
    tapi_log_buf_append(log_buf, "%s\n", print_oid(varbind.obj_id));
    tapi_snmp_log_op_end(log_buf, rc, *errstatus, 0);
    TAPI_SNMP_LOG_FLUSH(log_buf);

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
        return TE_ENOMEM;

    memcpy(full_path, dir_path, dir_path_len);
    full_path[dir_path_len] = '/';
    memcpy(full_path + dir_path_len + 1, mib_file, mib_file_len + 1);

    if (strchr(mib_file, '.') == NULL)
        strcat(full_path, ".my");

    if (read_mib(full_path) == NULL)
        return TE_ENOENT;

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
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            WARN("There is no MIB entries specified in configurator.conf");
            return 0;
        }
        else
        {
            ERROR("Failed to find by pattern '%s' in Configurator, %r",
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
            ERROR("Failed to get instance name by handle 0x%x, %r",
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
void
tapi_snmp_append_oid(tapi_snmp_oid_t *oid, int n, ...)
{
    int     i;
    va_list ap;

    va_start(ap, n);
    for (i = 0; i < n; i++)
    {
        if (oid->length >= MAX_OID_LEN)
        {
            ERROR("OID passed to %s is too long - operation has no effect",
                  __FUNCTION__);
            return;
        }
        oid->id[oid->length] = va_arg(ap, int);
        oid->length++;
    }
    va_end(ap);
}

/* See description in tapi_snmp.h */
int
tapi_snmp_make_oid(const char *oid_str, tapi_snmp_oid_t *bin_oid)
{
    if (!snmp_lib_initialized)
    {
        init_snmp("");
        snmp_lib_initialized = 1;
    }

    /* Initialize to zero */
    memset(bin_oid, 0, sizeof(*bin_oid));
    bin_oid->length = sizeof(bin_oid->id) / sizeof(bin_oid->id[0]);

    /* Check if we have the same length of an element of OID */
    if (sizeof(oid) == sizeof(bin_oid->id[0]))
    {
        if (snmp_parse_oid(oid_str, (oid *)bin_oid->id,
                           &(bin_oid->length)) == NULL)
        {
            return TE_RC(TE_TAPI, TE_ENOENT);
        }
    }
    else if (sizeof(bin_oid->id[0]) > sizeof(oid))
    {
        oid      name[bin_oid->length];
        unsigned i;

        if (snmp_parse_oid(oid_str, name, &bin_oid->length) == NULL)
        {
            return TE_RC(TE_TAPI, TE_ENOENT);
        }
        for (i = 0; i < bin_oid->length; i++)
        {
            bin_oid->id[i] = name[i];
        }
    }
    else
    {
        ERROR("Size of SUBID in NET-SNMP library is more than "
              "in tapi_snmp_oid_t");
        return TE_RC(TE_TAPI, TE_EFAULT);
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
        WARN("error in %s: %r", __FUNCTION__, rc);
        return;
    }

    VERB("parse SNMP file OK!");

    snmp_message = asn_read_indexed(packet, 0, "pdus");
    rc = tapi_snmp_packet_to_plain(snmp_message, &plain_msg);
    VERB("packet to plain rc %r", rc);
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
    char   tmp_name[] = "/tmp/te_snmp_trap_trrecv.XXXXXX";
    struct tapi_pkt_handler_data *i_data;

    if (ta_name == NULL || pattern == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((i_data = malloc(sizeof(*i_data))) == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    i_data->user_callback = cb;
    i_data->user_data = cb_data;

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
    {
        free(i_data);
        return TE_RC(TE_TAPI, rc);
    }

    rc = asn_save_to_file(pattern, tmp_name);
    if (rc)
    {
        unlink(tmp_name);
        free(i_data);
        return TE_RC(TE_TAPI, rc);
    }

    rc = rcf_ta_trrecv_start(ta_name, sid, snmp_csap, tmp_name,
                             (cb != NULL) ? tapi_snmp_trap_handler : NULL,
                             (cb != NULL) ? (void *) i_data : NULL,
                             timeout, num);
    if (rc != 0)
    {
        ERROR("%s() failed(%r) on TA %s:%d CSAP %d file %s",
              __FUNCTION__, rc, ta_name, sid, snmp_csap, tmp_name);
    }

#if !(TAPI_DEBUG)
    unlink(tmp_name);
#endif

    return rc;
}


/* See description in tapi_snmp.h */
int
tapi_snmp_make_instance(const char *oid_str, tapi_snmp_oid_t *bin_oid, ...)
{
    int                 rc;
    int                 dimension = 0;
    va_list             list;
    struct tree        *entry_node;
    struct index_list  *t_index;

    tapi_snmp_oid_t     bin_index;

    bin_index.length = 0;

    entry_node = find_tree_node(oid_str, -1);
    if (entry_node == NULL)
    {
        ERROR("Bad oid string %s", oid_str);
       return TE_EWRONGPTR;
    }

    /* This node must be scalar or table leaf */
    if (entry_node->parent == NULL)
    {
        ERROR("Parent doesn't exist, strange");
       return -1;
    }
    t_index = entry_node->parent->indexes;
    while (t_index)
    {
       dimension++;
       t_index = t_index->next;
    }

    rc = tapi_snmp_make_oid(oid_str, bin_oid);
    if (rc)
    {
       ERROR("Make oid failed for %s oid string", oid_str);
        return rc;
    }

    if (dimension == 0)
    {
        VERB("Make instance %s.0", oid_str);
       bin_oid->id[bin_oid->length++] = 0;
       return 0;
    }

    va_start(list, bin_oid);

    for (; dimension > 0; dimension--)
    {
        bin_oid->id[bin_oid->length++] = 
            bin_index.id[bin_index.length++] = va_arg(list, int);
    }
    va_end(list);
    VERB("Make instance %s%s", oid_str, print_oid(&bin_index));

    return 0;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_make_vb(tapi_snmp_varbind_t *vb, const char *oid_str,
                  const char *type, const char *value, ...)
{
    tapi_snmp_oid_t       bin_oid;
    netsnmp_pdu          *pdu;
    struct variable_list *var;
    int                   rc;
    enum snmp_obj_type    obj_type;

    if ((rc = tapi_snmp_make_oid(oid_str, &bin_oid)) != 0)
    {
       ERROR("Make oid failed for %s oid string", oid_str);
        return rc;
    }

    if ((rc = tapi_snmp_get_object_type(&bin_oid, &obj_type)) != 0)
    {
        ERROR("%s Cannot get type of %s object", __FUNCTION__, oid_str);
        return rc;
    }

    switch (obj_type)
    {
        case SNMP_OBJ_SCALAR:
            /* Scalar object - just append zero */
            if ((bin_oid.length + 1) >= MAX_OID_LEN)
            {
                ERROR("%s: Object %s has too long OID", __FUNCTION__,
                      oid_str);
                return TE_RC(TE_TAPI, TE_EFAULT);
            }
            tapi_snmp_append_oid(&bin_oid, 1, 0);

            break;

        case SNMP_OBJ_TBL_FIELD:
        {
            tapi_snmp_oid_t *tbl_index;
            va_list          ap;

            va_start(ap, value);

            /* Table object - apend index */
            tbl_index = va_arg(ap, tapi_snmp_oid_t *);
            tapi_snmp_cat_oid(&bin_oid, tbl_index);

            va_end(ap);

            break;
        }

        default:
            ERROR("%s: It is not allowed to pass objects other than "
                  "table fields and scalars.", __FUNCTION__);
            return TE_RC(TE_TAPI, TE_EFAULT);
    }

    pdu = snmp_pdu_create(SNMP_MSG_SET);
    rc = snmp_add_var(pdu, bin_oid.id, bin_oid.length, *type, value);
    if (rc != 0)
    {
        ERROR("Net-SNMP library cannot create VarBind for "
              "OID: %s, type: %c, value: %s", oid_str, *type, value);
        snmp_free_pdu(pdu);
        return TE_RC(TE_NET_SNMP, rc);
    }
    var = pdu->variables;
    if (var == NULL)
    {
        ERROR("Net-SNMP library does not create VarBind for "
              "OID: %s, type: %c, value: %s", oid_str, *type, value);
        snmp_free_pdu(pdu);
        return TE_RC(TE_TAPI, TE_EFAULT);
    }
    memcpy(&(vb->name), &bin_oid, sizeof(bin_oid));
    vb->type = var->type;
    vb->v_len = var->val_len;

    switch (vb->type)
    {
        case ASN_OCTET_STR:
#ifdef OPAQUE_SPECIAL_TYPES
        case ASN_OPAQUE_U64:
#endif
            if ((vb->oct_string = (unsigned char *)malloc(vb->v_len))
                                == NULL)
            {
                snmp_free_pdu(pdu);
                return TE_RC(TE_TAPI, TE_ENOMEM);
            }
            memcpy(vb->oct_string, var->val.string, vb->v_len);
            break;

        case ASN_OBJECT_ID:
            assert(sizeof(vb->obj_id->id[0]) == sizeof(oid));

            /* For OID 'v_len' keeps the number of Bub-IDs */
            vb->v_len /= sizeof(oid);

            if (vb->v_len > 
                sizeof(vb->obj_id->id) / sizeof(vb->obj_id->id[0]))
            {
                snmp_free_pdu(pdu);
                ERROR("%s(): The value %s of type 'OBJECT ID'"
                      " is too long", __FUNCTION__, value);
                return TE_EFAULT;
            }

            if ((vb->obj_id = 
                    (tapi_snmp_oid_t *)malloc(sizeof(*vb->obj_id))) == NULL)
            {
                snmp_free_pdu(pdu);
                return TE_RC(TE_TAPI, TE_ENOMEM);
            }

            memcpy(vb->obj_id->id, var->val.objid, var->val_len);
            vb->obj_id->length = vb->v_len;

            break;

        default:
            vb->integer = *(var->val.integer);
            break;
    }
    INFO("%s vb_len = %d", __FUNCTION__, vb->v_len);
    snmp_free_pdu(pdu);

    return 0;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_cmp_vb(const tapi_snmp_varbind_t *vb1,
                 const tapi_snmp_varbind_t *vb2,
                 tapi_snmp_vb_cmp_type cmp_type)
{
    int rc;

    switch (cmp_type)
    {
        case TAPI_SNMP_VB_VMP_FULL:
            /* FALLTHROUGH */

        case TAPI_SNMP_VB_VMP_OID_ONLY:
            if ((rc = tapi_snmp_cmp_oid(&(vb1->name), &(vb1->name))) != 0)
                return rc;

            if (cmp_type == TAPI_SNMP_VB_VMP_OID_ONLY)
                break;

            /* FALLTHROUGH */

        case TAPI_SNMP_VB_VMP_VALUE_ONLY:
            if (vb1->type != vb2->type)
            {
                INFO("'vb1' and 'vb2' has different types of value:\n"
                     "'vb1': %s - value type: %s\n'vb2': %s - "
                     "value type: %s",
                     print_oid(&vb1->name),
                     tapi_snmp_val_type_h2str(vb1->type),
                     print_oid(&vb2->name),
                     tapi_snmp_val_type_h2str(vb2->type));
                return -1;
            }
            if (vb1->v_len != vb2->v_len)
            {
                INFO("'vb1' and 'vb2' has the same value types %s but "
                     "different length of values:\n"
                     "'vb1': %s - value len: %d\n"
                     "'vb2': %s - value len: %d",
                     tapi_snmp_val_type_h2str(vb1->type),
                     print_oid(&vb1->name), vb1->v_len,
                     print_oid(&vb2->name), vb2->v_len);
                return -1;
            }
            switch (vb1->type)
            {
                case ASN_OCTET_STR:
#ifdef OPAQUE_SPECIAL_TYPES
                case ASN_OPAQUE_U64:
#endif
                    if (memcmp(vb1->oct_string, vb2->oct_string,
                               vb1->v_len) != 0)
                    {
                        INFO("'vb1' and 'vb2' has different values:\n"
                             "'vb1': %s - value: %d\n"
                             "'vb2': %s - value: %d",
                             print_oid(&vb1->name),
                             tapi_snmp_print_oct_str(vb1->oct_string,
                                                     vb1->v_len),
                             print_oid(&vb2->name),
                             tapi_snmp_print_oct_str(vb2->oct_string,
                                                     vb2->v_len));
                         return -1;
                    }
                    break;

                case ASN_OBJECT_ID:
                    assert(vb1->v_len == vb1->obj_id->length);
                    assert(vb2->v_len == vb2->obj_id->length);

                    if (tapi_snmp_cmp_oid(vb1->obj_id, vb2->obj_id) != 0)
                    {
                        INFO("'vb1' and 'vb2' has different values:\n"
                             "'vb1': %s - value: %d\n"
                             "'vb2': %s - value: %d",
                             print_oid(&vb1->name), print_oid(vb1->obj_id),
                             print_oid(&vb2->name), print_oid(vb2->obj_id));
                         return -1;
                    }
                    break;

                default:
                    if (vb1->integer != vb2->integer)
                    {
                        INFO("'vb1' and 'vb2' has different values:\n"
                             "'vb1': %s - %d\n'vb2': %s - %d",
                             print_oid(&vb1->name), vb1->integer,
                             print_oid(&vb2->name), vb2->integer);
                         return -1;
                    }
                    break;
            }
            break;

        default:
            ERROR("Unknown comparision type");
            return -1;
    } /* end switch (cmp_type) */

    return 0;
}

/* See description in tapi_snmp.h */
int
tapi_snmp_cmp_oid(const tapi_snmp_oid_t *oid1, const tapi_snmp_oid_t *oid2)
{
    size_t i;
    size_t min_len;

    min_len = (oid1->length < oid2->length) ? oid1->length : oid2->length;

    for (i = 0; i < min_len; i++)
    {
        if (oid1->id[i] != oid2->id[i])
        {
            if (oid1->id[i] > oid2->id[i])
                return 1;
            else
                return -1;
        }
    }

    /* Common Sub IDs are the same, check the rest Sub IDs */
    return (oid1->length - oid2->length);
}

/** Convert SNMP ERROR constants to string format */
const char *snmp_error_h2str(int error_val)
{
    switch (error_val)
    {
#define SNMP_ERR_H2STR(val_) \
    case SNMP_ERR_ ## val_:  \
        return #val_

        SNMP_ERR_H2STR(NOERROR);
        SNMP_ERR_H2STR(TOOBIG);
        SNMP_ERR_H2STR(NOSUCHNAME);
        SNMP_ERR_H2STR(BADVALUE);
        SNMP_ERR_H2STR(READONLY);
        SNMP_ERR_H2STR(GENERR);

        SNMP_ERR_H2STR(NOACCESS);
        SNMP_ERR_H2STR(WRONGTYPE);
        SNMP_ERR_H2STR(WRONGLENGTH);
        SNMP_ERR_H2STR(WRONGENCODING);
        SNMP_ERR_H2STR(WRONGVALUE);
        SNMP_ERR_H2STR(NOCREATION);
        SNMP_ERR_H2STR(INCONSISTENTVALUE);
        SNMP_ERR_H2STR(RESOURCEUNAVAILABLE);
        SNMP_ERR_H2STR(COMMITFAILED);
        SNMP_ERR_H2STR(UNDOFAILED);
        SNMP_ERR_H2STR(AUTHORIZATIONERROR);
        SNMP_ERR_H2STR(NOTWRITABLE);

        SNMP_ERR_H2STR(INCONSISTENTNAME);

#undef SNMP_ERR_H2STR

        default:
        {
            static char buf[255];

            snprintf(buf, sizeof(buf), "UNKNOWN (%d)", error_val);
            return buf;
        }
    }
    return "";
}

const char *
snmp_obj_type_h2str(enum snmp_obj_type obj_type)
{
    switch (obj_type)
    {
        case SNMP_OBJ_SCALAR: return "scalar";
        case SNMP_OBJ_TBL_FIELD: return "tabular";
        case SNMP_OBJ_TBL_ENTRY: return "table entry";
        case SNMP_OBJ_TBL: return "table itself";
        default: return "unknown";
    }
    return "";
}

const char *
tapi_snmp_val_type_h2str(enum tapi_snmp_vartypes_t type)
{
    switch (type)
    {
#define TAPI_SNMP_VAL_TYPE_H2STR(val_) \
        case TAPI_SNMP_ ## val_:  \
            return #val_

        TAPI_SNMP_VAL_TYPE_H2STR(OTHER);
        TAPI_SNMP_VAL_TYPE_H2STR(INTEGER);
        TAPI_SNMP_VAL_TYPE_H2STR(OCTET_STR);
        TAPI_SNMP_VAL_TYPE_H2STR(OBJECT_ID);
        TAPI_SNMP_VAL_TYPE_H2STR(IPADDRESS);
        TAPI_SNMP_VAL_TYPE_H2STR(COUNTER);
        TAPI_SNMP_VAL_TYPE_H2STR(UNSIGNED);
        TAPI_SNMP_VAL_TYPE_H2STR(TIMETICKS);
        TAPI_SNMP_VAL_TYPE_H2STR(NOSUCHOBJ);
        TAPI_SNMP_VAL_TYPE_H2STR(NOSUCHINS);
        TAPI_SNMP_VAL_TYPE_H2STR(ENDOFMIB);

#undef TAPI_SNMP_VAL_TYPE_H2STR
        default:
        {
            static char buf[255];

            snprintf(buf, sizeof(buf), "UNKNOWN (%d)", type);
            return buf;
        }
    }

    return "IMPOSSIBLE";
}

const char *
tapi_snmp_obj_status_h2str(enum tapi_snmp_mib_status obj_status)
{
    switch (obj_status)
    {
#define TAPI_SNMP_OBJ_STATUS_H2STR(val_) \
        case TAPI_SNMP_MIB_STATUS_ ## val_:  \
            return #val_
            
        TAPI_SNMP_OBJ_STATUS_H2STR(MANDATORY);
        TAPI_SNMP_OBJ_STATUS_H2STR(OPTIONAL);
        TAPI_SNMP_OBJ_STATUS_H2STR(OBSOLETE);
        TAPI_SNMP_OBJ_STATUS_H2STR(DEPRECATED);
        TAPI_SNMP_OBJ_STATUS_H2STR(CURRENT);


#undef TAPI_SNMP_OBJ_STATUS_H2STR
        default:
        {
            static char buf[255];

            snprintf(buf, sizeof(buf), "UNKNOWN (%d)", obj_status);
            return buf;
        }
    }

    return "IMPOSSIBLE";
    


}

/* Table of generic SNMP V1 traps */
const struct tapi_snmp_v1_gen_trap_name
tapi_snmp_v1_gen_trap_names[] = {
    { TAPI_SNMP_TRAP_COLDSTART,          "coldStart" },
    { TAPI_SNMP_TRAP_WARMSTART,          "warmStart" },
    { TAPI_SNMP_TRAP_LINKDOWN,           "linkDown" },
    { TAPI_SNMP_TRAP_LINKUP,             "linkUp" },
    { TAPI_SNMP_TRAP_AUTHFAIL,           "authenticationFailure" },
    { TAPI_SNMP_TRAP_EGPNEIGHBORLOSS,    "egpNeighborLoss" },
    { TAPI_SNMP_TRAP_ENTERPRISESPECIFIC, "enterpriseSpecific" }
};

/* See description in tapi_snmp.h */
tapi_snmp_gen_trap_t
tapi_snmp_gen_trap_by_name(const char *trap_name)
{
    size_t i;

    for (i = 0; i < sizeof(tapi_snmp_v1_gen_trap_names) /
            sizeof(tapi_snmp_v1_gen_trap_names[0]); i++)
    {
        if (strcmp(trap_name, tapi_snmp_v1_gen_trap_names[i].name) == 0)
        {
            return tapi_snmp_v1_gen_trap_names[i].id;
        }
    }
    return TAPI_SNMP_TRAP_ENTERPRISESPECIFIC;
}

const char *
tapi_snmp_gen_trap_h2str(enum tapi_snmp_gen_trap_t type)
{
    switch (type)
    {
#define TAPI_SNMP_GEN_TRAP_H2STR(val_) \
        case TAPI_SNMP_TRAP_ ## val_:  \
            return #val_

        TAPI_SNMP_GEN_TRAP_H2STR(COLDSTART);
        TAPI_SNMP_GEN_TRAP_H2STR(WARMSTART);
        TAPI_SNMP_GEN_TRAP_H2STR(LINKDOWN);
        TAPI_SNMP_GEN_TRAP_H2STR(LINKUP);
        TAPI_SNMP_GEN_TRAP_H2STR(AUTHFAIL);
        TAPI_SNMP_GEN_TRAP_H2STR(EGPNEIGHBORLOSS);
        TAPI_SNMP_GEN_TRAP_H2STR(ENTERPRISESPECIFIC);

#undef TAPI_SNMP_GEN_TRAP_H2STR
        default:
        {
            static char buf[255];

            snprintf(buf, sizeof(buf), "UNKNOWN (%d)", type);
            return buf;
        }
    }

    return "IMPOSSIBLE";
}

const char *
tapi_snmp_truth_value_h2str(enum tapi_snmp_truth_value val)
{
    switch (val)
    {
        case SNMP_FALSE: return "FALSE";
        case SNMP_TRUE: return "TRUE";

        default:
        {
            static char buf[255];

            snprintf(buf, sizeof(buf), "UNKNOWN (%d)", val);
            return buf;
        }
    }
    return "";
}


