/** @file
 * @brief Agent library
 *
 * Networking-related routines
 *
 *
 * Copyright (C) 2004-2016 Test Environment authors (see file AUTHORS
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
 * @author Alexandra N. Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER      "Agent library"

#include "agentlib.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_shell_cmd.h"
#include "logger_api.h"
#include "tq_string.h"

/* Pathname of teamnl tool. */
#define TEAMNL_PATHNAME "/usr/bin/teamnl"

/* See description in agentliv.h */
te_errno
ta_vlan_get_parent(const char *ifname, char *parent)
{
    te_errno rc = 0;
    char     f_buf[200];

    *parent = '\0';
#if defined __linux__
    {
        FILE *proc_vlans = fopen("/proc/net/vlan/config", "r");

        if (proc_vlans == NULL)
        {
            if (errno == ENOENT)
            {
                VERB("%s: no proc vlan file ", __FUNCTION__);
                return 0; /* no vlan support module loaded, no parent */
            }

            ERROR("%s(): Failed to open /proc/net/vlan/config %s",
                  __FUNCTION__, strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        while (fgets(f_buf, sizeof(f_buf), proc_vlans) != NULL)
        {
            char *delim;
            size_t space_ofs;
            char *s = f_buf;
            char *p = parent;

            /*
             * While paring VLAN record we should take into account
             * the format of /proc/net/vlan/config file:
             * <VLAN if name> | <VLAN ID> | <Parent if name>
             * Please note that <VLAN if name> field may be quite long
             * and as the result there can be NO space between
             * <VLAN if name>  value and '|' delimeter. Also we should
             * take into account that '|' character is allowed for
             * interface name value.
             *
             * Extract <VLAN if name> value first.
             */
            delim = strstr(s, "| ");
            if (delim == NULL)
                continue;

            *delim++ = '\0';
            /* Trim interface name (remove spaces before '|' delimeter) */
            space_ofs = strcspn(s, " \t\n\r");
            s[space_ofs] = 0;

            if (strcmp(s, ifname) != 0)
                continue;

            s = delim;
            /* Find next delimiter (we do not need VLAN ID field) */
            s = strstr(s, "| ");
            if (s == NULL)
                continue;

            s++;
            while (isspace(*s)) s++;

            while (!isspace(*s))
                *p++ = *s++;
            *p = '\0';
            break;
        }
        fclose(proc_vlans);
    }
#elif defined __sun__
    {
        int   out_fd = -1;
        FILE *out = NULL;
        int   status;
        char  cmd[80];
        pid_t dladm_cmd_pid;

        snprintf(cmd, sizeof(cmd),
                 "LANG=POSIX /usr/sbin/dladm show-link -p -o OVER %s",
                 ifname);
        dladm_cmd_pid = te_shell_cmd(cmd, -1, NULL, &out_fd, NULL);
        VERB("%s(<%s>): cmd pid %d, out fd %d",
             __FUNCTION__, ifname, (int)dladm_cmd_pid, out_fd);
        if (dladm_cmd_pid < 0)
        {
            ERROR("%s(): start of dladm failed", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }

        if ((out = fdopen(out_fd, "r")) == NULL)
        {
            ERROR("Failed to obtain file pointer for shell command output");
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            goto cleanup;
        }
        if (fgets(f_buf, sizeof(f_buf), out) != NULL)
        {
            size_t len = strlen(f_buf);

            /* Cut trailing new line character */
            if (len != 0 && f_buf[len - 1] == '\n')
                f_buf[len - 1] = '\0';
            snprintf(parent, IFNAMSIZ, "%s", f_buf);
        }
cleanup:
        if (out != NULL)
            fclose(out);
        close(out_fd);

        ta_waitpid(dladm_cmd_pid, &status, 0);
        if (status != 0)
        {
            ERROR("%s(): Non-zero status of dladm: %d",
                  __FUNCTION__, status);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
    }
#endif
    return rc;
}

/* See description in agentlib.h */
te_errno
ta_bond_get_slaves(const char *ifname, tqh_strings *slaves,
                   int *slaves_num, te_bool *is_team)
{
    int    i = 0;
    char   path[64];
    char  *line = NULL;
    size_t len = 0;
    char   buf[256];
    int    out_fd = -1;
    pid_t  cmd_pid = -1;
    int    rc = 0;
    int    status;

    te_bool teamnl_used = FALSE;

    if (slaves_num != NULL)
        *slaves_num = 0;

    TAILQ_INIT(slaves);

    TE_SPRINTF(path, "/proc/net/bonding/%s", ifname);

    FILE *proc_bond = fopen(path, "r");
    if (proc_bond == NULL && errno == ENOENT)
    {
        /* Consider an interface is not bond if teamnl does not exist. */
        if (access(TEAMNL_PATHNAME, X_OK) != 0)
            return 0;

        teamnl_used = TRUE;

        /* Set here path for logging purpose */
        TE_SPRINTF(path, "%s %s ports", TEAMNL_PATHNAME, ifname);
        TE_SPRINTF(buf,
               "sudo %s %s ports | "
               "sed s/[0-9]*:\\ */Slave\\ Interface:\\ / "
               "| sed 's/\\([0-9]\\):.*/\\1/'", TEAMNL_PATHNAME, ifname);
        cmd_pid = te_shell_cmd(buf, -1, NULL, &out_fd, NULL);
        if (cmd_pid < 0)
        {
            ERROR("%s(): getting list of teaming interfaces failed",
                  __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
        if ((proc_bond = fdopen(out_fd, "r")) == NULL)
        {
            ERROR("Failed to obtain file pointer for shell "
                  "command output");
            rc = TE_OS_RC(TE_TA_UNIX, te_rc_os2te(errno));
            goto cleanup;
        }
    }
    if (proc_bond == NULL)
    {
        if (errno == ENOENT)
        {
            VERB("%s: no proc bond file and no team", __FUNCTION__);
            return 0; /* no bond support module loaded, no slaves */
        }

        ERROR("%s(): Failed to read %s %s",
              __FUNCTION__, path, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    while (getline(&line, &len, proc_bond) != -1)
    {
        char *ifname = strstr(line, "Slave Interface");
        if (ifname == NULL)
            continue;
        ifname = strstr(line, ": ") + 2;
        if (ifname == NULL)
            continue;
        ifname[strlen(ifname) - 1] = '\0';
        if (strlen(ifname) > IFNAMSIZ)
        {
            ERROR("%s(): interface name is too long", __FUNCTION__);
            rc = TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
            goto cleanup;
        }

        rc = tq_strings_add_uniq_gen(slaves, ifname, TRUE);
        if (rc != 0)
        {
            ERROR("%s(): failed to add interface name to the list",
                  __FUNCTION__);
            rc = TE_RC(TE_TA_UNIX, rc);
            goto cleanup;
        }
        i++;
    }

cleanup:
    free(line);
    fclose(proc_bond);
    close(out_fd);

    if (cmd_pid >= 0)
    {
        ta_waitpid(cmd_pid, &status, 0);
        if (status != 0)
        {
            ERROR("%s(): Non-zero status of teamnl: %d",
                  __FUNCTION__, status);
            rc = TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
    }

    if (rc == 0)
    {
        if (slaves_num != NULL)
            *slaves_num = i;
    }
    else
    {
        tq_strings_free(slaves, &free);
    }

    if (rc == 0)
    {
        if (is_team != NULL)
            *is_team = teamnl_used;
    }

    return rc;
}
