#include <dirent.h>
#include "logger_api.h"
#include "conf_daemons.h"
#include "te_queue.h"
#include "te_string.h"
#include "te_errno.h"
#include "rcf_ch_api.h"

/** L2TP global section name */
#define L2TP_GLOBAL           "[global]"

/** L2TP executable file */
#define L2TP_SERVER_EXEC      "/usr/sbin/xl2tpd"

/** L2TP config file */
#define L2TP_SERVER_CONF      "/tmp/te.xl2tpd.conf"

/** CHAP secrets file */
#define L2TP_CHAP_SECRETS     "/etc/ppp/chap-secrets"

/** PAP secrets file */
#define L2TP_PAP_SECRETS      "/etc/ppp/pap-secrets"

/** Name of the option in L2TP config file */
#define PPP_OPTIONS           "pppoptfile"

/** Default buffer size for command-line construction */
#define L2TP_SERVER_LIST_SIZE 1024

/**
 * Default amount of memory allocated for list methods
 * of l2tp subtreee
 */
#define L2TP_CMDLINE_LENGTH   1024

/** Authentication type */
enum l2tp_secret_prot {
    L2TP_SECRET_PROT_CHAP = 0,   /**< CHAP authentication */
    L2TP_SECRET_PROT_PAP  = 1    /**< PAP authentication */
};

/** The class of the options */
enum l2tp_option_type {
    L2TP_OPTION_TYPE_PPP    = 0, /**< PPP options class */
    L2TP_OPTION_TYPE_L2TP   = 1, /**< L2TP options class */
    L2TP_OPTION_TYPE_SECRET = 2, /**< SECRET options */
};

/** CHAP|PAP secret's field */
enum l2tp_secret_field {
    L2TP_SECRET_FIELD_SERVER = 0,   /**< Server field */
    L2TP_SECRET_FIELD_SECRET = 1,   /**< Secret field  */
    L2TP_SECRET_FIELD_IPV4   = 2,   /**< IPv4 field  */
};

/** CHAP|PAP secret structure */
typedef struct te_l2tp_secret {
    SLIST_ENTRY(te_l2tp_secret) list;
    enum l2tp_secret_prot       type;   /**< CHAP|PAP secret */
    char                       *sipv4;  /**< IP address in human dot notation*/
    char                       *server; /**< Server name */
    char                       *secret; /**< Secret name */
} te_l2tp_secret;

/** Options for L2TP config file */
typedef struct te_l2tp_option {
    SLIST_ENTRY(te_l2tp_option) list;
    enum l2tp_option_type       type;   /**< PPP|L2TP options */
    char                       *name;   /**< Option name */
    char                       *value;  /**< Option value */
    te_l2tp_secret             *secret; /**< CHAP|PAP secret */
} te_l2tp_option;

/** The section of L2TP config file */
typedef struct te_l2tp_section {
    SLIST_ENTRY(te_l2tp_section) list;
    SLIST_HEAD(, te_l2tp_option) l2tp_option;   /**< Options of the section */

    char                        *secname;       /**< Section name */

} te_l2tp_section;

/** L2TP server configuration structure */
typedef struct te_l2tp_server {
    SLIST_HEAD(, te_l2tp_section)  section;       /**< Section of the L2TP server structure */
    te_bool                        initialised;   /**< Structure initialised flag */
    te_bool                        started;       /**< Admin status for pppoe server */
    te_bool                        changed;       /**< Configuration changed flag, used to detect
                                                       if L2TP-server restart is required */
} te_l2tp_server;

static te_bool
l2tp_is_running(te_l2tp_server *l2tp);

/** Static L2TP server structure */
static te_l2tp_server l2tp_server;

/**
 * Initialise L2TP server structure with default values
 *
 * @param l2tp         L2TP server structure to initialise
 *
 * @return N/A
 */
static void
l2tp_server_init(te_l2tp_server *l2tp)
{
    ENTRY("%s()", __FUNCTION__);
    SLIST_INIT(&l2tp->section);
    l2tp->started = l2tp_is_running(l2tp);
    l2tp->changed = l2tp->started;
    l2tp->initialised = TRUE;
}

/**
 * Return pointer to static L2TP server structure
 *
 * @return L2TP server structure
 */
static te_l2tp_server *
l2tp_server_find()
{
    te_l2tp_server *l2tp = &l2tp_server;
    if (!l2tp->initialised)
        l2tp_server_init(l2tp);
    return l2tp;
}

/**
 * Prepare configuration file for L2TP server
 *
 * @param l2tp    l2tp server structure
 *
 * @return Status code
 */
static te_errno
l2tp_server_save_conf(te_l2tp_server *l2tp)
{
    FILE            *l2tp_file;
    FILE            *ppp_file;
    FILE            *chap_secret_file;
    FILE            *pap_secret_file;
    FILE            *temp;
    te_l2tp_option  *l2tp_option;
    te_l2tp_option  *ppp_option;
    te_l2tp_section *l2tp_section;
    char            *pppfilename;

    ENTRY("%s()", __FUNCTION__);
    l2tp_file = fopen(L2TP_SERVER_CONF, "w");
    chap_secret_file = fopen(L2TP_CHAP_SECRETS, "w");
    pap_secret_file = fopen(L2TP_PAP_SECRETS, "w");
    if (l2tp_file == NULL || chap_secret_file == NULL
                          || pap_secret_file == NULL)
    {
        ERROR("Failed to open files for writing: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (l2tp_section = SLIST_FIRST(&l2tp->section); l2tp_section != NULL;
            l2tp_section = SLIST_NEXT(l2tp_section, list))
    {
        fprintf(l2tp_file, "%s\n", l2tp_section);
        for (l2tp_option = SLIST_FIRST(&l2tp_section->l2tp_option);
             l2tp_option != NULL; l2tp_option = SLIST_NEXT(l2tp_option, list))
        {
            if (l2tp_option->type != L2TP_OPTION_TYPE_L2TP)
                continue;
            fprintf(l2tp_file, "%s = %s\n", l2tp_option->name,
                    l2tp_option->value);
            if (strcmp(l2tp_option->name, PPP_OPTIONS) == 0)
            {
                pppfilename = l2tp_option->name;
            }
        }
        if (pppfilename != NULL)
            ppp_file = fopen(pppfilename, "w");
        if (ppp_file == NULL)
        {
            ERROR("Failed to open '%s' for writing: %s", ppp_file,
                  strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        for (ppp_option = SLIST_FIRST(&l2tp_section->l2tp_option); ppp_option != NULL;
             ppp_option = SLIST_NEXT(ppp_option, list))
        {
            if (l2tp_option->type != (L2TP_OPTION_TYPE_PPP || L2TP_OPTION_TYPE_L2TP))
            {
                switch (l2tp_option->secret->type)
                {
                    case L2TP_SECRET_PROT_CHAP:
                        temp = chap_secret_file;
                        break;
                    case L2TP_SECRET_PROT_PAP:
                        temp = pap_secret_file;
                        break;
                    default:
                        break;
                }
                if (temp != NULL)
                    fprintf(chap_secret_file,
                            "%s %s %s %s\n",
                            l2tp_option->name, l2tp_option->secret->server,
                            l2tp_option->secret->secret,
                            l2tp_option->secret->sipv4);
            }

            else if (l2tp_option->type == L2TP_OPTION_TYPE_PPP)
                fprintf(ppp_file, "%s %s\n",
                        ppp_option->name, ppp_option->value);
        }
    }

    if (fclose(l2tp_file) != 0 || fclose(ppp_file) != 0
        || fclose(chap_secret_file) != 0 || fclose(pap_secret_file) != 0)
    {
        ERROR("%s(): fclose() failed: %s", __FUNCTION__, strerror(errno));
        return  TE_OS_RC(TE_TA_UNIX, errno);
    }
    return 0;
}

/**
 * Find pid of xl2tpd's process
 *
 * @return pid number if it exists
 *         otherwise it returns 0
 */
int
l2tp_find_pid()
{
    DIR            *dir;
    FILE           *fp;
    struct dirent  *mydir;
    char           *gotname;
    char            str_buf[32];
    char            buf[32];
    int             dirnum;

    if ((dir = opendir("/proc/")) != NULL)
    {
        while ((mydir = readdir(dir)) != 0)
        {
            dirnum = atoi(mydir->d_name);
            if (dirnum == 0)
                continue;
            TE_SPRINTF(buf, "/proc/%s/stat", mydir->d_name);
            fp = fopen(buf, "r");
            if (!fp)
            {
                closedir(dir);
                return 0;
            }
            if (fgets(str_buf, sizeof(str_buf), fp) != NULL)
            {
                gotname = strtok(str_buf, "()");
                gotname = strtok(NULL, "()/");
                if (gotname == NULL)
                    break;
                if (strcmp("xl2tpd", gotname) == 0)
                {
                    closedir(dir);
                    fclose(fp);
                    return dirnum;
                }
            }
            fclose(fp);
        }
        closedir(dir);
    };
    return 0;
}

/**
 * Check if L2TP server is running
 *
 * @param l2tp  l2tp server structure
 *
 * @return l2tp server status
 */
static te_bool
l2tp_server_is_running(te_l2tp_server *l2tp)
{
    te_bool is_running;
    char buf[L2TP_CMDLINE_LENGTH];

    UNUSED(l2tp);

    is_running = (te_bool) (l2tp_find_pid() != 0);
    INFO("L2TP server is%s running", (is_running) ? "" : " not");
    return is_running;
}

/**
 * Stop l2tp server process
 *
 * @param l2tp   l2tp server structure
 *
 * @return status code
 */
static te_errno
l2tp_server_stop(te_l2tp_server *l2tp)
{
    char buf[L2TP_CMDLINE_LENGTH];

    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

    if (!l2tp_is_running(l2tp))
        return 0;
    TE_SPRINTF(buf, "kill -9 %i", l2tp_find_pid());
    if (ta_system(buf) != 0)
    {
        ERROR("Command %s failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
};

/**
 * Start l2tp server process
 *
 * @param l2tp   l2tp server structure
 *
 * @return status code
 */
static te_errno
l2tp_server_start(te_l2tp_server *l2tp)
{
    char buf[L2TP_CMDLINE_LENGTH];
    te_errno res;

    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

    res = l2tp_server_save_conf(l2tp);
    if (res != 0)
    {
        ERROR("Failed to save L2TP server configuration file");
        return res;
    }
    TE_SPRINTF(buf, "%s -c %s", L2TP_SERVER_EXEC, L2TP_SERVER_CONF);
    if (ta_system(buf) != 0)
    {
        ERROR("Command %s failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    return 0;
}

/**
 * Method to get the L2TP server status
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         L2TP server status result
 *
 * @return status code
 */
static te_errno
l2tp_server_get(unsigned int gid, const char *oid, char *value)
{
    te_l2tp_server *l2tp = l2tp_server_find();

    UNUSED(gid);
    UNUSED(oid);

    INFO("%s()", __FUNCTION__);

    TE_SPRINTF(value, "%s", l2tp_is_running(l2tp) ? "1" : "0");
    return 0;

}

/**
 * Set desired status to L2TP server
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         L2TP server status result
 *
 * @return status code
 */
static te_errno
l2tp_server_set(unsigned int gid, const char *oid, const char *value)
{
    te_l2tp_server *l2tp = l2tp_server_find();

    UNUSED(gid);
    UNUSED(oid);

    INFO("%s()", __FUNCTION__);

    l2tp->started = (te_bool) (strcmp(value, "1") == 0);
    if (l2tp->started != l2tp_is_running(l2tp));
    {
        l2tp->changed = TRUE;
    }

    return 0;
};

/**
 * Commit changes in L2TP server configuration.
 * (Re)star/stop L2TP server if required
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         L2TP server status result
 *
 * @return status code
 */
static te_errno
l2tp_server_commit(unsigned int gid, const char *oid)
{
    te_l2tp_server *l2tp = l2tp_server_find();
    te_errno        res;

    UNUSED(gid);
    UNUSED(oid);

    INFO("%s()", __FUNCTION__);

    if ((res = l2tp_server_stop(l2tp)) != 0)
    {
        ERROR("Failed to stop L2TP server");
        return res;
    }

    if (l2tp->started)
    {
        if ((res = l2tp_server_start(l2tp)) != 0)
        {
            ERROR("Failed to start L2TP server");
            return res;
        }
    }
    return 0;
}

/**
 * Return pointer to L2TP section structure
 * @param l2tp         L2TP server structure
 * @param name         Name of the section in the following
 *                     format - [lns default]|[global]
 *
 * @return L2TP section structure
 */
static te_l2tp_section *
l2tp_section_find(te_l2tp_server *l2tp, const char *name)
{
    te_l2tp_section *l2tp_section;

    for (l2tp_section = SLIST_FIRST(&l2tp->section);
         l2tp_section != NULL && strcmp(l2tp_section->secname, name) != 0;
         l2tp_section = SLIST_NEXT(l2tp_section, list));

    return l2tp_section;
}

/**
 * Find l2tp server option in options list
 *
 * @param l2tp          l2tp server structure
 * @param section       lns section where certain option is located
 * @param type          the class of the options (L2TP|PPP)
 * @param name          option name to look for
 *
 * @return l2tp server option structure
 */
static te_l2tp_option *
l2tp_find_option(te_l2tp_server *l2tp, const char *section, const char *name)
{
    te_l2tp_option  *opt;
    te_l2tp_section *sec = l2tp_section_find(l2tp, section);

    if (strcmp(sec->secname, section) != 0)
        for (opt = SLIST_FIRST(&sec->l2tp_option);
             opt != NULL && strcmp(opt->name, name) != 0;
             opt = SLIST_NEXT(opt, list));

    return opt;
}

/**
 * Return secret by client's name
 * @param l2tp         L2TP server structure
 * @param cname        Name of the client
 * @param type         CHAP|PAP secret
 *
 * @return L2TP section structure
 */
static te_l2tp_secret *
l2tp_client_find(te_l2tp_section *l2tp, const char *cname,
                 enum l2tp_secret_prot type)
{
    te_l2tp_option *client;
    te_l2tp_secret *l2tp_secret;
    for (client = SLIST_FIRST(&l2tp->l2tp_option);
         client != NULL; client = SLIST_NEXT(client, list))
    {
        if ((strcmp(client->name, cname) == 0) && (client->type == type))
            l2tp_secret = client->secret;
    }
    return l2tp_secret;
}

/**
 * Get callback for /agent/l2tp/listen or /agent/l2tp/port node.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         returned server option value
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param option        instance name to look for (listen|port)
 *
 * @return status code
 */
static te_errno
l2tp_global_opt_get(unsigned gid, const char *oid, char *value,
                    const char *l2tp_name, const char *option)
{
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_option *opt;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((opt = l2tp_find_option(l2tp, L2TP_GLOBAL, option)) != NULL)
    {
        strcpy(value, opt->value);
        return 0;
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Set callback for /agent/l2tp/listen or /agent/l2tp/port node.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         l2tp server option value
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param option        option name to modify
 *
 * @return status code
 */
static te_errno
l2tp_global_opt_set(unsigned int gid, const char *oid, const char *value,
                    const char *l2tp_name, const char *option)
{
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_option *opt;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((opt = l2tp_find_option(l2tp, L2TP_GLOBAL, option)) != NULL)
    {
        free(opt->value);
        opt->value = strdup(value);
        l2tp->changed = TRUE;
        return 0;
    }

    return TE_RC(TE_TA_UNIX,TE_ENOENT);
}


/**
 * Add callback for /agent/l2tp/lns node.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         unused value of /agent/l2tp/lns instance
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param name          lns section name to add to the list
 *
 * @return status code
 */
static te_errno
l2tp_lns_section_add(unsigned int gid, const char *oid, const char *value,
                     const char *l2tp_name, const char *name)
{
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *l2tp_section;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((l2tp_section = l2tp_section_find(l2tp, name)) != NULL)
    {
        return TE_RC(TE_TA_UNIX,  TE_EEXIST);
    }

    l2tp_section = (te_l2tp_section *)calloc(1, sizeof(te_l2tp_section));
    if (l2tp_section != NULL)
        l2tp_section->secname = strdup(name);
    else
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    SLIST_INSERT_HEAD(&l2tp->section, l2tp_section, list);
    l2tp->changed = TRUE;

    return 0;

}

/**
 * Delete callback from /agent/l2tp/lns node.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param name          lns section name to add to the list
 *
 * @return status code
 */
static te_errno
l2tp_lns_section_del(unsigned int gid, const char *oid,
                     const char *l2tp_name, const char *name)
{
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *l2tp_section;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((l2tp_section = l2tp_section_find(l2tp, name)) == NULL)
    {
        return TE_RC(TE_TA_UNIX,  TE_ENOENT);
    }
    SLIST_REMOVE(&l2tp->section, l2tp_section, te_l2tp_section, list);
    l2tp->changed = TRUE;

    return 0;
}

/**
 * List callback for /agent/l2tp/lns node.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param list          location of the lns sections list
 * @param l2tp_name     name of the l2tp instance is always empty

 * @return status code
 */
static te_errno
l2tp_lns_section_list(unsigned int gid, const char *oid,
                      char **list, const char *l2tp_name)
{
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *l2tp_section;
    uint32_t         list_size;
    uint32_t         list_len;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    list_size = L2TP_SERVER_LIST_SIZE;
    if ((*list = (char *)calloc(1, list_size)) == NULL)
    {
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    list_len = 0;

    SLIST_FOREACH(l2tp_section, &l2tp->section, list)
    {
        if (list_len + strlen(l2tp_section->secname) + 1 >= list_size)
        {
            list_size *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
            {
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }
        }

        list_len += sprintf(*list + list_len, "%s ", l2tp_section->secname);
    }

    return 0;

}

/**
 * Get callback for /agent/l2tp/lns/ip_range|lac_range|bit|auth/refuse|require.
 *              or  /agent/l2tp/lns/pppopt/mtu|mru|lcp-echo-interval
 *              |lcp-echo-failure
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         returned ip_range|bit value|
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param optname       option name to get in format '(no)ip range'
 *                      'hidden bit|length bit|flow bit'
 *                      'refuse|require chap|pap|authentication'
 *                      'unix authentication|challenge'
 * @param secname       name of the LNS
 *
 * @return status code
 */
static te_errno
l2tp_lns_option_get(unsigned int gid, const char *oid, char *value,
                    const char *l2tp_name, const char *optname,
                    const char *secname)
{
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_option *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((option = l2tp_find_option(l2tp, secname, optname)) != NULL)
    {
        strcpy(value, option->value);
        return 0;
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
};

/**
 * Set callback for /agent/l2tp/lns/ip_range|lac_range|bit|auth/refuse|require.
 *              or  /agent/l2tp/lns/pppopt/mtu|mru|lcp-echo-interval
 *              |lcp-echo-failure.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         ip_range value in format
 *                      'X.X.X.X-X.X.X.X(,Y.Y.Y.Y-Y.Y.Y.Y)'
 *                      bit value format is 'yes|no'
 *                      authentication value format is 'yes|no'
 *                      integer value for mtu|mru|lcp-echo-interval
 *                      |lcp-echo-failure
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param optname       option name to modify in format '(no)ip range'
 *                      'hidden bit|length bit|flow bit'
 *                      'refuse|require chap|pap|authentication'
 *                      'unix authentication|challenge'
 * @param secname       name of the LNS
 *
 * @return status code
 */
static te_errno
l2tp_lns_option_set(unsigned int gid, const char *oid, char *value,
                    const char *l2tp_name, const char *optname,
                    const char *secname)
{
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_option *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((option = l2tp_find_option(l2tp, secname, optname)) != NULL)
    {
        free(option->value);
        option->value = strdup(value);
        l2tp->changed = TRUE;
        return 0;
    }

    return TE_RC(TE_TA_UNIX,TE_ENOENT);
};

/**
 * Add callback for /agent/l2tp/lns/ip_range|lac_range|bit|auth/refuse|require
 *               or /agent/l2tp/pppopt/option.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         ip_range value in format
 *                      'X.X.X.X-X.X.X.X(,Y.Y.Y.Y-Y.Y.Y.Y)'
 *                      bit value format is 'yes|no'
 *                      authentication value format is 'yes|no'
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param optname       option name to modify in format '(no)ip range'
 *                      'hidden bit|length bit|flow bit'
 *                      'refuse|require chap|pap|authentication'
 *                      'unix authentication|challenge'
 *
 * @param secname       name of the LNS
 * @param option_type   L2TP option or PPP
 *
 * @return status code
 */
static te_errno
l2tp_lns_option_add(unsigned int gid, const char *oid, const char *value,
                    const char *l2tp_name, const char *optname,
                    const char *secname, enum l2tp_option_type option_type)
{
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *l2tp_section = l2tp_section_find(l2tp, secname);
    if (l2tp_section == NULL)
        return TE_RC(TE_TA_UNIX,  TE_ENOENT);
    te_l2tp_option  *l2tp_option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((l2tp_option = l2tp_find_option(l2tp, secname, optname)) != NULL)
    {
        return TE_RC(TE_TA_UNIX,  TE_EEXIST);
    }
    l2tp_option = (te_l2tp_option *)calloc(1, sizeof(te_l2tp_option));
    if (l2tp_option == NULL);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    l2tp_option->name = strdup(optname);
    switch (option_type)
    {
        case L2TP_OPTION_TYPE_L2TP:
            l2tp_option->value = strdup(" ");
            break;
        case L2TP_OPTION_TYPE_PPP:
            l2tp_option->value = strdup(value);
            break;
        default:
            break;
    }
    SLIST_INSERT_HEAD(&l2tp_section->l2tp_option, l2tp_option, list);

    l2tp->changed = TRUE;

    return 0;
}

/**
 * Delete callback for /agent/l2tp/lns/ip_range|lac_range|bit|auth/refuse|require.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param optname       option name to modify in format '(no)ip range'
 * @param secname       name of the LNS
 * @param option_type   L2TP option or PPP
 *
 * @return status code
 */
static te_errno
l2tp_lns_option_del(unsigned int gid, const char *oid,
                    const char *optname, const char *secname)
{
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *l2tp_section;
    te_l2tp_option  *l2tp_option;

    UNUSED(gid);
    UNUSED(oid);

    if ((l2tp_section = (te_l2tp_section *)
            (l2tp_section_find(l2tp, secname) == NULL)) ||
        (l2tp_option = (te_l2tp_option *)
                (l2tp_find_option(l2tp, secname, optname) == NULL)))
    {
        return TE_RC(TE_TA_UNIX,  TE_ENOENT);
    }
    SLIST_REMOVE(&l2tp_section->l2tp_option, l2tp_option, te_l2tp_option, list);

    l2tp->changed = TRUE;
    return 0;

}

/**
 * Add callback for /agent/l2tp/lns/auth/client
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         unused value of /agent/l2tp/lns/auth/client
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param cname         client name to add to the list
 * @param type          CHAP|PAP secret
 * @param secname       name of the LNS
 *
 * @return status code
 */
static te_errno
l2tp_lns_client_add(unsigned int gid, const char *oid, char *value,
                    const char *l2tp_name, const char *cname,
                    enum l2tp_secret_prot type, const char *secname)
{
    te_l2tp_secret *secret;
    te_l2tp_option *client;
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_section *section = l2tp_section_find(l2tp, secname);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(l2tp_name);

    if ((client = l2tp_find_option(l2tp, secname, cname)) != NULL)
    {
        return TE_RC(TE_TA_UNIX,  TE_EEXIST);
    }

    client = (te_l2tp_option *)calloc(1, sizeof(te_l2tp_option));
    if (client == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    client->name = strdup(cname);
    client->secret = (te_l2tp_secret *)malloc(sizeof(te_l2tp_secret));
    if (client->secret == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    client->secret->type = type;
    client->secret->secret = strdup("*");
    client->secret->server = strdup("*");
    client->secret->sipv4 = strdup("*");
    SLIST_INSERT_HEAD(&section->l2tp_option, client, list);
    l2tp->changed = TRUE;
    return 0;
};

/**
 * Delete callback for /agent/l2tp/lns/auth/client
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param cname         client name to delete from the list
 * @param type          CHAP|PAP secret
 * @param secname       name of the LNS
 *
 * @return status code
 */
static te_errno
l2tp_lns_client_del(unsigned int gid, const char *oid,
                    const char *l2tp_name, const char *cname,
                    enum l2tp_secret_prot type, const char *secname)
{
    te_l2tp_option *client;
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_section *section = l2tp_section_find(l2tp, secname);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((client = l2tp_find_option(l2tp, secname, cname)) == NULL)
    {
        return TE_RC(TE_TA_UNIX,  TE_ENOENT);
    }
    SLIST_REMOVE(&section->l2tp_option, client, te_l2tp_option, list);
    l2tp->changed = TRUE;

    return 0;
}

/**
 * Get callback for /agent/l2tp/lns/auth/client/secret|ipv4|server
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         client's secret|ipv4|server
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param cname         client name to modify
 * @param type          CHAP|PAP secret
 * @param secname       name of the LNS
 * @param field         type of the secret file's field
 *
 * @return status code
 */
static te_errno
l2tp_lns_secret_get(unsigned int gid, const char *oid, char *value,
                    const char *l2tp_name, const char *cname,
                    enum l2tp_secret_prot type, const char *secname,
                    enum l2tp_secret_field field)
{
    te_l2tp_secret *secret;
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_section *section = l2tp_section_find(l2tp, secname);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((secret = l2tp_client_find(section, cname, type)) != NULL)
    {
        switch (field)
        {
            case L2TP_SECRET_FIELD_SECRET:
                strcpy(value, secret->secret);
                break;
            case L2TP_SECRET_FIELD_SERVER:
                strcpy(value, secret->server);
                break;
            case L2TP_SECRET_FIELD_IPV4:
                strcpy(value, secret->sipv4);
                break;
            default:
                break;
        }
        return 0;

    }

    return TE_RC(TE_TA_UNIX,TE_ENOENT);
};

/**
 * Set callback for /agent/l2tp/lns/auth/client/secret|ipv4|server
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param value         client's secret|ipv4|server
 * @param l2tp_name     name of the l2tp instance is always empty
 * @param cname         client name to modify
 * @param type          CHAP|PAP secret
 * @param secname       name of the LNS
 * @param field         type of the secret file's field
 *
 * @return status code
 */
static te_errno
l2tp_lns_secret_set(unsigned int gid, const char *oid, const char *value,
                    const char *l2tp_name, const char *cname,
                    enum l2tp_secret_prot type, const char *secname,
                    enum l2tp_secret_field field)
{
    te_l2tp_secret *secret;
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_section *section = l2tp_section_find(l2tp, secname);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((secret = l2tp_client_find(section, cname, type)) != NULL)
    {
        switch (field)
        {
            case L2TP_SECRET_FIELD_SECRET:
                free(secret->secret);
                secret->secret = strdup(value);
                break;
            case L2TP_SECRET_FIELD_SERVER:
                free(secret->server);
                secret->server = strdup(value);
                break;
            case L2TP_SECRET_FIELD_IPV4:
                free(secret->sipv4);
                secret->sipv4 = strdup(value);
                break;
            default:
                break;
        }
        l2tp->changed;
        return 0;
    }
    return TE_RC(TE_TA_UNIX,TE_ENOENT);
};

/**
 * List callback for /agent/l2tp/lns/connected node.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param list          location of the lns sections list
 * @param l2tp_name     name of the l2tp instance is always empty

 * @return status code
 */
static te_errno
l2tp_lns_connected_list(unsigned int gid, const char *oid,
                        char **list, const char *l2tp_name)
{
    //ifconfig | tee /tmp/file.diff
}

static rcf_pch_cfg_object node_l2tp;

static rcf_pch_cfg_object node_l2tp_listen =
        { "listen", 0, NULL, NULL,
          (rcf_ch_cfg_get)l2tp_global_opt_get,
          (rcf_ch_cfg_set)l2tp_global_opt_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_port =
        { "port", 0, NULL, &node_l2tp_listen,
          (rcf_ch_cfg_get)l2tp_global_opt_get,
          (rcf_ch_cfg_set)l2tp_global_opt_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_ip_range =
        { "ip_range", 0, NULL, NULL,
           (rcf_ch_cfg_get)l2tp_lns_option_get,
           (rcf_ch_cfg_set)l2tp_lns_option_set,
           (rcf_ch_cfg_add)l2tp_lns_option_add,
           (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_lac_range =
        { "lac_range", 0, NULL, &node_l2tp_lns_ip_range,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_connected =
        { "connected", 0, NULL, &node_l2tp_lns_ip_range,
          NULL, NULL, NULL, NULL,
          (rcf_ch_cfg_list)l2tp_lns_connected_list,
          NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_bit =
        { "bit", 0, NULL, &node_l2tp_lns_lac_range,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns =
        { "lns", 0, &node_l2tp_connected, &node_l2tp_port,
          NULL, NULL,
          (rcf_ch_cfg_add)l2tp_lns_section_add,
          (rcf_ch_cfg_del)l2tp_lns_section_del,
          (rcf_ch_cfg_list)l2tp_lns_section_list, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_refuse =
        { "refuse", 0, NULL, NULL,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_require =
        { "require", 0, NULL, NULL,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };


static rcf_pch_cfg_object node_l2tp_lns_auth =
        { "auth", 0, &node_l2tp_lns_require, &node_l2tp_lns_bit,
          NULL, NULL, NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_unix_auth =
        { "unix_auth", 0, NULL, &node_l2tp_lns_auth,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_challenge =
        { "challenge", 0, NULL, &node_l2tp_lns_unix_auth,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_mtu =
        { "mtu", 0, NULL, NULL,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          NULL, NULL, NULL, NULL, &node_l2tp};

static rcf_pch_cfg_object node_l2tp_lns_ppp =
        { "pppopt", 0, &node_l2tp_lns_mtu, &node_l2tp_lns_challenge,
          NULL, NULL, NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_mru =
        { "mru", 0, NULL, &node_l2tp_lns_mtu,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_einterval =
        { "lcp-echo-interval", 0, NULL, &node_l2tp_lns_mru,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_efailure =
        { "lcp-echo-failure", 0, NULL, &node_l2tp_lns_einterval,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_pppoption =
        { "option", 0, NULL, &node_l2tp_lns_efailure,
          NULL, NULL,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_sserver =
        { "server",  0, NULL, NULL,
          (rcf_ch_cfg_get)l2tp_lns_secret_get,
          (rcf_ch_cfg_set)l2tp_lns_secret_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_sipv4 =
        { "ipv4",  0, NULL, &node_l2tp_lns_sserver,
          (rcf_ch_cfg_get)l2tp_lns_secret_get,
          (rcf_ch_cfg_set)l2tp_lns_secret_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_ssecret =
        { "secret",  0, NULL, &node_l2tp_lns_sipv4,
          (rcf_ch_cfg_get)l2tp_lns_secret_get,
          (rcf_ch_cfg_set)l2tp_lns_secret_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_sclient =
        { "client", 0, &node_l2tp_lns_ssecret, &node_l2tp_lns_refuse,
          NULL, NULL,
          (rcf_ch_cfg_add)l2tp_lns_client_add,
          (rcf_ch_cfg_del)l2tp_lns_client_del, NULL, NULL, &node_l2tp };
