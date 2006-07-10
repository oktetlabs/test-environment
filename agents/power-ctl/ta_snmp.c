/** @file
 * @brief Power Distribution Unit Proxy Test Agent
 *
 * Net-SNMP library wrapper functions.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Boris Misenov <Boris.Misenov@oktetlabs.ru>
 * 
 * $Id$
 */

#include "ta_snmp.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "te_sockaddr.h"

#ifndef HAVE_SNMP_PDU_TYPE
/**
 * Get the human-readable name of SNMP datagram type
 *
 * @param type  Internal type of datagram
 *
 * @note This function is taken from net-snmp 5.3 - it is not implemented
 *       in version 5.2 of the library.
 */
static const char *
snmp_pdu_type(int type)
{
    static char unknown[20];

    switch(type) {
        case SNMP_MSG_GET:
            return "GET";
        case SNMP_MSG_GETNEXT:
            return "GETNEXT";
        case SNMP_MSG_RESPONSE:
            return "RESPONSE";
        case SNMP_MSG_SET:
            return "SET";
        case SNMP_MSG_GETBULK:
            return "GETBULK";
        case SNMP_MSG_INFORM:
            return "INFORM";
        case SNMP_MSG_TRAP2:
            return "TRAP2";                                                             case SNMP_MSG_REPORT:
            return "REPORT";
        default:                                                                            snprintf(unknown, sizeof(unknown), "?0x%2X?", type);                            return unknown;
    }
}
#endif

/* See description in ta_snmp.h */
void
ta_snmp_init()
{
    init_snmp("snmpapp");
}

/* See description in ta_snmp.h */
ta_snmp_session *
ta_snmp_open_session(const struct sockaddr *net_addr, long int version,
                     const char *community)
{
    netsnmp_session session;
    char            comm[COMMUNITY_MAX_LEN + 1];
    char            peer[INET6_ADDRSTRLEN];

    snmp_sess_init(&session);
    session.version = version;
    session.community_len = strlen(community);
    strncpy(comm, community, sizeof(comm));
    if (session.community_len > COMMUNITY_MAX_LEN)
    {
        WARN("%s(): Community name '%s' length (%d) exceeded limit of %d, "
             "truncated", __FUNCTION__, community, session.community_len,
             COMMUNITY_MAX_LEN);
        session.community_len = COMMUNITY_MAX_LEN;
        comm[COMMUNITY_MAX_LEN] = '\0';
    }
    session.community = comm;
    strncpy(peer, te_sockaddr_get_ipstr(net_addr), sizeof(peer));
    session.peername = peer;

    return snmp_open(&session);
}

/* See description in ta_snmp.h */
void
ta_snmp_close_session(ta_snmp_session *session)
{
    snmp_close(session);
}

/**
 * Send SNMP response with single OID synchronously and check the response
 * (generic function)
 *
 * @param session    Opened SNMP session
 * @param pdu_type   SNMP PDU type to send
 * @param oid        OID of the object to include to PDU
 * @param oid_len    Length of OID (in OID components)
 * @param type       Value data type
 * @param value      Object value data
 * @param value_len  Length of object value data (in bytes)
 * @param response   Placeholder for pointer to response SNMP PDU structure
 *                   (should be freed with snmp_free_pdu())
 *
 * @return Status code.
 */
static te_errno
ta_snmp_request(ta_snmp_session *session, int pdu_type,
                ta_snmp_oid *oid, size_t oid_len,
                ta_snmp_type_t type, const uint8_t *value, size_t value_len,
                netsnmp_pdu **response)
{
    netsnmp_pdu *pdu;
    int          status;
    te_errno     retval = 0;

    if ((pdu = snmp_pdu_create(pdu_type)) == NULL)
    {
        ERROR("%s(): failed to create SNMP PDU", __FUNCTION__);
        return TE_RC(TE_TA, TE_ENOMEM);
    }
    if ((snmp_pdu_add_variable(pdu, oid, oid_len,
                               type, value, value_len)) == NULL)
    {
        ERROR("%s(): failed to prepare SNMP request", __FUNCTION__);
        snmp_free_pdu(pdu);
        return TE_RC(TE_NET_SNMP, TE_EINVAL);
    }

    status = snmp_synch_response(session, pdu, response);
    switch (status)
    {
        case STAT_SUCCESS:
            if ((*response)->errstat != SNMP_ERR_NOERROR)
            {
                ERROR("%s(): SNMP %s failed: %s",
                      __FUNCTION__, snmp_pdu_type(pdu_type),
                      snmp_errstring((*response)->errstat));
                retval = TE_RC(TE_NET_SNMP, TE_EFAIL);
            }
            break;

        case STAT_TIMEOUT:
            ERROR("%s(): Response timeout", __FUNCTION__);
            retval = TE_RC(TE_NET_SNMP, TE_ETIMEDOUT);
            break;

        default:
            {
                char *errmsg = NULL;

                snmp_error(session, NULL, NULL, &errmsg);
                ERROR("%s(): %s", __FUNCTION__, errmsg);
                SNMP_FREE(errmsg);
                retval = TE_RC(TE_NET_SNMP, TE_EFAIL);
            }
    }

    return retval;
}

/* See description in ta_snmp.h */
te_errno
ta_snmp_set(ta_snmp_session *session, ta_snmp_oid *oid, size_t oid_len,
            ta_snmp_type_t type, const uint8_t *value, size_t value_len)
{
    netsnmp_pdu *response = NULL;
    te_errno     retval;

    retval = ta_snmp_request(session, SNMP_MSG_SET, oid, oid_len,
                             type, value, value_len, &response);
    if (response != NULL)
        snmp_free_pdu(response);

    return retval;
}

/* See description in ta_snmp.h */
te_errno
ta_snmp_get(ta_snmp_session *session, ta_snmp_oid *oid, size_t oid_len,
            ta_snmp_type_t *type, uint8_t *buf, size_t *buf_len)
{
    netsnmp_pdu           *response = NULL;
    te_errno               retval;
    netsnmp_variable_list *vars;

    if ((retval = ta_snmp_request(session, SNMP_MSG_GET, oid, oid_len,
                                  TA_SNMP_NULL, NULL, 0, &response)) != 0)
        goto cleanup;

    vars = response->variables;
    if (vars != NULL && vars->name_length == oid_len &&
        memcmp(vars->name, oid, oid_len * sizeof(oid[0])) == 0)
    {
        if (*buf_len < vars->val_len)
        {
            ERROR("%s(): buffer is too small (%u vs %u)", __FUNCTION__,
                  *buf_len, vars->val_len);
            *buf_len = vars->val_len;
            retval = TE_ESMALLBUF;
            goto cleanup;
        }

        VERB("%s(): SNMP response, type %u len %u", __FUNCTION__,
             vars->type, vars->val_len);
        *buf_len = vars->val_len;
        switch (vars->type)
        {
            case ASN_INTEGER:
                *type = TA_SNMP_INTEGER;
                memcpy(buf, vars->val.integer, vars->val_len);
                break;

            case ASN_OCTET_STR:
                *type = TA_SNMP_STRING;
                memcpy(buf, vars->val.string, vars->val_len);
                break;

            default:
                ERROR("%s(): unsupported value type %u", __FUNCTION__,
                      vars->type);
        }
    }

cleanup:
    if (response != NULL)
        snmp_free_pdu(response);

    return retval;
}

/* See description in ta_snmp.h */
te_errno
ta_snmp_get_int(ta_snmp_session *session, ta_snmp_oid *oid, size_t oid_len,
                 long int *value)
{
    te_errno       rc;
    ta_snmp_type_t type;
    long int       result;
    size_t         buf_len = sizeof(result);

    if ((rc = ta_snmp_get(session, oid, oid_len, &type, (uint8_t *)&result,
                          &buf_len)) != 0)
        return rc;

    if (type != TA_SNMP_INTEGER)
        return TE_EINVAL;

    *value = result;
    return 0;
}

/* See description in ta_snmp.h */
te_errno
ta_snmp_get_string(ta_snmp_session *session, ta_snmp_oid *oid, size_t oid_len,
                   char *buf, size_t *buf_len)
{
    te_errno       rc;
    ta_snmp_type_t type;

    (*buf_len)--;    /* Leave space for trailing zero */
    rc = ta_snmp_get(session, oid, oid_len, &type, (uint8_t *)buf, buf_len);
    (*buf_len)++;
    if (rc != 0)
        return rc;

    if (type != TA_SNMP_STRING)
        return TE_EINVAL;

    buf[*buf_len - 1] = '\0';
    return 0;
}

