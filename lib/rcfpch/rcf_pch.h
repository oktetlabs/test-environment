/** @file
 * @brief RCF Portable Command Handler
 *
 * Definition of the C API provided by Portable Commands Handler to the
 * Test Agent and Commands Handlers libraries.
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_PCH_H__
#define __TE_RCF_PCH_H__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_common.h"
#include "comm_agent.h"

#include "rcf_internal.h"
#include "rcf_ch_api.h"
#include "rcf_pch_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start Portable Command Handler.
 *
 * Caller is blocked until shutdown command is recieved or error occur.
 * Custom and default command handlers are called when commands via 
 * Test Protocol are received.
 *
 * @param confstr   configuration string for communication library
 * @param info      if not NULL, the string to be send to the engine
 *                  after initialisation
 *
 * @return error code
 */
extern int rcf_pch_run(const char *confstr, const char *info);

/** Detach from the Test Engine after fork() */
extern void rcf_pch_detach(void);

/** @name Default Command Handlers.
 *
 * Default Command Handlers are called from rcf_pch_run() routine
 * when corresponding custom handler returns no support. Also default
 * command handlers may be called from custom.
 */

/**
 * Default vread command handler.
 *
 * @param conn          connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes to be copied from the command
 *                      to answer
 * @param type          variable type
 * @param var           variable name
 *
 * @return 0 or error returned by communication library
 */
extern int rcf_pch_vread(struct rcf_comm_connection *conn,
                         char *cbuf, size_t buflen, size_t answer_plen,
                         rcf_var_type_t type, const char *var);

/**
 * Default vwrite command handler.
 *
 * @param conn          connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes to be copied from the command
 *                      to answer
 * @param type          variable type
 * @param var           variable name
 * @param ...           value
 *
 * @return 0 or error returned by communication library
 */
extern int rcf_pch_vwrite(struct rcf_comm_connection *conn,
                          char *cbuf, size_t buflen, size_t answer_plen,
                          rcf_var_type_t type, const char *var, ...);


/**
 * Initialize RCF PCH configuration tree support.
 */
extern void rcf_pch_cfg_init(void);


/**
 * Default configure command hadler.
 *
 * @param conn          connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param ba            pointer to location of binary attachment in
 *                      the command buffer or NULL if no binary attachment
 *                      is provided
 * @param cmdlen        full length of the command including binary
 *                      attachment
 *
 * @param op            configure operation
 * @param oid           object instance identifier or NULL
 * @param val           object instance value or NULL
 *
 *
 * @return 0 or error returned by communication library
 */
extern int rcf_pch_configure(struct rcf_comm_connection *conn,
                             char *cbuf, size_t buflen, size_t answer_plen,
                             const uint8_t *ba, size_t cmdlen,
                             rcf_ch_cfg_op_t op,
                             const char *oid, const char *val);

/**
 * Default implementation of agent list accessor.
 * This function complies with rcf_ch_cfg_list protoеype.
 * 
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param list      (OUT) location for pointer to string with space
 *                  separated list (should be allocated using malloc())
 *
 * @retval 0        success
 * @retval TE_ENOMEM   memory allocation failure
 */
extern int rcf_pch_agent_list(unsigned int gid, const char *oid,
                              char **list);

/**
 * Default file processing handler.
 *
 * @param conn          connection handle
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
 * @param op            if TRUE, file should be put
 * @param filename      full name of the file in TA or NUT file system
 *
 *
 * @return 0 or error returned by communication library
 */
extern int rcf_pch_file(struct rcf_comm_connection *conn,
                        char *cbuf, size_t buflen, size_t answer_plen,
                        const uint8_t *ba, size_t cmdlen,
                        rcf_op_t op, const char *filename);

/**
 * Default routine call handler.
 *
 * @param conn          connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 *
 * @param rtn           routine entry point name
 * @param is_argv       if TRUE, then routine prototype is
 *                      (int argc, char **argv)
 * @param argc          number of arguments
 * @param params        pointer to array of RCF_MAX_PARAMS length
 *                      with routine arguments
 *
 *
 * @return 0 or error returned by communication library
 */
extern int rcf_pch_call(struct rcf_comm_connection *conn,
                        char *cbuf, size_t buflen, size_t answer_plen,
                        const char *rtn, te_bool is_argv, int argc,
                        void **params);

/**
 * RPC handler.
 *
 * @param conn          connection handle
 * @param sid           session identifier
 * @param data          pointer to data in the command buffer
 * @param len           length of encoded data
 * @param server        RPC server name
 * @param timeout       timeout in seconds or 0 for unlimited
 *
 * @return 0 or error returned by communication library
 */
extern int rcf_pch_rpc(struct rcf_comm_connection *conn, int sid, 
                       const char *data, size_t len,
                       const char *server, uint32_t timeout);

/** 
 * Initialize RCF RPC server structures and link RPC configuration
 * nodes to the root.
 */
extern void rcf_pch_rpc_init(void);

/** 
 * Cleanup RCF RPC server structures. Close all sockets. Release all
 * allocated memory. Do NOT kill any RPC servers.
 *
 * @param This function is intended to be used after fork in child
 *        process to release all resources allocated by RCF PCH RPC
 *        support.
 */
extern void rcf_pch_rpc_atfork(void);

/** 
 * Cleanup RCF RPC server structures.
 */
extern void rcf_pch_rpc_shutdown(void);

/**
 * Entry function for RPC server. 
 *
 * @param name    RPC server name
 *
 * @return NULL
 */
extern void *rcf_pch_rpc_server(const char *name);

/**
 * Wrapper to call rcf_pch_rpc_server via "ta exec func" mechanism. 
 *
 * @param argc    should be 1
 * @param argv    should contain pointer to RPC server name
 */
extern void rcf_pch_rpc_server_argv(int argc, char **argv);

/*--------------- Dynamically grabbed TA resources -------------------*/

/**
 * These resources are resources provided by TA host (interfaces, services)
 * which cannot be shared by several TAs and therefore should be
 * locked/unlocked by particular TA dynamically.
 *
 * When resource is grabbed/released by adding/deleting instance of
 * /agent/rsrc object via Configurator.
 * 
 * Three strings are associated with each resource:
 *     * resource instance name, used only for adding/deleting
 *       /agent/rsrc instances: e.g. r1 in /agent:A/rsrc:r1;
 *     * resource name, which unique identifies the resource:
 *       "/agent:A/interface:eth0", "/agent/dnsserver" 
 *       or "my_peripheral"; this name should be OID or string, 
 *       whithout '/' symbols;
 *     * resource generic name, which is used by rcfpch to discover
 *       callback for resource grabbing/releasing: e.g. /agent/interface.
 *
 * The name and generic name may be the same unless there are many resources
 * of the same class which grabbing/releasing is handled by the single
 * callback (e.g. network interfaces).
 * 
 * To use some dynamically grabbed resource one should:
 *
 * 1. Provide to rcfpch information about grabbing/releasing callbacks
 *    for the resource. This may be done via explicit call of 
 *    rcf_pch_rpcs_info during TA initialization or remote RCF or
 *    RCF RPC call. 
 *
 * 2. Grab the resource from the test or during initialization 
 *    adding /agent/rsrc instance via confapi or in cs.conf.
 *
 * RCFPCH automatically creates the lock for thre resource in TE_TMP
 * directory. Name of the lock file containing TA PID is constructed as:
 *     $(te_lockdir}/te_ta_lock_<converted name>
 *
 * "converted name" is resource name with '/' replaced by '%'.
 * te_lock_dir should be exported by the TA. 
 *
 * If lock of dead TA is found it is automatically removed.
 */

/** 
 * Callback for resource grabbing. 
 * 
 * @param name   resource name
 *
 * @return Status code
 */
typedef te_errno (* rcf_pch_rsrc_grab_callback)(const char *name);

/** 
 * Callback for resource releasing. 
 * 
 * @param name   resource name
 *
 * @return Status code
 */
typedef te_errno (* rcf_pch_rsrc_release_callback)(const char *name);


/**
 * Specify callbacks for dynamically registarable resource.
 *
 * @param name          resource generic name
 * @param grab          grabbing callback
 * @param release       releasing callback or NULL
 *
 * @return Status code
 */
extern te_errno rcf_pch_rsrc_info(const char *name, 
                                  rcf_pch_rsrc_grab_callback grab,
                                  rcf_pch_rsrc_release_callback release);

/* 
 * Dummy callbacks for resources which need not additional
 * processing during grabbing/releasing.
 *
 * Note that RCFPCH provides locking automatically. 
 * Moreover, rcf_pch_rsrc_accessible() function may be used for
 * checking of resource accessability (for example, during conf requests
 * processing).
 * Thus many resources need not additional processing and can use 
 * callbacks below.
 *
 * Additional processing may be:
 *    - modification of TA configuration tree to make some subtrees 
 *      completely invisible;
 *    - some initialization/shutdown, creation/restoring backups, etc.
 */
extern te_errno rcf_pch_rsrc_grab_dummy(const char *name);
extern te_errno rcf_pch_rsrc_release_dummy(const char *name);

/**
 * Check if the resource is accessible.
 *
 * @param fmt   format string for resource name
 *
 * @return TRUE is the resource is accessible
 *
 * @note The function should be called from TA main thread only.
 */
extern te_bool rcf_pch_rsrc_accessible(const char *fmt, ...);

/** 
 * Link resource configuration tree.
 */
extern void rcf_pch_rsrc_init(void);

/** Directory for locks creation */
extern const char *te_lockdir;

/*@}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_PCH_H__ */
