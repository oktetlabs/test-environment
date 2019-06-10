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
 * [:@attr_name{copy_timeout}=@attr_val{<timeout>}]
 * [:@attr_name{kill_timeout}=@attr_val{<timeout>}][:@attr_val{notcopy}]
 * [:@attr_val{sudo|su}][:@attr_val{<shell>}][:@attr_val{<parameters>}]
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
 * - @attr_name{copy_timeout} - specifies the maximum time duration
 *   (in seconds) that is allowed for image copy operation. If image copy
 *   takes more than this timeout, Test Agent start-up procedure fails;
 * - @attr_name{kill_timeout} - specifies the maximum time duration
 *   (in seconds) that is allowed for Test Agent termination procedure;
 * - @attr_val{notcopy} may be used to create symbolic link instead copying
 *   of the image;
 * - @attr_val{sudo|su} - specify this option when we need to run agent under
 *   @prog{sudo|su} (with root privileges). This can be necessary if Test Agent
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
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
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

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_shell_cmd.h"
#include "te_sleep.h"
#include "te_string.h"
#include "rcf_api.h"
#include "rcf_methods.h"

#include "comm_net_engine.h"

#include "logger_api.h"

/*
 * Configuration string for UNIX TA should have format:
 *
 * [[user@]<IP address or hostname>]:<port>
 *     [:key=<ssh private key file>][:ssh_port=<port>][:copy_timeout=<timeout>]
 *     [:kill_timeout=<timeout>][:notcopy][:sudo|su][:<shell>][:parameters]
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
#define NO_HKEY_CHK         "-o StrictHostKeyChecking=no"
#define RCFUNIX_REDIRECT    ">/dev/null 2>&1"

#define RCFUNIX_KILL_TIMEOUT    15
#define RCFUNIX_COPY_TIMEOUT    30

/** Maximum sleep between reconnect attempts */
#define RCFUNIX_RECONNECT_SLEEP_MAX 5

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
    char    ta_name[RCF_MAX_NAME];  /**< Test agent name */
    char    ta_type[RCF_MAX_NAME];  /**< Test Agent type */
    char    host[RCF_MAX_NAME];     /**< Test Agent host */
    char    connect[RCF_MAX_NAME];  /**< Test Agent address or hostname to
                                         connect */
    char    port[RCF_MAX_NAME];     /**< TCP port */
    char    run_dir[RCF_MAX_PATH];  /**< TA run directory */
    char    key[RCF_MAX_PATH];      /**< Private ssh key file */
    char    user[RCF_MAX_PATH];     /**< User to be used (with @) */

    unsigned int    ssh_port;       /**< 0 or special SSH port to use */
    unsigned int    copy_timeout;   /**< TA image copy timeout */
    unsigned int    kill_timeout;   /**< TA kill timeout */

    te_bool sudo;       /**< Manipulate process using sudo */
    te_bool su;         /**< Run process using "su -c" */
    te_bool notcopy;    /**< Do not copy TA image to remote host */
    te_bool is_local;   /**< TA is started on the local PC */

    char        cmd_prefix[RCFUNIX_SHELL_CMD_MAX];  /**< Command prefix */
    const char *cmd_suffix;                         /**< Command suffix
                                                         before redirection */

    uint32_t        pid;        /**< TA pid */
    unsigned int   *flags;      /**< Flags */
    pid_t           start_pid;  /**< PID of the SSH process which
                                     started the agent */

    struct rcf_net_connection  *conn;   /**< Connection handle */
} unix_ta;

/**
 * Execute the command without forever blocking.
 *
 * @param timeout       timeout in seconds
 * @param fmt           format string of the command to be executed
 *
 * @return Status code.
 * @return TE_ETIMEDOUT    Command timed out
 */
static te_errno
__attribute__((format(printf, 2, 3)))
system_with_timeout(int timeout, const char *fmt, ...)
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

    pid = te_shell_cmd(cmd, -1, NULL, &fd, NULL);
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
 * @return Error code.
 */
static te_errno
rcfunix_start(const char *ta_name, const char *ta_type,
              const char *conf_str, rcf_talib_handle *handle,
              unsigned int *flags)
{
    static unsigned int seqno = 0;

    te_errno    rc;
    unix_ta    *ta;
    char       *token;
    char        ta_type_dir[RCF_MAX_PATH];
    te_string   cmd = TE_STRING_INIT;
    char       *installdir;
    const char *logname;
    char       *tmp;
    char       *conf_str_dup;
    char       *dup;
    char       *shell;
    char       *ta_list_file;

    unsigned int timestamp;

    RING("Starting TA '%s' type '%s' conf_str '%s'",
         ta_name, ta_type, conf_str);

/** Get the next token from configuration string */
#define GET_TOKEN \
    do {                            \
        token = dup;                \
        if (dup != NULL)            \
        {                           \
            tmp = index(dup, ':');  \
            if (tmp == NULL)        \
                dup = NULL;         \
            else                    \
            {                       \
                *tmp = 0;           \
                dup = tmp + 1;      \
            }                       \
        }                           \
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
    snprintf(ta_type_dir, sizeof(ta_type_dir),
             "%s/agents/%s/", installdir, ta_type);

    if ((ta = *(unix_ta **)(handle)) == NULL &&
        (ta = (unix_ta *)calloc(1, sizeof(unix_ta))) == NULL)
    {
        ERROR("Memory allocation failure: %u bytes",
                  sizeof(unix_ta));
        return TE_ENOMEM;
    }

    strcpy(ta->ta_name, ta_name);
    strcpy(ta->ta_type, ta_type);

    /* Set default timeouts */
    ta->copy_timeout = RCFUNIX_COPY_TIMEOUT;
    ta->kill_timeout = RCFUNIX_KILL_TIMEOUT;

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
        return TE_ESMALLBUF;
    }

    VERB("Run directory '%s'", ta->run_dir);

    if ((dup = conf_str_dup = strdup(conf_str)) == NULL)
    {
        ERROR("Failed to duplicate string '%s'", conf_str);
        return TE_ENOMEM;
    }

    GET_TOKEN;
    if (token == NULL)
        goto bad_confstr;
    if (strlen(token) == 0)
    {
        ta->is_local = TRUE;
        *flags |= TA_LOCAL;
        snprintf(ta->host, sizeof(ta->host), "127.0.0.1");
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
    if (token != NULL && strcmp_start("user=", token) == 0)
    {
        char *user = token + strlen("user=");

        if (strlen(user) > 0)
            snprintf(ta->user, sizeof(ta->user), "%s@", user);

        GET_TOKEN;
    }
    if (token != NULL && strcmp_start("key=", token) == 0)
    {
        char *key = token + strlen("key=");

        if (strlen(key) > 0)
            snprintf(ta->key, sizeof(ta->key), "-i %s %s", key,
                    " -o UserKnownHostsFile=/dev/null "
                    "-o StrictHostKeyChecking=no");

        GET_TOKEN;
    }
    if (token != NULL && strcmp_start("ssh_port=", token) == 0)
    {
        char *value = token + strlen("ssh_port=");

        if (strlen(value) > 0)
        {
            ta->ssh_port = strtoul(value, &tmp, 0);
            if (tmp == value || *tmp != 0 || ta->ssh_port > UINT16_MAX)
                goto bad_confstr;
        }
        GET_TOKEN;
    }
    if (token != NULL && strcmp_start("copy_timeout=", token) == 0)
    {
        char *value = token + strlen("copy_timeout=");

        if (strlen(value) > 0)
        {
            ta->copy_timeout = strtoul(value, &tmp, 0);
            if (tmp == value || *tmp != 0)
                goto bad_confstr;
        }

        GET_TOKEN;
    }
    if (token != NULL && strcmp_start("kill_timeout=", token) == 0)
    {
        char *value = token + strlen("kill_timeout=");

        if (strlen(value) > 0)
        {
            ta->kill_timeout = strtoul(value, &tmp, 0);
            if (tmp == value || *tmp != 0)
                goto bad_confstr;
        }

        GET_TOKEN;
    }
    if (token != NULL && strcmp(token, "notcopy") == 0)
    {
        ta->notcopy = TRUE;
        GET_TOKEN;
    }
    else
        ta->notcopy = FALSE;
    if (token != NULL && strcmp(token, "sudo") == 0)
    {
        ta->sudo = TRUE;
        GET_TOKEN;
    }
    else
    {
        ta->sudo = FALSE;
        if (token != NULL && strcmp(token, "su") == 0)
        {
            ta->su = TRUE;
            GET_TOKEN;
        }
        else
            ta->su = FALSE;
    }
    if (token != NULL && strcmp_start("connect=", token) == 0)
    {
        char *value = token + strlen("connect=");

        if (strlen(value) >= sizeof(ta->connect))
        {
            ERROR("Too long value in connect= token: %s", value);
            goto bad_confstr;
        }
        strcpy(ta->connect, value);

        GET_TOKEN;
    }

    shell = token;

    /*
     * It's assumed that the rest of configuration string should be
     * passed to agent.
     */

    if (ta->is_local)
    {
        snprintf(ta->cmd_prefix, sizeof(ta->cmd_prefix), "(");
        ta->cmd_suffix = ")";
    }
    else
    {
        char ssh_port_str[10] = "";

        if (ta->ssh_port != 0)
            snprintf(ssh_port_str, sizeof(ssh_port_str), "-p %u", ta->ssh_port);

        snprintf(ta->cmd_prefix, sizeof(ta->cmd_prefix),
                 RCFUNIX_SSH "%s %s %s %s%s \"",
                 *flags & TA_NO_HKEY_CHK ? NO_HKEY_CHK : "", ta->key,
                 ssh_port_str, ta->user, ta->host);
        ta->cmd_suffix = "\"";
    }

    /*
     * DO NOT suppress command output in order to have a chance
     * to see possible problems.
     */
    if (ta->notcopy)
    {
        rc = te_string_append(&cmd,
                "%sln -s %s %s%s", ta->cmd_prefix,
                ta_type_dir, ta->run_dir, ta->cmd_suffix);
    }
    else if (ta->is_local)
    {
        rc = te_string_append(&cmd,
                "%scp -a %s %s%s", ta->cmd_prefix,
                ta_type_dir, ta->run_dir, ta->cmd_suffix);
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
         */
        rc = te_string_append(&cmd,
                "scp -rBpq %s %s %s %s %s%s:%s",
                ssh_port_str,
                *flags & TA_NO_HKEY_CHK ? NO_HKEY_CHK : "",
                ta->key, ta_type_dir, ta->user, ta->host, ta->run_dir);
    }
    if (rc == 0)
        rc = te_string_append(&cmd, " 2>&1 | te_tee %s %s 10 >ta.%s",
                              TE_LGR_ENTITY, ta->ta_name, ta->ta_name);
    if (rc != 0)
    {
        ERROR("Failed to compose TA copy command: %r\n%s", rc, cmd.ptr);
        te_string_free(&cmd);
        free(conf_str_dup);
        return rc;
    }

    RING("CMD to copy: %s", cmd.ptr);
    if (!(*flags & TA_FAKE) &&
        ((rc = system_with_timeout(ta->copy_timeout, "%s", cmd.ptr)) != 0))
    {
        ERROR("Failed to copy TA images/data %s to the %s:/tmp: %r",
              ta_type, ta->host, rc);
        ERROR("Failed cmd: %s", cmd.ptr);
        te_string_free(&cmd);
        free(conf_str_dup);
        return rc;
    }

    /* Clean up command string */
    te_string_reset(&cmd);

    rc = te_string_append(&cmd, "%s", ta->cmd_prefix);

#if defined RCF_UNIX_SOLARIS
    if (rc == 0)
        rc = te_string_append(&cmd,
                "%s/usr/bin/coreadm -g /tmp/core.%%n-%%p-%%t -e global; ",
                rcfunix_ta_sudo(ta));
#else
    if (rc == 0)
        rc = te_string_append(&cmd,
                "%ssysctl -w kernel.core_pattern=\"core.%%h-%%p-%%t\"; ",
                rcfunix_ta_sudo(ta));
#endif
    if (rc == 0)
        rc = te_string_append(&cmd, "%s",
                                   rcfunix_ta_sudo(ta));
    if (rc == 0 && ta->su)
        rc = te_string_append(&cmd, "su -c ");

    /*
     * Add directory with agent to the PATH
     */
    if (rc == 0)
        rc = te_string_append(&cmd,
                "PATH=%s${PATH}:%s ",
                ta->is_local ? "" : "\\", ta->run_dir);

    if (rc == 0 && (shell != NULL) && (strlen(shell) > 0))
    {
        VERB("Using '%s' as shell for TA '%s'", shell, ta->ta_name);
        rc = te_string_append(&cmd, "%s ", shell);
    }

    /*
     * Test Agent is always running in background, therefore it's
     * necessary to redirect its stdout and stderr to a file.
     */
    if (rc == 0)
        rc = te_string_append(&cmd,
                "%s/ta %s %s %s",
                ta->run_dir, ta->ta_name, ta->port,
                (conf_str == NULL) ? "" : conf_str);

    if (rc == 0 && ta->su)
        rc = te_string_append(&cmd, "'");

#if defined RCF_UNIX_SOLARIS
    if (rc == 0)
        rc = te_string_append(&cmd,
                "; %s/usr/bin/coreadm -g /tmp/core -e global ",
                rcfunix_ta_sudo(ta));
#else
    /* Enquote command in double quotes for non-local agent */
    if (rc == 0)
        rc = te_string_append(&cmd,
                "; %ssysctl -w kernel.core_pattern=\"core\" ",
                rcfunix_ta_sudo(ta));
#endif
    if (rc == 0)
        rc = te_string_append(&cmd,
                "; %smv /tmp/core* /var/tmp ", rcfunix_ta_sudo(ta));
    if (rc == 0)
        rc = te_string_append(&cmd,
                "%s 2>&1 | te_tee %s %s 10 >>ta.%s ",
                ta->cmd_suffix, TE_LGR_ENTITY, ta->ta_name, ta->ta_name);

    free(conf_str_dup);

    if (rc != 0)
    {
        ERROR("Failed to compose TA start command: %r\n%s", rc, cmd.ptr);
        te_string_free(&cmd);
        return rc;
    }

    RING("Command to start TA: %s", cmd.ptr);
    if (!(*flags & TA_FAKE) &&
        ((ta->start_pid =
          te_shell_cmd(cmd.ptr, -1, NULL, NULL, NULL)) <= 0))
    {
        rc = TE_OS_RC(TE_RCF_UNIX, errno);
        ERROR("Failed to start TA %s: %r", ta_name, rc);
        ERROR("Failed cmd: %s", cmd.ptr);
        te_string_free(&cmd);
        return rc;
    }

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

bad_confstr:
    free(conf_str_dup);
    RING("Bad configuration string for TA '%s'", ta_name);
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

    if (*(ta->flags) & TA_FAKE)
        return 0;

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
            rc = system_with_timeout(ta->kill_timeout,
                     "%s%skill %d%s " RCFUNIX_REDIRECT,
                     ta->cmd_prefix, rcfunix_ta_sudo(ta), ta->pid,
                     ta->cmd_suffix);
            if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
                return rc;

            rc = system_with_timeout(ta->kill_timeout,
                     "%s%skill -9 %d%s " RCFUNIX_REDIRECT,
                     ta->cmd_prefix, rcfunix_ta_sudo(ta), ta->pid,
                     ta->cmd_suffix);
            if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
                return rc;
        }

        rc = system_with_timeout(ta->kill_timeout,
                 "%s%skillall %s/ta%s " RCFUNIX_REDIRECT,
                 ta->cmd_prefix, rcfunix_ta_sudo(ta), ta->run_dir,
                 ta->cmd_suffix);
        if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
            return rc;

        rc = system_with_timeout(ta->kill_timeout,
                 "%s%skillall -9 %s/ta%s " RCFUNIX_REDIRECT,
                 ta->cmd_prefix, rcfunix_ta_sudo(ta),
                 ta->run_dir, ta->cmd_suffix);
        if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
            return rc;
    }

    rc = te_string_append(&cmd, "%srm -rf %s%s",
                       ta->cmd_prefix, ta->run_dir, ta->cmd_suffix);
    if (rc != 0)
    {
        ERROR("Failed to make rm command: %r", rc);
        goto done;
    }
    /* we want to be careful with what we remove */
    if (cmd.len < strlen("rm -rf /tmp/"))
    {
        rc = TE_RC(TE_RCF_UNIX, TE_ENOMEM);
        goto done;
    }

    RING("CMD to remove: %s", cmd.ptr);
    rc = system_with_timeout(ta->kill_timeout, "%s", cmd.ptr);
    if (rc == TE_RC(TE_RCF_UNIX, TE_ETIMEDOUT))
        goto done;

    if (ta->start_pid > 0)
    {
        killpg(getpgid(ta->start_pid), SIGTERM);
        killpg(getpgid(ta->start_pid), SIGKILL);
    }

done:
    te_string_free(&cmd);
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
    char       *host;
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

    if (ta->connect[0] != '\0')
        host = ta->connect;
    else
        host = ta->host;

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
        WARN("Connecting to TA failed (%r) - connect again after delay\n", rc);
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
    return rcf_net_engine_transmit(((unix_ta *)handle)->conn, data, len);
}

/**
 * Check pending data on the Test Agent connection.
 *
 * @param handle        TA handle
 *
 * @return TRUE, if data are pending; FALSE otherwise
 */
static te_bool
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

