/** @file
 * @brief API to call shell commands
 *
 * @defgroup te_tools_te_shell_cmd Call shell commands
 * @ingroup te_tools
 * @{
 *
 * Routines to call shell commands
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Alexandra Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 *
 */

#ifndef __TE_SHELL_CMD_H__
#define __TE_SHELL_CMD_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_exec_child.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function to base system()-like and popen()-like functions on it.
 * You MUST use uid parameter instead of "su - user -c", because su makes
 * one more fork, and you do not know how to kill this grandchild process.
 *
 * If you use this fuction from agent, this process SHOULD be catched by
 * ta_waitpid() or killed by ta_kill_death().
 *
 * @param cmd    command to run in the shell
 * @param uid    user id to run the shell; use -1 for default
 * @param in_fd  location to store file descriptor of pipe to stdin of
 *               the shell command; use NULL for standard stdin
 * @param out_fd location to store file descriptor of pipe to stdout of
 *               the shell command; use NULL for standard stdout
 * @param err_fd location to store file descriptor of pipe to stderr of
 *               the shell command; use NULL for standard stderr
 *
 * @return pid value, positive on success, negative on failure
 */
extern pid_t te_shell_cmd(const char *cmd, uid_t uid,
                          int *in_fd, int *out_fd, int *err_fd);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_SHELL_CMD_H__ */
/**@} <!-- END te_tools_te_shell_cmd --> */
