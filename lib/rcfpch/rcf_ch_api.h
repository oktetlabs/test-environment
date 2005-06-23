/** @file
 * @brief RCF Portable Command Handler
 *
 * Definition of the C API provided by Commands Handler libraries to RCF
 * Portable Commands Handler.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_CH_API_H__
#define __TE_RCF_CH_API_H__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "tad_common.h"
#include "rcf_common.h"
#include "rcf_internal.h"
#include "comm_agent.h"
#include "conf_oid.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * All functions may return -1 (unsupported). In this case no answer
 * is sent to TEN by handlers Portable Commands Handler processes
 * this situation.
 */

/** Generic routine prototype */
typedef int (* rcf_rtn)(void *arg, ...);

/** Generic threaded routine prototype */
typedef int (* rcf_thr_rtn)(void *sem, void *arg, ...);


/**
 * Initialize structures.
 *
 * @return error code
 */
extern int rcf_ch_init();


/**
 * Mutual exclusion lock access to data connection.
 */
extern void rcf_ch_lock();

/**
 * Unlock access to data connection.
 */
extern void rcf_ch_unlock();


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
 */
extern int rcf_ch_configure(struct rcf_comm_connection *handle,
                            char *cbuf, size_t buflen,
                            size_t answer_plen,
                            const uint8_t *ba, size_t cmdlen,
                            rcf_ch_cfg_op_t op,
                            const char *oid, const char *val);


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


/**
 * This function may be used by Portable Commands Handler to resolve
 * name of the variable or function to its address if rcf_ch_vread,
 * rcf_ch_vwrite or rcf_ch_call function returns -1. In this case
 * default command processing is performed by caller: it is assumed that
 * variable or function are in TA address space and variable is
 * unsigned 32 bit integer.
 *
 * @param name          symbol name
 * @param is_func       if TRUE, function name is required
 *
 * @return symbol address or NULL
 */
extern void *rcf_ch_symbol_addr(const char *name, te_bool is_func);


/**
 * This function may be used by Portable Commands Handler to symbol address
 * to its name.
 *
 * @param addr          symbol address
 *
 * @return symbol name or NULL
 */
extern char *rcf_ch_symbol_name(const void *addr);


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
 * @param results       if TRUE, received packets should be
 *                      passed to the TEN
 * @param timeout       maximum duration (in milliseconds) of
 *                      receiving the traffic
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
                               unsigned int num, te_bool results,
                               unsigned int timeout);

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
                              csap_handle_t csap, te_bool results,
                              unsigned int timeout);


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


/**
 * Start process on the Test Agent or NUT served by it.
 * Parameters of the function are not parsed.
 * The recommended processing is create wrapper routine to be used as
 * an entry point for the new process, which in advice calls
 * rcf_ch_call_routine(). This allows to use parameter parser provided
 * by Portable Commands Handler.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 *
 * @param priority      priority of the new process or -1 if the
 *                        priority is not specified in the command
 * @param rtn           routine entry point name
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
extern int rcf_ch_start_task(struct rcf_comm_connection *handle,
                             char *cbuf, size_t buflen,
                             size_t answer_plen, int priority,
                             const char *rtn, te_bool is_argv,
                             int argc, void **params);

/**
 * This function is similar to rcf_ch_start_task_thr, but
 * it is assumed that a new task will be run as a thread, 
 * not as a forked process
 *
 * @sa rcf_ch_start_task
 */
extern int rcf_ch_start_task_thr(struct rcf_comm_connection *handle,
                                 char *cbuf, size_t buflen,
                                 size_t answer_plen, int priority,
                                 const char *rtn, te_bool is_argv,
                                 int argc, void **params);


/**
 * Kill the process on the Test Agent or NUT served by it.
 *
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                        copied to the answer
 *
 * @param pid           process identifier
 *
 *
 * @return Indication of command support or error code
 *
 * @retval  0       command is supported
 * @retval -1       command is not supported
 * @retval other    error returned by communication library
 */
extern int rcf_ch_kill_task(struct rcf_comm_connection *handle,
                            char *cbuf, size_t buflen,
                            size_t answer_plen, unsigned int pid);


/** @name Standard configuration support */

/**
 * Prototype for get instance value routine.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 * @param instN     Nth instance name (maximum 10) or NULL
 *
 * @return error code
 */
typedef int (* rcf_ch_cfg_get)(unsigned int gid, const char *oid,
                               char *value, const char *instN, ...);

/**
 * Prototype for set instance value routine.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     value to set
 * @param instN     Nth instance name (maximum 10) or NULL
 *
 * @return error code
 */
typedef int (* rcf_ch_cfg_set)(unsigned int gid, const char *oid,
                               const char *value,
                               const char *instN, ...);

/**
 * Prototype for add instance routine.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     value to set or NULL
 * @param instN     Nth instance name (maximum 10) or NULL
 *
 * @return error code
 */
typedef int (* rcf_ch_cfg_add)(unsigned int gid, const char *oid,
                               const char *value,
                               const char *instN, ...);

/**
 * Prototype for delete instance routine.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param instN     Nth instance name (maximum 10) or NULL
 *
 * @return error code
 */
typedef int (* rcf_ch_cfg_del)(unsigned int gid, const char *oid,
                               const char *instN, ...);

/**
 * Prototype for get instance list routine. The routine should
 * allocate memory using malloc().  Caller is responsible for
 * releasing of this memory.
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param list      location for the list pointer
 * @param instN     Nth instance name (maximum 10, father's
 *                    instance name is last, if fit in 10)
 *
 * @return error code
 */
typedef int (* rcf_ch_cfg_list)(unsigned int gid, const char *oid,
                                char **list, const char *instN, ...);

/**
 * Propotype of the commit function.
 *
 * @param gid       group identifier
 * @param p_oid     parsed object instance identifier
 *
 * @return Status code.
 */
typedef int (* rcf_ch_cfg_commit)(unsigned int gid,
                                  const cfg_oid *p_oid);


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

} rcf_pch_cfg_object;


/**
 * Root of the Test Agent configuration tree. RCF PCH function is
 * used as list callback.
 *
 * @param _name     node name
 * @param _son      name of the first son
 */
#define RCF_PCH_CFG_NODE_AGENT(_name, _son) \
    static rcf_pch_cfg_object _name =                       \
        { "agent", 0, _son, NULL, NULL, NULL, NULL, NULL,   \
           (rcf_ch_cfg_list)rcf_pch_agent_list,             \
           NULL, NULL }

/**
 * Define non-accessible configuration tree node.
 *
 * @param _name     node name (rcf_pch_cfg_object)
 * @param _subid    subidentifier name (const char *)
 * @param _son      pointer to the first son node
 * @param _brother  pointer to the next brother node
 */
#define RCF_PCH_CFG_NODE_NA(_name, _subid, _son, _brother) \
    static rcf_pch_cfg_object _name =                       \
        { _subid, 0, _son, _brother,                        \
          NULL, NULL, NULL, NULL, NULL, NULL, NULL }

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
    static rcf_pch_cfg_object _name =                       \
        { _subid, 0, _son, _brother,                        \
          NULL, NULL, NULL, NULL, NULL, _f_commit, NULL }

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
    static rcf_pch_cfg_object _name =                               \
        { _subid, 0, _son, _brother,                                \
          (rcf_ch_cfg_get)_f_get, NULL, NULL, NULL, NULL,           \
          NULL, NULL }

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
    static rcf_pch_cfg_object _name =                       \
        { _subid, 0, _son, _brother,                        \
          (rcf_ch_cfg_get)_f_get, (rcf_ch_cfg_set)_f_set,   \
          NULL, NULL, NULL, NULL, NULL }

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
    static rcf_pch_cfg_object _name =                       \
        { _subid, 0, _son, _brother,                        \
          (rcf_ch_cfg_get)_f_get, (rcf_ch_cfg_set)_f_set,   \
          NULL, NULL, NULL, NULL, _commit }

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
    static rcf_pch_cfg_object _name =                               \
        { _subid, 0, _son, _brother,                                \
          NULL, NULL,                                               \
          (rcf_ch_cfg_add)_f_add, (rcf_ch_cfg_del)_f_del,           \
          (rcf_ch_cfg_list)_f_list, _f_commit, NULL }


/**
 * Get root of the tree of supported objects.
 *
 * @return Root pointer or NULL if standard configuration support
 *         is skipped.
 */
extern rcf_pch_cfg_object *rcf_ch_conf_root(void);

/**
 * Release resources allocated for configuration support.
 */
extern void rcf_ch_conf_release(void);

/**
 * Returns name of the Test Agent (caller does not free or change
 * the memory where the name is located).
 *
 * @return Agent name
 */
extern const char *rcf_ch_conf_agent(void);

/*@}*/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_CH_API_H__ */
