/** @file
 * @brief Unix Test Agent
 *
 * LDAP support
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Alexandra N. Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "conf_daemons.h"

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_shell_cmd.h"
#include "te_sleep.h"

/** Auxiliary buffer */
static char buf[256];

/**
 * Check if slapd with specified port is running.
 *
 * @param port  port in string representation
 *
 * @return pid of the daemon or 0
 */
static uint32_t
slapd_exists(char *port)
{
    FILE *f = popen("ps ax | grep 'slapd ' | grep -v grep", "r");
    char  line[128];
    int   len = strlen(port);

    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "slapd");

        tmp = strstr(tmp, ":");
        if (tmp == NULL)
            continue;
        tmp++;

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
 * Add a new slapd with specified port.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         unused
 * @param port          slapd port
 *
 * @return error code
 */
static te_errno
ds_slapd_add(unsigned int gid, const char *oid, const char *value,
            const char *port)
{
    uint32_t pid = slapd_exists((char *)port);
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

    sprintf(buf, "rm -rf /tmp/ldap; mkdir /tmp/ldap");
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    sprintf(buf, "/usr/sbin/slapadd -f %s/slapd.conf "
            "-l /tmp/sample.ldif", ta_dir);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    sprintf(buf, 
            "/usr/sbin/slapd -4 -f %s/slapd.conf -h ldap://0.0.0.0:%s/", 
            ta_dir, port);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Stop slapd with specified port.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param port          slapd port
 *
 * @return error code
 */
static te_errno
ds_slapd_del(unsigned int gid, const char *oid, const char *port)
{
    uint32_t pid = slapd_exists((char *)port);

    UNUSED(gid);
    UNUSED(oid);

    if (pid == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (kill(pid, SIGTERM) != 0)
    {
        int kill_errno = errno;
        ERROR("Failed to send SIGTERM to slapd daemon with PID=%u: %d",
              pid, kill_errno);
        /* Just to make sure */
        kill(pid, SIGKILL);
    }

    sprintf(buf, "rm -rf /tmp/ldap");
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    te_msleep(100);

    return 0;
}

/**
 * Return list of slapd daemons.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param list          location for the list pointer
 *
 * @return error code
 */
static te_errno
ds_slapd_list(unsigned int gid, const char *oid, char **list)
{
    FILE *f = popen("ps ax | grep 'slapd ' | grep -v grep", "r");
    char  line[128];
    char *s = buf;

    UNUSED(gid);
    UNUSED(oid);
    
    buf[0] = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *tmp = strstr(line, "slapd");
        
        tmp = strstr(tmp, ":") + 2;
        s += sprintf(s, "%u ", atoi(tmp));
    }
    
    pclose(f);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    return 0;
}

RCF_PCH_CFG_NODE_COLLECTION(node_ds_slapd, "slapd",
                            NULL, NULL, 
                            ds_slapd_add, ds_slapd_del, ds_slapd_list, 
                            NULL);

/**
 * Add slapd node to the configuration tree.
 *
 * @return Status code.
 */
te_errno 
slapd_add(void)
{
    return rcf_pch_add_node("/agent", &node_ds_slapd);
}
