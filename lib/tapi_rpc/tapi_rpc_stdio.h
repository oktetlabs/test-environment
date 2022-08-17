/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of some standard input/output
 * functions and useful extensions.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_RPC_STDIO_H__
#define __TE_TAPI_RPC_STDIO_H__

#include <stdio.h>

#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_stdio TAPI for standard I/O calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/** Maximum resulting command length for rpc_shell() */
#define RPC_SHELL_CMDLINE_MAX   1024

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
 * Execute shell command on the IPC server.
 * You SHOULD not use this function unless you are sure that shell command
 * will exit normally. If this RPC will timeout, you'll have no chance to
 * kill the child process, because you do not know its PID.
 *
 * @param rpcs          RPC server handle
 * @param cmd           format of the command to be executed
 * @param ...           parameters for the command format
 *
 * @return status of the process
 */
extern rpc_wait_status rpc_system_ex(rcf_rpc_server *rpcs,
                                     const char *cmd, ...);

/**
 * Open a process by creating a pipe, forking, and invoking the shell.
 *
 * @note Do not use this function unless you test it - use
 *       rpc_te_shell_cmd instead.
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
 * Execute shell command on the IPC server and read the output.
 * Fail if there was anything on stderr.
 * The routine allocates memory for the output buffer and places
 * null-terminated string to it.
 * @b pbuf pointer is initialized by NULL if no buffer is allocated.
 * If @b pbuf is NULL, stdout is not redirected.
 *
 * @param rpcs          RPC server handle
 * @param pbuf          location for the command output
 * @param cmd           format of the command to be executed
 * @param ...           parameters for command
 *
 * @return status of the process
 */
extern rpc_wait_status rpc_shell_get_all2(rcf_rpc_server *rpcs,
                                          char **pbuf,
                                          const char *cmd, ...);

/**
 * Execute shell command on the IPC server and read the output and stderr.
 * The routine allocates memory for the output buffers and places
 * null-terminated strings to them.
 * @b pbuf[0] and pbuf[1] pointers are initialized by NULL if no buffer
 * are allocated.
 * If @b pbuf is NULL, stdout is not redirected.
 *
 * @param rpcs          RPC server handle
 * @param pbuf          array of locations for the command output and stderr
 * @param cmd           format of the command to be executed
 * @param ...           parameters for command
 *
 * @return status of the process
 */
extern rpc_wait_status rpc_shell_get_all3(rcf_rpc_server *rpcs,
                                          char **pbuf,
                                          const char *cmd, ...);


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
 * @param out_fd    location to store pipe to stdout or NULL
 * @param err_fd    location to store pipe to stderr or NULL
 * @param ...       parameters to command format
 *
 * @return pid of spawned process or -1 on failure
 */
extern tarpc_pid_t rpc_te_shell_cmd(rcf_rpc_server *rpcs,
                                    const char *cmd, tarpc_uid_t uid,
                                    int *in_fd, int *out_fd, int *err_fd,
                                    ...);

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
 * Get integer environment variable.
 *
 * @param rpcs  RPC server handle
 * @param name  Variable name
 * @param val   Value location
 *
 * @return @c 0 on success, otherwise @c -1
 */
static inline int
rpc_getenv_int(rcf_rpc_server *rpcs, const char *name, int *val)
{
    char *strval = rpc_getenv(rpcs, name);

    if (strval == NULL)
        return -1;

    *val = atoi(strval);
    free(strval);
    return 0;
}

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
 * Remove environment variable.
 *
 * @param rpcs          RPC server handle
 * @param name          variable name
 *
 * @return return code
 */
extern int rpc_unsetenv(rcf_rpc_server *rpcs, const char *name);


/**@} <!-- END te_lib_rpc_stdio --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_STDIO_H__ */
