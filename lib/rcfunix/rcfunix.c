/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 *
 * @page rcfunix_conflib RCF Library for UNIX Test Agents
 *
 * @brief Test Environment: RCF library for UNIX Test Agents
 *
 * @ref te_engine_rcf_comm_lib_unix should be used to control and interact
 * with @ref te_agents on Unix-like hosts. It uses @prog{ssh} and @prog{scp}
 * utilities to run commands on other hosts and to copy files
 * (Test Agent executables) to other hosts. Output from Test Agent
 * (@var{stdout} and @var{stderr}) is directed to TE log with
 * @ref te_engine_rcf as logging entity name and Test Agent name as
 * logging user name and to @path{ta.<name>} file in run directory.
 *
 *
 * Apart from @attr_name{rcflib} attribute for each Test Agent
 * @ref te_engine_rcf configuration file contains @attr_name{confstr}
 * attribute that specifies configuration string passed to communication
 * library. The format of this configuration string is library specific.
 *
 * Configuration string of the @ref te_engine_rcf_comm_lib_unix has
 * the following format:
 * <pre class="fragment">
 * [[@attr_val{user}@]@attr_val{<IP_address_or_hostname>}]:@attr_val{<port>}
 * [:@attr_name{key}=@attr_val{<ssh_private_key_file>}]
 * [:@attr_name{ssh_port}=@attr_val{<port>}]
 * [:@attr_name{ssh_proxy}=@attr_val{<ssh-proxy>}]
 * [:@attr_name{copy_timeout}=@attr_val{<timeout>}]
 * [:@attr_name{copy_tries}=@attr_val{<number_of_tries>}]
 * [:@attr_name{kill_timeout}=@attr_val{<timeout>}]
 * [:@attr_val{sudo}][:@attr_val{<shell>}][:@attr_val{<parameters>}]
 * </pre>
 *
 * where elements in square brackets are optional and may be skipped.
 * - @attr_val{<IP_address_or_hostname>} - is IPv4, IPv6 or DNS address of
 *   the host where to run Test Agent. If the value is an empty string
 *   (skipped), then the Test Agent runs on local host (on a host where
 *   @ref te_engine runs);
 * - @attr_val{user} - if specified it is the user to log in as on the
 *   @attr_val{<IP_address_or_hostname>};
 * - @attr_val{<port>} - is TCP port to bind TCP server on the Test Agent
 *  (@ref te_engine_rcf_comm_lib_unix is based on TCP sockets and Test Agent
 *   plays role of TCP server in connection establishment with
 *   @ref te_engine_rcf, which means @ref te_engine_rcf side shall know to
 *   which address and port to connect);
 * - @attr_name{key} - specifies file from which the identity (private key)
 *   for RSA or DSA authentication is read;
 * - @attr_name{ssh_port} - specifies TCP port to be used by SSH. May be
 *   unspecified or 0 to use standard SSH port 22;
 * - @attr_name{ssh_proxy} - specifies SSH proxy to be used, it may
 *   include SSH options and must include proxy host name or IP address;
 * - @attr_name{copy_timeout} - specifies the maximum time duration
 *   (in seconds) that is allowed for image copy operation. If image copy
 *   takes more than this timeout, Test Agent start-up procedure fails,
 *   provided that the copy operation was the last try (see copy_tries);
 * - @attr_name{copy_tries} - specifies the number of tries to perform
 *   image copy operation. The time to wait between tries is doubled from
 *   RCFUNIX_COPY_RETRY_SLEEP_FIRST_SEC to RCFUNIX_COPY_RETRY_SLEEP_MAX_SEC
 *   with every next try. If all copy tries fail, Test Agent
 *   start-up procedure fails;
 * - @attr_name{kill_timeout} - specifies the maximum time duration
 *   (in seconds) that is allowed for Test Agent termination procedure;
 * - @attr_val{sudo} - specify this option when we need to run agent under
 *   @prog{sudo} (with root privileges). This can be necessary if Test Agent
 *   access resources that require privileged permissions (for example
 *   network interface configuration);
 * - @attr_val{<shell>} - is usually used to run the Test Agent under
 *   @prog{valgrind} tool with a set of options
 *   (e.g. @prog{valgrind} @prog_option{--tool=memcheck}).
 *   Note that this part of configuration string CANNOT contain collons;
 * - @attr_val{<parameters>} - string value that is transparently passed to
 *   the Test Agent executables as command-line parameters (each token
 *   separated with spaces will go as a separate command line parameter).
 * .
 *
 * Here are few examples of configuration strings suitable for
 * @ref te_engine_rcf_comm_lib_unix :
 * -
 * <pre class="fragment">
 * <ta name="Agt_A" type="linux" rcflib="rcfunix"
 *     confstr=@attr_val{":12000::"}/>
 * </pre>
 * will run Test Agent on localhost host with user privileges,
 * TCP server will be bound to port 12000;
 * -
 * <pre class="fragment">
 * <ta name="Agt_A" type="linux" rcflib="rcfunix"
 *     confstr=@attr_val{":12000::valgrind --tool=helgrind:"}/>
 * </pre>
 * will run Test Agent on localhost host with user privileges,
 * TCP server will be bound to port 12000, the Test Agent will be run under
 * @prog{valgrind} with a data-race detection tool;
 * -
 * <pre class="fragment">
 * <ta name="Agt_A" type="linux" rcflib="rcfunix"
 *     confstr=@attr_val{"loki:12000:sudo:valgrind --tool=memcheck --num-callers=32:"}/>
 * </pre>
 * will run Test Agent on @attr_val{loki} host with root privileges.
 * The Test Agent will be run under @prog{valgrind} with memory checking
 * tool and 32-depth call backtrace.
 * .
 *
 * @note
 * It is worth nothing that a user who runs @ref te_engine_dispatcher script
 * should be able to enter hosts specified in @ref te_engine_rcf
 * configuration file without password prompt (e.g. using public key).
 * It requires special tunings of SSH daemon on remote host as well.
 * File name that keeps private key for a particular Test Agent can be
 * specified with @attr_name{key} option of configuration string.
 *
 * @note
 * If @attr_val{sudo} element is specified in the configuration string of
 * a Test Agent, then it is assumed that user is sudoer without password.
 *
 * @section rcfunix_conflib Building RCF UNIX library
 * In order to build RCF UNIX library you should just mention rcfunix
 * in the list of platform libraries:
 * TE_PLATFORM([], [], [-D_GNU_SOURCE], [-D_GNU_SOURCE], [],
 *             [tools ... rcfunix ...])
 * This will build default version of rcfunix suitable for Linux,
 * but there can be some UNIX specific issues that need to be resolved
 * with an alternative build of rcfunix. For example there is a minor
 * difference of this library for Solaris (On Solaris we should use
 * 'coreadm' utility to configure the name of core files).
 * To sort this out we can build a version of rcfunix for Solaris:
 *
 * TE_PLATFORM([], [], [-D_GNU_SOURCE], [-D_GNU_SOURCE], [],
 *             [tools ... rcfunix rcfsolaris ...])
 * TE_LIB_PARMS([rcfsolaris], [], [rcfunix], [], [-DRCF_UNIX_SOLARIS])
 *
 * Then in RCF configuration file we can specify 'rcfsolaris' as
 * the value of 'rcflib' attribute.
 */

/*
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 *
 *
 *
 *
 */

#define TE_LGR_USER     "RCF Unix"

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
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

#include <dirent.h>

#include "te_alloc.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_shell_cmd.h"
#include "te_sleep.h"
#include "te_string.h"
#include "te_str.h"
#include "te_kvpair.h"
#include "te_proto.h"
#include "rcf_api.h"
#include "rcf_methods.h"
#include "rcf_tce_conf.h"

#include "comm_net_engine.h"

#include "logger_api.h"

/*
 * Configuration string for UNIX TA should have format:
 *
 * [[user@]<IP address or hostname>]:<port>
 *     [:key=<ssh private key file>][:ssh_port=<port>][:ssh_proxy=<hostname>]
 *     [:copy_timeout=<timeout>][:kill_timeout=<timeout>]
 *     [:sudo][:<shell>][:parameters]
 *
 * If host is not specified, the Test Agent is started on the local
 * host.  It is assumed that user starting Dispatcher may use ssh/scp
 * with specified host using ssh without password.  If sudo is specified
 * it is assumed that user is sudoer without password.
 *
 * Note that shell part of configuration string CANNOT contain collons.
 * Implementation should be extended to allow collons inside parameter.
 *
 * First parameter of the Test Agent executable is a name of the TA;
 * second one is a TCP port.
 */

#define RCFUNIX_SSH         "ssh -qxT -o BatchMode=yes"
#define NO_HKEY_CHK         "-o StrictHostKeyChecking=no"
#define RCFUNIX_REDIRECT    ">/dev/null 2>&1"

#define RCFUNIX_KILL_TIMEOUT    15
#define RCFUNIX_COPY_TIMEOUT    30
#define RCFUNIX_COPY_TRIES      1
#define RCFUNIX_COPY_RETRY_SLEEP_FIRST_SEC  1
#define RCFUNIX_COPY_RETRY_SLEEP_MAX_SEC    10

/** Maximum sleep between reconnect attempts */
#define RCFUNIX_RECONNECT_SLEEP_MAX 5

#define RCFUNIX_WAITPID_N_MAX       100
#define RCFUNIX_WAITPID_SLEEP_US    10000

#define RCFUNIX_DEF_CORE_PATTERN "/var/tmp/core.te.%h-%p-%t"

/*
 * This library is appropriate for usual and proxy UNIX agents.
 * All agents which type has postfix "ctl" are assumed as proxy.
 * All other agents are not proxy.
 */

/** The state of the TCE information. */
typedef enum {
    RCFUNIX_TCE_S_NA = 0,  /**< is in an undefined state */
    RCFUNIX_TCE_S_CLEARED, /**< has been cleared */
    RCFUNIX_TCE_S_SAVED,   /**< has been saved */
} rcfunix_tce_state_t;

/** UNIX Test Agent descriptor */
typedef struct unix_ta {
    char    ta_name[RCF_MAX_NAME];  /**< Test agent name */
    char    ta_type[RCF_MAX_NAME];  /**< Test Agent type */
    char    host[RCF_MAX_NAME];     /**< Test Agent host */
    char    connect[RCF_MAX_NAME];  /**< Test Agent address or hostname to
                                         connect */
    char    port[RCF_MAX_NAME];     /**< TCP port */
    char    run_dir[RCF_MAX_PATH];  /**< TA run directory */
    char    key[RCF_MAX_PATH];      /**< Private ssh key file */
    char    user[RCF_MAX_PATH];     /**< User to be used (with @) */
    char    ssh_proxy[RCF_MAX_NAME];/**< SSH proxy host */

    unsigned int    ssh_port;       /**< 0 or special SSH port to use */
    unsigned int    copy_timeout;   /**< TA image copy timeout */
    unsigned int    copy_tries;     /**< Number of times to try to
                                         copy TA image */
    unsigned int    kill_timeout;   /**< TA kill timeout */

    bool sudo;       /**< Manipulate process using sudo */
    bool is_local;   /**< TA is started on the local PC */

    bool ext_rcf_listener;  /**< Listener socket used to accept RCF
                                    connection is created before
                                    exec(ta). This is useful when TA
                                    is created in another network
                                    namespace to which RCF cannot
                                    connect. */

    te_string   ssh_opts;     /**< SSH options common for ssh and sftp */

    te_string   cmd_prefix;     /**< Command prefix */
    te_string   start_prefix;   /**< TA start command prefix */
    const char *cmd_suffix;     /**< Command suffix before redirection */

    uint32_t        pid;        /**< TA pid */
    unsigned int   *flags;      /**< Flags */
    pid_t           start_pid;  /**< PID of the SSH process which
                                     started the agent */

    pid_t core_watcher_pid;     /**< PID of TA core watcher or its
                                     SSH process */
    int core_watcher_in;        /**< TA core watcher standard input
                                     FD. It is used only to terminate
                                     core watcher gracefully. */

    struct rcf_net_connection  *conn;   /**< Connection handle */

    /** The TE engine part of the TCE configuration. */
    const rcf_tce_local_conf_t *tce_local;
    /** The TA agent part of the TCE configuration. */
    const rcf_tce_type_conf_t  *tce_type;
    /** The state of the TCE information. */
    rcfunix_tce_state_t         tce_state;
} unix_ta;

/** Free resources allocated for TA control structure */
static void
rcfunix_ta_free(unix_ta *ta)
{
    te_string_free(&ta->ssh_opts);
    te_string_free(&ta->cmd_prefix);
    te_string_free(&ta->start_prefix);
    free(ta);
}

/**
 * Execute the command without forever blocking.
 *
 * @param timeout       timeout in seconds
 * @param out           where to store stdout (if not NULL)
 * @param fmt           format string of the command to be executed
 *
 * @return Status code.
 * @return TE_ETIMEDOUT    Command timed out
 */
static te_errno
TE_LIKE_PRINTF(3, 4)
system_with_timeout(int timeout, te_string *out, const char *fmt, ...)
{
    va_list         ap;
    char           *cmd;
    pid_t           pid;
    int             fd = -1;
    char            buf[64] = { 0, };
    te_errno        rc;
    int             status;
    unsigned int    waitpid_tries = 0;

    va_start(ap, fmt);
    cmd = te_string_fmt_va(fmt, ap);
    va_end(ap);
    if (cmd == NULL)
    {
        ERROR("Failed to make string with command to run");
        return TE_RC(TE_RCF_UNIX, TE_ENOMEM);
    }

    pid = te_shell_cmd(cmd, -1, TE_EXEC_CHILD_DEV_NULL_FD, &fd, NULL);
    if (pid < 0 || fd < 0)
    {
        rc = TE_OS_RC(TE_RCF_UNIX, errno);
        ERROR("te_shell_cmd() for the command <%s> failed", cmd);
        free(cmd);
        return rc;
    }

    while (1)
    {
        struct timeval tv;
        fd_set set;
        ssize_t read_rc;

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
            te_msleep(100);
            if (killpg(getpgid(pid), SIGKILL) == 0)
                RING("Process of the shell command killed by SIGKILL");
            free(cmd);
            return TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT);
        }

        memset(buf, 0, sizeof(buf));
        if ((read_rc = read(fd, buf, sizeof(buf) - 1)) == 0)
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
                free(cmd);
                return rc;
            }
            else if (rc == 0)
            {
                ERROR("Shell command <%s> seems to be finished, "
                      "but no child was available", cmd);
            }
            else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            {
                free(cmd);
                return TE_RC(TE_RCF_UNIX, TE_ESHCMD);
            }

            free(cmd);
            return 0;
        }
        else if (read_rc > 0 && out != NULL)
        {
            te_string_append(out, "%s", buf);
        }
    }

    /* Unreachable */
}

/**
 * Sudo command prefix if required.
 */
static const char *
rcfunix_ta_sudo(unix_ta *ta)
{
    return ta->sudo ? "sudo -n " : "";
}

/**
 * Get length of the TA type prefix which should be used in TA run
 * directory name.
 *
 * Use up to the second underscore (e.g. linux_ta).
 *
 * @param ta_type       Test agent type
 *
 * @return Prefix length to be used.
 */
static unsigned int
rcfunix_ta_type_prefix_len(const char *ta_type)
{
    char *s = strchr(ta_type, '_');

    if (s != NULL)
    {
        s = strchr(s + 1, '_');
        if (s != NULL)
            return s - ta_type;
    }

    return strlen(ta_type);
}

/**
 * Get hostname to connect to taking host and connect attributes
 * into account.
 *
 * When test agent is started, the function is used to get real
 * host to connect to to setup port forwarding and SSH proxy
 * settings should be ignored.
 *
 * @param ta            Test agent configuration
 * @param ignore_proxy  If SSH proxy should be used
 *
 * @return String with host or IP address to connect to.
 */
static const char *
rcfunix_connect_to(const unix_ta *ta, bool ignore_proxy)
{
    const char *host;

    if (!ignore_proxy && ta->ssh_proxy[0] != '\0')
        host = "localhost";
    else if (ta->connect[0] != '\0')
        host = ta->connect;
    else
        host = ta->host;

    return host;
}

/**
 * Form the shell command to remove the TCE information for a specific
 * TA component.
 *
 * @param      ta   The TA configuration.
 * @param      comp The TCE configuration of a TA component.
 * @param[out] cmd  The command formed.
 */
static void
ta_form_clean_tce_comp_cmd(unix_ta *ta, const rcf_tce_comp_conf_t *comp,
                           te_string *cmd)
{
    const rcf_tce_type_conf_t *type = ta->tce_type;

    te_string_reset(cmd);

    te_string_append(cmd,
                     "%s%s \"%s/ta_clean_tce\" component \"%s/%s\" %s 2>&1 "
                     "| te_tee \"%s\" \"%s\" 10 " RCFUNIX_REDIRECT,
                     te_string_value(&ta->start_prefix), rcfunix_ta_sudo(ta),
                     ta->run_dir, type->base, comp->build, ta->cmd_suffix,
                     TE_LGR_ENTITY, ta->ta_name);
}

/**
 * Form the shell command to remove the base directory of TA builds.
 *
 * @param      ta  The TA configuration.
 * @param[out] cmd The command formed.
 */
static void
ta_form_clean_tce_ws_cmd(unix_ta *ta, te_string *cmd)
{
    const rcf_tce_type_conf_t *type = ta->tce_type;

    te_string_reset(cmd);

    te_string_append(cmd,
                     "%s%s \"%s/ta_clean_tce\" workspace \"%s\" %s 2>&1 "
                     "| te_tee \"%s\" \"%s\" 10 " RCFUNIX_REDIRECT,
                     te_string_value(&ta->start_prefix), rcfunix_ta_sudo(ta),
                     ta->run_dir, type->base, ta->cmd_suffix,
                     TE_LGR_ENTITY, ta->ta_name);
}

/**
 * Clean the TCE information on a TA agent.
 *
 * The sequences of the cleaning:
 * - the TCE information is not interfered by the previous runs,
 * - it does not hold a space after the TA agent have been terminated.
 *
 * @param ta    The TA configuration.
 * @param rm_ws If @c true, remove the base directory of component builds also.
 *
 * @return The status code.
 */
static te_errno
ta_clean_tce(unix_ta *ta, bool rm_ws)
{
    const rcf_tce_type_conf_t *type = ta->tce_type;
    const rcf_tce_comp_conf_t *comp = NULL;
    te_string cmd = TE_STRING_INIT;
    te_errno rc = 0;

    if (type == NULL)
        return 0;

    while ((comp = rcf_tce_get_next_comp_conf(type, comp)) != NULL)
    {
        ta_form_clean_tce_comp_cmd(ta, comp, &cmd);

        INFO("CMD to clean TCE '%s': %s", comp->name, te_string_value(&cmd));

        if ((rc = system_with_timeout(ta->copy_timeout, NULL, "%s",
                                      te_string_value(&cmd))) != 0)
        {
            ERROR("Failed to clean TCE '%s'", comp->name);
            goto out;
        }
    }

    if (rm_ws)
    {
        ta_form_clean_tce_ws_cmd(ta, &cmd);

        INFO("CMD to clean TCE workspace: %s", te_string_value(&cmd));

        if ((rc = system_with_timeout(ta->copy_timeout, NULL, "%s",
                                      te_string_value(&cmd))) != 0)
        {
            ERROR("Failed to clean TCE workspace");
            goto out;
        }
    }

    ta->tce_state = RCFUNIX_TCE_S_CLEARED;

out:
    te_string_free(&cmd);

    return rc;
}

/**
 * Form options for the SSH command to be used to fetch the TCE information
 * from a TA host.
 *
 * @param      ta  The TA configuration.
 * @param[out] cmd The command formed.
 */
static void
ta_form_fetch_tce_ssh_opts(unix_ta *ta, te_string *cmd)
{
    te_string ops = TE_STRING_INIT;

    te_string_reset(cmd);

    if (ta->is_local)
        return;

    if (ta->ssh_port != 0)
        te_string_append(&ops, "-p %u ", ta->ssh_port);;

    if (ta->key[0] != '\0')
        te_string_append(&ops, "%s ", ta->key);

    if (strstr(te_string_value(&ta->ssh_opts), NO_HKEY_CHK) != NULL)
        te_string_append(&ops, "%s ", NO_HKEY_CHK);

    if (strlen(te_string_value(&ops)) != 0)
        te_string_append(cmd, "--ssh-ops=\"%s\" ", te_string_value(&ops));

    if (ta->ssh_proxy[0] != '\0')
        te_string_append(cmd, "--ssh-proxy=\"-W %%h:%%p %s\" ", ta->ssh_proxy);

    te_string_append(cmd, "--ssh=\"%s%s\" ", ta->user, ta->host);

    te_string_free(&ops);
}

/**
 * Form the command to fetch the TCE information provided by a TA component.
 *
 * @param      ta      The TA configuration.
 * @param      ssh_ops The SSH options.
 * @param      comp    The TCE configuration of the TA component.
 * @param[out] cmd     The command formed.
 */
static void
ta_form_fetch_tce_comp_cmd(unix_ta *ta, const te_string *ssh_ops,
                           const rcf_tce_comp_conf_t *comp, te_string *cmd)
{
    const rcf_tce_local_conf_t *local = ta->tce_local;
    const rcf_tce_type_conf_t *type = ta->tce_type;

    te_string_reset(cmd);

    te_string_append(cmd,
                     "\"%s/te_fetch_tce\" %s "
                     "--run=\"%s\" --build=\"%s/%s\" --tcews=\"%s\" "
                     "--type=\"%s\" --comp=\"%s\" --name=\"%s\" 2>&1 "
                     "| te_tee %s %s 10 " RCFUNIX_REDIRECT,
                     local->tebin, te_string_value(ssh_ops),
                     ta->run_dir, type->base, comp->build, local->tcews,
                     ta->ta_type, comp->name, ta->ta_name,
                     TE_LGR_ENTITY, ta->ta_name);
}

/**
 * Fetch and save the TCE information provided by a TA agent.
 *
 * There are two points to save the TCE information:
 * - An RCF/ controller is destroyed on the finish operation.
 * - A TA agent is requested to reboot the TA host when no soft reboot is
 *   allowed.
 * The former is considered to be usual. In this case the TA agent is terminated
 * and then the TCE information is fetched. In the later the TA agent is still
 * running. Thus mark the TCE information is saved and left the second case as
 * a fallback.
 *
 * @param ta The TA configuration.
 *
 * @return The status code.
 */
static te_errno
ta_save_tce(unix_ta *ta)
{
    const rcf_tce_type_conf_t *type = ta->tce_type;
    const rcf_tce_comp_conf_t *comp = NULL;
    te_string ssh = TE_STRING_INIT;
    te_string cmd = TE_STRING_INIT;
    te_errno rc;

    if (type == NULL)
        return 0;

    switch (ta->tce_state)
    {
        case RCFUNIX_TCE_S_SAVED:
            return 0;
        case RCFUNIX_TCE_S_CLEARED:
            break;
        default:
            WARN("TCE: Save not cleared TCE information");
            break;
    }

    ta->tce_state = RCFUNIX_TCE_S_SAVED;

    ta_form_fetch_tce_ssh_opts(ta, &ssh);

    while ((comp = rcf_tce_get_next_comp_conf(type, comp)) != NULL)
    {
        ta_form_fetch_tce_comp_cmd(ta, &ssh, comp, &cmd);

        INFO("CMD to save TCE '%s': %s", comp->name, te_string_value(&cmd));

        if ((rc = system_with_timeout(ta->copy_timeout, NULL, "%s",
                                      te_string_value(&cmd))) != 0)
        {
            ERROR("Failed to fetch TCE '%s'", comp->name);
            break;
        }
    }

    te_string_free(&cmd);
    te_string_free(&ssh);

    return rc;
}

/**
 * Load the TCE configuration on a TA agent started.
 *
 * The RCF/unix controller can be re-started when:
 * - the soft reboot does not allowed,
 * - the TA agent has been rebooted.
 * In the such case the finish operation has not been called and the TCE
 * configuration is already loaded.
 *
 * @param ta    The TA configuration.
 * @param param RCF parameters.
 *
 * @return The status code.
 */
static void
ta_conf_tce(unix_ta *ta, const rcf_talib_param *param)
{
    const rcf_tce_conf_t *conf = param->tce_conf;
    const rcf_tce_type_conf_t *type;

    if (ta->tce_local != NULL)
        return;

    ta->tce_state = RCFUNIX_TCE_S_NA;

    if ((type = rcf_tce_get_type_conf(conf, ta->ta_type)) == NULL)
        return;

    ta->tce_local = &conf->local;
    ta->tce_type = type;
}

/*
 * Start ta_core_watcher on TA host. It will look for new core files
 * in the specified location and print logs with backtraces when
 * the new core file comes from one of the binaries in TA directory.
 * If all flag is set, ta_core_watcher will print backtraces for all
 * binaries.
 */
static void
start_core_watcher(unix_ta *ta, const char *core_pattern, bool all)
{
    int i;
    long int len = 0;

    static const int sleep_time = 10000;
    static const int max_attempts = 500;

    te_string cores_log = TE_STRING_INIT_STATIC(RCF_MAX_PATH);
    te_string cmd = TE_STRING_INIT;

    if (core_pattern == NULL)
        core_pattern = "";

    te_string_append(&cores_log, "ta_cores.%s", ta->ta_name);

    te_string_append(&cmd, "%s%s%s/ta_core_watcher \\\"%s\\\" %s%s%s "
                     " %s 2>&1 | te_tee %s %s 10 >%s",
                     te_string_value(&ta->cmd_prefix),
                     rcfunix_ta_sudo(ta),
                     ta->run_dir, core_pattern,
                     all ? "" : "\\\"", all ? "" : ta->run_dir,
                     all ? "" : "\\\"", ta->cmd_suffix,
                     TE_LGR_ENTITY, ta->ta_name,
                     te_string_value(&cores_log));

    RING("Command to start core_watcher: %s",
         te_string_value(&cmd));

    ta->core_watcher_pid = te_shell_cmd(te_string_value(&cmd), -1,
                                        &ta->core_watcher_in,
                                        NULL, NULL);
    te_string_free(&cmd);

    if (ta->core_watcher_pid < 0)
    {
        ERROR("Failed to start core watcher for agent %s", ta->ta_name);
        return;
    }

    /*
     * Wait until core watcher initializes, so that TA is started only
     * after that.
     */
    for (i = 0; i < max_attempts; i++)
    {
        FILE *f = NULL;

        usleep(sleep_time);

        f = fopen(te_string_value(&cores_log), "r");
        if (f == NULL)
            continue;

        fseek(f, 0, SEEK_END);
        len = ftell(f);
        fclose(f);

        if (len > 0)
            break;
    }

    if (len <= 0)
    {
        ERROR("Failed to wait until core watcher starts for %s",
              ta->ta_name);
    }
}

/*
 * Terminate ta_core_watcher process; try to do it gracefully and
 * wait for its termination to collect any remaining logs.
 */
static void
stop_core_watcher(unix_ta *ta)
{
    bool terminated = false;
    int i;
    static const int time2wait = 5;

    if (ta->core_watcher_in >= 0)
    {
        write(ta->core_watcher_in, "q", 1);
        close(ta->core_watcher_in);
        ta->core_watcher_in = -1;

        for (i = 0; i < TE_SEC2US(time2wait) / RCFUNIX_WAITPID_SLEEP_US; i++)
        {
            pid_t wait_rc;

            wait_rc = waitpid(ta->core_watcher_pid, NULL, WNOHANG);
            if (wait_rc > 0 || (wait_rc < 0 && errno == ECHILD))
            {
                terminated = true;
                break;
            }

            usleep(RCFUNIX_WAITPID_SLEEP_US);
        }
    }

    if (!terminated)
    {
        WARN("Failed to wait until core watcher terminates for agent %s",
             ta->ta_name);
        killpg(getpgid(ta->core_watcher_pid), SIGTERM);
        killpg(getpgid(ta->core_watcher_pid), SIGKILL);
    }

    ta->core_watcher_pid = -1;
}

/**
 * Start the Test Agent. Note that it's not necessary
 * to restart the proxy Test Agents after rebooting of
 * the NUT, which it serves.
 *
 * @param ta_name       Test Agent name
 * @param ta_type       Test Agent type (Test Agent executable is equal
 *                      to ta_type and is located in TE_INSTALL/agents/bin)
 * @param param         Parameters for RCF/ controllers.
 * @param conf          TA-specific configurations list of kvpairs
 * @param handle        location for TA handle
 * @param flags         location for TA flags
 *
 * @return Error code.
 */
static te_errno
rcfunix_start(const char *ta_name, const char *ta_type,
              const rcf_talib_param *param,
              const te_kvpair_h *conf, rcf_talib_handle *handle,
              unsigned int *flags)
{
    static unsigned int seqno = 0;

    te_errno    rc;
    unix_ta    *ta = NULL;
    char        ta_type_dir[RCF_MAX_PATH];
    te_string   cmd = TE_STRING_INIT;
    te_string   cfg_str = TE_STRING_INIT;
    char       *installdir;
    const char *logname;
    char       *tmp;
    const char *shell;
    const char *val;
    char       *ta_list_file;
    const char *ld_preload = NULL;
    bool shell_is_bash = true;

    unsigned int timestamp;

    if (ta_name == NULL || ta_type == NULL ||
        ta_name[0] == '\0' || strlen(ta_name) >= RCF_MAX_NAME ||
        ta_type[0] == '\0' || strlen(ta_type) >= RCF_MAX_NAME ||
        conf == NULL || flags == NULL)
    {
        return TE_EINVAL;
    }
    if ((rc = te_kvpair_to_str(conf, &cfg_str)) != 0)
    {
        te_string_free(&cfg_str);
        return rc;
    }

    RING("Starting TA '%s' type '%s' conf_str '%s'",
         ta_name, ta_type, cfg_str.ptr);

    if ((installdir = getenv("TE_INSTALL")) == NULL)
    {
        VERB("FATAL ERROR: TE_INSTALL is not exported");
        te_string_free(&cfg_str);
        return TE_ENOENT;
    }
    snprintf(ta_type_dir, sizeof(ta_type_dir),
             "%s/agents/%s/", installdir, ta_type);

    ta = *(unix_ta **)(handle);
    if (ta == NULL)
        ta = TE_ALLOC(sizeof(unix_ta));

    ta->ssh_opts = (te_string)TE_STRING_INIT;
    ta->cmd_prefix = (te_string)TE_STRING_INIT;
    ta->start_prefix = (te_string)TE_STRING_INIT;

    strcpy(ta->ta_name, ta_name);
    strcpy(ta->ta_type, ta_type);

    ta_conf_tce(ta, param);

    /* Set default timeouts */
    ta->copy_timeout = RCFUNIX_COPY_TIMEOUT;
    ta->copy_tries = RCFUNIX_COPY_TRIES;
    ta->kill_timeout = RCFUNIX_KILL_TIMEOUT;

    ta->core_watcher_pid = -1;
    ta->core_watcher_in = -1;

    if (strcmp(ta_type + strlen(ta_type) - strlen("ctl"), "ctl") == 0)
        *flags |= TA_PROXY;

    ta->flags = flags;
    logname = getenv("LOGNAME");
    if (logname == NULL)
        logname = "";
    timestamp = (unsigned int)time(NULL);
    if (snprintf(ta->run_dir, sizeof(ta->run_dir), "/tmp/%.*s_%s_%u_%u_%u",
                 rcfunix_ta_type_prefix_len(ta_type), ta_type, logname,
                 (unsigned int)getpid(), timestamp, ++seqno) >=
        (int)sizeof(ta->run_dir))
    {
        ERROR("Failed to compose TA run directory '/tmp/%s_%s_%u_%u_%u' - "
              "provided buffer too small",
              ta_type, logname, (unsigned int)getpid(), timestamp, seqno);
        te_string_free(&cfg_str);
        rcfunix_ta_free(ta);
        return TE_ESMALLBUF;
    }

    VERB("Run directory '%s'", ta->run_dir);

    if (te_str_is_null_or_empty(val = te_kvpairs_get(conf, "host")))
    {
        ta->is_local = true;
        *flags |= TA_LOCAL;
        snprintf(ta->host, sizeof(ta->host), "127.0.0.1");
    }
    else
    {
        ta->is_local = false;
        te_strlcpy(ta->host, val, RCF_MAX_NAME);
    }
    VERB("Test Agent host %s", ta->host);

    val = te_kvpairs_get(conf, "port");
    if (te_str_is_null_or_empty(val) ||
        (strtol(val, &tmp, 0), (tmp == val || *tmp != 0)))
    {
        goto bad_conf;
    }
    te_strlcpy(ta->port, val, RCF_MAX_NAME);

    if (!te_str_is_null_or_empty(val = te_kvpairs_get(conf, "user")))
        snprintf(ta->user, sizeof(ta->user), "%s@", val);

    if (!te_str_is_null_or_empty(val = te_kvpairs_get(conf, "key")))
    {
        snprintf(ta->key, sizeof(ta->key), "-i %s %s", val,
                " -o UserKnownHostsFile=/dev/null "
                "-o StrictHostKeyChecking=no");
    }

    if (!te_str_is_null_or_empty(val = te_kvpairs_get(conf, "ssh_port")))
    {
        ta->ssh_port = strtoul(val, &tmp, 0);
        if (tmp == val || *tmp != 0 || ta->ssh_port > UINT16_MAX)
            goto bad_conf;
    }

    if (!te_str_is_null_or_empty(val = te_kvpairs_get(conf, "ssh_proxy")))
        te_strlcpy(ta->ssh_proxy, val, sizeof(ta->ssh_proxy));

    if (!te_str_is_null_or_empty(val = te_kvpairs_get(conf, "copy_timeout")))
    {
        ta->copy_timeout = strtoul(val, &tmp, 0);
        if (tmp == val || *tmp != 0)
            goto bad_conf;
    }

    if (!te_str_is_null_or_empty(val = te_kvpairs_get(conf, "copy_tries")))
    {
        ta->copy_tries = strtoul(val, &tmp, 0);
        if (tmp == val || *tmp != 0)
            goto bad_conf;
    }

    if (!te_str_is_null_or_empty(val = te_kvpairs_get(conf, "kill_timeout")))
    {
        ta->kill_timeout = strtoul(val, &tmp, 0);
        if (tmp == val || *tmp != 0)
            goto bad_conf;
    }

    if ((val = te_kvpairs_get(conf, "notcopy")) != NULL)
        WARN("The deprecated RCF parameter 'notcopy' is skipped");

    if ((val = te_kvpairs_get(conf, "sudo")) != NULL)
    {
        ta->sudo = true;
    }
    else
    {
        ta->sudo = false;
    }

    if (!te_str_is_null_or_empty(val = te_kvpairs_get(conf, "connect")))
    {
        if (strlen(val) >= sizeof(ta->connect))
        {
            ERROR("Too long value in connect parameter: %s", val);
            goto bad_conf;
        }
        strcpy(ta->connect, val);
    }

    if ((val = te_kvpairs_get(conf, "ext_rcf_listener")) != NULL)
    {
        ta->ext_rcf_listener = true;
    }

    shell = te_kvpairs_get(conf, "shell");

    /*
     * It's assumed that the rest of configuration string should be
     * passed to agent.
     */

    if (ta->is_local)
    {
        te_string_append(&ta->cmd_prefix, "(");
        te_string_append(&ta->start_prefix, "%s", ta->cmd_prefix.ptr);
        ta->cmd_suffix = ")";
    }
    else
    {
        if (ta->ssh_proxy[0] != '\0')
            te_string_append(&ta->ssh_opts,
                             " -o ProxyCommand='%s -W %%h:%%p %s'",
                             RCFUNIX_SSH, ta->ssh_proxy);
        if ((*flags & TA_NO_HKEY_CHK))
            te_string_append(&ta->ssh_opts, " %s", NO_HKEY_CHK);
        if (ta->key[0] != '\0')
            te_string_append(&ta->ssh_opts, " %s", ta->key);
        te_string_append(&ta->ssh_opts, " %s%s", ta->user, ta->host);

        te_string_append(&ta->cmd_prefix, "%s", RCFUNIX_SSH);
        if (ta->ssh_port != 0)
            te_string_append(&ta->cmd_prefix, " -p %u", ta->ssh_port);

        /*
         * If SSH proxy is used, TA start command should forward RCF port
         * since it is assumed that target host is not directly reachable.
         */
        te_string_append(&ta->start_prefix, "%s", ta->cmd_prefix.ptr);
        if (ta->ssh_proxy[0] != '\0')
            te_string_append(&ta->start_prefix, " -L %s:%s:%s",
                             ta->port, rcfunix_connect_to(ta, true), ta->port);

        /* Add common SSH options including host to connect to */
        te_string_append(&ta->cmd_prefix, "%s \"", ta->ssh_opts.ptr);
        te_string_append(&ta->start_prefix, "%s \"", ta->ssh_opts.ptr);

        ta->cmd_suffix = "\"";
    }

    /*
     * DO NOT suppress command output in order to have a chance
     * to see possible problems.
     * DO NOT redirect output to te_tee to see it in logs, since
     * pipeline breaks coping return status.
     */
    if (ta->is_local)
    {
        /*
         * Do mkdir without -p to be sure that the directory does not
         * exists yet and fail otherwise.
         * Use dot at the end of cp source path to copy the directory
         * content including hidden files to destination.
         */
        te_string_append(&cmd, "%smkdir %s && cp -a %s/. %s%s",
                         ta->cmd_prefix.ptr, ta->run_dir, ta_type_dir,
                         ta->run_dir, ta->cmd_suffix);
    }
    else
    {
        char ssh_port_str[10] = "";

        if (ta->ssh_port != 0)
            snprintf(ssh_port_str, sizeof(ssh_port_str), "-P %u", ta->ssh_port);

        /*
         * Preserves modification times, access times, and modes.
         * Disables the progress meter.
         * Be quite, but DO NOT suppress command output in order
         * to have to see possible problems.
         * Do mkdir without -p to be sure that the directory does not
         * exists yet and fail otherwise.
         */
        te_string_append(&cmd,
                         "%smkdir %s%s && echo put %s/. %s | sftp -rpq %s%s",
                         ta->cmd_prefix.ptr, ta->run_dir, ta->cmd_suffix,
                         ta_type_dir, ta->run_dir, ssh_port_str,
                         ta->ssh_opts.ptr);
    }

    RING("CMD to copy: %s", cmd.ptr);
    if (!(*flags & TA_FAKE))
    {
        unsigned int sleep_sec = RCFUNIX_COPY_RETRY_SLEEP_FIRST_SEC;
        unsigned int i;

        for (rc = TE_RC(TE_RCF_UNIX, TE_EFAIL), i = 0; i < ta->copy_tries; i++)
        {
            rc = system_with_timeout(ta->copy_timeout, NULL, "%s", cmd.ptr);
            if (rc == 0)
                break;
            te_sleep(sleep_sec);

            sleep_sec = MIN(RCFUNIX_COPY_RETRY_SLEEP_MAX_SEC, sleep_sec * 2);
        }
        if (rc != 0)
        {
            ERROR("Failed to copy TA images/data %s to the %s:/tmp: %r",
                  ta_type, ta->host, rc);
            ERROR("Failed cmd: %s", cmd.ptr);
            te_string_free(&cfg_str);
            te_string_free(&cmd);
            rcfunix_ta_free(ta);
            return rc;
        }
    }

    /*
     * Detect shell name for a non-local TA
     */
    if (!ta->is_local)
    {
        /* Expected string is '/bin/shell_name' and 32 bytes is enough */
        te_string cmd_stdout = TE_STRING_INIT_STATIC(32);

        te_string_reset(&cmd);
        /*
         * We need two backslashes here: the first is C escape sequence,
         * the second is to avoid processing variable on the engine side.
         */
        te_string_append(&cmd, "%secho -n \\$SHELL%s",
                         ta->start_prefix.ptr, ta->cmd_suffix);

        RING("Command to detect shell name: %s", cmd.ptr);
        if (!(*flags & TA_FAKE))
        {
            /*
             * Limit the command execution time to 'copy_timeout' value: we
             * need to make sure that the command will not be executed forever.
             * To do this, it makes no sense to introduce an additional
             * configuration parameter for TA.
             */
            rc = system_with_timeout(ta->copy_timeout, &cmd_stdout,
                                     "%s", cmd.ptr);
            if (rc != 0)
            {
                ERROR("Failed to detect shell name: %r", rc);
                te_string_free(&cfg_str);
                te_string_free(&cmd);
                rcfunix_ta_free(ta);
                return rc;
            }

            RING("Shell is: %s", cmd_stdout.ptr);
            if (strcmp(cmd_stdout.ptr, "/bin/bash") != 0)
                shell_is_bash = false;
        }
    }

    if ((rc = ta_clean_tce(ta, false)) != 0)
    {
        te_string_free(&cfg_str);
        te_string_free(&cmd);
        rcfunix_ta_free(ta);
        return rc;
    }

    /* Clean up command string */
    te_string_reset(&cmd);

    te_string_append(&cmd, "%s", ta->start_prefix.ptr);
    te_string_append(&cmd, "%s", rcfunix_ta_sudo(ta));

    /*
     * Run non-local TA in /bin/bash if it is needed
     */
    if (!ta->is_local && !shell_is_bash)
        te_string_append(&cmd, "/bin/bash -c '");

    /*
     * Add directory with agent to the PATH
     */
    te_string_append(&cmd, "PATH=%s:%s${PATH} ",
                     ta->run_dir, ta->is_local ? "" : "\\");

    /*
     * Add agent working directory to the LD_LIBRARY_PATH
     */
    te_string_append(&cmd,
        "LD_LIBRARY_PATH=%s${LD_LIBRARY_PATH}%s${LD_LIBRARY_PATH:+:}%s ",
        ta->is_local ? "" : "\\", ta->is_local ? "" : "\\", ta->run_dir);

    /*
     * Update LD_PRELOAD variable, existing LD_PRELOAD variable
     * will be overwritten because:
     * - in many cases 2 LD_PRELOADs do not work nicely together;
     * - we do not know any conditions when LD_PRELOAD is non-empty initially.
     */
    ld_preload = te_kvpairs_get(conf, "ld_preload");
    if (!te_str_is_null_or_empty(ld_preload))
        te_string_append(&cmd, "LD_PRELOAD=%s ", ld_preload);

    if (ta->ext_rcf_listener)
        te_string_append(&cmd, "%s/ta_rcf_listener %s ", ta->run_dir, ta->port);

    if ((shell != NULL) && (strlen(shell) > 0))
    {
        VERB("Using '%s' as shell for TA '%s'", shell, ta->ta_name);
        te_string_append(&cmd, "%s ", shell);
    }

    /*
     * Test Agent is always running in background, therefore it's
     * necessary to redirect its stdout and stderr to a file.
     */
    te_string_append(&cmd, "%s/ta %s %s %s",
                     ta->run_dir, ta->ta_name, ta->port,
                     (cfg_str.ptr == NULL) ? "" : cfg_str.ptr);

    /*
     * Add the final single quote if /bin/bash is used
     */
    if (!ta->is_local && !shell_is_bash)
        te_string_append(&cmd, "'");

    te_string_append(&cmd, "%s 2>&1 | te_tee %s %s 10 >ta.%s ",
                     ta->cmd_suffix, TE_LGR_ENTITY, ta->ta_name, ta->ta_name);

    if (rc != 0)
    {
        ERROR("Failed to compose TA start command: %r\n%s", rc, cmd.ptr);
        te_string_free(&cfg_str);
        te_string_free(&cmd);
        rcfunix_ta_free(ta);
        return rc;
    }

    val = te_kvpairs_get(conf, "core_watcher");
    if (!te_str_is_null_or_empty(val))
    {
        if (strcasecmp(val, "yes") == 0 || strcasecmp(val, "ta_dir") == 0)
        {
            start_core_watcher(ta, te_kvpairs_get(conf, "core_pattern"),
                               false);
        }
        else if (strcasecmp(val, "all") == 0)
        {
            start_core_watcher(ta, te_kvpairs_get(conf, "core_pattern"), true);
        }
    }

    RING("Command to start TA: %s", cmd.ptr);
    if (!(*flags & TA_FAKE) &&
        ((ta->start_pid =
          te_shell_cmd(cmd.ptr, -1, TE_EXEC_CHILD_DEV_NULL_FD,
                       NULL, NULL)) <= 0))
    {
        rc = TE_OS_RC(TE_RCF_UNIX, errno);
        ERROR("Failed to start TA %s: %r", ta_name, rc);
        ERROR("Failed cmd: %s", cmd.ptr);
        te_string_free(&cfg_str);
        te_string_free(&cmd);
        rcfunix_ta_free(ta);
        return rc;
    }

    te_string_free(&cfg_str);
    te_string_free(&cmd);

    *handle = (rcf_talib_handle)ta;

    if ((ta_list_file = getenv("TE_TA_LIST_FILE")) != NULL)
    {
        FILE *f = fopen(ta_list_file, "a");
        if (f != NULL)
        {
            fprintf(f, "%s\t\t%s\t\t%s\t\t%s",
                    ta->ta_name, ta->host, ta->ta_type, ta->run_dir);
            fclose(f);
        }
        else
            ERROR("Failed to open '%s' for writing", ta_list_file);
    }

    return 0;

bad_conf:
    RING("Bad configuration for TA '%s'", ta_name);
    te_string_free(&cfg_str);
    rcfunix_ta_free(ta);
    return TE_RC(TE_RCF_UNIX, TE_EINVAL);
}

/**
 * Kill all processes related to TA on the station where it is run.
 * Reboot station which TA is running on (if it's allowed).
 * Handle should not be freed.
 *
 * @param handle        TA handle location, may already contain memory
 *                      pointer in the case of TA restart
 * @param parms         library-specific parameters
 *
 * @return Error code.
 */
static te_errno
rcfunix_finish(rcf_talib_handle handle, const char *parms)
{
    unix_ta    *ta = (unix_ta *)handle;
    te_string   cmd = TE_STRING_INIT;
    te_errno    rc;

    (void)parms;

    if (ta == NULL)
        return TE_EINVAL;

    RING("Finish method is called for TA %s", ta->ta_name);

    ta_save_tce(ta);

    if (*(ta->flags) & TA_FAKE)
    {
        rc = 0;
        goto done;
    }

    if ((ta->pid > 0) &&
        ((*(ta->flags) & TA_DEAD) ||
         strcmp_start("solaris2", ta->ta_type) == 0))
    {
        /* Kill TA itself */
        if (ta->is_local)
        {
            kill(ta->pid, SIGTERM);
            kill(ta->pid, SIGKILL);
        }
        else
        {
            rc = system_with_timeout(ta->kill_timeout, NULL,
                     "%s%skill %d%s " RCFUNIX_REDIRECT,
                     ta->cmd_prefix.ptr, rcfunix_ta_sudo(ta), ta->pid,
                     ta->cmd_suffix);
            if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
                goto done;

            rc = system_with_timeout(ta->kill_timeout, NULL,
                     "%s%skill -9 %d%s " RCFUNIX_REDIRECT,
                     ta->cmd_prefix.ptr, rcfunix_ta_sudo(ta), ta->pid,
                     ta->cmd_suffix);
            if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
                goto done;
        }

        rc = system_with_timeout(ta->kill_timeout, NULL,
                 "%s%skillall %s/ta%s " RCFUNIX_REDIRECT,
                 ta->cmd_prefix.ptr, rcfunix_ta_sudo(ta), ta->run_dir,
                 ta->cmd_suffix);
        if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
            goto done;

        rc = system_with_timeout(ta->kill_timeout, NULL,
                 "%s%skillall -9 %s/ta%s " RCFUNIX_REDIRECT,
                 ta->cmd_prefix.ptr, rcfunix_ta_sudo(ta),
                 ta->run_dir, ta->cmd_suffix);
        if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
            goto done;
    }

    ta_clean_tce(ta, true);

    if (ta->start_pid > 0)
    {
        killpg(getpgid(ta->start_pid), SIGTERM);
        killpg(getpgid(ta->start_pid), SIGKILL);
    }

    if (ta->core_watcher_pid > 0)
        stop_core_watcher(ta);

    te_string_append(&cmd, "%srm -rf %s%s",
                     ta->cmd_prefix.ptr, ta->run_dir, ta->cmd_suffix);

    /* we want to be careful with what we remove */
    if (cmd.len < strlen("rm -rf /tmp/"))
    {
        rc = TE_RC(TE_RCF_UNIX, TE_ENOMEM);
        goto done;
    }

    RING("CMD to remove: %s", cmd.ptr);
    /*
     * Let's use copy_timeout here, it's greater than kill_timeout and
     * with kill_timeout 'rm -fr' fails rarely with timeout, despite it
     * still working not hanging.
     */
    rc = system_with_timeout(ta->copy_timeout, NULL, "%s", cmd.ptr);
    if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
        goto done;

done:
    te_string_free(&cmd);
    rcfunix_ta_free(ta);
    return rc;
}

/**
 * Close all interactions with TA.
 *
 * @param handle        TA handle
 * @param select_set    FD_SET to be updated with the TA connection file
 *                      descriptor (for Test Agents supporting listening
 *                      mode) (IN/OUT)
 *
 * @return Error code.
 */
static te_errno
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
 * @return Error code.
 */
static te_errno
rcfunix_connect(rcf_talib_handle handle, fd_set *select_set,
                struct timeval *select_tm)
{
    unix_ta    *ta = (unix_ta *)handle;
    te_errno    rc;
    char        buf[16];
    char       *tmp;
    size_t      len = 16;
    const char *host;
    int         tries = 3;
    int         sleep_sec = 1;

    char                *env_retry_max;
    char                *endptr;

    char                *ta_list_fn;
    FILE                *ta_list_f = NULL;

    (void)select_tm;

#define TA_LIST_F_ERROR \
    do {                                          \
        if (ta_list_f != NULL)                    \
        {                                         \
            fprintf(ta_list_f, "\t\t<ERROR>\n");  \
            fclose(ta_list_f);                    \
            ta_list_f = NULL;                     \
        }                                         \
    } while (0)

    if ((ta_list_fn = getenv("TE_TA_LIST_FILE")) != NULL)
    {
        ta_list_f = fopen(ta_list_fn, "a");
        if (ta_list_f == NULL)
            ERROR("Failed to open '%s' for writing", ta_list_fn);
    }

    env_retry_max = getenv("RCF_TA_MAX_CONN_ATTEMPTS");
    if (env_retry_max != NULL)
    {
        rc = strtol(env_retry_max, &endptr, 10);
        if (!(endptr == NULL || *endptr != '\0'))
            tries = rc;
    }

    host = rcfunix_connect_to(ta, false);

    tmp = strchr(host, '@');
    if (tmp != NULL)
        host = tmp + 1;

    VERB("Connecting to TA '%s'", ta->ta_name);

    do {
        rc = rcf_net_engine_connect(host, ta->port, &ta->conn, select_set);
        if (rc == 0)
        {
            rc = rcf_net_engine_receive(ta->conn, buf, &len, &tmp);
            if (rc == 0)
                break;

            (void)rcf_net_engine_close(&ta->conn, select_set);
            if (rc != TE_OS_RC(TE_COMM, EPIPE))
            {
                ERROR("Cannot read TA PID from the TA %s (error %r)",
                      ta->ta_name, rc);
                break;
            }
        }
        WARN("Connecting to TA %s %s:%s failed (%r) - "
             "connect again after delay\n",
             ta->ta_name, host, ta->port, rc);
        te_sleep(sleep_sec);

        if (sleep_sec < RCFUNIX_RECONNECT_SLEEP_MAX)
        {
            sleep_sec *= 2;
            if (sleep_sec > RCFUNIX_RECONNECT_SLEEP_MAX)
                sleep_sec = RCFUNIX_RECONNECT_SLEEP_MAX;
        }
    } while (rc != 0 && --tries > 0);
    if (rc != 0)
    {
        TA_LIST_F_ERROR;
        return rc;
    }

    if (strncmp(buf, "PID ", 4) != 0 ||
        (ta->pid = strtol(buf + 4, &tmp, 10), *tmp != 0))
    {
        ta->pid = 0;
        TA_LIST_F_ERROR;
        return TE_RC(TE_RCF, TE_EINVAL);
    }

    INFO("PID of TA %s is %d", ta->ta_name, ta->pid);
    if (ta_list_f != NULL)
    {
        fprintf(ta_list_f, "\t\t%lu\n", (long unsigned int)ta->pid);
        fclose(ta_list_f);
        ta_list_f = NULL;
    }

    return 0;
}

/**
 * Determine a TA command is the reboot command.
 *
 * @param cmd The command.
 *
 * @return @c true on the reboot command or @c false otherwise.
 */
static bool
cmd_is_reboot(const char *cmd)
{
    int pos = 0;

    sscanf(cmd, "SID %*d " TE_PROTO_REBOOT "%n", &pos);

    return pos > 0 && cmd[pos] == '\0';
}

/**
 * Transmit data to the Test Agent.
 *
 * @param handle        TA handle
 * @param data          data to be transmitted
 * @param len           data length
 *
 * @return Error code.
 */
static te_errno
rcfunix_transmit(rcf_talib_handle handle, const void *data, size_t len)
{
    unix_ta *ta = (unix_ta *) handle;

    /*
     * There are two points to save the TCE information:
     * - An RCF/ controller is destroyed on the finish operation.
     * - A TA agent is requested to reboot the TA host when no soft reboot is
     *   allowed and so that no finish operation has been called.
     * In the former case save the TCE information before the reboot command is
     * transmitted.
     */
    if (ta->tce_type != NULL && cmd_is_reboot(data))
        ta_save_tce(ta);

    return rcf_net_engine_transmit(((unix_ta *)handle)->conn, data, len);
}

/**
 * Check pending data on the Test Agent connection.
 *
 * @param handle        TA handle
 *
 * @return @c true, if data are pending; @c false otherwise
 */
static bool
rcfunix_is_ready(rcf_talib_handle handle)
{
    return (handle == NULL) ? false :
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
 * @return Error code.
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
static te_errno
rcfunix_receive(rcf_talib_handle handle, char *buf, size_t *len, char **pba)
{
    return rcf_net_engine_receive(((unix_ta *)handle)->conn, buf, len, pba);
}

RCF_TALIB_METHODS_DEFINE(rcfunix);
