/** @file
 * @brief Linux Test Agent
 *
 * DHCP server configuring 
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

#ifdef WITH_DHCP_SERVER

#include "linuxconf_daemons.h"


/** Default OMAPI port to be used to control DHCP server */
#define DHCP_SERVER_OMAPI_PORT      5000


/** List of known possible locations of DHCP server scripts */
static const char *dhcp_server_scripts[] = {
    "/etc/init.d/dhcpd",
    "/etc/init.d/dhcp3-server"
};

/** Number of known possible locations of DHCP server scripts */
static unsigned int dhcp_server_n_scripts =
    sizeof(dhcp_server_scripts) / sizeof(dhcp_server_scripts[0]);

/** DHCP server script name */
static const char *dhcp_server_script = NULL;


/** List of known possible locations of DHCP server executables */
static const char *dhcp_server_execs[] = {
    "/usr/sbin/dhcpd",
    "/usr/sbin/dhcpd3"
};

/** Number of known possible locations of DHCP server executables */
static unsigned int dhcp_server_n_execs =
    sizeof(dhcp_server_execs) / sizeof(dhcp_server_execs[0]);

/** DHCP server executable name */
static const char *dhcp_server_exec = NULL;


/** List of known possible locations of DHCP server configuration file */
static const char *dhcp_server_confs[] = {
    "/etc/dhcpd.conf",
    "/etc/dhcp3/dhcpd.conf"
};

/** Number of known possible locations of DHCP server executables */
static unsigned int dhcp_server_n_confs =
    sizeof(dhcp_server_confs) / sizeof(dhcp_server_confs[0]);

/** DHCP server configuration file name */
static const char *dhcp_server_conf = NULL;


/** Index of the DHCP server configuration file backup */
static int dhcp_server_conf_backup = -1;


/** Auxiliary buffer */
static char buf[2048];

/**
 * List of options, which should be quoted automatilally; for other
 * option quotes should be specified in value, if necessary.
 */
static char *quoted_options[] = {
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

/** Definitions of types for DHCP configuring */
typedef struct dhcp_option {
    struct dhcp_option *next;

    char               *name;
    char               *value;
} dhcp_option;

/** Release all memory allocated for option structure */
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
void
ds_shutdown_dhcp_server()
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
        ERROR("No OMAPI port is specified in /etc/dhcpd.conf");
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
            ds_shutdown_dhcp_server();                  \
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


/** Is DHCP server daemon running */
static te_bool
ds_dhcpserver_is_run(void)
{
    sprintf(buf, "killall -CONT %s >/dev/null 2>&1", dhcp_server_exec);
    return (ta_system(buf) == 0);
}

/** Get DHCP server daemon on/off */
static int
ds_dhcpserver_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    sprintf(value, "%u", ds_dhcpserver_is_run());

    return 0;
}

/** Stop DHCP server */
static int
ds_dhcpserver_stop(void)
{
    sprintf(buf, "%s stop >/dev/null 2>&1", dhcp_server_script);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_LINUX, ETESHCMD);
    }
    return 0;
}

/** Start DHCP server */
static int
ds_dhcpserver_start(void)
{
    int rc;

    sprintf(buf, "%s start >/dev/null 2>&1", dhcp_server_script);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_LINUX, ETESHCMD);
    }
    
    rc = init_omapi();
    if ((rc != 0) &&
        (ds_dhcpserver_stop() != 0))
    {
        ERROR("Failed to roll back DHCP server start");
    }

    return TE_RC(TE_TA_LINUX, rc);
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

    if (*value == '1')
    {
        rc = ds_dhcpserver_start();
    }
    else
    {
        rc = ds_dhcpserver_stop();
    }

    return TE_RC(TE_TA_LINUX, rc);
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
                    ds_dhcpserver_get, ds_dhcpserver_set);
#else
RCF_PCH_CFG_NODE_RW(node_ds_dhcpserver, "dhcpserver",
                    &node_ds_host,
                    NULL,
                    ds_dhcpserver_get, ds_dhcpserver_set);
#endif


/**
 * (Re)initialize host & group lists parsing dhcpd.conf
 *
 * @return status code
 */
void
ds_init_dhcp_server(rcf_pch_cfg_object **last)
{
    int     rc = 0;
    te_bool restart = FALSE;

    /* Find DHCP server executable */
    rc = find_file(dhcp_server_n_execs, dhcp_server_execs, TRUE);
    if (rc < 0)
    {
        WARN("Failed to find DHCP server executable"
             " - DHCP will not be available");
        return;
    }
    dhcp_server_exec = dhcp_server_execs[rc];

    /* Find DHCP server script */
    rc = find_file(dhcp_server_n_scripts, dhcp_server_scripts, TRUE);
    if (rc < 0)
    {
        WARN("Failed to find DHCP server script"
             " - DHCP will not be available");
        return;
    }
    dhcp_server_script = dhcp_server_scripts[rc];

    /* Find DHCP server configuration file */
    rc = find_file(dhcp_server_n_confs, dhcp_server_confs, FALSE);
    if (rc < 0)
    {
        WARN("Failed to find DHCP server configuration file"
             " - DHCP will not be available");
        return;
    }
    dhcp_server_conf = dhcp_server_confs[rc];

    /* Test existing configuration file */
    snprintf(buf, sizeof(buf), "%s -t >/dev/null 2>&1", 
             dhcp_server_exec);
    if (ta_system(buf) != 0)
    {
        WARN("Bad found DHCP server configution file '%s'"
             " - DHCP will not be available", dhcp_server_conf);
        return;
    }

    /* Initialize DHCP server control interface */
    if (dhcpctl_initialize() != ISC_R_SUCCESS)
    {
        WARN("dhcpctl_initialize() failed"
             " - DHCP will not be available");
        return;
    }

    /* Parse found configuration file */ 
    f = fopen(dhcp_server_conf, "r");
    if (f == NULL)
    {
        WARN("Failed to open DHCP server configuration file '%s' "
             "for reading - DHCP will not be available",
             dhcp_server_conf);
        return;
    }

    /* Create DHCP server configuration file backup */
    rc = ds_create_backup(NULL, dhcp_server_conf,
                          &dhcp_server_conf_backup);
    if (rc != 0)
    {
        WARN("Failed to create DHCP server backup"
             " - DHCP will not be available");
        return;
    }

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

    fclose(f);

    if (rc != 0)
    {
        WARN("Failed to parse DHCP server configuration file '%s'"
             " - DHCP will not be available", dhcp_server_conf);
        ds_shutdown_dhcp_server();
        return;
    }

    if (omapi_port != 0)
    {
        WARN("OMAPI port is specified in %s", dhcp_server_conf);
    }
    else
    {
        omapi_port = DHCP_SERVER_OMAPI_PORT;

        f = fopen(dhcp_server_conf, "a");
        if (f == NULL)
        {
            WARN("Failed to open DHCP server configuration file '%s' "
                 "to append - DHCP will not be available",
                 dhcp_server_conf);
            ds_shutdown_dhcp_server();
            return;
        }
        fprintf(f, "omapi-port %u;\n", omapi_port);
        fclose(f);

        restart = TRUE;
    }
    
    if (ds_dhcpserver_is_run())
    {
        if (restart)
        {
        }
        init_omapi();
    }

    DS_REGISTER(dhcpserver);
}

#endif
