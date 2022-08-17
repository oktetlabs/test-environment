/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RCF Portable Command Handler
 *
 * Declaration of the C API provided by Commands Handler libraries to RCF
 * Portable Commands Handler.
 * Functions declared in this header should be defined
 * for each type of supported Test Agent (find definitions of these
 * functions under agent/<type> subdirectory).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RCF_CH_API_H__
#define __TE_RCF_CH_API_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "tad_common.h"
#include "rcf_common.h"
#include "rcf_internal.h"
#include "comm_agent.h"
#include "conf_oid.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup rcf_ch Test Agents: Command Handler
 * Common Handler (CH) is an interface to hide the details of Test Agent
 * implementation for Portable Command Handler (PCH).
 * When you support a new type of Test Agent you will need to implement
 * only these set of functions to let other TE components access
 * functionality exported by your Test Agent in generic way.
 *
 * Definition of these functions could be found under
 * @path{${TE_BASE}/agents/[agent type]} directory.
 *
 * @ingroup te_agents
 * @{
 */

/*
 * All functions may return -1 (unsupported). In this case no answer
 * is sent to TEN by handlers Portable Commands Handler processes
 * this situation.
 */

/** Generic routine prototype */
typedef te_errno (* rcf_rtn)(void *arg, ...);

/** Generic threaded routine prototype */
typedef te_errno (* rcf_thr_rtn)(void *sem, void *arg, ...);

/** argv/argc routine prototype */
typedef te_errno (* rcf_argv_rtn)(int argv, char **argc);

/** argv/argc threaded routine prototype */
typedef te_errno (* rcf_argv_thr_rtn)(void *sem, int argc, char **argv);


/**
 * Initialize structures.
 *
 * @return Status code
 */
extern int rcf_ch_init(void);


/**
 * Mutual exclusion lock access to data connection.
 */
extern void rcf_ch_lock(void);

/**
 * Unlock access to data connection.
 *
 * @attention To be asynchronous cancellation safe, unlock should work
 *            fine in not locked state.
 */
extern void rcf_ch_unlock(void);

/**
 * POSIX thread cancellation-unsafe lock access to data connection.
 */
#define RCF_CH_LOCK     rcf_ch_lock()

/**
 * POSIX thread cancellation-unsafe unlock access to data connection.
 */
#define RCF_CH_UNLOCK   rcf_ch_unlock()

#if HAVE_PTHREAD_H

/**
 * POSIX thread cancellation-safe lock access to data connection.
 *
 * @attention The macro uses pthread_cleanup_push() and, therefore,
 *            it has to occur together with RCF_CH_SAFE_UNLOCK in
 *            the same function, at the same level of block nesting.
 */
#define RCF_CH_SAFE_LOCK \
    pthread_cleanup_push((void (*)(void *))rcf_ch_unlock, NULL);    \
    rcf_ch_lock()

/**
 * POSIX thread cancellation-safe unlock access to data connection.
 *
 * @attention The macro uses pthread_cleanup_pop() and, therefore,
 *            it has to occur together with RCF_CH_SAFE_LOCK in
 *            the same function, at the same level of block nesting.
 */
#define RCF_CH_SAFE_UNLOCK \
    pthread_cleanup_pop(!0)

#endif /* !HAVE_PTHREAD_H */

/** @defgroup rcf_ch_reboot Command Handler: Reboot and shutdown support
 * @ingroup rcf_ch
 * @{
 */

/**
 * Shutdown the Test Agent (answer should be sent before shutdown).
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 *
 * @return Indication of command support
 *
 * @retval  0   command is supported
 * @retval -1   command is not supported
 */
extern int rcf_ch_shutdown(struct rcf_comm_connection *handle,
                           char *cbuf, size_t buflen,
                           size_t answer_plen);


/**
 * Reboot the Test Agent or NUT served by it (answer should be sent
 * before reboot).
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param ba            pointer to location of binary attachment
 *                      in the command buffer or NULL if no binary
 *                      attachment is provided
 * @param cmdlen        full length of the command including binary
 *                      attachment
 *
 * @param params        reboot parameters
 *
 *
 * @return Indication of command support
 *
 * @retval  0   command is supported
 * @retval -1   command is not supported
 */
extern int rcf_ch_reboot(struct rcf_comm_connection *handle,
                         char *cbuf, size_t buflen, size_t answer_plen,
                         const uint8_t *ba, size_t cmdlen,
                         const char *params);
/**@} <!-- END rcf_ch_reboot --> */

/**
 * @addtogroup rcf_ch_cfg
 * @{
 */

/** Configure operations */
typedef enum {
    RCF_CH_CFG_GET,
    RCF_CH_CFG_SET,
    RCF_CH_CFG_ADD,
    RCF_CH_CFG_DEL,
    RCF_CH_CFG_GRP_START,
    RCF_CH_CFG_GRP_END,
} rcf_ch_cfg_op_t;

/**
 * Configure the Test Agent or NUT served by it.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param ba            pointer to location of binary attachment
 *                      in the command buffer or NULL if no binary
 *                      attachment is provided
 * @param cmdlen        full length of the command including binary
 *                      attachment
 *
 * @param op            configure operation
 * @param oid           object instance identifier or NULL
 * @param val           object instance value or NULL
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 *
 * @note If Test Agent does not want to support their own handler for
 * configure operations, then a Test Agent shall implement this function
 * as returning -1 telling PCH that we rely on generic function available
 * in PCH.
 */
extern int rcf_ch_configure(struct rcf_comm_connection *handle,
                            char *cbuf, size_t buflen,
                            size_t answer_plen,
                            const uint8_t *ba, size_t cmdlen,
                            rcf_ch_cfg_op_t op,
                            const char *oid, const char *val);
/**@} <!-- END rcf_ch_cfg --> */

/** @defgroup rcf_ch_var Command Handler: Variables support
 * @ingroup rcf_ch
 * @{
 */

/**
 * Get value of the variable from the Test Agent or NUT served by it.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 *
 * @param type          type of the variable
 * @param var           name of the variable
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_vread(struct rcf_comm_connection *handle,
                        char *cbuf, size_t buflen, size_t answer_plen,
                        rcf_var_type_t type, const char *var);


/**
 * Change value of the variable on the Test Agent or NUT served by it.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 *
 * @param type          type of the variable
 * @param var           name of the variable
 * @param ...           new value
 *
 *
 * @return indication of command support or error code
 * @retval  0            command is supported
 * @retval -1            command is not supported
 * @retval other         error returned by communication library
 */
extern int rcf_ch_vwrite(struct rcf_comm_connection *handle,
                         char *cbuf, size_t buflen, size_t answer_plen,
                         rcf_var_type_t type, const char *var, ...);
/**@} <!-- END rcf_ch_var --> */

/** @defgroup rcf_ch_file Command Handler: File maniputation support
 * @ingroup rcf_ch
 * @{
 */

/**
 * Put/get file to/from the Test Agent or NUT served by it.  If the
 * function returns -1, default command processing (using stdio
 * library) is  performed by caller.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param ba            pointer to location of binary attachment
 *                      in the command buffer or NULL if no binary
 *                      attachment is provided
 * @param cmdlen        full length of the command including binary
 *                      attachment
 * @param opt           RCFOP_FGET, RCFOP_FPUT or RCFOP_FDEL
 * @param filename      full name of the file in TA or NUT file system
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_file(struct rcf_comm_connection *handle,
                       char *cbuf, size_t buflen, size_t answer_plen,
                       const uint8_t *ba, size_t cmdlen,
                       rcf_op_t op, const char *filename);
/**@} <!-- END rcf_ch_file --> */

/** @defgroup rcf_ch_tad Command Handler: Traffic Application Domain (TAD) support
 * A set of functions exported by a Test Agent to support TAD.
 * The only supported version of TAD available at @path{lib/tad},
 * so Test Agent code located under @path{agents/[agent type]} should
 * not care about this interface.
 * @ingroup rcf_ch
 * @{
 */

/**
 * Initialize RCF TAD (Traffic Application Domain).
 *
 * @return Status code.
 * @retval TE_ENOSYS    Traffic Application Domain is not supported
 */
extern te_errno rcf_ch_tad_init(void);

/**
 * Shutdown RCF TAD (Traffic Application Domain).
 *
 * @return Status code.
 */
extern te_errno rcf_ch_tad_shutdown(void);


/**
 * Create a CSAP (Communication Service Access Point).
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param ba            pointer to location of binary attachment
 *                      in the command buffer or NULL if no binary
 *                      attachment is provided
 * @param cmdlen        full length of the command including binary
 *                      attachment
 *
 * @param stack         protocol stack identifier
 * @param params        parameters passed in the command line or NULL
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_csap_create(struct rcf_comm_connection *handle,
                              char *cbuf, size_t buflen,
                              size_t answer_plen,
                              const uint8_t *ba, size_t cmdlen,
                              const char *stack, const char *params);

/**
 * Delete the CSAP.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 *
 * @param csap          CSAP handle
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_csap_destroy(struct rcf_comm_connection *handle,
                               char *cbuf, size_t buflen,
                               size_t answer_plen,
                               csap_handle_t csap);

/**
 * Get CSAP parameter
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *
 * @param csap          CSAP handle
 * @param param         name of the CSAP-specific parameter
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_csap_param(struct rcf_comm_connection *handle,
                             char *cbuf, size_t buflen,
                             size_t answer_plen,
                             csap_handle_t csap, const char *param);


/**
 * "trsend_start" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param ba            pointer to location of binary attachment
 *                      in the command buffer or NULL if no binary
 *                      attachment is provided
 * @param cmdlen        full length of the command including binary
 *                      attachment
 *
 * @param csap          CSAP handle
 * @param postponed     "postponed" flag value
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trsend_start(struct rcf_comm_connection *handle,
                               char *cbuf, size_t buflen,
                               size_t answer_plen,
                               const uint8_t *ba, size_t cmdlen,
                               csap_handle_t csap, te_bool postponed);

/**
 * "trsend_stop" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 *
 * @param csap          CSAP handle
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trsend_stop(struct rcf_comm_connection *handle,
                              char *cbuf, size_t buflen,
                              size_t answer_plen,
                              csap_handle_t csap);


/** Traffic receive mode flags */
typedef enum rcf_ch_trrecv_flags {
    RCF_CH_TRRECV_PACKETS = 1,           /**< Receive and report packets */
    RCF_CH_TRRECV_PACKETS_NO_PAYLOAD = 2,/**< Do not report packets
                                              payload */
    RCF_CH_TRRECV_PACKETS_SEQ_MATCH = 4, /**< Use pattern sequence for
                                              matching */
    RCF_CH_TRRECV_MISMATCH = 8,          /**< Store mismatch packets
                                              to get from test later */
} rcf_ch_trrecv_flags;

/**
 * "trrecv_start" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param ba            pointer to location of binary attachment
 *                      in the command buffer or NULL if no binary
 *                      attachment is provided
 * @param cmdlen        full length of the command including binary
 *                      attachment
 *
 * @param csap          CSAP handle
 * @param num           number of packets to be sent
 *                      (0 if number of packets is unlimited)
 * @param timeout       maximum duration (in milliseconds) of
 *                      receiving the traffic
 * @param flags         traffic receive flags (see rcf_ch_trrecv_flags)
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trrecv_start(struct rcf_comm_connection *handle,
                               char *cbuf, size_t buflen,
                               size_t answer_plen, const uint8_t *ba,
                               size_t cmdlen, csap_handle_t csap,
                               unsigned int num, unsigned int timeout,
                               unsigned int flags);

/**
 * "trrecv_stop" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 *
 * @param csap          CSAP handle
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trrecv_stop(struct rcf_comm_connection *handle,
                              char *cbuf, size_t buflen,
                              size_t answer_plen,
                              csap_handle_t csap);

/**
 * "trrecv_get" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 *
 * @param csap          CSAP handle
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trrecv_get(struct rcf_comm_connection *handle,
                             char *cbuf, size_t buflen,
                             size_t answer_plen,
                             csap_handle_t csap);

/**
 * "trrecv_wait" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 *
 * @param csap          CSAP handle
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trrecv_wait(struct rcf_comm_connection *handle,
                             char *cbuf, size_t buflen,
                             size_t answer_plen,
                             csap_handle_t csap);

/**
 * "trsend_recv" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 * @param ba            pointer to location of binary attachment
 *                        in the command buffer or NULL if no binary
 *                        attachment is provided
 * @param cmdlen        full length of the command including binary
 *                        attachment
 *
 * @param csap          CSAP handle  * @param results "results" flag value
 * @param timeout       timeout value in milliseconds (if > 0)
 * @param flags         traffic receive flags (see rcf_ch_trrecv_flags)
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trsend_recv(struct rcf_comm_connection *handle,
                              char *cbuf, size_t buflen,
                              size_t answer_plen,
                              const uint8_t *ba, size_t cmdlen,
                              csap_handle_t csap, unsigned int timeout,
                              unsigned int flags);

/**
 * "trpoll" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 *
 * @param csap          CSAP handle
 * @param timeout       maximum duration (in milliseconds) of poll
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trpoll(struct rcf_comm_connection *handle,
                         char *cbuf, size_t buflen, size_t answer_plen,
                         csap_handle_t csap, unsigned int timeout);

/**
 * "trpoll_cancel" command handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 *
 * @param csap          CSAP handle
 * @param poll_id       Poll request ID
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_trpoll_cancel(struct rcf_comm_connection *handle,
                                char *cbuf, size_t buflen,
                                size_t answer_plen, csap_handle_t csap,
                                unsigned int poll_id);
/**@} <!-- END rcf_ch_tad --> */

/** @defgroup rcf_ch_func Command Handler: Function call support
 * @ingroup rcf_ch
 * @{
 */

/**
 * Execute routine on the Test Agent or NUT served by it.
 * Parameters of the function are not parsed. If the handler
 * returns -1, then default processing is performed by caller:
 * Portable Commands Handler obtains address of the
 * function using rcf_ch_symbol_addr(), calls a routine
 * rcf_ch_call_routine() to execute the routine and sends an answer.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 *
 * @param rtn           routine name
 * @param is_argv       if TRUE, then routine prototype is
 *                        (int argc, char **argv)
 * @param argc          number of arguments
 * @param params        pointer to array of RCF_MAX_PARAMS length
 *                        with routine arguments
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_call(struct rcf_comm_connection *handle,
                       char *cbuf, size_t buflen, size_t answer_plen,
                       const char *rtn, te_bool is_argv,
                       int argc, void **params);
/**@} <!-- END rcf_ch_func --> */

/** @defgroup rcf_ch_proc Command Handler: Process/thread support
 * A set of functions exported by a Test Agent to support interface
 * of Command Handler for Test Agent thread and process manipulations.
 * @ingroup rcf_ch
 * @{
 */

/**
 * Start process on the Test Agent or NUT served by it.
 *
 * @param pid           location of pid of the new task
 * @param priority      priority of the new process or -1 if the
 *                      priority is not specified in the command
 * @param rtn           routine entry point name.
 *                      It is expected that a function has
 *                      the following argument list (int argc, char **argv)
 * @param do_exec       whether to do execve after fork() in
 *                      a newly created process or just to call
 *                      @p rtn function in a new process
 * @param argc          number of arguments
 * @param params        pointer to array of RCF_MAX_PARAMS length
 *                      with routine arguments
 *
 * @return Status code
 */
extern int rcf_ch_start_process(pid_t *pid, int priority,
                                const char *rtn, te_bool do_exec,
                                int argc, void **params);

/**
 * Start thread on the Test Agent or NUT served by it.
 *
 * @param tid           location of tid of the new task
 * @param priority      priority of the new process or -1 if the
 *                        priority is not specified in the command
 * @param rtn           routine entry point name
 * @param is_argv       if TRUE, then routine prototype is
 *                        (int argc, char **argv)
 * @param argc          number of arguments
 * @param params        pointer to array of RCF_MAX_PARAMS length
 *                      with routine arguments
 *
 * @return Status code
 */
extern int rcf_ch_start_thread(int *tid, int priority,
                               const char *rtn, te_bool is_argv,
                               int argc, void **params);


/**
 * Kill the process on the Test Agent or NUT served by it.
 *
 * @param pid           process identifier
 *
 * @return Status code
 */
extern int rcf_ch_kill_process(unsigned int pid);

/**
 * Free process data stored on the Test Agent.
 *
 * @param pid           process identifier
 *
 * @return Status code
 */
extern int rcf_ch_free_proc_data(unsigned int pid);

/**
 * Kill the thread on the Test Agent or NUT served by it.
 *
 * @param tid           thread identifier
 *
 * @return Status code
 */
extern int rcf_ch_kill_thread(unsigned int tid);
/**@} <!-- END rcf_ch_proc --> */

/** @defgroup rcf_ch_cfg Command Handler: Configuration support
 * A set of functions exported by a Test Agent to support interface
 * of Command Handler for Test Agent configuration.
 * @ingroup rcf_ch
 * @{
 */

/**
 * Prototype for get instance value routine.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 *                  (a buffer of RCF_MAX_VAL bytes size)
 * @param ...       Up to 10 instance names (if there are less instances
 *                  in configuration path than arguments, extra arguments
 *                  will be set to @c NULL)
 *
 * @return Status code
 */
typedef te_errno (* rcf_ch_cfg_get)(unsigned int gid, const char *oid,
                                    char *value, ...);

/**
 * Prototype for set instance value routine.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     value to set
 * @param ...       Up to 10 instance names (if there are less instances
 *                  in configuration path than arguments, extra arguments
 *                  will be set to @c NULL)
 *
 * @return Status code
 */
typedef te_errno (* rcf_ch_cfg_set)(unsigned int gid, const char *oid,
                                    const char *value, ...);

/**
 * Prototype for add instance routine.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     value to set or NULL
 * @param ...       Up to 10 instance names (if there are less instances
 *                  in configuration path than arguments, extra arguments
 *                  will be set to @c NULL)
 *
 * @return Status code
 */
typedef te_errno (* rcf_ch_cfg_add)(unsigned int gid, const char *oid,
                                    const char *value, ...);

/**
 * Prototype for delete instance routine.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param ...       Up to 10 instance names (if there are less instances
 *                  in configuration path than arguments, extra arguments
 *                  will be set to @c NULL)
 *
 * @return Status code
 */
typedef te_errno (* rcf_ch_cfg_del)(unsigned int gid, const char *oid,
                                    ...);

/**
 * Prototype for a routine that returns the list of instance names
 * for an object.
 * The routine should allocate memory for instance list with malloc().
 * Caller is responsible for releasing of this memory.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier of the parent
 * @param sub_id    ID (name) of the object to be listed
 * @param list      location for the returned list pointer
 *                  (list entries should be separated with SPACE character.
 *                  For example if we need to return names 'a', 'b', 'c',
 *                  then returned string would be "a b c"
 * @param ...       Up to 10 instance names (if there are less instances
 *                  in configuration path than arguments, extra arguments
 *                  will be set to @c NULL)
 *
 * @return Status code
 */
typedef te_errno (* rcf_ch_cfg_list)(unsigned int gid, const char *oid,
                                     const char *sub_id,
                                     char **list, ...);

/**
 * Propotype of the commit function.
 *
 * @param gid       group identifier
 * @param p_oid     parsed object instance identifier
 *
 * @return Status code
 */
typedef te_errno (* rcf_ch_cfg_commit)(unsigned int gid,
                                       const cfg_oid *p_oid);

/**
 * Type of the function to apply the substitution.
 *
 * @param src The string in which the substitution is performed.
 * @param new The string to replace.
 * @param old The string to be replaced.
 *
 * @return Status code
 */
typedef te_errno (* rcf_ch_substitution_apply)(te_string *src, const char *new,
                                               const char *old);
typedef struct rcf_pch_cfg_substitution {
    /** Name of an instance value the substitution applies to */
    const char *name;
    /** OID for getting the substitution value */
    const char *ref_name;
    /** Substitution apply method */
    rcf_ch_substitution_apply apply;
} rcf_pch_cfg_substitution;

/** Configuration tree node */
typedef struct rcf_pch_cfg_object {

    /** Object sub-identifier */
    char            sub_id[RCF_MAX_NAME];
    /** Length of the object identifier */
    unsigned int    oid_len;

    /** @name Configuration tree links */
    /** First in the list of children */
    struct rcf_pch_cfg_object *son;
    /** Next in the list of brothers */
    struct rcf_pch_cfg_object *brother;
    /*@}*/

    /** @name Access routines */
    rcf_ch_cfg_get  get;    /**< Get accessor */
    rcf_ch_cfg_set  set;    /**< Set accessor */
    rcf_ch_cfg_add  add;    /**< Add method */
    rcf_ch_cfg_del  del;    /**< Delete method */
    rcf_ch_cfg_list list;   /**< List method */
    /*@}*/

    /** @name Commit support */
    /** Function to commit changes or NULL*/
    rcf_ch_cfg_commit           commit;
    /** Parent which supports commit operation */
    struct rcf_pch_cfg_object  *commit_parent;
    /*@}*/

    /** Pointer to a linear array of substitutions */
    const rcf_pch_cfg_substitution *subst;

} rcf_pch_cfg_object;

/**
 * @defgroup rcf_ch_cfg_node_def Configuration node definition macros
 * Macros to define configuration nodes supported by @ref te_agents.
 * Read more information on how to support new nodes at @ref te_agents_conf.
 * @ingroup rcf_ch_cfg
 * @{
 */

/**
 * Define non-accessible configuration tree node.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 */
#define RCF_PCH_CFG_NODE_NA(_name, _subid, _son, _brother) \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
        }

/**
 * Define non-accessible configuration tree node with commit
 * capability.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_commit commit function
 */
#define RCF_PCH_CFG_NODE_NA_COMMIT(_name, _subid, _son, \
                                   _brother, _f_commit)     \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
         .commit = _f_commit,                               \
        }

/**
 * Define read-only singleton.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_get    get accessor
 */
#define RCF_PCH_CFG_NODE_RO(_name, _subid, _son, _brother, _f_get) \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
         .get = (rcf_ch_cfg_get)_f_get,                     \
        }

/**
 * Define read-write singleton w/o commit support.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_get    get accessor
 * @param _f_set    set accessor
 */
#define RCF_PCH_CFG_NODE_RW(_name, _subid, _son, _brother, \
                            _f_get, _f_set)                 \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
         .get = (rcf_ch_cfg_get)_f_get,                     \
         .set = (rcf_ch_cfg_set)_f_set,                     \
        }

/**
 * Define read-write singleton with on-parent commit support.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_get    get accessor
 * @param _f_set    set accessor
 * @param _commit   pointer to parent with commit function
 */
#define RCF_PCH_CFG_NODE_RWC(_name, _subid, _son, _brother, \
                             _f_get, _f_set, _commit)       \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
         .get = (rcf_ch_cfg_get)_f_get,                     \
         .set = (rcf_ch_cfg_set)_f_set,                     \
         .commit_parent = _commit                           \
        }

/**
 * Define read-write singleton w/o commit support and with
 * an array of substitutions.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_get    get accessor
 * @param _f_set    set accessor
 * @param _subst    array of substitutions (rcf_pch_cfg_substitution)
 */
#define RCF_PCH_CFG_NODE_RW_WITH_SUBST(_name, _subid, _son, _brother, \
                                       _f_get, _f_set, _subst)        \
    static rcf_pch_cfg_object _name = {                               \
        .sub_id = _subid,                                             \
        .son = _son,                                                  \
        .brother = _brother,                                          \
        .get = (rcf_ch_cfg_get)_f_get,                                \
        .set = (rcf_ch_cfg_set)_f_set,                                \
        .subst = _subst                                               \
    }

/**
 * Define node collection.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_add    add accessor
 * @param _f_del    delete accessor
 * @param _f_list   list accessor
 * @param _f_commit commit function
 */
#define RCF_PCH_CFG_NODE_COLLECTION(_name, _subid, _son, _brother, \
                                    _f_add, _f_del, _f_list,        \
                                    _f_commit)                      \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
         .add = (rcf_ch_cfg_add)_f_add,                     \
         .del = (rcf_ch_cfg_del)_f_del,                     \
         .list = (rcf_ch_cfg_list)_f_list,                  \
         .commit = _f_commit,                               \
        }

/**
 * Define node collection that can be set
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_add    add accessor
 * @param _f_del    delete accessor
 * @param _f_list   list accessor
 * @param _f_commit commit function
 */
#define RCF_PCH_CFG_NODE_RW_COLLECTION(_name, _subid, _son, _brother,   \
                                       _f_get,  _f_set,                 \
                                       _f_add, _f_del, _f_list,         \
                                       _f_commit)                       \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
         .get = (rcf_ch_cfg_get)_f_get,                     \
         .set = (rcf_ch_cfg_set)_f_set,                     \
         .add = (rcf_ch_cfg_add)_f_add,                     \
         .del = (rcf_ch_cfg_del)_f_del,                     \
         .list = (rcf_ch_cfg_list)_f_list,                  \
         .commit = _f_commit,                               \
        }

/**
 * Define node collection that can be set.
 * Unlike RCF_PCH_CFG_NODE_RW_COLLECTION(), this macro allows to
 * specify a parent with commit function instead of commit function
 * for the current node.
 *
 * @param _name     name of the declared node variable
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_get    get accessor
 * @param _f_set    set accessor
 * @param _f_add    add accessor
 * @param _f_del    delete accessor
 * @param _f_list   list accessor
 * @param _commit   parent with commit function
 */
#define RCF_PCH_CFG_NODE_RWC_COLLECTION(_name, _subid, _son, _brother,   \
                                       _f_get,  _f_set,                 \
                                       _f_add, _f_del, _f_list,         \
                                       _commit)                         \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
         .get = (rcf_ch_cfg_get)_f_get,                     \
         .set = (rcf_ch_cfg_set)_f_set,                     \
         .add = (rcf_ch_cfg_add)_f_add,                     \
         .del = (rcf_ch_cfg_del)_f_del,                     \
         .list = (rcf_ch_cfg_list)_f_list,                  \
         .commit_parent = _commit,                          \
        }

/**
 * Define node collection that can be set with an array of substitutions
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_add    add accessor
 * @param _f_del    delete accessor
 * @param _f_list   list accessor
 * @param _f_commit commit function
 * @param _subst    array of substitutions (rcf_pch_cfg_substitution)
 */
#define RCF_PCH_CFG_NODE_RW_COLLECTION_WITH_SUBST(_name, _subid, _son,       \
                                                   _brother, _f_get, _f_set, \
                                                   _f_add, _f_del, _f_list,  \
                                                   _f_commit, _subst)        \
    static rcf_pch_cfg_object _name = {                                      \
        .sub_id = _subid,                                                    \
        .son = _son,                                                         \
        .brother = _brother,                                                 \
        .get = (rcf_ch_cfg_get)_f_get,                                       \
        .set = (rcf_ch_cfg_set)_f_set,                                       \
        .add = (rcf_ch_cfg_add)_f_add,                                       \
        .del = (rcf_ch_cfg_del)_f_del,                                       \
        .list = (rcf_ch_cfg_list)_f_list,                                    \
        .commit = _f_commit,                                                 \
        .subst = _subst                                                      \
    }

/**
 * Define read-only node collection.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 * @param _f_get    add accessor
 * @param _f_list   list accessor
 */
#define RCF_PCH_CFG_NODE_RO_COLLECTION(_name, _subid, _son, _brother, \
                                       _f_get, _f_list)               \
    static rcf_pch_cfg_object _name = {                     \
         .sub_id = _subid,                                  \
         .son = _son,                                       \
         .brother = _brother,                               \
         .get = (rcf_ch_cfg_get)_f_get,                     \
         .list = (rcf_ch_cfg_list)_f_list,                  \
        }


/* @} */

/**
 * Initialize configuration support of Command Handler
 * (Test Agent specific initialization).
 *
 * @return Status of the operation
 * @retval  0  initialization successfully completed
 * @retval -1  initialization failed
 *
 * @note In this function a Test Agent calls rcf_pch_add_node()
 * in order to register nodes that it will support during its operation.
 */
extern int rcf_ch_conf_init(void);

/**
 * Release resources allocated for configuration support.
 */
extern void rcf_ch_conf_fini(void);

/**
 * Returns name of the Test Agent (caller does not free or change
 * the memory where the name is located).
 *
 * @return Agent name
 */
extern const char *rcf_ch_conf_agent(void);

/*@}*/

/** A convenience constructor to define substitutions */
#define RCF_PCH_CFG_SUBST_SET(...) \
    { __VA_ARGS__, {NULL, NULL, NULL} }

/*@} <!-- END rcf_ch --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_CH_API_H__ */
