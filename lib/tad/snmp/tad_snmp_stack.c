/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Dummy FILE protocol implementaion, stack-related callbacks.
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

#include "tad_snmp_impl.h"

#define TE_LGR_USER     "TAD SNMP"
#include "logger_ta.h"

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


#undef SNMPDEBUG

#define NEW_SNMP_API 1

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
tad_snmp_free_pdu(void *ptr)
{
    struct snmp_pdu *pdu = (struct snmp_pdu *)ptr;

    if (pdu == NULL) 
        return; 

    snmp_free_pdu(pdu);
}

int
snmp_csap_input(
    int op,
    struct snmp_session *session,
    int reqid,
    struct snmp_pdu *pdu,
    void *magic)
{
    snmp_csap_specific_data_p   spec_data; 

    UNUSED(session);
    UNUSED(reqid);

    spec_data = (snmp_csap_specific_data_p) magic;
    VERB("input callback, operation: %d", op); 

    if (op == RECEIVED_MESSAGE)
        spec_data->pdu = snmp_clone_pdu(pdu); 

    if ( op == TIMED_OUT )
    {
        ;
    }
    return 1; 
}


/**
 * Callback for release internal data after traffic processing. 
 *
 * @param csap_descr    pointer to CSAP descriptor structure
 *
 * @return Status code
 */
int 
snmp_release_cb(csap_p csap_descr)
{
    int layer = csap_descr->read_write_layer;
    snmp_csap_specific_data_p spec_data = 
        (snmp_csap_specific_data_p) csap_descr->layers[layer].specific_data;

    if (spec_data->pdu != NULL)
        tad_snmp_free_pdu(spec_data->pdu);
    spec_data->pdu = NULL;

    return 0;
}

/**
 * Callback for read data from media of 'snmp' CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
snmp_read_cb (csap_p csap_descr, int timeout, char *buf, size_t buf_len)
{ 
    int rc = 0; 
    int layer;

    fd_set fdset;
    int    n_fds = 0;
    int    block = 0;
    struct timeval sel_timeout; 


    snmp_csap_specific_data_p   spec_data;
    struct snmp_session       * ss;

    VERB("read callback");

    if (csap_descr == NULL) return -1;

    layer = csap_descr->read_write_layer;

    spec_data = (snmp_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 
    ss = spec_data->ss;

    FD_ZERO(&fdset);
    sel_timeout.tv_sec =  timeout / 1000000L;
    sel_timeout.tv_usec = timeout % 1000000L;

    if (spec_data->sock < 0)
        snmp_select_info(&n_fds, &fdset, &sel_timeout, &block);
    else
    {
        FD_SET(spec_data->sock, &fdset);
        n_fds = spec_data->sock + 1;
    }
    
    if (spec_data->pdu) 
        tad_snmp_free_pdu(spec_data->pdu); 
    spec_data->pdu = NULL;

    rc = select(n_fds, &fdset, 0, 0, &sel_timeout);
    VERB("%s(): CSAP %d, after select, rc %d\n",
         __FUNCTION__, csap_descr->id, rc);

    if (rc > 0) 
    { 
        size_t n_bytes = buf_len;
        
        snmp_read(&fdset);
        
        if (n_bytes < sizeof(struct snmp_pdu))
        {
            RING("In %s, buf_len %d less then sizeof struct snmp_pdu %d", 
                 __FUNCTION__, buf_len, sizeof(struct snmp_pdu));
        }
        else
            n_bytes = sizeof(struct snmp_pdu);

        if (spec_data->pdu)
        {
            memcpy(buf, spec_data->pdu, n_bytes);
            return n_bytes;
        }
        rc = 0;
    } 
    
    return rc;
}

/**
 * Callback for write data to media of 'snmp' CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
snmp_write_cb (csap_p csap_descr, char *buf, size_t buf_len)
{
    int layer;

    snmp_csap_specific_data_p   spec_data;
    struct snmp_session       * ss;
    struct snmp_pdu           * pdu = (struct snmp_pdu *) buf;

    VERB("write callback\n"); 
    UNUSED(buf_len);

    if (csap_descr == NULL) return -1;

    layer = csap_descr->read_write_layer;

    spec_data = (snmp_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 
    ss = spec_data->ss;

    if (!snmp_send(ss, pdu))
    {
        VERB("send SNMP pdu failed\n");
    } 

    return 1;
}

/**
 * Callback for write data to media of 'snmp' CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param w_buf         buffer with data to be written.
 * @param w_buf_len     length of data in w_buf.
 * @param r_buf         buffer for data to be read.
 * @param r_buf_len     available length r_buf.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
snmp_write_read_cb(csap_p csap_descr, int timeout,
                   char *w_buf, size_t w_buf_len,
                   char *r_buf, size_t r_buf_len)
{
    fd_set fdset;
    int    n_fds = 0; 
    int    block = 0;
    struct timeval sel_timeout; 

    int layer;
    int rc = 0; 

    snmp_csap_specific_data_p   spec_data;
    struct snmp_session       * ss;
    struct snmp_pdu           * pdu = (struct snmp_pdu *) w_buf;

    UNUSED(timeout);
    UNUSED(w_buf_len);
    UNUSED(r_buf_len);

    if (csap_descr == NULL) return -1;
    layer = csap_descr->read_write_layer;

    spec_data = (snmp_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 
    ss = spec_data->ss; 

    FD_ZERO(&fdset);
    sel_timeout.tv_sec  = (ss->timeout) / 1000000L;
    sel_timeout.tv_usec = (ss->timeout) % 1000000L;

    if (snmp_send(ss, pdu) == 0)
    {
        ERROR("Send PDU failed, see the reason in stderr output");
        snmp_perror("Send PDU failed");
        return 0;
    } 

    if (spec_data->sock < 0)
        snmp_select_info(&n_fds, &fdset, &sel_timeout, &block);
    else
    {
        FD_SET(spec_data->sock, &fdset);
        n_fds = spec_data->sock + 1;
    }

    if (spec_data->pdu)
        tad_snmp_free_pdu(spec_data->pdu);

    spec_data->pdu = NULL;

    rc = select(n_fds, &fdset, 0, 0, &sel_timeout);
    
    VERB("%s(): CSAP %d, after select, rc %d\n",
         __FUNCTION__, csap_descr->id, rc);

    if (rc > 0) 
    {
        snmp_read(&fdset);
        
        if (spec_data->pdu)
        {
            memcpy(r_buf, spec_data->pdu, sizeof(struct snmp_pdu));
            return sizeof(struct snmp_pdu);
        }
        rc = 0;
    }

    return rc; 
}


int 
snmp_single_check_pdus(csap_p csap_descr, asn_value *traffic_nds)
{
    char choice_label[20];
    int  rc;

    UNUSED(csap_descr);

    VERB("%s callback, CSAP # %d", __FUNCTION__, csap_descr->id); 

    if (traffic_nds == NULL)
    {
        ERROR("%s: NULL traffic nds!", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, EINVAL);
    }

    rc = asn_get_choice(traffic_nds, "pdus.0", choice_label, 
                        sizeof(choice_label));

    VERB("%s callback, got choice rc %X", __FUNCTION__, rc); 

    if (rc && rc != EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    if (rc == EASNINCOMPLVAL)
    {
        asn_value *snmp_pdu = asn_init_value(ndn_snmp_message); 
        asn_value *asn_pdu    = asn_init_value(ndn_generic_pdu); 
        
        asn_write_component_value(asn_pdu, snmp_pdu, "#snmp");
        asn_insert_indexed(traffic_nds, asn_pdu, 0, "pdus"); 

        asn_free_value(asn_pdu);
        asn_free_value(snmp_pdu);
    } 
    else if (strcmp (choice_label, "snmp") != 0)
    {
        WARN("%s callback, got unexpected choice %s", 
                __FUNCTION__, choice_label); 
        return ETADWRONGNDS;
    }
    return 0;
}

#define COMMUNITY 1

/**
 * Callback for init 'snmp' CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
snmp_single_init_cb(int csap_id, const asn_value *csap_nds, int layer)
{
    int      rc;
    char     community[COMMUNITY_MAX_LEN + 1]; 
    char     snmp_agent[100]; 
    int      timeout;
    int      version;
    size_t   v_len;

    csap_p csap_descr;

    struct snmp_session         csap_session; 
    struct snmp_session        *ss = NULL; 

    snmp_csap_specific_data_p   snmp_spec_data;
    const asn_value            *snmp_csap_spec;

    char                        security_model_name[32];
    ndn_snmp_sec_model_t        security_model;
    ndn_snmp_sec_level_t        security_level;
    ndn_snmp_priv_proto_t       priv_proto;
    ndn_snmp_auth_proto_t       auth_proto;

    uint8_t const              *auth_pass = NULL;
    uint8_t const              *priv_pass = NULL;
    char                        security_name[SNMP_MAX_SEC_NAME_SIZE + 1];
    size_t                      security_name_len;


    VERB("Init callback\n");

    if (!csap_nds || (csap_id <=0))
        return TE_EWRONGPTR;

    rc = asn_get_indexed(csap_nds, &snmp_csap_spec, layer);
    if (rc != 0)
    {
        ERROR("%s(): get csap spec layer failed %X", __FUNCTION__, rc);
        return rc;
    }
    
#if NEW_SNMP_API
    snmp_sess_init(&csap_session);
#else
    memset(&csap_session, 0, sizeof(csap_session));
#endif

    /* Timeout */
    v_len = sizeof(timeout); 
    rc = asn_read_value_field(snmp_csap_spec, &timeout,
                              &v_len, "timeout.#plain");
    if (rc == EASNINCOMPLVAL) 
        timeout = SNMP_CSAP_DEF_TIMEOUT;
    else if (rc != 0)
    {
        ERROR("%s: error reading 'timeout': %X", __FUNCTION__, rc);
        return rc;
    }
    csap_session.timeout = timeout * 1000000L;

    /* Version */
    v_len = sizeof(version); 
    rc = asn_read_value_field(snmp_csap_spec, &version,
                              &v_len, "version.#plain");
    if (rc == EASNINCOMPLVAL) 
        version = SNMP_CSAP_DEF_VERSION;
    else if (rc != 0)
    {
        ERROR("%s: error reading 'version': %X", __FUNCTION__, rc);
        return rc;
    }
    csap_session.version = version;

    /* Local port */
    v_len = sizeof(csap_session.local_port); 
    rc = asn_read_value_field(snmp_csap_spec, &csap_session.local_port,
                              &v_len, "local-port.#plain");
    if (rc == EASNINCOMPLVAL) 
        csap_session.local_port = SNMP_CSAP_DEF_LOCPORT;
    else if (rc != 0)
    {
        ERROR("%s: error reading 'local-port': %X", __FUNCTION__, rc);
        return rc;
    }

    /* Remote port */
    v_len = sizeof(csap_session.remote_port); 
    rc = asn_read_value_field(snmp_csap_spec, &csap_session.remote_port,
                              &v_len, "remote-port.#plain");
    if (csap_session.local_port == SNMP_CSAP_DEF_LOCPORT) 
    { 
        if (rc == EASNINCOMPLVAL) 
            csap_session.remote_port = SNMP_CSAP_DEF_REMPORT;
        else if (rc != 0)
        {
            ERROR("%s: error reading 'remote-port': %X", __FUNCTION__, rc);
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
    if (rc == EASNINCOMPLVAL) 
    {
        if (csap_session.local_port == SNMP_CSAP_DEF_LOCPORT)
            strcpy(snmp_agent, SNMP_CSAP_DEF_AGENT);
        else 
            snmp_agent[0] = '\0';
    }
    else if (rc != 0)
    {
        ERROR("%s: error reading 'snmp-agent': %X", __FUNCTION__, rc);
        return rc;
    }
    csap_session.peername = snmp_agent;

    /* Security model */
    v_len = sizeof(security_model_name);
    rc = asn_get_choice(snmp_csap_spec, "security", security_model_name,
                        sizeof(security_model_name));
    if (rc == EASNINCOMPLVAL)
        security_model = NDN_SNMP_SEC_MODEL_DEFAULT;
    else if (rc != 0)
    {
        ERROR("%s: error reading 'security': %X", __FUNCTION__, rc);
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
        return ENOENT;
    }
   
    /* Community-based security model */
    if (security_model == NDN_SNMP_SEC_MODEL_V2C)
    {
        v_len = sizeof(community);
        rc = asn_read_value_field(snmp_csap_spec, community,
                                  &v_len, "security.#v2c.community");
        if (rc == EASNINCOMPLVAL) 
            strcpy(community, SNMP_CSAP_DEF_COMMUNITY);
        else if (rc != 0)
        {
            ERROR("%s: error reading community: %X", __FUNCTION__, rc);
            return rc;
        }

        csap_session.securityModel = SNMP_SEC_MODEL_SNMPv2c;
        csap_session.community = community;
        csap_session.community_len = strlen(community);
    }

    /* User-based security model */
    else if (security_model == NDN_SNMP_SEC_MODEL_USM)
    {
        /* Security name */
        security_name_len = sizeof(security_name);
        rc = asn_read_value_field(snmp_csap_spec, security_name,
                                  &security_name_len, "security.#usm.name");
        if (rc == EASNINCOMPLVAL)
        {
            ERROR("%s: there is no securityName provided",  __FUNCTION__);
            return rc;
        }
        if (rc == TE_ESMALLBUF)
        {
            ERROR("%s: securityName is too long (max %d is valid)",
                  __FUNCTION__, SNMP_MAX_SEC_NAME_SIZE);
            return rc;
        }
        if (rc != 0)
        {
            ERROR("%s: error reading securityName, rc=%X",
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
        if (rc == EASNINCOMPLVAL)
            security_level = NDN_SNMP_SEC_LEVEL_NOAUTH;
        else if (rc != 0)
        {
            ERROR("%s: error reading securityLevel: %X", __FUNCTION__, rc);
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
            if (rc == EASNINCOMPLVAL)
                auth_proto = NDN_SNMP_AUTH_PROTO_DEFAULT;
            else if (rc != 0)
            {
                ERROR("%s: error reading 'auth-protocol': %X",
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
                ERROR("%s: error reading 'auth-pass': %X",
                      __FUNCTION__, rc);
                return rc;
            }

            /* Key generation */
            csap_session.securityAuthKeyLen = 
                sizeof(csap_session.securityAuthKey);
            if (generate_Ku(csap_session.securityAuthProto,
                            csap_session.securityAuthProtoLen,
                            (unsigned char *)auth_pass, strlen(auth_pass),
                            csap_session.securityAuthKey,
                            &csap_session.securityAuthKeyLen)
                    != SNMPERR_SUCCESS)
            {
                ERROR("%s: failed to generate a key from authentication "
                      "passphrase: %s", __FUNCTION__, 
                      snmp_api_errstring(snmp_errno));
                return ETADLOWER;
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
            if (rc == EASNINCOMPLVAL)
                priv_proto = NDN_SNMP_PRIV_PROTO_DEFAULT;
            else if (rc != 0)
            {
                ERROR("%s: error reading 'priv-protocol': %X",
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
                    return ETADLOWER;
#endif
                    break;

                default:
                    assert(0);
            }

            rc = asn_get_field_data(snmp_csap_spec, &priv_pass,
                                    "security.#usm.priv-pass");
            if (rc != 0)
            {
                ERROR("%s: error reading 'priv-pass': %X",
                      __FUNCTION__, rc);
                return rc;
            }

            /* Key generation */
            csap_session.securityPrivKeyLen =
                sizeof(csap_session.securityPrivKey);
            if (generate_Ku(csap_session.securityAuthProto,
                            csap_session.securityAuthProtoLen,
                            (unsigned char *)priv_pass, strlen(priv_pass),
                            csap_session.securityPrivKey,
                            &csap_session.securityPrivKeyLen)
                    != SNMPERR_SUCCESS)
            {
                ERROR("%s: failed to generate a key from privacy "
                      "passphrase: %s", __FUNCTION__,
                      snmp_api_errstring(snmp_errno));
                return ETADLOWER;
            }
        }
    }

    csap_descr = csap_find(csap_id);
    snmp_spec_data = malloc(sizeof(snmp_csap_specific_data_t));
    if (snmp_spec_data == NULL)
    {
        ERROR("%s: malloc failed", __FUNCTION__);
        return ENOMEM;
    }
    
    if (csap_descr->check_pdus_cb == NULL)
        csap_descr->check_pdus_cb = snmp_single_check_pdus;

    csap_descr->write_cb         = snmp_write_cb; 
    csap_descr->read_cb          = snmp_read_cb; 
    csap_descr->write_read_cb    = snmp_write_read_cb; 
    csap_descr->release_cb       = snmp_release_cb;
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 2000000; 

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
#if NEW_SNMP_API
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
        VERB("%s(): CSAP %d, sock = %d", __FUNCTION__, csap_id,
             snmp_spec_data->sock);
#else
        ss = snmp_open(&csap_session); 
#endif
    } while(0);

    if (ss == NULL)
    {
        ERROR("%s: open session or transport error: %s",
              __FUNCTION__, snmp_api_errstring(snmp_errno));

        free(snmp_spec_data);
        return ETADLOWER;
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

    csap_descr->layers[layer].specific_data = snmp_spec_data;
    snmp_spec_data->ss  = ss;
    snmp_spec_data->pdu = 0;

    return 0;
}

/**
 * Callback for destroy 'snmp' CSAP layer  if single in stack.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed 
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
snmp_single_destroy_cb (int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    snmp_csap_specific_data_p spec_data = 
        (snmp_csap_specific_data_p) csap_descr->layers[layer].specific_data;

    VERB("Destroy callback, id %d\n", csap_id);
    if (spec_data->pdu != NULL)
        tad_snmp_free_pdu(spec_data->pdu);

    if (spec_data->ss)
    {
        struct snmp_session *s = spec_data->ss;

        snmp_close(s);
    }
    return 0;
}



