/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Transmit module. 
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
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_UNISTD_H
#include <unistd.h> 
#endif

#include <stdio.h>
#include <string.h>
#include <pthread.h>

/* for ntohs, etc */
#include <netinet/in.h> 

#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"


#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "ndn.h" 

#define TE_LGR_USER     "TAD Send"
#include "logger_ta.h"

/* buffer for send answer */
#define RBUF 100 

#define SEND_ANSWER(_fmt...) \
    do {                                                            \
        struct rcf_comm_connection *handle = context->rcf_handle;   \
        int r_c;                                                    \
                                                                    \
        if (snprintf(answer_buffer + ans_len,                       \
                     RBUF - ans_len, _fmt) >= RBUF - ans_len)       \
        {                                                           \
            ERROR("answer is truncated");                           \
        }                                                           \
        rcf_ch_lock();                                              \
        VERB("Answer to send (%s:%d): %s",                          \
               __FILE__, __LINE__, answer_buffer);                  \
        r_c = rcf_comm_agent_reply(handle, answer_buffer,           \
                                   strlen(answer_buffer) + 1);      \
        rcf_ch_unlock();                                            \
    } while (0)


/**
 * Start routine for trsend thread. 
 *
 * @param arg      start argument, should be pointer to tad_task_context.
 *
 * @return nothing. 
 */
void * 
tad_tr_send_thread(void * arg)
{
    struct rcf_comm_connection *handle; 

    tad_task_context  *context = arg; 
    csap_pkts          packets_root; 

    uint32_t    sent = 0;
    uint32_t    delay;
    int         rc = 0;
    csap_p      csap_descr;
    char        answer_buffer[RBUF];  
    int         ans_len;

    asn_value        *nds; 

    const asn_value  *nds_pdus;
    const asn_value  *nds_payload;

    tad_payload_spec_t    pld_spec;

    tad_tmpl_iter_spec_t *arg_set_specs = NULL;
    tad_tmpl_arg_t       *arg_iterated = NULL;
    int                   arg_num = 0; 

    struct timeval npt; /* Next packet time moment */


    if (arg == NULL)
    {
        ERROR("tr_send thread start point: null argument! exit");
        return NULL;
    }
    handle = context->rcf_handle;

    memset(&pld_spec, 0, sizeof(pld_spec));

    csap_descr = context->csap;
    nds        = context->nds; 

    if (csap_descr == NULL)
    {
        ERROR("tr_send thread: null CSAP! exit.");
        free(context);
        return NULL;
    }
    strcpy(answer_buffer, csap_descr->answer_prefix);
    ans_len = strlen(answer_buffer);

    do {
        const asn_value *arg_sets;

        if (csap_descr->prepare_send_cb)
        {
            rc = csap_descr->prepare_send_cb(csap_descr);
            if (rc)
            {
                ERROR("prepare for send failed %r", rc);
                break;
            }
        }

        rc = asn_get_child_value(nds, &nds_pdus, PRIVATE, NDN_TMPL_PDUS);
        if (rc)
            break;

        rc = tad_confirm_pdus(csap_descr, (asn_value *)nds_pdus); 
        if (rc)
            break;

        rc = asn_get_child_value(nds, &nds_payload, PRIVATE,
                                 NDN_TMPL_PAYLOAD); 
        if (rc == 0)
        {
            rc = tad_convert_payload(nds_payload, &pld_spec);
        }
        else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            rc = 0; /* there is no payload */
            pld_spec.type = TAD_PLD_UNKNOWN;
        } 

        if (rc != 0)
        {
            ERROR("%s(): prepare payload failed rc %r", __FUNCTION__, rc);
            break;
        } 

        rc = asn_get_subvalue(nds, &arg_sets, "arg-sets");

        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) 
        {
            rc = 0; 
            break;
        }

        if (rc)
            break;

        arg_num =  asn_get_length(arg_sets, ""); 
        if (arg_num <= 0) 
            break;

        arg_set_specs = calloc(arg_num, sizeof(tad_tmpl_iter_spec_t));
        rc = tad_get_tmpl_arg_specs(arg_sets, arg_set_specs, arg_num);

        VERB("get_tmpl_arg_specs rc: %x\n", rc);

        arg_iterated = calloc(arg_num, sizeof(tad_tmpl_arg_t));
        if (arg_iterated == NULL)
            rc = TE_ENOMEM;

        if (rc)
            break;
        
        rc = tad_init_tmpl_args(arg_set_specs, arg_num, arg_iterated);
        VERB("tad_init_arg_specs rc: %x\n", rc); 

        {
            size_t v_len = sizeof(delay);

            rc = asn_read_value_field(nds, &delay, &v_len, "delays");
        }

        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            delay = 0;
            rc = 0; 
            break;
        }
        if (rc)
            break;

    } while (0);


    if ((rc == 0) && !(csap_descr->state & TAD_STATE_FOREGROUND)) 
        SEND_ANSWER("0 0"); 

    if (rc)
    {
        ERROR("preparing template error: %r", rc);
        SEND_ANSWER("%d",  TE_RC(TE_TAD_CSAP, rc)); 
        csap_descr->command = TAD_OP_IDLE;
        csap_descr->state = 0;
        rc = 0;
    } 
    else
    {
        do { 
            csap_pkts *pkt;

            if (!(csap_descr->state   & TAD_STATE_FOREGROUND) &&
                 (csap_descr->command == TAD_OP_STOP))
            {
                strcpy(answer_buffer, csap_descr->answer_prefix);
                ans_len = strlen(answer_buffer); 
                break;
            }

            rc = tad_tr_send_prepare_bin(csap_descr, nds, 
                                         arg_iterated, arg_num, 
                                         &pld_spec, &packets_root);
            F_VERB("send_prepare_bin rc: %r", rc);

            if (rc)
                break;

            /* Delay for send, if necessary. */
            if (sent == 0)
            {
                gettimeofday(&npt, NULL);
                F_VERB("check start send moment: %u.%u", 
                        npt.tv_sec, npt.tv_usec);
            }
            else if (delay > 0)
            {
                struct timeval tv_delay = {0, 0};
                struct timeval cm;

                gettimeofday(&cm, NULL);
                F_VERB("calc of delay, current moment: %u.%u", 
                        cm.tv_sec, cm.tv_usec);

                /* calculate moment until we should wait before next 
                 * get log */
                npt.tv_sec  += delay / 1000;
                npt.tv_usec += delay * 1000;

                if (npt.tv_usec >= 1000000)
                {
                    npt.tv_usec -= 1000000;
                    npt.tv_sec ++;
                }
                F_VERB("wait for next send moment: %u.%u", 
                        npt.tv_sec, npt.tv_usec);

                /* calculate delay of waiting we should wait before 
                 * next get log */
                if (npt.tv_sec >= cm.tv_sec)
                    tv_delay.tv_sec  = npt.tv_sec  - cm.tv_sec;

                if (npt.tv_usec >= cm.tv_usec)
                    tv_delay.tv_usec = npt.tv_usec - cm.tv_usec;
                else if (tv_delay.tv_sec > 0)
                {
                    tv_delay.tv_usec = (npt.tv_usec + 1000000) - cm.tv_usec;
                    tv_delay.tv_sec--;
                } 

                F_VERB("calculated delay: %u.%u", 
                        tv_delay.tv_sec, tv_delay.tv_usec);
                select(0, NULL, NULL, NULL, &tv_delay); 
            }

            /* Delay performed, now send prepared data. */ 

            for (pkt = &packets_root; pkt!= NULL; pkt = pkt->next)
            {
                rc = csap_descr->write_cb(csap_descr, pkt->data, pkt->len);

                if (rc < 0)
                {
                    F_ERROR("CSAP #%d internal write error %d", 
                                csap_descr->id, csap_descr->last_errno);
                    rc = TE_OS_RC(TE_TAD_CSAP, csap_descr->last_errno);
                    break;
                }

                gettimeofday (&csap_descr->last_pkt, NULL);
                if (!sent)
                    csap_descr->first_pkt = csap_descr->last_pkt;

                sent++;
                csap_descr->total_bytes += rc;

                F_VERB("CSAP #%d write, %d bytes, sent %d pkts", 
                        csap_descr->id, rc, sent);

                if (pkt->free_data_cb)
                    pkt->free_data_cb(pkt->data);
                else
                    free(pkt->data);

                pkt->data = 0;
                rc = 0;
            }
            for (pkt = packets_root.next; pkt != NULL;
                 packets_root.next = pkt->next, free(pkt),
                 pkt = packets_root.next);

            if (rc)
                break;
            
        } while (tad_iterate_tmpl_args(arg_set_specs, 
                                       arg_num, arg_iterated) > 0);
    }

    /* Release all resources, finish task */

    if (csap_descr->release_cb)
        csap_descr->release_cb(csap_descr);

    asn_free_value(nds); 
    free(arg_iterated);
    tad_tmpl_args_clear(arg_set_specs, arg_num);
    free(arg_set_specs);

    if ((csap_descr->state   & TAD_STATE_FOREGROUND) || 
        (csap_descr->command == TAD_OP_STOP) )
    {
        VERB("blocked or long trsend finished. rc %x, sent %d", 
             rc, sent);

        csap_descr->command = TAD_OP_IDLE;
        csap_descr->state   = 0;

        if (rc)
            SEND_ANSWER("%d",  TE_RC(TE_TAD_CH, rc)); 
        else 
            SEND_ANSWER("0 %d", sent); 
    }
    else if (rc)
    { 
        csap_descr->last_errno = rc; 
        csap_descr->state |= TAD_STATE_COMPLETE;

        while(1) 
        {
            struct timeval wait_for_stop_delay = {0, 30000};
            select(0, NULL, NULL, NULL, &wait_for_stop_delay);

            if (csap_descr->command == TAD_OP_STOP)
            {
                csap_descr->command = TAD_OP_IDLE;
                csap_descr->state = 0;
                strcpy(answer_buffer, csap_descr->answer_prefix);
                ans_len = strlen(answer_buffer); 
                SEND_ANSWER("%d",  TE_RC(TE_TAD_CH, rc)); 
                break;
            }
        }
    }

    free(context); 
    tad_payload_spec_clear(&pld_spec);
    return NULL;
}


/* see description in tad_utils.h */
int
tad_convert_payload(const asn_value *ndn_payload, 
                    tad_payload_spec_t *pld_spec)
{
    const asn_value *payload;

    asn_tag_class t_class;
    uint16_t      t_val;

    int rc = 0;

    if (ndn_payload == NULL || pld_spec == NULL)
        return TE_EWRONGPTR;

    rc = asn_get_choice_value(ndn_payload, &payload, &t_class, &t_val);
    if (rc != 0)
    {
        ERROR("%s(): get choice of Payload failed %r", __FUNCTION__, rc);
        return rc;
    }

    switch (t_val)
    {
        case NDN_PLD_BYTES:  
            pld_spec->type = TAD_PLD_BYTES;
            {
                size_t d_len = asn_get_length(payload, "");
                void  *data = malloc(d_len);

                rc = asn_read_value_field(payload, data, &d_len, "");
                if (rc)
                {
                    free(data);
                    return TE_RC(TE_TAD_CH, rc);
                }
                pld_spec->plain.length = d_len;
                pld_spec->plain.data = data;
            }
            break;

        case NDN_PLD_FUNC:  
            {
                char   func_name[256];
                size_t fn_len = sizeof(func_name);

                pld_spec->type = TAD_PLD_FUNCTION;
                rc = asn_read_value_field(payload, func_name, &fn_len, "");

                if (rc == 0 && 
                    (pld_spec->func = (tad_user_generate_method) 
                            rcf_ch_symbol_addr(func_name, 1)) 
                    == NULL)
                {
                    ERROR("Function %s for pld generation NOT found",
                          func_name);
                    rc = TE_RC(TE_TAD_CH, TE_ENOENT); 
                }
            }
            break;

        case NDN_PLD_LEN:   
            pld_spec->type = TAD_PLD_LENGTH;
            {
                int32_t len;
                asn_read_int32(payload, &len, "");
                pld_spec->plain.length = len;
                pld_spec->plain.data = NULL;
            }
            break;

        case NDN_PLD_STREAM:
            {
                char   func_name[256];
                size_t fn_len = sizeof(func_name);

                const asn_value *du_field;

                pld_spec->type = TAD_PLD_STREAM;
                rc = asn_read_value_field(payload, func_name, &fn_len,
                                          "function");
                if (rc != 0)
                    break;
                RING("%s(): stream function <%s>", __FUNCTION__, func_name);
                if ((pld_spec->stream.func = (tad_stream_callback) 
                                    rcf_ch_symbol_addr(func_name, 1)) 
                    == NULL)
                {
                    ERROR("Function %s for stream NOT found",
                          func_name);
                    rc = TE_RC(TE_TAD_CH, TE_ENOENT); 
                }

                if ((rc = asn_get_child_value(payload, &du_field, PRIVATE,
                                              NDN_PLD_STR_OFF)) != 0)
                    break;

                tad_data_unit_convert_simple(du_field, 
                                             &(pld_spec->stream.offset));

                if ((rc = asn_get_child_value(payload, &du_field, PRIVATE,
                                              NDN_PLD_STR_LEN)) != 0)
                    break;

                tad_data_unit_convert_simple(du_field, 
                                             &(pld_spec->stream.length));
            }
            break;

        default: 
            pld_spec->type = TAD_PLD_UNKNOWN;
    }

    if (rc != 0)
        ERROR("%s failed %r", __FUNCTION__, rc);

    return rc;
}

/* see description in tad_utils.h */
void
tad_payload_spec_clear(tad_payload_spec_t *pld_spec)
{
    if (pld_spec == NULL)
        return;
    switch (pld_spec->type)
    {
        case TAD_PLD_UNKNOWN:
        case TAD_PLD_LENGTH:
        case TAD_PLD_FUNCTION:
            /* do nothing */
            break;

        case TAD_PLD_BYTES:
            free(pld_spec->plain.data);
            break;

        case TAD_PLD_STREAM:
            tad_data_unit_clear(&pld_spec->stream.offset);
            tad_data_unit_clear(&pld_spec->stream.length);
            break;
    }
    memset(pld_spec, 0, sizeof(*pld_spec));
    pld_spec->type = TAD_PLD_UNKNOWN;
}

/* see description in tad_utils.h */
int
tad_tr_send_prepare_bin(csap_p csap_descr, asn_value_p nds, 
                        const tad_tmpl_arg_t *args, size_t arg_num,
                        tad_payload_spec_t *pld_data,
                        csap_pkts_p pkts)
{ 
    tad_payload_spec_t local_pld_data;

    csap_pkts *up_packets  = NULL;
    csap_pkts *low_packets = NULL;

    int  level = 0;
    int  rc = 0;

    if (pld_data == NULL)
    {
        const asn_value *ndn_payload;

        pld_data = &local_pld_data;

        rc = asn_get_child_value(nds, &ndn_payload, PRIVATE,
                                 NDN_TMPL_PAYLOAD);
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            pld_data->type = TAD_PLD_UNKNOWN;
            rc = 0;
        }
        else if (rc != 0)
            return TE_RC(TE_TAD_CH, rc);
        else 
        { 
            if ((rc = tad_convert_payload(ndn_payload, pld_data)) != 0)
                return TE_RC(TE_TAD_CH, rc); 
        }
    }

    if (pld_data->type != TAD_PLD_UNKNOWN)
    { 
        up_packets = malloc(sizeof(csap_pkts));
        memset(up_packets, 0, sizeof(csap_pkts));
    }

    switch (pld_data->type)
    {
        case TAD_PLD_FUNCTION:
        { 
            size_t d_len = asn_get_length(nds, "payload.#bytes");
            void  *data = malloc(d_len);

            if (pld_data->func == NULL)
            {
                ERROR("%s(): null function pointer, error", __FUNCTION__);
                return TE_RC(TE_TAD_CH, TE_ETADMISSNDS);
            }

            rc = pld_data->func(csap_descr->id, -1 /* payload */,
                                nds); 
            if (rc)
                return TE_RC(TE_TAD_CH, rc); 

            rc = asn_read_value_field(nds, data, &d_len, "payload.#bytes");
            if (rc)
            {
                free(data);
                return TE_RC(TE_TAD_CH, rc);
            }
            up_packets->data = data;
            up_packets->len  = d_len;
            break;
        } 
        case TAD_PLD_BYTES: 
            up_packets->data = malloc(up_packets->len = 
                                      pld_data->plain.length);
            memcpy(up_packets->data, pld_data->plain.data, up_packets->len);
            break;
        
        case TAD_PLD_LENGTH:
            up_packets->data = malloc(up_packets->len = 
                                      pld_data->plain.length);
            memset(up_packets->data, 0x5a, up_packets->len);
            break;

        case TAD_PLD_STREAM: 
            {
                uint32_t offset;
                uint32_t length;

                rc = tad_data_unit_to_bin(&pld_data->stream.length,
                                          args, arg_num,
                                          (uint8_t *)&length,
                                          sizeof(length));
                if (rc != 0)
                    break;
                length = ntohl(length);

                rc = tad_data_unit_to_bin(&pld_data->stream.offset,
                                          args, arg_num,
                                          (uint8_t *)&offset,
                                          sizeof(offset));
                if (rc != 0)
                    break;
                offset = ntohl(offset);

                up_packets->data = malloc(up_packets->len = length);
                if (pld_data->stream.func == NULL)
                {
                    ERROR("%s(): null stream function pointer, error",
                          __FUNCTION__);
                    return TE_RC(TE_TAD_CH, TE_ETADMISSNDS);
                }
                rc = pld_data->stream.func(offset, length, 
                                           up_packets->data);
            }
            break;

        default:
            break;
        /* TODO: processing of other choices should be inserted here. */
    } 

    if (rc != 0)
    {
        free(up_packets->data);
        return TE_RC(TE_TAD_CH, rc);
    }

    for (level = 0; rc == 0 && level < csap_descr->depth; level ++)
    { 
        csap_spt_type_p  csap_spt_descr; 
        const asn_value *level_pdu = NULL; 

        char  label[20];

        sprintf(label, "pdus.%d", level);

        low_packets = malloc(sizeof(csap_pkts));
        memset(low_packets, 0, sizeof(csap_pkts));

        rc = asn_get_subvalue(nds, &level_pdu, label); 
        if (rc != 0)
        {
            ERROR("get subvalue in generate packet fails %r", rc);
            rc = TE_RC(TE_TAD_CH, rc);
        }

        if (rc == 0)
        {
            csap_spt_descr = csap_descr->layers[level].proto_support;

            rc = csap_spt_descr->generate_cb(csap_descr, level, 
                                             level_pdu, args, arg_num,
                                             up_packets, low_packets); 
        }

        while (up_packets != NULL)
        {
            csap_pkts_p next = up_packets->next;

            if (up_packets->free_data_cb)
                up_packets->free_data_cb(up_packets->data);
            else
                free(up_packets->data);

            free(up_packets);
            up_packets = next;
        }

        if (rc != 0) 
        {
            ERROR("generate binary data error; "
                  "rc: %r, csap id: %d, level: %d", 
                  rc, csap_descr->id, level);

            rc = TE_RC(TE_TAD_CSAP, rc);
        }

        up_packets = low_packets;
    }
    /* free of low_packets should be here. */

    if (csap_descr->depth)
    {
        memcpy(pkts, low_packets, sizeof(*pkts)); 
        free(low_packets);
    }
    return TE_RC(TE_TAD_CH, rc);
}


/**
 * Description see in tad_utils.h
 */
int 
tad_iterate_tmpl_args(tad_tmpl_iter_spec_t *arg_specs, 
                      size_t arg_specs_num, 
                      tad_tmpl_arg_t *arg_iterated)
{
    int     dep = arg_specs_num - 1;
    te_bool performed = FALSE;

    tad_tmpl_iter_spec_t *arg_spec_p;

    if (arg_specs == NULL)
        return 0;

    if (arg_iterated == NULL)
        return -1;

    for (arg_spec_p = arg_specs + dep; 
         dep >= 0 && !performed; 
         dep--, arg_spec_p--)
    {
        switch(arg_spec_p->type)
        {
            case TAD_TMPL_ITER_FOR:
                if (arg_iterated[dep].arg_int < arg_spec_p->simple_for.end)
                { 
                    arg_iterated[dep].arg_int += 
                        arg_spec_p->simple_for.step;
                    performed = TRUE;
                }
                else
                {
                    arg_iterated[dep].arg_int = 
                        arg_spec_p->simple_for.begin;
                    /* formally, it's unnecessary here, but algorithm
                       is more clear with this operator. */
                }
                VERB("for, value %d, dep %d, performed %d", 
                     arg_iterated[dep].arg_int, dep, (int)performed); 
                break;

            case TAD_TMPL_ITER_INT_ASSOC:
            case TAD_TMPL_ITER_INT_SEQ:
                {
                    int new_index = arg_spec_p->int_seq.last_index + 1;

                    if ((size_t)new_index == arg_spec_p->int_seq.length) 
                        new_index = 0;
                    else if (arg_spec_p->type == TAD_TMPL_ITER_INT_SEQ)
                        performed = TRUE;

                    arg_iterated[dep].arg_int =
                        arg_spec_p->int_seq.ints[new_index];

                    VERB("ints, index %d, value %d, dep %d, performed %d", 
                         new_index, arg_iterated[dep].arg_int,
                         dep, (int)performed); 

                    arg_spec_p->int_seq.last_index = new_index;
                }
                break;

            case TAD_TMPL_ITER_STR_SEQ:
                /* TODO: implement other choices in template argument */
                return -TE_EOPNOTSUPP;
                break;
        }
    }

    return (int)performed;
}

/**
 * Description see in tad_utils.h
 */
int 
tad_get_tmpl_arg_specs(const asn_value *arg_set, 
                       tad_tmpl_iter_spec_t *arg_specs, size_t arg_num)
{
    unsigned i; 
    int      rc = 0;
    int32_t  arg_param = 0;
    size_t   enum_len;

    uint16_t      t_val;
    asn_tag_class t_class;

    if (arg_set == NULL || arg_specs == NULL)
        return -1; 

    for (i = 0; i < arg_num; i++)
    { 
        const asn_value *arg_val;
        const asn_value *arg_spec_elem;
        rc = asn_get_indexed(arg_set, &arg_spec_elem, i);
        if (rc) break;

        rc = asn_get_choice_value(arg_spec_elem, &arg_val,
                                  &t_class, &t_val);
        if (rc) break;

        VERB("iter tag class %d, tag val %d", (int)t_class, (int)t_val);


        switch (t_val)
        {
            case NDN_ITER_INTS:
            case NDN_ITER_INTS_ASSOC:
                arg_specs[i].type = ((t_val == NDN_ITER_INTS) ? 
                        TAD_TMPL_ITER_INT_SEQ : TAD_TMPL_ITER_INT_ASSOC);
                enum_len = arg_specs[i].int_seq.length =
                                                asn_get_length(arg_val, "");
                arg_specs[i].int_seq.last_index = -1;
                arg_specs[i].int_seq.ints = calloc(enum_len,
                                                   sizeof(int32_t));

                {
                    unsigned j;
                    for (j = 0; j < enum_len; j++)
                    {
                        const asn_value *int_val;
                        asn_get_indexed(arg_val, &int_val, j);
                        asn_read_int32(int_val, &arg_param, "");
                        arg_specs[i].int_seq.ints[j] = arg_param;
                    }
                }
                break;

            case NDN_ITER_FOR:
                arg_specs[i].type = TAD_TMPL_ITER_FOR;
                rc = asn_read_int32(arg_val, &arg_param, "begin");
                arg_specs[i].simple_for.begin = ((rc == 0) ? arg_param : 
                                            TAD_ARG_SIMPLE_FOR_BEGIN_DEF);

                VERB("simple-for, begin %d", arg_specs[i].simple_for.begin);

                rc = asn_read_int32(arg_val, &arg_param, "step");
                arg_specs[i].simple_for.step = ((rc == 0) ? arg_param :
                                            TAD_ARG_SIMPLE_FOR_STEP_DEF); 
                VERB("simple-for, step %d", arg_specs[i].simple_for.step);

                rc = asn_read_int32(arg_val, &arg_param, "end");
                if (rc != 0)
                    break; /* There is no default for end of 'simple-for' */
                arg_specs[i].simple_for.end = arg_param;
                VERB("simple-for, end %d", arg_specs[i].simple_for.end);
                break;

            default:
            /* TODO: implement other choices in template argument */
                rc = TE_EOPNOTSUPP;
                break;
        }
        if (rc != 0)
            break;
    }
    return rc;
}
            


/**
 * Description see in tad_utils.h
 */
int 
tad_init_tmpl_args(tad_tmpl_iter_spec_t *arg_specs, size_t arg_specs_num,
                   tad_tmpl_arg_t *arg_iterated)
{
    unsigned i;

    if (arg_specs == NULL || arg_specs_num == 0)
        return 0;

    if (arg_iterated == NULL)
        return TE_EWRONGPTR;

    memset(arg_iterated, 0, arg_specs_num * sizeof(tad_tmpl_arg_t));

    for (i = 0; i < arg_specs_num; i++)
    { 
        switch (arg_specs[i].type)
        {
            case TAD_TMPL_ITER_INT_SEQ:
            case TAD_TMPL_ITER_INT_ASSOC:
                arg_iterated[i].arg_int = arg_specs[i].int_seq.ints[0];
                arg_specs[i].int_seq.last_index = 0;
                arg_iterated[i].type = TAD_TMPL_ARG_INT;
                break;

            case TAD_TMPL_ITER_FOR:
                arg_iterated[i].arg_int = arg_specs[i].simple_for.begin; 
                arg_iterated[i].type = TAD_TMPL_ARG_INT;
                break;

            case TAD_TMPL_ITER_STR_SEQ:
                arg_iterated[i].type = TAD_TMPL_ARG_STR;
                break;
        }
    }
    return 0;
}

/**
 * Description see in tad_utils.h
 */
void
tad_tmpl_args_clear(tad_tmpl_iter_spec_t *arg_specs, size_t arg_num)
{
    unsigned i;

    if (arg_specs == NULL)
        return;
    for (i = 0; i < arg_num; i++)
    {
        switch (arg_specs[i].type)
        {
            case TAD_TMPL_ITER_INT_SEQ:
            case TAD_TMPL_ITER_INT_ASSOC:
                free(arg_specs[i].int_seq.ints);
                break;
            case TAD_TMPL_ITER_STR_SEQ:
            case TAD_TMPL_ITER_FOR:
                /* nothing to do */
                break;
        }
    }
}

