/** @file
 * @brief Linux Test Agent
 *
 * Linux daemons configuring implementation
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#ifdef WITH_DHCP_SERVER
#include <dhcpctl.h>
#endif

#include "te_errno.h"
#include "te_defs.h"
#include "comm_agent.h"
#include "rcf_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#define TE_LGR_USER      "Daemons"
#include "logger_ta.h"

#include "linux_internal.h"

#define MAC_ADDR_LEN        6

/** Directory where all TE temporary files are located */
#define TE_TMP_PATH         "/tmp/"

/** Suffix for service backup files */
#define TE_TMP_BKP_SUFFIX   ".te_backup"

/** Suffix for temporary files */
#define TE_TMP_FILE_SUFFIX  ".tmpf"

/** Directory where xinetd service configuration files are located */
#define XINETD_ETC_DIR      "/etc/xinetd.d/"

/** Directory where vsftpd configuration file is located */
#define FTPD_CONF           "vsftpd.conf"
#define FTPD_CONF_BACKUP    TE_TMP_PATH FTPD_CONF TE_TMP_BKP_SUFFIX
/** Full name of the FTP daemon configuration file */
static const char *ftpd_conf = NULL;

#define GET_DAEMON_NAME(_oid) \
    ((strstr(_oid, "dhcpserver") != NULL) ? "dhcpd" :                  \
     (strstr(_oid, "dnsserver") != NULL) ? "named" :                   \
     (strstr(_oid, "todudpserver") != NULL) ? "time-udp" :             \
     (strstr(_oid, "tftpserver") != NULL) ? "tftp" :                   \
     (strstr(_oid, "ftpserver") != NULL) ? "vsftpd" :                  \
     (strstr(_oid, "echoserver") != NULL) ? "echo" : NULL)

/* Auxiliary buffer */
static char  buf[2048] = {0, };

/* Get current state daemon or xinetd service */
static int
daemon_get(unsigned int gid, const char *oid, char *value)
{
    const char *daemon_name = GET_DAEMON_NAME(oid);

    UNUSED(gid);

    if (daemon_name == NULL)
    {
        return TE_RC(TE_TA_LINUX, ENOENT);
    }
    sprintf(buf, "killall -CONT %s >/dev/null 2>&1", daemon_name);
    sprintf(value, "%s", ta_system(buf) == 0 ? "1" : "0");

    return 0;
}

/* On/off daemon */
static int
daemon_set(unsigned int gid, const char *oid, const char *value)
{
    const char *daemon_name = GET_DAEMON_NAME(oid);
    
    char value0[2];
    int  rc;

    UNUSED(gid);

    if ((rc = daemon_get(gid, oid, value0)) != 0)
        return rc;
    
    if (strlen(value) != 1 || (*value != '0' && *value != '1'))
        return TE_RC(TE_TA_LINUX, EINVAL);

    if (daemon_name == NULL)
    {
        return TE_RC(TE_TA_LINUX, ENOENT);
    }

    if (value0[0] == value[0])
        return 0;

    sprintf(buf, "/etc/init.d/%s %s >/dev/null 2>&1", daemon_name,
            *value == '0' ? "stop" : "start");

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_LINUX, ETESHCMD);
    }

    return 0;
}

/* Get current state of xinetd service */
static int
xinetd_get(unsigned int gid, const char *oid, char *value)
{
    FILE       *f;
    const char *daemon_name = GET_DAEMON_NAME(oid);

    UNUSED(gid);

    if (daemon_name == NULL)
    {
        return TE_RC(TE_TA_LINUX, ENOENT);
    }

    sprintf(buf, "LANG= /sbin/chkconfig --list %s", daemon_name);
    if ((f = popen(buf, "r")) == NULL)
        return errno;

    if (fgets(buf, sizeof(buf), f) == NULL)
        return TE_RC(TE_TA_LINUX, EPIPE);

    sprintf(value, "%s", strstr(buf, "on") ? "1" : "0");
    pclose(f);

    return 0;
}

/* On/off xinetd service */
static int
xinetd_set(unsigned int gid, const char *oid, const char *value)
{
    const char *daemon_name = GET_DAEMON_NAME(oid);

    UNUSED(gid);

    if (strlen(value) != 1 || (*value != '0' && *value != '1'))
        return TE_RC(TE_TA_LINUX, EINVAL);

    if (daemon_name == NULL)
    {
        return TE_RC(TE_TA_LINUX, ENOENT);
    }

    sprintf(buf, "/sbin/chkconfig %s %s >/dev/null 2>&1", daemon_name,
            *value == '0' ? "off" : "on");

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_LINUX, ETESHCMD);
    }

    ta_system("/etc/init.d/xinetd reload >/dev/null 2>&1");

    return 0;
}

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
    unsigned int  addr;
    char         *service_path = NULL; /* Path to xinetd service
                                          configuration file */
    char         *tmp_path = NULL; /* Path to temporary file */
    char         *cmd = NULL;
    int           path_len;
    const char   *fmt = "mv %s %s >/dev/null 2>&1";
    FILE         *f;
    FILE         *g;

    if (inet_aton(value, (struct in_addr *)&addr) == 0)
        return EINVAL;

    path_len = ((strlen(TE_TMP_PATH) > strlen(XINETD_ETC_DIR)) ?
                strlen(TE_TMP_PATH) : strlen(XINETD_ETC_DIR)) +
               strlen(service) + strlen(TE_TMP_FILE_SUFFIX) + 1;

#define FREE_BUFFERS \
    do {                     \
        free(service_path);  \
        free(tmp_path);      \
        free(cmd);           \
    } while (0)

    if ((service_path = (char *)malloc(path_len)) == NULL ||
        (tmp_path = (char *)malloc(path_len)) == NULL ||
        (cmd = (char *)malloc(2 * path_len + strlen(fmt))) == NULL)
    {
        FREE_BUFFERS;
        return TE_RC(TE_TA_LINUX, ENOMEM);
    }

    snprintf(service_path, path_len, "%s%s", XINETD_ETC_DIR, service);
    snprintf(tmp_path, path_len, "%s%s%s",
             TE_TMP_PATH, service, TE_TMP_FILE_SUFFIX);
    snprintf(cmd, 2 * path_len + strlen(fmt), fmt, tmp_path, service_path);

    if ((f = fopen(service_path, "r")) == NULL)
    {
        FREE_BUFFERS;
        return TE_RC(TE_TA_LINUX, errno);
    }
    if ((g = fopen(tmp_path, "w")) == NULL)
    {
        FREE_BUFFERS;
        fclose(f);
        return 0;
    }

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

    /* Update service configuration file */
    ta_system(cmd);
    ta_system("/etc/init.d/xinetd reload >/dev/null 2>&1");

    FREE_BUFFERS;

#undef FREE_BUFFERS

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
    int           path_len;
    char         *path;
    FILE         *f;

    path_len = strlen(XINETD_ETC_DIR) + strlen(service) + 1;
    if ((path = (char *)malloc(path_len)) == NULL)
        return TE_RC(TE_TA_LINUX, ENOMEM);

    snprintf(path, path_len, "%s%s", XINETD_ETC_DIR, service);
    if ((f = fopen(path, "r")) == NULL)
    {
        free(path);
        return TE_RC(TE_TA_LINUX, errno);
    }
    free(path);

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


#ifdef WITH_DHCP_SERVER

/*
 * List of options, which should be quoted automatilally; for other
 * option quotes should be specified in value, if necessary.
 */
static char *quoted_options[] = {
    "bootfile-name", "domain-name", "extension-path-name", "merit-dump",
    "nis-domain", "nisplus-domain", "root-path", "uap-servers",
    "tftp-server-name", "uap-servers", "fqdn.fqdn"
};

/* Definitions of types for DHCP configuring */
typedef struct dhcp_option {
    struct dhcp_option *next;

    char               *name;
    char               *value;
} dhcp_option;

/* Release all memory allocated for option structure */
#define FREE_OPTION(_opt) \
    do {                        \
        free(_opt->name);       \
        free(_opt->value);      \
        free(_opt);             \
    } while (0)

typedef struct host {
    struct host  *next;
    char         *name;
    struct group *group;
    char         *chaddr;
    char         *client_id;
    char         *ip_addr;
    char         *next_server;
    char         *filename;
    dhcp_option  *options;
    te_bool       dynamic;      /* The host is added dynamically */
    te_bool       deleted;      /* Static host, which was deleted */
} host;

static host *hosts = NULL;

typedef struct group {
    struct group  *next;
    char          *name;
    char          *filename;
    char          *next_server;
    dhcp_option   *options;
    te_bool        dynamic;      /* The host is added dynamically */
    te_bool        deleted;      /* Static host, which was deleted */
} group;

static group *groups = NULL;

static char *s = buf;   /* Pointer to the place of parsing configuration
                           in the buf during /etc/dhcpd.conf parsing */
static FILE *f = NULL;  /* Pointer to opened /etc/dhcpd.conf */

static unsigned short omapi_port = 0;

/* Auxiliary variables for OMAPI using */
static dhcpctl_handle conn = NULL;
static dhcpctl_handle lo = NULL;         /* Lease object */

static int init_dhcp_data();

/* Check, if the option should be quoted */
static te_bool
is_quoted(const char *opt_name)
{
    unsigned int i;
    for (i = 0; i < sizeof(quoted_options)/sizeof(char *); i++)
        if (strcmp(opt_name, quoted_options[i]) == 0)
            return TRUE;
    return FALSE;
}

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
static dhcp_option *
find_option(dhcp_option *opt, const char *name)
{
    for (; opt != NULL && strcmp(opt->name, name) != 0; opt = opt->next);

    return opt;
}

/* Release all memory allocated for host structure */
static void
free_host(host *h)
{
    dhcp_option *opt, *next;

    free(h->name);
    free(h->chaddr);
    free(h->client_id);
    free(h->ip_addr);
    free(h->next_server);
    free(h->filename);
    for (opt = h->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
}

static void
free_group(group * g)
{
    dhcp_option *opt, *next;

    free(g->name);
    free(g->next_server);
    free(g->filename);
    for (opt = g->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
}
/* Release all memory allocated for DHCP data */
static void
free_dhcp_data(void)
{
    host  *host, *host_tmp;
    group *group, *group_tmp;

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
}

/*--------------------- dhcpd.conf processing --------------------------*/

/*
 * It is assumed that syntax of dhcpd.conf is correct - this is checked
 * before file processing using dhcpd with option -t
 */

/** Get one non-empty (without comments) line from the file stream. */
static int
get_line()
{
    char *tmp;

    do {
        if (fgets(buf, sizeof(buf), f) == NULL)
            return TE_RC(TE_TA_LINUX, EOF);
        if (strlen(buf) + 1 == sizeof(buf))
        {
            VERB("too log line in /etc/dhcpd.conf\n");
            return TE_RC(TE_TA_LINUX, ENOMEM);
        }
        if ((tmp = strchr(buf, '#')) != NULL)
            *tmp = 0;
        if ((tmp = strchr(buf, '\r')) != NULL)
            *tmp = 0;
        if ((tmp = strchr(buf, '\n')) != NULL)
            *tmp = 0;
    } while (*buf == 0);

    return 0;
}

/**
 * Find a token in the stream. If s already points to the token,
 * do nothing.
 */
static int
get_token()
{
    int rc;

    while (TRUE)
    {
        while (isspace(*s))
            s++;

        if (*s == 0)
        {
            if ((rc = get_line()) != 0)
                return TE_RC(TE_TA_LINUX, rc);
            s = buf;
        }
        else
            break;
    }
    return 0;
}

/* Remove quotes from the token */
#define REMOVE_QUOTES(_s) \
    do {                            \
        if (*_s == '"')             \
        {                           \
            int n = strlen(_s) - 1; \
            memmove(_s, _s + 1, n); \
            *(_s + n - 1) = 0;      \
        }                           \
    } while (0)

/** Make copy of the current token to _s0 and move s to the next token */
static char *
extract_token()
{
    char *tmp;
    char *scopy;

    for (tmp = s;
         !isspace(*tmp) && *tmp != 0 && *tmp != ';' && *tmp != '{';
         tmp++);

    if (*tmp == 0)
    {
        scopy = strdup(s);
        get_line();
    }
    else
    {
        char c = *tmp;
        *tmp = 0;
        scopy = strdup(s);
        *tmp = c;
        s = tmp;
    }
    if (strlen(scopy) >= RCF_MAX_ID)
    {
        free(scopy);
        VERB("too long token in /etc/dhcpd.conf\n");
        return NULL;
    }
    return scopy;
}

/** Process the record specifiing OMAPI port. */
static int
process_omapi_record()
{
    s += strlen("omapi-port");

    get_token();

    if ((omapi_port = strtol(s, &s, 10)) == 0)
    {
        VERB("bad OMAPI port is specified in /etc/dhcpd.conf\n");
        return TE_RC(TE_TA_LINUX, EINVAL);
    }

    get_token();
    assert(*s == ';');
    s++;

    return 0;
}

/** Process the record, which is not group, host or omapi-port. */
static int
process_other_record()
{
    te_bool in_quotes = FALSE;
    int     brackets = 0;
    int     rc = 0;

    for (; *s != 0 || (rc = get_token()) == 0; s++)
    {
        if (*s == '"')
        {
            in_quotes = !in_quotes;
            continue;
        }

        if (in_quotes)
        {
            if (*s == '\\')
                s++;
            continue;
        }

        if (*s == ';' && brackets == 0)
        {
            s++;
            break;
        }

        if (*s == '}')
        {
            if (--brackets == 0)
            {
                s++;
                /*
                 * If the record is ...{...};, then ';' will be skipped
                 * by the next call of the function.
                 */
                break;
            }
        }

        if (*s == '{')
            brackets++;
    }

    return TE_RC(TE_TA_LINUX, rc);
}

/*
 * Macros below are defined to process filename, next-server and
 * options definitions. They should be called from process_host_record()
 * and process_group_record() because relies on local macro CHECKNULL.
 *
 * Usage:
 *
 * CHECK_FILENAME(gh)
 *     or
 * else CHECK_FILENAME(gh)
 *
 * gh should be either group * or host *.
 */
#define CHECK_FILENAME(_gh) \
    if (strncasecmp(s, "filename", strlen("filename")) == 0)       \
    {                                                              \
        s += strlen("filename");                                   \
        get_token();                                               \
        CHECKNULL((_gh)->filename = extract_token());              \
        REMOVE_QUOTES((_gh)->filename);                            \
    }

#define CHECK_NEXT_SERVER(_gh) \
    if (strncasecmp(s, "next-server", strlen("next-server")) == 0)      \
    {                                                                   \
        s += strlen("next-server");                                     \
        get_token();                                                    \
        CHECKNULL((_gh)->next_server = extract_token());                \
    }

#define CHECK_FIXED_ADDRESS(_gh) \
    if (strncasecmp(s, "fixed-address", strlen("fixed-address")) == 0)  \
    {                                                                   \
        s += strlen("fixed-address");                                   \
        get_token();                                                    \
        CHECKNULL((_gh)->ip_addr = extract_token());                    \
    }

#define CHECK_OPTION(_gh) \
    if (strncasecmp(s, "option", strlen("option")) == 0)                \
    {                                                                   \
        dhcp_option *opt;                                               \
        char        *s0;                                                \
                                                                        \
        CHECKNULL(opt = (dhcp_option *)calloc(sizeof(*opt), 1));        \
        CHECKNULL(opt->value = (char *)calloc(RCF_MAX_VAL, 1));         \
        opt->next = (_gh)->options;                                     \
        (_gh)->options = opt;                                           \
        s += strlen("option");                                          \
        get_token();                                                    \
        CHECKNULL(opt->name = extract_token());                         \
        for (s0 = opt->name; *s0 != 0; s0++)                            \
            *s0 = tolower(*s0);                                         \
        get_token();                                                    \
        for (s0 = opt->value;                                           \
             *s != ';';                                                 \
             s++, (void)(*s == 0 ? get_line() : 0))                     \
        {                                                               \
            *s0++ = *s;                                                 \
            if (s0 - opt->value >= RCF_MAX_VAL)                         \
            {                                                           \
                VERB("too long option in /etc/dhcpd.conf\n");      \
                CHECKNULL(NULL);                                        \
            }                                                           \
        }                                                               \
        for (*s0-- = 0; isspace(*s0); s0--)                             \
            *s0 = 0;                                                    \
        REMOVE_QUOTES(opt->value);                                      \
        s++;                                                            \
    }

/** Process the host record in dhcpd.conf. First line is already in buf. */
static int
process_host_record()
{
    host *h;

    dhcp_option *opt;
    dhcp_option *prev;

#define CHECKNULL(x) \
    do {                                        \
        if ((x) == NULL)                        \
        {                                       \
            free_host(h);                       \
            return TE_RC(TE_TA_LINUX, ENOMEM);  \
        }                                       \
    } while (0)

    if ((h = (host *)calloc(sizeof(host), 1)) == NULL)
        return TE_RC(TE_TA_LINUX, ENOMEM);

    /* Parse host name */
    s += strlen("host");
    get_token();
    CHECKNULL(h->name = extract_token());

    get_token();
    assert(*s == '{');
    s++;

    while (TRUE)
    {
        get_token();
        if (*s == '}')
        {
           s++;
           break;
        }
        else CHECK_FILENAME(h)
        else CHECK_NEXT_SERVER(h)
        else CHECK_FIXED_ADDRESS(h)
        else CHECK_OPTION(h)
        else if (strncasecmp(s, "hardware", strlen("hardware")) == 0)
        {
            s += strlen("hardware");
            get_token();
            if (strncasecmp(s, "ethernet", strlen("ethernet")) != 0)
            {
                VERB("hardware type %s specified in /etc/dhcpd.conf"
                          " is not supported\n", s);
                free(h);
                return TE_RC(TE_TA_LINUX, EINVAL);
            }
            s += strlen("ethernet");
            get_token();
            CHECKNULL(h->chaddr = extract_token());
        }
        else if (strncasecmp(s, "group", strlen("group")) == 0)
        {
            char *g;

            s += strlen("group");
            get_token();
            CHECKNULL(g = extract_token());
            h->group = find_group(g);
            free(g);
        }
        else
            process_other_record();
    }

    /* Look for client identifier option */
    for (opt = h->options, prev = NULL;
         opt != NULL && strcmp(opt->name, "dhcp-client-identifier") != 0;
         prev = opt, opt = opt->next);

    if (opt != NULL)
    {
        if (prev != NULL)
            prev->next = opt->next;
        else
            h->options = opt->next;
        h->client_id = opt->value;
        free(opt->name);
        free(opt);
    }

    h->next = hosts;
    hosts = h;

    return 0;

#undef CHECKNULL
}

/**
 * Process the group or subnet record in dhcpd.conf.
 * First line is already in buf.
 */
static int
process_group_record()
{
    group *g;
    host  *h_old = hosts;
    host  *h;

    if ((g = (group *)calloc(sizeof(group), 1)) == NULL)
        return TE_RC(TE_TA_LINUX, ENOMEM);

#define CHECKNULL(x) \
    do {                                        \
        if ((x) == NULL)                        \
        {                                       \
            free_group(g);                      \
            return TE_RC(TE_TA_LINUX, ENOMEM);  \
        }                                       \
    } while (0)

    if (*s == 'g')
    {
        /* Parse group name */
        s += strlen("group");
        get_token();
        if (*s != '{')
        {
            CHECKNULL(g->name = extract_token());
            get_token();
        }
    }
    else
    {
        while (*s != '{')
        {
            s++;
            if (*s == 0)
                get_token();
        }
    }
    assert(*s == '{');
    s++;

    while (TRUE)
    {
        get_token();
        if (*s == '}')
        {
           s++;
           break;
        }
        if (strncasecmp(s, "host", strlen("host")) == 0)
        {
            if (process_host_record() != 0)
                return TE_RC(TE_TA_LINUX, EOF);
        } else if (strncasecmp(s, "subnet", strlen("subnet")) == 0 ||
                   strncasecmp(s, "shared-network",
                               strlen("shared-network")) == 0 ||
                   strncasecmp(s, "group", strlen("group")) == 0)
        {
            if (process_group_record() != 0)
                return TE_RC(TE_TA_LINUX, EOF);
        }
        else CHECK_FILENAME(g)
        else CHECK_NEXT_SERVER(g)
        else CHECK_OPTION(g)
        else
            process_other_record();
    }

    /*
     * Fill grup information in host structures associated with host
     * records inside group record.
     */
    for (h = hosts; h != h_old; h = h->next)
    {
        if (g->name != NULL)
        {
            h->group = g;
        }
        else
        {
            dhcp_option *opt;

            if (g->filename != NULL && h->filename == NULL)
                CHECKNULL(h->filename = strdup(g->filename));

            if (g->next_server != NULL && h->next_server == NULL)
                CHECKNULL(h->next_server = strdup(g->next_server));

            for (opt = g->options; opt != NULL; opt = opt->next)
            {
                dhcp_option *tmp;

                for (tmp = h->options;
                     tmp != NULL && strcmp(tmp->name, opt->name) != 0;
                     tmp = tmp->next);

                if (tmp != NULL)
                    continue;

                CHECKNULL(tmp = (dhcp_option *)calloc(sizeof(*tmp), 1));
                tmp->next = h->options;
                h->options = tmp;
                CHECKNULL(tmp->name = strdup(opt->name));
                CHECKNULL(tmp->value = strdup(opt->value));
            }
        }
    }

    if (g->name == NULL)
    {
        free_group(g);
        return 0;
    }

    /* Insert the named group into the global list */
    g->next = groups;
    groups = g;
    return 0;

#undef CHECKNULL
}

/** Initialize omapi objects and the connection to DHCP server */
static int
init_omapi()
{
#define CHECKERR(x) \
    do {                                                \
        isc_result_t status = x;                        \
        if (status != ISC_R_SUCCESS)                    \
        {                                               \
            free_dhcp_data();                           \
            VERB("cannot interact with DHCP daemon\n"); \
            return TE_RC(TE_TA_LINUX, EPERM);           \
        }                                               \
    } while (0)

    conn = lo = NULL;
    CHECKERR(dhcpctl_connect(&conn, "127.0.0.1", omapi_port, 0));
    CHECKERR(dhcpctl_new_object(&lo, conn, "lease"));
    return 0;

#undef CHECKERR
}

/** On/off DHCP server */
static int
ds_dhcpserver_set(unsigned int gid, const char *oid, const char *value)
{
    char val[2];
    int  rc;

    UNUSED(oid);

    if ((rc = daemon_get(gid, "dhcpserver", val)) != 0)
        return TE_RC(TE_TA_LINUX, rc);

    if (strlen(value) != 1 || (*value != '0' && *value != '1'))
        return TE_RC(TE_TA_LINUX, EINVAL);

    /*
     * We don't need to change state of DHCP Server:
     * The current state is the same as desired.
     */
    if (*val == *value)
        return 0;

    sprintf(buf, "/etc/init.d/dhcpd %s >/dev/null 2>&1",
            *value == '0' ? "stop" : "start");

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_LINUX, ETESHCMD);
    }

    if (0 && *value == '1' && *val != *value)
    {
        rc = init_omapi();
        if (rc != 0)
            ta_system("/etc/init.d/dhcpd stop >/dev/null 2>&1");
        return TE_RC(TE_TA_LINUX, rc);
    }

    return 0;
}

#define CHECKSTATUS(x) \
    do {                                                            \
        isc_result_t _rc = x;                                       \
        if (_rc != ISC_R_SUCCESS)                                   \
        {                                                           \
            VERB("failure: OMAPI returned %d; TA configuration "    \
                 "database may be corrupted\n", _rc);               \
            return TE_RC(TE_TA_LINUX, EPERM);                       \
        }                                                           \
    } while (0)

#define CHECKSTATUS_DEL(x) \
     if ((rc = x) != ISC_R_SUCCESS) \
         break

#define DELETE_OBJECT(_o, _name) \
    do {                                                                \
        CHECKSTATUS_DEL(dhcpctl_set_string_value(_o, _name, "name"));   \
        CHECKSTATUS_DEL(dhcpctl_open_object(_o, conn, 0));              \
        CHECKSTATUS_DEL(dhcpctl_wait_for_completion(_o, &rc));          \
        CHECKSTATUS_DEL(rc);                                            \
        CHECKSTATUS_DEL(dhcpctl_object_remove(conn, _o));               \
        CHECKSTATUS_DEL(dhcpctl_wait_for_completion(_o, &rc));          \
    } while (0)

/** Apply changes provided by user to DHCP daemon using OMAPI */
static int
set_group(group *g)
{
    dhcp_option *opt;
    unsigned int addr;
    isc_result_t rc;

    dhcpctl_handle go = NULL;

    if (conn == NULL)
        return TE_RC(TE_TA_LINUX, EPERM);

    CHECKSTATUS(dhcpctl_new_object(&go, conn, "group"));

    DELETE_OBJECT(go, g->name);
    if (rc != ISC_R_NOTFOUND)
        CHECKSTATUS(rc);

    CHECKSTATUS(dhcpctl_set_string_value(go, g->name, "name"));
    *buf = 0;
    if (g->filename != NULL)
        sprintf(buf, "filename \"%s\"; ", g->filename);
    if (g->next_server != NULL)
    {
        if (inet_aton(g->next_server, (struct in_addr *)&addr) == 0)
        {
            VERB("IP address in dotted notation should be specified "
                 "as next-server (otherwise OMAPI kills DHCP daemon).\n");
            return TE_RC(TE_TA_LINUX, EINVAL);
        }
        sprintf(buf + strlen(buf), "next-server %s; ", g->next_server);
    }
    for (opt = g->options; opt != NULL; opt = opt->next)
    {
        if (is_quoted(opt->name))
            sprintf(buf + strlen(buf), "option %s \"%s\"; ",
                    opt->name, opt->value);
        else
            sprintf(buf + strlen(buf), "option %s %s; ",
                    opt->name, opt->value);
    }
    if (*buf == 0)
        strcpy(buf, " ; ");

    CHECKSTATUS(dhcpctl_set_string_value(go, buf, "statements"));
    CHECKSTATUS(dhcpctl_open_object(go, conn, DHCPCTL_CREATE));
    CHECKSTATUS(dhcpctl_wait_for_completion(go, &rc));
    CHECKSTATUS(rc);

    return 0;
}

/** Apply changes provided by user to DHCP daemon using OMAPI */
static int
set_host(host *h)
{
    dhcpctl_data_string ip = NULL;
    dhcpctl_data_string mac = NULL;

    dhcp_option *opt;
    isc_result_t rc;

    dhcpctl_handle ho = NULL;

    if (conn == NULL)
        return TE_RC(TE_TA_LINUX, EPERM);

    CHECKSTATUS(dhcpctl_new_object(&ho, conn, "host"));

    DELETE_OBJECT(ho, h->name);
    if (rc != ISC_R_NOTFOUND)
        CHECKSTATUS(rc);

    if (h->client_id == NULL && h->chaddr == NULL)
    {
        /*
         * It's not necessary to create such host :)
         * In any case OMAPI does not allow this.
         */
        return 0;
    }

    CHECKSTATUS(dhcpctl_set_string_value(ho, h->name, "name"));
    if (h->ip_addr != NULL)
    {
        unsigned int addr;

        if (inet_aton(h->ip_addr, (struct in_addr *)&addr) == 0)
            return TE_RC(TE_TA_LINUX, EINVAL);
        omapi_data_string_new(&ip, sizeof(addr), MDL);
        memcpy(ip->value, &addr, sizeof(addr));
        CHECKSTATUS(dhcpctl_set_value(ho, ip, "ip-address"));
    }
    if (h->chaddr != NULL)
    {
        unsigned int  mi[MAC_ADDR_LEN];
        unsigned char m[MAC_ADDR_LEN];
        unsigned char c;
        int           i;

        /*
         * It's possible to use m instead mi in sscanf, but the code
         * below seems more safe and warning-less.
         */
        if (sscanf(h->chaddr, "%02x:%02x:%02x:%02x:%02x:%02x%c",
                   mi, mi + 1, mi + 2, mi + 3, mi + 4, mi + 5, &c) != 6)
        {
            return TE_RC(TE_TA_LINUX, EINVAL);
        }
        for (i = 0; i < MAC_ADDR_LEN; i++)
            m[i] = (unsigned char)mi[i];

        omapi_data_string_new(&mac, sizeof(m), MDL);
        memcpy(mac->value, m, sizeof(m));
        CHECKSTATUS(dhcpctl_set_int_value(ho, 1, "hardware-type"));
        CHECKSTATUS(dhcpctl_set_value(ho, mac, "hardware-address"));
    }
    if (h->group != NULL)
        CHECKSTATUS(dhcpctl_set_string_value(ho, h->group->name, "group"));
    if (h->client_id != NULL)
        CHECKSTATUS(dhcpctl_set_string_value(ho, h->client_id,
                                             "dhcp-client-identifier"));
    *buf = 0;

    if (h->filename != NULL)
        sprintf(buf, "filename \"%s\"; ", h->filename);
    if (h->next_server != NULL)
    {
        unsigned int addr;

        if (inet_aton(h->next_server, (struct in_addr *)&addr) == 0)
        {
            VERB("IP address in dotted notation should be specified "
                 "as next-server (otherwise OMAPI kills DHCP daemon).\n");
            return TE_RC(TE_TA_LINUX, EINVAL);
        }
        sprintf(buf + strlen(buf), "next-server %s; ", h->next_server);
    }

    for (opt = h->options; opt != NULL; opt = opt->next)
    {
        if (is_quoted(opt->name))
            sprintf(buf + strlen(buf), "option %s \"%s\"; ",
                    opt->name, opt->value);
        else
            sprintf(buf + strlen(buf), "option %s %s; ",
                    opt->name, opt->value);
    }

    if (*buf != 0)
        CHECKSTATUS(dhcpctl_set_string_value(ho, buf, "statements"));
    CHECKSTATUS(dhcpctl_open_object(ho, conn, DHCPCTL_CREATE));
    CHECKSTATUS(dhcpctl_wait_for_completion(ho, &rc));
    CHECKSTATUS(rc);

    if (ip != NULL)
        dhcpctl_data_string_dereference(&ip, MDL);

    if (mac != NULL)
        dhcpctl_data_string_dereference(&mac, MDL);

    return 0;
}

/** Definition of list method for host and groups */
#define LIST_METHOD(_gh) \
static int \
ds_##_gh##_list(unsigned int gid, const char *oid, char **list) \
{                                                               \
    _gh *gh;                                                    \
                                                                \
    UNUSED(oid);                                                \
    UNUSED(gid);                                                \
                                                                \
    *buf = 0;                                                   \
                                                                \
    for (gh = _gh##s; gh != NULL; gh = gh->next)                \
    {                                                           \
        if (!gh->deleted)                                       \
            sprintf(buf + strlen(buf), "%s ",  gh->name);       \
    }                                                           \
                                                                \
    return (*list = strdup(buf)) == NULL ?                      \
               TE_RC(TE_TA_LINUX, ENOMEM) : 0;                  \
}

LIST_METHOD(host)
LIST_METHOD(group)

/** Definition of add method for host and groups */
#define ADD_METHOD(_gh) \
static int \
ds_##_gh##_add(unsigned int gid, const char *oid, const char *value,    \
               const char *dhcpserver, const char *name)                \
{                                                                       \
    _gh *gh;                                                            \
    int  rc;                                                            \
                                                                        \
    UNUSED(gid);                                                        \
    UNUSED(oid);                                                        \
    UNUSED(dhcpserver);                                                 \
    UNUSED(value);                                                      \
                                                                        \
    if ((gh = find_##_gh(name)) != NULL)                                \
        return TE_RC(TE_TA_LINUX, gh->deleted ? EPERM : EEXIST);        \
                                                                        \
    if ((gh = (_gh *)calloc(sizeof(_gh), 1)) == NULL)                   \
        return TE_RC(TE_TA_LINUX, ENOMEM);                              \
                                                                        \
    if ((gh->name = strdup(name)) == NULL)                              \
    {                                                                   \
        free(gh);                                                       \
        return TE_RC(TE_TA_LINUX, ENOMEM);                              \
    }                                                                   \
                                                                        \
    if ((rc = set_##_gh(gh)) != 0)                                      \
    {                                                                   \
        free(gh->name);                                                 \
        free(gh);                                                       \
        return TE_RC(TE_TA_LINUX, rc);                                  \
    }                                                                   \
                                                                        \
    gh->dynamic = TRUE;                                                 \
    gh->next = _gh##s;                                                  \
    _gh##s = gh;                                                        \
                                                                        \
    return 0;                                                           \
}

ADD_METHOD(host)
ADD_METHOD(group)

/** Definition of get method for host and groups */
#define GET_METHOD(_gh) \
static int \
ds_##_gh##_get(unsigned int gid, const char *oid, char *value,          \
               const char *dhcpserver, const char *name)                \
{                                                                       \
    _gh *gh;                                                            \
                                                                        \
    UNUSED(gid);                                                        \
    UNUSED(oid);                                                        \
    UNUSED(dhcpserver);                                                 \
    UNUSED(value);                                                      \
                                                                        \
    if ((gh = find_##_gh(name)) == NULL || gh->deleted)                 \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);                       \
                                                                        \
    return 0;                                                           \
}

GET_METHOD(host)
GET_METHOD(group)

/** Definition of delete method for host and groups */
#define DEL_METHOD(_gh) \
static int \
ds_##_gh##_del(unsigned int gid, const char *oid,       \
               const char *dhcpserver, const char *name)\
{                                                       \
    _gh *gh;                                            \
    _gh *prev;                                          \
                                                        \
    isc_result_t rc;                                    \
                                                        \
    dhcpctl_handle _o = NULL;                           \
                                                        \
    UNUSED(gid);                                        \
    UNUSED(oid);                                        \
    UNUSED(dhcpserver);                                 \
                                                        \
    rc = dhcpctl_new_object(&_o, conn, #_gh);           \
    CHECKSTATUS(rc);                                    \
                                                        \
    for (gh = _gh##s, prev = NULL;                      \
         gh != NULL && strcmp(gh->name, name) != 0;     \
         prev = gh, gh = gh->next);                     \
                                                        \
    if (gh == NULL || gh->deleted)                      \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);       \
                                                        \
    DELETE_OBJECT(_o, name);                            \
    if (rc == ISC_R_NOTFOUND)                           \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);       \
    CHECKSTATUS(rc);                                    \
                                                        \
    if (!gh->dynamic)                                   \
        gh->deleted = TRUE;                             \
    else                                                \
    {                                                   \
        if (prev)                                       \
            prev->next = gh->next;                      \
        else                                            \
            _gh##s = gh->next;                          \
        free_##_gh(gh);                                 \
    }                                                   \
                                                        \
    return 0;                                           \
}

DEL_METHOD(host)
DEL_METHOD(group)

/** Obtain the group of the host */
static int
ds_host_group_get(unsigned int gid, const char *oid, char *value,
                  const char *dhcpserver, const char *name)
{
    host *h;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    if ((h = find_host(name)) == NULL || h->deleted)
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);

    if (h->group == NULL)
        *value = 0;
    else
        strcpy(value, h->group->name);

    return 0;
}

/** Change the group of the host */
static int
ds_host_group_set(unsigned int gid, const char *oid, const char *value,
                  const char *dhcpserver, const char *name)
{
    host  *h;
    group *old;
    int    rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    if ((h = find_host(name)) == NULL || h->deleted)
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);

    if (!h->dynamic)
        return TE_RC(TE_TA_LINUX, EPERM);

    old = h->group;
    if (*value == 0)
        h->group = NULL;
    else if ((h->group = find_group(value)) == NULL)
    {
        h->group = old;
        return TE_RC(TE_TA_LINUX, EINVAL);
    }

    if ((rc = set_host(h)) != 0)
        h->group = old;

    return TE_RC(TE_TA_LINUX, rc);
}

/*
 * Definition of the routines for obtaining/changing of host/group
 * attributes (except options).
 */
#define ATTR_GET(_attr, _gh) \
static int \
ds_##_gh##_##_attr##_get(unsigned int gid, const char *oid,     \
                         char *value, const char *dhcpserver,   \
                         const char *name)                      \
{                                                               \
    _gh *gh;                                                    \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    if ((gh = find_##_gh(name)) == NULL || gh->deleted)         \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    if (gh->_attr == NULL)                                      \
        *value = 0;                                             \
    else                                                        \
        strcpy(value, gh->_attr);                               \
                                                                \
    return 0;                                                   \
}

#define ATTR_SET(_attr, _gh) \
static int \
ds_##_gh##_##_attr##_set(unsigned int gid, const char *oid,     \
                         const char *value,                     \
                         const char *dhcpserver,                \
                         const char *name)                      \
{                                                               \
    _gh  *gh;                                                   \
    char *old_val;                                              \
    int   rc;                                                   \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    if ((gh = find_##_gh(name)) == NULL || gh->deleted)         \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    if (!gh->dynamic)                                           \
        return TE_RC(TE_TA_LINUX, EPERM);                       \
                                                                \
    old_val = gh->_attr;                                        \
    if (*value == 0)                                            \
        gh->_attr = NULL;                                       \
    else                                                        \
    {                                                           \
        if ((gh->_attr = strdup(value)) == NULL)                \
        {                                                       \
            return TE_RC(TE_TA_LINUX, ENOMEM);                  \
        }                                                       \
    }                                                           \
                                                                \
    if ((rc = set_##_gh(gh)) != 0)                              \
    {                                                           \
        free(gh->_attr);                                        \
        gh->_attr = old_val;                                    \
        return TE_RC(TE_TA_LINUX, rc);                          \
    }                                                           \
                                                                \
    free(old_val);                                              \
                                                                \
    return 0;                                                   \
}

ATTR_GET(chaddr, host)
ATTR_SET(chaddr, host)
ATTR_GET(client_id, host)
ATTR_SET(client_id, host)
ATTR_GET(ip_addr, host)
ATTR_SET(ip_addr, host)
ATTR_GET(next_server, host)
ATTR_SET(next_server, host)
ATTR_GET(filename, host)
ATTR_SET(filename, host)
ATTR_GET(next_server, group)
ATTR_SET(next_server, group)
ATTR_GET(filename, group)
ATTR_SET(filename, group)

/**
 * Definition of the method for obtaining of options list
 * for the host/group.
 */
#define GET_OPT_LIST(_gh) \
static int \
ds_##_gh##_option_list(unsigned int gid, const char *oid,       \
                       char **list, const char *dhcpserver,     \
                       const char *name)                        \
{                                                               \
   _gh *gh;                                                     \
                                                                \
    dhcp_option *opt;                                           \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    if ((gh = find_##_gh(name)) == NULL || gh->deleted)         \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    *buf = 0;                                                   \
    for (opt = gh->options; opt != NULL; opt = opt->next)       \
    {                                                           \
        sprintf(buf + strlen(buf), "%s ", opt->name);           \
    }                                                           \
                                                                \
    return (*list = strdup(buf)) == NULL ?                      \
              TE_RC(TE_TA_LINUX, ENOMEM) : 0;                   \
}

GET_OPT_LIST(host)
GET_OPT_LIST(group)

/* Method for adding of the option for group/host */
#define ADD_OPT(_gh) \
static int \
ds_##_gh##_option_add(unsigned int gid, const char *oid,        \
                      const char *value, const char *dhcpserver,\
                      const char *name, const char *optname)    \
{                                                               \
    _gh *gh;                                                    \
    int  rc;                                                    \
                                                                \
    dhcp_option *opt;                                           \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    if (*value == 0)                                            \
        return TE_RC(TE_TA_LINUX, EINVAL);                      \
                                                                \
    if ((gh = find_##_gh(name)) == NULL || gh->deleted)         \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    if (!gh->dynamic)                                           \
        return TE_RC(TE_TA_LINUX, EPERM);                       \
                                                                \
    if (find_option(gh->options, optname) != NULL)              \
        return TE_RC(TE_TA_LINUX, EEXIST);                      \
                                                                \
    if ((opt = (dhcp_option *)calloc(sizeof(*opt), 1))          \
        == NULL || (opt->name = strdup(optname)) == NULL ||     \
        (opt->value = strdup(value)) == NULL)                   \
    {                                                           \
        FREE_OPTION(opt);                                       \
        return TE_RC(TE_TA_LINUX, ENOMEM);                      \
    }                                                           \
                                                                \
    opt->next = gh->options;                                    \
    gh->options = opt;                                          \
                                                                \
    if ((rc = set_##_gh(gh)) != 0)                              \
    {                                                           \
        gh->options = opt->next;                                \
        FREE_OPTION(opt);                                       \
        return TE_RC(TE_TA_LINUX, rc);                          \
    }                                                           \
                                                                \
    return 0;                                                   \
}

ADD_OPT(host)
ADD_OPT(group)

/* Method for obtaining of the option for group/host */
#define GET_OPT(_gh) \
static int \
ds_##_gh##_option_get(unsigned int gid, const char *oid,        \
                      char *value, const char *dhcpserver,      \
                      const char *name, const char *optname)    \
{                                                               \
    _gh *gh;                                                    \
                                                                \
    dhcp_option *opt;                                           \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    if ((gh = find_##_gh(name)) == NULL || gh->deleted)         \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    if ((opt = find_option(gh->options, optname)) == NULL)      \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    strcpy(value, opt->value);                                  \
                                                                \
    return 0;                                                   \
}

GET_OPT(host)
GET_OPT(group)

/* Method for changing of the option value for group/host */
#define SET_OPT(_gh) \
static int \
ds_##_gh##_option_set(unsigned int gid, const char *oid,        \
                      const char *value, const char *dhcpserver,\
                      const char *name, const char *optname)    \
{                                                               \
    _gh *gh;                                                    \
    int  rc;                                                    \
                                                                \
    char *old;                                                  \
                                                                \
    dhcp_option *opt;                                           \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    if ((gh = find_##_gh(name)) == NULL || gh->deleted)         \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    if (!gh->dynamic)                                           \
        return TE_RC(TE_TA_LINUX, EPERM);                       \
                                                                \
    if ((opt = find_option(gh->options, optname)) == NULL)      \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    old = opt->value;                                           \
    if (*value == 0)                                            \
        opt->value = NULL;                                      \
    else                                                        \
    {                                                           \
        if ((opt->value = strdup(value)) == NULL)               \
        {                                                       \
            opt->value = old;                                   \
            return TE_RC(TE_TA_LINUX, ENOMEM);                  \
        }                                                       \
    }                                                           \
                                                                \
    if ((rc = set_##_gh(gh)) != 0)                              \
    {                                                           \
        free(opt->value);                                       \
        opt->value = old;                                       \
        return TE_RC(TE_TA_LINUX, rc);                          \
    }                                                           \
                                                                \
    free(old);                                                  \
                                                                \
    return 0;                                                   \
}

SET_OPT(host)
SET_OPT(group)


/* Method for deletion of the option value for group/host */
#define DEL_OPT(_gh) \
static int \
ds_##_gh##_option_del(unsigned int gid, const char *oid,        \
                      const char *dhcpserver, const char *name, \
                      const char *optname)                      \
{                                                               \
    _gh *gh;                                                    \
    int  rc;                                                    \
                                                                \
    dhcp_option *opt, *prev;                                    \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    if ((gh = find_##_gh(name)) == NULL || gh->deleted)         \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    if (!gh->dynamic)                                           \
        return TE_RC(TE_TA_LINUX, EPERM);                       \
                                                                \
    for (opt = gh->options, prev = NULL;                        \
         opt != NULL && strcmp(opt->name, optname) != 0;        \
         prev = opt, opt = opt->next);                          \
                                                                \
    if (opt == NULL)                                            \
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);               \
                                                                \
    if (prev)                                                   \
        prev->next = opt->next;                                 \
    else                                                        \
        gh->options = opt->next;                                \
                                                                \
    if ((rc = set_##_gh(gh)) != 0)                              \
    {                                                           \
        opt->next = gh->options;                                \
        gh->options = opt;                                      \
        return TE_RC(TE_TA_LINUX, rc);                          \
    }                                                           \
                                                                \
    FREE_OPTION(opt);                                           \
                                                                \
    return 0;                                                   \
}

DEL_OPT(host)
DEL_OPT(group)

#ifdef DHCP_LEASES_SUPPORTED

#define ADDR_LIST_BULK      128

/** Obtain the list of leases */
static int
ds_lease_list(unsigned int gid, const char *oid, char **list)
{
    FILE *f;
    char *name;
    int   len = ADDR_LIST_BULK;

    UNUSED(gid);
    UNUSED(oid);

    if ((f = fopen("/var/lib/dhcp/dhcpd.leases", "r")) == NULL)
        return TE_RC(TE_TA_LINUX, errno);

    if ((*list = (char *)malloc(ADDR_LIST_BULK)) == NULL)
        return TE_RC(TE_TA_LINUX, ENOMEM);

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
                return TE_RC(TE_TA_LINUX, ENOMEM);
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
        return TE_RC(TE_TA_LINUX, EPERM);

    if (inet_aton(name, (struct in_addr *)&addr) == 0)
        return TE_RC(TE_TA_LINUX, ETENOSUCHNAME);

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
static int
ds_lease_get(unsigned int gid, const char *oid, char *value,
             const char *dhcpserver, const char *name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(dhcpserver);

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
        return TE_RC(TE_TA_LINUX, rc);                  \
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
static int
ds_lease_state_get(unsigned int gid, const char *oid, char *value,
                   const char *dhcpserver, const char *name)
GET_INT_LEASE_ATTR("state")

/** Get lease client identifier */
static int
ds_lease_client_id_get(unsigned int gid, const char *oid, char *value,
                       const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    if ((rc = open_lease(name)) != 0)
        return TE_RC(TE_TA_LINUX, rc);

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
static int
ds_lease_hostname_get(unsigned int gid, const char *oid, char *value,
                      const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    if ((rc = open_lease(name)) != 0)
        return TE_RC(TE_TA_LINUX, rc);

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "client-hostname"));

    memcpy(value, val->value, val->len);
    *(value + val->len) = 0;

    dhcpctl_data_string_dereference(&val, MDL);

    return 0;
}

/** Get lease IP address */
static int
ds_lease_host_get(unsigned int gid, const char *oid, char *value,
                  const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(dhcpserver);

    if ((rc = open_lease(name)) != 0)
        return TE_RC(TE_TA_LINUX, rc);

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "host"));

    /* It's not clear what to do next :) */

    return 0;
}

/** Get lease MAC address */
static int
ds_lease_chaddr_get(unsigned int gid, const char *oid, char *value,
                    const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;
    unsigned char      *mac;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    if ((rc = open_lease(name)) != 0)
        return TE_RC(TE_TA_LINUX, rc);

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "hardware-address"));

    mac = (unsigned char *)(val->value);
    sprintf(value, "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    dhcpctl_data_string_dereference(&val, MDL);

    return 0;
}

/* Get lease attributes */

static int
ds_lease_ends_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("ends")

static int
ds_lease_tstp_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("tstp")

static int
ds_lease_cltt_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("cltt")

#endif /* DHCP_LEASES_SUPPORTED */

/**
 * (Re)initialize host & group lists parsing dhcpd.conf
 */
static int
init_dhcp_data()
{
    int  rc = 0;

    if (ta_system("/usr/sbin/dhcpd -t >/dev/null 2>&1") != 0)
    {
        VERB("bad or absent /etc/dhcpd.conf - "
                  "DHCP will not be available\n");
        node_ds_dhcpserver.son = NULL;
        return 0;
    }

    if (dhcpctl_initialize() != ISC_R_SUCCESS)
    {
        VERB("dhcpctl_initialize() failed\n");
        return TE_RC(TE_TA_LINUX, EPERM);
    }

    f = fopen("/etc/dhcpd.conf", "r");
    *buf = 0;

    while (rc == 0)
    {
        if ((rc = get_token()) != 0)
            break;
        if (strncasecmp(s, "group", strlen("group")) == 0 ||
            strncasecmp(s, "shared-network",
                        strlen("shared-network")) == 0 ||
            strncasecmp(s, "subnet", strlen("subnet")) == 0)
        {
            rc = process_group_record();
        }
        else if (strncasecmp(s, "omapi-port", strlen("omapi-port")) == 0)
            rc = process_omapi_record();
        else if (strncasecmp(s, "host", strlen("host")) == 0)
            rc = process_host_record();
        else
            rc = process_other_record();
    }

    if (rc == EOF)
        rc = 0;

    if (omapi_port == 0)
    {
        VERB("no OMAPI port is specified in /etc/dhcpd.conf\n");
        rc = EINVAL;
    }

    fclose(f);

    if (rc != 0)
    {
        free_dhcp_data();
        return TE_RC(TE_TA_LINUX, rc);
    }

    sprintf(buf, "killall -CONT dhcpd >/dev/null 2>&1");
    if (ta_system(buf) != 0)
        return 0;

    init_omapi();

    return 0;
}

static void
print_dhcp_data()
{
    host *h;
    group *g;
    dhcp_option *opt;

    for (h = hosts; h != NULL; h = h->next)
    {
        printf("Host: %s\n", h->name);
        if (h->group)
            printf("\tgroup: %s\n", h->group->name);
        if (h->chaddr)
            printf("\tchaddr: %s\n", h->chaddr);
        if (h->client_id)
            printf("\tclient_id: %s\n", h->client_id);
        if (h->ip_addr)
            printf("\tip_addr: %s\n", h->ip_addr);
        if (h->next_server)
            printf("\tnext_server: %s\n", h->next_server);
        if (h->filename)
            printf("\tfilename: %s\n", h->filename);
        for (opt = h->options; opt != NULL; opt = opt->next)
            printf("\t%s: %s\n", opt->name, opt->value);
    }

    for (g = groups; g != NULL; g = g->next)
    {
        printf("Group: %s\n", g->name);
        if (g->next_server)
            printf("\tnext_server: %s\n", g->next_server);
        if (g->filename)
            printf("\tfilename: %s\n", g->filename);
        for (opt = g->options; opt != NULL; opt = opt->next)
            printf("\t%s: %s\n", opt->name, opt->value);
    }
}


#endif /* WITH_DHCP_SERVER */

#ifdef WITH_TFTP_SERVER

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

    if ((f = fopen("/etc/xinetd.d/tftp", "r")) == NULL)
        return TE_RC(TE_TA_LINUX, errno);

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
        return TE_RC(TE_TA_LINUX, EINVAL);

    if ((f = fopen("/tmp/tftp.te_backup", "r")) == NULL)
        return TE_RC(TE_TA_LINUX, errno);

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

    ta_system("/etc/init.d/xinetd reload >/dev/null 2>&1");

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
        return TE_RC(TE_TA_LINUX, EINVAL);
    }

    /*
     * TFTP logs does not containt year stamp, so that we get current
     * local time, and use extracted year for the log message timstamp.
     */

    /* Get current UTC time */
    if ((cur_time = time(NULL)) == ((time_t)-1))
        return TE_RC(TE_TA_LINUX, errno);

    if (gmtime_r(&cur_time, &tm) == NULL)
        return TE_RC(TE_TA_LINUX, EINVAL);

    /* Use current year for the messsage */
    last_tm->tm_year = tm.tm_year;

    return TE_RC(TE_TA_LINUX, EINVAL);
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
                        return TE_RC(TE_TA_LINUX, ENOMEM);
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
    return TE_RC(TE_TA_LINUX, rc);
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

#ifdef WITH_SSHD

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
            fclose(f);
            return atoi(line);
        }
    }
    
    fclose(f);

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
sshd_add(unsigned int gid, const char *oid, const char *value,
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
        return TE_RC(TE_TA_LINUX, EINVAL);
    
    if (pid != 0)
        return TE_RC(TE_TA_LINUX, EEXIST);
        
    sprintf(buf, "/usr/sbin/sshd -p %s", port);

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_LINUX, ETESHCMD);
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
sshd_del(unsigned int gid, const char *oid, const char *port)
{
    uint32_t pid = sshd_exists((char *)port);

    UNUSED(gid);
    UNUSED(oid);

    if (pid == 0)
        return TE_RC(TE_TA_LINUX, ENOENT);
        
    if (kill(pid, SIGTERM) != 0)
    {
        int kill_errno = errno;
        ERROR("Failed to send SIGTERM "
              "to process SSH daemon with PID=%u: %X",
              pid, kill_errno);
        /* Just to make sure */
        kill(pid, SIGKILL);
    }
    
    return 0;
}

/**
 * Return list
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param list          location for the list pointer
 *
 * @return error code
 */
static int
sshd_list(unsigned int gid, const char *oid, char **list)
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
    
    fclose(f);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_LINUX, ENOMEM);
    
    return 0;
}

#endif /* WITH_SSHD */

/*
 * Daemons configuration tree in reverse order.
 */

#ifdef WITH_DHCP_SERVER

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

static rcf_pch_cfg_object node_ds_group =
    { "group", 0, &node_ds_group_next, NULL,
      (rcf_ch_cfg_get)ds_group_get, NULL,
      (rcf_ch_cfg_add)ds_group_add,
      (rcf_ch_cfg_del)ds_group_del,
      (rcf_ch_cfg_list)ds_group_list, NULL, NULL };

static rcf_pch_cfg_object node_ds_host_option =
    { "option", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_host_option_get,
      (rcf_ch_cfg_set)ds_host_option_set,
      (rcf_ch_cfg_add)ds_host_option_add,
      (rcf_ch_cfg_del)ds_host_option_del,
      (rcf_ch_cfg_list)ds_host_option_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_host_file, "file",
                    NULL, &node_ds_host_option,
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

static rcf_pch_cfg_object node_ds_host =
    { "host", 0, &node_ds_host_group, &node_ds_group,
      (rcf_ch_cfg_get)ds_host_get, NULL,
      (rcf_ch_cfg_add)ds_host_add,
      (rcf_ch_cfg_del)ds_host_del,
      (rcf_ch_cfg_list)ds_host_list, NULL, NULL};

#if DHCP_LEASES_SUPPORTED
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

#endif /* DHCP_LEASES_SUPPORTED */

#if DHCP_LEASES_SUPPORTED
RCF_PCH_CFG_NODE_RW(node_ds_dhcpserver, "dhcpserver",
                    &node_ds_client,
                    NULL,
                    daemon_get, ds_dhcpserver_set);
#else
RCF_PCH_CFG_NODE_RW(node_ds_dhcpserver, "dhcpserver",
                    &node_ds_host,
                    NULL,
                    daemon_get, ds_dhcpserver_set);
#endif

#endif /* WITH_DHCP_SERVER */

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

#ifdef WITH_TODUDP_SERVER

RCF_PCH_CFG_NODE_RW(node_ds_todudpserver_addr, "net_addr",
                    NULL, NULL,
                    ds_todudpserver_addr_get, ds_todudpserver_addr_set);

RCF_PCH_CFG_NODE_RW(node_ds_todudpserver, "todudpserver",
                    &node_ds_todudpserver_addr, NULL,
                    xinetd_get, xinetd_set);

#endif /* WITH_TODUDP_SERVER */

#ifdef WITH_TFTP_SERVER

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

#endif /* WITH_TFTP_SERVER */

#ifdef WITH_DNS_SERVER

RCF_PCH_CFG_NODE_RW(node_ds_dnsserver, "dnsserver",
                    NULL, NULL,
                    daemon_get, daemon_set);

#endif /* WITH_DNS_SERVER */

#ifdef WITH_FTP_SERVER

RCF_PCH_CFG_NODE_RW(node_ds_ftpserver, "ftpserver",
                    NULL, NULL,
                    daemon_get, daemon_set);

#endif /* WITH_FTP_SERVER */

#ifdef WITH_SSHD

RCF_PCH_CFG_NODE_COLLECTION(node_ds_sshd, "sshd",
                            NULL, NULL, 
                            sshd_add, sshd_del, sshd_list, NULL);

#endif /* WITH_SSHD */

/**
 * Maximum number of xinetd services the implemntation supports
 * @todo Rename it to something pretty and move in somewhere generic place
 */
#define XINETD_SERVICE_MAX 10
/* Array of service names */
static char *services[XINETD_SERVICE_MAX];
/* number of services registered */
static unsigned int    n_serv = 0;
static unsigned int    max_serv_name = 0;

/** Restore initial state of the FTP daemon */
static void
restore_backup()
{
    unsigned int i;

    for (i = 0; i < n_serv; i++)
    {
        sprintf(buf, "mv " TE_TMP_PATH "%s" TE_TMP_BKP_SUFFIX " "
                XINETD_ETC_DIR "%s >/dev/null 2>&1",
                services[i], services[i]);
        ta_system(buf);
    }

    ta_system("/etc/init.d/xinetd reload >/dev/null 2>&1");

#ifdef WITH_FTP_SERVER
    if (ftpd_conf != NULL)
    {
        char    ftp_enable[2];
        char    cmd[256]; /* FIXME */

        sprintf(cmd, "mv " FTPD_CONF_BACKUP " %s", ftpd_conf);
        if (ta_system(cmd) != 0)
        {
            ERROR("\"%s\" failed", cmd);
        }
        ta_system("chmod o-w /var/ftp/pub");
        daemon_get(0, "ftpserver", ftp_enable);
        if (*ftp_enable == '1')
        {
            daemon_set(0, "ftpserver", "0");
            daemon_set(0, "ftpserver", "1");
        }
    }

#endif /* WITH_FTP_SERVER */
}


#ifdef WITH_FTP_SERVER
/**
 * Initialize FTP daemon.
 *
 * @return Status code.
 */
int
ftpd_init(void)
{
    FILE *f = NULL;
    FILE *g = NULL;
    char  ftp_enable[2];

    do {
        struct stat stat_buf;

        ftpd_conf = "/etc/vsftpd/" FTPD_CONF;
        if (stat(ftpd_conf, &stat_buf) == 0)
            break;

        ftpd_conf = "/etc/" FTPD_CONF;
        if (stat(ftpd_conf, &stat_buf) == 0)
            break;

        ftpd_conf = NULL;

    } while (FALSE);

    if (ftpd_conf == NULL)
    {
        ERROR("Failed to locate VSFTPD configuration file");
        return TE_RC(TE_TA_LINUX, ETENOSUPP);
    }

    {
        char cmd[256]; /* FIXME */

        sprintf(cmd, "cp -a %s " FTPD_CONF_BACKUP, ftpd_conf);
        if (ta_system(cmd) != 0)
        {
            ERROR("Cannot create backup file " FTPD_CONF_BACKUP);
            restore_backup();
            return TE_RC(TE_TA_LINUX, errno);
        }
    }
    /* Enable anonymous upload for ftp */
    if ((f = fopen(FTPD_CONF_BACKUP , "r")) == NULL)
    {
        ERROR("Failed to open backup file " FTPD_CONF_BACKUP
                  "for reading");
        restore_backup();
        return TE_RC(TE_TA_LINUX, errno);
    }

    if ((g = fopen(ftpd_conf, "w")) == NULL)
    {
        ERROR("Failed to open configuration file '%s' for writing",
                  ftpd_conf);
        restore_backup();
        fclose(f);
        return TE_RC(TE_TA_LINUX, errno);
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strstr(buf, "anonymous_enable") != NULL ||
            strstr(buf, "anon_mkdir_write_enable") != NULL ||
            strstr(buf, "write_enable") != NULL ||
            strstr(buf, "anon_upload_enable") != NULL)
        {
            continue;
        }
        fwrite(buf, 1, strlen(buf), g);
    }
    fprintf(g, "anonymous_enable=YES\n");
    fprintf(g, "anon_mkdir_write_enable=YES\n");
    fprintf(g, "write_enable=YES\n");
    fprintf(g, "anon_upload_enable=YES\n");
    fclose(f);
    fclose(g);
    ta_system("mkdir -p /var/ftp/pub");
    ta_system("chmod o+w /var/ftp/pub");
    daemon_get(0, "ftpserver", ftp_enable);
    if (*ftp_enable == '1')
    {
        daemon_set(0, "ftpserver", "0");
        daemon_set(0, "ftpserver", "1");
    }

    return 0;
}
#endif /* WITH_FTP_SERVER */

/**
 * Initializes linuxconf_daemons support.
 *
 * @param last  node in configuration tree (last sun of /agent) to be
 *              updated
 *
 * @return status code (see te_errno.h)
 */
int
linuxconf_daemons_init(rcf_pch_cfg_object **last)
{


/*
 * Creates a copy of xinetd service configuration file in TMP directory
 * to restore it after Agent finishes
 *
 * @param serv_name_  Service name - filename from /etc/xinetd.d/ directory
 */
#define CREATE_XINETD_SERVICE_BACKUP(serv_name_) \
    do {                                                                   \
        if (n_serv == sizeof(services) / sizeof(services[0]))              \
        {                                                                  \
            restore_backup();                                              \
            ERROR("Too many services of xinetd are registered\n");         \
            return TE_RC(TE_TA_LINUX, EMFILE);                             \
        }                                                                  \
                                                                           \
        if (ta_system("cp " XINETD_ETC_DIR serv_name_ " "                  \
                   TE_TMP_PATH serv_name_ TE_TMP_BKP_SUFFIX                \
                   " >/dev/null 2>&1") != 0)                               \
        {                                                                  \
            restore_backup();                                              \
            ERROR("cannot create backup file "                             \
                      TE_TMP_PATH serv_name_ TE_TMP_BKP_SUFFIX "\n");      \
            return 0;                                                      \
        }                                                                  \
        if (max_serv_name < strlen(serv_name_))                            \
        {                                                                  \
            max_serv_name = strlen(serv_name_);                            \
        }                                                                  \
        services[n_serv++] = serv_name_;                                   \
    } while (0)


#ifdef WITH_ECHO_SERVER
    CREATE_XINETD_SERVICE_BACKUP("echo");
#endif /* WITH_ECHO_SERVER */

#ifdef WITH_TODUDP_SERVER
    CREATE_XINETD_SERVICE_BACKUP("daytime-udp");
#endif /* WITH_TODUDP_SERVER */

#ifdef WITH_TFTP_SERVER
    {
    FILE *f = NULL;
    FILE *g = NULL;

    CREATE_XINETD_SERVICE_BACKUP("tftp");

    /* Set -v option to tftp */
    if ((f = fopen(TE_TMP_PATH "tftp" TE_TMP_BKP_SUFFIX, "r")) == NULL)
    {
        restore_backup();
        return 0;
    }

    if ((g = fopen(XINETD_ETC_DIR "tftp", "w")) == NULL)
    {
        restore_backup();
        fclose(f);
        return 0;
    }

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
    }
#endif /* WITH_TFTP_SERVER */

#ifdef WITH_FTP_SERVER
    if (ftpd_init() == 0)
    {
        (*last)->brother = &node_ds_ftpserver;
        *last = &node_ds_ftpserver;
    }
#endif /* WITH_FTP_SERVER */

#ifdef WITH_DHCP_SERVER
    if (init_dhcp_data() != 0)
    {
        restore_backup();
        return 1;
    }
#endif /* WITH_DHCP_SERVER */

#ifdef WITH_DNS_SERVER
    (*last)->brother = &node_ds_dnsserver;
    *last = &node_ds_dnsserver;
#endif

#ifdef WITH_DHCP_SERVER
    (*last)->brother = &node_ds_dhcpserver;
    *last = &node_ds_dhcpserver;
#endif

#ifdef WITH_ECHO_SERVER
    (*last)->brother = &node_ds_echoserver;
    *last = &node_ds_echoserver;
#endif

#ifdef WITH_TODUDP_SERVER
    (*last)->brother = &node_ds_todudpserver;
    *last = &node_ds_todudpserver;
#endif

#ifdef WITH_TFTP_SERVER
    (*last)->brother = &node_ds_tftpserver;
    *last = &node_ds_tftpserver;
#endif

#ifdef WITH_SSHD
    (*last)->brother = &node_ds_sshd;
    *last = &node_ds_sshd;
#endif

    return 0;

#undef CREATE_XINETD_SERVICE_BACKUP
}

/**
 * Release resources allocated for the configuration support.
 */
void
linux_daemons_release()
{
#ifdef WITH_DHCP_SERVER
    free_dhcp_data();
#endif /* WITH_DHCP_SERVER */
    restore_backup();
}

