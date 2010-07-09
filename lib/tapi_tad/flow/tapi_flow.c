/*
 * Test qos/flow_check
 * QoS Classifiers creating/adjusting/deleting test
 *
 * Copyright (C) 2010 OKTET Labs, St.-Petersburg, Russia
 *
 * $Id: flow_check.c 64234 2010-05-11 16:21:28Z kam $
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 */
#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <stdio.h>
#include <ctype.h>

#include "asn_impl.h"
#include "asn_usr.h"
#include "ndn.h"
#include "ndn_internal.h"
#include "ndn_ipstack.h"
#include "ndn_eth.h"
#include "ndn_flow.h"
#include "tapi_cfg.h"
#include "tapi_tad.h"
#include "tapi_ndn.h"
#include "tapi_flow.h"
#include "logger_api.h"

#include "tapi_test.h"

#define TAPI_FLOW_PREFIX "FLOW."

#define TAPI_FLOW_PEER_LINK_FMT "cfg:/local:/peer:%s:"
#define TAPI_FLOW_PLAIN_VAL_FMT "plain:%s"
#define TAPI_FLOW_QUOTED_FMT    "'%s'"

te_errno
tapi_flow_conf_get(int argc, char **argv,
                   tapi_flow_t *flow)
{
    int rc;
    int syms;

    RING("%s() started", __FUNCTION__);

    /* To make sure all links are resolved */
    if ((argv = tapi_flow_preprocess_args(argc, argv)) == NULL)
    {
        TEST_FAIL("Failed to preprocess test parameters");
    }

    /* Create root configuration node */
    if ((rc = asn_parse_value_text("{ endpoint {}, traffic {} }",
                                   ndn_flow, &flow->flow_spec,
                                   &syms)) != 0)
    {
        ERROR("Cannot initialize FLOW root node, rc=%r", rc);
        return rc;
    }

    if ((rc = tapi_asn_params_get(argc, argv, TAPI_FLOW_PREFIX,
                                  ndn_flow, flow->flow_spec)) != 0)
    {
        ERROR("Failed to process FLOW params, rc=%r", rc);
    }

    return rc; 
}

void
tapi_flow_init(tapi_flow_t *flow)
{
    RING("%s() started", __FUNCTION__);

    flow->flow_spec = NULL;

    SLIST_INIT(&flow->ep_list);
    SLIST_INIT(&flow->traffic_list);
}

tapi_flow_ep *
tapi_flow_find_ep(tapi_flow_t *flow, const char *name)
{
    tapi_flow_ep *ep;

    RING("%s() started", __FUNCTION__);

    for (ep = SLIST_FIRST(&flow->ep_list);
         ep != NULL;
         ep = SLIST_NEXT(ep, link))
    {
        if (strcmp(ep->name, name) == 0)
            break;
    }

    return ep;
}

tapi_flow_traffic *
tapi_flow_find_traffic(tapi_flow_t *flow, const char *name)
{
    tapi_flow_traffic *traffic;

    RING("%s() started", __FUNCTION__);

    for (traffic = SLIST_FIRST(&flow->traffic_list);
         traffic != NULL;
         traffic = SLIST_NEXT(traffic, link))
    {
        if (strcmp(traffic->name, name) == 0)
            break;
    }

    return traffic;
}

/* See description in tapi_flow.h */
te_errno
tapi_flow_prepare_endpoints(tapi_flow_t *flow)
{
    te_errno        rc = 0;

    /* endpoints */
    tapi_flow_ep   *ep;
    asn_value      *endpoints = NULL;
    asn_value      *ep_desc = NULL;
    asn_value      *layers = NULL;
    int             ep_count = 0;
    int             i;
    int             syms;
    char            buf[4096];

    RING("%s() started", __FUNCTION__);

    /* get list of endpoints */
    rc = asn_get_descendent(flow->flow_spec,
                            &endpoints,
                            "endpoint");
    if (rc != 0 && rc != TE_EASNINCOMPLVAL)
    {
        TEST_FAIL("Failed to get endpoints list from ASN value");
    }

    if (rc == TE_EASNINCOMPLVAL)
    {
        /* No endpoints => nothing to do */
        RING("%s: no endpoints to setup: %r",
             __FUNCTION__, rc);
        return rc;
    }

    if ((ep_count = asn_get_length(endpoints, "")) == 0)
    {
        /* No endpoints => nothing to do */
        RING("%s: no endpoints to setup peer to",
             __FUNCTION__);
        return rc;
    }

    RING("%s: found %d endpoints", __FUNCTION__, ep_count);
    for (i = 0; i < ep_count; i++)
    {
        RING("%s: process %d endpoint", __FUNCTION__, i);

        /* get endpoint value */
        rc = asn_get_indexed(endpoints, &ep_desc, i, "");
        if (rc != 0)
        {
            ERROR("Failed get endpoint #%d from ASN, rc=%r", i, rc);
            return rc;
        }

        if ((ep = (tapi_flow_ep *)calloc(1, sizeof(tapi_flow_ep))) == NULL)
        {
            ERROR("Failed to allocate memory for endpoint structure");
            return rc;
        }

        /* Get endpoint name */
        if ((rc = asn_read_string(ep_desc, &ep->name, "name")) != 0)
        {
            ERROR("Failed to get endpoint name, rc=%r", rc);
            return rc;
        }

        /* Get endpoint TA */
        if ((rc = asn_read_string(ep_desc, &ep->ta, "ta")) != 0)
        {
            ERROR("Failed to get endpoint TA, rc=%r", rc);
        }

        RING("%s: process endpoint '%s' on agent '%s'",
             __FUNCTION__, ep->name, ep->ta);

        /* Get connection vlan id */
        if ((rc = asn_get_descendent(ep_desc, &layers, "layers")) != 0)
        {
            TEST_FAIL("Failed to read get endpoint %s CSAP layers, rc=%r",
                      ep->name, rc);
        }

        if ((rc = asn_parse_value_text("{ layers {} }", &ndn_csap_spec_s,
                                       &ep->csap_spec, &syms)) != 0)
        {
            ERROR("Failed to initialise empty csap template: rc=%r", rc);
            return rc;
        }

        if ((rc = asn_write_component_value(ep->csap_spec, layers,
                                            "layers")) != 0)
        {
            ERROR("Failed to complete CSAP asn value for %s endpoint: rc=%r",
                  ep->name, rc);
        }

        memset(buf, 0, 4096);
        asn_sprint_value(ep->csap_spec, buf, sizeof(buf), 4);
        RING("%s() csap specification:\n%s", __FUNCTION__, buf);

        /* Add endpoint to the list */
        SLIST_INSERT_HEAD(&flow->ep_list, ep, link);
    }

    return 0;
}

static tapi_flow_ep *
tapi_flow_ep_copy(tapi_flow_t *flow, char *ep_name, char *new_ep_name)
{
    tapi_flow_ep *ep = tapi_flow_find_ep(flow, ep_name);
    tapi_flow_ep *new_ep = NULL;

    RING("%s() started", __FUNCTION__);

    if (ep == NULL)
    {
        ERROR("Failed to duplicate endpoint structure");
        return NULL;
    }

    new_ep = (tapi_flow_ep *)calloc(1, sizeof(tapi_flow_ep));
    if (new_ep == NULL)
    {
        ERROR("Failed to allocate memory to duplicate endpoint structure");
        return NULL;
    }

    new_ep->name = strdup(new_ep_name);
    new_ep->value = ep->value;
    new_ep->ta = strdup(ep->ta);
    new_ep->csap_spec = ep->csap_spec;

    /* Add endpoint to the list */
    SLIST_INSERT_HEAD(&flow->ep_list, new_ep, link);

    return new_ep;
}

te_errno
tapi_flow_setup_endpoints(tapi_flow_t *flow)
{
    int rc;
    tapi_flow_ep *ep;

    RING("%s() started", __FUNCTION__);

    for (ep = SLIST_FIRST(&flow->ep_list);
         ep != NULL;
         ep = SLIST_NEXT(ep, link))
    {
        /* Create session  */
        if ((rc = rcf_ta_create_session(ep->ta, &ep->sid)) != 0)
        {
            ERROR("Failed to create session: %r", rc);
            return rc;
        }

        if ((rc = tapi_flow_csap_spec_to_stack(ep->csap_spec,
                                               &ep->csap_desc)) != 0)
        {
            ERROR("Failed to parse CSAP description");
            return rc;
        }

        if ((rc = tapi_tad_csap_create(ep->ta, ep->sid,
                                       ep->csap_desc,
                                       ep->csap_spec,
                                       &ep->csap_id)) != 0)
        {
            ERROR("Failed to create send CSAP '%s'", ep->csap_desc);
            return rc;
        }
    }

    return 0;
}


/* See description in tapi_flow.h */
te_errno
tapi_flow_prepare_traffic(tapi_flow_t *flow)
{
    te_errno           rc = 0;

    /* endpoints */
    tapi_flow_traffic *traffic;
    asn_value         *traffic_list = NULL;
    asn_value         *traffic_desc = NULL;
    asn_value         *send_pdu = NULL;
    asn_value         *recv_pdu = NULL;
    char              *snd_ep = NULL;
    char              *rcv_ep = NULL;
    char              *rcv_base_ep = NULL;
    int                list_size = 0;
    int                syms;
    int                i;

    RING("%s() started", __FUNCTION__);

    /* get list of traffic descriptions */
    rc = asn_get_descendent(flow->flow_spec, &traffic_list, "traffic");
    if (rc != 0 && rc != TE_EASNINCOMPLVAL)
    {
        TEST_FAIL("Failed to get traffic list from ASN value");
    }

    if (rc == TE_EASNINCOMPLVAL)
    {
        /* No traffic descriptions => nothing to do */
        RING("%s: no traffic entries to process: %r",
             __FUNCTION__, rc);
        return rc;
    }

    list_size = asn_get_length(traffic_list, "");
    if (list_size == 0)
    {
        /* No traffic descriptions => nothing to do */
        RING("%s: no traffic descriptions to configure",
             __FUNCTION__);
        return 0;
    }

    for (i = 0; i < list_size; i++)
    {
        /* get traffic value */
        rc = asn_get_indexed(traffic_list, &traffic_desc, i, "");
        if (rc != 0)
        {
            ERROR("Failed get traffic #%d from ASN, rc=%r", i, rc);
            return rc;
        }

        if ((traffic = (tapi_flow_traffic *)
                 calloc(1, sizeof(tapi_flow_traffic))) == NULL)
        {
            ERROR("Failed to allocate memory for traffic structure");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }

        /* Get traffic name */
        if ((rc = asn_read_string(traffic_desc, &traffic->name, "name")) != 0)
        {
            ERROR("Failed to get traffic entry name, rc=%r", rc);
            return rc;
        }

        /* Process send template */

        do {
            /* Get traffic sender */
            if ((rc = asn_read_string(traffic_desc, &snd_ep, "src")) != 0)
            {
                ERROR("Failed to get traffic source endpoint name, rc=%r",
                      rc);
                break;
            }

            if ((traffic->snd = tapi_flow_find_ep(flow, snd_ep)) == NULL)
            {
                ERROR("Failed to find traffic source endpoint %s", snd_ep);
                return TE_RC(TE_TAPI, TE_EINVAL);
            }

            /* Get send template pdu */
            if ((rc = asn_get_descendent(traffic_desc, &send_pdu,
                                         "send")) != 0)
            {
                WARN("Failed to read send template, rc=%r", rc);
                break;
            }

            if ((rc = asn_parse_value_text("{ pdus {} }",
                                           &ndn_traffic_template_s,
                                           &traffic->send_tmpl, &syms)) != 0)
            {
                ERROR("Failed to initialise empty csap send template: rc=%r",
                      rc);
                return rc;
            }

            if ((rc = asn_write_component_value(traffic->send_tmpl, send_pdu,
                                                "pdus")) != 0)
            {
                ERROR("Failed to commit pdu to csap send template: rc=%r",
                      rc);
                return rc;
            }
        } while (0);

        do {
            /* Get traffic receiver */
            if ((rc = asn_read_string(traffic_desc, &rcv_ep, "dst")) != 0)
            {
                WARN("Failed to get traffic source endpoint name, rc=%r", rc);
                break;
            }

            if ((traffic->rcv = tapi_flow_find_ep(flow, rcv_ep)) == NULL)
            {
                ERROR("Failed to find traffic source endpoint %s", rcv_ep);
                return TE_RC(TE_TAPI, TE_EINVAL);
            }

            /* Get receive pattern */
            if ((rc = asn_get_descendent(traffic_desc, &recv_pdu, "recv")) != 0)
            {
                WARN("Failed to read receive pattern, rc=%r", rc);
                break;
            }

            if ((rc = asn_parse_value_text("{{ pdus {} }}",
                                           &ndn_traffic_pattern_s,
                                           &traffic->recv_ptrn, &syms)) != 0)
            {
                ERROR("Failed to initialise empty csap template: rc=%r", rc);
                return rc;
            }

            if ((rc = asn_write_component_value(traffic->recv_ptrn, recv_pdu,
                                                "0.pdus")) != 0)
            {
                ERROR("Failed to commit CSAP asn receive pattern: rc=%r", rc);
                return rc;
            }

            /* Create base traffic receiver */
            rcv_base_ep = te_sprintf("%s-base", rcv_ep);
            if (tapi_flow_find_ep(flow, rcv_base_ep) == NULL)
            {
                if ((traffic->rcv_base =
                     tapi_flow_ep_copy(flow, rcv_ep, rcv_base_ep)) == NULL)
                {
                    ERROR("Failed to duplicate receive endpoint "
                          "for base matching, rc=%r", rc);
                    return rc;
                }
            }

            /* Generate base pattern for receive matching */
            if ((rc = tapi_flow_gen_base_ptrn(traffic->recv_ptrn,
                                              &traffic->recv_base_ptrn)) != 0)
            {
                ERROR("Failed to make base receive pattern: rc=%r", rc);
                return rc;
            }
        } while (0);

        if ((rc = asn_read_int32(traffic_desc, &traffic->plen,
            "plen.#plain")) != 0)
        {
            traffic->plen = 0;
        }

        if ((rc = asn_read_int32(traffic_desc, &traffic->count,
            "count.#plain")) != 0)
        {
            traffic->count = 1;
        }
        else
        {
            if ((rc =tapi_tad_add_iterator_for(traffic->send_tmpl,
                                               1, traffic->count, 1)) != 0)
            {
                ERROR("Failed to add FOR iterator to send template: rc=%r",
                      rc);
                return rc;
            }
        }

        SLIST_INSERT_HEAD(&flow->traffic_list, traffic, link);
    }

    return 0;
}

#define TAPI_FLOW_PREPROCESS_BUF_SIZE 1024

#define TAPI_CFG_RANDOM_PREFIX  "/random:"

#define TAPI_CFG_RANDOM_PORT_MIN    10000
#define TAPI_CFG_RANDOM_PORT_MAX    60000

#define TAPI_CFG_RANDOM_MCAST_0_MIN 224
#define TAPI_CFG_RANDOM_MCAST_0_MAX 254

#define TAPI_CFG_RANDOM_IP_0_MIN    64
#define TAPI_CFG_RANDOM_IP_0_MAX    126

#define TAPI_CFG_RANDOM_IP_X_MIN    1
#define TAPI_CFG_RANDOM_IP_X_MAX    254

static inline char *tapi_cfg_link_random(char *link)
{
    char *type = strstr(link, TAPI_CFG_RANDOM_PREFIX);
    if (type != NULL)
        type += strlen(TAPI_CFG_RANDOM_PREFIX);
    if (strcmp(type, "port") == 0)
    {
        return te_sprintf("plain:%d",
                          rand_range(TAPI_CFG_RANDOM_PORT_MIN,
                                     TAPI_CFG_RANDOM_PORT_MAX));
    }
    else if ((strcmp(type, "ip") == 0) || (strcmp(type, "mcast") == 0))
    {
        int i;
        uint8_t addr[4];

        if (strcmp(type, "ip") == 0)
        {
            addr[0] = rand_range(TAPI_CFG_RANDOM_IP_0_MIN,
                                 TAPI_CFG_RANDOM_IP_0_MAX);
        }
        else
        {
            addr[0] = rand_range(TAPI_CFG_RANDOM_MCAST_0_MIN,
                                 TAPI_CFG_RANDOM_MCAST_0_MAX);
        }
        for (i = 1; i < 4; i++)
        {
            addr[i] = rand_range(TAPI_CFG_RANDOM_IP_0_MIN,
                                 TAPI_CFG_RANDOM_IP_0_MAX);
        }

        return te_sprintf("plain:'%02x %02x %02x %02x'H",
                          addr[0], addr[1], addr[2], addr[3]);
    }
    else if (strcmp(type, "mac") == 0)
    {
        int i;
        uint8_t mac[6];
        for (i = 0; i < 6; i++)
        {
            mac[i] = rand_range(TAPI_CFG_RANDOM_IP_0_MIN,
                                TAPI_CFG_RANDOM_IP_0_MAX);
        }

        return te_sprintf("plain:'%02x %02x %02x %02x %02x %02x'H",
                          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    return NULL;
}

/**
 * Resolve configurator link into its value.
 *
 * @param link     Link to string to dereference
 *
 * @return Dereferenced link value from the configuration tree.
 */
static inline char *tapi_cfg_link_dereference(char *link)
{
    char           *cur = link + strlen(TAPI_CFG_LINK_PREFIX);
    cfg_handle      handle;
    cfg_obj_descr   descr;
    cfg_val_type    val_type;
    char           *value = NULL;
    int             rc = 0;

    RING("%s() started", __FUNCTION__);
    if (strstr(link, TAPI_CFG_RANDOM_PREFIX) != NULL)
    {
        return tapi_cfg_link_random(link);
    }

    if ((rc = cfg_find_fmt(&handle, "%s", cur)) != 0)
    {
        ERROR("Failed to get configurator object: rc=%r", rc);
        return NULL;
    }

    if ((rc = cfg_get_object_descr(handle, &descr)) != 0)
    {
        ERROR("Failed to get configurator object description: rc=%r", rc);
        return NULL;
    }

    val_type = descr.type;

    VERB("Get '%s' of type %d", cur, val_type);

    if (val_type == CVT_INTEGER)
    {
        int int_val = 0;
        if ((rc = cfg_get_instance_fmt(&val_type, &int_val, cur)) != 0)
        {
            ERROR("Failed to get configurator object %s: rc=%r", cur, rc);
            return NULL;
        }
        value = te_sprintf("%d", int_val);
    }
    else if (val_type == CVT_ADDRESS)
    {
        struct sockaddr *addr = NULL;

        if ((rc = cfg_get_instance_fmt(&val_type, &addr, cur)) != 0)
        {
            ERROR("Failed to get configurator object %s: rc=%r", cur, rc);
            return NULL;
        }

        if ((strstr(link, "address") != NULL) || 
            (strstr(link, "net_addr") != NULL))
        {
            uint8_t *ip =
                (uint8_t *)&((struct sockaddr_in *)addr)->sin_addr.s_addr;
            value = te_sprintf("'%02hhx %02hhx %02hhx %02hhx'H",
                               ip[0], ip[1], ip[2], ip[3]);
        }
        else if ((strstr(link, "mac") != NULL) || 
                 (strstr(link, "link_addr") != NULL))
        {
            uint8_t *mac = (uint8_t *)addr->sa_data;
            value = te_sprintf("'%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx'H",
                               mac[0], mac[1], mac[2],
                               mac[3], mac[4], mac[5]);
        }

        VERB("Got address %s, sa_family=%d", value, addr->sa_family);
    }
    else if ((val_type == CVT_STRING) || (val_type == CVT_NONE))
    {
        if ((rc = cfg_get_instance_fmt(NULL, &value, cur)) != 0)
        {
            ERROR("Failed to get configurator object %s: rc=%r", cur, rc);
            return NULL;
        }
    }

    VERB("%s(): %s=%s", __FUNCTION__, link, value);
    return value;
}


/**
 * Extract configurator link from the buffer.
 *
 * @param buf   String started with configurator link
 *
 * @return      Configuator link name or NULL, if buffer does not start with
 *              configurator link prefix.
 */
static inline char *
tapi_cfg_extract_link(char *buf)
{
    char *p = buf + strlen(TAPI_CFG_LINK_PREFIX);

    if ((buf == NULL) || (!tapi_is_cfg_link(buf)))
        return NULL;

    while (*p == '/' || *p == ':' || *p == '-' || *p == '_' || isalpha(*p))
        p++;

    if ((p = strndup(buf, p - buf)) == NULL)
    {
        ERROR("Failed to allocate memory for cfg link");
        return NULL;
    }
    VERB("Found link: %s", p);

    return p;
}

/**
 * Preprocess textual flow description and
 * interpret values in single quotes.
 *
 * @param param     Textual flow description
 *
 * @return          Preprocessed flow description which could be parsed by
 *                  asn_parse_text(), or NULL if something fails
 */
char *
tapi_flow_preprocess_quotes(const char *param)
{
    char *open_q   = NULL;          /* Opening quote */
    char *close_q  = NULL;          /* Closing quote */
    char *src      = (char *)param;
    int   buf_size = (strlen(param) * 2) + 1;
    char *buf      = malloc(buf_size);
    char *dst      = buf;
    char *link     = NULL;
    char *value    = NULL;
    int   unused   = strlen(param);

    RING("%s() started", __FUNCTION__);

    if (buf == NULL)
    {
        ERROR("Failed to allocate memory to process flow spec");
        return NULL;
    }

    while (src != NULL)
    {
        /* Find opening quote */
        if ((open_q = strchr(src, '\'')) == NULL)
        {
            VERB("copy the rest of buffer: %s", src);
            strcpy(dst, src);
            break;
        }

        /* Copy text before open quote */
        memcpy(dst, src, open_q - src);
        dst += open_q - src;

        /* Find closing quote */
        if ((close_q = strchr(open_q + 1, '\'')) == NULL)
        {
            VERB("copy the rest of buffer: %s", src);
            strcpy(dst, open_q);
            break;
        }

        link = strndup(open_q + 1, close_q - open_q - 1);
        src = open_q + strlen(link) + 2;
        value = link;

        /* Check if quoted value is has '/' symbol */
        if (strchr(link, '/') != NULL)
        {
            link = te_sprintf(TAPI_FLOW_PEER_LINK_FMT, link);
            value = tapi_cfg_link_dereference(link);
        }

        /* Check if quoted value already has 'plain:' prefix */
        if ((open_q > param) && (*(open_q - 1) != ':'))
        {
            value = te_sprintf(TAPI_FLOW_PLAIN_VAL_FMT, value);
        }
        else /* Keep value quoted */
        {
            value = te_sprintf(TAPI_FLOW_QUOTED_FMT, value);
        }

        VERB("Link: %s = %s", link, value);

        if (strlen(value) > 2 + strlen(link) + unused)
        {
            buf_size += strlen(value) - strlen(link) - unused;
            unused = 0;

            if ((buf = realloc(buf, buf_size)) == NULL)
            {
                ERROR("Failed to allocate memory for preprocessed flow");
                return NULL;
            }
        }
        else
        {
            unused += strlen(link) - strlen(value);
        }
        strcpy(dst, value);

        dst += strlen(value);
        free(value);
        free(link);
    }

    RING("%s: Preprocessed buf:\n%s", __FUNCTION__, buf);
    return buf;
}


/**
 * Preprocess textual flow description and dereference configurator links.
 *
 * @param param     Textual flow description
 *
 * @return              Preprocessed flow description which could be parsed by
 *                      asn_parse_text(), or NULL if something fails
 */
char *
tapi_flow_preprocess_links(const char *param)
{
    char *src      = (char *)param;
    int   buf_size = strlen(param) + 1;
    char *buf      = malloc(buf_size);
    char *dst      = buf;
    char *link     = NULL;
    char *value    = NULL;
    int   unused   = 0;

    RING("%s() started", __FUNCTION__);

    if (buf == NULL)
    {
        ERROR("Failed to allocate memory to process flow spec");
        return NULL;
    }

    while (param != NULL)
    {
        if ((src = strstr(param, TAPI_CFG_LINK_PREFIX)) == NULL)
        {
            VERB("copy the rest of buffer: %s", param);
            strcpy(dst, param);
            break;
        }

        memcpy(dst, param, src - param);
        dst += src - param;

        link  = tapi_cfg_extract_link(src);
        value = tapi_cfg_link_dereference(link);

        VERB("Link: %s = %s", link, value);
        if (strlen(value) > strlen(link) + unused)
        {
            buf_size += strlen(value) - strlen(link) - unused;
            unused = 0;

            if ((buf = realloc(buf, buf_size)) == NULL)
            {
                ERROR("Failed to allocate memory for preprocessed flow");
                return NULL;
            }
        }
        else
        {
            unused += strlen(link) - strlen(value);
        }
        strcpy(dst, value);

        dst += strlen(value);
        param = src + strlen(link);
        free(value);
        free(link);
    }

    RING("%s: Preprocessed buf:\n%s", __FUNCTION__, buf);
    return buf;
}

#define TAPI_TAD_CSAP_DESC_LEN_MAX  128

/**
 * Parse CSAP layer names and prepares CSAP stack specification.
 *
 * @param spec  ASN specification of CSAP to process.
 * @param stack Returned CSAP stack specification (for ex.: 'udp.ip4.eth').
 *
 * @return      0 on success, or an error otherwise.
 */
te_errno
tapi_flow_csap_spec_to_stack(asn_value *spec, char **stack)
{
    asn_value *layer = NULL;
    int        i;
    int        rc    = 0;
    char      *buf   = calloc(1, TAPI_TAD_CSAP_DESC_LEN_MAX);
    char       log_buf[4096];
    int        len   = 0;

    RING("%s() started", __FUNCTION__);

    memset(log_buf, 0, 4096);
    asn_sprint_value(spec, log_buf, sizeof(buf), 4);
    RING("%s(): process csap:\n%s", __FUNCTION__, log_buf);

    if (buf == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    for (i = 0; rc == 0; i++)
    {
        RING("%s(): try to get %d layer", __FUNCTION__, i);
        if ((rc = asn_get_indexed(spec, &layer, i, "layers")) != 0)
        {
            RING("%s(): failed to get %d layer", __FUNCTION__, i);
            break;
        }
        if ((rc = asn_get_choice_value(layer, &layer, NULL, NULL)) != 0)
        {
            ERROR("%s(): failed to get %d layer choice", __FUNCTION__, i);
            return rc;
        }
        RING("%s: layer %d - %s", __FUNCTION__, i, layer->name);
        len += sprintf(buf + len, "%s%s", (i > 0) ? "." : "", layer->name);
    }

    if (stack != NULL)
        *stack = buf;

    RING("%s() returns %s", __FUNCTION__, buf);

    return 0;
}

#define TAPI_FLOW_ADDR_LEN_MAX 16

/**
 * Prepare base receive pattern to match that sent packets are received.
 *
 * @param rcv_ptrn      Receive pattern to process
 * @param base_ptrn_p   Pointer to prepared base receive pattern
 *
 * @return      0 on success, or an error otherwise.
 */
te_errno
tapi_flow_gen_base_ptrn(asn_value *rcv_ptrn, asn_value **base_ptrn_p)
{
    asn_value *base_ptrn                    = NULL;
    asn_value *layer                        = NULL;
    asn_value *tmp_pdu                      = NULL;
    int        rc                           = 0;
    int        value                        = 0;
    uint8_t    addr[TAPI_FLOW_ADDR_LEN_MAX] = {0, };
    int        addr_len;
    int        num;
    int        i;
    uint8_t    buf[4096];

    RING("%s() started", __FUNCTION__);

    if (buf == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    if ((rc = asn_parse_value_text("{ { pdus {} } }",
                                   ndn_traffic_pattern,
                                   &base_ptrn, &num)) != 0)
    {
        ERROR("Failed to prepare empty receive pattern");
        return rc;
    }

    VERB("%s() started", __FUNCTION__);
    for (i = 0;; i++)
    {
        VERB("%s(): try to get %d layer", __FUNCTION__, i);
        if ((rc = asn_get_indexed(rcv_ptrn, &layer, i, "0.pdus")) != 0)
        {
            VERB("%s(): failed to get %d layer", __FUNCTION__, i);
            break;
        }
        if ((rc = asn_get_choice_value(layer, &layer, NULL, NULL)) != 0)
        {
            ERROR("%s(): failed to get %d layer choice", __FUNCTION__, i);
            return rc;
        }
        VERB("%s: layer %d - %s", __FUNCTION__, i, layer->name);

        if ((strcmp(layer->name, "udp") == 0) ||
            (strcmp(layer->name, "tcp") == 0))
        {
            if (strcmp(layer->name, "udp") == 0)
            {
                rc = tapi_tad_tmpl_ptrn_add_layer(&base_ptrn, TRUE,
                                                  ndn_udp_header, "#udp",
                                                  &tmp_pdu);
            } else {
                rc = tapi_tad_tmpl_ptrn_add_layer(&base_ptrn, TRUE,
                                                  ndn_tcp_header, "#tcp",
                                                  &tmp_pdu);
            }
            if (rc != 0)
            {
                ERROR("Failed to add %s layer to base pattern: rc=%r",
                      layer->name, rc);
                return rc;
            }

            if ((rc = asn_read_int32(layer, &value,
                                     "dst-port.#plain")) == 0)
            {
                if ((rc = asn_write_int32(tmp_pdu, value,
                                          "dst-port.#plain")) != 0)
                {
                    ERROR("Failed to write 'dst-port' field to "
                          "%s layer: rc=%r", layer->name, rc);
                    return rc;
                }
            }

            if ((rc = asn_read_int32(layer, &value,
                                     "src-port.#plain")) == 0)
            {
                if ((rc = asn_write_int32(tmp_pdu, value,
                                          "src-port.#plain")) != 0)
                {
                    ERROR("Failed to write 'src-port' field to "
                          "%s layer: rc=%r", layer->name, rc);
                    return rc;
                }
            }
        } else if ((strcmp(layer->name, "ip4") == 0) ||
                   (strcmp(layer->name, "eth") == 0))
        {
            if (strcmp(layer->name, "ip4") == 0)
            {
                rc = tapi_tad_tmpl_ptrn_add_layer(&base_ptrn, TRUE,
                                                  ndn_ip4_header, "#ip4",
                                                  &tmp_pdu);
            } else {
                rc = tapi_tad_tmpl_ptrn_add_layer(&base_ptrn, TRUE,
                                                  ndn_eth_header, "#eth",
                                                  &tmp_pdu);
            }
            if (rc != 0)
            {
                ERROR("Failed to add %s layer to base pattern: rc=%r",
                      layer->name, rc);
                return rc;
            }

            if ((addr_len = asn_get_length(layer, "src-addr.#plain")) > 0)
            {
                VERB("Length of src-addr: %d bytes", addr_len);

                if ((rc = asn_read_value_field(layer, addr, &addr_len,
                                               "src-addr.#plain")) != 0)
                {
                    ERROR("Failed to get src-addr from receive pattern");
                    return rc;
                }

                if ((rc = asn_write_value_field(tmp_pdu, addr, addr_len,
                                               "src-addr.#plain")) != 0)
                {
                    ERROR("Failed to get src-addr from receive pattern");
                    return rc;
                }
            }

            if ((addr_len = asn_get_length(layer, "dst-addr.#plain")) > 0)
            {
                VERB("Length of dst-addr: %d bytes", addr_len);

                if ((rc = asn_read_value_field(layer, addr, &addr_len,
                                               "dst-addr.#plain")) != 0)
                {
                    ERROR("Failed to get src-addr from receive pattern");
                    return rc;
                }

                if ((rc = asn_write_value_field(tmp_pdu, addr, addr_len,
                                               "dst-addr.#plain")) != 0)
                {
                    ERROR("Failed to get src-addr from receive pattern");
                    return rc;
                }
            }

            if (strcmp(layer->name, "ip4") == 0)
            {
                if ((rc = asn_read_int32(layer, &value,
                                          "protocol.#plain")) == 0)
                {
                    if ((rc = asn_write_int32(tmp_pdu, value,
                                              "protocol.#plain")) != 0)
                    {
                        ERROR("Failed to write ip protocol to base pattern");
                        return rc;
                    }
                }
            }
            else
            {
                if ((rc = asn_read_int32(layer, &value,
                                         "ether-type.#plain")) == 0)
                {
                    if ((rc = asn_write_int32(tmp_pdu, value,
                                              "ether-type.#plain")) != 0)
                    {
                        ERROR("Failed to write ether-type to base pattern");
                        return rc;
                    }
                }
            }
        }
    }

    memset(buf, 0, 4096);
    asn_sprint_value(base_ptrn, buf, sizeof(buf), 4);

    RING("%s() returns %s", __FUNCTION__, buf);

    if (base_ptrn_p != NULL)
        *base_ptrn_p = base_ptrn;

    return 0;
}

char **
tapi_flow_preprocess_args(int argc, char **argv)
{
    int i;
    char **new_argv = (char **)calloc(argc, sizeof(char *));

    RING("%s() started", __FUNCTION__);

    if (new_argv == NULL)
    {
        ERROR("Failed to allocate memory for preprocessed args");
        return NULL;
    }

    for (i = 0; i < argc; i++)
    {
        char *tmp = tapi_flow_preprocess_quotes(argv[i]);

        if ((new_argv[i] = tapi_flow_preprocess_links(tmp)) == NULL)
        {
            ERROR("Failed to preprocess argument '%s'", argv[i]);
        }
    }

    return new_argv;
}


te_errno
tapi_flow_prepare(int argc, char **argv, tapi_flow_t *flow)
{
    RING("%s() started", __FUNCTION__);

    TEST_GET_FLOW_CONFIG(flow);

    CHECK_RC(tapi_flow_prepare_endpoints(flow));
    CHECK_RC(tapi_flow_prepare_traffic(flow));

    CHECK_RC(tapi_flow_setup_endpoints(flow));

    return 0;
}

void
tapi_flow_fini(tapi_flow_t *flow)
{
    int rc = 0;
    tapi_flow_ep *ep;

    if (flow == NULL)
        return;

    RING("%s() started", __FUNCTION__);

    for (ep = SLIST_FIRST(&flow->ep_list);
         ep != NULL;
         ep = SLIST_NEXT(ep, link))
    {
        if ((rc = tapi_tad_csap_destroy(ep->ta, ep->sid,
                                        ep->csap_id)) != 0)
        {
            ERROR("Failed to destroy CSAP of %s endpoint", ep->name);
        }
    }
}

/* See header */
static void
tapi_flow_fill_random(void *data, unsigned int len)
{
    unsigned char *p = data;
    unsigned char *const beyond_end = p + len;

    RING("%s() started", __FUNCTION__);

    while (p < beyond_end)
        *p++ = rand_range(0, UCHAR_MAX);
}

te_errno
tapi_flow_start(tapi_flow_t *flow, char *name)
{
    int           rc;

    uint8_t             *payload = NULL;
    tapi_flow_traffic   *traffic = NULL;

    RING("%s() started", __FUNCTION__);

    if ((traffic = tapi_flow_find_traffic(flow, name)) == NULL)
    {
        ERROR("Cannot find traffic entry with name '%s'", name);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (traffic->started)
    {
        ERROR("Traffic exchange '%' is already running", name);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (traffic->plen > 0)
    {
        if ((payload = malloc(traffic->plen)) == NULL)
        {
            ERROR("Failed to allocate memory for PDU payload");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }

        RING("Generate random %d bytes of payload", traffic->plen);
        tapi_flow_fill_random(payload, traffic->plen);

        if ((rc = asn_write_value_field(traffic->send_tmpl, payload,
                                        traffic->plen,
                                        "payload.#bytes")) != 0)
        {
                ERROR("Failed to fill payload data into send template: rc=%r", rc);
                return rc;
        }
    }
    else
    {
        RING("No payload length specified");
    }

    if ((traffic->rcv_base != NULL) && (traffic->recv_base_ptrn != NULL))
    {
        /* Start base receive */
        if ((rc = tapi_tad_trrecv_start(traffic->rcv_base->ta,
                                        traffic->rcv_base->sid,
                                        traffic->rcv_base->csap_id,
                                        traffic->recv_base_ptrn,
                                        TAD_TIMEOUT_INF,
                                        traffic->count + 1,
                                        RCF_TRRECV_COUNT)) != 0)
        {
            ERROR("Failed to start receive operation");
            return rc;
        }
    }

    if ((traffic->rcv != NULL) && (traffic->recv_ptrn != NULL))
    {
        /* Start matching receive */
        if ((rc = tapi_tad_trrecv_start(traffic->rcv->ta,
                                        traffic->rcv->sid,
                                        traffic->rcv->csap_id,
                                        traffic->recv_ptrn,
                                        TAD_TIMEOUT_INF,
                                        traffic->count + 1,
                                        RCF_TRRECV_COUNT)) != 0)
        {
            ERROR("Failed to start receive operation");
            return rc;
        }
    }

    if ((traffic->snd != NULL) && (traffic->send_tmpl != NULL))
    {
        /* Send traffic */
        if ((rc = tapi_tad_trsend_start(traffic->snd->ta,
                                        traffic->snd->sid,
                                        traffic->snd->csap_id,
                                        traffic->send_tmpl,
                                        RCF_MODE_BLOCKING)) != 0)
        {
            ERROR("Failed to start send operation");
            return rc;
        }
    }

    traffic->started = TRUE;

    return 0;
}

te_errno
tapi_flow_stop(tapi_flow_t *flow, char *name,
               int *rcv_num_p, int *rcv_base_num_p)
{
    int           rc;

    unsigned int  rcv_num  = 0;
    unsigned int  rcv_base_num  = 0;

    tapi_flow_traffic   *traffic = NULL;

    RING("%s() started", __FUNCTION__);

    if ((traffic = tapi_flow_find_traffic(flow, name)) == NULL)
    {
        ERROR("Cannot find traffic entry with name '%s'", name);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (!traffic->started)
    {
        ERROR("Traffic step is not started");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((traffic->rcv != NULL) && (traffic->recv_ptrn != NULL))
    {
        if ((rc = tapi_tad_trrecv_stop(traffic->rcv->ta,
                                       traffic->rcv->sid,
                                       traffic->rcv->csap_id,
                                       NULL, &rcv_num)) != 0)
        {
            ERROR("Failed to stop receive operation");
            return rc;
        }
    }

    if ((traffic->rcv_base != NULL) && (traffic->recv_base_ptrn != NULL))
    {
        if ((rc = tapi_tad_trrecv_stop(traffic->rcv_base->ta,
                                       traffic->rcv_base->sid,
                                       traffic->rcv_base->csap_id,
                                       NULL, &rcv_base_num)) != 0)
        {
            ERROR("Failed to stop receive operation");
            return rc;
        }
    }

    if (rcv_num_p != NULL)
        *rcv_num_p = rcv_num;
    if (rcv_base_num_p != NULL)
        *rcv_base_num_p = rcv_base_num;

    traffic->started = FALSE;

    return 0;
}

/* Send and receive traffic described in traffic pattern */
te_errno
tapi_flow_check(tapi_flow_t *flow, char *name,
                int *rcv_num_p, int *rcv_base_num_p)
{
    int           rc;

    if ((rc = tapi_flow_start(flow, name)) != 0)
    {
        ERROR("Failed to start traffic '%s'", name);
        return rc;
    }

    MSLEEP(TAPI_FLOW_RECV_TIMEOUT);

    if ((rc = tapi_flow_stop(flow, name, rcv_num_p, rcv_base_num_p)) != 0)
    {
        ERROR("Failed to stop traffic '%s'", name);
        return rc;
    }

    return 0;
}

te_errno
tapi_flow_check_sequence(tapi_flow_t *flow,...)
{
    int rc = 0;
    char *name = NULL;
    int rcv_matched;
    int rcv_base;
    va_list ap;

    va_start(ap, flow);
    while ((name = va_arg(ap, char *)) != NULL)
    {
        if ((rc = tapi_flow_check(flow, name,
                                  &rcv_matched, &rcv_base)) != 0)
        {
            ERROR("Failed to perform '%s' traffic exchange", name);
            return rc;
        }

        if (rcv_matched != rcv_base)
        {
            ERROR("No matched packets received for '%s' traffic exchange");
            return rc;
        }
    }
    va_end(ap);

    return 0;
}

te_errno
tapi_flow_check_all(tapi_flow_t *flow, const char *traffic_prefix)
{
    tapi_flow_traffic  *traffic;
    char               *name = NULL;
    int                 rcv_matched;
    int                 rcv_base;
    int                 errors = 0;
    int                 rc = 0;

    RING("%s() started", __FUNCTION__);

    for (traffic = SLIST_FIRST(&flow->traffic_list);
         traffic != NULL;
         traffic = SLIST_NEXT(traffic, link))
    {
        if ((traffic_prefix != NULL) &&
            (strncmp(traffic->name, traffic_prefix,
             strlen(traffic_prefix)) != 0))
            continue;

        if ((rc = tapi_flow_check(flow, traffic->name,
                                  &rcv_matched, &rcv_base)) != 0)
        {
            ERROR_VERDICT("Failed to perform '%s' traffic exchange",
                          traffic->name);
            errors++;
        }

        if (rcv_matched != rcv_base)
        {
            ERROR_VERDICT("No matched packets received for '%s' "
                          "traffic exchange", traffic->name);
            errors++;
        }
    }

    if (errors > 0)
        return TE_RC(TE_TAPI, TE_ETIMEDOUT);
    else
        return 0;
}

