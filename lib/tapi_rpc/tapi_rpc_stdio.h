/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of some standard input/output
 * functions and useful extensions.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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

/** Maximum resulting command length for rpc_shell() */
#define RPC_SHELL_CMDLINE_MAX   256

/** 'FILE *' equivalent */
typedef rpc_ptr rpc_file_p;

/**
 * Query the descriptor of the given file on RPC server side
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
 * Open a file on RPC server side and associate it with a stream
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
 * Close the stream associated with the file.
 *
 * @param rpcs           RPC server handle.
 * @param file           Stream to close.
 *
 * @return   Zero on success, otherwise -1.
 */
extern int rpc_fclose(rcf_rpc_server *rpcs, rpc_file_p file);

/**
 * Execute shell command on the IPC server.
 *
 * @param rpcs          RPC server handle
 * @param cmd           the command to be executed
 *
 * @return status of the process
 */
extern rpc_wait_status rpc_system(rcf_rpc_server *rpcs, const char *cmd);

/**
 * Popen-like command returning pid of spawned process and file descriptor.
 *
 * @param rpcs          RPC server handle
 * @param cmd           the command to be executed
 * @param mode          "r" or "w"
 * @param pid           location to store pid of child
 *
 * @return file descriptor to read/write from/to process.
 */
extern int rpc_popen_fd(rcf_rpc_server *rpcs,
                        const char *cmd, const char *mode, 
                        tarpc_pid_t *pid);

/**
 * Execute shell command on the IPC server and read the output.
 * The routine allocates memory for the output buffer and places
 * null-terminated string to it.
 * @b pbuf pointer is initialized by NULL if no buffer is allocated.
 *
 * @param rpcs          RPC server handle
 * @param pbuf          location for the command output buffer 
 * @param cmd           format of the command to be executed
 *
 * @return status of the process
 */
extern rpc_wait_status rpc_shell_get_all(rcf_rpc_server *rpcs,
                                         char **pbuf, const char *cmd, ...);

/**
 * Execute shell command on the IPC server and return file descriptor
 * for it's standard input or output.
 *
 * @param rpcs          RPC server handle
 * @param pid           pid of spawned process
 * @param mode          access mode. the following values
 *                      are supported
 *                      - "w" write access
 *                      - "r" read access

 * @param cmd           format of the command to be executed
 *
 * @return File descriptor or -1 in the case of failure
 */
extern int rpc_cmd_spawn(rcf_rpc_server *rpcs, int * pid,
                         const char *mode,
                         const char *cmd,...);


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
 * Read all data from file descriptor in the RPC and close the @fd.
 * The routine allocates memory for the output buffer and places
 * null-terminated string to it.
 * @b pbuf pointer is initialized by NULL if no buffer is allocated.
 *
 * @param rpcs          RPC server handle
 * @param fd            file descriptor to read from
 * @param pbuf          location for the command output buffer
 * @param bytes         location for the number of bytes read
 *
 * @retval  0 success
 * @retval -1 failure
 */
extern int rpc_read_all(rcf_rpc_server *rpcs, int fd, 
                        char **pbuf, int *bytes);

/**
 * Fork a process from RPC server and execute a shell command.
 *
 * @param rpcs  RPC server handle
 * @param cmd   command to run in a shell
 *
 * @return pid of child process or -1 on failure
 */
tarpc_pid_t
rpc_fork_and_shell(rcf_rpc_server *rpcs, const char *cmd);

#endif /* !__TE_TAPI_RPC_STDIO_H__ */
