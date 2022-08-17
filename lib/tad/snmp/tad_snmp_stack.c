/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD SNMP
 *
 * Traffic Application Domain Command Handler.
 * SNMP CSAP implementaion, stack-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD SNMP"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(x)
#endif

#include "tad_snmp_impl.h"
#include "logger_api.h"
#include "logger_ta_fast.h"


#undef SNMPDEBUG

/* AES defined as AES128 in NET-SNMP 5.1 */
#undef WITHOUT_AES
#ifndef USM_PRIV_PROTO_AES_LEN
#ifdef USM_PRIV_PROTO_AES128_LEN
#define usmAESPrivProtocol usmAES128PrivProtocol
#define USM_PRIV_PROTO_AES_LEN USM_PRIV_PROTO_AES128_LEN
#else
#define WITHOUT_AES
#endif
#endif

#define WITHOUT_AES

#ifdef SNMPDEBUG
void
print_oid(unsigned long * subids, size_t len)
{
    unsigned int i;

    if (subids == NULL)
        printf(".NULL. :-)");

    for (i = 0; i < len; i++)
        printf(".%lu", subids[i]);
}
#endif

#ifndef RECEIVED_MESSAGE
#define RECEIVED_MESSAGE NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE
#endif

#ifndef TIMED_OUT
#define TIMED_OUT NETSNMP_CALLBACK_OP_TIMED_OUT
#endif


void
tad_snmp_free_pdu(void *ptr, size_t len)
{
    struct snmp_pdu *pdu = (struct snmp_pdu *)ptr;

    UNUSED(len);
    if (pdu == NULL)
        return;

    snmp_free_pdu(pdu);
}

int
snmp_csap_input(int                  op,
                struct snmp_session *session,
                int                  reqid,
                struct snmp_pdu     *pdu,
                void                *magic)
{
    snmp_csap_specific_data_p   spec_data;

    UNUSED(session);
    UNUSED(reqid);

    spec_data = (snmp_csap_specific_data_p) magic;
    VERB("input callback, operation: %d", op);

    if (op == RECEIVED_MESSAGE)
    {
        assert(spec_data->pdu == NULL);

        spec_data->pdu = snmp_clone_pdu(pdu);

        if (spec_data->pdu == NULL)
            ERROR("%s(): Failed to clone received SNMP PDU",
                  __FUNCTION__);
        else
            F_VERB("%s(): SNMP PDU received", __FUNCTION__);
    }

    if (op == TIMED_OUT)
    {
        F_VERB("%s(): SNMP operation timed out", __FUNCTION__);
    }

    return 1;
}


/**
 * Callback for release internal data after traffic processing.
 *
 * @param csap    pointer to CSAP descriptor structure
 *
 * @return Status code
 */
te_errno
tad_snmp_release_cb(csap_p csap)
{
    snmp_csap_specific_data_p spec_data =
        csap_get_proto_spec_data(csap, csap_get_rw_layer(csap));

    assert(spec_data->pdu == NULL);

    return 0;
}

/* See description in tad_snmp_impl.h */
te_errno
tad_snmp_read_cb(csap_p csap, unsigned int timeout,
                 tad_pkt *pkt, size_t *pkt_len)
{
    snmp_csap_specific_data_p   spec_data;

    te_errno        rc;
    int             ret;

    int             n_fds = 0;
    int             block = 0;
    fd_set          fdset;
    struct timeval  sel_timeout;

    assert(pkt != NULL);
    assert(pkt_len != NULL);

    spec_data = csap_get_proto_spec_data(csap, csap_get_rw_layer(csap));

    FD_ZERO(&fdset);
    TE_US2TV(timeout, &sel_timeout);

    if (spec_data->sock < 0)
    {
        snmp_select_info(&n_fds, &fdset, &sel_timeout, &block);
    }
    else
    {
        FD_SET(spec_data->sock, &fdset);
        n_fds = spec_data->sock + 1;
    }

    assert(spec_data->pdu == NULL);

    ret = select(n_fds, &fdset, 0, 0, &sel_timeout);
    VERB("%s(): CSAP %d, after select, ret %d\n",
         __FUNCTION__, csap->id, ret);

    if (ret > 0)
    {
        tad_pkt_seg *seg;

        seg = tad_pkt_first_seg(pkt);
        if (seg == NULL)
        {
            seg = tad_pkt_alloc_seg(NULL, 0, NULL);
            if (seg == NULL)
                return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
            tad_pkt_append_seg(pkt, seg);
        }

        /* snmp_csap_input() callback is called */
        snmp_read(&fdset);

        if (spec_data->pdu != NULL)
        {
            tad_pkt_put_seg_data(pkt, seg, spec_data->pdu,
                                 sizeof(*spec_data->pdu),
                                 tad_snmp_free_pdu);
            spec_data->pdu = NULL;
            *pkt_len = sizeof(*spec_data->pdu);
            rc = 0;
        }
        else
        {
            rc = TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
        }
    }
    else if (ret == 0)
    {
        rc = TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
    }
    else
    {
        rc = TE_OS_RC(TE_TAD_CSAP, errno);
        assert(rc != 0);
    }

    return rc;
}

/* See description in tad_snmp_impl.h */
te_errno
tad_snmp_write_cb(csap_p csap, const tad_pkt *pkt)
{
    snmp_csap_specific_data_p   spec_data;
    struct snmp_pdu            *pdu = NULL;

    assert(pkt != NULL);

    spec_data = csap_get_proto_spec_data(csap, csap_get_rw_layer(csap));

    if ((tad_pkt_seg_num(pkt) != 1) ||
        ((pdu = tad_pkt_first_seg(pkt)->data_ptr) == NULL) ||
        (tad_pkt_first_seg(pkt)->data_len != sizeof(*pdu)))
    {
        ERROR(CSAP_LOG_FMT "Invalid packet to be sent as SNMP PDU: "
              "n_segs=%u pdu=%p len=%u(vs %u)", CSAP_LOG_ARGS(csap),
              tad_pkt_seg_num(pkt), pdu,
              tad_pkt_first_seg(pkt)->data_len, sizeof(*pdu));
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    /* TODO: Investigate how to avoid PDU coping */
    pdu = snmp_clone_pdu(pdu);
    if (pdu == NULL)
    {
        ERROR(CSAP_LOG_FMT "Failed to close SNMP PDU to be sent",
              CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    if (snmp_send(spec_data->ss, pdu) == 0)
    {
        /* PDU is not owned, have to free it */
        snmp_free_pdu(pdu);
        ERROR(CSAP_LOG_FMT "Send SNMP PDU failed", CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_EIO);
    }

    return 0;
}


#define COMMUNITY 1

/* See description in tad_snmp_impl.h */
te_errno
tad_snmp_rw_init_cb(csap_p csap)
{
    int      rc;
    char     community[COMMUNITY_MAX_LEN + 1];
    char     snmp_agent[100];
    int      timeout;
    int      version;
    size_t   v_len;

    struct snmp_session         csap_session;
    struct snmp_session        *ss = NULL;

    snmp_csap_specific_data_p   snmp_spec_data;
    const asn_value            *snmp_csap_spec =
        csap->layers[csap_get_rw_layer(csap)].nds;

    char                        security_model_name[32];
    ndn_snmp_sec_model_t        security_model;
    ndn_snmp_sec_level_t        security_level;
    ndn_snmp_priv_proto_t       priv_proto;
    ndn_snmp_auth_proto_t       auth_proto;

    uint8_t const              *auth_pass = NULL;
    int                         auth_pass_len;
    uint8_t const              *priv_pass = NULL;
    int                         priv_pass_len;
    char                        security_name[SNMP_MAX_SEC_NAME_SIZE + 1];
    size_t                      security_name_len;


    VERB("Init callback\n");

    snmp_sess_init(&csap_session);

    /* Timeout */
    v_len = sizeof(timeout);
    rc = asn_read_value_field(snmp_csap_spec, &timeout,
                              &v_len, "timeout.#plain");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        timeout = SNMP_CSAP_DEF_TIMEOUT;
    else if (rc != 0)
    {
        ERROR("%s: error reading 'timeout': %r", __FUNCTION__, rc);
        return rc;
    }
    csap_session.timeout = timeout * 1000000L;

    /* Version */
    v_len = sizeof(version);
    rc = asn_read_value_field(snmp_csap_spec, &version,
                              &v_len, "version.#plain");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        version = SNMP_CSAP_DEF_VERSION;
    else if (rc != 0)
    {
        ERROR("%s: error reading 'version': %r", __FUNCTION__, rc);
        return rc;
    }
    csap_session.version = version;

    /* Local port */
    v_len = sizeof(csap_session.local_port);
    rc = asn_read_value_field(snmp_csap_spec, &csap_session.local_port,
                              &v_len, "local-port.#plain");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        csap_session.local_port = SNMP_CSAP_DEF_LOCPORT;
    else if (rc != 0)
    {
        ERROR("%s: error reading 'local-port': %r", __FUNCTION__, rc);
        return rc;
    }

    /* Remote port */
    v_len = sizeof(csap_session.remote_port);
    rc = asn_read_value_field(snmp_csap_spec, &csap_session.remote_port,
                              &v_len, "remote-port.#plain");
    if (csap_session.local_port == SNMP_CSAP_DEF_LOCPORT)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
            csap_session.remote_port = SNMP_CSAP_DEF_REMPORT;
        else if (rc != 0)
        {
            ERROR("%s: error reading 'remote-port': %r", __FUNCTION__, rc);
            return rc;
        }
    }
    else
    {
        csap_session.remote_port = 0;
        if (rc == 0)
        {
            RING("%s: local port set to %d, ignoring remote port",
                 __FUNCTION__, csap_session.local_port);
        }
    }

    /* Agent name */
    v_len = sizeof(snmp_agent);
    rc = asn_read_value_field(snmp_csap_spec, snmp_agent,
                              &v_len, "snmp-agent.#plain");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        if (csap_session.local_port == SNMP_CSAP_DEF_LOCPORT)
            strcpy(snmp_agent, SNMP_CSAP_DEF_AGENT);
        else
            snmp_agent[0] = '\0';
    }
    else if (rc != 0)
    {
        ERROR("%s: error reading 'snmp-agent': %r", __FUNCTION__, rc);
        return rc;
    }
    csap_session.peername = snmp_agent;

    /* Security model */
    v_len = sizeof(security_model_name);
    rc = asn_get_choice(snmp_csap_spec, "security", security_model_name,
                        sizeof(security_model_name));
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        security_model = NDN_SNMP_SEC_MODEL_DEFAULT;
    else if (rc != 0)
    {
        ERROR("%s: error reading 'security': %r", __FUNCTION__, rc);
        return rc;
    }
    else if (strcmp(security_model_name, "usm") == 0)
        security_model = NDN_SNMP_SEC_MODEL_USM;
    else if (strcmp(security_model_name, "v2c") == 0)
        security_model = NDN_SNMP_SEC_MODEL_V2C;
    else
    {
        ERROR("%s: unknown security model '%s'", __FUNCTION__,
              security_model_name);
        return TE_ENOENT;
    }

    /* Community-based security model */
    if (security_model == NDN_SNMP_SEC_MODEL_V2C)
    {
        v_len = sizeof(community);
        rc = asn_read_value_field(snmp_csap_spec, community,
                                  &v_len, "security.#v2c.community");
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
            strcpy(community, SNMP_CSAP_DEF_COMMUNITY);
        else if (rc != 0)
        {
            ERROR("%s: error reading community: %r", __FUNCTION__, rc);
            return rc;
        }

        csap_session.securityModel = SNMP_SEC_MODEL_SNMPv2c;
        csap_session.community = (u_char *)community;
        csap_session.community_len = strlen(community);
    }

    /* User-based security model */
    else if (security_model == NDN_SNMP_SEC_MODEL_USM)
    {
        /* Security name */
        security_name_len = sizeof(security_name);
        rc = asn_read_value_field(snmp_csap_spec, security_name,
                                  &security_name_len, "security.#usm.name");
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            ERROR("%s: there is no securityName provided",  __FUNCTION__);
            return rc;
        }
        if (TE_RC_GET_ERROR(rc) == TE_ESMALLBUF)
        {
            ERROR("%s: securityName is too long (max %d is valid)",
                  __FUNCTION__, SNMP_MAX_SEC_NAME_SIZE);
            return rc;
        }
        if (rc != 0)
        {
            ERROR("%s: error reading securityName, rc=%r",
                  __FUNCTION__, rc);
            return rc;
        }

        csap_session.securityModel = SNMP_SEC_MODEL_USM;
        csap_session.securityName = security_name;
        csap_session.securityNameLen = strlen(security_name);

        /* Security level */
        v_len = sizeof(security_level);
        rc = asn_read_value_field(snmp_csap_spec, &security_level,
                                  &v_len, "security.#usm.level");
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
            security_level = NDN_SNMP_SEC_LEVEL_NOAUTH;
        else if (rc != 0)
        {
            ERROR("%s: error reading securityLevel: %r", __FUNCTION__, rc);
            return rc;
        }

        if (security_level == NDN_SNMP_SEC_LEVEL_NOAUTH)
        {
            csap_session.securityLevel = SNMP_SEC_LEVEL_NOAUTH;
        }
        else if (security_level == NDN_SNMP_SEC_LEVEL_AUTHNOPRIV ||
                 security_level == NDN_SNMP_SEC_LEVEL_AUTHPRIV)
        {
            csap_session.securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;

            /* Authentication parameters */
            v_len = sizeof(auth_proto);
            rc = asn_read_value_field(snmp_csap_spec, &auth_proto,
                                      &v_len,
                                      "security.#usm.auth-protocol");
            if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
                auth_proto = NDN_SNMP_AUTH_PROTO_DEFAULT;
            else if (rc != 0)
            {
                ERROR("%s: error reading 'auth-protocol': %r",
                      __FUNCTION__, rc);
                return rc;
            }

            switch (auth_proto)
            {
                case NDN_SNMP_AUTH_PROTO_DEFAULT:
                    csap_session.securityAuthProto = SNMP_DEFAULT_AUTH_PROTO;
                    csap_session.securityAuthProtoLen =
                        SNMP_DEFAULT_AUTH_PROTOLEN;
                    break;

                case NDN_SNMP_AUTH_PROTO_MD5:
                    csap_session.securityAuthProto = usmHMACMD5AuthProtocol;
                    csap_session.securityAuthProtoLen = USM_AUTH_PROTO_MD5_LEN;
                    break;

                case NDN_SNMP_AUTH_PROTO_SHA:
                    csap_session.securityAuthProto = usmHMACSHA1AuthProtocol;
                    csap_session.securityAuthProtoLen = USM_AUTH_PROTO_SHA_LEN;
                    break;

                default:
                    assert(0);
            }

            rc = asn_get_field_data(snmp_csap_spec, &auth_pass,
                                    "security.#usm.auth-pass");
            if (rc != 0)
            {
                ERROR("%s: error reading 'auth-pass': %r",
                      __FUNCTION__, rc);
                return rc;
            }
            auth_pass_len = asn_get_length(snmp_csap_spec,
                                           "security.#usm.auth-pass");
            if (auth_pass_len < 0)
            {
                ERROR("%s: asn_get_length() failed unexpectedly",
                      __FUNCTION__);
                return TE_RC(TE_TAD_CSAP, TE_EFAULT);
            }

            /* Key generation */
            csap_session.securityAuthKeyLen =
                sizeof(csap_session.securityAuthKey);
            if (generate_Ku(csap_session.securityAuthProto,
                            csap_session.securityAuthProtoLen,
                            (unsigned char *)auth_pass, auth_pass_len,
                            csap_session.securityAuthKey,
                            &csap_session.securityAuthKeyLen)
                    != SNMPERR_SUCCESS)
            {
                ERROR("%s: failed to generate a key from authentication "
                      "passphrase: %s", __FUNCTION__,
                      snmp_api_errstring(snmp_errno));
                return TE_ETADLOWER;
            }
        }

        if (security_level == NDN_SNMP_SEC_LEVEL_AUTHPRIV)
        {
            csap_session.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;

            /* Privacy parameters */
            v_len = sizeof(priv_proto);
            rc = asn_read_value_field(snmp_csap_spec, &priv_proto,
                                      &v_len,
                                      "security.#usm.priv-protocol");
            if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
                priv_proto = NDN_SNMP_PRIV_PROTO_DEFAULT;
            else if (rc != 0)
            {
                ERROR("%s: error reading 'priv-protocol': %r",
                      __FUNCTION__, rc);
                return rc;
            }

            switch (priv_proto)
            {
                case NDN_SNMP_PRIV_PROTO_DEFAULT:
                    csap_session.securityPrivProto = SNMP_DEFAULT_PRIV_PROTO;
                    csap_session.securityPrivProtoLen =
                        SNMP_DEFAULT_PRIV_PROTOLEN;
                    break;

                case NDN_SNMP_PRIV_PROTO_DES:
                    csap_session.securityPrivProto = usmDESPrivProtocol;
                    csap_session.securityPrivProtoLen = USM_PRIV_PROTO_DES_LEN;
                    break;

                case NDN_SNMP_PRIV_PROTO_AES:
#ifndef WITHOUT_AES
                    csap_session.securityPrivProto = usmAESPrivProtocol;
                    csap_session.securityPrivProtoLen = USM_PRIV_PROTO_AES_LEN;
#else
                    ERROR("%s: there is no AES support in NET-SNMP",
                          __FUNCTION__);
                    return TE_ETADLOWER;
#endif
                    break;

                default:
                    assert(0);
            }

            rc = asn_get_field_data(snmp_csap_spec, &priv_pass,
                                    "security.#usm.priv-pass");
            if (rc != 0)
            {
                ERROR("%s: error reading 'priv-pass': %r",
                      __FUNCTION__, rc);
                return rc;
            }
            priv_pass_len = asn_get_length(snmp_csap_spec,
                                           "security.#usm.priv-pass");
            if (priv_pass_len < 0)
            {
                ERROR("%s: asn_get_length() failed unexpectedly",
                      __FUNCTION__);
                return TE_RC(TE_TAD_CSAP, TE_EFAULT);
            }

            /* Key generation */
            csap_session.securityPrivKeyLen =
                sizeof(csap_session.securityPrivKey);
            if (generate_Ku(csap_session.securityAuthProto,
                            csap_session.securityAuthProtoLen,
                            (unsigned char *)priv_pass, priv_pass_len,
                            csap_session.securityPrivKey,
                            &csap_session.securityPrivKeyLen)
                    != SNMPERR_SUCCESS)
            {
                ERROR("%s: failed to generate a key from privacy "
                      "passphrase: %s", __FUNCTION__,
                      snmp_api_errstring(snmp_errno));
                return TE_ETADLOWER;
            }
        }
    }

    snmp_spec_data = malloc(sizeof(snmp_csap_specific_data_t));
    if (snmp_spec_data == NULL)
    {
        ERROR("%s: malloc failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    VERB("try to open SNMP session: \n");
    VERB("  version:    %d\n", csap_session.version);
    VERB("  rem-port:   %d\n", csap_session.remote_port);
    VERB("  loc-port:   %d\n", csap_session.local_port);
    VERB("  timeout:    %d\n", csap_session.timeout);
    VERB("  peername:   %s\n", csap_session.peername ?
                               csap_session.peername : "(null)" );
#if COMMUNITY
    VERB("  community:  %s\n", csap_session.community);
#endif
    csap_session.callback       = snmp_csap_input;
    csap_session.callback_magic = snmp_spec_data;

    snmp_spec_data->sock = -1;

    do {
        char buf[32];
        netsnmp_transport *transport = NULL;

        snprintf(buf, sizeof(buf), "%s:%d",
                (csap_session.remote_port != 0 && strlen(snmp_agent) > 0)
                ? snmp_agent : "0.0.0.0",
                csap_session.remote_port != 0
                ? csap_session.remote_port : csap_session.local_port);

        transport = netsnmp_tdomain_transport(buf,
                                        !csap_session.remote_port, "udp");
        if (transport == NULL)
        {
            ERROR("%s: failed to create transport: %s",
                  __FUNCTION__, snmp_api_errstring(snmp_errno));
            break;
        }

        ss = snmp_add(&csap_session, transport, NULL, NULL);
        snmp_spec_data->sock = transport->sock;
        VERB("%s(): CSAP %d, sock = %d", __FUNCTION__, csap->id,
             snmp_spec_data->sock);
    } while(0);

    if (ss == NULL)
    {
        ERROR("%s: open session or transport error: %s",
              __FUNCTION__, snmp_api_errstring(snmp_errno));

        free(snmp_spec_data);
        return TE_ETADLOWER;
    }
#if 0
    VERB("in init, session: %x\n", ss);

    VERB("OPENED  SNMP session: \n");
    VERB("  version:    %d\n", ss->version);
    VERB("  rem-port:   %d\n", ss->remote_port);
    VERB("  loc-port:   %d\n", ss->local_port);
    VERB("  timeout:    %d\n", ss->timeout);
    VERB("  peername:   %s\n", ss->peername);
#if COMMUNITY
    VERB("  community:  %s\n", ss->community);
#endif
#endif

    csap_set_proto_spec_data(csap, csap_get_rw_layer(csap), snmp_spec_data);
    csap_set_rw_data(csap, snmp_spec_data);
    snmp_spec_data->ss  = ss;
    snmp_spec_data->pdu = 0;

    return 0;
}


/* See description in tad_snmp_impl.h */
int
tad_snmp_rw_destroy_cb(csap_p csap)
{
    snmp_csap_specific_data_p spec_data = csap_get_rw_data(csap);

    VERB("Destroy callback, id %d\n", csap->id);
    assert(spec_data->pdu == NULL);

    if (spec_data->ss)
    {
        struct snmp_session *s = spec_data->ss;

        snmp_close(s);
    }
    return 0;
}
