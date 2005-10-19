/** @file
 * @brief Unix Test Agent
 *
 * Unix daemons configuring implementation
 *
 *
 * Copyright (C) 2004, 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 *
 * $Id$
 */

#include "conf_daemons.h"

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_shell_cmd.h"


/** Maximum number of attempts to wait a daemon in expected state */
#define TA_UNIX_DAEMON_WAIT_ATTEMPTS    1000
/** Time to wait between checks of the daemon state, in microseconds */
#define TA_UNIX_DAEMON_WAIT_USEC        10000


/* Array of daemons/services names */
static struct {
    char   *config_file;
    char   *backup;
    te_bool changed;
} ds[UNIX_SERVICE_MAX];

/* Number of services registered */
static int n_ds = 0;

/** Auxiliary buffer */
static char buf[2048];

/** /etc/hosts backup index */
static int hosts_index;


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
    return (index >= n_ds || index < 0) ? "" : ds[index].config_file;
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
    
    for (i = 0; i < n_ds; i++)
    {
        if (strncmp(dir, ds[i].config_file, dirlen) == 0 &&
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
    return (index >= n_ds || index < 0) ? "" : ds[index].backup;
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
    return (index >= n_ds || index < 0) ? FALSE : ds[index].changed;
}

/** 
 * Notify backup manager that the configuration file was touched.
 *
 * @param index         daemon/service index
 */
void 
ds_config_touch(int index)
{
    if (index < n_ds && index >= 0)
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
    int   rc;
    int   my_pid = getpid();
    
    TE_SPRINTF(buf, "ls %s* 2>/dev/null", backup);
    if ((f = popen(buf, "r")) == NULL)
    {
        rc = errno;
        ERROR("popen(%s) failed with errno %d", buf, rc);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    s = fgets(buf, sizeof(buf), f);
    (void)pclose(f);

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
        
        if (kill(pid, SIGCONT) == 0)
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
    const char *filename;
    FILE       *f;
    int         rc;

    if (name == NULL)
    {
        ERROR("%s(): Invalid parameter", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    filename = strrchr(name, '/');
    if (filename == NULL)
        filename = name;
    else
        filename++;

    if (n_ds == sizeof(ds) / sizeof(ds[0]))          
    {                                                              
        WARN("Too many services are registered\n");     
        return TE_RC(TE_TA_UNIX, TE_EMFILE);                         
    }
    
    TE_SPRINTF(buf, "%s%s", dir ? : "", name);
    ds[n_ds].config_file = strdup(buf);
    if (ds[n_ds].config_file == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    if ((f = fopen(ds[n_ds].config_file, "a")) == NULL)
    {
        WARN("Failed to create backup for %s - no such file", 
             ds[n_ds].config_file);
        free(ds[n_ds].config_file);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    fclose(f);
    
    TE_SPRINTF(buf, TE_TMP_PATH"%s"TE_TMP_BKP_SUFFIX, filename);
    /* Addition memory for pid */
    ds[n_ds].backup = malloc(strlen(buf) + 16); 
    if (ds[n_ds].backup == NULL)
    {
        free(ds[n_ds].config_file);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    strcpy(ds[n_ds].backup, buf);
    
    if ((rc = copy_or_rename(ds[n_ds].config_file, ds[n_ds].backup)) != 0)
    {
        free(ds[n_ds].config_file);
        free(ds[n_ds].backup);
        return rc;
    }

    TE_SPRINTF(buf, "diff -q %s %s >/dev/null 2>&1", 
               ds[n_ds].config_file, ds[n_ds].backup);
    
    ds[n_ds].changed = (ta_system(buf) != 0);
    
    if (index != NULL)                                        
        *index = n_ds;
    n_ds++;
    return 0;                               
} 

/** Restore initial state of the services */
void
ds_restore_backup()
{
    int i;
    
    RING("Restoring backups");

    for (i = 0; i < n_ds; i++)
    {
        char *backup = ds[i].backup;
        
        if (ds[i].changed)
        {
            sprintf(buf, "mv %s %s >/dev/null 2>&1", 
                    ds_backup(i), ds_config(i));
        }
        else
        {
            sprintf(buf, "rm %s >/dev/null 2>&1", ds_backup(i));
        }
        ds[i].backup = "";
        if (ta_system(buf) != 0)
            ERROR("Command <%s> failed", buf);
            
        free(backup);
        free(ds[i].config_file);
    }
    
    n_ds = 0;
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
int
daemon_get(unsigned int gid, const char *oid, char *value)
{
    const char *daemon_name = get_ds_name(oid);

    UNUSED(gid);

    if (daemon_name == NULL)
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

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
        if (ta_system("ps ax | grep '/usr/lib/postfix/master'"
                      "| grep -v grep >/dev/null") == 0)
        {
            sprintf(value, "1");
            return 0;
        }
        /* Fall through to daemon_get() */
    }

    if (strcmp(daemon_name, "qmail") == 0)
        daemon_name = "qmail-send";
    
    sprintf(buf, "killall -CONT %s >/dev/null 2>&1", daemon_name);
    if (ta_system(buf) == 0)
    {
         sprintf(value, "1");
         return 0;
    }
    sprintf(value, "0");

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
int
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

    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("Command '%s' failed with exit code %d", buf, rc);
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
        ta_system("ps ax");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }
    
    return 0;
}

#ifdef WITH_XINETD

/* Get current state of xinetd service */
static int
xinetd_get(unsigned int gid, const char *oid, char *value)
{
    int   index = ds_lookup(XINETD_ETC_DIR, get_ds_name(oid));
    FILE *f;

    UNUSED(gid);
    
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
static int
xinetd_set(unsigned int gid, const char *oid, const char *value)
{
    int   index = ds_lookup(XINETD_ETC_DIR, get_ds_name(oid));
    FILE *f, *g;
    int   rc;
    
    te_bool inside = FALSE;
    char   *server = xinetd_server;
    
    UNUSED(gid);

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
    SLEEP(1);

    if ((rc = ta_system("/etc/init.d/xinetd start")) != 0)
    {
        ERROR("xinetd failed to start with exit code %d", rc);
        return -1;
    }

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
static int
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
static int
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


#ifdef WITH_TFTP_SERVER

static int tfpt_server_index;

/**
 * Get address which TFTP daemon should bind to.
 *
 * @param gid   group identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value value location
 *
 * @return status code
 */
static int
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
static int
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
static int
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
static int
ds_tftpserver_file_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_tftpserver_last_access_params_get(value, NULL);
}

/** Get last time of the TFTP access */
static int
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

/** 
 * Patch TFTP server configuration file.
 *
 * @param last  configuration tree node
 */
void
ds_init_tftp_server(rcf_pch_cfg_object **last)
{
    FILE *f = NULL;
    FILE *g = NULL;
    
    if (ds_create_backup(XINETD_ETC_DIR, "tftp", &tftp_index) != 0)
        return;
    
    ds_config_touch(tftp_index);

    /* Set -v option to tftp */
    OPEN_BACKUP(tftp_index, f);
    OPEN_CONFIG(tftp_index, g);

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

    DS_REGISTER(tftpserver);
    
    return 0;
}

#endif /* WITH_TFTP_SERVER */

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
static int
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
static int
ds_todudpserver_addr_set(unsigned int gid, const char *oid,
                         const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_xinetd_service_addr_set("daytime-udp", value);
}

#endif /* WITH_TODUDP_SERVER */

#ifdef WITH_ECHO_SERVER

/** Get protocol type used by echo server (tcp or udp) */
static int
ds_echoserver_proto_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    return 0;
}

/** Set protocol type used by echo server (tcp or udp) */
static int
ds_echoserver_proto_set(unsigned int gid, const char *oid,
                        const char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    return 0;
}

/** Get IPv4 address echo server is attached to */
static int
ds_echoserver_addr_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_xinetd_service_addr_get("echo", value);
}

/** Attach echo server to specified IPv4 address */
static int
ds_echoserver_addr_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    return ds_xinetd_service_addr_set("echo", value);
}

#endif /* WITH_ECHO_SERVER */

#ifdef WITH_FTP_SERVER


enum ftp_server_kinds { FTP_VSFTPD, FTP_WUFTPD, FTP_PROFTPD };


static int                     ftp_indices[] = {-1, -1, -1};
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
                                               "/etc/wu-ftpd/",
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
        {"vsftpd", "wu-ftpd", "proftpd"};
    return ftpd_names[ftp_server_kind];
}


static void
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
}

#ifdef WITH_XINETD
static int
ds_ftpserver_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(oid);
    
    if (!ftp_standalone)
        xinetd_server = (ftp_server_kind == FTP_VSFTPD) ? 
                        "/usr/sbin/vsftpd" :
                        (ftp_server_kind == FTP_PROFTPD) ? 
                        "/usr/sbin/proftpd" : NULL;
        
    return ftp_standalone ? daemon_set(gid, "ftpserver", value) : 
           xinetd_set(gid, "ftp", value);
}

static int
ds_ftpserver_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);
    return ftp_standalone ? daemon_get(gid, "ftpserver", value) :
           xinetd_get(gid, "ftp", value);
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


static int
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
        ERROR("/etc/xinet.d/ftp not found");
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

static int
ds_ftpserver_server_get(unsigned int gid, const char *oid, char *value)
{
    strcpy(value, ftpd_conf_names[ftp_server_kind][ftp_standalone]);
    UNUSED(gid);
    UNUSED(oid);
    return 0;
}


RCF_PCH_CFG_NODE_RW(node_ds_ftpserver_server, "server",
                    NULL, NULL,
                    ds_ftpserver_server_get, 
                    ds_ftpserver_server_set);

RCF_PCH_CFG_NODE_RW(node_ds_ftpserver, "ftpserver",
                    &node_ds_ftpserver_server, NULL,
                    ds_ftpserver_get, ds_ftpserver_set);


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

/**
 * Initialize FTP daemon.
 *
 * @param last  configuration tree node
 */
void
ds_init_ftp_server(rcf_pch_cfg_object **last)
{
    te_bool ftp_register;
    
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
        return;

    ds_ftpserver_update_config();
    if (ta_system("mkdir -p /var/ftp/pub") != 0)
    {
        WARN("Cannot create /var/ftp/pub");
        return;
    }
    if (ta_system("chmod o+w /var/ftp/pub") !=0)
    {
        ERROR("Cannot chmod /var/ftp/pub");
        return;
    }
    if (ftpserver_running())
    {
        ds_ftpserver_set(0, "ftpserver", "0");
        ds_ftpserver_set(0, "ftpserver", "1");
    }
    DS_REGISTER(ftpserver);
}

/* Restart FTP server, if necesary */
void
ds_shutdown_ftp_server()
{
    ta_system("chmod o-w /var/ftp/pub 2>/dev/null");
    if (ftpserver_running())
    {
        ds_ftpserver_set(0, "ftpserver", "0");
        ds_ftpserver_set(0, "ftpserver", "1");
    }
}

#endif /* WITH_FTP_SERVER */

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
    FILE *f = popen("ps ax | grep 'sshd -p' | grep -v grep", "r");
    char  line[128];
    int   len = strlen(port);
    
    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "sshd");
        
        tmp = strstr(tmp, "-p") + 2;
        while (*++tmp == ' ');
        
        if (strncmp(tmp, port, len) == 0 && !isdigit(*(tmp + len)))
        {
            pclose(f);
            return atoi(line);
        }
    }
    
    pclose(f);

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
static int
ds_sshd_add(unsigned int gid, const char *oid, const char *value,
            const char *port)
{
    uint32_t pid = sshd_exists((char *)port);
    uint32_t p;
    char    *tmp;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    
    p = strtol(port, &tmp, 10);
    if (tmp == port || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    
    if (pid != 0)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
        
    sprintf(buf, "/usr/sbin/sshd -p %s", port);

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
static int
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
 * @param oid           full object instence identifier (unused)
 * @param list          location for the list pointer
 *
 * @return error code
 */
static int
ds_sshd_list(unsigned int gid, const char *oid, char **list)
{
    FILE *f = popen("ps ax | grep 'sshd -p' | grep -v grep", "r");
    char  line[128];
    char *s = buf;

    UNUSED(gid);
    UNUSED(oid);
    
    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "sshd");
        
        tmp = strstr(tmp, "-p") + 2;
        while (*++tmp == ' ');
        
        s += sprintf(s, "%u ", atoi(tmp));
    }
    
    pclose(f);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    return 0;
}

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
    FILE *f = popen("ps ax | grep 'Xvfb' | grep -v grep", "r");
    char  line[128];
    int   len = strlen(number);
    
    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "Xvfb");

        if (tmp == NULL)
        {
            ERROR("xvfb_list: ps returned %s", line);
            break;
        }
        
        if ((tmp  = strstr(tmp, ":")) == NULL)
            continue;
        
        tmp++;
        
        if (strncmp(tmp, number, len) == 0 && !isdigit(*(tmp + len)))
        {
            pclose(f);
            return atoi(line);
        }
    }
    
    pclose(f);

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
static int
ds_xvfb_add(unsigned int gid, const char *oid, const char *value,
            const char *number)
{
    uint32_t pid = xvfb_exists((char *)number);
    uint32_t n;
    char    *tmp;
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    
    n = strtol(number, &tmp, 10);
    if (tmp == number || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    
    if (pid != 0)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
        
    sprintf(buf, "Xvfb :%s -ac 2>/dev/null &", number);

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
static int
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
 * @param gid    group identifier (unused)
 * @param oid    full object instence identifier (unused)
 * @param list   location for the list pointer
 *
 * @return error code
 */
static int
ds_xvfb_list(unsigned int gid, const char *oid, char **list)
{
    FILE *f = popen("ps ax | grep 'Xvfb' | grep -v grep", "r");
    char  line[128];
    char *s = buf;

    UNUSED(gid);
    UNUSED(oid);
    
    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "Xvfb");
        int   n;
        
        if (tmp == NULL)
        {
            ERROR("xvfb_list: ps returned %s", line);
            break;
        }

        if ((tmp  = strstr(tmp, ":")) == NULL || (n = atoi(tmp + 1)) == 0)
            continue;
        
        s += sprintf(s, "%u ", n);
    }
    
    pclose(f);
    
    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    return 0;
}

#ifdef WITH_VNCSERVER

/** Read VNC password */
static int
ds_vncpasswd_get(unsigned int gid, const char *oid, char *value)
{
    FILE *f;
    int   rc;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((f = fopen("/tmp/.vnc/passwd", "r")) == NULL)
    {
        rc = errno;
        ERROR("Failed to open /tmp/.vnc directory");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    memset(value, 0, RCF_MAX_VAL);
    fread(value, 1, RCF_MAX_VAL - 1, f);
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
static int
ds_vncserver_add(unsigned int gid, const char *oid, const char *value,
                 const char *number)
{
    uint32_t n;
    char    *tmp;
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    
    n = strtol(number, &tmp, 10);
    if (tmp == number || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    
    if (vncserver_exists((char *)number))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
        
    sprintf(buf, "HOME=/tmp vncserver :%s >/dev/null", number);

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    sprintf(buf, "HOME=/tmp DISPLAY=:%s xhost + >/dev/null", number);

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        sprintf(buf, "HOME=/tmp vncserver -kill :%s >/dev/null 2>&1", 
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
static int
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
 * @param gid    group identifier (unused)
 * @param oid    full object instence identifier (unused)
 * @param list   location for the list pointer
 *
 * @return error code
 */
static int
ds_vncserver_list(unsigned int gid, const char *oid, char **list)
{
    FILE *f = popen("ls /tmp/.vnc/*.pid 2>/dev/null", "r");
    char  line[128];
    char *s = buf;

    UNUSED(gid);
    UNUSED(oid);
    
    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp;
        int   n;

        if ((tmp  = strstr(line, ":")) == NULL || (n = atoi(tmp + 1)) == 0)
            continue;
        
        s += sprintf(s, "%u ", n);
    }
    pclose(f);
    
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

/**
 * Initialize VNC password file.
 *
 * @param last  configuration tree node
 */
void
ds_init_vncserver(rcf_pch_cfg_object **last)
{
    uint8_t passwd[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
    int     fd;

    ta_system("rm -rf /tmp/.vnc");
        
    if (mkdir("/tmp/.vnc", 0700) < 0)
    {
        WARN("Failed to create /tmp/.vnc directory");
        return;
    } 
    
    if ((fd = open("/tmp/.vnc/passwd", O_CREAT | O_WRONLY, 0600)) <= 0)
    {
        WARN("Failed to create file /tmp/.vnc/passwd; errno %x", errno);
        return;
    }
    
    if (write(fd, passwd, sizeof(passwd)) < 0)
    {
        WARN("write() failed for the file /tmp/.vnc/passwd; errno %x", 
             errno);
        close(fd);
        return;
    }
    
    if (close(fd) < 0)
    {
        WARN("close() failed for the file /tmp/.vnc/passwd");
        return;
    }

    DS_REGISTER(vncpasswd);
    DS_REGISTER(vncserver);
}

#endif /* WITH_VNCSERVER */

#ifdef WITH_SMTP

#define SMTP_EMPTY_SMARTHOST    "0.0.0.0"

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
    "exim",      
    "sendmail",
    "postfix",
    "qmail"
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
    fprintf(g, "%s te_tester%u", ip, smarthost_name_index);
    fclose(f);
    fclose(g);
    
    /* Commit all changes in config files */
    sync();

    return 0;
}

/*---------------------- sendmail staff ------------------------------*/

/** sendmail configuration location */
#define SENDMAIL_CONF_DIR   "/etc/mail/"

/** Smarthost option format */
#define SENDMAIL_SMARTHOST_OPT_S  "define(`SMART_HOST',`te_tester"
#define SENDMAIL_SMARTHOST_OPT_F  "%u')\n"

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
            *enable = 1;
            return 0;
        }
    }
    *enable = 0;
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
        if (strstr(buf, "SMART_HOST") == NULL)
            fwrite(buf, 1, strlen(buf), g);
    }
    if (enable != 0)
        fprintf(g, SENDMAIL_SMARTHOST_OPT_S SENDMAIL_SMARTHOST_OPT_F, 
                smarthost_name_index);

    fclose(f);
    fclose(g);

    /* Commit all changes in config files before restart of the service */
    sync();

    if ((rc = ta_system("make -C " SENDMAIL_CONF_DIR)) != 0)
    {
        ERROR("make -C " SENDMAIL_CONF_DIR " failed with code %d", rc);
        return -1;
    }
    
    return 0;
}

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


/*------------------ Common mail staff --------------------------*/

/** Get SMTP smarthost */
static int
ds_smtp_smarthost_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    strcpy(value, SMTP_EMPTY_SMARTHOST);
    if (smtp_current == NULL)
        return 0;
        
    if (strcmp(smtp_current, "sendmail") == 0)
    {
        te_bool enable;
        int     rc;
        
        if ((rc = sendmail_smarthost_get(&enable)) != 0)
            return rc;
            
        if (enable)
            strcpy(value, smtp_current_smarthost);
    }
    else if (strcmp(smtp_current, "postfix") == 0)
    {
        te_bool enable;
        int     rc;
        
        if ((rc = postfix_smarthost_get(&enable)) != 0)
            return rc;
            
        if (enable)
            strcpy(value, smtp_current_smarthost);
    }
    else if (strcmp(smtp_current, "exim") == 0)
    {
        te_bool enable;
        int     rc;
        
        if ((rc = exim_smarthost_get(&enable)) != 0)
            return rc;
            
        if (enable)
            strcpy(value, smtp_current_smarthost);
    }
    else if (strcmp(smtp_current, "qmail") == 0)
    {
        te_bool enable;
        int     rc;
        
        if ((rc = qmail_smarthost_get(&enable)) != 0)
            return rc;
            
        if (enable)
            strcpy(value, smtp_current_smarthost);
    }

    return 0;
}

/** Set SMTP smarthost */
static int
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
static int
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
static int
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
static int
ds_smtp_server_set(unsigned int gid, const char *oid, const char *value)
{
    unsigned int i;
    
    int rc;
    
    char *smtp_prev = smtp_current;
    char *smtp_prev_daemon = smtp_current_daemon;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if (smtp_current != NULL && daemon_running(smtp_current_daemon))
    {
        ERROR("Cannot set smtp to %s: %s is running", oid, 
              smtp_current_daemon);
        ta_system("ps -ax");
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
static int
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

RCF_PCH_CFG_NODE_RW(node_ds_smtp_smarthost, "smarthost",
                    NULL, NULL,
                    ds_smtp_smarthost_get, ds_smtp_smarthost_set);

RCF_PCH_CFG_NODE_RW(node_ds_smtp_server, "server",
                    NULL, &node_ds_smtp_smarthost,
                    ds_smtp_server_get, ds_smtp_server_set);

RCF_PCH_CFG_NODE_RW(node_ds_smtp, "smtp",
                    &node_ds_smtp_server, NULL,
                    ds_smtp_get, ds_smtp_set);

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
    else if (strcmp(smtp_current, "postfix") == 0)
    {
        rc = ta_system("/etc/init.d/postfix flush");
    }
    else if (strcmp(smtp_current, "qmail") == 0)
    {
        rc = ta_system("killall -ALRM qmail-send");
    }
    else if (strcmp(smtp_current, "sendmail") == 0)
    {
        rc = ta_system("sendmail-mta -q");
        if (rc != 0)
            rc = ta_system("sendmail -q");
    }
    else if (strcmp(smtp_current, "exim") == 0)
    {
        char buf[30];
        snprintf(buf, sizeof(buf), "%s -qff", exim_name);
        rc = te_shell_cmd(buf, -1, NULL, NULL);
        if (rc > 0)
            rc = 0;
        else
            rc = -1;
    }
    else
    {
        WARN("Flushing not implemented for %s", smtp_current);
    }
    if (rc != 0)
        ERROR("Flushing failed with code %d", rc);
}


/** 
 * Initialize SMTP-related variables. 
 *
 * @param last  configuration tree node
 */
void
ds_init_smtp(rcf_pch_cfg_object **last)
{
    unsigned int i;
    
    if (file_exists(SENDMAIL_CONF_DIR "sendmail.mc") &&
        ds_create_backup(SENDMAIL_CONF_DIR, "sendmail.mc", 
                         &sendmail_index) != 0)
    {
        return;
    }

    if (file_exists(EXIM_CONF_DIR "update-exim.conf.conf"))
    {
        if (ds_create_backup(EXIM_CONF_DIR, "update-exim.conf.conf", 
                             &exim_index) != 0)
            return;
    }
    else if (file_exists(EXIM4_CONF_DIR "update-exim4.conf.conf"))
    {
        exim_name = "exim4";
        if (ds_create_backup(EXIM4_CONF_DIR, "update-exim4.conf.conf", 
                             &exim_index) != 0)
            return;                             
    }

    if (file_exists(POSTFIX_CONF_DIR "main.cf") &&
        ds_create_backup(POSTFIX_CONF_DIR, "main.cf", 
                         &postfix_index) != 0)
    {
        return;
    }

    if (file_exists(QMAIL_CONF_DIR "smtproutes") &&
        ds_create_backup(QMAIL_CONF_DIR, "smtproutes", 
                         &qmail_index) != 0)
    {
        return;
    }

        
    smtp_current_smarthost = strdup(SMTP_EMPTY_SMARTHOST);
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
    DS_REGISTER(smtp);
}

/** Restore SMTP */
void
ds_shutdown_smtp()
{
    if (sendmail_index >= 0 && ds_config_changed(sendmail_index))
    {
        if (file_exists(SENDMAIL_CONF_DIR))
        {
            ta_system("make -C " SENDMAIL_CONF_DIR);
        }
    }
    if (exim_index >= 0 && ds_config_changed(exim_index))
    {
        sprintf(buf, "update-%s.conf >/dev/null 2>&1", exim_name);
        ta_system(buf);
    }
    if (smtp_current != NULL)
        daemon_set(0, smtp_current_daemon, "0");

    if (smtp_initial != NULL)
        daemon_set(0, smtp_initial, "1");

    free(smtp_current_smarthost);        
}

#endif /* WITH_SMTP */

/*
 * Daemons configuration tree in reverse order.
 */

#ifdef WITH_ECHO_SERVER

RCF_PCH_CFG_NODE_RW(node_ds_echoserver_addr, "net_addr",
                    NULL, NULL,
                    ds_echoserver_addr_get, ds_echoserver_addr_set);

RCF_PCH_CFG_NODE_RW(node_ds_echoserver_proto, "proto",
                    NULL, &node_ds_echoserver_addr,
                    ds_echoserver_proto_get, ds_echoserver_proto_set);

RCF_PCH_CFG_NODE_RW(node_ds_echoserver, "echoserver",
                    &node_ds_echoserver_proto, NULL,
                    xinetd_get, xinetd_set);

#endif /* WITH_ECHO_SERVER */

#ifdef WITH_TELNET
RCF_PCH_CFG_NODE_RW(node_ds_telnet, "telnetd",
                    NULL, NULL, xinetd_get, xinetd_set);
#endif /* WITH_TELNET */

#ifdef WITH_RSH
RCF_PCH_CFG_NODE_RW(node_ds_rsh, "rshd",
                    NULL, NULL, xinetd_get, xinetd_set);
#endif /* WITH_RSH */

#ifdef WITH_TODUDP_SERVER

RCF_PCH_CFG_NODE_RW(node_ds_todudpserver_addr, "net_addr",
                    NULL, NULL,
                    ds_todudpserver_addr_get, ds_todudpserver_addr_set);

RCF_PCH_CFG_NODE_RW(node_ds_todudpserver, "todudpserver",
                    &node_ds_todudpserver_addr, NULL,
                    xinetd_get, xinetd_set);

#endif /* WITH_TODUDP_SERVER */

RCF_PCH_CFG_NODE_COLLECTION(node_ds_sshd, "sshd",
                            NULL, NULL, 
                            ds_sshd_add, ds_sshd_del, ds_sshd_list, NULL);

RCF_PCH_CFG_NODE_COLLECTION(node_ds_xvfb, "Xvfb",
                            NULL, NULL, 
                            ds_xvfb_add, ds_xvfb_del, ds_xvfb_list, NULL);


static void *
supervise_backups(void *arg)
{
    UNUSED(arg);
    
    while (TRUE)
    {
        int i;
        
        for (i = 0; i < n_ds; i++)
        {
            char       *backup = strdup(ds_backup(i));
            struct stat st;
            
            if (backup[0] == 0)
            {
                free(backup);
                continue;
            }
            if (stat(backup, &st) != 0)
            {
                WARN("Backup %s disappeared", backup);
                ta_system("ls /tmp/te*backup");
                USLEEP(200);
                ta_system("ps ax");
                free(backup);
                return NULL;
            }
            free(backup);
        }
        sleep(1);
    }
}


/**
 * Initializes conf_daemons support.
 *
 * @param last  node in configuration tree (last sun of /agent) to be
 *              updated
 *
 * @return status code (see te_errno.h)
 */
int
ta_unix_conf_daemons_init(rcf_pch_cfg_object **last)
{
#ifdef WITH_ECHO_SERVER
    if (ds_create_backup(XINETD_ETC_DIR, "echo", NULL) == 0)
        DS_REGISTER(echoserver);
#endif /* WITH_ECHO_SERVER */

#ifdef WITH_TODUDP_SERVER
    if (ds_create_backup(XINETD_ETC_DIR, "daytime-udp", NULL) == 0)
        DS_REGISTER(todudpserver);
#endif /* WITH_TODUDP_SERVER */

#ifdef WITH_TELNET
    if (ds_create_backup(XINETD_ETC_DIR, "telnet", NULL) == 0)
        DS_REGISTER(telnet);
#endif /* WITH_TELNET */

#ifdef WITH_RSH
    if (ds_create_backup(XINETD_ETC_DIR, "rsh", NULL) == 0)
        DS_REGISTER(rsh);
#endif /* WITH_RSH */

#ifdef WITH_TFTP_SERVER
    ds_init_tftp_server(last);
#endif /* WITH_TFTP_SERVER */

#ifdef WITH_FTP_SERVER
    ds_init_ftp_server(last);
#endif /* WITH_FTP_SERVER */

#ifdef WITH_VNCSERVER
    ds_init_vncserver(last);
#endif

#ifdef WITH_DHCP_SERVER
    ds_init_dhcp_server(last);
#endif /* WITH_DHCP_SERVER */

#ifdef WITH_RADIUS_SERVER
    ds_init_radius_server(last);
#endif /* WITH_RADIUS_SERVER */

#ifdef WITH_DNS_SERVER
    ds_init_dns_server(last);
#endif /* WITH_DMS_SERVER */

#ifdef WITH_VTUND
    ds_init_vtund(last);
#endif

#ifdef WITH_SMTP
    if (ds_create_backup("/etc/", "hosts", &hosts_index) == 0)
        ds_init_smtp(last);
    else
        ERROR("SMTP server updates /etc/hosts and cannot be initialized");
#endif /* WITH_SMTP */

    DS_REGISTER(sshd);

    DS_REGISTER(xvfb);
    
    {
        pthread_t tid = 0;
        
        pthread_create(&tid, NULL, supervise_backups, NULL);
    }

    /* Commit all changes in config files */
    sync();

    return 0;

#undef CHECK_RC
}


/**
 * Release resources allocated for the configuration support.
 */
void
ta_unix_conf_daemons_release()
{
    ds_restore_backup();

#ifdef WITH_DHCP_SERVER
    ds_shutdown_dhcp_server();
#endif /* WITH_DHCP_SERVER */

#ifdef WITH_FTP_SERVER
    ds_shutdown_ftp_server();
#endif    

#ifdef WITH_SMTP
    ds_shutdown_smtp();
#endif

#ifdef WITH_RADIUS_SERVER
    ds_shutdown_radius_server();
#endif

#ifdef WITH_VTUND
    ds_shutdown_vtund();
#endif

    ta_system("/etc/init.d/xinetd restart >/dev/null");
}

