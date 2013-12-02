/** @file
 * @breaf TE TAPI for Forwarder additional module implementations
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */


#include "te_config.h"

#define TE_LGR_USER "TAPI Forwarder" 


#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h" 
#include "tapi_forw.h"
#include "rcf_api.h"
#include "asn_usr.h"
#include "ndn_forw.h" 
#include "logger_api.h"

#define FORW_TASK_ADD_FNAME     "tadf_add_forw_task"
#define FORW_TASK_PKT           "tadf_forw_packet"

/**
 * Create forwarding task according with specification. 
 *
 * @param ta            name of TA, on which forw. task should 
 *                      be added
 * @param sid           RCF session ID. 
 * @param forw_action   plain C structure with specification of 
 *                      forwarder task
 * @param sendq_id      Id of destination sending queue
 * 
 * @return zero on success or error code. 
 */
int 
tapi_forw_task_create(const char *ta, int sid,
                      const ndn_forw_action_plain *forw_action,
                      int sendq_id)
{
    char fa_buffer[1000];
    int  rc = 0;
    int  num = 0;
    int  rc_ta_call = 0;

    asn_value *fa_asn;

    if (ta == NULL || forw_action == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    do {
        rc = ndn_forw_action_plain_to_asn(forw_action, &fa_asn);
        if (rc != 0)
            break;

        num = asn_sprint_value(fa_asn, fa_buffer, sizeof(fa_buffer), 0);
        if (num < 0)
        { rc = TE_EASNGENERAL; break; } 

        VERB("%s, buffer with forw action ASN spec: --\n%s", 
                __FUNCTION__, fa_buffer);

        rc = rcf_ta_call(ta, sid, FORW_TASK_ADD_FNAME, &rc_ta_call, 2, FALSE, 
                         RCF_STRING, fa_buffer, RCF_INT32, sendq_id);
        if (rc != 0)
        {
            ERROR("rcf call for %s failed: %r", FORW_TASK_ADD_FNAME, rc);
            break;
        }

        if (rc_ta_call)
        {
            ERROR("work of %s failed: %r", FORW_TASK_ADD_FNAME, rc_ta_call);
            rc = rc_ta_call;
            break; 
        } 
    } while(0); 

    return TE_RC(TE_TAPI, rc);
}


/**
 * Create send queue with specified parameters.
 *
 * @param ta            name of TA, on which forw. task should 
 *                      be added
 * @param sid           RCF session ID. 
 * @param csap          ID of CSAP, which should send data  
 * @param band          Bandwidth of send queue, zero for unlimited
 * @param bufsize       Buffer size of send queue
 * @param sendq_id      location for id of destination sending queue (OUT)
 * 
 * @return zero on success or error code. 
 */
int 
tapi_forw_sendq_create(const char *ta,  int sid,
                       csap_handle_t csap, int band, int bufsize, 
                       int *sendq_id)
{ 
    int rc;

    rc = rcf_ta_call(ta, sid, "tadf_sendq_create", sendq_id, 3, FALSE, 
                     RCF_INT32, csap, RCF_INT32, band, RCF_INT32, bufsize);

    if (rc == 0 && (*sendq_id) < 0)
        rc = TE_EFAULT;
    
    return TE_RC(TE_TAPI, rc); 
}

/**
 * Destroy send queue with specified ID.
 *
 * @param ta            name of TA, on which send queue is running. 
 * @param sid           RCF session ID. 
 * @param sendq_id      id of sending queue 
 * 
 * @return zero on success or error code. 
 */
int 
tapi_forw_sendq_destroy(const char *ta, int sid, int sendq_id)
{ 
    int rc = 0;
    int func_rc = 0;
    
    rc = rcf_ta_call(ta, sid, "tadf_sendq_destroy", &func_rc, 1, FALSE, 
                     RCF_INT32, sendq_id); 
    
    if (rc == 0)
        return TE_RC(TE_TAPI, func_rc); 

    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_forw.h */
int 
tapi_forw_task_destroy(const char *ta, int sid, const char *ftask_name)
{
    int rc;
    int func_rc = 0;

    rc = rcf_ta_call(ta, sid, "tadf_del_forw_task", &func_rc, 1, FALSE, 
                     RCF_STRING, ftask_name); 

    RING("forw task '%s' destroy, rcf call rc %r, frow_task delete rc %r", 
         ftask_name, rc, func_rc); 
    
    if (rc == 0)
        return TE_RC(TE_TAPI, func_rc); 

    return TE_RC(TE_TAPI, rc);
}



/**
 * Set forwarding task parameter.
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID.
 * @param ftask_name Name of forwarder task.
 * @param param      Parameter name, which should be ASN lib labels string,
 *                   see ASN lib docs and NDN specification for 
 *                   Forwarder-Action ASN type. 
 * @param val        New parameter value.
 *
 * @return zero on success or error code.
 */
int 
tapi_forw_task_set_param(const char *ta, int sid, const char *ftask_name,
                         const char *param, int val)
{
    int rc;
    int rc_ta_call = 0;
    
    if (ta == NULL || ftask_name == NULL || param == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    rc = rcf_ta_call(ta, sid, "tadf_forw_task_set_param", 
                     &rc_ta_call, 3, FALSE,
                     RCF_STRING, ftask_name, 
                     RCF_STRING, param, RCF_INT32, val);
    if (rc)
    {
        ERROR("%s: rcf_ta_call failed with rc %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (rc_ta_call)
    {
        ERROR("%s: set forw_task %s parameter %s to value %d, failed %r", 
              __FUNCTION__, ftask_name, param, val, rc_ta_call);
        return TE_RC(TE_TAPI, rc_ta_call);;
    }

    INFO("%s: set forw task %s parameter %s to value %d success", 
          __FUNCTION__, ftask_name, param, val);

    return 0;
}

/* See description in tapi_forw.h */
int 
tapi_forw_task_set_drop_rate(const char *ta, int sid,
                             const char *ftask_name, int rate)
{ 
    return tapi_forw_task_set_param(ta, sid, ftask_name, 
                                    "drop.#random-rate", rate);
}

/* See description in tapi_forw.h */
int 
tapi_forw_task_set_delay_min(const char *ta, int sid,
                             const char *ftask_name, int delay)
{
    return tapi_forw_task_set_param(ta, sid, ftask_name, 
                                    "delay.#cont.delay-min", delay);
}


/* See description in tapi_forw.h */
int 
tapi_forw_task_set_delay_max(const char *ta, int sid,
                             const char *ftask_name, int delay)
{
    return tapi_forw_task_set_param(ta, sid, ftask_name, 
                                    "delay.#cont.delay-max", delay);
}

/* See description in tapi_forw.h */
int
tapi_forw_task_set_reorder_type(const char *ta, int sid,
                                const char *ftask_name,
                                ndn_forw_reorder_type_t type)
{
    return tapi_forw_task_set_param(ta, sid, ftask_name, 
                                    "reorder.type", type);
}

/* See description in tapi_forw.h */
int
tapi_forw_task_set_reorder_to(const char *ta, int sid,
                              const char *ftask_name, int to)
{
    return tapi_forw_task_set_param(ta, sid, ftask_name, 
                                    "reorder.timeout.#plain", to);
}

/* See description in tapi_forw.h */
int
tapi_forw_task_set_reorder_sz(const char *ta, int sid,
                              const char *ftask_name, int sz)
{
    return tapi_forw_task_set_param(ta, sid, ftask_name, 
                                    "reorder.reorder-size.#plain", sz);
}


/**
 * Set send queue parameters.
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID.
 * @param param      Parameter name in human form ("size_max" or
 *                   "bandwidth")
 * @param val        New parameter value.
 * @param sendq_id   ID of the send queue.
 *
 * @return zero on success or error code.
 */
int 
tapi_forw_sendq_set_param(const char *ta, int sid,
                          const char *param, int val, int sendq_id)
{
    int rc;
    int rc_ta_call = 0;
    
    rc = rcf_ta_call(ta, sid, "tadf_sendq_set_param", 
                     &rc_ta_call, 3, FALSE,
                     RCF_INT32, sendq_id, 
                     RCF_STRING, param, RCF_INT32, val);
    if (rc)
    {
        ERROR("%s: rcf_ta_call failed with rc %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (rc_ta_call)
    {
        ERROR("%s: set sendq %d parameter %s to value %d, failed %r", 
              __FUNCTION__, sendq_id, param, val, rc_ta_call);
        return TE_RC(TE_TAPI, rc_ta_call);
    }

    INFO("%s: set sendq %d parameter %s to value %d success", 
          __FUNCTION__, sendq_id, param, val);

    return 0;
}



/**
 * Get send queue parameters.
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID.
 * @param param      Parameter name in human form ("size_max" or
 *                   "bandwidth")
 * @param sendq_id   ID of the send queue.
 * @param val        Value of the parameter (OUT)
 *
 * @return zero on success or error code.
 */
int 
tapi_forw_sendq_get_param(const char *ta, int sid,
                          const char *param, int sendq_id, int *val)
{
    int rc;
    
    rc = rcf_ta_call(ta, sid, "tadf_sendq_get_param", val, 2, FALSE, 
                     RCF_INT32, sendq_id, RCF_STRING, param);
    return TE_RC(TE_TAPI, rc);
}



/**
 * Set forw task name to pattern unit
 *
 * @param pat_unit   ASN value of type Traffic-Pattern-Unit
 * @param task_name  Name of forwarder task.
 *
 * @return zero on success or error code.
 */
int 
tapi_forw_set_ftask_name(asn_value *pat_unit, const char *task_name)
{
    /* '+ 2' for colon and tailing zero */
    size_t buf_len = strlen(FORW_TASK_PKT) + strlen(task_name) + 2;
    char *val_buffer = malloc(buf_len); 
    int syms;
    int rc = 0;

    if (pat_unit == NULL || task_name == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    syms = snprintf(val_buffer, buf_len, FORW_TASK_PKT ":%s", task_name);

    rc = asn_write_value_field(pat_unit, val_buffer, syms, 
                               "actions.0.#function");
    free(val_buffer);

    return TE_RC(TE_TAPI, rc);
}



