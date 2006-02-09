/** @file
 * @brief Test Environment
 *
 * RCF library for UNIX Test Agents
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "RCF Unix"

#include "te_config.h"

/*
 * To get getpgid() prototype from unistd.h.
 * 'man' recommends to define _XOPEN_SOURCE  and _XOPEN_SOURCE_EXTENDED,
 * but prototype of snprintf() is lost in such case.
 */
#define _GNU_SOURCE

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_shell_cmd.h"
#include "rcf_api.h"
#include "rcf_methods.h"

#include "comm_net_engine.h"

#include "logger_api.h"

/*
 * Configuration string for UNIX TA should have format:
 *
 * [[user@]<IP address or hostname>]:<port>
 *     [:key=<ssh private key file>][:notcopy][:sudo][:<shell>][:parameters]
 *
 * If host is not specified, the Test Agent is started on the local
 * host.  It is assumed that user starting Dispatcher may use ssh/scp
 * with specified host using ssh without password.  If sudo is specified
 * it is assumed that user is sudoer without password.
 *
 * notcopy may be used to create symbolic link instead copying of the image.
 *
 * Note that shell part of configuration string CANNOT contain collons.
 * Implementation should be extended to allow collons inside parameter.
 *
 * First parameter of the Test Agent executable is a name of the TA;
 * second one is a TCP port.
 */

#define RCFUNIX_SSH         "ssh -qxTn -o BatchMode=yes "
#define RCFUNIX_REDIRECT    ">/dev/null 2>&1"

#define RCFUNIX_KILL_TIMEOUT    60
#define RCFUNIX_COPY_TIMEOUT    60
#define RCFUNIX_START_TIMEOUT   40

#define RCFUNIX_SHELL_CMD_MAX   2048

#define RCFUNIX_WAITPID_N_MAX       100
#define RCFUNIX_WAITPID_SLEEP_US    10000


/*
 * This library is appropriate for usual and proxy UNIX agents.
 * All agents which type has postfix "ctl" are assumed as proxy.
 * All other agents are not proxy.
 */


/** UNIX Test Agent descriptor */
typedef struct unix_ta {
    char     ta_name[RCF_MAX_NAME];   /**< Test agent name */
    char     ta_type[RCF_MAX_NAME];   /**< Test Agent type */
    char     host[RCF_MAX_NAME];      /**< Test Agent host */
    char     port[RCF_MAX_NAME];      /**< TCP port */
    char     exec_name[RCF_MAX_PATH]; /**< Name of the started file */
    char     key[RCF_MAX_PATH];       /**< Private ssh key file */
    te_bool  sudo;                    /**< Manipulate process using sudo */
    te_bool  notcopy;                 /**< Do not copy TA image to remote
                                           host */
    te_bool  is_local;                /**< TA is started on the local PC */
    uint32_t pid;                     /**< TA pid */
    int     *flags;                   /**< Flags */
    pid_t    start_pid;               /**< PID of the SSH process which
                                           started the agent */
    
    struct rcf_net_connection *conn;  /**< Connection handle */
} unix_ta;

/**
 * Execute the command without forever blocking.
 *
 * @param cmd           command to be executed
 * @param timeout       timeout in seconds
 *
 * @return Status code.
 * @return TE_ETIMEDOUT    Command timed out
 */   
static int
system_with_timeout(const char *cmd, int timeout)
{
    pid_t           pid;
    int             fd;
    char            buf[64] = { 0, };
    int             rc;
    int             status;
    unsigned int    waitpid_tries = 0;

    pid = te_shell_cmd_inline(cmd, -1, NULL, &fd);
    if (pid < 0)
    {
        rc = errno;
        ERROR("te_shell_cmd() for the command <%s> failed", cmd);
        return TE_OS_RC(TE_RCF_UNIX, rc);
    }
    
    while (1)
    {
        struct timeval tv;
        fd_set set;
        
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        
        FD_ZERO(&set);
        FD_SET(fd, &set);
        
        if (select(fd + 1, &set, 0, 0, &tv) == 0)
        {
            ERROR("Command <%s> timed out", cmd);
            if (close(fd) != 0)
                ERROR("Failed to close() pipe from stdout of the shell "
                      "command: %r", TE_OS_RC(TE_RCF_UNIX, errno));
            if (killpg(getpgid(pid), SIGTERM) != 0)
                ERROR("Failed to kill() process of the shell command: %r",
                      TE_OS_RC(TE_RCF_UNIX, errno));
            MSLEEP(100);
            if (killpg(getpgid(pid), SIGKILL) == 0)
                RING("Process of the shell command killed by SIGKILL");
            return TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT);
        }
        
        if (read(fd, buf, sizeof(buf)) == 0)
        {
            if (close(fd) != 0)
                ERROR("Failed to close() pipe from stdout of the shell "
                      "command: %r", TE_OS_RC(TE_RCF_UNIX, errno));

            while (((rc = waitpid(pid, &status, WNOHANG)) == 0) &&
                   (waitpid_tries++ < RCFUNIX_WAITPID_N_MAX))
            {
                usleep(RCFUNIX_WAITPID_SLEEP_US);
            }
            if (rc < 0)
            {
                rc = TE_OS_RC(TE_RCF_UNIX, errno);
                ERROR("Waiting of the shell command <%s> pid %d "
                      "error: %r", cmd, (int)pid, rc);
                return rc;
            }
            else if (rc == 0)
            {
                ERROR("Shell command <%s> seems to be finished, "
                      "but no child was available", cmd);
            }
            else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            {
                return TE_RC(TE_RCF_UNIX, TE_ESHCMD);
            }

            return 0;
        }
    }
    
    /* Unreachable */
}


/**
 * Start the Test Agent. Note that it's not necessary
 * to restart the proxy Test Agents after rebooting of
 * the NUT, which it serves.
 *
 * @param ta_name       Test Agent name
 * @param ta_type       Test Agent type (Test Agent executable is equal
 *                      to ta_type and is located in TE_INSTALL/agents/bin)
 * @param conf_str      TA-specific configuration string
 * @param handle        location for TA handle
 * @param flags         location for TA flags
 *
 * @return error code
 */
int
rcfunix_start(char *ta_name, char *ta_type, char *conf_str,
              rcf_talib_handle *handle, int *flags)
{
    static unsigned int seqno = 0;

    int      rc;
    unix_ta *ta;
    char    *token;
    char     path[RCF_MAX_PATH];
    char     cmd[RCFUNIX_SHELL_CMD_MAX];
    char    *installdir;
    char    *tmp;
    char    *dup;
    char    *shell;

    RING("Starting TA '%s' type '%s' conf_str '%s'",
         ta_name, ta_type, conf_str);

/** Get the next token from configuration string */
#define GET_TOKEN \
    do {                                \
        token = conf_str;               \
        if (conf_str != NULL)           \
        {                               \
            tmp = index(conf_str, ':'); \
            if (tmp == NULL)            \
                conf_str = NULL;        \
            else                        \
            {                           \
                *tmp = 0;               \
                conf_str = tmp + 1;     \
            }                           \
        }                               \
    } while (FALSE)


    if (ta_name == NULL || ta_type == NULL || 
        strlen(ta_name) >= RCF_MAX_NAME ||
        strlen(ta_type) >= RCF_MAX_NAME ||
        conf_str == NULL || flags == NULL)
    {
        return TE_EINVAL;
    }

    if ((installdir = getenv("TE_INSTALL")) == NULL)
    {
        VERB("FATAL ERROR: TE_INSTALL is not exported");
        return TE_ENOENT;
    }
    sprintf(path, "%s/agents/bin/ta%s", installdir, ta_type);

#if HAVE_SYS_STAT_H
    {
        struct stat statbuf;

        if (stat(path, &statbuf) != 0 || !(statbuf.st_mode & S_IXOTH))
        {
            ERROR("Permission denied to execute %s", path);
            return TE_ENOENT;
        }
    }
#endif

    if ((ta = *(unix_ta **)(handle)) == NULL &&
        (ta = (unix_ta *)calloc(1, sizeof(unix_ta))) == NULL)
    {
        ERROR("Memory allocation failure: %u bytes",
                  sizeof(unix_ta));
        return TE_ENOMEM;
    }

    strcpy(ta->ta_name, ta_name);
    strcpy(ta->ta_type, ta_type);

    if (strcmp(ta_type + strlen(ta_type) - strlen("ctl"), "ctl") == 0)
        *flags |= TA_PROXY;

    ta->flags = flags;
    tmp = getenv("LOGNAME");
    sprintf(ta->exec_name, "ta%s_%s_%u_%u", ta_type,
            (tmp == NULL) ? "" : tmp, (unsigned int)time(NULL), seqno++);

    VERB("Executable name '%s'", ta->exec_name);

    if ((conf_str = strdup(conf_str)) == NULL)
    {
        ERROR("Failed to duplicate string '%s'", conf_str);
        return TE_ENOMEM;
    }
    dup = conf_str;

    GET_TOKEN;
    if (token == NULL)
        goto bad_confstr;
    if (strlen(token) == 0)
    {
        ta->is_local = TRUE;
        *flags |= TA_LOCAL;
        sprintf(ta->host, "127.0.0.1");
    }
    else
    {
        ta->is_local = FALSE;
        strncpy(ta->host, token, RCF_MAX_NAME);
    }
    VERB("Test Agent host %s", ta->host);

    GET_TOKEN;
    if (token == NULL || strlen(token) == 0 ||
        (strtol(token, &tmp, 0), (tmp == token || *tmp != 0)))
        goto bad_confstr;

    strncpy(ta->port, token, RCF_MAX_NAME);

    GET_TOKEN;
    if (token != NULL && strcmp_start("key=", token) == 0)
    {
        char *key = token + strlen("key=");
        
        if (strlen(key) > 0)
            sprintf(ta->key, "-i %s", key); 
        
        GET_TOKEN;
    }
    if (token != NULL && strcmp(token, "notcopy") == 0)
    {
        ta->notcopy = TRUE;
        GET_TOKEN;
    }
    else
        ta->sudo = FALSE;
    if (token != NULL && strcmp(token, "sudo") == 0)
    {
        ta->sudo = TRUE;
        GET_TOKEN;
    }
    else
        ta->sudo = FALSE;

    shell = token;

    /* 
     * It's assumed that the rest of configuration string should be 
     * passed to agent.
     */

    if (ta->is_local)
    {
        /* 
         * DO NOT suppress command output in order to have a chance
         * to see possible problems.
         */
        if (ta->notcopy)
            sprintf(cmd, "ln -s %s /tmp/%s", path, ta->exec_name);
        else
            sprintf(cmd, "cp -a %s /tmp/%s", path, ta->exec_name);
    }
    else
    {
        if (ta->notcopy)
        {
            sprintf(cmd, RCFUNIX_SSH "%s %s ln -s %s /tmp/%s",
                    ta->key, ta->host, path, ta->exec_name);
        }
        else
        {
            /* 
             * Preserves modification times, access times, and modes.
             * Disables the progress meter.
             * Be quite, but DO NOT suppress command output in order
             * to have to see possible problems.
             */
            sprintf(cmd, "scp -Bpq %s %s %s:/tmp/%s",
                    ta->key, path, ta->host, ta->exec_name);
        }
    }

    VERB("%s", cmd);
    if (!(*flags & TA_FAKE) &&
        ((rc = system_with_timeout(cmd, RCFUNIX_COPY_TIMEOUT)) != 0))
    {
        ERROR("Failed to copy TA image %s to the %s:/tmp: %r",
              ta->exec_name, ta->host, rc);
        free(dup);
        return rc;
    }
    
    if (!(*flags & TA_FAKE) && strcmp_start("win32", ta->ta_type) == 0)
    {
        /* Copy RPC server image */
        FILE *f;
        
        sprintf(cmd, "%s_rpcserver.exe", path);
        if ((f = fopen(cmd, "r")) != NULL)
        {
            fclose(f);
            sprintf(cmd, 
                    "scp -Bpq %s %s_rpcserver.exe %s:/tmp/%s_rpcserver",
                    ta->key, path, ta->host, ta->exec_name);
                
            if ((rc = system_with_timeout(cmd, RCFUNIX_COPY_TIMEOUT)) != 0)
            {
                ERROR("Failed to copy RPC server image %s_rpcserver "
                      "to the %s:/tmp: %r", ta->exec_name, ta->host, rc);
                sprintf(cmd, RCFUNIX_SSH "%s %s rm /tmp/%s", 
                        ta->key, ta->host, ta->exec_name);
                
                if (system_with_timeout(cmd, RCFUNIX_COPY_TIMEOUT) != 0)
                    ERROR("Failed to remove TA image from %s", ta->host);
                    
                free(dup);
                return rc;
            }
        }

        sprintf(cmd, "%s_rpcserver64.exe", path);
        if ((f = fopen(cmd, "r")) != NULL)
        {
            fclose(f);
            sprintf(cmd, 
                    "scp -Bpq %s %s_rpcserver64.exe %s:/tmp/%s_rpcserver64",
                    ta->key, path, ta->host, ta->exec_name);
                
            if ((rc = system_with_timeout(cmd, RCFUNIX_COPY_TIMEOUT)) != 0)
            {
                ERROR("Failed to copy RPC server image %s_rpcserver "
                      "to the %s:/tmp: %r", ta->exec_name, ta->host, rc);
                sprintf(cmd, RCFUNIX_SSH "%s %s rm /tmp/%s*", 
                        ta->key, ta->host, ta->exec_name);
                
                if (system_with_timeout(cmd, RCFUNIX_COPY_TIMEOUT) != 0)
                    ERROR("Failed to remove TA image from %s", ta->host);
                    
                free(dup);
                return rc;
            }
        }
    }
    
    /* Clean up command string */
    cmd[0] = '\0';

    if (!ta->is_local)
    {
        sprintf(cmd, RCFUNIX_SSH " %s \"", ta->host);
    }
    if (ta->sudo)
    {
        strcat(cmd, "sudo ");
    }
    if ((shell != NULL) && (strlen(shell) > 0))
    {
        VERB("Using '%s' as shell for TA '%s'", shell, ta->ta_name);
        strcat(cmd, shell);
        strcat(cmd, " ");
    }

    /* 
     * Test Agent is always running in background, therefore it's
     * necessary to redirect its stdout and stderr to a file.
     */
    sprintf(cmd + strlen(cmd), "/tmp/%s %s %s %s",
            ta->exec_name, ta->ta_name, ta->port,
            (conf_str == NULL) ? "" : conf_str);

    /* Enquote command in double quotes for non-local agent */
    if (!ta->is_local)
    {
        sprintf(cmd + strlen(cmd), "\"");
    }
    sprintf(cmd + strlen(cmd), " 2>&1 | te_tee %s %s 10 >ta.%s ", 
            TE_LGR_ENTITY, ta->ta_name, ta->ta_name);

#if 0
    /* Always run in background */
    strcat(cmd, " &");
#endif

    free(dup);

    VERB("Command to start TA: %s", cmd);
    if (!(*flags & TA_FAKE) &&
        ((ta->start_pid = te_shell_cmd_inline(cmd, -1, NULL, NULL)) <= 0))
    {
        rc = TE_OS_RC(TE_RCF_UNIX, errno);
        ERROR("Failed to start TA %s: %r", ta_name, rc);
        return rc;
    }

    *handle = (rcf_talib_handle)ta;

    return 0;

bad_confstr:
    free(dup);
    VERB("Bad configuration string for TA '%s'", ta_name);
    return TE_RC(TE_RCF_UNIX, TE_EINVAL);
}

/**
 * Kill all processes related to TA on the station where it is run.
 * Reboot station which TA is runing on (if it's allowed).
 * Handle should not be freed.
 *
 * @param handle        TA handle locaton, may already contain memory
 *                      pointer in the case of TA restart
 * @param parms         library-specific parameters
 *
 * @return error code 
 */
int
rcfunix_finish(rcf_talib_handle handle, char *parms)
{
    unix_ta *ta = (unix_ta *)handle;
    int      rc;

    char  cmd[RCFUNIX_SHELL_CMD_MAX];
    
    (void)parms;

    if (ta == NULL)
        return TE_EINVAL;

    RING("Finish method is called for TA %s", ta->ta_name);
    
    if (*(ta->flags) & TA_FAKE)
        return 0;
    
    if (ta->pid > 0 && (*(ta->flags) & TA_DEAD))
    {
        /* Kill TA itself */
        if (ta->is_local)
        {
            kill(ta->pid, SIGTERM);
            kill(ta->pid, SIGKILL);
        }
        else
        {
            sprintf(cmd,
                    RCFUNIX_SSH " %s \"%skill %d\" " RCFUNIX_REDIRECT,
                    ta->host, ta->sudo ? "sudo " : "" , ta->pid);
            rc = system_with_timeout(cmd, RCFUNIX_KILL_TIMEOUT);
            if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
                return rc;
    
            sprintf(cmd,
                    RCFUNIX_SSH " %s \"%skill -9 %d\" " RCFUNIX_REDIRECT,
                    ta->host, ta->sudo ? "sudo " : "" , ta->pid);
            rc = system_with_timeout(cmd, RCFUNIX_KILL_TIMEOUT);
            if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
                return rc;
        }

        if (ta->is_local)
            sprintf(cmd,
                    "%skillall %s " RCFUNIX_REDIRECT,
                    ta->sudo ? "sudo " : "" , ta->exec_name);
        else
            sprintf(cmd,
                    RCFUNIX_SSH " %s \"%skillall %s\" " RCFUNIX_REDIRECT,
                    ta->host, ta->sudo ? "sudo " : "" , ta->exec_name);
        rc = system_with_timeout(cmd, RCFUNIX_KILL_TIMEOUT);
        if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
            return rc;

        if (ta->is_local)
            sprintf(cmd,
                    "%skillall -9 %s " RCFUNIX_REDIRECT,
                    ta->sudo ? "sudo " : "" , ta->exec_name);
        else
            sprintf(cmd,
                    RCFUNIX_SSH " %s \"%skillall -9 %s\" " RCFUNIX_REDIRECT,
                    ta->host, ta->sudo ? "sudo " : "" , ta->exec_name);
        rc = system_with_timeout(cmd, RCFUNIX_KILL_TIMEOUT);
        if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
            return rc;
    }

    /* FIXME */
    if ((*(ta->flags) & TA_DEAD) || strcmp_start("win32", ta->ta_type) == 0)
    {
        if (ta->is_local)
            sprintf(cmd, "rm -f /tmp/%s*", ta->exec_name);
        else
            sprintf(cmd, RCFUNIX_SSH " %s \"rm -f /tmp/%s*\"",
                    ta->host, ta->exec_name);
        rc = system_with_timeout(cmd, RCFUNIX_KILL_TIMEOUT);
        if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
            return rc;
    }
    
    if (ta->start_pid > 0)
    {
        killpg(getpgid(ta->start_pid), SIGTERM);
        killpg(getpgid(ta->start_pid), SIGKILL);
    }

    return 0;
}

/**
 * Close all interactions with TA.
 * 
 * @param handle        TA handle
 * @param select_set    FD_SET to be updated with the TA connection file
 *                      descriptor (for Test Agents supporting listening
 *                      mode) (IN/OUT)
 *
 * @return error code
 */
int
rcfunix_close(rcf_talib_handle handle, fd_set *select_set)
{
    return rcf_net_engine_close(&(((unix_ta *)handle)->conn), select_set);
}

/**
 * Establish connection with the Test Agent. Note that it's not necessary
 * to perform real reconnect to proxy Test Agents after rebooting of
 * the NUT, which it serves.
 *
 * @param handle        TA handle
 * @param select_set    FD_SET to be updated with the TA connection file
 *                      descriptor (for Test Agents supporting listening
 *                      mode) (IN/OUT)
 *
 * @param select_tm     Timeout value for the select to be updated with
 *                      TA polling interval (for Test Agents supporting
 *                      polling mode only)
 *                      (IN/OUT)
 *
 * @return error code
 */
int
rcfunix_connect(rcf_talib_handle handle, fd_set *select_set,
                struct timeval *select_tm)
{
    int     rc;
    char    buf[16];
    char   *tmp;
    size_t  len = 16;
    char   *host;
    
    (void)select_tm;

    unix_ta *ta = (unix_ta *)handle;
    
    host = strchr(ta->host, '@');
    if (host != NULL)
        host++;
    else
        host = ta->host;

    VERB("Connecting to TA '%s'", ta->ta_name);

    if ((rc = rcf_net_engine_connect(host, ta->port, &ta->conn, 
                                     select_set)) != 0)
    {
        return rc;
    }

    if ((rc = rcf_net_engine_receive(ta->conn, buf, &len, &tmp)) != 0)
    {
        ERROR("Cannot read TA PID from the TA %s (error %x)", ta->ta_name, 
              rc);
    }
    
    if (strncmp(buf, "PID ", 4) != 0 || 
        (ta->pid = strtol(buf + 4, &tmp, 10), *tmp != 0))
    {
        ta->pid = 0;
        return TE_RC(TE_RCF, TE_EINVAL);
    }
    
    INFO("PID of TA %s is %d", ta->ta_name, ta->pid);
    
    return 0;
}

/**
 * Transmit data to the Test Agent.
 *
 * @param handle        TA handle
 * @param data          data to be transmitted
 * @param len           data length
 *
 * @return error code
 */
int
rcfunix_transmit(rcf_talib_handle handle, char *data, size_t len)
{
    return rcf_net_engine_transmit(((unix_ta *)handle)->conn, data, len);
}

/**
 * Check pending data on the Test Agent connection.
 *
 * @param handle        TA handle
 *
 * @return TRUE, if data are pending; FALSE otherwise
 */
te_bool
rcfunix_is_ready(rcf_talib_handle handle)
{
    return (handle == NULL) ? FALSE :
               rcf_net_engine_is_ready(((unix_ta *)handle)->conn);
}

/**
 * Receive one commend (possibly with attachment) from the Test Agent
 * or its part.
 *
 * @param handle        TA handle
 * @param buf           location for received data
 * @param len           should be filled by the caller to length of
 *                      the buffer; is filled by the routine to length of
 *                      received data
 * @param pba           location for address of first byte after answer
 *                      end marker (is set only if binary attachment
 *                      presents)
 *
 * @return error code
 * @retval 0            success
 *
 * @retval TE_ESMALLBUF Buffer is too small for the command. The part
 *                      of the command is written to the buffer. Other
 *                      part(s) of the message can be read by the subsequent
 *                      routine calls. ETSMALLBUF is returned until last
 *                      part of the message is read.
 *
 * @retval TE_EPENDING  Attachment is too big to fit into the buffer.
 *                      The command and a part of the attachment is written
 *                      to the buffer. Other part(s) can be read by the
 *                      subsequent routine calls. TE_EPENDING is returned
 *                      until last part of the message is read.
 *
 * @retval other        OS errno
 */
int
rcfunix_receive(rcf_talib_handle handle, char *buf, size_t *len, char **pba)
{
    return rcf_net_engine_receive(((unix_ta *)handle)->conn, buf, len, pba);
}

