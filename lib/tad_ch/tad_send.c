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

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#define TE_LGR_USER     "TAD CH"
#include "logger_ta.h"

#include "tad.h" 
#include "ndn.h" 

/* buffer for send answer */
#define RBUF 100 

#define SEND_ANSWER(_fmt...) \
    do {                                                            \
        struct rcf_comm_connection *handle = context->rcf_handle;   \
        int r_c;                                                    \
        if (snprintf(answer_buffer + ans_len,                       \
                     RBUF - ans_len, _fmt) >= RBUF - ans_len)       \
        {                                                           \
            ERROR("answer is truncated\n");                         \
        }                                                           \
        rcf_ch_lock();                                              \
        VERB("Answer to send (%s:%d): %s",                          \
               __FILE__, __LINE__, answer_buffer);                  \
        r_c = rcf_comm_agent_reply(handle, answer_buffer,           \
                                   strlen(answer_buffer) + 1);      \
        rcf_ch_unlock();                                            \
    } while (0)



/**
 * Prepare binary data by NDS.
 *
 * @param csap_descr    CSAP description structure;
 * @param nds           ASN value with traffic-template NDS, should be
 *                      preprocessed (all iteration and function calls
 *                      performed);
 * @param handle        handle of RCF connection;
 * @param pkts          packets with generated binary data;
 *
 * @return zero on success, otherwise error code.  
 */
int
tad_tr_send_prepare_bin(csap_p csap_descr, asn_value_p nds, 
        struct rcf_comm_connection *handle, 
        const tad_template_arg_t *args, size_t arg_num,
        tad_payload_type pld_type, const void *pld_data, csap_pkts_p pkts)
{
    csap_pkts *up_packets  = NULL;
    csap_pkts *low_packets = NULL;

    int level = 0;
    int rc = 0;
    char pld_label[10] = "";

    UNUSED (handle);

    if (pld_type == TAD_PLD_UNKNOWN || pld_data == NULL)
    {
        rc = asn_get_choice(nds, "payload", pld_label, sizeof(pld_label));
        if (rc == EASNINCOMPLVAL)
            rc = 0;
        else if (rc)
            return TE_RC(TE_TAD_CH, rc);
        else 
        {
            pld_type = tad_payload_asn_label_to_enum(pld_label);
            if (pld_data == NULL)
            {
                int pld_data_len = asn_get_length (nds, "payload");
                uint8_t *pld_data_local = malloc(pld_data_len);

                if (pld_data_local == NULL) 
                    return ENOMEM;
                rc = asn_read_value_field(nds, 
                                pld_data_local, &pld_data_len, "payload");
                if (rc)
                    return TE_RC(TE_TAD_CH, rc);
                pld_data = pld_data_local;
            }
        }
    }

    switch (pld_type)
    {
        case TAD_PLD_FUNCTION:
        { 
            tad_user_generate_method function_addr = pld_data;

            /* pld_data is really 'char *' with function name */
            if (pld_data == NULL)
                return EINVAL;


            rc = function_addr(csap_descr->id, -1 /*payload is strange level*/,
                                nds); 
            if (rc)
                return TE_RC(TE_TAD_CH, rc); 

        } /* fall through! */
        case TAD_PLD_BYTES:
        {
            int d_len = asn_get_length(nds, "payload.#bytes");
            void *data = malloc(d_len);
            rc = asn_read_value_field(nds, data, &d_len, "payload.#bytes");
            if (rc)
            {
                free(data);
                return TE_RC(TE_TAD_CH, rc);
            }
            up_packets = malloc(sizeof(csap_pkts));
            memset(up_packets, 0, sizeof(csap_pkts));
            up_packets->data = data;
            up_packets->len  = d_len;
        }
        break;
        case TAD_PLD_LENGTH:
        {
            int d_len, l;
            void *data;

            l = sizeof (d_len);
            rc = asn_read_value_field(nds, &d_len, &l, "payload.#length");
            if (rc)
                return TE_RC(TE_TAD_CH, rc);

            data = malloc(d_len);
            up_packets = malloc(sizeof(csap_pkts));
            memset(up_packets, 0, sizeof(csap_pkts));
            up_packets->data = data;
            up_packets->len  = d_len;
            memset(data, 0x5a, d_len);
        }
        break;
        default:
        ;
        break;
        /* TODO: processing of other choices should be inserted here. */
    } 

    VERB("called\n");

    for (level = 0; rc == 0 && level < csap_descr->depth; level ++)
    { 
        csap_spt_type_p csap_spt_descr; 
        const asn_value *level_pdu = NULL; 
        char label[20];

        sprintf (label, "pdus.%d", level);

        low_packets = malloc(sizeof(csap_pkts));
        memset(low_packets, 0, sizeof(csap_pkts));

        rc = asn_get_subvalue(nds, &level_pdu, label); 
        if (rc)
        {
            ERROR("get subvalue in generate packet fails %x", rc);
            rc = TE_RC(TE_TAD_CH, rc);
        }

        if (rc == 0)
        {
            csap_spt_descr = find_csap_spt (csap_descr->proto[level]);

            F_VERB("before generate_cb, level: %d, up_pkts: %x\n",
                   level, up_packets);
            rc = csap_spt_descr->generate_cb(csap_descr->id, level, 
                                             level_pdu, args, arg_num,
                                             up_packets, low_packets);
        
        }

        if (up_packets)
        {
            if (up_packets->free_data_cb)
                up_packets->free_data_cb(up_packets->data);
            else
                free(up_packets->data);

            free(up_packets);
            up_packets = NULL;
        }

        if (rc) 
        {
            ERROR(
                       "generate binary data error; "
                       "rc: 0x%x, csap id: %d, level: %d\n", 
                       rc, csap_descr->id, level);

            rc = TE_RC(TE_TAD_CSAP, rc);
        }

        up_packets = low_packets;
    }
    /* free of low_packets should be here. */

    if (csap_descr->depth)
    {
        memcpy (pkts, low_packets, sizeof (*pkts));

        free(low_packets);
    }
    return TE_RC(TE_TAD_CH, rc);
}



/**
 * Start routine for trsend thread. 
 *
 * @param arg      start argument, should be pointer to tad_task_context struct.
 *
 * @return nothing. 
 */
void * 
tad_tr_send_thread(void * arg)
{
    struct rcf_comm_connection *handle; 

    tad_task_context  *context = arg; 
    csap_pkts          packets_root; 

    uint32_t    delay;
    int         rc = 0;
    csap_p      csap_descr;
    char        answer_buffer[RBUF];  
    int         ans_len;
    asn_value  *nds; 
    const asn_value *pdus;
    uint32_t    sent = 0;
    tad_payload_type pld_type = TAD_PLD_UNKNOWN;
    uint8_t     *pld_data = NULL;

    tad_template_arg_spec_t *arg_set_specs = NULL;
    tad_template_arg_t      *arg_iterated = NULL;
    int         arg_num; 

    struct timeval npt; /* Next packet time moment */

    if (arg == NULL)
    {
        ERROR("tr_send thread start point: null argument! exit");
        return NULL;
    }
    handle = context->rcf_handle;

    csap_descr = context->csap;
    nds        = context->nds; 

    if (csap_descr == NULL)
    {
        ERROR("tr_send thread: null CSAP! exit.");
        free(context);
        return NULL;
    }
    strcpy(answer_buffer, csap_descr->answer_prefix);
    ans_len = strlen (answer_buffer);

    do {
        const asn_value *arg_sets;
        char pld_label[10] = "";
        int pld_spec_len;


        if (csap_descr->prepare_send_cb)
        {
            rc = csap_descr->prepare_send_cb(csap_descr);
            if (rc)
            {
                ERROR("prepare for send failed %x", rc);
                break;
            }
        }

        rc = asn_get_subvalue (nds, &pdus, "pdus");
        if (rc)
            break;

        rc = tad_confirm_pdus(csap_descr, (asn_value *)pdus); 
        if (rc)
            break;

        rc = asn_get_choice(nds, "payload", pld_label, sizeof(pld_label));

        VERB("get_choice rc: %x, choice: <%s>\n", rc, pld_label);

        if (rc == 0)
            pld_type = tad_payload_asn_label_to_enum(pld_label);

        if (rc == EASNINCOMPLVAL)
        {
            rc = 0; /* there is no payload */
            pld_type = TAD_PLD_UNKNOWN;
        }

        if (rc)
        {
            F_ERROR("get payload type in trsend thread rc %x", rc);
            break;
        }

        pld_spec_len = sizeof(int); /* pass through */
        switch (pld_type)
        {
            case TAD_PLD_BYTES:
            case TAD_PLD_FUNCTION:
                pld_spec_len = asn_get_length(nds, "payload"); 
                /* pass through */
            case TAD_PLD_LENGTH:
                if (pld_spec_len <= 0)
                    break;
                pld_data = calloc (1, pld_spec_len); 

                rc = asn_read_value_field(nds, 
                        pld_data, &pld_spec_len, "payload");
                if (pld_type == TAD_PLD_FUNCTION)
                {
                    tad_user_generate_method function_addr;

                    function_addr = (tad_user_generate_method) 
                                    rcf_ch_symbol_addr((char *)pld_data, 1);

                    if (function_addr == NULL)
                        rc = TE_RC(TE_TAD_CH, ETENOSUCHNAME); 
                    free(pld_data);
                    pld_data = (uint8_t *) function_addr;
                }
                break;
            case TAD_PLD_SCRIPT:
            case TAD_PLD_UNKNOWN:
            default: 
                pld_data = NULL;
        }

        if (rc)
            break;

        rc = asn_get_subvalue(nds, &arg_sets, "arg-sets");

        if (rc == EASNINCOMPLVAL) 
        {
            rc = 0; 
            break;
        }

        if (rc)
            break;

        arg_num =  asn_get_length(arg_sets, ""); 
        if (arg_num <= 0) 
            break;

        arg_set_specs = calloc(arg_num, sizeof (tad_template_arg_spec_t));
        rc = tad_get_tmpl_arg_specs(arg_sets, arg_set_specs, arg_num);

        VERB("get_tmpl_arg_specs rc: %x\n", rc);

        arg_iterated = calloc(arg_num, sizeof (tad_template_arg_t));
        if (arg_iterated == NULL)
            rc = ENOMEM;

        if (rc)
            break;
        
        rc = tad_init_tmpl_args(arg_set_specs, arg_num, arg_iterated);
        VERB("tad_init_arg_specs rc: %x\n", rc);


        {
            size_t v_len = sizeof(delay);
            rc = asn_read_value_field(nds, &delay, &v_len, "delays");
        }

        if (rc == EASNINCOMPLVAL)
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
        ERROR("preparing template error: 0x%x", rc);
        SEND_ANSWER("%d",  TE_RC(TE_TAD_CSAP, rc)); 
        csap_descr->command = 0;
        csap_descr->state = 0;
        rc = 0;
    } 
    else
    {
        do { 
            csap_pkts *pkt;

            if (!(csap_descr->state   & TAD_STATE_FOREGROUND) &&
                 (csap_descr->command & TAD_COMMAND_STOP))
            {
                strcpy(answer_buffer, csap_descr->answer_prefix);
                ans_len = strlen (answer_buffer); 
                break;
            }

            rc = tad_tr_send_prepare_bin(csap_descr, nds, handle, arg_iterated,
                        arg_num, pld_type, pld_data, &packets_root);
            F_VERB("send_prepare_bin rc: %x\n", rc);

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
                struct timeval tv_delay = {0,0};
                struct timeval cm;

                gettimeofday(&cm, NULL);
                F_VERB("calc of delay, current moment: %u.%u", 
                        cm.tv_sec, cm.tv_usec);

                /* calculate moment until we should wait before next get log */
                npt.tv_sec  += delay / 1000;
                npt.tv_usec += delay * 1000;

                if (npt.tv_usec >= 1000000)
                {
                    npt.tv_usec -= 1000000;
                    npt.tv_sec ++;
                }
                F_VERB("wait for next send moment: %u.%u", 
                        npt.tv_sec, npt.tv_usec);

                /* calculate delay of waiting we should wait before next get log */
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

            for(pkt = &packets_root; pkt!= NULL; pkt = pkt->next)
            {
                rc = csap_descr->write_cb(csap_descr, pkt->data, pkt->len);

                if (rc < 0)
                {
                    F_ERROR("CSAP #%d internal write error 0x%x", 
                                csap_descr->id, csap_descr->last_errno);
                    rc = TE_RC(TE_TAD_CSAP, csap_descr->last_errno);
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
            if (rc)
                break;
            
        } while (tad_iterate_tmpl_args(arg_set_specs, arg_num, arg_iterated) > 0);
    }

    /* Release all resources, finish task */

    if (csap_descr->release_cb)
        csap_descr->release_cb(csap_descr);

    asn_free_value(nds); 
    free(arg_iterated);
    free(arg_set_specs);

    if (pld_type != TAD_PLD_FUNCTION)
        free(pld_data);


    if ((csap_descr->state   & TAD_STATE_FOREGROUND) || 
        (csap_descr->command & TAD_COMMAND_STOP) )
    {
        VERB(
                "blocked or long trsend finished. rc %x, sent %d", 
                rc, sent);

        csap_descr->command = 0;
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
            select (0, NULL, NULL, NULL, &wait_for_stop_delay);

            if (csap_descr->command & TAD_COMMAND_STOP)
            {
                csap_descr->command = 0;
                csap_descr->state = 0;
                strcpy(answer_buffer, csap_descr->answer_prefix);
                ans_len = strlen (answer_buffer); 
                SEND_ANSWER("%d",  TE_RC(TE_TAD_CH, rc)); 
                break;
            }
        }
    }

    csap_descr->command = 0;
    csap_descr->state = 0;
    free(context); 
    return NULL;
}

/**
 * Perform next iteration for passed template arguments.
 *
 * @param arg_specs     array of template argument specifications.
 * @param arg_specs_num length of array.
 * @param arg_iterated  array of template arguments (OUT).
 *
 * @retval      - positive on success itertaion.
 * @retval      - zero if iteration finished.
 * @retval      - negative if invalid arguments passed.
 */
int 
tad_iterate_tmpl_args(tad_template_arg_spec_t *arg_specs, size_t arg_specs_num, 
                                  tad_template_arg_t *arg_iterated)
{
    int dep = arg_specs_num - 1;
    tad_template_arg_spec_t *arg_spec_p;
    int performed = 0;

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
            case ARG_TMPL_FOR:
                if (arg_iterated[dep].arg_int < arg_spec_p->simple_for.end)
                { 
                    arg_iterated[dep].arg_int += arg_spec_p->simple_for.step;
                    performed = 1;
                }
                else
                {
                    arg_iterated[dep].arg_int = arg_spec_p->simple_for.begin; 
                    continue; /* formally, it's unnecessary here, but algorithm
                                 is more clear with this operator. */
                }
                break;
            case ARG_TMPL_INT_SEQ:
            case ARG_TMPL_STR_SEQ:
                /* TODO: implement other choices in template argument */
                return ETENOSUPP;
                break;
        }
    }

    return performed;
}

/**
 * Get argument set from template ASN value and put it into plain-C array
 *
 * @param arg_set       ASN value of type "SEQENCE OF Template-Parameter",
 *                      which is subvalue with label 'arg-sets' in
 *                      Traffic-Template.
 * @param arg_specs     memory block for arg_spec array. should be allocated by 
 *                      user. 
 * @param arg_num       length of arg_spec array, i.e. quantity of
 *                      Template-Arguments in template.
 *
 * @return zero on success, otherwise error code. 
 */
int 
tad_get_tmpl_arg_specs (const asn_value *arg_set, 
                           tad_template_arg_spec_t *arg_specs, size_t arg_num)
{
    const asn_value *arg_val;
    unsigned i; 
    int rc;
    char lab[20];
    char choice[20];

    if (arg_set == NULL || arg_specs == NULL)
        return -1; 


    for (i = 0; i < arg_num; i++)
    { 
        snprintf (lab, sizeof(lab), "%d", i);
        rc = asn_get_choice(arg_set, lab, choice, sizeof(choice));
        if (rc) break;

        rc = asn_get_subvalue(arg_set, &arg_val, lab);
        if (rc) break; 
        VERB("get_template_arg_specs, choice for %d: <%s>\n",
                    i, choice);
        if (strcmp(choice, "simple-for") == 0)
        {
            int v_len;
            arg_specs[i].type = ARG_TMPL_FOR;
            v_len = sizeof(arg_specs[i].simple_for.begin);
            rc = asn_read_value_field (arg_val, 
                    &(arg_specs[i].simple_for.begin), &v_len, "begin");
            if (rc)
                arg_specs[i].simple_for.begin = TAD_ARG_SIMPLE_FOR_BEGIN_DEF; 

            v_len = sizeof(arg_specs[i].simple_for.step);
            rc = asn_read_value_field (arg_val, 
                    &(arg_specs[i].simple_for.step), &v_len, "step");
            if (rc)
                arg_specs[i].simple_for.step = TAD_ARG_SIMPLE_FOR_STEP_DEF; 

            v_len = sizeof(arg_specs[i].simple_for.end);
            rc = asn_read_value_field (arg_val, 
                    &(arg_specs[i].simple_for.end), &v_len, "end");
            if (rc)
                break; /* There is no default for end of 'simple-for' */
        }
        else /* TODO: implement other choices in template argument */
        {
            rc = ETENOSUPP;
            break;
        }
    }
    return rc;
}
            


/**
 * Description see in tad.h
 */
int 
tad_init_tmpl_args(tad_template_arg_spec_t *arg_specs, size_t arg_specs_num, 
                            tad_template_arg_t *arg_iterated)
{
    unsigned i;

    if (arg_specs == NULL || arg_specs_num == 0)
        return 0;

    if (arg_iterated == NULL)
        return ETEWRONGPTR;

    memset (arg_iterated, 0, arg_specs_num * sizeof (tad_template_arg_t));

    for (i = 0; i < arg_specs_num; i++)
    { 
        switch (arg_specs[i].type)
        {
            case ARG_TMPL_FOR:
                arg_iterated[i].arg_int = arg_specs[i].simple_for.begin;
                /* fall through! */

                /* TODO: init of first iteration parameters to other 
                 * template argument types */
            case ARG_TMPL_INT_SEQ:
                arg_iterated[i].type = ARG_INT;
                break;
            case ARG_TMPL_STR_SEQ:
                arg_iterated[i].type = ARG_STR;
                break;
        }
    }
    return 0;
}


