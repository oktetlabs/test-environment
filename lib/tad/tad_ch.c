/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Definition of routines provided for Portable Command Handler * 
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

#include <string.h>
#include <pthread.h>

#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "te_defs.h"

#define TE_LGR_USER     "TAD CH"
#include "logger_ta.h"


#ifndef DUMMY_TAD
#include "tad.h"
#include "ndn.h" 


static struct timeval tv_zero = {0,0};

#define SEND_ANSWER(_fmt...) \
    do {                                                                   \
        int r_c;                                                           \
                                                                           \
        if ((size_t)snprintf(cbuf + answer_plen, buflen - answer_plen,     \
                             _fmt) >= (buflen - answer_plen))              \
        {                                                                  \
            VERB("answer is truncated\n");                                 \
        }                                                                  \
        rcf_ch_lock();                                                     \
        r_c = rcf_comm_agent_reply(handle, cbuf, strlen(cbuf) + 1);        \
        rcf_ch_unlock();                                                   \
        if (r_c)                                                           \
            fprintf(stderr, "rc from rcf_comm_agent_reply: 0x%X\n", r_c);  \
    } while (0)

#endif

static int is_initialized = 0;

#ifdef WITH_ETH
extern int csap_support_eth_register(void);
#endif

#ifdef WITH_IPSTACK
extern int csap_support_ipstack_register(void);
#endif

#ifdef WITH_FILE
extern int csap_support_file_register(void);
#endif

#ifdef WITH_SNMP
extern int csap_support_snmp_register(void);
#endif

#ifdef WITH_DHCP
extern int csap_support_dhcp_register(void);
#endif

#ifdef WITH_BRIDGE
extern int csap_support_bridge_register(void);
#endif

#ifdef WITH_CLI
extern int csap_support_cli_register(void);
#endif


static void
check_init(void)
{
    if (is_initialized) return;

#ifndef DUMMY_TAD
    csap_db_init();
    init_csap_spt();
#ifdef WITH_ETH
    csap_support_eth_register();
#endif 
#ifdef WITH_IPSTACK
    csap_support_ipstack_register();
#endif
#ifdef WITH_FILE
    csap_support_file_register();
#endif
#ifdef WITH_SNMP
    csap_support_snmp_register();
#endif
#ifdef WITH_CLI
    csap_support_cli_register();
#endif
#ifdef WITH_DHCP
    csap_support_dhcp_register();
#endif
#ifdef WITH_BRIDGE
    csap_support_bridge_register();
#endif
#endif /* DUMMY_TAD */

    is_initialized = 1;
}

void
tad_ch_init(void)
{
    check_init();
}

#ifndef DUMMY_TAD 

/**
 * Safe compare two strings. Almost equivalent to standard "strcmp", but
 * correct work if one or both arguments are NULL. 
 * If both arguments are empty strings or NULL (in any combination), they 
 * are considered as equal and zero is returned. 
 * 
 *
 * @param l        first string
 * @param r        second string
 *
 * @return same value as strcmp. 
 */
static int
strcmp_imp(const char *l, const char *r)
{
    if (l == NULL)
    {
        if ((r == NULL) || (*r == 0))
            return 0;
        else 
            return -1;
    } 
    if (r == NULL)
    {
        if (*l == 0)
            return 0;
        else
            return 1;
    }

    return strcmp(l, r);
}

#endif /* DUMMY_TAD */

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_create(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen,
                   const char *stack, const char *params)
{
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("CSAP create: stack <%s> params <%s>\n", stack, params);
    return -1;
#else
    csap_p      new_csap;
    asn_value_p csap_nds;

    int new_csap_id; 
    int level;
    int syms; 
    int rc; 

    UNUSED(cmdlen);
    UNUSED(params);


    check_init();

    new_csap_id = csap_create(stack);

    INFO("CSAP '%s' created, new id: %d", stack, new_csap_id);

    if (new_csap_id == 0)
    {
        /* ERROR! CSAP is not created. */
        SEND_ANSWER("%d CSAP creation internal error", ETADCSAPDB);
        return 0;
    }


    if (ba == NULL)
    {
        /* ERROR! NDS is mandatory for CSAP create command. */
        ERROR("missing attached NDS");
        SEND_ANSWER("%d missing attached NDS", ETADMISSNDS);
        csap_destroy(new_csap_id);
        return 0;
    }
    new_csap = csap_find(new_csap_id); 

    /* NDS is parsed in textual form now*/

    rc = asn_parse_value_text(ba, ndn_csap_spec, &csap_nds, &syms);
    if (rc)
    {
        /* ERROR! NDS is not parsed correctly */

        VERB("parse error in attached NDS, rc: 0x%x, sym: %d", rc, syms);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        csap_destroy(new_csap_id); 
        return 0;
    }
    syms = asn_get_length(csap_nds, "");
    INFO("length of PDU list in NDS: %d, csap depth %d", 
          syms, new_csap->depth); 

    for (level = 0; level < new_csap->depth; level ++)
    {
        csap_layer_neighbour_list_p nbr_p; 
        csap_spt_type_p csap_spt_descr; 
        char *lower_proto = NULL;
        int val_len;

        csap_spt_descr = find_csap_spt(new_csap->proto[level]);

        if (csap_spt_descr == NULL)
        {
            ERROR("Protocol[%d] '%s' is not supported", 
                    level, new_csap->proto[level]);
            /* ERROR! asked protocol is not supported. */
            SEND_ANSWER("%d Protocol is not supported", 
                    TE_RC(TE_TAD_CH, EPROTONOSUPPORT));
            csap_destroy(new_csap_id); 
            asn_free_value(csap_nds);
            return 0;
        }

        val_len = asn_get_length(csap_nds, "");
        VERB("CSAP spec array len %d", val_len);


        if (level + 1 < new_csap->depth)
            lower_proto = new_csap->proto[level + 1];
        
        for (nbr_p = csap_spt_descr->neighbours;
             nbr_p;
             nbr_p = nbr_p->next)
        {
            if (strcmp_imp(lower_proto, nbr_p->nbr_type) == 0)
            {
                rc = nbr_p->init_cb(new_csap_id, csap_nds, level);
                if (rc) 
                {
                    /* Init ERROR!*/
                    ERROR("CSAP init error %X", rc);
                    SEND_ANSWER("%d CSAP init error", TE_RC(TE_TAD_CH, rc));
                    csap_destroy(new_csap_id); 
                    asn_free_value(csap_nds);
                    return 0;
                }
                break;
            }
        }
        if (nbr_p == NULL)
        {
            /* ERROR! there was not found neighvour -- 
               this stack is not supported.*/
            ERROR("Protocol stack for low '%s' under '%s' is not supported",
                    lower_proto, new_csap->proto[level]);
            SEND_ANSWER("%d Protocol stack is not supported", 
                TE_RC(TE_TAD_CH, EPROTONOSUPPORT));
            csap_destroy(new_csap_id); 
            asn_free_value(csap_nds);
            return 0;
        }
    }
    SEND_ANSWER("0 %d", new_csap_id); 
    asn_free_value(csap_nds);
    return 0; 
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_destroy(struct rcf_comm_connection *handle,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    csap_handle_t csap)
{
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("CSAP destroy: handle %d\n", csap);
    return -1;
#else
    csap_p csap_descr_p;

    int level;
    int rc; 
    
    check_init();

    VERB("%s: CSAP %d\n", __FUNCTION__, csap); 

    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        WARN("%s: CSAP %d not exists", __FUNCTION__, csap);
        SEND_ANSWER("%d CSAP not exists", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap_descr_p);
    if (csap_descr_p->command)
    {
        WARN("%s: CSAP %d is busy", __FUNCTION__, csap);
        CSAP_DA_UNLOCK(csap_descr_p);
        SEND_ANSWER("%d Specified CSAP is busy", 
                    TE_RC(TE_TAD_CH, ETADCSAPSTATE));
        return 0;
    } 
    CSAP_DA_UNLOCK(csap_descr_p);

    for (level = 0; level < csap_descr_p->depth; level ++)
    {
        csap_layer_neighbour_list_p nbr_p; 
        
        csap_spt_type_p  csap_spt_descr; 
        char            *lower_proto = NULL;

        csap_spt_descr = find_csap_spt(csap_descr_p->proto[level]);

        if (csap_spt_descr == NULL)
        {
            ERROR("protocol support is not found.");

            SEND_ANSWER("%d Generic error: protocol support is not found.",
                        TE_RC(TE_TAD_CH, EPROTONOSUPPORT));
            return 0;
        }
        VERB("found protocol support for <%s>.", 
             csap_descr_p->proto[level]);

        if (level + 1 < csap_descr_p->depth)
            lower_proto = csap_descr_p->proto[level + 1];
        
        for (nbr_p = csap_spt_descr->neighbours;
             nbr_p;
             nbr_p = nbr_p->next)
        {
            if (strcmp_imp(lower_proto, nbr_p->nbr_type)==0)
            {
                VERB("found destroy neigbour callback"); 
                rc = nbr_p->destroy_cb(csap, level);
                break;
            }
        }
    }
    csap_destroy(csap);
    
    SEND_ANSWER("0");

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_start(struct rcf_comm_connection *handle,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    const uint8_t *ba, size_t cmdlen,
                    csap_handle_t csap, te_bool postponed)
{
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("TRSEND start: handle %d %s\n",
         csap, postponed ? "postponed" : "");
    return -1;
#else
    int            rc;
    int            syms;
    asn_value_p    nds; 
    csap_p         csap_descr_p;
    pthread_t      send_thread;
    pthread_attr_t pthread_attr;

    tad_task_context *send_context;

    UNUSED(cmdlen);

    VERB("trsend_start CSAP %d\n", csap);

    check_init();

    if (ba == NULL)
    {
        VERB("missing attached NDS");
        SEND_ANSWER("%d missing attached NDS", 
                    TE_RC(TE_TAD_CH, ETADMISSNDS));
        return 0;
    }

    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        ERROR("CSAP not exists");
        SEND_ANSWER("%d Wrong CSAP id", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
        return 0;
    }

    csap_descr_p->total_bytes = 0;
    csap_descr_p->first_pkt = tv_zero;
    csap_descr_p->last_pkt  = tv_zero;

    rc = asn_parse_value_text(ba, ndn_traffic_template, &nds, &syms);

    if (rc)
    {
        /* ERROR! NDS is not parsed correctly */
        ERROR("parse error in attached NDS, code 0x%x, symbol: %d",
              rc, syms);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        return 0;
    } 

    CSAP_DA_LOCK(csap_descr_p);

    strncpy(csap_descr_p->answer_prefix, cbuf, MAX_ANS_PREFIX - 1);
    csap_descr_p->answer_prefix[answer_plen] = 0; 

    csap_descr_p->state = csap_descr_p->command = TAD_OP_SEND;

    if (postponed)
        csap_descr_p->state |= TAD_STATE_FOREGROUND;

    CSAP_DA_UNLOCK(csap_descr_p);

    if (csap_descr_p->check_pdus_cb)
    {
        rc = csap_descr_p->check_pdus_cb(csap_descr_p, nds);
        if (rc) 
        {
            ERROR("send nds check_pdus error: %x", rc);
            SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
            return 0;
        }
    }

    send_context = malloc(sizeof(tad_task_context));
    send_context->csap          = csap_descr_p;
    send_context->nds           = nds;
    send_context->rcf_handle    = handle;
    
    if ((rc = pthread_attr_init(&pthread_attr)) != 0 ||
        (rc = pthread_attr_setdetachstate(&pthread_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0)
    {
        ERROR("Cannot initialize pthread attribute variable: %X", rc);
        SEND_ANSWER("%d cannot initialize pthread attribute variable",
                    TE_RC(TE_TAD_CH, rc));
        free(send_context);
        return 0;
    }

    rc = pthread_create(&send_thread, &pthread_attr, 
                        (void *)&tad_tr_send_thread, send_context);
    if (rc != 0)
    {
        ERROR("Cannot create a new SEND thread: %d", rc);
        SEND_ANSWER("%d cannot create a new SEND thread", 
                    TE_RC(TE_TAD_CH, rc));
        free(send_context);
    }
    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_stop(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap)
{
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRSEND stop handle %d\n", csap);
    return -1;
#else
    csap_p csap_descr_p;

    VERB("trsend_stop CSAP %d", csap);

    check_init(); 

    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        VERB("CSAP not exists");
        SEND_ANSWER("%d Wrong CSAP id", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap_descr_p);

    if (csap_descr_p->command & TAD_OP_SEND)
    {
        strncpy(csap_descr_p->answer_prefix, cbuf, answer_plen);
        csap_descr_p->answer_prefix[answer_plen] = 0; 
        csap_descr_p->command |= TAD_COMMAND_STOP;
        CSAP_DA_UNLOCK(csap_descr_p);
    }
    else
    {
        CSAP_DA_UNLOCK(csap_descr_p);
        F_VERB("Inappropriate command, CSAP is not sending ");
        SEND_ANSWER("%d Inappropriate command in current state", 
                    TE_RC(TE_TAD_CH, ETADCSAPSTATE));
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_start(struct rcf_comm_connection *handle,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    const uint8_t *ba, size_t cmdlen, csap_handle_t csap,
                    unsigned int num, te_bool results, unsigned int timeout)
{
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(csap);
    UNUSED(num);
    UNUSED(results);
    UNUSED(timeout);

    VERB("TRRECV start: handle %d num %u timeout %u %s\n",
         csap, num, timeout, results ? "results" : "");
    return -1;
#else
    char        label_buf[20];
    pthread_t   recv_thread;
    int         syms;
    int         rc;
    asn_value_p nds = NULL; 
    csap_p      csap_descr_p;
    int         i, num_units;

    tad_task_context *recv_context;
    pthread_attr_t    pthread_attr;
    char              srname[] = "trsend_recv";
    int               sr_flag = 0;

    UNUSED(cmdlen);

    VERB("CSAP %d", csap);

    check_init(); 

    if (strncmp(cbuf + answer_plen, srname, strlen(srname)) == 0)
        sr_flag = 1;

    if (ba == NULL)
    {
        VERB("missing attached NDS");
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, ETADMISSNDS));
        return 0;
    }

    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        VERB("CSAP not exists");
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap_descr_p);
    if (csap_descr_p->command)
    {
        CSAP_DA_UNLOCK(csap_descr_p);
        VERB("CSAP is busy");
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, ETADCSAPSTATE));
        return 0;
    }
    CSAP_DA_UNLOCK(csap_descr_p);

    csap_descr_p->total_bytes = 0;
    csap_descr_p->first_pkt = tv_zero;
    csap_descr_p->last_pkt  = tv_zero;

    if (sr_flag)
        rc = asn_parse_value_text(ba, ndn_traffic_template, &nds, &syms);
    else
        rc = asn_parse_value_text(ba, ndn_traffic_pattern, &nds, &syms);

    if (rc)
    { 
        VERB("parse error in NDS, rc: 0x%x, sym: %d", rc, syms);
        VERB("sr_flag %d, NDS: <%s>", sr_flag, ba);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        return 0;
    } 
    VERB("nds parsed, pointer: %x", nds);

    
    CSAP_DA_LOCK(csap_descr_p);

    csap_descr_p->command = TAD_OP_RECV; 
    if (sr_flag) 
    {
        csap_descr_p->command |= TAD_OP_SEND; 
        csap_descr_p->state |= TAD_STATE_FOREGROUND;
    }

    csap_descr_p->state |= csap_descr_p->command;

    strncpy (csap_descr_p->answer_prefix, cbuf, answer_plen);
    csap_descr_p->answer_prefix[answer_plen] = 0; 

    CSAP_DA_UNLOCK(csap_descr_p);

    VERB("csap %d, timeout %u", csap_descr_p->id, 
            timeout);
    if (timeout && timeout != TAD_TIMEOUT_INF)
    { 
        gettimeofday(&csap_descr_p->wait_for, NULL);
        csap_descr_p->wait_for.tv_usec += timeout * 1000;
        csap_descr_p->wait_for.tv_sec += 
            (csap_descr_p->wait_for.tv_usec / 1000000);
        csap_descr_p->wait_for.tv_usec %= 1000000;

        VERB("init, csap %d, wait_for set %u.%u", 
                    csap_descr_p->id,  
                    csap_descr_p->wait_for.tv_sec,
                    csap_descr_p->wait_for.tv_usec);
    }
    else 
        memset(&csap_descr_p->wait_for, 0, sizeof(struct timeval));

    csap_descr_p->num_packets = num;

    CSAP_DA_LOCK(csap_descr_p);
    if (results)
        csap_descr_p->command |= TAD_COMMAND_RESULTS;
    CSAP_DA_UNLOCK(csap_descr_p);

    recv_context = malloc(sizeof(tad_task_context));
    recv_context->csap          = csap_descr_p;
    recv_context->nds           = nds;
    recv_context->rcf_handle    = handle; 

    if (sr_flag == 0)
        num_units = asn_get_length(nds, "");
    else
        num_units = 1; /* There is a single unit */
    rc = 0;
    for (i = 0; i < num_units; i++)
    {
        asn_value *pattern_unit;
        int level;

        if (sr_flag == 0)
            pattern_unit = asn_read_indexed(nds, i, "");
        else
            pattern_unit = nds;

        if (pattern_unit == NULL)
        {
            WARN("%s: NULL pattern unit #%d", __FUNCTION__, i); 
            rc = ETADWRONGNDS;
            break;
        }

        if (csap_descr_p->check_pdus_cb)
        {
            rc = csap_descr_p->check_pdus_cb(csap_descr_p, pattern_unit);
            if (rc) 
            {
                WARN("recv nds check_pdus error: %x", rc);
                break;
            }
        }

        for (level = 0; (rc == 0) && (level < csap_descr_p->depth); level++)
        {
            csap_spt_type_p csap_spt_descr; 
            asn_value_p level_pdu; 

            level_pdu = asn_read_indexed(pattern_unit, level, "pdus"); 
            if (level_pdu == NULL)
            {
                WARN("CSAP #%d, cannot get level pdu #%d", csap, level);
                rc = ETADWRONGNDS;
                break;
            }

            csap_spt_descr = find_csap_spt (csap_descr_p->proto[level]);

            rc = csap_spt_descr->confirm_cb(csap_descr_p->id, level, 
                                            level_pdu);

            if (rc) 
            {
                WARN("%s: csap #%d confirm pattern pdu fails; 0x%x\n", 
                        __FUNCTION__, csap, rc);
                break;
            }
            snprintf (label_buf, sizeof(label_buf), "pdus.%d.#%s", 
                    level, csap_descr_p->proto[level]);

            rc = asn_write_component_value(pattern_unit, level_pdu, 
                                           label_buf);

            asn_free_value(level_pdu);

            if (rc)
            {
                WARN("replace component value with confirmed pdu " 
                     "fails; 0x%x\n", rc);
            } 
        }
        if (sr_flag == 0)
        {
            if (rc == 0)
            {
                rc = asn_write_indexed(nds, pattern_unit, i, "");
            }
            asn_free_value(pattern_unit);
        }
        if (rc)
            break; 
    }
    if (rc) 
    {
        WARN("pattern does not confirm to CSAP; rc 0x%x", rc);
        asn_free_value(nds);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        return 0;
    }

    if ((rc = pthread_attr_init(&pthread_attr)) != 0 ||
        (rc = pthread_attr_setdetachstate(&pthread_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0)
    {
        ERROR("Cannot initialize pthread attribute variable: %X", rc);
        SEND_ANSWER("%d cannot initialize pthread attribute variable",
                    TE_RC(TE_TAD_CH, rc));
        free(recv_context);
        return 0;
    }

    if ((rc = pthread_create(&recv_thread, &pthread_attr,
                             tad_tr_recv_thread, recv_context)) != 0)
    {
        ERROR("recv thread create error; rc 0x%x", rc);
        asn_free_value(recv_context->nds);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        free(recv_context);
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_stop(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap)
{
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRRECV stop handle %d\n", csap);
    return -1;
#else
    csap_p csap_descr_p;

    VERB("%s: CSAP %d", __FUNCTION__, csap);

    check_init();

    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        ERROR("%s: CSAP %d not exists", __FUNCTION__, csap);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap_descr_p);

    if (csap_descr_p->command & TAD_OP_RECV)
    {
        csap_descr_p->command |= TAD_COMMAND_STOP; 
        CSAP_DA_UNLOCK(csap_descr_p);

        if (csap_descr_p->state & TAD_STATE_FOREGROUND)
        {
            /* With some probability number of received packets, 
             * reported here, may be wrong. */
            SEND_ANSWER("0 %u", csap_descr_p->num_packets); 
        }
    }
    else
    {
        F_ERROR("%s: inappropriate command, CSAP %d is not receiving; "
                "command %x; state %x ", __FUNCTION__, csap_descr_p->id, 
                (int)csap_descr_p->command, (int)csap_descr_p->state);
        CSAP_DA_UNLOCK(csap_descr_p);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, ETADCSAPSTATE));
    }
    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_wait(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap)
{
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(csap);

    VERB("TRRECV wait: handle %d \n", csap);
    
    return -1;
#else
    csap_p csap_descr_p;

    VERB("CSAP %d", csap);

    check_init();

    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        VERB("CSAP not exists");
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap_descr_p);

    if (csap_descr_p->command & TAD_OP_RECV)
    {
        csap_descr_p->command |= TAD_COMMAND_WAIT; 
        CSAP_DA_UNLOCK(csap_descr_p);
    }
    else
    {
        ERROR("Inappropriate command, CSAP %d is not receiving; "
              "command %x; state %x ",
              csap_descr_p->id, 
              (int)csap_descr_p->command, (int)csap_descr_p->state);
        CSAP_DA_UNLOCK(csap_descr_p);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, ETADCSAPSTATE));
    }
    return 0;
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_get(struct rcf_comm_connection *handle,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap)
{
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRRECV get handle %d\n", csap);
    return -1;
#else
    csap_p      csap_descr_p;

    check_init();

    VERB("trrecv_get for CSAP %d", csap);

    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        F_ERROR("CSAP #%d not exists", csap);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap_descr_p);
    if (csap_descr_p->command & TAD_OP_RECV)
    {
        csap_descr_p->command |= TAD_COMMAND_GET;
        CSAP_DA_UNLOCK(csap_descr_p);
    }
    else
    {
        ERROR("Inappropriate command, CSAP %d is not receiving; "
              "command %x; state %x ", csap_descr_p->id, 
              (int)csap_descr_p->command, (int)csap_descr_p->state);
        CSAP_DA_UNLOCK(csap_descr_p);

        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, ETADCSAPSTATE));
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_csap_param(struct rcf_comm_connection *handle,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap, const char *param)
{
#ifdef DUMMY_TAD
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("CSAP param: handle %d param <%s>\n", csap, param);
    return -1;
#else
    csap_p csap_descr_p;

    check_init();
    VERB("CSAP param: handle %d param <%s>\n", csap, param);
    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        VERB("CSAP not exists");
        SEND_ANSWER("%d Wrong CSAP id", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
    }
    else if (strcmp(param, CSAP_PARAM_STATUS) == 0)
    {
        tad_csap_status_t status = 0;

        CSAP_DA_LOCK(csap_descr_p);
        if (csap_descr_p->command == 0)
            status = CSAP_IDLE;
        else if (csap_descr_p->state & TAD_STATE_COMPLETE)
        {
            if (csap_descr_p->last_errno)
                status = CSAP_ERROR;
            else
                status = CSAP_COMPLETED;
        }
        else
            status = CSAP_BUSY;

        F_VERB("CSAP get_param, status %d, command flag %x\n", 
               (int)status, (int)csap_descr_p->command);
        CSAP_DA_UNLOCK(csap_descr_p);

        SEND_ANSWER("0 %d", (int)status);
    }
    else if (strcmp(param, CSAP_PARAM_TOTAL_BYTES) == 0)
    {
        VERB("CSAP get_param, get total bytes %d\n", 
             csap_descr_p->total_bytes);
        SEND_ANSWER("0 %u", csap_descr_p->total_bytes);
    }
    else if (strcmp(param, CSAP_PARAM_FIRST_PACKET_TIME) == 0)
    {
        VERB("CSAP get_param, get first pkt, %u.%u\n",
                                (uint32_t)csap_descr_p->first_pkt.tv_sec, 
                                (uint32_t)csap_descr_p->first_pkt.tv_usec);
        SEND_ANSWER("0 %u.%u",  (uint32_t)csap_descr_p->first_pkt.tv_sec, 
                                (uint32_t)csap_descr_p->first_pkt.tv_usec);
    }
    else if (strcmp(param, CSAP_PARAM_LAST_PACKET_TIME) == 0)
    {
        VERB("CSAP get_param, get last pkt, %u.%u\n",
                                (uint32_t)csap_descr_p->last_pkt.tv_sec, 
                                (uint32_t)csap_descr_p->last_pkt.tv_usec);
        SEND_ANSWER("0 %u.%u",  (uint32_t)csap_descr_p->last_pkt.tv_sec, 
                                (uint32_t)csap_descr_p->last_pkt.tv_usec);
    }
    else if (csap_descr_p->get_param_cb == NULL)
    {
        VERB("CSAP does not support get_param\n");
        SEND_ANSWER("%d CSAP does not support get_param", 
                    TE_RC(TE_TAD_CH, ETENOSUPP));
    }
    else
    {
        int layer = csap_descr_p->read_write_layer;
        char *param_value = csap_descr_p->get_param_cb[layer](csap_descr_p,
                                                              layer, param);
        if (param_value)
        {
            VERB("got value: <%s>\n", param_value);
            SEND_ANSWER("0 %s", param_value);
            free(param_value);
        }
        else
        {
            VERB("CSAP return error for get_param\n");
            SEND_ANSWER("%d CSAP return error for get_param", 
                        TE_RC(TE_TAD_CH, ETENOSUPP));
        }
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_recv(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen, csap_handle_t csap,
                   te_bool results, unsigned int timeout)
{ 
#ifdef DUMMY_TAD 
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("TRSEND recv: handle %d timeout %d %s\n",
                        csap, timeout, results ? "results" : "");
    return -1;
#else
    csap_p csap_descr_p;

    UNUSED(cmdlen);

    VERB("CSAP %d", csap);

    check_init(); 

    if ((csap_descr_p = csap_find(csap)) == NULL)
    {
        SEND_ANSWER("%d Wrong CSAP id", TE_RC(TE_TAD_CH, ETADCSAPNOTEX));
        return 0;
    }
    CSAP_DA_LOCK(csap_descr_p);
    if (csap_descr_p->command)
    {
        CSAP_DA_UNLOCK(csap_descr_p);
        SEND_ANSWER("%d Specified CSAP is busy", 
            TE_RC(TE_TAD_CH, ETADCSAPSTATE));
        return 0;
    }
    CSAP_DA_UNLOCK(csap_descr_p);

    return rcf_ch_trrecv_start(handle, cbuf, buflen, answer_plen, ba,
                               cmdlen, csap, 1 /* one packet */, 
                               results, timeout);
#endif
}


