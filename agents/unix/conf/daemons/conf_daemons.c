/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix daemons configuring implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "conf_daemons_internal.h"

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_shell_cmd.h"
#include "te_sleep.h"

/** Maximum number of attempts to wait a daemon in expected state */
#define TA_UNIX_DAEMON_WAIT_ATTEMPTS    1000
/** Time to wait between checks of the daemon state, in microseconds */
#define TA_UNIX_DAEMON_WAIT_USEC        50000

/* Array of daemons/services names */
static struct {
    char   *config_file;
    char   *backup;
    te_bool changed;
} ds[UNIX_SERVICE_MAX];

/** Auxiliary buffer */
static char buf[2048];


/**
 * Find the first existing file in the list.
 *
 * @param n         Number of entries
 * @param files     Array with file names
 * @param exec      Should the file be executable
 *
 * @return Index of the found file or -1
 */
int
find_file(unsigned int n, const char * const *files, te_bool exec)
{
    unsigned int    i;
    struct stat     st;

    assert(files != NULL);
    for (i = 0; i < n; ++i)
    {
        if (files[i] != NULL &&
            stat(files[i], &st) == 0 &&
            ((st.st_mode & S_IFMT) == S_IFREG ||
             (st.st_mode & S_IFMT) == S_IFLNK) &&
            (exec == !!(st.st_mode & S_IXUSR)))
        {
            return i;
        }
    }

    return -1;
}

/**
 * Get configuration file name for the daemon/service.
 *
 * @param index index returned by the ds_create_backup
 *
 * @return Pathname of the configuration file
 */
const char *
ds_config(int index)
{
    return (index >= UNIX_SERVICE_MAX || index < 0 ||
            ds[index].config_file == NULL) ? "" : ds[index].config_file;
}

/**
 * Look for registered service with specified configuration directory
 * and file name.
 *
 * @param dir   configuration directory name
 * @param name  service name
 *
 * @return index or -1
 */
int
ds_lookup(const char *dir, const char *name)
{
    int dirlen = strlen(dir);
    int i;

    for (i = 0; i < UNIX_SERVICE_MAX; i++)
    {
        if (ds[i].config_file != NULL &&
            strncmp(dir, ds[i].config_file, dirlen) == 0 &&
            strcmp(name, ds[i].config_file + dirlen) == 0)
        {
            return i;
        }
    }

    return -1;
}

/**
 * Get name of the configuration file name backup for the daemon/service.
 *
 * @param index index returned by the ds_create_backup
 *
 * @return Pathname of the configuration file
 */
const char *
ds_backup(int index)
{
    return (index >= UNIX_SERVICE_MAX || index < 0 ||
            ds[index].backup == NULL) ? "" : ds[index].backup;
}

/**
 * Check, if the daemon/service configuration file was changed.
 *
 * @param index index returned by the ds_create_backup
 *
 * @return TRUE, if the file was changed
 */
te_bool
ds_config_changed(int index)
{
    return (index >= UNIX_SERVICE_MAX || index < 0 ||
            ds[index].backup == NULL) ? FALSE : ds[index].changed;
}

/**
 * Notify backup manager that the configuration file was touched.
 *
 * @param index         daemon/service index
 */
void
ds_config_touch(int index)
{
    if (index < UNIX_SERVICE_MAX && index >= 0)
        ds[index].changed = TRUE;
}

/**
 * Create a backup or rename unused backup.
 *
 * @param config        pathname of the configuration file
 * @param backup        patname of the backup without pid postfix
 *                      (updated by the routine)
 *
 * @return Status code
 */
static int
copy_or_rename(const char *config, char *backup)
{
    FILE *f;
    char *s;
    int   rc = 0;
    int   my_pid = getpid();
    pid_t cmd_pid;

    TE_SPRINTF(buf, "ls %s* 2>/dev/null", backup);
    if ((rc = ta_popen_r(buf, &cmd_pid, &f)) < 0)
        return rc;
    s = fgets(buf, sizeof(buf), f);
    if ((rc = ta_pclose_r(cmd_pid, f)) < 0)
        return rc;

    if (s == NULL)
    {
        TE_SPRINTF(buf, "cp %s %s.%d", config, backup, my_pid);
    }
    else
    {
        size_t  len = strlen(buf);
        char   *tmp = strrchr(buf, '.');
        int     pid = (tmp == NULL) ? 0 : atoi(tmp + 1);

        if (buf[len - 1] == '\n')
        {
            buf[len - 1] = '\0';
            --len;
        }

        if (pid == 0)
        {
            ERROR("Backup '%s' of the old version of Unix TA is found",
                  buf);
            return TE_RC(TE_TA_UNIX, TE_EEXIST);
        }

        /* Zero signal just check a possibility to send signal */
        if (kill(pid, 0) == 0)
        {
            ERROR("Backup '%s' of running TA with PID=%d is found - "
                  "corresponding service(s) are not usable",
                  buf, pid);
            return TE_RC(TE_TA_UNIX, TE_EEXIST);
        }
        else
        {
            WARN("Consider backup '%s' of dead TA with PID=%d as ours",
                 buf, pid);
            TE_SPRINTF(buf, "mv %s.%d %s.%d", backup, pid, backup, my_pid);
        }
    }
    if (ta_system(buf) != 0)
    {
        ERROR("Cannot create backup: command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    sprintf(backup + strlen(backup), ".%d", my_pid);
    return 0;
}

/*
 * Creates a copy of service configuration file in TMP directory
 * to restore it after Agent finishes
 *
 * @param dir      configuration directory (with trailing '/')
 * @param name     backup file name
 * @param index    index in the services array
 *
 * @return status code
 */
int
ds_create_backup(const char *dir, const char *name, int *index)
{
    int         i;
    const char *filename;
    FILE       *f;
    int         rc;

    if (name == NULL)
    {
        ERROR("%s(): Invalid parameter", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    for (i = 0; i < UNIX_SERVICE_MAX; i++)
        if (ds[i].backup == NULL)
            break;

    if (i == UNIX_SERVICE_MAX)
    {
        ERROR("Too many services are registered");
        return TE_RC(TE_TA_UNIX, TE_EMFILE);
    }

    filename = strrchr(name, '/');
    if (filename == NULL)
        filename = name;
    else
        filename++;

    TE_SPRINTF(buf, "%s%s", dir ? : "", name);
    ds[i].config_file = strdup(buf);

    if (ds[i].config_file == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((f = fopen(ds[i].config_file, "a")) == NULL)
    {
        WARN("Failed to create backup for %s - no such file",
             ds[i].config_file);
        free(ds[i].config_file);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    fclose(f);

    TE_SPRINTF(buf, TE_TMP_PATH"%s"TE_TMP_BKP_SUFFIX, filename);

    /* Addition memory for pid */
    ds[i].backup = malloc(strlen(buf) + 16);
    if (ds[i].backup == NULL)
    {
        free(ds[i].config_file);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    strcpy(ds[i].backup, buf);

    if ((rc = copy_or_rename(ds[i].config_file, ds[i].backup)) != 0)
    {
        free(ds[i].config_file);
        free(ds[i].backup);
        return rc;
    }

    TE_SPRINTF(buf, "diff %s %s >/dev/null 2>&1",
               ds[i].config_file, ds[i].backup);

    ds[i].changed = (ta_system(buf) != 0);

    if (index != NULL)
        *index = i;

    return 0;
}

/**
 * Restore initial state of the service.
 *
 * @param index         service index
 */
void
ds_restore_backup(int index)
{
    if (index < 0 || index >= UNIX_SERVICE_MAX || ds[index].backup == NULL)
        return;

    if (ds[index].changed)
    {
        TE_SPRINTF(buf, "mv %s %s >/dev/null 2>&1",
                ds[index].backup, ds[index].config_file);
    }
    else
    {
        TE_SPRINTF(buf, "rm %s >/dev/null 2>&1", ds[index].backup);
    }

    if (ta_system(buf) != 0)
        ERROR("Command <%s> failed", buf);

    free(ds[index].backup);
    free(ds[index].config_file);
    ds[index].config_file = NULL;
    ds[index].backup = NULL;

    sync();
}

/**
 * Get current state daemon.
 *
 * @param gid   unused
 * @param oid   daemon name
 * @param value value location
 *
 * @return Status code
 */
te_errno
daemon_get(unsigned int gid, const char *oid, char *value)
{
    const char *daemon_name = get_ds_name(oid);
#if defined __sun__
    int         rc;
#endif

    UNUSED(gid);

    if (daemon_name == NULL)
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

#if defined __linux__
    if (strcmp(daemon_name, "sendmail") == 0)
    {
        if (ta_system("find /var/run/ -name sendmail.pid 2>/dev/null "
                      "| grep pid >/dev/null 2>&1") == 0)
        {
            sprintf(value, "1");
            return 0;
        }
        /* Fall through to daemon_get() */
    }
    else if (strcmp(daemon_name, "postfix") == 0)
    {
        if (ta_system(PS_ALL_COMM " | grep '/usr/lib/postfix/master'"
                      "| grep -v grep >/dev/null") == 0)
        {
            sprintf(value, "1");
            return 0;
        }
        /* Fall through to daemon_get() */
    }

    if (strcmp(daemon_name, "qmail") == 0)
        daemon_name = "qmail-send";

    TE_SPRINTF(buf, "killall -CONT %s >/dev/null 2>&1", daemon_name);
    if (ta_system(buf) == 0)
    {
         strcpy(value, "1");
         return 0;
    }
#elif defined __sun__
    TE_SPRINTF(buf, "/usr/bin/svcs -Ho STATE %s > /dev/null 2>&1",
               daemon_name);
    if ((rc = ta_system(buf)) == 0)
    {
        TE_SPRINTF(buf,
                   "[ \"`/usr/bin/svcs -Ho STATE %s`\" = \"online\" ]",
                   daemon_name);
        strcpy(value, ta_system(buf) == 0 ? "1" : "0");
        return 0;
    }
    else
    {
        ERROR("Command '%s' (getting %s service status) "
              "failed with exit code %d", buf, daemon_name, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
#endif
    strcpy(value, "0");
    return 0;
}

/**
 * Set current state daemon.
 *
 * @param gid   unused
 * @param oid   daemon name
 * @param value value location
 *
 * @return Status code
 */
te_errno
daemon_set(unsigned int gid, const char *oid, const char *value)
{
    const char *daemon_name = get_ds_name(oid);

    char            value0[2];
    int             rc;
    unsigned int    attempt;

    UNUSED(gid);

    if ((rc = daemon_get(gid, oid, value0)) != 0)
        return rc;

    if (strlen(value) != 1 || (*value != '0' && *value != '1'))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (daemon_name == NULL)
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (value0[0] == value[0])
        return 0;

#if defined __linux__
    if (strncmp(daemon_name, "exim", strlen("exim")) == 0)
        sprintf(buf, "/etc/init.d/%s %s >/dev/null", daemon_name,
               *value == '0' ? "stop" : "start");
    else if (strcmp(daemon_name, "named") == 0 &&
             file_exists("/etc/init.d/bind9"))
        /* a hack for Debian */
        sprintf(buf, "/etc/init.d/bind9 %s >/dev/null",
               *value == '0' ? "stop" : "start");
    else
        sprintf(buf, "/etc/init.d/%s %s >/dev/null", daemon_name,
               *value == '0' ? "stop" : "start");

    /*
     * Workaround for '/etc/init.d/...' script (OL bug 864): when no
     * delay performed here 'sendmail' does not completely start for some
     * strange reason although it should: only submission port (587)
     * is opened but smtp one (25) is not; however, if some delay, say,
     * 3 seconds is performed here then both submission (587) and smtp (25)
     * ports are successfully opened by 'sendmail' as they should be;
     * so the workaround for this 'feature' is 3 seconds delay.
     * It is too strange that if the delay is performed only when 'sendmail'
     * starts/stops then the workaround does not work.
     *
     * So, this fix is quite mysterious hack.
     */
    sleep(3); /** Voluntaristic value of 3 seconds */

#elif defined __sun__
    TE_SPRINTF(buf, "/usr/sbin/svcadm %s %s",
               *value == '0' ? "disable -st" : "enable -rst",
               get_ds_name(oid));
#endif
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("Command '%s' failed with exit code %r", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    for (attempt = 0;
         daemon_get(gid, oid, value0),
         (*value0 != *value) && (attempt < TA_UNIX_DAEMON_WAIT_ATTEMPTS);
         ++attempt)
    {
        usleep(TA_UNIX_DAEMON_WAIT_USEC);
    }
    if (*value0 != *value)
    {
        ERROR("After set %s to %s daemon is %srunning",
              oid, value, *value0 == '0' ? "not " : "");
        ta_system(PS_ALL_PID_ARGS);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    return 0;
}

#ifdef WITH_XINETD

/* Get current state of xinetd service */
static te_errno
xinetd_get(unsigned int gid, const char *oid, char *value)
{
#if defined __linux__
    int   index = ds_lookup(XINETD_ETC_DIR, get_ds_name(oid));
    FILE *f;

    if (index < 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((f = fopen(ds_config(index), "r")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    strcpy(value, "1");
    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        char *tmp = strstr(buf, "disable");
        char *comment = strstr(buf, "#");

        if (tmp == NULL || (comment != NULL && comment < tmp))
            continue;

        if (strstr(tmp, "yes") != NULL)
        {
            strcpy(value, "0");
            break;
        }
    }
    fclose(f);
#elif defined __sun__
    char const *const service_name = get_ds_name(oid);
    int               rc;

    TE_SPRINTF(buf, "/usr/bin/svcs -Ho STATE %s > /dev/null 2>&1",
               service_name);
    if ((rc = ta_system(buf)) == 0)
    {
        TE_SPRINTF(buf,
                   "[ \"`/usr/bin/svcs -Ho STATE %s`\" = \"online\" ]",
                   service_name);
        strcpy(value, ta_system(buf) == 0 ? "1" : " 0");
    }
    else
    {
        ERROR("Command '%s' (getting %s service status) "
              "failed with exit code %d", buf, service_name, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
#endif

    UNUSED(gid);

    return 0;
}

/**
 * This global variable is set to value of server field in xinetd.d config
 * and used by xinetd_set. It is cleared automatically after xinetd_set
 * finishing.
 * If it is NULL, server field is not updated.
 */
static char *xinetd_server;

/* On/off xinetd service */
static te_errno
xinetd_set(unsigned int gid, const char *oid, const char *value)
{
    int   rc;
#if defined __linux__
    int   index = ds_lookup(XINETD_ETC_DIR, get_ds_name(oid));
    FILE *f, *g;

    te_bool inside = FALSE;
    char   *server = xinetd_server;

    xinetd_server = NULL;

    if (index < 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strlen(value) != 1 || (*value != '0' && *value != '1'))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((f = fopen(ds_backup(index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        return rc;
    }

    if ((g = fopen(ds_config(index), "w")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for writing", ds_config(index));
        fclose(f);
        return rc;
    }
    ds_config_touch(index);

    ta_system("/etc/init.d/xinetd stop");

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        char *tmp = strstr(buf, "server");

        if (tmp != NULL)
            tmp += strlen("server");

        if (strstr(buf, "disable") == NULL &&
            (server == NULL || tmp == NULL ||
             (!isspace(tmp[0]) && tmp[0] != '=')))
        {
            fwrite(buf, 1, strlen(buf), g);
        }

        if (strstr(buf, "{") && !inside)
        {
            inside = TRUE;
            fprintf(g, "\tdisable = %s\n", *value == '0' ? "yes" : "no");
            if (server != NULL)
                fprintf(g, "\tserver = %s\n", server);
        }
    }
    fclose(f);
    fclose(g);

    /* Commit all changes in config files before restart of the service */
    sync();
    /* I don't know why, but xinetd does not start without this sleep. */
    te_msleep(1);

    if ((rc = ta_system("/etc/init.d/xinetd start")) != 0)
    {
        ERROR("xinetd failed to start with exit code %d", rc);
        return -1;
    }
#elif defined __sun__
    char const *const service_name = get_ds_name(oid);

    TE_SPRINTF(buf, "/usr/sbin/svcadm %s %s",
               *value == '0' ? "disable -st" : "enable -rst", service_name);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("svcadm failed to start %s service with exit code %d",
              service_name, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
#endif

    UNUSED(gid);

    return 0;
}


#if defined(WITH_TODUDP_SERVER) || defined(WITH_ECHO_SERVER)

/**
 * Updates "bind" ("interface") attribute of an xinetd service.
 * Attribute "bind" allows a service to be bound to a specific interface
 * on the machine.
 *
 * @param service  xinetd service name (MUST be the same as the service
 *                 configuration file located in /etc/xinetd.d directory)
 * @param value    IP address of interface to which server to be bound
 *
 * @alg This function reads service configuration file located
 * under XINETD_ETC_DIR directory and copies it into temorary file
 * string by string with one exception - strings that include "bind" and
 * "interface" words are not copied.
 * After all it appends "bind" attribute to the end of the temporay file and
 * replace service configuration file with that file.
 */
static te_errno
ds_xinetd_service_addr_set(const char *service, const char *value)
{
    in_addr_t  addr;
    FILE      *f;
    FILE      *g;
    int        index = ds_lookup(XINETD_ETC_DIR, service);
    int        rc;

    if (inet_aton(value, (struct in_addr *)&addr) == 0)
        return TE_EINVAL;

    if (index < 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strlen(value) != 1 || (*value != '0' && *value != '1'))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((f = fopen(ds_backup(index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        return rc;
    }

    if ((g = fopen(ds_config(index), "w")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for writing", ds_config(index));
        fclose(f);
        return rc;
    }
    ds_config_touch(index);

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        char *comments = strchr(buf, '#');

        if (comments != NULL)
            sprintf(comments, "\n");

        if (strchr(buf, '}') != NULL)
        {
            if (addr != 0xFFFFFFFF)
                fprintf(g, "bind = %s\n}", value);
            else
                fprintf(g, "}");
            break;
        }

        if (strstr(buf, "bind") == NULL &&
            strstr(buf, "interface") == NULL)
        {
            fwrite(buf, 1, strlen(buf), g);
        }
    }
    fclose(f);
    fclose(g);

    /* Commit all changes in config files before restart of the service */
    sync();

    /* Update service configuration file */
    ta_system("/etc/init.d/xinetd restart >/dev/null");

    return 0;
}

/**
 * Gets value of "bind" ("interface") attribute of an xinetd service.
 *
 * @param service  xinetd service name (MUST be the same as the service
 *                 configuration file located in /etc/xinetd.d directory)
 * @param value    Buffer for IP address to return (OUT)
 *
 * @se It is assumed that value buffer is big enough.
 */
static te_errno
ds_xinetd_service_addr_get(const char *service, char *value)
{
    unsigned int  addr;
    FILE         *f;
    int           index = ds_lookup(XINETD_ETC_DIR, service);

    if ((f = fopen(ds_config(index), "r")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        char *comments = strchr(buf, '#');
        char *tmp;

        if (comments != NULL)
            sprintf(comments, "\n");

        if ((tmp = strstr(buf, "bind")) != NULL ||
            (tmp = strstr(buf, "interface")) != NULL)
        {
            char *val = value;

            while (!isdigit(*tmp))
                tmp++;

            while (isdigit(*tmp) || *tmp == '.')
                *val++ = *tmp++;

            *val = 0;

            if (inet_aton(value, (struct in_addr *)&addr) == 0)
                break;

            fclose(f);
            return 0;
        }
    }
    fclose(f);

    sprintf(value, "255.255.255.255");

    return 0;
}

#endif

#endif /* WITH_XINETD */

#ifdef WITH_ECHO_SERVER

/** Get protocol type used by echo server (tcp or udp) */
static te_errno
ds_echoserver_proto_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    return 0;
}

/** Set protocol type used by echo server (tcp or udp) */
static te_errno
ds_echoserver_proto_set(unsigned int gid, const char *oid,
                        const char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    return 0;
}

/** Get IPv4 address echo server is attached to */
static te_errno
ds_echoserver_addr_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_xinetd_service_addr_get("echo", value);
}

/** Attach echo server to specified IPv4 address */
static te_errno
ds_echoserver_addr_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_xinetd_service_addr_set("echo", value);
}

RCF_PCH_CFG_NODE_RW(node_ds_echoserver_addr, "net_addr",
                    NULL, NULL,
                    ds_echoserver_addr_get, ds_echoserver_addr_set);

RCF_PCH_CFG_NODE_RW(node_ds_echoserver_proto, "proto",
                    NULL, &node_ds_echoserver_addr,
                    ds_echoserver_proto_get, ds_echoserver_proto_set);

RCF_PCH_CFG_NODE_RW(node_ds_echoserver, "echoserver",
                    &node_ds_echoserver_proto, NULL,
                    xinetd_get, xinetd_set);

static int echo_index;

te_errno
echoserver_grab(const char *name)
{
    te_errno rc;

    UNUSED(name);

    if ((rc = rcf_pch_add_node("/agent", &node_ds_echoserver)) != 0)
        return rc;

    if ((rc = ds_create_backup(XINETD_ETC_DIR, "echo", &echo_index)) != 0)
    {
        rcf_pch_del_node(&node_ds_echoserver);
        return rc;
    }

    return 0;
}

te_errno
echoserver_release(const char *name)
{
    UNUSED(name);

    if (rcf_pch_del_node(&node_ds_echoserver) != 0)
        return 0;
    ds_restore_backup(echo_index);
    ta_system("/etc/init.d/xinetd restart >/dev/null");
}

#endif /* WITH_ECHO_SERVER */


#ifdef WITH_TODUDP_SERVER

/**
 * Get address which TOD UDP daemon should bind to.
 *
 * @param gid   group identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value value location
 *
 * @return status code
 */
static te_errno
ds_todudpserver_addr_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_xinetd_service_addr_get("daytime-udp", value);
}

/**
 * Get address which TOD UDP daemon should bind to.
 *
 * @param gid   group identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value new address
 *
 * @return status code
 */
static te_errno
ds_todudpserver_addr_set(unsigned int gid, const char *oid,
                         const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_xinetd_service_addr_set("daytime-udp", value);
}

RCF_PCH_CFG_NODE_RW(node_ds_todudpserver_addr, "net_addr",
                    NULL, NULL,
                    ds_todudpserver_addr_get, ds_todudpserver_addr_set);

RCF_PCH_CFG_NODE_RW(node_ds_todudpserver, "todudpserver",
                    &node_ds_todudpserver_addr, NULL,
                    xinetd_get, xinetd_set);

static int todudp_index;

te_errno
todudpserver_grab(const char *name)
{
    te_errno rc;

    UNUSED(name);

    if ((rc = rcf_pch_add_node("/agent", &node_ds_todudpserver)) != 0)
        return rc;

    if ((rc = ds_create_backup(XINETD_ETC_DIR, "daytime-udp",
                               &todudp_index)) != 0)
    {
        rcf_pch_del_node(&node_ds_todudpserver);
        return rc;
    }

    return 0;
}

te_errno
todudpserver_release(const char *name)
{
    UNUSED(name);

    if (rcf_pch_del_node(&node_ds_todudpserver) != 0)
        return 0;
    ds_restore_backup(todudp_index);
    ta_system("/etc/init.d/xinetd restart >/dev/null");
}


#endif /* WITH_TODUDP_SERVER */

#ifdef WITH_TELNET

RCF_PCH_CFG_NODE_RW(node_ds_telnetd, "telnetd",
                    NULL, NULL, xinetd_get, xinetd_set);

#if defined __linux__
static int telnetd_index;
#endif

te_errno
telnetd_grab(const char *name)
{
    te_errno rc;

    UNUSED(name);

    if ((rc = rcf_pch_add_node("/agent", &node_ds_telnetd)) != 0)
        return rc;

#if defined __linux__
    if ((rc = ds_create_backup(XINETD_ETC_DIR, get_ds_name(name),
                               &telnetd_index)) != 0)
    {
        rcf_pch_del_node(&node_ds_telnetd);
        return rc;
    }
#endif

    return 0;
}

te_errno
telnetd_release(const char *name)
{
    UNUSED(name);

    if (rcf_pch_del_node(&node_ds_telnetd) != 0)
        return 0;

#if defined __linux__
    ds_restore_backup(telnetd_index);
    ta_system("/etc/init.d/xinetd restart >/dev/null");
#endif

    return 0;
}

#endif /* WITH_TELNET */

#ifdef WITH_RSH

RCF_PCH_CFG_NODE_RW(node_ds_rshd, "rshd",
                    NULL, NULL, xinetd_get, xinetd_set);

#if defined __linux__
static int rshd_index;
#endif

te_errno
rshd_grab(const char *name)
{
    te_errno rc;

    UNUSED(name);

    if ((rc = rcf_pch_add_node("/agent", &node_ds_rshd)) != 0)
        return rc;

#if defined __linux__
    if ((rc = ds_create_backup(XINETD_ETC_DIR, "rsh",
                               &rshd_index)) != 0)
    {
        rcf_pch_del_node(&node_ds_rshd);
        return rc;
    }
#endif

    return 0;
}

te_errno
rshd_release(const char *name)
{
    UNUSED(name);

    if (rcf_pch_del_node(&node_ds_rshd) != 0)
        return 0;

#if defined __linux__
    ds_restore_backup(rshd_index);
    ta_system("/etc/init.d/xinetd restart >/dev/null");
#endif

    return 0;
}

#endif /* WITH_RSH */


#ifdef WITH_TFTP_SERVER

static int tftp_server_index;

/**
 * Get address which TFTP daemon should bind to.
 *
 * @param gid   group identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value value location
 *
 * @return status code
 */
static te_errno
ds_tftpserver_addr_get(unsigned int gid, const char *oid, char *value)
{
    unsigned int addr;

    FILE *f;

    UNUSED(gid);
    UNUSED(oid);

    if ((f = fopen(ds_config(tftp_server_index), "r")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        char *comments = strchr(buf, '#');

        if (comments != NULL)
            sprintf(comments, "\n");

        if (strstr(buf, "server_args") != NULL)
        {
            char *tmp;
            char *val = value;

            if ((tmp = strstr(buf, "-a")) == NULL)
                break;

            for (tmp += 2; isspace(*tmp); tmp++);

            while (isdigit(*tmp) || *tmp == '.')
                *val++ = *tmp++;

            *val = 0;

            if (inet_aton(value, (struct in_addr *)&addr) == 0)
                break;

            fclose(f);

            return 0;
        }
    }

    sprintf(value, "255.255.255.255");
    fclose(f);

    return 0;
}

/**
 * Change address which TFTP daemon should bind to.
 *
 * @param gid   group identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value new address
 *
 * @return status code
 */
static te_errno
ds_tftpserver_addr_set(unsigned int gid, const char *oid, const char *value)
{
    unsigned int addr;
    te_bool      addr_set = FALSE;

    FILE *f;
    FILE *g;

    UNUSED(gid);
    UNUSED(oid);

    if (inet_aton(value, (struct in_addr *)&addr) == 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((f = fopen("/tmp/tftp.te_backup", "r")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if ((g = fopen("/etc/xinetd.d/tftp", "w")) == NULL)
    {
        fclose(f);
        return 0;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        char *comments = strchr(buf, '#');
        char *tmp;

        if (comments != NULL)
            sprintf(comments, "\n");

        if (!addr_set && (tmp = strchr(buf, '}')) != NULL)
        {
            if (addr != 0xFFFFFFFF)
                fprintf(g, "server_args -a %s\n}", value);
            else
                fprintf(g, "}");
            break;
        }

        if (!addr_set && strstr(buf, "server_args") != NULL)
        {
            char *opt;

            addr_set = TRUE;

            if ((opt = strstr(buf, "-a")) != NULL)
            {
                for (opt += 2; isspace(*opt); opt++);

                for (tmp = opt; isdigit(*tmp) || *tmp == '.'; tmp++);

               fwrite(buf, 1, opt - buf, g);
               if (addr != 0xFFFFFFFF)
                   fwrite(value, 1, strlen(value), g);
               fwrite(tmp, 1, strlen(tmp), g);
               continue;
            }
            else if (addr != 0xFFFFFFFF)
            {
                tmp = strchr(buf, '\n');
                if (tmp == NULL)
                    tmp = buf + strlen(buf);
                sprintf(tmp, " -a %s\n",  value);
            }
        }
        fwrite(buf, 1, strlen(buf), g);
    }
    fclose(f);
    fclose(g);

    /* Commit all changes in config files before restart of the service */
    sync();

    ta_system("/etc/init.d/xinetd restart >/dev/null");

    return 0;
}

/** Get home directory of the TFTP server */
static te_errno
ds_tftpserver_root_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    /* @todo Use the same directory as on daemon startup (option -s) */
    strcpy(value, "/tftpboot");

    return 0;
}

/**
 * Parses buf according to the following format:
 * "Month (3 symbol abbreviation) Day Hour:Min:Sec"
 * and updates 'last_tm' with these values.
 *
 * @param buf      Location of the string with date/time value
 * @param last_tm  Location for the time value
 *
 * @return Status of the operation
 *
 * @se Function uses current year as a year stamp of the message, because
 * message does not contain year value.
 */
static int
ds_log_get_timestamp(const char *buf, struct tm *last_tm)
{
    struct tm tm;
    time_t    cur_time;

    if (strptime(buf, "%b %e %T", last_tm) == NULL)
    {
        assert(0);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /*
     * TFTP logs does not containt year stamp, so that we get current
     * local time, and use extracted year for the log message timstamp.
     */

    /* Get current UTC time */
    if ((cur_time = time(NULL)) == ((time_t)-1))
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (gmtime_r(&cur_time, &tm) == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    /* Use current year for the messsage */
    last_tm->tm_year = tm.tm_year;

    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/**
 * Extracts parameters (file name and timestamp) of the last successful
 * access to TFTP server
 *
 * @param fname     Location for the file name
 * @param time_val  Location for access time value
 *
 * @return  Status of the operation
 */
static int
ds_tftpserver_last_access_params_get(char *fname, time_t *time_val)
{
    struct tm  last_tm;
    struct tm  prev_tm;
    te_bool    again = FALSE;
    FILE      *f;
    int        last_sess_id = -1; /* TFTP last transaction id */
    int        sess_id;
    char      *prev_fname = NULL;

    if (fname != NULL)
        *fname = 0;

    again:

    if ((f = fopen(again ? "./messages.1.txt" :
                           "./messages.txt", "r")) == NULL)
    {
        return 0;
    }

    memset(&last_tm, 0, sizeof(last_tm));

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        char *tmp;

        if ((tmp = strstr(buf, "tftpd[")) != NULL)
        {
            if (sscanf(tmp, "tftpd[%d]:", &sess_id) != 1)
                continue;

            if (last_sess_id == sess_id)
            {
                if (strstr(tmp, "NAK") != NULL)
                {
                    if (fname != NULL)
                    {
                        strcpy(fname, prev_fname);
                               free(prev_fname);
                        prev_fname = NULL;
                    }
                    last_tm = prev_tm;
                }
            }
            else
            {
                /* We've found a log message from a new TFTP transaction */
                if ((tmp = strstr(tmp, "filename")) == NULL)
                    continue;

                   if (fname != NULL)
                {
                    char *val = fname;

                    free(prev_fname);
                    /* make a backup of fname of the previous transaction */
                    if ((prev_fname = strdup(fname)) == NULL)
                    {
                        fclose(f);
                        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
                    }

                    for (tmp += strlen("filename"); isspace(*tmp); tmp++);

                    /* Fill in filename value */
                    while (!isspace(*tmp) && *tmp != 0 && *tmp != '\n')
                        *val++ = *tmp++;

                    *val = 0;
                }
                /*
                 * make a backup of access time of the previous
                 * transaction
                 */
                prev_tm = last_tm;

                /*
                 * Update month, day, hour, min, sec apart from the year,
                 * because it is not provided in the log message.
                 */
                ds_log_get_timestamp(buf, &last_tm);

                last_sess_id = sess_id;
            }

            /* Continue the search to find the last record */
        }
    }

    free(prev_fname);
    fclose(f);

    if (fname != NULL && *fname == '\0' && !again)
    {
        again = TRUE;
        goto again;
    }

    if (time_val != NULL)
        *time_val = mktime(&last_tm);

    return 0;
}

/** Get name of the last file obtained via TFTP */
static te_errno
ds_tftpserver_file_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_tftpserver_last_access_params_get(value, NULL);
}

/** Get last time of the TFTP access */
static te_errno
ds_tftpserver_time_get(unsigned int gid, const char *oid, char *value)
{
    int    rc;
    time_t time_val;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = ds_tftpserver_last_access_params_get(NULL, &time_val)) == 0)
    {
        sprintf(value, "%ld", time_val);
    }
    else
    {
        *value = '\0';
    }
    return rc;
}

RCF_PCH_CFG_NODE_RO(node_ds_tftppserver_root_directory, "root_dir",
                    NULL, NULL,
                    ds_tftpserver_root_get);

RCF_PCH_CFG_NODE_RO(node_ds_tftppserver_last_time, "last_time",
                    NULL, &node_ds_tftppserver_root_directory,
                    ds_tftpserver_time_get);

RCF_PCH_CFG_NODE_RO(node_ds_tftppserver_last_fname, "last_fname",
                    NULL, &node_ds_tftppserver_last_time,
                    ds_tftpserver_file_get);

RCF_PCH_CFG_NODE_RW(node_ds_tftppserver_addr, "net_addr",
                    NULL, &node_ds_tftppserver_last_fname,
                    ds_tftpserver_addr_get, ds_tftpserver_addr_set);

RCF_PCH_CFG_NODE_RW(node_ds_tftpserver, "tftpserver",
                    &node_ds_tftppserver_addr, NULL,
                    xinetd_get, xinetd_set);

te_errno
tftpserver_grab(const char *name)
{
    FILE *f = NULL;
    FILE *g = NULL;
    int   rc;

    UNUSED(name);

    if ((rc = ds_create_backup(XINETD_ETC_DIR,
                               "tftp", &tftp_server_index)) != 0)
    {
        return rc;
    }

    ds_config_touch(tftp_server_index);

    /* Set -v option to tftp */
    OPEN_BACKUP(tftp_server_index, f);
    OPEN_CONFIG(tftp_server_index, g);

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strstr(buf, "server_args") != NULL &&
            strstr(buf, "-vv") == NULL)
        {
            char *tmp = strchr(buf, '\n');
            if (tmp == NULL)
                tmp = buf + strlen(buf);
            sprintf(tmp, " -vv\n");
        }
        fwrite(buf, 1, strlen(buf), g);
    }
    fclose(f);
    fclose(g);

    /* Commit all changes in config files */
    sync();

    if ((rc = rcf_pch_add_node("/agent", &node_ds_tftpserver)) != 0)
    {
        ds_restore_backup(tftp_server_index);
        return rc;
    }

    return 0;
}

te_errno
tftpserver_release(const char *name)
{
    UNUSED(name);

    if (rcf_pch_del_node(&node_ds_tftpserver) != 0)
        return 0;
    ds_restore_backup(tftp_server_index);
    ta_system("/etc/init.d/xinetd restart >/dev/null");

    return 0;
}


#endif /* WITH_TFTP_SERVER */

#ifdef WITH_FTP_SERVER

enum ftp_server_kinds { FTP_VSFTPD, FTP_WUFTPD, FTP_PROFTPD };


static int                     ftp_indices[] = { -1, -1, -1 };
static int                     ftp_xinetd_index = -1;
static te_bool                 ftp_standalone   = TRUE;
static enum  ftp_server_kinds  ftp_server_kind  = FTP_VSFTPD;

#define VSFTPD_CONF  "vsftpd.conf"
#define WUFTPD_CONF  "ftpaccess"
#define PROFTPD_CONF "proftpd.conf"
static const char *const ftp_config_files[] = {VSFTPD_CONF,
                                               WUFTPD_CONF,
                                               PROFTPD_CONF};
static const char *const ftp_config_dirs[]  = {"/etc/vsftpd/",
#if defined __linux__
                                               "/etc/wu-ftpd/",
#elif defined __sun__
                                               "/etc/ftpd/",
#endif
                                               "/etc/proftpd/"};

static const char *const ftpd_conf_names[][2] =
                                            {{"xinetd_vsftpd", "vsftpd"},
                                             {"xinetd_wuftpd", "wuftpd"},
                                             {"xinetd_proftpd", "proftpd"},
                                            };

const char *
get_ftp_daemon_name(void)
{
    static const char *const ftpd_names[] =
        {"vsftpd",
#if defined __linux__
         "wu-ftpd",
#elif defined __sun__
         "svc:/network/ftp:default",
#endif
         "proftpd"};

    return ftp_standalone ? ftpd_names[ftp_server_kind] : "ftp";
}


static te_errno
ds_ftpserver_update_config(void)
{
    FILE *f = NULL;
    FILE *g = NULL;

    /* Enable anonymous upload for ftp */
    ds_config_touch(ftp_indices[ftp_server_kind]);
    OPEN_CONFIG(ftp_indices[ftp_server_kind], g);

    switch (ftp_server_kind)
    {
        case FTP_VSFTPD:
            OPEN_BACKUP(ftp_indices[ftp_server_kind], f);
            while (fgets(buf, sizeof(buf), f) != NULL)
            {
                if (strstr(buf, "anonymous_enable") != NULL ||
                    strstr(buf, "anon_mkdir_write_enable") != NULL ||
                    strstr(buf, "write_enable") != NULL ||
                    strstr(buf, "anon_upload_enable") != NULL ||
                    strstr(buf, "listen") != NULL)
                {
                    continue;
                }
                fwrite(buf, 1, strlen(buf), g);
            }
            fprintf(g, "anonymous_enable=YES\n");
            fprintf(g, "anon_mkdir_write_enable=YES\n");
            fprintf(g, "write_enable=YES\n");
            fprintf(g, "anon_upload_enable=YES\n");
            fprintf(g, "listen=%s\n", ftp_standalone ? "YES" : "NO");
            fclose(f);
            break;

        case FTP_WUFTPD:
            fputs("passwd-check none\n"
                  "class all real,guest,anonymous *\n"
                  "overwrite yes anonymous\n"
                  "upload * * yes * * 0666 dirs\n",
                  g);

            break;

        case FTP_PROFTPD:
        {
            te_bool inside_anonymous = FALSE;

            OPEN_BACKUP(ftp_indices[ftp_server_kind], f);
            while (fgets(buf, sizeof(buf), f) != NULL)
            {
                if (inside_anonymous)
                {
                    if (strstr(buf, "</Anonymous>") != NULL)
                        inside_anonymous = FALSE;
                }
                else if (strstr(buf, "<Anonymous"))
                {
                    inside_anonymous = TRUE;
                }
                else if (strstr(buf, "ServerType") == NULL &&
                         strstr(buf, "AllowOverwrite") == NULL)
                {
                    fputs(buf, g);
                }

            }
            fprintf(g, "\nServerType %s\n",
                    ftp_standalone ? "standalone" : "inetd");
            fputs("AllowOverwrite on\n"
                  "<Anonymous ~ftp>\n"
                  "\tUser ftp\n"
                  "\tGroup nogroup\n"
                  "\tUserAlias anonymous ftp\n"
                  "\tDirFakeUser on ftp\n"
                  "\tDirFakeGroup on nogroup\n"
                  "\tRequireValidShell off\n"
                  "\t<Directory *>\n"
                  "\t\t<Limit WRITE>\n"
                  "\t\t\tDenyAll\n"
                  "\t\t</Limit>\n"
                  "\t</Directory>\n"
                  "\t<Directory pub>\n"
                  "\t\t<Limit STOR WRITE READ>\n"
                  "\t\t\tAllowAll\n"
                  "\t\t</Limit>\n"
                  "\t</Directory>\n"
                  "</Anonymous>\n\n", g);
            fclose(f);
            break;
        }
        default:
            /* should not go here */
            assert(0);
    }

    fclose(g);

    /* Commit all changes in config files */
    sync();

    return 0;
}

#ifdef WITH_XINETD
static te_errno
ds_ftpserver_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(oid);

    if (!ftp_standalone)
        xinetd_server = (ftp_server_kind == FTP_VSFTPD) ?
                        "/usr/sbin/vsftpd" :
                        (ftp_server_kind == FTP_PROFTPD) ?
                        "/usr/sbin/proftpd" : NULL;

    return ftp_standalone ? daemon_set(gid, "ftpserver", value) :
           xinetd_set(gid, "ftpserver", value);
}

static te_errno
ds_ftpserver_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);
    return ftp_standalone ? daemon_get(gid, "ftpserver", value) :
           xinetd_get(gid, "ftpserver", value);
}
#else
#define ds_ftpserver_set daemon_set
#define ds_ftpserver_get daemon_get
#endif

/**
 * Check, if daemon/service is running (enabled).
 *
 * @param daemon    daemon/service name
 *
 * @return TRUE, if daemon is running
 */
static inline te_bool
ftpserver_running()
{
    char enable[2];

    if (ds_ftpserver_get(0, "ftpserver", enable) != 0)
        return 0;

    return enable[0] == '1';
}

static te_errno
ds_ftpserver_server_set(unsigned int gid, const char *oid,
                        const char *value)
{
    te_bool standalone = (strncmp(value, "xinetd_", 7) != 0);
    int     newkind;
    char    tmp[2];

    if (strcmp(value, "vsftpd") != 0 &&
        strcmp(value, "xinetd_vsftpd") != 0 &&
        strcmp(value, "wuftpd") != 0 &&
        strcmp(value, "proftpd") != 0 &&
        strcmp(value, "xinetd_proftpd") != 0)
    {
        ERROR("Invalid server name: %s", value);
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }


    if (!standalone && ftp_xinetd_index < 0)
    {
#ifdef WITH_XINETD
        ERROR("/etc/xinetd.d/ftp not found");
#else
        ERROR("TA compiled without xinetd support")
#endif
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    ds_ftpserver_get(gid, oid, tmp);
    if (tmp[0] != '0')
    {
        ERROR("Cannot change FTP server type when it's running");
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    newkind = (strstr(value, "vsftpd") != NULL ? FTP_VSFTPD :
               (strstr(value, "wuftpd") != NULL ? FTP_WUFTPD :
                FTP_PROFTPD));

    if (ftp_indices[newkind] < 0)
    {
        ERROR("This server is not installed");
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    ftp_standalone  = standalone;
    ftp_server_kind = newkind;

    ds_ftpserver_update_config();
    UNUSED(gid);
    UNUSED(oid);
    return 0;
}

static te_errno
ds_ftpserver_server_get(unsigned int gid, const char *oid, char *value)
{
    strcpy(value, ftpd_conf_names[ftp_server_kind][ftp_standalone]);
    UNUSED(gid);
    UNUSED(oid);
    return 0;
}


static te_bool
ftp_create_backup(enum ftp_server_kinds kind)
{
    const char *dir = NULL;
    static char tmp[PATH_MAX + 1];

    strcpy(tmp, ftp_config_dirs[kind]);
    strcat(tmp, ftp_config_files[kind]);

    if (file_exists(tmp))
    {
        dir = ftp_config_dirs[kind];
    }
    else
    {
        strcpy(tmp, "/etc/");
        strcat(tmp, ftp_config_files[kind]);
        if (!file_exists(tmp))
            return FALSE;
        dir = "/etc/";
    }

    if (ds_create_backup(dir, ftp_config_files[kind],
                         &ftp_indices[kind]) != 0)
        return FALSE;

    ftp_server_kind = kind;

    return TRUE;
}

RCF_PCH_CFG_NODE_RW(node_ds_ftpserver_server, "server",
                    NULL, NULL,
                    ds_ftpserver_server_get,
                    ds_ftpserver_server_set);

RCF_PCH_CFG_NODE_RW(node_ds_ftpserver, "ftpserver",
                    &node_ds_ftpserver_server, NULL,
                    ds_ftpserver_get, ds_ftpserver_set);

te_errno
ftpserver_grab(const char *name)
{
    te_errno rc;
    te_bool  ftp_register;

    UNUSED(name);

    ftp_register = ftp_create_backup(FTP_PROFTPD);
    ftp_register |= ftp_create_backup(FTP_WUFTPD);
    ftp_register |= ftp_create_backup(FTP_VSFTPD);

#ifdef WITH_XINETD
    if (file_exists(XINETD_ETC_DIR "ftp"))
    {
        ftp_register |= ds_create_backup(XINETD_ETC_DIR, "ftp",
                                         &ftp_xinetd_index);
    }
#endif

    if (!ftp_register)
    {
        ERROR("No FTP servers are discovered");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if ((rc = rcf_pch_add_node("/agent", &node_ds_ftpserver)) != 0)
    {
        return rc;
    }

    if ((rc = ds_ftpserver_update_config()) != 0)
    {
        ftpserver_release(NULL);
        return rc;
    }

    if (ta_system("mkdir -p /var/ftp/pub") != 0)
    {
        ERROR("Cannot create /var/ftp/pub");
        ftpserver_release(NULL);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    if (ta_system("chmod o+w /var/ftp/pub") !=0)
    {
        ERROR("Cannot chmod /var/ftp/pub");
        ftpserver_release(NULL);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    if (ftpserver_running())
    {
        ds_ftpserver_set(0, "ftpserver", "0");
        ds_ftpserver_set(0, "ftpserver", "1");
    }

    return 0;
}

te_errno
ftpserver_release(const char *name)
{
    int i;

    UNUSED(name);

    if (rcf_pch_del_node(&node_ds_ftpserver) != 0)
        return 0;

    /* Restore backups */

    for (i = 0;
         i < (int)(sizeof(ftp_indices) / sizeof(ftp_indices[0]));
         i++)
    {
        if (ftp_indices[i] != -1)
            ds_restore_backup(ftp_indices[i]);
    }

#ifdef WITH_XINETD
    if (ftp_xinetd_index != -1)
    {
        ds_restore_backup(ftp_xinetd_index);
        ta_system("/etc/init.d/xinetd restart >/dev/null");
    }
#endif

    ta_system("chmod o-w /var/ftp/pub 2>/dev/null");
    if (ftpserver_running())
    {
        ds_ftpserver_set(0, "ftpserver", "0");
        ds_ftpserver_set(0, "ftpserver", "1");
    }

    return 0;
}

#endif /* WITH_FTP_SERVER */

#ifdef WITH_SMTP

#define SMTP_EMPTY_SMARTHOST    "0.0.0.0"

/** /etc/hosts backup index */
static int hosts_index;

/** Index of smarthost name: te_tester<index> */
static unsigned int smarthost_name_index = 0;

/** Initial SMTP server */
static char *smtp_initial;

/** Current SMTP server */
static char *smtp_current;

/** Name of daemon corresponding to current SMTP server */
static char *smtp_current_daemon;

/** Current smarthost IP address */
static char *smtp_current_smarthost;

/** "exim" or "exim4" */
static char *exim_name = "exim";

/* Possible kinds of SMTP servers - do not change order */
static char *smtp_servers[] = {
#if defined __linux__
    "exim",
    "sendmail",
    "postfix",
    "qmail"
#elif defined __sun__
    "sendmail"
#endif
};

/**
 * Update /etc/hosts with entry te_tester <IP>.
 *
 * @return status code
 */
static int
update_etc_hosts(char *ip)
{
    FILE *f = NULL;
    FILE *g = NULL;
    int   rc;

    if (strcmp(ip, SMTP_EMPTY_SMARTHOST) == 0)
        return 0;

    if ((f = fopen(ds_backup(hosts_index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for reading", ds_backup(hosts_index));
        return rc;
    }

    if ((g = fopen(ds_config(hosts_index), "w")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for writing", ds_config(hosts_index));
        fclose(f);
        return rc;
    }
    ds_config_touch(hosts_index);

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strstr(buf, "te_tester") == NULL)
            fwrite(buf, 1, strlen(buf), g);
    }
    fprintf(g, "%s te_tester%u\n", ip, smarthost_name_index);
    fclose(f);
    fclose(g);

    /* Commit all changes in config files */
    sync();
    ta_system("/usr/sbin/nscd -i hosts");
    sync();

    /* XXX remove me after debugging SMTP failures! - Sasha */
    ta_system("echo \"/etc/hosts updated:\"; cat /etc/hosts");

    return 0;
}

/*---------------------- sendmail staff ------------------------------*/

/** sendmail configuration location */
#if defined __linux__
#define SENDMAIL_CONF_DIR   "/etc/mail/"
#elif defined __sun__
#define SENDMAIL_CONF_DIR   "/etc/mail/cf/cf/"
#endif

/** Smarthost option format */
#define SENDMAIL_SMARTHOST_OPT_S  "define(`SMART_HOST',`te_tester"
#define SENDMAIL_SMARTHOST_OPT_F  "%u')\n"

#define SENDMAIL_ACCESSDB_FEATURE "FEATURE(`access_db')\n"
#define SENDMAIL_LISTEN_ALL_IFS   "DAEMON_OPTIONS(`Family=inet, " \
                                  "Name=MTA-v4, Port=smtp')\n"
#define SENDMAIL_ACPT_UNRES_DOMN  "FEATURE(`accept_unresolvable_domains')\n"

static int sendmail_index = -1;

/** Check if smarthost option presents in the sendmail configuration file */
static int
sendmail_smarthost_get(te_bool *enable)
{
    FILE *f;
    int   rc;

    if ((f = fopen(ds_config(sendmail_index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for reading",
              ds_config(sendmail_index));
        return rc;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strcmp_start(SENDMAIL_SMARTHOST_OPT_S, buf) == 0)
        {
            fclose(f);
            *enable = TRUE;
            return 0;
        }
    }
    *enable = FALSE;
    fclose(f);
    return 0;
}

/** Enable/disable smarthost option in the sendmail configuration file */
static int
sendmail_smarthost_set(te_bool enable)
{
    FILE *f = NULL;
    FILE *g = NULL;
    int   rc;

    if (sendmail_index < 0)
    {
        ERROR("Cannot find sendmail configuration file");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    ds_config_touch(sendmail_index);
    if ((f = fopen(ds_backup(sendmail_index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for reading", ds_backup(sendmail_index));
        return rc;
    }

    if ((g = fopen(ds_config(sendmail_index), "w")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for writing", ds_config(sendmail_index));
        fclose(f);
        return rc;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        /** Remove old 'smarthost' specification */
        if (strstr(buf, "SMART_HOST") == NULL &&

            /** Remove old listen on interaces specification */
            (strstr(buf, "DAEMON_OPTIONS") == NULL ||
             strstr(buf, "Family=inet") == NULL ||
             strstr(buf, "Port=smtp") == NULL) &&

            /** Remove old 'access_db' specification */
            (strstr(buf, "FEATURE") == NULL ||
             strstr(buf, "access_db") == NULL) &&

            /** Remove smarthost-related specification */
            (strstr(buf, "define") == NULL ||
             strstr(buf, "confFALLBACK_SMARTHOST") == NULL)
           )
            fwrite(buf, 1, strlen(buf), g);
    }
    if (enable != 0)
    {
        /** Provide new 'access_db' specification */
        fprintf(g, SENDMAIL_ACCESSDB_FEATURE);

        /** Provide sendmail to listen on all interfaces (OL Bug 435 fix)
         *  ('Addr=127.0.0.1' specification is removed)
         */
        fprintf(g, SENDMAIL_LISTEN_ALL_IFS);

        /** Accept unresolvable domains option is needed because
         * 'client@tester' is unresolvable
         */
        fprintf(g, SENDMAIL_ACPT_UNRES_DOMN);

        /** Provide new 'smarthost' specification */
        fprintf(g, SENDMAIL_SMARTHOST_OPT_S SENDMAIL_SMARTHOST_OPT_F,
                smarthost_name_index);
    }

    fclose(f);
    fclose(g);

    /* Commit all changes in config files before restart of the service */
    sync();

#if defined __linux__
    if ((rc = ta_system("make -C " SENDMAIL_CONF_DIR)) != 0)
    {
        ERROR("make -C " SENDMAIL_CONF_DIR " failed with code %d", rc);
#elif defined __sun__
    if ((rc = ta_system("cd " SENDMAIL_CONF_DIR " && make")) != 0)
    {
        ERROR("cd " SENDMAIL_CONF_DIR " && make failed with code %d", rc);
#endif
        return -1;
    }

    return 0;
}

#if defined __linux__

/*---------------------- postfix staff ------------------------------*/

/** postfix configuration location */
#define POSTFIX_CONF_DIR   "/etc/postfix/"

/** Smarthost option */
#define POSTFIX_SMARTHOST_OPT_S  "relayhost = te_tester"
#define POSTFIX_SMARTHOST_OPT_F  "%u\n"

static int postfix_index = -1;

/** Check if ost option presents in the postfix configuration file */
static int
postfix_smarthost_get(te_bool *enable)
{
    FILE *f;
    int   rc;

    if ((f = fopen(ds_config(postfix_index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for reading",
              ds_config(postfix_index));
        return rc;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strcmp_start(POSTFIX_SMARTHOST_OPT_S, buf) == 0)
        {
            fclose(f);
            *enable = 1;
            return 0;
        }
    }
    *enable = 0;
    fclose(f);
    return 0;
}

/** Enable/disable smarthost option in the postfix configuration file */
static int
postfix_smarthost_set(te_bool enable)
{
    FILE *f = NULL;
    FILE *g = NULL;
    int   rc;

    if (postfix_index < 0)
    {
        ERROR("Cannot find postfix configuration file");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    ds_config_touch(postfix_index);
    if ((f = fopen(ds_backup(postfix_index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for reading", ds_backup(postfix_index));
        return rc;
    }

    if ((g = fopen(ds_config(postfix_index), "w")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for writing", ds_config(postfix_index));
        fclose(f);
        return rc;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strstr(buf, "relayhost") == NULL &&
            strstr(buf, "relaydomains") == NULL)
        {
            fwrite(buf, 1, strlen(buf), g);
        }
    }
    if (enable != 0)
    {
        fprintf(g, POSTFIX_SMARTHOST_OPT_S POSTFIX_SMARTHOST_OPT_F,
                smarthost_name_index);
        fprintf(g, "relaydomains = $mydomain");
    }
    fclose(f);
    fclose(g);

    /* Commit all changes in config files */
    sync();

    return 0;
}

/*---------------------- exim staff ------------------------------*/

/** exim configuration location */
#define EXIM_CONF_DIR   "/etc/exim/"
#define EXIM4_CONF_DIR   "/etc/exim4/"

/** Smarthost option */
#define EXIM_SMARTHOST_OPT_S  "dc_smarthost='te_tester"
#define EXIM_SMARTHOST_OPT_F  "%u'\n"

static int exim_index = -1;

/** Check if ost option presents in the postfix configuration file */
static int
exim_smarthost_get(te_bool *enable)
{
    FILE *f;
    int   rc;

    if ((f = fopen(ds_config(exim_index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for reading", ds_config(exim_index));
        return rc;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strcmp_start(EXIM_SMARTHOST_OPT_S, buf) == 0)
        {
            fclose(f);
            *enable = 1;
            return 0;
        }
    }
    *enable = 0;
    fclose(f);
    return 0;
}

/** Enable/disable smarthost option in the exim configuration file */
static int
exim_smarthost_set(te_bool enable)
{
    FILE *f = NULL;
    FILE *g = NULL;
    int   rc;

    if (exim_index < 0)
    {
        ERROR("Cannot find exim configuration file");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    ds_config_touch(exim_index);
    if ((f = fopen(ds_backup(exim_index), "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for reading", ds_backup(exim_index));
        return rc;
    }

    if ((g = fopen(ds_config(exim_index), "w")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for writing", ds_config(exim_index));
        fclose(f);
        return rc;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strstr(buf, "dc_smarthost") == NULL)
            fwrite(buf, 1, strlen(buf), g);
    }
    if (enable != 0)
    {
        fprintf(g, EXIM_SMARTHOST_OPT_S EXIM_SMARTHOST_OPT_F,
                smarthost_name_index);
    }
    fclose(f);
    fclose(g);

    /* Commit all changes in config files before restart of the service */
    sync();

    sprintf(buf, "update-%s.conf >/dev/null 2>&1", exim_name);
    ta_system(buf);

    return 0;
}

/*------------------ qmail staff --------------------------*/

/** qmail configuration location */
#define QMAIL_CONF_DIR   "/var/qmail/control/"

static int qmail_index = -1;

/** Check if ost option presents in the postfix configuration file */
static int
qmail_smarthost_get(te_bool *enable)
{
    FILE *f;
    int   rc;

    if ((f = fopen(ds_config(qmail_index), "r")) == NULL)
    {
        rc = errno;
        WARN("Cannot open file %s for reading: %d",
              ds_config(qmail_index), rc);
        return 0;
    }

    *enable = 0;
    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (*buf == ':')
        {
            *enable = 1;
            break;
        }
    }
    fclose(f);
    return 0;
}

/** Enable/disable smarthost option in the qmail configuration file */
static int
qmail_smarthost_set(te_bool enable, const char *relay)
{
    FILE *g = NULL;
    int   rc;

    if (qmail_index < 0)
    {
        ERROR("Cannot find qmail configuration file");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    ds_config_touch(qmail_index);
    if ((g = fopen(ds_config(qmail_index), "w")) == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open file %s for writing", ds_config(postfix_index));
        return rc;
    }

    if (enable != 0)
    {
        fprintf(g, ":[%s]\n", relay);
    }
    fclose(g);

    /* Commit all changes in config files */
    sync();

    return 0;
}
#endif


/*------------------ Common mail staff --------------------------*/

/** Get SMTP smarthost */
static te_errno
ds_smtp_smarthost_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    strcpy(value, SMTP_EMPTY_SMARTHOST);
    if (smtp_current == NULL)
        return 0;

    if (strcmp(smtp_current, "sendmail") == 0)
    {
        te_bool enable = FALSE;
        int     rc;

        if ((rc = sendmail_smarthost_get(&enable)) != 0)
            return rc;

        if (enable)
            strcpy(value, smtp_current_smarthost);
    }
#if defined __linux__
    else if (strcmp(smtp_current, "postfix") == 0)
    {
        te_bool enable = FALSE;
        int     rc;

        if ((rc = postfix_smarthost_get(&enable)) != 0)
            return rc;

        if (enable)
            strcpy(value, smtp_current_smarthost);
    }
    else if (strcmp(smtp_current, "exim") == 0)
    {
        te_bool enable = FALSE;
        int     rc;

        if ((rc = exim_smarthost_get(&enable)) != 0)
            return rc;

        if (enable)
            strcpy(value, smtp_current_smarthost);
    }
    else if (strcmp(smtp_current, "qmail") == 0)
    {
        te_bool enable = FALSE;
        int     rc;

        if ((rc = qmail_smarthost_get(&enable)) != 0)
            return rc;

        if (enable)
            strcpy(value, smtp_current_smarthost);
    }
#endif

    return 0;
}

/** Set SMTP smarthost */
static te_errno
ds_smtp_smarthost_set(unsigned int gid, const char *oid,
                      const char *value)
{
    uint32_t addr;
    char    *new_host = NULL;
    int      rc;

    UNUSED(gid);
    UNUSED(oid);

    if (inet_pton(AF_INET, value, (void *)&addr) <= 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (smtp_current == NULL)
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if ((new_host = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    smarthost_name_index++;

    if ((rc = update_etc_hosts(new_host)) != 0)
    {
         free(new_host);
         return rc;
    }

    if (strcmp(smtp_current, "sendmail") == 0)
    {
        if ((rc = sendmail_smarthost_set(addr != 0)) != 0)
            goto error;
    }
#if defined __linux__
    else if (strcmp(smtp_current, "postfix") == 0)
    {
        if ((rc = postfix_smarthost_set(addr != 0)) != 0)
            goto error;
    }
    else if (strcmp(smtp_current, "exim") == 0)
    {
        if ((rc = exim_smarthost_set(addr != 0)) != 0)
            goto error;
    }
    else if (strcmp(smtp_current, "qmail") == 0)
    {
        if ((rc = qmail_smarthost_set(addr != 0, new_host)) != 0)
            goto error;
    }
#endif
    else
        goto error;

    free(smtp_current_smarthost);
    smtp_current_smarthost = new_host;

    if (daemon_running(smtp_current_daemon))
    {
        daemon_set(gid, smtp_current_daemon, "0");
        daemon_set(gid, smtp_current_daemon, "1");
    }

    return 0;

error:
    update_etc_hosts(smtp_current_smarthost);
    free(new_host);
    return rc;
}

/** Get SMTP server program */
static te_errno
ds_smtp_server_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    if (smtp_current == NULL)
        value[0] = 0;
    else
        strcpy(value, smtp_current);

    return 0;
}

/** Check if SMTP server is enabled */
static te_errno
ds_smtp_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    if (smtp_current == NULL)
    {
        value[0] = 0;
        return 0;
    }

    return daemon_get(gid, smtp_current_daemon, value);
}

/** Set SMTP server program */
static te_errno
ds_smtp_server_set(unsigned int gid, const char *oid, const char *value)
{
    unsigned int i;

    te_errno rc;

    char *smtp_prev = smtp_current;
    char *smtp_prev_daemon = smtp_current_daemon;

    UNUSED(gid);
    UNUSED(oid);

    if (smtp_current != NULL && daemon_running(smtp_current_daemon))
    {
        ERROR("Cannot set smtp to %s: %s is running", oid,
              smtp_current_daemon);
        ta_system(PS_ALL_PID_ARGS);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    if (*value == '\0')
    {
        smtp_current = NULL;
        return 0;
    }

    for (i = 0; i < sizeof(smtp_servers) / sizeof(char *); i++)
    {
        if (strcmp(smtp_servers[i], value) == 0)
        {
            smtp_current = smtp_servers[i];
            if (strcmp(smtp_current, "exim") == 0)
                smtp_current_daemon = exim_name;
            else
                smtp_current_daemon = smtp_current;
            if ((rc  = ds_smtp_smarthost_set(0, NULL,
                                             smtp_current_smarthost)) != 0)
            {
                ERROR("Failed to update smarthost for %s", smtp_current);
                smtp_current = smtp_prev;
                smtp_current_daemon = smtp_prev_daemon;
                return rc;
            }
            return 0;
        }
    }

    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/** Enable/disable SMTP server */
static te_errno
ds_smtp_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(oid);
    if (smtp_current == NULL)
    {
        if (value[0] == '0')
            return 0;
        else if (value[0] == '1')
            return TE_RC(TE_TA_UNIX, TE_EPERM);
        else
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    return daemon_set(gid, smtp_current_daemon, value);
}


/**
 * Flush the current SMTP server's queue, so that
 * all messages be delivered instantly.
 * Intended to be called via RPC.
 */
void
flush_smtp_server_queue(void)
{
    int rc = 0;

    if (smtp_current == NULL)
        ERROR("No SMTP server running");
#if defined __linux__
    else if (strcmp(smtp_current, "postfix") == 0)
    {
        rc = ta_system("/etc/init.d/postfix flush");
    }
    else if (strcmp(smtp_current, "qmail") == 0)
    {
        rc = ta_system("killall -ALRM qmail-send");
    }
#endif
    else if (strcmp(smtp_current, "sendmail") == 0)
    {
        rc = ta_system("sendmail-mta -q");
        if (rc != 0)
            rc = ta_system("sendmail -q");
    }
#if defined __linux__
    else if (strcmp(smtp_current, "exim") == 0)
    {
        char buf[30];
        snprintf(buf, sizeof(buf), "%s -qff", exim_name);
        rc = te_shell_cmd(buf, -1, NULL, NULL, NULL);
        if (rc > 0)
            rc = 0;
        else
            rc = -1;
    }
#endif
    else
    {
        WARN("Flushing is not implemented for %s", smtp_current);
    }
    if (rc != 0)
        ERROR("Flushing failed with code %d", rc);
}

RCF_PCH_CFG_NODE_RW(node_ds_smtp_smarthost, "smarthost",
                    NULL, NULL,
                    ds_smtp_smarthost_get, ds_smtp_smarthost_set);

RCF_PCH_CFG_NODE_RW(node_ds_smtp_server, "server",
                    NULL, &node_ds_smtp_smarthost,
                    ds_smtp_server_get, ds_smtp_server_set);

RCF_PCH_CFG_NODE_RW(node_ds_smtp, "smtp",
                    &node_ds_smtp_server, NULL,
                    ds_smtp_get, ds_smtp_set);

te_errno
smtp_grab(const char *name)
{
    unsigned int i;
    int          rc;

    UNUSED(name);

    if ((rc = rcf_pch_add_node("/agent", &node_ds_smtp)) != 0)
        return rc;

    if ((rc = ds_create_backup("/etc/", "hosts", &hosts_index)) != 0)
    {
        ERROR("SMTP server updates /etc/hosts and cannot be initialized");
        smtp_release(NULL);
        return rc;
    }


    /* in case smtp config file is missing we report
     * that smtp is not installed */
    if ((rc = ds_create_backup(SENDMAIL_CONF_DIR, "sendmail.mc",
                               &sendmail_index)) != 0)
    {
        smtp_release(NULL);
        return rc;
    }

#if defined __linux__
    if (file_exists(EXIM_CONF_DIR "update-exim.conf.conf"))
    {
        if ((rc = ds_create_backup(EXIM_CONF_DIR, "update-exim.conf.conf",
                                   &exim_index)) != 0)
        {
            smtp_release(NULL);
            return rc;
        }
    }
    else if (file_exists(EXIM4_CONF_DIR "update-exim4.conf.conf"))
    {
        exim_name = "exim4";
        if ((rc = ds_create_backup(EXIM4_CONF_DIR,
                                   "update-exim4.conf.conf",
                                   &exim_index)) != 0)
        {
            smtp_release(NULL);
            return rc;
        }
    }

    if (file_exists(POSTFIX_CONF_DIR "main.cf") &&
        (rc = ds_create_backup(POSTFIX_CONF_DIR, "main.cf",
                               &postfix_index)) != 0)
    {
        smtp_release(NULL);
        return rc;
    }

    if (file_exists(QMAIL_CONF_DIR "smtproutes") &&
        (rc = ds_create_backup(QMAIL_CONF_DIR, "smtproutes",
                               &qmail_index)) != 0)
    {
        smtp_release(NULL);
        return rc;
    }
#endif

    if ((smtp_current_smarthost = strdup(SMTP_EMPTY_SMARTHOST)) == NULL)
    {
        smtp_release(NULL);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    for (i = 0; i < sizeof(smtp_servers) / sizeof(char *); i++)
    {
        smtp_current = smtp_servers[i];
        if (strcmp(smtp_current, "exim") == 0)
            smtp_current_daemon = exim_name;
        else
            smtp_current_daemon = smtp_current;
        if (daemon_running(smtp_current_daemon))
        {
            smtp_initial = smtp_current_daemon;
            break;
        }
        smtp_current = NULL;
    }

    return 0;
}

te_errno
smtp_release(const char *name)
{
    UNUSED(name);

    if (rcf_pch_del_node(&node_ds_smtp) != 0)
        return 0;

    /* Restore backups */
    ds_restore_backup(hosts_index);
    ds_restore_backup(sendmail_index);
#if defined __linux__
    ds_restore_backup(exim_index);
    ds_restore_backup(postfix_index);
    ds_restore_backup(qmail_index);
#endif

    if (sendmail_index >= 0 && ds_config_changed(sendmail_index))
    {
        if (file_exists(SENDMAIL_CONF_DIR))
        {
#if defined __linux__
            ta_system("make -C " SENDMAIL_CONF_DIR);
#elif defined __sun__
            ta_system("cd " SENDMAIL_CONF_DIR " && make");
#endif
        }
    }

#if defined __linux__
    if (exim_index >= 0 && ds_config_changed(exim_index))
    {
        sprintf(buf, "update-%s.conf >/dev/null 2>&1", exim_name);
        ta_system(buf);
    }
#endif

    if (smtp_current != NULL)
        daemon_set(0, smtp_current_daemon, "0");

    if (smtp_initial != NULL)
        daemon_set(0, smtp_initial, "1");

    free(smtp_current_smarthost);
    smtp_current_smarthost = NULL;

    return 0;
}

#endif /* WITH_SMTP */

#ifdef WITH_VNCSERVER

/** Read VNC password */
static te_errno
ds_vncpasswd_get(unsigned int gid, const char *oid, char *value)
{
    FILE *f;
    int   rc;
    int   sysrc;

    UNUSED(gid);
    UNUSED(oid);

    if ((f = fopen("/tmp/.vnc/passwd", "r")) == NULL)
    {
        rc = errno;
        ERROR("Failed to open /tmp/.vnc directory");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    memset(value, 0, RCF_MAX_VAL);
    sysrc = fread(value, 1, RCF_MAX_VAL - 1, f);
    if (sysrc < 0)
    {
        ERROR("Failed to read data from file \"%s\"", "/tmp/.vnc/passwd");
        fclose(f);
        return TE_RC(TE_TA_UNIX, errno);
    }
    fclose(f);

    return 0;
}

/**
 * Check if the VNC server with specified display is running.
 *
 * @param number  display number
 *
 * @return TRUE, if the server exists
 */
static te_bool
vncserver_exists(char *number)
{
    sprintf(buf,
            "ls /tmp/.vnc/*.pid 2>/dev/null | grep %s >/dev/null 2>&1",
            number);
    return ta_system(buf) == 0;
}

/**
 * Add a new VNC server with specified display number.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         unused
 * @param number        display number
 *
 * @return error code
 */
static te_errno
ds_vncserver_add(unsigned int gid, const char *oid, const char *value,
                 const char *number)
{
    char    *tmp;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    strtol(number, &tmp, 10);
    if (tmp == number || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (vncserver_exists((char *)number))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    sprintf(buf, "HOME=/tmp vncserver :%s", number);

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    ta_system("cp /tmp/.vnc/.Xauthority /tmp/");

    sprintf(buf, "HOME=/tmp DISPLAY=:%s xhost +", number);

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        sprintf(buf, "HOME=/tmp vncserver -kill :%s",
                number);
        ta_system(buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Stop VNC server with specified display number.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param number        display number
 *
 * @return error code
 */
static te_errno
ds_vncserver_del(unsigned int gid, const char *oid, const char *number)
{
    UNUSED(gid);
    UNUSED(oid);

    if (!vncserver_exists((char *)number))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(buf, "HOME=/tmp vncserver -kill :%s >/dev/null 2>&1", number);

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Return list of VNC servers.
 *
 * @param gid     group identifier (unused)
 * @param oid     full parent object instance identifier (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    location for the list pointer
 *
 * @return error code
 */
static te_errno
ds_vncserver_list(unsigned int gid, const char *oid,
                  const char *sub_id, char **list)
{
    FILE *f;
    char  line[128];
    char *s = buf;
    pid_t cmd_pid;
    int   rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if ((rc = ta_popen_r("ls /tmp/.vnc/*.pid 2>/dev/null",
                         &cmd_pid, &f)) < 0)
        return rc;

    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp;
        int   n;

        if ((tmp  = strstr(line, ":")) == NULL ||
            (n = atoi(tmp + 1)) == 0)
            continue;

        s += sprintf(s, "%u ", n);
    }

    if ((rc = ta_pclose_r(cmd_pid, f)) < 0)
        return rc;

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

RCF_PCH_CFG_NODE_RO(node_ds_vncpasswd, "vncpasswd",
                    NULL, NULL, ds_vncpasswd_get);

RCF_PCH_CFG_NODE_COLLECTION(node_ds_vncserver, "vncserver",
                            NULL, NULL,
                            ds_vncserver_add, ds_vncserver_del,
                            ds_vncserver_list, NULL);

te_errno
vncserver_grab(const char *name)
{
    uint8_t passwd[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
    int     fd;
    int     rc;

    UNUSED(name);

    ta_system("rm -rf /tmp/.vnc");

    rc = ta_system("which vncserver");
    if (!WIFEXITED(rc))
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    else if (WEXITSTATUS(rc) != 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (mkdir("/tmp/.vnc", 0700) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to create /tmp/.vnc directory; errno %d", rc);
        return rc;
    }

    if ((fd = open("/tmp/.vnc/passwd", O_CREAT | O_WRONLY, 0600)) <= 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to create file /tmp/.vnc/passwd; errno %r", rc);
        return rc;
    }

    if (write(fd, passwd, sizeof(passwd)) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("write() failed for the file /tmp/.vnc/passwd; errno %r",
              rc);
        close(fd);
        return rc;
    }

    if (close(fd) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("close() failed for the file /tmp/.vnc/passwd; errno %r", rc);
        return rc;
    }

    if ((rc = rcf_pch_add_node("/agent", &node_ds_vncserver)) != 0 ||
        (rc = rcf_pch_add_node("/agent", &node_ds_vncpasswd)) != 0)
    {
        vncserver_release(NULL);
    }

    return rc;
}

te_errno
vncserver_release(const char *name)
{
    UNUSED(name);

    rcf_pch_del_node(&node_ds_vncpasswd);
    rcf_pch_del_node(&node_ds_vncserver);

    ta_system("rm -rf /tmp/.vnc /tmp/.Xauthority");

    return 0;
}

#endif /* WITH_VNCSERVER */


/*--------------------------- SSH daemon ---------------------------------*/

/**
 * Check if the SSH daemon with specified port is running.
 *
 * @param port  port in string representation
 *
 * @return pid of the daemon or 0
 */
static uint32_t
sshd_exists(char *port)
{
    FILE *f;
    char  line[128];
    int   len = strlen(port);
    pid_t cmd_pid;

    if (ta_popen_r(PS_ALL_PID_ARGS " | grep 'sshd -p' | "
                   "grep -v grep", &cmd_pid, &f) < 0)
        return 0;

    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "sshd");

        tmp = strstr(tmp, "-p") + 2;
        while (*++tmp == ' ');

        if (strncmp(tmp, port, len) == 0 && !isdigit(*(tmp + len)))
        {
            int rc = ta_pclose_r(cmd_pid, f);
            return (rc < 0 ) ? 0 : atoi(line);
        }
    }

    ta_pclose_r(cmd_pid, f);

    return 0;
}

/**
 * Add a new SSH daemon with specified port.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         unused
 * @param addr          SSHD port
 *
 * @return error code
 */
static te_errno
ds_sshd_add(unsigned int gid, const char *oid, const char *value,
            const char *port)
{
    uint32_t pid = sshd_exists((char *)port);
    char    *tmp;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    strtol(port, &tmp, 10);
    if (tmp == port || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (pid != 0)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

#if defined __linux__
    sprintf(buf, "/usr/sbin/sshd -p %s", port);
#elif defined __sun__
    sprintf(buf, "/usr/lib/ssh/sshd -p %s", port);
#endif

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Stop SSHD with specified port.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param addr
 *
 * @return error code
 */
static te_errno
ds_sshd_del(unsigned int gid, const char *oid, const char *port)
{
    uint32_t pid = sshd_exists((char *)port);

    UNUSED(gid);
    UNUSED(oid);

    if (pid == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (kill(pid, SIGTERM) != 0)
    {
        int kill_errno = errno;
        ERROR("Failed to send SIGTERM "
              "to process SSH daemon with PID=%u: %d",
              pid, kill_errno);
        /* Just to make sure */
        kill(pid, SIGKILL);
    }

    return 0;
}

/**
 * Return list of SSH daemons.
 *
 * @param gid           group identifier (unused)
 * @param oid           full parent object instance identifier (unused)
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 *
 * @return error code
 */
static te_errno
ds_sshd_list(unsigned int gid, const char *oid,
             const char *sub_id, char **list)
{
    FILE *f;
    char  line[128];
    char *s = buf;
    pid_t cmd_pid;
    int   rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if ((rc = ta_popen_r(PS_ALL_ARGS " | grep 'sshd -p' | grep -v grep",
                         &cmd_pid, &f)) < 0)
        return rc;

    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "sshd");

        tmp = strstr(tmp, "-p") + 2;
        while (*++tmp == ' ');

        s += sprintf(s, "%u ", atoi(tmp));
    }

    if ((rc = ta_pclose_r(cmd_pid, f)) < 0)
        return rc;

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

RCF_PCH_CFG_NODE_COLLECTION(node_ds_sshd, "sshd",
                            NULL, NULL,
                            ds_sshd_add, ds_sshd_del, ds_sshd_list, NULL);

/*--------------------------- X server ---------------------------------*/

/**
 * Check if the Xvfb daemon with specified display number is running.
 *
 * @param number  display number
 *
 * @return pid of the daemon or 0
 */
static uint32_t
xvfb_exists(char *number)
{
    FILE *f;
    char  line[1024];
    int   len = strlen(number);
    pid_t cmd_pid;

    if (ta_popen_r(PS_ALL_PID_ARGS " | grep -w 'Xvfb' | grep -v grep",
                   &cmd_pid, &f) < 0)
        return 0;

    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "Xvfb");

        if (tmp == NULL)
        {
            ERROR("xvfb_exists: ps returned %s", line);
            break;
        }

        if ((tmp  = strstr(tmp, ":")) == NULL)
            continue;

        tmp++;

        if (strncmp(tmp, number, len) == 0 && !isdigit(*(tmp + len)))
        {
            int rc = ta_pclose_r(cmd_pid, f);
            return (rc < 0 ) ? 0 : atoi(line);
        }
    }

    ta_pclose_r(cmd_pid, f);

    return 0;
}

/**
 * Add a new Xvfb daemon with specified display number.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         unused
 * @param number        display number
 *
 * @return error code
 */
static te_errno
ds_xvfb_add(unsigned int gid, const char *oid, const char *value,
            const char *number)
{
    uint32_t pid = xvfb_exists((char *)number);
    char    *tmp;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    strtol(number, &tmp, 10);
    if (tmp == number || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (pid != 0)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    sprintf(buf, "Xvfb :%s -ac &", number);

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Stop Xvfb with specified display number.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param number        display number
 *
 * @return error code
 */
static te_errno
ds_xvfb_del(unsigned int gid, const char *oid, const char *number)
{
    int             pid;
    unsigned int    attempt = TA_UNIX_DAEMON_WAIT_ATTEMPTS;
    te_errno        err = TE_ETIMEDOUT;

    UNUSED(gid);
    UNUSED(oid);

    pid = xvfb_exists((char *)number);
    if (pid == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (kill(pid, SIGTERM) == 0)
    {
        for (attempt = 0;
             xvfb_exists((char *)number) &&
             (attempt < TA_UNIX_DAEMON_WAIT_ATTEMPTS);
             ++attempt)
        {
            usleep(TA_UNIX_DAEMON_WAIT_USEC);
        }
    }
    else
    {
        err = te_rc_os2te(errno);
    }

    if (attempt == TA_UNIX_DAEMON_WAIT_ATTEMPTS)
    {
        ERROR("Failed to stop Xvfb '%s' with PID=%d: %r",
              number, pid, err);
        return TE_RC(TE_TA_UNIX, err);
    }

    return 0;
}

/**
 * Return list of Xvfb servers.
 *
 * @param gid     group identifier (unused)
 * @param oid     full parent object instance identifier (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    location for the list pointer
 *
 * @return error code
 */
static te_errno
ds_xvfb_list(unsigned int gid, const char *oid,
             const char *sub_id, char **list)
{
    FILE *f;
    char  line[1024];
    char *s = buf;
    pid_t cmd_pid;
    int   rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if ((rc = ta_popen_r(PS_ALL_ARGS " | grep -w 'Xvfb' | grep -v grep",
                         &cmd_pid, &f)) < 0)
        return rc;

    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "Xvfb");
        int   n;

        if (tmp == NULL)
        {
            WARN("xvfb_list: ps returned %s", line);
            break;
        }

        if ((tmp  = strstr(tmp, ":")) == NULL || (n = atoi(tmp + 1)) == 0)
            continue;

        s += sprintf(s, "%u ", n);
    }

    if ((rc = ta_pclose_r(cmd_pid, f)) < 0)
        return rc;

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

RCF_PCH_CFG_NODE_COLLECTION(node_ds_xvfb, "Xvfb",
                            NULL, NULL,
                            ds_xvfb_add, ds_xvfb_del, ds_xvfb_list, NULL);


/** Information about all dynamically grabbed daemons/services */
static struct {
    const char *name;

    rcf_pch_rsrc_grab_callback    grab;
    rcf_pch_rsrc_release_callback release;
} ds_info[] = {

#ifdef WITH_L2TP
    { "/agent/l2tp", l2tp_grab, l2tp_release },
#endif

#ifdef WITH_SOCKS
    { "/agent/socks", socks_grab, socks_release },
#endif

#ifdef WITH_RADVD
    { "/agent/radvd", radvd_grab, radvd_release },
#endif

#ifdef WITH_DHCP_SERVER
    { "/agent/dhcpserver", dhcpserver_grab, dhcpserver_release },
#endif

#ifdef WITH_OPENVPN
    { "/agent/openvpn", openvpn_grab, openvpn_release },
#endif

#ifdef WITH_PPPOE_SERVER
    { "/agent/pppoeserver", pppoeserver_grab, pppoeserver_release },
#endif

#ifdef WITH_ECHO_SERVER
    { "/agent/echoserver", echoserver_grab, echoserver_release },
#endif /* WITH_ECHO_SERVER */

#ifdef WITH_TODUDP_SERVER
    { "/agent/todudpserver", todudpserver_grab, todudpserver_release },
#endif /* WITH_TODUDP_SERVER */

#ifdef WITH_TELNET
    { "/agent/telnetd", telnetd_grab, telnetd_release },
#endif /* WITH_TELNET */

#ifdef WITH_RSH
    { "/agent/rshd", rshd_grab, rshd_release },
#endif /* WITH_RSH */

#ifdef WITH_TFTP_SERVER
    { "/agent/tftpserver", tftpserver_grab, tftpserver_release },
#endif /* WITH_TFTP_SERVER */

#ifdef WITH_FTP_SERVER
    { "/agent/ftpserver", ftpserver_grab, ftpserver_release },
#endif /* WITH_FTP_SERVER */

#ifdef WITH_SMTP
    { "/agent/smtp", smtp_grab, smtp_release },
#endif /* WITH_SMTP */

#ifdef WITH_VNCSERVER
    { "/agent/vncserver", vncserver_grab, vncserver_release },
#endif /* WITH_VNCSERVER */

#ifdef WITH_DNS_SERVER
    { "/agent/dnsserver", dnsserver_grab, dnsserver_release },
#endif /* WITH_DMS_SERVER */

#ifdef WITH_RADIUS_SERVER
    { "/agent/radiusserver", radiusserver_grab, radiusserver_release },
#endif /* WITH_RADIUS_SERVER */

#ifdef WITH_VTUND
    { "/agent/vtund", vtund_grab, vtund_release }
#endif

};

/**
 * Initializes conf_daemons support.
 *
 * @return Status code (see te_errno.h)
 */
te_errno
ta_unix_conf_daemons_init()
{
    te_errno rc;
    int      i;

    /* Dynamically grabbed services */

    for (i = 0; i < (int)(sizeof(ds_info) / sizeof(ds_info[0])); i++)
    {
        if ((rc = rcf_pch_rsrc_info(ds_info[i].name,
                                    ds_info[i].grab,
                                    ds_info[i].release)) != 0)
        {
            return rc;
        }
    }

    /* Static services */

    if ((rc = rcf_pch_add_node("/agent", &node_ds_sshd)) != 0 ||
        (rc = rcf_pch_add_node("/agent", &node_ds_xvfb)) != 0 ||
        (rc = slapd_add()) != 0)
    {
        return rc;
    }

#ifdef WITH_PPPOE_SERVER
    if ((rc = pppoe_client_add()) != 0)
        return rc;
#endif

    return 0;
}


/**
 * Release resources allocated for the configuration support.
 * In theory nothing should happen here - CS should care about
 * shutdown of dynamically grabbed resources. However if connection
 * with engine is broken, it's better to cleanup.
 */
void
ta_unix_conf_daemons_release()
{
    int i;

    for (i = 0; i < (int)(sizeof(ds_info) / sizeof(ds_info[0])); i++)
        (ds_info[i].release)(NULL);
}

