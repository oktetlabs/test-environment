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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define TE_LGR_USER     "TAD SNMP"
#include "logger_ta.h"

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <string.h>

#include "tad_snmp_impl.h"


#undef SNMPDEBUG

#define NEW_SNMP_API 1

#ifdef SNMPDEBUG
void 
print_oid(unsigned long * subids, int len)
{
    int i;

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
    {
        struct variable_list *vars, *t_vars = NULL;
        spec_data->pdu = snmp_clone_pdu(pdu); 

        for (vars = pdu->variables; vars; vars = vars->next_variable)
        {
            if (vars == pdu->variables)
                t_vars = spec_data->pdu->variables = snmp_clone_varbind(vars);
            else 
                t_vars = t_vars->next_variable = snmp_clone_varbind(vars); 
#ifdef SNMPDEBUG
            printf ("\nvar :");
            print_oid( vars->name, vars->name_length);
            printf ("\ntype: %d, val: ", vars->type);
#endif
        }
    }


    if ( op == TIMED_OUT )
    {
        ;
    }
    return 1; 
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
snmp_read_cb (csap_p csap_descr, int timeout, char *buf, int buf_len)
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

    spec_data = (snmp_csap_specific_data_p) csap_descr->layer_data[layer]; 
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
    
    if (spec_data->pdu) snmp_free_pdu (spec_data->pdu); 
    spec_data->pdu = 0;

    rc = select(n_fds, &fdset, 0, 0, &sel_timeout);
    VERB("%s(): CSAP %d, after select, rc %d\n",
         __FUNCTION__, csap_descr->id, rc);

    if (rc > 0) 
    { 
        size_t n_bytes = buf_len;
        
        snmp_read(&fdset);
        
        if (buf_len < sizeof(struct snmp_pdu))
        {
            RING("In %s, buf_len %d less then sizeof struct snmp_pdu %d", 
                 __FUNCTION__, buf_len, sizeof(struct snmp_pdu));
            n_bytes = buf_len;
        }
        else
            n_bytes = sizeof(struct snmp_pdu);

        if (spec_data->pdu)
        {
            memcpy (buf, spec_data->pdu, n_bytes);
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
snmp_write_cb (csap_p csap_descr, char *buf, int buf_len)
{
    int layer;

    snmp_csap_specific_data_p   spec_data;
    struct snmp_session       * ss;
    struct snmp_pdu           * pdu = (struct snmp_pdu *) buf;

    VERB("write callback\n"); 
    UNUSED(buf_len);

    if (csap_descr == NULL) return -1;

    layer = csap_descr->read_write_layer;

    spec_data = (snmp_csap_specific_data_p) csap_descr->layer_data[layer]; 
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
                   char *w_buf, int w_buf_len,
                   char *r_buf, int r_buf_len)
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

    spec_data = (snmp_csap_specific_data_p) csap_descr->layer_data[layer]; 
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
        snmp_free_pdu(spec_data->pdu);

    spec_data->pdu = 0;

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
snmp_single_init_cb (int csap_id, const asn_value_p csap_nds, int layer)
{
    int      rc;
    char     community[COMMUNITY_MAX_LEN]; 
    char     snmp_agent[100]; 
    int      timeout;
    int      version;
    int      v_len;
    int      l_port = 0;
    int      r_port = 0;

    csap_p csap_descr;

    struct snmp_session         csap_session; 
    struct snmp_session       * ss; 

    snmp_csap_specific_data_p   snmp_spec_data;
    asn_value_p                 snmp_csap_spec;


    VERB("Init callback\n");

    if (!csap_nds || (csap_id <=0))
        return ETEWRONGPTR;

    snmp_csap_spec = asn_read_indexed(csap_nds, layer, "");

    v_len = sizeof(community);
    rc = asn_read_value_field(snmp_csap_spec, community, &v_len, "community.#plain");
    if (rc == EASNINCOMPLVAL) 
        strcpy (community, SNMP_CSAP_DEF_COMMUNITY);
    else if (rc) return rc;

    v_len = sizeof(timeout); 
    rc = asn_read_value_field(snmp_csap_spec, &timeout, &v_len, "timeout.#plain");
    if (rc == EASNINCOMPLVAL) 
        timeout = SNMP_CSAP_DEF_TIMEOUT;
    else if (rc) return rc;

    v_len = sizeof(version); 
    rc = asn_read_value_field(snmp_csap_spec, &version, &v_len, "version.#plain");
    if (rc == EASNINCOMPLVAL) 
        version = SNMP_CSAP_DEF_VERSION;
    else if (rc) return rc;

    v_len = sizeof(l_port); 
    rc = asn_read_value_field(snmp_csap_spec, &l_port, &v_len, "local-port.#plain");
    if (rc == EASNINCOMPLVAL) 
        l_port = SNMP_CSAP_DEF_LOCPORT;
    else if (rc) return rc;

    v_len = sizeof(r_port); 
    rc = asn_read_value_field(snmp_csap_spec, &r_port, &v_len, "remote-port.#plain");
    if (l_port == SNMP_CSAP_DEF_LOCPORT) 
    { 
        if (rc == EASNINCOMPLVAL) 
            r_port = SNMP_CSAP_DEF_REMPORT;
        else 
            if (rc) return rc;
    } else {
        r_port = 0;
        if (rc == 0) 
            RING("Init of SNMP CSAP, local port set to %d, "
                 "ignoring remote port", l_port);
    }

    v_len = sizeof(snmp_agent);
    rc = asn_read_value_field(snmp_csap_spec, snmp_agent, &v_len, "snmp-agent.#plain");
    if (rc == EASNINCOMPLVAL) 
    {
        if (l_port == SNMP_CSAP_DEF_LOCPORT)
            strcpy (snmp_agent, SNMP_CSAP_DEF_AGENT);
        else 
            snmp_agent[0] = '\0';
    }
    else if (rc) return rc; 

    csap_descr = csap_find(csap_id);
    snmp_spec_data = malloc (sizeof(snmp_csap_specific_data_t));

    memset(&csap_session, 0, sizeof(csap_session));

    
    if (csap_descr->check_pdus_cb == NULL)
        csap_descr->check_pdus_cb = snmp_single_check_pdus;

    csap_descr->write_cb         = snmp_write_cb; 
    csap_descr->read_cb          = snmp_read_cb; 
    csap_descr->write_read_cb    = snmp_write_read_cb; 
    csap_descr->read_write_layer = layer; 
    csap_descr->timeout          = 2000000; 

#if NEW_SNMP_API
    snmp_sess_init(&csap_session);
#endif

    csap_session.version     = version;
    csap_session.remote_port = r_port; 
    csap_session.local_port  = l_port; 
    csap_session.timeout     = timeout * 1000000L;
    csap_session.community   = malloc (strlen(community)+1);
    strcpy (csap_session.community, community);

    csap_session.community_len = strlen(community);

    if (strlen(snmp_agent))
    {
        csap_session.peername = malloc (strlen(snmp_agent)+1);
        strcpy (csap_session.peername, snmp_agent);
    }
    else
        csap_session.peername = NULL;

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
                (r_port && strlen(snmp_agent)) ? snmp_agent : "0.0.0.0", 
                r_port ? r_port : l_port);

        transport = netsnmp_tdomain_transport(buf, !r_port, "udp");
        if (transport == NULL)
        {
            ERROR("NULL transport!");
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
        ERROR("open session or transport error\n");

        snmp_perror("open session or transport error");

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

    csap_descr->layer_data[layer] = snmp_spec_data;
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
        (snmp_csap_specific_data_p) csap_descr->layer_data[layer]; 

    VERB("Destroy callback, id %d\n", csap_id);

    if (spec_data->ss)
    {
        struct snmp_session *s = spec_data->ss;

        snmp_close(s);
    }

    return 0;
}


