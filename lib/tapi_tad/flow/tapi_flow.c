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

#include "asn_impl.h"
#include "ndn.h"
#include "ndn_internal.h"
#include "ndn_flow.h"
#include "tapi_cfg.h"
#include "tapi_tad.h"
#include "tapi_flow.h"
#include "logger_api.h"

#include "tapi_test.h"

#define TAPI_FLOW_PREPROCESS_BUF_SIZE 1024

static char *
tapi_cfg_parse_link(char *buf)
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
//    printf("\nGet link: %s\n", p);
    RING("Get link: %s", p);

    return p;
}

char *
tapi_flow_preprocess(char *flow_spec)
{
    char *src      = flow_spec;
    int   buf_size = strlen(flow_spec) + 1;
    char *buf      = malloc(buf_size);
    char *dst      = buf;
    char *link     = NULL;
    char *value    = NULL;

    if (buf == NULL)
    {
        ERROR("Failed to allocate memory to process flow spec");
        return NULL;
    }

    while (flow_spec != NULL)
    {
        if ((src = strstr(flow_spec, TAPI_CFG_LINK_PREFIX)) == NULL)
        {
            strcpy(dst, flow_spec);
            break;
        }

        memcpy(dst, flow_spec, src - flow_spec);
        dst += src - flow_spec;

        link  = tapi_cfg_parse_link(src);
        RING("Try to resolve link: %s", link);
        value = tapi_get_cfg_link(link);
        if (strlen(value) > strlen(link))
        {
            buf_size += strlen(value) - strlen(link);
            if ((buf = realloc(buf, buf_size)) == NULL)
            {
                ERROR("Failed to allocate memory for preprocessed flow");
                return NULL;
            }
        }
        strcpy(dst, value);

        dst += strlen(value);
        flow_spec = src + strlen(value);
        free(value);
        free(link);
    }

    return buf;
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

    if ((rc = asn_read_string(flow_spec, &flow->snd.csap_desc,
                              "send.csap-desc.#plain")) != 0)
    {
        ERROR("Sender CSAP description is missing");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    RING("Sender CSAP: %s", flow->snd.csap_desc);

    /* Get sender CSAP spec */
    if ((rc = asn_get_descendent(flow_spec, &flow->snd.csap_spec,
                                 "send.csap-spec")) != 0)
    {
        ERROR("Sender CSAP is not specified");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

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

    if ((rc = asn_read_string(flow_spec, &flow->rcv.csap_desc,
                              "recv.csap-desc.#plain")) != 0)
    {
        ERROR("Receiver CSAP description is missing");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    RING("Receiver CSAP: %s", flow->rcv.csap_desc);

    /* Get receiver CSAP spec */
    if ((rc = asn_get_descendent(flow_spec, &flow->rcv.csap_spec,
                                 "recv.csap-spec")) != 0)
    {
        ERROR("Receiver CSAP is not specified");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Get PDUs to send */
    if ((rc = asn_get_descendent(flow_spec, &flow->traffic, "traffic")) != 0)
    {
        ERROR("Sender endpoint CSAP is not specified");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *flow_p = flow;
    return 0;
}

#define TAPI_FLOW_RECV_TIMEOUT 1000
#define TAPI_FLOW_RECV_COUNT_MAX 10

/* Remove ip4.ip-tos & eth.eth-prio fields from receive pattern */
te_errno
tapi_flow_remove_marking_ptrn(asn_value *ptrn)
{
    UNUSED(ptrn);
    return 0;
}

/* Send and receive traffic described in traffic pattern */
te_errno
tapi_flow_check(tapi_flow_t *flow, te_bool expected, int *rcv_num_p, int *exp_num_p)
{
    int           i;
    int           rc;
    asn_value    *step     = NULL;
    asn_value    *snd_tmpl = NULL;
    asn_value    *rcv_ptrn = NULL;
    asn_value    *exp_ptrn = NULL;
    unsigned int  rcv_num  = 0;
    unsigned int  exp_num  = 0;

    UNUSED(expected);

    for (i = 0;; i++)
    {
        if ((rc = asn_get_indexed(flow->traffic, &step, i, NULL)) != 0)
        {
            break;
        }

        /* Get send template */
        if ((rc = asn_get_descendent(step, &snd_tmpl,
                                     "send")) != 0)
        {
            ERROR("Failed to get send traffic template");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        /* Get receive pattern with matching rules */
        if ((rc = asn_get_descendent(step, &rcv_ptrn,
                                     "recv")) != 0)
        {
            ERROR("Failed to get receive pattern");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        /* Duplicate receive pattern with matching rules */
        if ((exp_ptrn = asn_copy_value(rcv_ptrn)) == NULL)
        {
            ERROR("Failed to copy receive pattern value");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }

        /* Remove ip-tos and eth-prio from receive pattern */
        if ((rc = tapi_flow_remove_marking_ptrn(rcv_ptrn)) != 0)
        {
            ERROR("Failed to remove marking fields from receive pattern");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }

        /* Start matching receive */
        if ((rc = tapi_tad_trrecv_start(flow->exp.ta, flow->exp.sid,
                                        flow->exp.csap_id, exp_ptrn,
                                        -1, TAPI_FLOW_RECV_COUNT_MAX,
                                        RCF_TRRECV_PACKETS)) != 0)
        {
            ERROR("Failed to start receive operation");
            return rc;
        }

        /* Start receive */
        if ((rc = tapi_tad_trrecv_start(flow->rcv.ta, flow->rcv.sid,
                                        flow->rcv.csap_id, rcv_ptrn,
                                        -1, TAPI_FLOW_RECV_COUNT_MAX,
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
            ERROR("Failed to start receive operation");
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

        if ((rc = tapi_tad_trrecv_stop(flow->exp.ta, flow->exp.sid,
                                       flow->exp.csap_id, NULL,
                                       &exp_num)) != 0)
        {
            ERROR("Failed to stop receive operation");
            return rc;
        }

        *rcv_num_p = rcv_num;
        *exp_num_p = exp_num;

#if 0
        if (rcv_num == 0)
        {
            TEST_VERDICT("Failed to receive traffic on the receiver's side");
        }

        if (exp_num == 0)
        {
            if (expected)
                TEST_VERDICT("Received traffic is not remarked");
        }
        else
        {
            if (expected)
                TEST_VERDICT("Received traffic is unexpectedly remarkered");
        }
#endif

    }

    return 0;
}


#if 0
te_errno
tapi_flow_csap_spec_to_stack(asn_value *spec, char **stack)
{
    asn_value *layer = NULL;
    int i;
    int rc = 0;

    for (i = 0; rc == 0; i++)
    {
        if ((rc = asn_get_indexed(spec, &layer, i, NULL)) != 0)
        {
            break;
        }
        if ((rc = asn_get_choice_value(layer, &layer, NULL, NULL)) != 0)
        {
            break;
        }
    }

    return rc;
}
#endif

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
    if ((rc = rcf_ta_create_session(flow->rcv.ta, &flow->exp.sid)) != 0)
    {
        ERROR("Failed to create expected receive session");
        return rc;
    }

    if ((rc = tapi_tad_csap_create(flow->rcv.ta, flow->exp.sid,
                                   flow->rcv.csap_desc,
                                   flow->rcv.csap_spec,
                                   &flow->rcv.csap_id)) != 0)
    {
        ERROR("Failed to create expected receive CSAP '%s'",
              flow->rcv.csap_desc);
        return rc;
    }

    return 0;
}

te_errno
tapi_flow_prepare(tapi_flow_t **flow_p, const char *flow_text)
{
    int rc         = 0;
    int num        = 0;
    char *flow_text_value = NULL;
    char buf[1024] = {0, };
    asn_value *asn_flow = NULL;

    if ((flow_text_value = tapi_flow_preprocess(flow_text)) == NULL)
    {
        ERROR("Failed to preprocess textual flow specification: %r", rc);
        return rc;
    }

    if ((rc = asn_parse_value_text(flow_text, ndn_flow, &asn_flow, &num)) != 0)
    {
        ERROR("parsed only %d syms, starting from: %s", num, flow_text + num);
        return rc;
    }

    asn_sprint_value(asn_flow, buf, 1024, 4);
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
    if ((flow->snd.sid >= 0) && (flow->snd.csap_id != CSAP_INVALID_HANDLE))
    {
        if ((rc = tapi_tad_csap_destroy(flow->snd.ta, flow->snd.sid,
                                    flow->snd.csap_id)) != 0)
        {
            ERROR("Failed to destroy send CSAP");
        }
    }

    /* Destroy matching csap  */
    if ((flow->snd.sid >= 0) && (flow->snd.csap_id != CSAP_INVALID_HANDLE))
    {
        if ((rc = tapi_tad_csap_destroy(flow->snd.ta, flow->snd.sid,
                                    flow->snd.csap_id)) != 0)
        {
            ERROR("Failed to destroy send CSAP");
        }
    }
}
