/** @file
 * @brief Unix Test Agent
 *
 * DHCP server configuring 
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

/* Still debugging, it switches off debug messages */

#ifdef WITH_DHCP_SERVER

#include "conf_daemons.h"
#include "dhcp_server.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <libgen.h>

/**
 * Define it to use DHCP server native configuration:
 * - parse/backup/update/rollback of existing configuration file(s) and
 *   database of leases;
 * - use /etc/init.d/ script to start/stop daemon.
 *
 * Otherwise, DHCP server is stopped at TA start up and empty private
 * configuration file and leases database are used.
 */
#undef TA_UNIX_ISC_DHCPS_NATIVE_CFG


/** List of known possible locations of DHCP server scripts */
static const char *dhcp_server_scripts[] = {
    "/etc/init.d/dhcpd",
    "/etc/init.d/dhcp3-server",
    "/etc/init.d/dhcp"
};

/** Number of known possible locations of DHCP server scripts */
static unsigned int dhcp_server_n_scripts =
    sizeof(dhcp_server_scripts) / sizeof(dhcp_server_scripts[0]);


/** List of known possible locations of DHCP server executables */
static const char *dhcp_server_execs[] = {
    "/usr/sbin/dhcpd",
    "/usr/sbin/dhcpd3",
    "/usr/lib/inet/in.dhcpd"
};

/** Number of known possible locations of DHCP server executables */
static unsigned int dhcp_server_n_execs =
    sizeof(dhcp_server_execs) / sizeof(dhcp_server_execs[0]);


#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__

/** List of known possible locations of DHCP server configuration file */
static const char *dhcp_server_confs[] = {
    "/etc/dhcpd.conf",
    "/etc/dhcp3/dhcpd.conf",
    "/etc/inet/dhcpsvc.conf"
};

/** Number of known possible locations of DHCP server configuration file */
static unsigned int dhcp_server_n_confs =
    sizeof(dhcp_server_confs) / sizeof(dhcp_server_confs[0]);

#endif /* !TA_UNIX_ISC_DHCPS_NATIVE_CFG || __sun__ */

#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG

/**
 * List of known possible locations of DHCP server auxilury
 * configuration file
 */
static const char *dhcp_server_aux_confs[] = {
    "/etc/sysconfig/dhcpd",
    "/etc/default/dhcp3-server"
};

/**
 * Number of known possible locations of DHCP server auxilury
 * configuration file
 */
static unsigned int dhcp_server_n_aux_confs =
    sizeof(dhcp_server_aux_confs) / sizeof(dhcp_server_aux_confs[0]);

#endif /* !TA_UNIX_ISC_DHCPS_NATIVE_CFG */

/** DHCP server script name */
static const char *dhcp_server_script = NULL;

/** DHCP server executable name */
static const char *dhcp_server_exec = NULL;

/** DHCP server configuration file name */
static const char *dhcp_server_conf = NULL;

#if defined __linux__
/** DHCP server leases database file name */
static const char *dhcp_server_leases = NULL;
#endif

/** DHCP server interfaces */
static char *dhcp_server_ifs = NULL;


#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__

/** Index of the DHCP server configuration file backup */
static int dhcp_server_conf_backup = -1;

#endif /* !TA_UNIX_ISC_DHCPS_NATIVE_CFG || __sun__ */

#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG

/** DHCP server auxilury configuration file name */
static const char *dhcp_server_aux_conf = NULL;

/** Index of the DHCP server auxilury configuration file backup */
static int dhcp_server_aux_conf_backup = -1;

#else

/** Was DHCP server enabled at TA start up? */
static te_bool dhcp_server_was_run = FALSE;

#endif

#define DHCP_SERVER_INIT_CHECK \
    do {                                                                    \
        int rc;                                                             \
        if (!dhcp_server_initialised)                                       \
        {                                                                   \
            if ((rc = dhcpserver_init()) != 0)                              \
            {                                                               \
                ERROR("Failed to initialise dhcpserver structures, rc=%r",  \
                      rc);                                                  \
                return rc;                                                  \
            }                                                               \
            dhcp_server_initialised = TRUE;                                 \
        }                                                                   \
    } while (0)

/** DHCP server admin status */
static te_bool dhcp_server_initialised = FALSE;

/** DHCP server admin status */
static te_bool dhcp_server_started = FALSE;

/** Changed flag for DHCP server configuration */
static te_bool dhcp_server_changed = FALSE;


/** Auxiliary buffer */
static char buf[2048];

#if defined __linux__
/**
 * List of options, which should be quoted automatilally; for other
 * option quotes should be specified in value, if necessary.
 */
static char *isc_dhcp_quoted_options[] = {
    "bootfile-name",
    "domain-name",
    "extension-path-name",
    "merit-dump",
    "nis-domain",
    "nisplus-domain",
    "root-path",
    "uap-servers",
    "tftp-server-name",
    "uap-servers",
    "fqdn.fqdn"
};
#endif

/** Release all memory allocated for option structure */
#define FREE_OPTION(_opt) \
    do {                        \
        free(_opt->name);       \
        free(_opt->value);      \
        free(_opt);             \
    } while (0)

static TAILQ_HEAD(te_dhcp_server_subnets, te_dhcp_server_subnet) subnets;

static host *hosts = NULL;

static group *groups = NULL;

#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
static FILE *f = NULL;  /* Pointer to opened /etc/dhcpd.conf */
#endif

static te_errno dhcpserver_init(void);

#if defined __linux__
/* Check, if the option should be quoted */
static te_bool
is_quoted(const char *opt_name)
{
    unsigned int i;

    for (i = 0; i < sizeof(isc_dhcp_quoted_options) / sizeof(char *); i++)
        if (strcmp(opt_name, isc_dhcp_quoted_options[i]) == 0)
            return TRUE;

    return FALSE;
}
#endif

/* Find the host in the list */
static host *
find_host(const char *name)
{
    host *h;

    for (h = hosts; h != NULL && strcmp(h->name, name) != 0; h = h->next);

    return h;
}

/* Find the group in the list */
static group *
find_group(const char *name)
{
    group *g;

    for (g = groups; g != NULL && strcmp(g->name, name) != 0; g = g->next);

    return g;
}

/* Find the option in specified options list */
static te_dhcp_option *
find_option(te_dhcp_option *opt, const char *name)
{
    for (; opt != NULL && strcmp(opt->name, name) != 0; opt = opt->next);

    return opt;
}

/* Release all memory allocated for host structure */
static void
free_host(host *h)
{
    te_dhcp_option *opt, *next;

    free(h->name);
    free(h->chaddr);
    free(h->client_id);
    free(h->ip_addr);
    free(h->next_server);
    free(h->filename);
    free(h->flags);
    for (opt = h->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
}

static void
free_group(group * g)
{
    te_dhcp_option *opt, *next;

    free(g->name);
    free(g->next_server);
    free(g->filename);
    for (opt = g->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
}

/** Save configuration to the file */
static int
ds_dhcpserver_save_conf(void)
{
#if defined __sun__
    int                    rc;
    char                   *p = dhcp_server_ifs;
#endif
    te_dhcp_server_subnet  *s;
    host                   *h;
#if defined __linux__
    te_dhcp_option         *opt;
#endif
    FILE                   *f = fopen(dhcp_server_conf, "w");

    INFO("%s()", __FUNCTION__);

    if (f == NULL)
    {
        ERROR("Failed to open '%s' for writing: %s",
              dhcp_server_conf, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
#if defined __sun__
    ds_config_touch(dhcp_server_conf_backup);

    if (p != NULL)
        for (p = strchr(p, ' '); p != NULL; p = strchr(p, ' '))
          *p = ',';

    fprintf(f,
            "BOOTP_COMPAT=automatic\n"
            "DAEMON_ENABLED=TRUE\n"
            "RESOURCE=SUNWbinfiles\n"
            "RUN_MODE=server\n"
            "PATH=/var/mydhcp\n" /** FIXME */
            "CONVER=1\n"
            "INTERFACES=%s\n", dhcp_server_ifs != NULL ? dhcp_server_ifs : "");

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

    ta_system("rm -f /var/mydhcp/*"); /** FIXME */
#endif

#if defined __linux__
    fprintf(f, "deny unknown-clients;\n\n");
    fprintf(f, "\n");
#endif
    TAILQ_FOREACH(s, &subnets, links)
    {
        struct in_addr  mask;

        mask.s_addr = htonl(PREFIX2MASK(s->prefix_len));
#if defined __linux__
        fprintf(f, "subnet %s netmask %s {\n",
                s->subnet, inet_ntop(AF_INET, &mask, buf, sizeof(buf)));
#elif defined __sun__
        TE_SPRINTF(buf, "/usr/sbin/pntadm -C %s", s->subnet);
        if ((rc = ta_system(buf)) != 0)
            return rc;

        /* FIXME ('/etc/inet/netmasks' must be maintained) */
//        add_xxx(s->subnet, inet_ntop(AF_INET, &mask, buf, sizeof(buf)));
#endif
#if defined __linux__
        if (s->range != NULL)
        {
            fprintf(f, "\trange %s;\n", s->range);
        }
        for (opt = s->options; opt != NULL; opt = opt->next)
        {
            te_bool quoted = is_quoted(opt->name);

            fprintf(f, "\toption %s %s%s%s;\n", opt->name,
                    quoted ? "\"" : "", opt->value, quoted ? "\"" : "");
        }
        fprintf(f, "}\n");
#endif
    }

#if defined __linux__
    fprintf(f, "\n");
#endif
    for (h = hosts; h != NULL; h = h->next)
    {
#if defined __linux__
        fprintf(f, "host %s {\n", h->name);
        if (h->chaddr)
            fprintf(f, "\thardware ethernet %s;\n", h->chaddr);
        if (h->client_id)
            fprintf(f, "\tclient-id %s;\n", h->client_id);
#endif
        if (h->ip_addr)
#if defined __linux__
            fprintf(f, "\tfixed-address %s;\n", h->ip_addr);
#elif defined __sun__
        {
            char *p;

            TE_SPRINTF(buf, "pntadm -f %s -A %s %s", h->flags, h->ip_addr, h->ip_addr);
            if ((p = strrchr(buf, '.')) != NULL)
            {
                p[1] = '0';
                p[2] = '\0';
            }
            if ((rc = ta_system(buf)) != 0)
                return rc;
        }
#endif

#if defined __linux__
        if (h->next_server)
            fprintf(f, "\tnext-server %s;\n", h->next_server);
        if (h->filename)
            fprintf(f, "\tfilename \"%s\";\n", h->filename);
        for (opt = h->options; opt != NULL; opt = opt->next)
        {
            te_bool quoted = is_quoted(opt->name);

            fprintf(f, "\toption %s %s%s%s;\n", opt->name,
                    quoted ? "\"" : "", opt->value, quoted ? "\"" : "");
        }
        fprintf(f, "}\n");
#endif
    }

#if defined __linux__
    fprintf(f, "\n");
#endif

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

/** Is DHCP server daemon running */
static te_bool
ds_dhcpserver_is_run(void)
{
    int     rc = 0;
    char   *name = NULL;

#if defined __linux__
    if ((name = strrchr(dhcp_server_exec, '/')) == NULL)
        name = (char *)dhcp_server_exec;
    else
        name++;
    sprintf(buf, PS_ALL_COMM "| grep -v grep | grep -q %s >/dev/null 2>&1",
            name);
#elif defined __sun__
    TE_SPRINTF(buf, "[ \"`/usr/bin/svcs -H -o STATE dhcp-server`\" = \"online\" ]");
#else
    return FALSE;
#endif

    rc = ta_system(buf);

    INFO("%s() returns %s", __FUNCTION__, (rc == 0) ? "TRUE" : "FALSE");

    return (rc == 0);
}

/** Get DHCP server daemon on/off */
static te_errno
ds_dhcpserver_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    strcpy(value, ds_dhcpserver_is_run() ? "1" : "0");

    return 0;
}

/** Stop DHCP server using script from /etc/init.d */
static te_errno
ds_dhcpserver_script_stop(void)
{
    INFO("%s()", __FUNCTION__);
    TE_SPRINTF(buf, "%s stop", dhcp_server_script);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/** Stop DHCP server */
static te_errno
ds_dhcpserver_stop(void)
{
    int     rc = 0;
    char   *name = NULL;

    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

#if defined __linux__
    if ((name = strrchr(dhcp_server_exec, '/')) == NULL)
        name = (char *)dhcp_server_exec;
    else
        name++;
    TE_SPRINTF(buf, "killall %s", name);
#elif defined __sun__
    TE_SPRINTF(buf, "/usr/sbin/svcadm disable -st %s", get_ds_name("dhcpserver"));
#endif
    rc = ta_system(buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    {
        ERROR("Command '%s' failed, rc=%r", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}


/** Start DHCP server using script from /etc/init.d */
static te_errno
ds_dhcpserver_script_start(void)
{
    INFO("%s()", __FUNCTION__);
    TE_SPRINTF(buf, "%s start", dhcp_server_script);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/** Start DHCP server */
static te_errno
ds_dhcpserver_start(void)
{
    te_errno rc;

    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

    rc = ds_dhcpserver_save_conf();
    if (rc != 0)
    {
        ERROR("Failed to save DHCP server configuration file");
        return rc;
    }

#if defined __linux__

    TE_SPRINTF(buf, "%s -t -cf %s",
               dhcp_server_exec, dhcp_server_conf);
    if (ta_system(buf) != 0)
    {
        ERROR("Configuration file verification failed, command '%s'", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    TE_SPRINTF(buf, "%s -T -lf %s",
               dhcp_server_exec, dhcp_server_leases);
    if (ta_system(buf) != 0)
    {
        ERROR("Leases database verification failed, command '%s'", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    TE_SPRINTF(buf, "%s -cf %s -lf %s %s",
               dhcp_server_exec, dhcp_server_conf,
               dhcp_server_leases, dhcp_server_ifs ? : "");

#elif defined __sun__

    TE_SPRINTF(buf, "/usr/sbin/svcadm disable -st %s", get_ds_name("dhcpserver"));
    if (ta_system(buf) != 0)
    {
        ERROR("Failed to stop DHCP server, command '%s'", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    TE_SPRINTF(buf, "/usr/sbin/svcadm enable -rst %s", get_ds_name("dhcpserver"));

#else

    ERROR("DHCP server configuration is not supported");

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);

#endif

    if (ta_system(buf) != 0)
    {
        ERROR("Failed to start DHCP server, command '%s'", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Set desired status of DHCP server
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         desired dhcp server status
 *
 * @return status code
 */
static te_errno
ds_dhcpserver_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s(): value=%s", __FUNCTION__, value);

    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    dhcp_server_started = (strcmp(value, "1") == 0);
    if (dhcp_server_started != ds_dhcpserver_is_run())
    {
        dhcp_server_changed = TRUE;
    }

    return 0;
}


/** On/off DHCP server */
static te_errno
ds_dhcpserver_commit(unsigned int gid, const char *oid)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    /*
     * We don't need to change state of DHCP Server:
     * The current state is the same as desired.
     */
    if (!dhcp_server_changed)
        return 0;

    /* Stop DHCP server */
    if (ds_dhcpserver_is_run())
    {
#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
        rc = ds_dhcpserver_script_stop();
#else
        rc = ds_dhcpserver_stop();
#endif
        if (rc != 0)
        {
            ERROR("Failed to stop DHCP server");
            return rc;
        }
    }

    /* (Re)start DHCP server, if required */
    if (dhcp_server_started)
    {
#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
        rc = ds_dhcpserver_script_start();
#else
        rc = ds_dhcpserver_start();
#endif
        if (rc != 0)
        {
            ERROR("Failed to start DHCP server");
            return rc;
        }
    }

    dhcp_server_changed = FALSE;

    return rc;
}

/** Get DHCP server interfaces */
static te_errno
ds_dhcpserver_ifs_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    strcpy(value, dhcp_server_ifs ? : "");

    return 0;
}

/** Get DHCP server interfaces */
static te_errno
ds_dhcpserver_ifs_set(unsigned int gid, const char *oid, const char *value)
{
    char *copy;

    UNUSED(gid);
    UNUSED(oid);

    /* TODO Check value */
    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    copy = strdup(value);
    if (copy == NULL)
    {
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, value);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    free(dhcp_server_ifs);
    dhcp_server_ifs = copy;

    dhcp_server_changed = TRUE;

    return 0;
}


static void
free_subnet(te_dhcp_server_subnet *s)
{
    te_dhcp_option *opt, *next;

    free(s->subnet);
    free(s->range);
    for (opt = s->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
    free(s);
}

static te_dhcp_server_subnet *
find_subnet(const char *subnet)
{
    te_dhcp_server_subnet  *s;

    for (s = TAILQ_FIRST(&subnets);
         s != NULL && strcmp(s->subnet, subnet) != 0;
         s = TAILQ_NEXT(s, links));

    return s;
}

static te_errno
ds_subnet_get(unsigned int gid, const char *oid, char *value,
              const char *dhcpserver, const char *subnet)
{
    te_dhcp_server_subnet  *s;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((s = find_subnet(subnet)) == NULL)
        return TE_OS_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%d", s->prefix_len);

    return 0;
}

static te_errno
ds_subnet_set(unsigned int gid, const char *oid, const char *value,
              const char *dhcpserver, const char *subnet)
{
    te_dhcp_server_subnet  *s;
    int                     prefix_len;
    char                   *end;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((s = find_subnet(subnet)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    prefix_len = strtol(value, &end, 10);
    if (value == end || *end != '\0')
        return TE_RC(TE_TA_UNIX, TE_EFMT);

    s->prefix_len = prefix_len;

    dhcp_server_changed = TRUE;

    return 0;
}

static te_errno
ds_subnet_add(unsigned int gid, const char *oid, const char *value,
              const char *dhcpserver, const char *subnet)
{
    te_dhcp_server_subnet  *s;
    int                     prefix_len;
    char                   *end;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

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
    s->range = NULL;

    TAILQ_INSERT_TAIL(&subnets, s, links);

    dhcp_server_changed = TRUE;

    return 0;
}

static te_errno
ds_subnet_del(unsigned int gid, const char *oid,
              const char *dhcpserver, const char *subnet)
{
    te_dhcp_server_subnet  *s;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((s = find_subnet(subnet)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&subnets, s, links);
    free_subnet(s);

    dhcp_server_changed = TRUE;

    return 0;
}

static te_errno
ds_subnet_list(unsigned int gid, const char *oid, char **list)
{
    te_dhcp_server_subnet  *s;

    UNUSED(gid);
    UNUSED(oid);

    DHCP_SERVER_INIT_CHECK;

    *buf = '\0';
    TAILQ_FOREACH(s, &subnets, links)
    {
        sprintf(buf + strlen(buf), "%s ",  s->subnet);
    }

    return (*list = strdup(buf)) == NULL ?
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;
}


/** Definition of list method for host and groups */
#define LIST_METHOD(_gh) \
static te_errno \
ds_##_gh##_list(unsigned int gid, const char *oid, char **list) \
{                                                               \
    _gh *gh;                                                    \
                                                                \
    UNUSED(oid);                                                \
    UNUSED(gid);                                                \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    *buf = '\0';                                                \
                                                                \
    for (gh = _gh##s; gh != NULL; gh = gh->next)                \
    {                                                           \
        sprintf(buf + strlen(buf), "%s ",  gh->name);           \
    }                                                           \
                                                                \
    return (*list = strdup(buf)) == NULL ?                      \
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;                \
}

LIST_METHOD(host)
LIST_METHOD(group)

/** Definition of add method for host and groups */
#define ADD_METHOD(_gh) \
static te_errno \
ds_##_gh##_add(unsigned int gid, const char *oid, const char *value,    \
               const char *dhcpserver, const char *name)                \
{                                                                       \
    _gh *gh;                                                            \
                                                                        \
    UNUSED(gid);                                                        \
    UNUSED(oid);                                                        \
    UNUSED(dhcpserver);                                                 \
    UNUSED(value);                                                      \
                                                                        \
    DHCP_SERVER_INIT_CHECK;                                             \
                                                                        \
    if ((gh = find_##_gh(name)) != NULL)                                \
        return TE_RC(TE_TA_UNIX, TE_EEXIST);                            \
                                                                        \
    if ((gh = (_gh *)calloc(1, sizeof(_gh))) == NULL)                   \
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);                            \
                                                                        \
    if ((gh->name = strdup(name)) == NULL)                              \
    {                                                                   \
        free(gh);                                                       \
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);                            \
    }                                                                   \
                                                                        \
    gh->next = _gh##s;                                                  \
    _gh##s = gh;                                                        \
                                                                        \
    dhcp_server_changed = TRUE;                                         \
                                                                        \
    return 0;                                                           \
}

ADD_METHOD(host)
ADD_METHOD(group)

/** Definition of delete method for host and groups */
#define DEL_METHOD(_gh) \
static te_errno \
ds_##_gh##_del(unsigned int gid, const char *oid,       \
               const char *dhcpserver, const char *name)\
{                                                       \
    _gh *gh;                                            \
    _gh *prev;                                          \
                                                        \
    UNUSED(gid);                                        \
    UNUSED(oid);                                        \
    UNUSED(dhcpserver);                                 \
                                                        \
    DHCP_SERVER_INIT_CHECK;                             \
                                                        \
    for (gh = _gh##s, prev = NULL;                      \
         gh != NULL && strcmp(gh->name, name) != 0;     \
         prev = gh, gh = gh->next);                     \
                                                        \
    if (gh == NULL)                                     \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);            \
                                                        \
    if (prev)                                           \
        prev->next = gh->next;                          \
    else                                                \
        _gh##s = gh->next;                              \
    free_##_gh(gh);                                     \
                                                        \
    dhcp_server_changed = TRUE;                         \
                                                        \
    return 0;                                           \
}

DEL_METHOD(host)
DEL_METHOD(group)

/** Obtain the group of the host */
static te_errno
ds_host_group_get(unsigned int gid, const char *oid, char *value,
                  const char *dhcpserver, const char *name)
{
    host *h;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((h = find_host(name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (h->group == NULL)
        *value = 0;
    else
        strcpy(value, h->group->name);

    return 0;
}

/** Change the group of the host */
static te_errno
ds_host_group_set(unsigned int gid, const char *oid, const char *value,
                  const char *dhcpserver, const char *name)
{
    host  *h;
    group *old;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((h = find_host(name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    old = h->group;
    if (*value == 0)
        h->group = NULL;
    else if ((h->group = find_group(value)) == NULL)
    {
        h->group = old;
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    dhcp_server_changed = TRUE;

    return 0;
}

/*
 * Definition of the routines for obtaining/changing of
 * host/group/subnet attributes (except options).
 */
#define ATTR_GET(_attr, _ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_##_attr##_get(unsigned int gid, const char *oid,    \
                          char *value, const char *dhcpserver,  \
                          const char *name)                     \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if (ghs->_attr == NULL)                                     \
        *value = 0;                                             \
    else                                                        \
        strcpy(value, ghs->_attr);                              \
                                                                \
    return 0;                                                   \
}

#define SSS(S) #S

#define ATTR_SET(_attr, _ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_##_attr##_set(unsigned int gid, const char *oid,    \
                          const char *value,                    \
                          const char *dhcpserver,               \
                          const char *name)                     \
{                                                               \
    _ghs_type  *ghs;                                            \
    char *old_val;                                              \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    old_val = ghs->_attr;                                       \
                                                                \
    if (*value == 0)                                            \
        ghs->_attr = NULL;                                      \
    else                                                        \
    {                                                           \
        if ((ghs->_attr = strdup(value)) == NULL)               \
        {                                                       \
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);                \
        }                                                       \
    }                                                           \
                                                                \
    free(old_val);                                              \
                                                                \
    dhcp_server_changed = TRUE;                                 \
                                                                \
    return 0;                                                   \
}

ATTR_GET(chaddr, host, host)
ATTR_SET(chaddr, host, host)
ATTR_GET(client_id, host, host)
ATTR_SET(client_id, host, host)
ATTR_GET(ip_addr, host, host)
ATTR_SET(ip_addr, host, host)
ATTR_GET(next_server, host, host)
ATTR_SET(next_server, host, host)
ATTR_GET(filename, host, host)
ATTR_SET(filename, host, host)
ATTR_GET(flags, host, host)
ATTR_SET(flags, host, host)
ATTR_GET(next_server, group, group)
ATTR_SET(next_server, group, group)
ATTR_GET(filename, group, group)
ATTR_SET(filename, group, group)
ATTR_GET(range, subnet, te_dhcp_server_subnet)
ATTR_SET(range, subnet, te_dhcp_server_subnet)

/**
 * Definition of the method for obtaining of options list
 * for the host/group/subnet.
 */
#define GET_OPT_LIST(_ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_option_list(unsigned int gid, const char *oid,      \
                       char **list, const char *dhcpserver,     \
                       const char *name)                        \
{                                                               \
   _ghs_type *ghs;                                              \
                                                                \
    te_dhcp_option *opt;                                        \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    *buf = 0;                                                   \
    for (opt = ghs->options; opt != NULL; opt = opt->next)      \
    {                                                           \
        sprintf(buf + strlen(buf), "%s ", opt->name);           \
    }                                                           \
                                                                \
    return (*list = strdup(buf)) == NULL ?                      \
              TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;                 \
}

GET_OPT_LIST(host, host)
GET_OPT_LIST(group, group)
GET_OPT_LIST(subnet, te_dhcp_server_subnet)

/* Method for adding of the option for group/host/subnet */
#define ADD_OPT(_ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_option_add(unsigned int gid, const char *oid,       \
                      const char *value, const char *dhcpserver,\
                      const char *name, const char *optname)    \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    te_dhcp_option *opt;                                        \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if (*value == 0)                                            \
        return TE_RC(TE_TA_UNIX, TE_EINVAL);                    \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if (find_option(ghs->options, optname) != NULL)             \
        return TE_RC(TE_TA_UNIX, TE_EEXIST);                    \
                                                                \
    if ((opt = (te_dhcp_option *)calloc(sizeof(*opt), 1))       \
        == NULL || (opt->name = strdup(optname)) == NULL ||     \
        (opt->value = strdup(value)) == NULL)                   \
    {                                                           \
        FREE_OPTION(opt);                                       \
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);                    \
    }                                                           \
                                                                \
    opt->next = ghs->options;                                   \
    ghs->options = opt;                                         \
                                                                \
    dhcp_server_changed = TRUE;                                 \
                                                                \
    return 0;                                                   \
}

ADD_OPT(host, host)
ADD_OPT(group, group)
ADD_OPT(subnet, te_dhcp_server_subnet)

/* Method for obtaining of the option for group/host/subnet */
#define GET_OPT(_ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_option_get(unsigned int gid, const char *oid,       \
                      char *value, const char *dhcpserver,      \
                      const char *name, const char *optname)    \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    te_dhcp_option *opt;                                        \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if ((opt = find_option(ghs->options, optname)) == NULL)     \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    strcpy(value, opt->value);                                  \
                                                                \
    return 0;                                                   \
}

GET_OPT(host, host)
GET_OPT(group, group)
GET_OPT(subnet, te_dhcp_server_subnet)

/* Method for changing of the option value for group/host/subnet */
#define SET_OPT(_ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_option_set(unsigned int gid, const char *oid,       \
                      const char *value, const char *dhcpserver,\
                      const char *name, const char *optname)    \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    char *old;                                                  \
                                                                \
    te_dhcp_option *opt;                                        \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if ((opt = find_option(ghs->options, optname)) == NULL)     \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    old = opt->value;                                           \
    if (*value == 0)                                            \
        opt->value = NULL;                                      \
    else                                                        \
    {                                                           \
        if ((opt->value = strdup(value)) == NULL)               \
        {                                                       \
            opt->value = old;                                   \
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);                \
        }                                                       \
    }                                                           \
                                                                \
    free(old);                                                  \
                                                                \
    dhcp_server_changed = TRUE;                                 \
                                                                \
    return 0;                                                   \
}

SET_OPT(host, host)
SET_OPT(group, group)
SET_OPT(subnet, te_dhcp_server_subnet)


/* Method for deletion of the option value for group/host/subnet */
#define DEL_OPT(_ghs, _ghs_type) \
static te_errno      \
ds_##_ghs##_option_del(unsigned int gid, const char *oid,       \
                      const char *dhcpserver, const char *name, \
                      const char *optname)                      \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    te_dhcp_option *opt, *prev;                                 \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    for (opt = ghs->options, prev = NULL;                       \
         opt != NULL && strcmp(opt->name, optname) != 0;        \
         prev = opt, opt = opt->next);                          \
                                                                \
    if (opt == NULL)                                            \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if (prev)                                                   \
        prev->next = opt->next;                                 \
    else                                                        \
        ghs->options = opt->next;                               \
                                                                \
    FREE_OPTION(opt);                                           \
                                                                \
    dhcp_server_changed = TRUE;                                 \
                                                                \
    return 0;                                                   \
}

DEL_OPT(host, host)
DEL_OPT(group, group)
DEL_OPT(subnet, te_dhcp_server_subnet)

#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED

#define ADDR_LIST_BULK      128

/** Obtain the list of leases */
static te_errno
ds_lease_list(unsigned int gid, const char *oid, char **list)
{
    FILE *f;
    char *name;
    int   len = ADDR_LIST_BULK;

    UNUSED(gid);
    UNUSED(oid);

    DHCP_SERVER_INIT_CHECK;

    if ((f = fopen("/var/lib/dhcp/dhcpd.leases", "r")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if ((*list = (char *)malloc(ADDR_LIST_BULK)) == NULL)
    {
        fclose(f);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    **list = 0;

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strncmp(buf, "lease", strlen("lease")) != 0)
            continue;

        name = buf + strlen("lease") + 1;
        *(strchr(name, ' ') + 1) = 0;
        if (strstr(*list, name) != NULL)
            continue;

        if (strlen(*list) + strlen(name) >= len)
        {
            char *tmp;

            len += ADDR_LIST_BULK;
            if ((tmp = (char *)realloc(*list, len)) == NULL)
            {
                free(*list);
                fclose(f);
                return TE_OS_RC(TE_TA_UNIX, TE_ENOMEM);
            }
            *list = tmp;
        }
        strcat(*list, name);
    }
    fclose(f);

    return 0;
}

/** Open lease object with specified IP address */
static int
open_lease(const char *name)
{
    dhcpctl_data_string ip = NULL;

    isc_result_t rc;
    unsigned int addr;

    if (conn == NULL)
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if (inet_aton(name, (struct in_addr *)&addr) == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    omapi_data_string_new(&ip, sizeof(addr), MDL);
    memcpy(ip->value, &addr, sizeof(addr));
    CHECKSTATUS(dhcpctl_set_value(lo, ip, "ip-address"));
    CHECKSTATUS(dhcpctl_open_object(lo, conn, 0));
    CHECKSTATUS(dhcpctl_wait_for_completion(lo, &rc));
    CHECKSTATUS(rc);
    dhcpctl_data_string_dereference(&ip, MDL);

    return 0;
}

/** Get lease node */
static te_errno
ds_lease_get(unsigned int gid, const char *oid, char *value,
             const char *dhcpserver, const char *name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    return open_lease(name);
}

/* Body for methods returning integer lease attributes */
#define GET_INT_LEASE_ATTR(_attr) \
{                                                       \
    int rc;                                             \
    int res;                                            \
                                                        \
    dhcpctl_data_string val;                            \
                                                        \
    UNUSED(gid);                                        \
    UNUSED(oid);                                        \
    UNUSED(dhcpserver);                                 \
                                                        \
    if ((rc = open_lease(name)) != 0)                   \
        return rc;                                      \
                                                        \
    CHECKSTATUS(dhcpctl_get_value(&val, lo, _attr));    \
    memcpy(&res, val->value, val->len);                 \
    res = ntohl(res);                                   \
    sprintf(value, "%d", *(int *)(val->value));         \
    dhcpctl_data_string_dereference(&val, MDL);         \
                                                        \
    return 0;                                           \
}

/** Get lease state */
static te_errno
ds_lease_state_get(unsigned int gid, const char *oid, char *value,
                   const char *dhcpserver, const char *name)
GET_INT_LEASE_ATTR("state")

/** Get lease client identifier */
static te_errno
ds_lease_client_id_get(unsigned int gid, const char *oid, char *value,
                       const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((rc = open_lease(name)) != 0)
        return rc;

    /*
     * Very bad hack to know the type of the particular
     * dhcp-client-identifier: srting or binary.
     */
    {
        omapi_value_t *tv = (omapi_value_t *)0;

        CHECKSTATUS(omapi_get_value_str(lo, (omapi_object_t *)0,
                    "dhcp-client-identifier", &tv));

        if (tv->value->type == omapi_datatype_string)
        {
            sprintf(value, "%.*s", tv->value->u.buffer.value,
                    tv->value->u.buffer.len);
        }
        else
        {
            int i;

            sprintf(value, "\"");
            for (i = 0; i < tv->value->u.buffer.len; i++)
                sprintf(value + strlen(value), "%02x%s",
                        ((unsigned char *)(tv->value->u.buffer.value))[i],
                        i == tv->value->u.buffer.len - 1 ? "\"" : ":");
        }

        omapi_value_dereference (&tv, MDL);
    }

    return 0;
}

/** Get lease hostname */
static te_errno
ds_lease_hostname_get(unsigned int gid, const char *oid, char *value,
                      const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((rc = open_lease(name)) != 0)
        return rc;

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "client-hostname"));

    memcpy(value, val->value, val->len);
    *(value + val->len) = 0;

    dhcpctl_data_string_dereference(&val, MDL);

    return 0;
}

/** Get lease IP address */
static te_errno
ds_lease_host_get(unsigned int gid, const char *oid, char *value,
                  const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((rc = open_lease(name)) != 0)
        return rc;

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "host"));

    /* It's not clear what to do next :) */

    return 0;
}

/** Get lease MAC address */
static te_errno
ds_lease_chaddr_get(unsigned int gid, const char *oid, char *value,
                    const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;
    unsigned char      *mac;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((rc = open_lease(name)) != 0)
        return rc;

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "hardware-address"));

    mac = (unsigned char *)(val->value);
    sprintf(value, "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    dhcpctl_data_string_dereference(&val, MDL);

    return 0;
}

/* Get lease attributes */

static te_errno
ds_lease_ends_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("ends")

static te_errno
ds_lease_tstp_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("tstp")

static te_errno
ds_lease_cltt_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("cltt")

#endif /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */


#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
RCF_PCH_CFG_NODE_RO(node_ds_lease_cltt, "cltt",
                    NULL, NULL,
                    ds_lease_cltt_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_tstp, "tstp",
                    NULL, &node_ds_lease_cltt,
                    ds_lease_tstp_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_ends, "ends",
                    NULL, &node_ds_lease_tstp,
                    ds_lease_ends_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_chaddr, "chaddr",
                    NULL, &node_ds_lease_ends,
                    ds_lease_chaddr_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_host, "host",
                    NULL, &node_ds_lease_chaddr,
                    ds_lease_host_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_hostname, "hostname",
                    NULL, &node_ds_lease_host,
                    ds_lease_hostname_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_client_id, "client_id",
                    NULL, &node_ds_lease_hostname,
                    ds_lease_client_id_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_state, "state",
                    NULL, &node_ds_lease_client_id,
                    ds_lease_state_get);

static rcf_pch_cfg_object node_ds_lease =
    { "lease", 0, &node_ds_lease_state, &node_ds_host,
      (rcf_ch_cfg_get)ds_lease_get, NULL, NULL, NULL,
      (rcf_ch_cfg_list)ds_lease_list, NULL, NULL};

static rcf_pch_cfg_object node_ds_client_lease =
    { "lease", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_client_lease_get, NULL, NULL, NULL,
      (rcf_ch_cfg_list)ds_client_lease_list, NULL, NULL };

static rcf_pch_cfg_object node_ds_client =
    { "client", 0, &node_ds_client_lease, &node_ds_lease,
      (rcf_ch_cfg_get)ds_client_get, NULL, NULL, NULL,
      (rcf_ch_cfg_list)ds_client_list, NULL, NULL };

#endif /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */

static rcf_pch_cfg_object node_ds_group_option =
    { "option", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_group_option_get,
      (rcf_ch_cfg_set)ds_group_option_set,
      (rcf_ch_cfg_add)ds_group_option_add,
      (rcf_ch_cfg_del)ds_group_option_del,
      (rcf_ch_cfg_list)ds_group_option_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_group_file, "file",
                    NULL, &node_ds_group_option,
                    ds_group_filename_get, ds_group_filename_set);

RCF_PCH_CFG_NODE_RW(node_ds_group_next, "next",
                    NULL, &node_ds_group_file,
                    ds_group_next_server_get, ds_group_next_server_set);

#if TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
RCF_PCH_CFG_NODE_COLLECTION(node_ds_group, "group",
                            &node_ds_group_next, &node_ds_client,
                            ds_group_add, ds_group_del,
                            ds_group_list, NULL);
#else
RCF_PCH_CFG_NODE_COLLECTION(node_ds_group, "group",
                            &node_ds_group_next, NULL,
                            ds_group_add, ds_group_del,
                            ds_group_list, NULL);
#endif

static rcf_pch_cfg_object node_ds_host_option =
    { "option", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_host_option_get,
      (rcf_ch_cfg_set)ds_host_option_set,
      (rcf_ch_cfg_add)ds_host_option_add,
      (rcf_ch_cfg_del)ds_host_option_del,
      (rcf_ch_cfg_list)ds_host_option_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_host_flags, "flags",
                    NULL, &node_ds_host_option,
                    ds_host_flags_get, ds_host_flags_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_file, "file",
                    NULL, &node_ds_host_flags,
                    ds_host_filename_get, ds_host_filename_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_next, "next",
                    NULL, &node_ds_host_file,
                    ds_host_next_server_get, ds_host_next_server_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_ip_addr, "ip-address",
                    NULL, &node_ds_host_next,
                    ds_host_ip_addr_get, ds_host_ip_addr_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_client_id, "client-id",
                    NULL, &node_ds_host_ip_addr,
                    ds_host_client_id_get, ds_host_client_id_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_chaddr, "chaddr",
                    NULL, &node_ds_host_client_id,
                    ds_host_chaddr_get, ds_host_chaddr_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_group, "group",
                    NULL, &node_ds_host_chaddr,
                    ds_host_group_get, ds_host_group_set);

RCF_PCH_CFG_NODE_COLLECTION(node_ds_host, "host",
                            &node_ds_host_group, &node_ds_group,
                            ds_host_add, ds_host_del,
                            ds_host_list, NULL);

static rcf_pch_cfg_object node_ds_subnet_option =
    { "option", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_subnet_option_get,
      (rcf_ch_cfg_set)ds_subnet_option_set,
      (rcf_ch_cfg_add)ds_subnet_option_add,
      (rcf_ch_cfg_del)ds_subnet_option_del,
      (rcf_ch_cfg_list)ds_subnet_option_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_subnet_range, "range",
                    NULL, &node_ds_subnet_option,
                    ds_subnet_range_get, ds_subnet_range_set);

static rcf_pch_cfg_object node_ds_subnet =
    { "subnet", 0, &node_ds_subnet_range, &node_ds_host,
      (rcf_ch_cfg_get)ds_subnet_get,
      (rcf_ch_cfg_set)ds_subnet_set,
      (rcf_ch_cfg_add)ds_subnet_add,
      (rcf_ch_cfg_del)ds_subnet_del,
      (rcf_ch_cfg_list)ds_subnet_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_dhcpserver_ifs, "interfaces",
                    NULL, &node_ds_subnet,
                    ds_dhcpserver_ifs_get, ds_dhcpserver_ifs_set);

static rcf_pch_cfg_object node_ds_dhcpserver =
    { "dhcpserver", 0,
      &node_ds_dhcpserver_ifs, NULL,
      (rcf_ch_cfg_get)ds_dhcpserver_get,
      (rcf_ch_cfg_set)ds_dhcpserver_set,
      NULL, NULL, NULL,
      (rcf_ch_cfg_commit)ds_dhcpserver_commit, NULL };


static te_errno 
dhcpserver_init(void)
{
    int rc = 0;

    TAILQ_INIT(&subnets);

    /* Find DHCP server executable */
    rc = find_file(dhcp_server_n_execs, dhcp_server_execs, TRUE);
    if (rc < 0)
    {
        ERROR("Failed to find DHCP server executable"
             " - DHCP will not be available");
        rcf_pch_del_node(&node_ds_dhcpserver);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    dhcp_server_exec = dhcp_server_execs[rc];

    /* Find DHCP server script */
    rc = find_file(dhcp_server_n_scripts, dhcp_server_scripts, TRUE);
    if (rc < 0)
    {
        ERROR("Failed to find DHCP server script"
              " - DHCP will not be available");
        rcf_pch_del_node(&node_ds_dhcpserver);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    dhcp_server_script = dhcp_server_scripts[rc];

    /* FIXME (original 'dhcpsvc.conf' gets erased after epilog) */
#if defined __sun__
    if ((rc = ta_system("touch /etc/inet/dhcpsvc.conf")) != 0)
        return rc;
    if ((rc = ta_system("if test ! -d /var/mydhcp; "
                        "then mkdir -p /var/mydhcp; fi")) != 0)
        return rc;
#endif /* !__sun__ */

#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__
    /* Find DHCP server configuration file */
    rc = find_file(dhcp_server_n_confs, dhcp_server_confs, FALSE);
    if (rc < 0)
    {
        ERROR("Failed to find DHCP server configuration file"
             " - DHCP will not be available");
        rcf_pch_del_node(&node_ds_dhcpserver);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    dhcp_server_conf = dhcp_server_confs[rc];
#endif /* !(defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__) */

#if defined __sun__
    rc = ds_create_backup("/etc/inet/", "dhcpsvc.conf", &dhcp_server_conf_backup);
#elif defined __linux__
#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG
    /* Test existing configuration file and leases DB */
    snprintf(buf, sizeof(buf), "%s -q -t -T", dhcp_server_exec);
    if (ta_system(buf) != 0)
    {
        ERROR("Bad found DHCP server configution file '%s'"
             " - DHCP will not be available", dhcp_server_conf);
        rcf_pch_del_node(&node_ds_dhcpserver);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = isc_dhcp_server_cfg_parse(dhcp_server_conf);
    if (rc != 0)
    {
        ERROR("Failed to parse DHCP server configuration file '%s'"
              " - DHCP will not be available", dhcp_server_conf);
        ds_shutdown_dhcp_server();
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#else
    /* FIXME */
    dhcp_server_conf = "/tmp/te.dhcpd.conf";
    dhcp_server_leases = "/tmp/te.dhcpd.leases";
    {
        struct passwd *p;
        
        int f = creat(dhcp_server_leases, 00666);

        if (f < 0)
        {
            ERROR("Failed to open '%s' for writing: %s",
                  dhcp_server_leases, strerror(errno));
            rcf_pch_del_node(&node_ds_dhcpserver);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        close(f);
        p = getpwnam("dhcpd");
        if (p != NULL)
            (void)chown(dhcp_server_leases, p->pw_uid, p->pw_gid);
    }

    if (ds_dhcpserver_is_run())
    {
        rc = ds_dhcpserver_script_stop();
        if (rc != 0)
        {
            ERROR("Failed to stop DHCP server"
                  " - DHCP will not be available");
            rcf_pch_del_node(&node_ds_dhcpserver);
            return rc;
        }
        dhcp_server_was_run = TRUE;
    }
#endif /* TA_UNIX_ISC_DHCPS_NATIVE_CFG */
#endif

    dhcp_server_initialised = TRUE;
    dhcp_server_changed = TRUE;

    return 0;
}

te_errno
dhcpserver_grab(const char *name)
{
    int rc = 0;

    UNUSED(name);

    DHCP_SERVER_INIT_CHECK;

    /* Stop DHCP server */
    if (ds_dhcpserver_is_run())
    {
        WARN("Another DHCP server is running, shutting it down...");
#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
        rc = ds_dhcpserver_script_stop();
#else
        rc = ds_dhcpserver_stop();
#endif
        if (rc != 0)
        {
            ERROR("Failed to stop DHCP server");
            return rc;
        }
    }

    dhcp_server_started = FALSE;
    dhcp_server_changed = FALSE;

    if ((rc = rcf_pch_add_node("/agent", &node_ds_dhcpserver)) != 0)
        return rc;

    return 0;
}

te_errno
dhcpserver_release(const char *name)
{
    te_errno    rc;
    host       *host, *host_tmp;
    group      *group, *group_tmp;

    UNUSED(name);

    if (!dhcp_server_initialised)
        return 0;

    rc = rcf_pch_del_node(&node_ds_dhcpserver);
    if (rc != 0)
        return rc;

    /* Free old lists */
    for (host = hosts; host != NULL; host = host_tmp)
    {
        host_tmp = host->next;
        free_host(host);
    }
    hosts = NULL;

    for (group = groups; group != NULL; group = group_tmp)
    {
        group_tmp = group->next;
        free_group(group);
    }
    groups = NULL;

#if defined __sun__
    ds_restore_backup(dhcp_server_conf_backup);

    /* FIXME */
    if ((rc = ta_system("rm -fr /var/mydhcp")) != 0)
        return rc;
#endif

#ifndef TA_UNIX_ISC_DHCPS_NATIVE_CFG
    if (dhcp_server_was_run)
    {
        if (ds_dhcpserver_is_run())
        {
            WARN("DHCP server was disabled at start up from TE point "
                 "of view, however it is enabled at shutdown. It looks "
                 "like you have configuration rollback issues.");
            (void)ds_dhcpserver_stop();
        }
        if (ds_dhcpserver_script_start() != 0)
        {
            ERROR("Failed to start DHCP server on rollback"
                 " - DHCP server will not be available");
        }
        dhcp_server_was_run = FALSE;
    }

    if (dhcp_server_conf != NULL && unlink(dhcp_server_conf) != 0 &&
        errno != ENOENT)
    {
        ERROR("Failed to delete DHCP server temporary configuration "
              "file '%s': %s", dhcp_server_conf, strerror(errno));
    }

#if defined __linux__
    if (dhcp_server_leases != NULL && unlink(dhcp_server_leases) != 0)
    {
        ERROR("Failed to delete DHCP server temporary leases data base "
              "file '%s': %s", dhcp_server_leases, strerror(errno));
    }
#endif

#endif

    return 0;
}

#endif /* WITH_DHCP_SERVER */
