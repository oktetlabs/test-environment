/** @file
 * @brief Unix Test Agent
 *
 * PPPoE server configuring
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
 * @author Igor Labutin <Igor.Labutin@oktetlabs.ru>
 *
 * $Id: dhcp_server.c 50987 2008-06-26 12:50:41Z igorl $
 */

#include <netinet/in.h>
#include "conf_daemons.h"
#include "pppoe_server.h"

/** List of known possible locations of PPPoE server executables */
static const char *pppoe_server_execs[] = {
    "/usr/sbin/pppoe-server"
};

/** Number of known possible locations of PPPoE server executables */
static unsigned int pppoe_server_n_execs =
    sizeof(pppoe_server_execs) / sizeof(pppoe_server_execs[0]);

/** PPPoE server executable name */
static const char *pppoe_server_exec = NULL;

/** PPPoE server configuration file name */
static const char *pppoe_server_conf = NULL;

/** PPPoE server subnet */
static te_pppoe_server_subnet *pppoe_server_subnet = NULL;

/** PPPoE server interfaces */
static char *pppoe_server_ifs = NULL;

/** Was PPPoE server enabled at TA start up? */
static te_bool pppoe_server_was_run = FALSE;

/** Auxiliary buffer */
static char buf[2048];

/** Release all memory allocated for option structure */
#define FREE_OPTION(_opt) \
    do {                        \
        free(_opt->name);       \
        free(_opt->value);      \
        free(_opt);             \
    } while (0)

/* Find the option in specified options list */
static te_pppoe_option *
find_option(te_pppoe_option *opt, const char *name)
{
    for (; opt != NULL && strcmp(opt->name, name) != 0; opt = opt->next);

    return opt;
}

/** Save configuration to the file */
static int
ps_pppoeserver_save_conf(char **args)
{
    FILE                    *f = fopen(pppoe_server_conf, "w");
    static char              args_buf[2048];
    unsigned int             mask;
    struct in_addr           addr, new_addr;
    char                    *p;
    char                    *token;

    if (f == NULL)
    {
        ERROR("Failed to open '%s' for writing: %s",
              pppoe_server_conf, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

#if defined __linux__
    fprintf(f, "noauth\n");
    fprintf(f, "lcp-echo-interval 10\n");
    fprintf(f, "lcp-echo-failure 2\n");
    fprintf(f, "nodefaultroute\n");
    fprintf(f, "mru 1492\n");
    fprintf(f, "mtu 1492\n");
#endif

    mask = PREFIX2MASK(pppoe_server_subnet->prefix_len);
    addr.s_addr = inet_addr(pppoe_server_subnet->subnet);
    addr.s_addr = htonl(ntohl(addr.s_addr) & mask);

    new_addr.s_addr = htonl(ntohl(addr.s_addr) + 1);
    sprintf(buf, "-L %s", inet_ntoa(new_addr));
    strcpy(args_buf, buf);

/* Default number of pppoe clients supported by pppoe-server */
#define N_PPP_CLIENTS   64

    /* This fix is ugly, indeed. Just to prevent overlapping of local
     * and remote IPs for multiple pppoe clients. To be replaced by
     * more nice one after all problems with pppoe and dhcp servers
     * have been fixed.
     */
    new_addr.s_addr = htonl(ntohl(addr.s_addr) + N_PPP_CLIENTS + 1);
#if 0
    /* This works good if pppoe-server has no more than ONE client. */
    new_addr.s_addr = htonl(ntohl(addr.s_addr) + 2);
#endif
    sprintf(buf, " -R %s", inet_ntoa(new_addr));
    strcat(args_buf, buf);

    p = pppoe_server_ifs;
    while (p != NULL && *p != '\0')
    {
        token = p;
        p = strchr(p, ' ');
        if (p != NULL && *p != '\0')
        {
            *p = '\0';
            p++;
        }

        sprintf(buf, " -I %s", token);
        strcat(args_buf, buf);
    }

    if (args != NULL)
    {
        *args = args_buf;
    }

#if defined __linux__
    if (fsync(fileno(f)) != 0)
    {
        int err = errno;

        ERROR("%s(): fsync() failed: %s", __FUNCTION__, strerror(err));
        (void)fclose(f);
        return TE_OS_RC(TE_TA_UNIX, err);
    }

    if (fclose(f) != 0)
    {
        ERROR("%s(): fclose() failed: %s", __FUNCTION__, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
#endif

    return 0;
}

/** Is PPPoE server daemon running */
static te_bool
ps_pppoeserver_is_run(void)
{
#if defined __linux__
    /*
     * We should kill all pppd sessions. Killing with some polite signals
     * does not help
     */
#if 0
    (void)ta_system("killall -KILL pppd");
#endif
    sprintf(buf, "killall -CONT %s >/dev/null 2>&1", pppoe_server_exec);
#endif

    return (ta_system(buf) == 0);
}

/** Get PPPoE server daemon on/off */
static te_errno
ps_pppoeserver_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    strcpy(value, ps_pppoeserver_is_run() ? "1" : "0");

    return 0;
}

/** Stop PPPoE server */
static te_errno
ps_pppoeserver_stop(void)
{
    ENTRY("%s()", __FUNCTION__);

#if defined __linux__
    TE_SPRINTF(buf, "killall %s", pppoe_server_exec);
#endif
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    (void)ta_system("killall -KILL pppd");
    return 0;
}

/** Start DHCP server */
static te_errno
ps_pppoeserver_start(void)
{
    te_errno rc;
    char    *args = NULL;

    ENTRY("%s()", __FUNCTION__);

    rc = ps_pppoeserver_save_conf(&args);
    if (rc != 0)
    {
        ERROR("Failed to save PPPoE server configuration file");
        return rc;
    }

#if defined __linux__
    TE_SPRINTF(buf, "%s -O %s -l %s",
               pppoe_server_exec, pppoe_server_conf, args);
#endif
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/** On/off PPPoE server */
static te_errno
ps_pppoeserver_set(unsigned int gid, const char *oid, const char *value)
{
    te_bool  is_run = ps_pppoeserver_is_run();
    te_bool  do_run;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    ENTRY("%s(): value=%s", __FUNCTION__, value);

    if (strlen(value) != 1 || (*value != '0' && *value != '1'))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    do_run = (*value == '1');

    /*
     * We don't need to change state of PPPoE Server:
     * The current state is the same as desired.
     */
    if (is_run == do_run)
        return 0;

    if (do_run)
    {
        rc = ps_pppoeserver_start();
    }
    else
    {
        rc = ps_pppoeserver_stop();
    }

    return rc;
}

/** Get PPPoE server interfaces */
static te_errno
ps_pppoeserver_ifs_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    strcpy(value, pppoe_server_ifs ? : "");

    return 0;
}

/** Set PPPoE server interfaces */
static te_errno
ps_pppoeserver_ifs_set(unsigned int gid, const char *oid, const char *value)
{
    char *copy;

    UNUSED(gid);
    UNUSED(oid);

    /* TODO Check value */

    copy = strdup(value);
    if (copy == NULL)
    {
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, value);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    free(pppoe_server_ifs);
    pppoe_server_ifs = copy;

    return 0;
}

static void
free_subnet(te_pppoe_server_subnet *s)
{
    te_pppoe_option *opt, *next;

    free(s->subnet);
    for (opt = s->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
    free(s);
}

static te_pppoe_server_subnet *
find_subnet(const char *subnet)
{
    UNUSED(subnet);
    return pppoe_server_subnet;
}

static te_errno
ps_subnet_get(unsigned int gid, const char *oid, char *value,
              const char *pppoeserver, const char *subnet)
{
    te_pppoe_server_subnet *s;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoeserver);

    if ((s = find_subnet(subnet)) == NULL)
        return TE_OS_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%d", s->prefix_len);

    return 0;
}

static te_errno
ps_subnet_set(unsigned int gid, const char *oid, const char *value,
              const char *pppoeserver, const char *subnet)
{
    te_pppoe_server_subnet *s;
    int                     prefix_len;
    char                   *end;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoeserver);

    if ((s = find_subnet(subnet)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    prefix_len = strtol(value, &end, 10);
    if (value == end || *end != '\0')
        return TE_RC(TE_TA_UNIX, TE_EFMT);

    s->prefix_len = prefix_len;

    return 0;
}

static te_errno
ps_subnet_add(unsigned int gid, const char *oid, const char *value,
              const char *pppoeserver, const char *subnet)
{
    te_pppoe_server_subnet *s;
    int                     prefix_len;
    char                   *end;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoeserver);

    if ((s = find_subnet(subnet)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    prefix_len = strtol(value, &end, 10);
    if (value == end || *end != '\0')
        return TE_RC(TE_TA_UNIX, TE_EFMT);

    if ((s = calloc(1, sizeof(*s))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((s->subnet = strdup(subnet)) == NULL)
    {
        free(s);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    s->prefix_len = prefix_len;

    pppoe_server_subnet = s;

    return 0;
}

static te_errno
ps_subnet_del(unsigned int gid, const char *oid,
              const char *pppoeserver, const char *subnet)
{
    te_pppoe_server_subnet  *s;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoeserver);

    if ((s = find_subnet(subnet)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    pppoe_server_subnet = NULL;
    free_subnet(s);

    return 0;
}

static te_errno
ps_subnet_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

    if (pppoe_server_subnet != NULL)
    {
        sprintf(buf, "%s", pppoe_server_subnet->subnet);
    }
    else
    {
        strcpy(buf, "");
    }

    return (*list = strdup(buf)) == NULL ?
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;
}


static rcf_pch_cfg_object node_ps_subnet =
    { "subnet", 0, NULL, NULL,
      (rcf_ch_cfg_get)ps_subnet_get,
      (rcf_ch_cfg_set)ps_subnet_set,
      (rcf_ch_cfg_add)ps_subnet_add,
      (rcf_ch_cfg_del)ps_subnet_del,
      (rcf_ch_cfg_list)ps_subnet_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ps_pppoeserver_ifs, "interfaces",
                    NULL, &node_ps_subnet,
                    ps_pppoeserver_ifs_get, ps_pppoeserver_ifs_set);

RCF_PCH_CFG_NODE_RW(node_ps_pppoeserver, "pppoeserver",
                    &node_ps_pppoeserver_ifs, NULL,
                    ps_pppoeserver_get, ps_pppoeserver_set);

te_errno
pppoeserver_grab(const char *name)
{
    int rc = 0;

    UNUSED(name);

    pppoe_server_subnet = NULL;

    if ((rc = rcf_pch_add_node("/agent", &node_ps_pppoeserver)) != 0)
        return rc;

    /* Find PPPoE server executable */
    rc = find_file(pppoe_server_n_execs, pppoe_server_execs, TRUE);
    if (rc < 0)
    {
        ERROR("Failed to find PPPoE server executable"
             " - PPPoE will not be available");
        rcf_pch_del_node(&node_ps_pppoeserver);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    pppoe_server_exec = pppoe_server_execs[rc];


#if defined __linux__
    /* FIXME */
    pppoe_server_conf = "/tmp/te.pppoe-server.conf";

    if (ps_pppoeserver_is_run())
    {
        rc = ps_pppoeserver_stop();
        if (rc != 0)
        {
            ERROR("Failed to stop PPPoE server"
                  " - PPPoE will not be available");
            rcf_pch_del_node(&node_ps_pppoeserver);
            return rc;
        }
        pppoe_server_was_run = TRUE;
    }
#endif

    return 0;
}

te_errno
pppoeserver_release(const char *name)
{
    te_errno    rc;

    UNUSED(name);

    rc = rcf_pch_del_node(&node_ps_pppoeserver);
    if (rc != 0)
        return rc;

    if (pppoe_server_was_run)
    {
        if (ps_pppoeserver_is_run())
        {
            WARN("PPPoE server was disabled at start up from TE point "
                 "of view, however it is enabled at shutdown. It looks "
                 "like you have configuration rollback issues.");
            (void)ps_pppoeserver_stop();
        }
        /*
         * FIXME: Cannot restore original PPPoE server, since we didn't
         * store any backups of configuration files
         */
        WARN("TE is unable to restore PPPoE server state (running) to "
             "the initial state");
        pppoe_server_was_run = FALSE;
    }
    if (pppoe_server_conf != NULL && unlink(pppoe_server_conf) != 0 &&
        errno != ENOENT)
    {
        ERROR("Failed to delete PPPoE server temporary configuration "
              "file '%s': %s", pppoe_server_conf, strerror(errno));
    }

    return 0;
}

