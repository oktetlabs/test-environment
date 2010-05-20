/** @file
 * @breaf TE TAPI for Forwarder additional module external declarations
 *
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_TAPI_FORW_H__
#define __TE_TAPI_FORW_H__ 


#include "te_defs.h"
#include "ndn_forw.h"
#include "tad_common.h"



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
extern int tapi_forw_task_create(const char *ta,  int sid,
                                 const ndn_forw_action_plain *forw_action,
                                 int sendq_id);

/**
 * Create send queue with specified parameters.
 *
 * @param ta            name of TA, on which forw. task should 
 *                      be added
 * @param sid           RCF session ID. 
 * @param csap          ID of CSAP, which should send data  
 * @param band          Bandwidth of send queue in bytes per second,
 *                      zero for unlimited
 * @param bufsize       Buffer size of send queue
 * @param sendq_id      location for id of destination sending queue (OUT)
 * 
 * @return zero on success or error code. 
 */
extern int tapi_forw_sendq_create(const char *ta,  int sid,
                                  csap_handle_t csap, int band,
                                  int bufsize, int *sendq_id);


/**
 * Destroy send queue with specified ID.
 *
 * @param ta            name of TA, on which send queue is running. 
 * @param sid           RCF session ID. 
 * @param sendq_id      id of sending queue 
 * 
 * @return zero on success or error code. 
 */
extern int tapi_forw_sendq_destroy(const char *ta, int sid, int sendq_id);

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
extern int tapi_forw_task_set_param(const char *ta, int sid,
                                    const char *ftask_name, 
                                    const char *param, int val);

/**
 * Set forwarding task drop rate.
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID.
 * @param ftask_name Name of forwarder task.
 * @param rate       New drop rate value.
 *
 * @return zero on success or error code.
 */
extern int tapi_forw_task_set_drop_rate(const char *ta, int sid,
                                        const char *ftask_name, int rate);

/**
 * Set forwarding task minimum delay.
 * Applicable for constant delay type. If delay was disabled, it becomes
 * 'constant' with specified value. 
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID
 * @param ftask_name Name of forwarder task
 * @param delay      New delay value in microseconds
 *
 * @return zero on success or error code.
 */
extern int tapi_forw_task_set_delay_min(const char *ta, int sid,
                                        const char *ftask_name, int delay);



/**
 * Set forwarding task reorder type
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID
 * @param ftask_name Name of forwarder task
 * @param type       New type value
 *
 * @return zero on success or error code.
 */
extern int tapi_forw_task_set_reorder_type(const char *ta, int sid,
                                           const char *ftask_name,
                                           ndn_forw_reorder_type_t type);


/**
 * Set forwarding task reorder timeout.
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID
 * @param ftask_name Name of forwarder task
 * @param to         New timeout value in microseconds
 *
 * @return zero on success or error code.
 */
extern int tapi_forw_task_set_reorder_to(const char *ta, int sid,
                                         const char *ftask_name, int to);


/**
 * Set forwarding task reorder size.
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID
 * @param ftask_name Name of forwarder task
 * @param sz         New reorder size value
 *
 * @return zero on success or error code.
 */
extern int tapi_forw_task_set_reorder_sz(const char *ta, int sid,
                                         const char *ftask_name, int sz);



/**
 * Set forwarding task maximum delay.
 * Applicable for constant delay type. If delay was disabled, it becomes
 * 'constant' with specified value. 
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID.
 * @param ftask_name Name of forwarder task.
 * @param delay      New delay value in microseconds.
 *
 * @return zero on success or error code.
 */
extern int tapi_forw_task_set_delay_max(const char *ta, int sid,
                                        const char *ftask_name, int delay);


/**
 * Destroy forwarding task.
 *
 * @param ta         name of TA, on which the send queue is situated
 * @param sid        RCF session ID.
 * @param ftask_name Name of forwarder task.
 *
 * @return zero on success or error code.
 */
extern int tapi_forw_task_destroy(const char *ta, int sid,
                                  const char *ftask_name);


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
extern int tapi_forw_sendq_set_param(const char *ta, int sid,
                                     const char *param, int val, 
                                     int sendq_id);

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
extern int tapi_forw_sendq_get_param(const char *ta, int sid,
                                     const char *param, int sendq_id, 
                                     int *val);


/**
 * Set forw task name to pattern unit
 *
 * @param pat_unit   ASN value of type Traffic-Pattern-Unit
 * @param task_name  Name of forwarder task.
 *
 * @return zero on success or error code.
 */
extern int tapi_forw_set_ftask_name(asn_value *pat_unit, 
                                    const char *task_name);

#endif /* __TE_TAPI_FORW_H__ */

