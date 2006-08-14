/** @file
 * @brief Unix Test Agent
 *
 * DNS server configuring 
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef WITH_DNS_SERVER

#include "conf_daemons.h"
#include <limits.h>

#define NAMED_CONF           "named.conf"

static int     dns_index;
static te_bool dns_recursive;
static te_bool named_conf_was_running;
static char    dns_forwarder[64] = "0.0.0.0";
static char    dns_directory[PATH_MAX];


static int
dns_update_config(void)
{   
    FILE *config;

    ds_config_touch(dns_index);
    config = fopen(ds_config(dns_index), "w");
    if (config == NULL)
    {
        int rc = errno;
        ERROR("Cannot open '%s' for writing: %s", strerror(rc));
        return rc;
    }
    
    fputs("/* Autogenerated by TE*/\noptions {\n", config);
    if (*dns_directory != '\0')
    {
        fprintf(config, "\tdirectory \"%s\";\n", dns_directory);
    }
    if (*dns_forwarder != '\0')
    {
        fprintf(config, "\tforwarders { %s; };\n", dns_forwarder);
    }
    fprintf(config, "\trecursion %s;\n};\n", dns_recursive ? "yes" : "no");

    fsync(fileno(config));
    fclose(config);

    if (daemon_running("dnsserver"))
    {
        daemon_set(0, "dnsserver", "0");
        daemon_set(0, "dnsserver", "1");
    }
    return 0;
}


static te_errno
ds_dns_forwarder_get(unsigned int gid, const char *oid,
                     char *value, const char *instN, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instN);
    if (*dns_forwarder == '\0')
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    strcpy(value, dns_forwarder);
    return 0;
}

static te_errno
ds_dns_forwarder_set(unsigned int gid, const char *oid,
                     const char *value, const char *instN, ...)
{
    if (named_conf_was_running)
    {
        WARN("DNS server was running");
        return TE_RC(TE_TA_UNIX, TE_ENOSYS);
    }

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instN);
    strcpy(dns_forwarder, value);
    return dns_update_config();
}

static te_errno
ds_dns_directory_get(unsigned int gid, const char *oid,
                     char *value, const char *instN, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instN);
    strcpy(value, dns_directory);
    return 0;
}

static te_errno
ds_dns_directory_set(unsigned int gid, const char *oid,
                     const char *value, const char *instN, ...)
{
    if (named_conf_was_running)
    {
        WARN("DNS server was running");
        return TE_RC(TE_TA_UNIX, TE_ENOSYS);
    }

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instN);
    strcpy(dns_directory, value);
    return dns_update_config();
}


static te_errno
ds_dns_recursive_get(unsigned int gid, const char *oid,
                     char *value, const char *instN, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instN);
    *value = dns_recursive ? '1' : '0';
    value[1] = '\0';
    return 0;
}

static te_errno
ds_dns_recursive_set(unsigned int gid, const char *oid,
                     const char *value, const char *instN, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instN);
    dns_recursive = (strtol(value, NULL, 10) != 0);
    return dns_update_config();
}

/* config parser interface */
void
dns_parse_set_forwarder(const char *fwd)
{
    strcpy(dns_forwarder, fwd);
}

void
dns_parse_set_directory(const char *dir)
{
    strcpy(dns_directory, dir);
}

void
dns_parse_set_recursion(int r)
{
    dns_recursive = r;
}

RCF_PCH_CFG_NODE_RW(node_ds_dnsserver_directory, "directory",
                    NULL, NULL,
                    ds_dns_directory_get, ds_dns_directory_set);

RCF_PCH_CFG_NODE_RW(node_ds_dnsserver_forwarder, "forwarder",
                    NULL, &node_ds_dnsserver_directory,
                    ds_dns_forwarder_get, ds_dns_forwarder_set);

RCF_PCH_CFG_NODE_RW(node_ds_dnsserver_recursive, "recursive",
                    NULL, &node_ds_dnsserver_forwarder,
                    ds_dns_recursive_get, ds_dns_recursive_set);

RCF_PCH_CFG_NODE_RW(node_ds_dnsserver, "dnsserver",
                    &node_ds_dnsserver_recursive, NULL,
                    daemon_get, daemon_set);

te_errno 
dnsserver_grab(const char *name)
{
    char        *dir = NULL;
    int          rc = 0;
    extern FILE *yyin;

    extern int yyparse(void);
    
    UNUSED(name);

    if (file_exists("/etc/named/" NAMED_CONF))
        dir = "/etc/named/";
    else if (file_exists("/etc/bind/" NAMED_CONF))
        dir = "/etc/bind/";
    else if (file_exists("/etc/" NAMED_CONF))
        dir = "/etc/";
    else
    {
        INFO("Failed to locate DNS configuration file");
    }
    
    if ((rc = rcf_pch_add_node("/agent", &node_ds_dnsserver)) != 0)
        return rc;

    if (dir != NULL)
    {
        if ((rc = ds_create_backup(dir, NAMED_CONF, &dns_index)) != 0)
        {
            ERROR("Cannot create backup for %s" NAMED_CONF ": %d", dir, rc);
            return rc;
        }

        OPEN_BACKUP(dns_index, yyin);
        yyparse();
    }

    named_conf_was_running = daemon_running("dnsserver");

    return rc;
}

te_errno 
dnsserver_release(const char *name)
{
    UNUSED(name);

    ds_restore_backup(dns_index);
    if (daemon_running("dnsserver"))
    {
        daemon_set(0, "dnsserver", "0");
        daemon_set(0, "dnsserver", "1");
    }

    return rcf_pch_del_node(&node_ds_dnsserver);
}

void
yyerror(const char *msg)
{
    ERROR("DNS config parser error: %s", msg);
}

#endif /* WITH_DNS_SERVER */


