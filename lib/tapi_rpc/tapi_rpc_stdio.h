/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of some standard input/output
 * functions and useful extensions.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_STDIO_H__
#define __TE_TAPI_RPC_STDIO_H__

#include <stdio.h>

#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum resulting command length for rpc_shell() */
#define RPC_SHELL_CMDLINE_MAX   256

/** 'FILE *' equivalent */
typedef rpc_ptr rpc_file_p;

/**
 * Query the descriptor of the given file on RPC server side.
 *
 * @note See fileno manual page for more information
 * @param rpcs   RPC server handle
 * @param f      file stream
 *
 * @return  The stream file descriptor, otherwise -1 when
 *          error occured
 */
extern int rpc_fileno(rcf_rpc_server *rpcs,
                      rpc_file_p f);

/**
 * Open a file on RPC server side and associate it with a stream.
 *
 * @note See @b fopen manual page for more information
 * @param rpcs     RPC server handle
 * @param path     file name
 * @param mode     type of file access
 *
 * @return Stream associated with the file name, otherwise @b NULL on error
 */
extern rpc_file_p rpc_fopen(rcf_rpc_server *rpcs,
                            const char *path, const char *mode);

/**
 * Open a file on RPC server side and associate it with a stream.
 *
 * @note See @b fdopen manual page for more information
 * @param rpcs     RPC server handle
 * @param fd       file descriptor
 * @param mode     type of file access
 *
 * @return Stream associated with the file name, otherwise @b NULL on error
 */
extern rpc_file_p rpc_fdopen(rcf_rpc_server *rpcs, int fd,
                             const char *mode);
 
/**
 * Close the stream associated with the file.
 *
 * @param rpcs           RPC server handle
 * @param file           Stream to close
 *
 * @return   Zero on success, otherwise -1.
 */
extern int rpc_fclose(rcf_rpc_server *rpcs, rpc_file_p file);

/**
 * Execute shell command on the IPC server.
 * You SHOULD not use this function unless you are sure that shell command
 * will exit normally. If this RPC will timeout, you'll have no chance to
 * kill the child process, because you do not know its PID.
 *
 * @param rpcs          RPC server handle
 * @param cmd           the command to be executed
 *
 * @return status of the process
 */
extern rpc_wait_status rpc_system(rcf_rpc_server *rpcs, const char *cmd);

/**
 * Open a process by creating a pipe, forking, and invoking the shell.
 *
 * @note Do not use this function unless you test it - use 
 *       rpc_ta_shell_cmd_ex instead.
 *
 * @param rpcs     RPC server handle
 * @param cmd      NULL-terminated shell command line
 * @param mode     "r" (for reading command output) or "w" (for writing to
 *                 command standard input)
 *
 * @return Stream to be used for reading or writing or @b NULL on error
 */
extern rpc_file_p rpc_popen(rcf_rpc_server *rpcs,
                            const char *cmd, const char *mode);
 
/**
 * Close the stream returned by @b popen().
 *
 * @param rpcs           RPC server handle
 * @param file           Stream to close
 *
 * @return   Zero on success, otherwise -1
 */
extern int rpc_pclose(rcf_rpc_server *rpcs, rpc_file_p file);

/**
 * Execute shell command on the IPC server and read the output.
 * The routine allocates memory for the output buffer and places
 * null-terminated string to it.
 * @b pbuf pointer is initialized by NULL if no buffer is allocated.
 *
 * @param rpcs          RPC server handle
 * @param pbuf          location for the command output buffer 
 * @param cmd           format of the command to be executed
 * @param uid           user id to execute as
 * @param ...           parameters for command
 *
 * @return status of the process
 */
extern rpc_wait_status rpc_shell_get_all(rcf_rpc_server *rpcs,
                                         char **pbuf, const char *cmd, 
                                         tarpc_uid_t uid, ...);

/**
 * Execute command on the RPC server as a given user and redirect
 * stdin/stdout to pipe(s) if necessary.
 * You MUST use uid parameter instead of "su - user -c", because su makes
 * one more fork, and you do not know how to kill this grandchild process.
 * You SHOULD destroy this process by calling rpc_ta_kill_death() instead 
 * of rpc_kill(RPC_SIGKILL).
 *
 * @param rpcs      RPC server handle
 * @param cmd       command format to execute
 * @param uid       user to run as
 * @param in_fd     location to store pipe to stdin or NULL
 * @param out_fd    location to stdin pipe to stdout or NULL
 * @param ...       parameters to command format
 *
 * @return pid of spawned process or -1 on failure
 */
extern tarpc_pid_t rpc_ta_shell_cmd_ex(rcf_rpc_server *rpcs, 
                                       const char *cmd, tarpc_uid_t uid, 
                                       int *in_fd, int *out_fd, ...);

/**
 * Get environment variable.
 *
 * @param rpcs          RPC server handle
 * @param name          variable name
 *
 * @return variable value (memory is allocated by the function) or NULL
 */
extern char *rpc_getenv(rcf_rpc_server *rpcs, const char *name);

/**
 * Add the variable with specified value to the environment or change
 * value of the existing variable.
 *
 * @param rpcs          RPC server handle
 * @param name          variable name
 * @param value         new value
 * @param overwrite     0, value of the existing variable is not changed
 *
 * @return variable value (memory is allocated by the function) or NULL
 */
extern int rpc_setenv(rcf_rpc_server *rpcs, 
                      const char *name, const char *value, int overwrite);

/**
 * Read all data from file descriptor in the RPC.
 * The routine allocates memory for the output buffer and places
 * null-terminated string to it.
 * @b pbuf pointer is initialized by NULL if no buffer is allocated.
 *
 * @param rpcs          RPC server handle
 * @param fd            file descriptor to read from
 * @param pbuf          location for the command output buffer
 * @param bytes         location for the number of bytes read
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_read_all(rcf_rpc_server *rpcs, int fd, 
                        char **pbuf, size_t *bytes);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_STDIO_H__ */
