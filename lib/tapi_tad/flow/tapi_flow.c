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

extern te_errno
tapi_flow_csap_spec_to_stack(asn_value *spec, char **stack);

#define TAPI_FLOW_PREPROCESS_BUF_SIZE 1024

/**
 * Resolve configurator link into its value.
 *
 * @param link     Link to string to dereference
 *
 * @return Dereferenced link value from the configuration tree.
 */
static inline char * tapi_cfg_link_dereference(char *link)
{
    char           *cur = link + strlen(TAPI_CFG_LINK_PREFIX);
    cfg_handle      handle;
    cfg_obj_descr   descr;
    cfg_val_type    val_type;
    char           *value = NULL;
    int             rc = 0;

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
 * Preprocess textual flow description and dereference configurator links.
 *
 * @param flow_spec     Textual flow description
 *
 * @return              Preprocessed flow description which could be parsed by
 *                      asn_parse_text(), or NULL if something fails
 */
char *
tapi_flow_preprocess(const char *flow_spec)
{
    char *src      = (char *)flow_spec;
    int   buf_size = strlen(flow_spec) + 1;
    char *buf      = malloc(buf_size);
    char *dst      = buf;
    char *link     = NULL;
    char *value    = NULL;
    int   unused   = 0;

    if (buf == NULL)
    {
        ERROR("Failed to allocate memory to process flow spec");
        return NULL;
    }

    while (flow_spec != NULL)
    {
        if ((src = strstr(flow_spec, TAPI_CFG_LINK_PREFIX)) == NULL)
        {
            VERB("copy the rest of buffer: %s", flow_spec);
            strcpy(dst, flow_spec);
            break;
        }

        memcpy(dst, flow_spec, src - flow_spec);
        dst += src - flow_spec;

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
        flow_spec = src + strlen(link);
        free(value);
        free(link);
    }

    VERB("Preprocessed buf:\n%s", buf);
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
    int        len   = 0;

    if (buf == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    VERB("%s() started", __FUNCTION__);
    for (i = 0; rc == 0; i++)
    {
        VERB("%s(): try to get %d layer", __FUNCTION__, i);
        if ((rc = asn_get_indexed(spec, &layer, i, "layers")) != 0)
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
        len += sprintf(buf + len, "%s%s", (i > 0) ? "." : "", layer->name);
    }

    if (stack != NULL)
        *stack = buf;

    VERB("%s() returns %s", __FUNCTION__, buf);

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
        RING("%s(): try to get %d layer", __FUNCTION__, i);
        if ((rc = asn_get_indexed(rcv_ptrn, &layer, i, "0.pdus")) != 0)
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
                RING("Lenngth of src-addr: %d bytes", addr_len);

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
                RING("Lenngth of dst-addr: %d bytes", addr_len);

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


te_errno
tapi_flow_parse(tapi_flow_t **flow_p, asn_value *flow_spec)
{
    char            *snd_ta = NULL;
    char            *rcv_ta = NULL;
    tapi_flow_t *flow   = *flow_p;
    int              rc     = 0;

    if (flow == NULL)
    {
        if ((flow = (tapi_flow_t *)calloc(1, sizeof(tapi_flow_t))) == 
            NULL)
        {
            ERROR("Not enough memory for QoS flow initialisation");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }
    }

    if ((rc = asn_read_string(flow_spec, &flow->name, "name.#plain")) != 0)
    {
        ERROR("Flow name is missing");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /*
     * Parse sender endpoint
     */
    /* Get sender TA */
    if ((rc = asn_read_string(flow_spec, &snd_ta,
                              "send.ta.#plain")) != 0)
    {
        ERROR("Sender TA name is missing");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    flow->snd.ta = te_sprintf("agt_%s", snd_ta);
    RING("Sender TA: %s", flow->snd.ta);

    /* Get sender CSAP spec */
    if ((rc = asn_get_descendent(flow_spec, &flow->snd.csap_spec,
                                 "send.csap")) != 0)
    {
        ERROR("Sender CSAP is not specified");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((rc = tapi_flow_csap_spec_to_stack(flow->snd.csap_spec,
                                           &flow->snd.csap_desc)) != 0)
    {
        ERROR("Failed to get CSAP description");
        return rc;
    }
    RING("Sender CSAP: %s", flow->snd.csap_desc);

    /*
     * Parse receiver endpoint
     */
    /* Get receiver TA */
    if ((rc = asn_read_string(flow_spec, &rcv_ta,
                              "recv.ta.#plain")) != 0)
    {
        ERROR("Receiver TA name is missing");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    flow->rcv.ta = te_sprintf("agt_%s", rcv_ta);
    RING("Receiver TA: %s", flow->rcv.ta);

    /* Get receiver CSAP spec */
    if ((rc = asn_get_descendent(flow_spec, &flow->rcv.csap_spec,
                                 "recv.csap")) != 0)
    {
        ERROR("Receiver CSAP is not specified");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((rc = tapi_flow_csap_spec_to_stack(flow->rcv.csap_spec,
                                           &flow->rcv.csap_desc)) != 0)
    {
        ERROR("Failed to get CSAP description");
        return rc;
    }
    RING("Sender CSAP: %s", flow->rcv.csap_desc);

    /* Get traffic description */
    if ((rc = asn_get_descendent(flow_spec, &flow->traffic, "traffic")) != 0)
    {
        ERROR("Sender endpoint CSAP is not specified");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    tapi_flow_csap_spec_to_stack(flow->snd.csap_spec, NULL);

    *flow_p = flow;
    return 0;
}



te_errno
tapi_flow_init(tapi_flow_t *flow)
{
    int rc = 0;

    if (flow == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    /* Create send csap  */
    if ((rc = rcf_ta_create_session(flow->snd.ta, &flow->snd.sid)) != 0)
    {
        ERROR("Failed to create send session: %r", rc);
        return rc;
    }

    if ((rc = tapi_tad_csap_create(flow->snd.ta, flow->snd.sid,
                                   flow->snd.csap_desc,
                                   flow->snd.csap_spec,
                                   &flow->snd.csap_id)) != 0)
    {
        ERROR("Failed to create send CSAP '%s'", flow->snd.csap_desc);
        return rc;
    }

    /* Create receive csap */
    if ((rc = rcf_ta_create_session(flow->rcv.ta, &flow->rcv.sid)) != 0)
    {
        ERROR("Failed to create receive session: %r", rc);
        return rc;
    }

    if ((rc = tapi_tad_csap_create(flow->rcv.ta, flow->rcv.sid,
                                   flow->rcv.csap_desc,
                                   flow->rcv.csap_spec,
                                   &flow->rcv.csap_id)) != 0)
    {
        ERROR("Failed to create receive CSAP '%s'", flow->rcv.csap_desc);
        return rc;
    }

    /* Create second receive csap for remarkering matching */
    flow->rcv_base.ta = flow->rcv.ta;
    flow->rcv_base.csap_desc = flow->rcv.csap_desc;
    flow->rcv_base.csap_spec = flow->rcv.csap_spec;

    if ((rc = rcf_ta_create_session(flow->rcv.ta, &flow->rcv_base.sid)) != 0)
    {
        ERROR("Failed to create base receive session");
        return rc;
    }

    if ((rc = tapi_tad_csap_create(flow->rcv_base.ta, flow->rcv_base.sid,
                                   flow->rcv_base.csap_desc,
                                   flow->rcv_base.csap_spec,
                                   &flow->rcv_base.csap_id)) != 0)
    {
        ERROR("Failed to create base receive CSAP '%s'",
              flow->rcv_base.csap_desc);
        return rc;
    }

    return 0;
}

te_errno
tapi_flow_prepare(tapi_flow_t **flow_p, const char *flow_text)
{
    int         rc = 0;
    int         num = 0;
    char       *flow_text_deref = NULL;
    asn_value  *asn_flow = NULL;
    char        buf[4096] = {0, };

    if ((flow_text_deref = tapi_flow_preprocess(flow_text)) == NULL)
    {
        ERROR("Failed to preprocess textual flow specification: %r", rc);
        return rc;
    }
    RING("Preprocessed flow:\n%s", flow_text_deref);

    if ((rc = asn_parse_value_text(flow_text_deref, ndn_flow, &asn_flow, &num)) != 0)
    {
        ERROR("parsed only %d syms, starting from: %s", num, flow_text_deref + num);
        return rc;
    }

    asn_sprint_value(asn_flow, buf, 4096, 4);
    RING("Parsed %d:\n%s", num, buf);

    if ((rc = tapi_flow_parse(flow_p, asn_flow)) != 0)
    {
        ERROR("Failed to parse flow specification: %r", rc);
        return rc;
    }

    if ((rc = tapi_flow_init(*flow_p)) != 0)
    {
        ERROR("Failed to create flow endpoints: %r", rc);
        return rc;
    }

    return 0;
}

void
tapi_flow_fini(tapi_flow_t *flow)
{
    int rc = 0;

    if (flow == NULL)
        return;

    /* Destroy sender csap  */
    if ((flow->snd.sid >= 0) && (flow->snd.csap_id != CSAP_INVALID_HANDLE))
    {
        if ((rc = tapi_tad_csap_destroy(flow->snd.ta, flow->snd.sid,
                                    flow->snd.csap_id)) != 0)
        {
            ERROR("Failed to destroy send CSAP");
        }
    }

    /* Destroy receiver csap  */
    if ((flow->rcv.sid >= 0) && (flow->rcv.csap_id != CSAP_INVALID_HANDLE))
    {
        if ((rc = tapi_tad_csap_destroy(flow->rcv.ta, flow->rcv.sid,
                                    flow->rcv.csap_id)) != 0)
        {
            ERROR("Failed to destroy send CSAP");
        }
    }

    /* Destroy matching csap  */
    if ((flow->rcv_base.sid >= 0) && (flow->rcv_base.csap_id != CSAP_INVALID_HANDLE))
    {
        if ((rc = tapi_tad_csap_destroy(flow->rcv_base.ta, flow->rcv_base.sid,
                                    flow->rcv_base.csap_id)) != 0)
        {
            ERROR("Failed to destroy send CSAP");
        }
    }
}

#define TAPI_FLOW_RECV_TIMEOUT 10000
#define TAPI_FLOW_RECV_COUNT_MAX 10

/* Remove ip4.ip-tos & eth.eth-prio fields from receive pattern */
te_errno
tapi_flow_remove_marking_ptrn(asn_value *ptrn)
{
    UNUSED(ptrn);
    return 0;
}

/* See header */
static void
tapi_flow_fill_random(void *data, unsigned int len)
{
    unsigned char *p = data;
    unsigned char *const beyond_end = p + len;

    while (p < beyond_end)
        *p++ = rand_range(0, UCHAR_MAX);
}

/* Send and receive traffic described in traffic pattern */
te_errno
tapi_flow_check(tapi_flow_t *flow, int *rcv_num_p, int *rcv_base_num_p)
{
    int           i;
    int           rc;
    asn_value    *step     = NULL;
    asn_value    *snd_tmpl = NULL;
    asn_value    *rcv_ptrn = NULL;
    asn_value    *rcv_base_ptrn = NULL;
    int           pld_len = 0;
    unsigned int  rcv_num  = 0;
    unsigned int  rcv_base_num  = 0;
    char          buf[4096] = {0, };

    for (i = 0;; i++)
    {
        if ((rc = asn_get_indexed(flow->traffic, &step, i, NULL)) != 0)
        {
            break;
        }

        asn_sprint_value(step, buf, 4096, 4);
        RING("Step %d:\n%s", i, buf);

        /* Get send template */
        if ((rc = asn_get_descendent(step, &snd_tmpl,
                                     "send")) != 0)
        {
            ERROR("Failed to get send traffic template");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        /* Add payload to send template, if required */
        if ((rc = asn_read_int32(step, &pld_len, "payload-length.#plain")) == 0)
        {
            uint8_t *payload = malloc(pld_len);
            if (payload == NULL)
            {
                ERROR("Failed to allocate memory for PDU payload");
                return TE_RC(TE_TAPI, TE_ENOMEM);
            }
            ERROR("Payload length %d bytes", pld_len);
            tapi_flow_fill_random(payload, pld_len);
            if ((rc = asn_write_value_field(snd_tmpl, payload,
                                            pld_len, "payload.#bytes")) != 0)
            {
                ERROR("Failed to fill payload data into send template: rc=%r", rc);
                return rc;
            }
        }
        else
        {
            ERROR("No payload length specified, rc=%r", rc);
        }

        /* Get receive pattern with matching rules */
        if ((rc = asn_get_descendent(step, &rcv_ptrn,
                                     "recv")) != 0)
        {
            ERROR("Failed to get receive pattern");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        if ((rc = tapi_flow_gen_base_ptrn(rcv_ptrn, &rcv_base_ptrn)) != 0)
        {
            ERROR("Failed to prepare base pattern");
            return rc;
        }

        /* Start base receive */
        if ((rc = tapi_tad_trrecv_start(flow->rcv_base.ta, flow->rcv_base.sid,
                                        flow->rcv_base.csap_id, rcv_base_ptrn,
                                        TAD_TIMEOUT_INF, TAPI_FLOW_RECV_COUNT_MAX,
                                        RCF_TRRECV_PACKETS)) != 0)
        {
            ERROR("Failed to start receive operation");
            return rc;
        }

        /* Start matching receive */
        if ((rc = tapi_tad_trrecv_start(flow->rcv.ta, flow->rcv.sid,
                                        flow->rcv.csap_id, rcv_ptrn,
                                        TAD_TIMEOUT_INF, TAPI_FLOW_RECV_COUNT_MAX,
                                        RCF_TRRECV_PACKETS)) != 0)
        {
            ERROR("Failed to start receive operation");
            return rc;
        }

        /* Send traffic */
        if ((rc = tapi_tad_trsend_start(flow->snd.ta, flow->snd.sid,
                                        flow->snd.csap_id, snd_tmpl,
                                        RCF_MODE_BLOCKING)) != 0)
        {
            ERROR("Failed to start send operation");
            return rc;
        }

        MSLEEP(TAPI_FLOW_RECV_TIMEOUT);

        if ((rc = tapi_tad_trrecv_stop(flow->rcv.ta, flow->rcv.sid,
                                       flow->rcv.csap_id, NULL,
                                       &rcv_num)) != 0)
        {
            ERROR("Failed to stop receive operation");
            return rc;
        }

        if ((rc = tapi_tad_trrecv_stop(flow->rcv_base.ta, flow->rcv_base.sid,
                                       flow->rcv_base.csap_id, NULL,
                                       &rcv_base_num)) != 0)
        {
            ERROR("Failed to stop receive operation");
            return rc;
        }

        if (rcv_num_p != NULL)
            *rcv_num_p = rcv_num;
        if (rcv_base_num_p != NULL)
            *rcv_base_num_p = rcv_base_num;

    }

    return 0;
}
