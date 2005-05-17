/** @file
 * @brief Test Environment
 *
 * Definition of the C API provided by RCF to TE subsystems and tests
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_RCF_API_H__
#define __TE_RCF_API_H__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_defs.h"
#include "rcf_common.h"
#include "tad_common.h"


/** 
 * List of modes that can be used while calling RCF functions especially
 * for those that deal with traffic application domain
 */
typedef enum rcf_call_mode {
    RCF_MODE_NONBLOCKING, /**< Used for non-blocking calls */
    RCF_MODE_BLOCKING, /**< Used for blocking call */
} rcf_call_mode_t;


#ifdef __cplusplus
extern "C" {
#endif

/* All routines block caller until response from RCF */

/**
 * This function returns list of Test Agents (names) running.
 *
 * @param buf           location for the name list; 
 *                      names are put to the buffer one-by-one and are
 *                      separated by "'\0'"; the list is finished by 
 *                      "'\0'" as well
 *
 * @param len           pointer to length variable (should be set
 *                      to the length the buffer by caller;
 *                      is filled to amount of space used for the name list
 *                      by the routine)
 *          
 *
 * @return error code
 *
 * @retval 0            success
 * @retval ETESMALLBUF  the buffer is too small
 * @retval ETEIO        cannot interact with RCF 
 * @retval ENOMEM       out of memory
 */
extern int rcf_get_ta_list(char *buf, size_t *len);

/**
 * This function maps name of the Test Agent to its type.
 *
 * @param ta_name       Test Agent name                 
 * @param ta_type       Test Agent type location (should point to
 *                      the buffer at least of RCF_MAX_NAME length)
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running Test Agent is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval ENOMEM       out of memory
 */
extern int rcf_ta_name2type(const char *ta_name, char *ta_type);

/**
 * This function returns unique session identifier for the Test Agent.
 * This session identifier may be passed to all other TA-related routines.
 * Command with session identifier X will be passed to the Test Agent 
 * before obtaining of answer on the command with session identifier Y
 * (X != Y). However if the Test Agent does not support simultaneous 
 * processing of several commands, this mechanism does not give any
 * advantage.
 *
 * @param ta_name       Test Agent name                 
 * @param session       location for the session identifier
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running Test Agent is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval ENOMEM       out of memory
 */
extern int rcf_ta_create_session(const char *ta_name, int *session);

/**
 * This function reboots the Test Agent or NUT served by it. It sends 
 * "reboot" command to the Test Agent, calls reboot function provided by 
 * RCF TA-specific library, restarts the TA and re-connects to it.
 * The function may be called by Configurator only. 
 * It is prohibited to call the function for the TA running on the Testing
 * Node.
 *
 * @param ta_name       Test Agent name              
 * @param boot_params   complete parameter string to be passed to the TA
 *                      in the "reboot" command and to the reboot function
 *                      of RCF TA-specific library (without quotes) or NULL
 *
 * @param image         name of the bootable image (without path) to be 
 *                      passed to the Test Agent as binary attachment or
 *                      NULL (it is assumed that NUT images are located in
 *                      ${TE_INSTALL_NUT}/bin or ${TE_INSTALL}/nuts/bin)
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running or located on TN Test Agent 
 *                      is provided or parameter string is too long
 * @retval ETEIO        cannot interact with RCF 
 * @retval EINPROGRESS  operation is in progress
 * @retval ENOENT       cannot open NUT image file
 * @retval ENOMEM       out of memory
 */
extern int rcf_ta_reboot(const char *ta_name, const char *boot_params,
                         const char *image);

/**
 * This function is used to obtain value of object instance by its
 * identifier.  The function may be called by Configurator only. 
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 * @param val_buf       location for the object instance value 
 * @param len           location length
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or OID string is
 *                      too long
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETESMALLBUF  the buffer is too small
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_cfg_get(const char *ta_name, int session,
                          const char *oid, 
                          char *val_buf, size_t len);

/**
 * This function is used to change value of object instance.
 * The function may be called by Configurator only. 
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 * @param val           object instance value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or too OID or value
 *                      strings are too long
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_cfg_set(const char *ta_name, int session,
                          const char *oid, const char *val);

/**
 * This function is used to create new object instance and assign the 
 * value to it.
 * The function may be called by Configurator only. 
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 * @param val           object instance value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or too OID or value
 *                      strings are too long
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_cfg_add(const char *ta_name, int session,
                          const char *oid, const char *val);

/**
 * This function is used to remove the object instance.
 * The function may be called by Configurator only. 
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or OID string is
 *                      too long
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_cfg_del(const char *ta_name, int session,
                          const char *oid);

/**
 * This function is used to begin/finish group of configuration commands.
 * The function may be called by Configurator only.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param is_start      Is start(TRUE) or end(FALSE) of a group?
 *
 * @return error code
 *
 * @retval 0            success
 * @retval ETEIO        cannot interact with RCF 
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_cfg_group(const char *ta_name, int session,
                            te_bool is_start);

/**
 * This function is used to get bulk of log from the Test Agent.
 * The function may be called by Logger only. 
 *
 * @param ta_name       Test Agent name              
 * @param log_file      name of the local file where log should be put
 *                      (the file is truncated to zero length before
 *                      updating)
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or bad file name 
 *                      are provided 
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_get_log(const char *ta_name, char *log_file);

/**
 * This function is used to obtain value of the variable from the Test Agent
 * or NUT served by it.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param var_name      name of the variable to be read 
 * @param var_type      type according to which string returned from the TA
 *                      should be converted
 * 
 * @param var_len       length of the location buffer
 * @param val           location for variable value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       one of arguments is invalid (NULL, too long or
 *                      has inappropriate value)
 * @retval ENOENT       no such variable
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETESMALLBUF  the buffer is too small
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_get_var(const char *ta_name, int session,
                          const char *var_name, int var_type,
                          size_t var_len, void *val);

/**
 * This function is used to change value of the variable from the Test Agent
 * or NUT served by it.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param var_name      name of the variable to be changed
 * @param var_type      type according to which variable value should
 *                      appear in the protocol command
 *
 * @param val           new value of the variable
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       one of arguments is invalid (NULL, too long or
 *                      has inappropriate value)
 * @retval ENOENT       no such variable
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_set_var(const char *ta_name, int session,
                          const char *var_name, 
                          int var_type, const char *val);

/**
 * This function loads file from Test Agent or NUT served by it to the 
 * testing node.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param rfile         full name of the file in the TA/NUT file system
 * @param lfile         full name of the file in the TN file system
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent, non-existent
 *                      session identifier or bad file name are provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval ENOENT       no such file
 * @retval EPERM        operation not permitted
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 */
extern int rcf_ta_get_file(const char *ta_name, int session, 
                           const char *rfile, const char *lfile);

/**
 * This function loads file from the testing node to Test Agent or NUT 
 * served by it.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param lfile         full name of the file in the TN file system
 * @param rfile         full name of the file in the TA/NUT file system
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent, non-existent
 *                      session identifier or bad file name are provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval ENOENT       no such file
 * @retval EPERM        operation not permitted
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 */
extern int rcf_ta_put_file(const char *ta_name, int session, 
                           const char *lfile, const char *rfile);

/**
 * This function deletes file from the Test Agent or NUT served by it.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param rfile         full name of the file in the TA/NUT file system
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent, non-existent
 *                      session identifier or bad file name are provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval ENOENT       no such file
 * @retval EPERM        operation not permitted
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 */
extern int rcf_ta_del_file(const char *ta_name, int session,
                           const char *rfile);

/**
 * This function creates CSAP (communication service access point) on the 
 * Test Agent. 
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param stack_id      protocol stack identifier
 * @param params        parameters necessary for CSAP creation (string
 *                      or file name); if file with *params name exists,
 *                      it is attached to the CSAP creation command as
 *                      a binary attachment; otherwise the string is
 *                      appended to the command
 *
 * @param csap_id       location for unique CSAP handle
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 *
 * @sa rcf_ta_csap_destroy
 */
extern int rcf_ta_csap_create(const char *ta_name, int session,
                              const char *stack_id, const char *params,
                              csap_handle_t *csap_id);

/**
 * This function destroys CSAP.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param csap_id       CSAP handle
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval EBADF        no such CSAP
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 *
 * @sa rcf_ta_csap_create
 */
extern int rcf_ta_csap_destroy(const char *ta_name, int session,
                               csap_handle_t csap_id);

/**
 * This function is used to obtain CSAP parameter value.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param csap_id       CSAP handle
 * @param var           parameter name
 * @param var_len       length of the location buffer
 * @param val           location for variable value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval EBADF        no such CSAP
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ETESMALLBUF  the buffer is too small
 */
extern int rcf_ta_csap_param(const char *ta_name, int session,
                             csap_handle_t csap_id,
                             const char *var, size_t var_len, char *val);

/**
 * This function is used to force sending of traffic via already created
 * CSAP.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param csap_id       CSAP handle
 * @param templ         full name of the file with traffic template
 * @param blk_mode      mode of the operation:
 *                      in blocking mode it suspends the caller
 *                      until sending of all the traffic finishes
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval EBADF        no such CSAP
 * @retval EINPROGRESS  operation is already in progress
 * @retval EBUSY        CSAP is used for receiving now
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 *
 * @se It may block caller according to "blk_mode" parameter value
 *
 * @sa rcf_ta_trsend_stop
 */
extern int rcf_ta_trsend_start(const char *ta_name, int session, 
                               csap_handle_t csap_id, const char *templ,
                               rcf_call_mode_t blk_mode);

/**
 * This function is used to stop sending of traffic started by
 * rcf_ta_trsend_start called in non-blocking mode.
 *
 * @param ta_name       Test Agent name                 
 * @param session       session identifier
 * @param csap_id       CSAP handle
 * @param num           location where number of sent packets should be
 *                      placed
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval EBADF        no such CSAP
 * @retval EALREADY     traffic sending is not in progress now
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 *
 * @sa rcf_ta_trsend_start
 */
extern int rcf_ta_trsend_stop(const char *ta_name, int session,
                              csap_handle_t csap_id, int *num);

/** Function - handler of received packets */
typedef void (*rcf_pkt_handler)(
    char *pkt,         /**< File name where received packet was saved */
    void *user_param   /**< Parameter provided by caller of 
                            rcf_ta_trrecv_start */
);

/**
 * This function is used to force receiving of traffic via already created
 * CSAP. 
 *
 * @param ta_name      - Test Agent name.
 * @param session      - TA session or 0.
 * @param csap_id      - CSAP handle.
 * @param pattern      - Full name of the file with traffic pattern.
 * @param handler      - Address of function to be used to process
 *                       received packets or NULL.
 * @param user_param   - User parameter to be passed to the handler.
 * @param timeout      - Timeout for traffic receive operation. After this
 *                       time interval CSAP stops capturing any traffic on
 *                       the agent and will be waiting until
 *                       rcf_ta_trrecv_stop or rcf_ta_trrecv_wait are
 *                       called.
 * @param num          - Number of packets that needs to be captured;
 *                       if it is zero, the number of received packets
 *                       is not limited.
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ETEIO        cannot interact with RCF
 * @retval EBADF        no such CSAP
 * @retval EINPROGRESS  operation is already in progress
 * @retval EBUSY        CSAP is used for sending now
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       not enough memory in the system
 * @retval other        error returned by command handler on the TA
 *
 * @sa rcf_ta_trrecv_stop rcf_ta_trrecv_wait
 */
extern int rcf_ta_trrecv_start(const char *ta_name, int session,
                               csap_handle_t csap_id, const char *pattern,
                               rcf_pkt_handler handler, void *user_param, 
                               unsigned int timeout, int num);

/**
 * Blocks the caller until all the traffic, which is being captured,
 * is received by the CSAP or timeout occurres (timeout specified 
 * with rcf_ta_trrecv_start).
 * If handler was specified in rcf_ta_trrecv_start function,
 * it is called for all received packets.
 * This function can only be called after rcf_ta_trrecv_start.
 *
 * @param ta_name   Test Agent name.
 * @param session   session identifier.
 * @param csap_id   CSAP handle.
 * @param num       Number of received packets (OUT).
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_RC(TE_TAD_CSAP, ETIMEDOUT)
 *                      timeout occured before all the requested traffic
 *                      received - this means that TAD has captured less
 *                      than the number of packets specified with
 *                      rcf_ta_trrecv_start.
 *                      Nevertheless, number of captured packets is set to 
 *                      "num" parameter.
 *
 * @sa rcf_ta_trrecv_start
 */
extern int rcf_ta_trrecv_wait(const char *ta_name, int session,
                              csap_handle_t csap_id, int *num);

/**
 * This function is used to stop receiving of traffic started by
 * rcf_ta_trrecv_start.
 * If handler was specified in the function rcf_ta_trrecv_start, 
 * it is called for all received packets.
 *
 * @param ta_name       Test Agent name                 
 * @param session       session identifier
 * @param csap_id       CSAP handle
 * @param num           location where number of received packets 
 *                      should be placed
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval EBADF        no such CSAP
 * @retval EALREADY     traffic receiving is not in progress now
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 *
 * @sa rcf_ta_trrecv_start
 */
extern int rcf_ta_trrecv_stop(const char *ta_name, int session,
                              csap_handle_t csap_id, int *num);

/**
 * This function is used to force processing of received packets 
 * without stopping of traffic receiving (handler specified in
 * rcf_ta_trrecv_start is used for packets processing).
 *
 * @param ta_name       Test Agent name                 
 * @param session       session identifier
 * @param csap_id       CSAP handle
 * @param num           location where number of processed packets 
 *                      should be placed
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ETEIO        cannot interact with RCF 
 * @retval EBADF        no such CSAP
 * @retval EALREADY     traffic receiving is not in progress now
 * @retval ENODATA      no data available on TA, because handler was not
 *                      specified in rcf_ta_trrecv_start
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 *
 * @sa rcf_ta_trrecv_start
 */
extern int rcf_ta_trrecv_get(const char *ta_name, int session,
                             csap_handle_t csap_id, int *num);

/**
 * This function is used to send exactly one packet via CSAP and receive
 * an answer (it may be used for CLI, SNMP, ARP, ICMP, DNS, etc.)
 * This function blocks the caller until the packet is received by 
 * traffic application domain or timeout occures.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param csap_id       CSAP handle
 * @param templ         Full name of the file with traffic template
 * @param handler       Callback function used in processing of
 *                      received packet or NULL
 * @param user_param    User parameter to be passed to the handler
 * @param timeout       Timeout for answer waiting
 * @param error         Location for protocol-specific error extracted
 *                      from the answer or NULL (if error is 0, then answer
 *                      is positive; if error is -1, then it's not possible
 *                      to extract the error)
 *
 * @return error code
 *
 * @retval 0            success
 * @retval ETIMEDOUT    timeout occured before a packet that matches
 *                      the template received
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or flag blocking
 *                      is used together with num 0
 * @retval ETEIO        cannot interact with RCF 
 * @retval EBADF        no such CSAP
 * @retval EBUSY        CSAP is busy
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern int rcf_ta_trsend_recv(const char *ta_name, int session,
                              csap_handle_t csap_id, 
                              const char *templ, rcf_pkt_handler handler, 
                              void *user_param, unsigned int timeout,
                              int *error);

/**
 * This function is used to call remotely a routine on the Test Agent or
 * NUT served by it.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param rtn           routine name
 * @param rc            location for the code returned by the routine
 * @param argc          number of parameters
 * @param argv          if TRUE, rest of parameters are char *;
 *                      otherwise pairs: type, value.
 *
 * Usage example:
 * rcf_ta_call("loki.oktetlabs.ru", 25, "foo_rtn", &rc, 3, TRUE, 
 *             str, "foo", "bar");
 * rcf_ta_call("thor", 13, "bar_rtn", &rc, 3, FALSE, 
 *             RCF_INT8, 7, RCF_STRING, "foo", RCF_STRING, arg)
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ENOENT       no such routine
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 */
extern int rcf_ta_call(const char *ta_name, int session,
                       const char *rtn, int *rc,
                       int argc, te_bool argv, ...);
                       
/**
 * This function is used to start a process on the Test Agent or 
 * NUT served by it.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param priority      priority of the new process
 * @param rtn           routine name (task entry point)
 * @param pid           location for process identifier
 * @param argc          number of parameters
 * @param argv          if TRUE, rest of parameters are char *;
 *                      otherwise pairs: type, value.
 *
 * Usage example:
 * rcf_ta_start_task("loki.oktetlabs.ru", 25, HIGH_PRIORITY,
 *                   "foo_entry_pont", &pid, 3, TRUE,
 *                   str, "foo", "bar");
 * rcf_ta_start_task("thor", 13, LOW_PRIORITY, "bar_entry_point",
 *                   &pid, 3, FALSE,
 *                   RCF_INT8, 7, RCF_STRING, "foo", RCF_STRING, arg)
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ENOENT       no such routine
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 *
 * @sa rcf_ta_kill_process
 */
extern int rcf_ta_start_task(const char *ta_name, int session,
                             int priority,  const char *rtn,
                             pid_t *pid, int argc, te_bool argv, ...);

/**
 * This function is similar to rcf_ta_start_task, but
 * the task should be started as a separate thread, not a process
 *
 * @sa rcf_ta_start_task
 */
extern int rcf_ta_start_thread(const char *ta_name, int session,
                               int priority,  const char *rtn,
                               int *tid, int argc, te_bool argv, ...);
                                
/**
 * This function is used to kill a process on the Test Agent or 
 * NUT served by it.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param pid           identifier of the process to be killed
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval ENOENT       no such process
 * @retval ETEIO        cannot interact with RCF 
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval ENOMEM       out of memory
 *
 * @sa rcf_ta_start_process
 */
extern int rcf_ta_kill_task(const char *ta_name, int session, pid_t pid);

/**
 * Call SUN RPC on the TA.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0
 * @param rpcserver     Name of the RPC server
 * @param timeout       RPC timeout in milliseconds or 0 (unlimited)
 * @param rpc_name      Name of the RPC (e.g. "bind")
 * @param in            Input parameter C structure
 * @param in            Output parameter C structure
 *
 * @return Status code
 */
extern int rcf_ta_call_rpc(const char *ta_name, int session, 
                           const char *rpcserver, int timeout,
                           const char *rpc_name, void *in, void *out);

/**
 * Clean up resources allocated by RCF API.
 * This routine should be called from each thread used RCF API.
 *
 * Usually user should not worry about calling of the function, since
 * it is called automatically using atexit() or similar mechanism.
 */
extern void rcf_api_cleanup(void);

/**
 * This function is used to check that all running are still
 * working.
 *
 * @return error code
 *
 * @retval 0            success
 * @retval ETAREBOOTED  if at least one test agent has been normally
 *                      rebooted
 * @retval ETADEAD      if at least one agent was dead
 * @retval ETEIO        cannot interact with RCF 
 * 
 */
extern int rcf_check_agents(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RCF_API_H__ */
