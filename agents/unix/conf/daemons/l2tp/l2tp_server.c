#include <dirent.h>
#include <te_shell_cmd.h>
#include <ifaddrs.h>
#include <regex.h>
#include "conf_daemons.h"
#include "te_queue.h"
#include "te_string.h"

/** Pattern to match strings like XXX.XXX.XXX.XXX
 *  where X - number from 0 to 9
 */
#define L2TP_SEMI_IP_PATTERN "([0-9]{1,3})\.([0-9]{1,3})\." \
                             "([0-9]{1,3})\.([0-9]{1,3})"

/** L2TP global section name */
#define L2TP_GLOBAL            "global"

/** L2TP executable file */
#define L2TP_SERVER_EXEC       "/usr/sbin/xl2tpd"

/** Part of the L2TP config file's name */
#define L2TP_SERVER_CONF_BASIS "/tmp/te.xl2tpd.conf"

/** L2TP TA pid file */
#define L2TP_TA_PIDFILE       "/tmp/te.xl2tpd.pid"

/** L2TP pid file */
#define L2TP_SERVER_PIDFILE   "/var/run/xl2tpd/xl2tpd.pid"

/** L2TP script */
#define L2TP_INIT_SCRIPT      "/etc/init.d/xl2tpd"

/** CHAP secrets file */
#define L2TP_CHAP_SECRETS     "/etc/ppp/chap-secrets"

/** PAP secrets file */
#define L2TP_PAP_SECRETS      "/etc/ppp/pap-secrets"

/** Name of the option in L2TP config file */
#define PPP_OPTIONS            "pppoptfile"

/** Marker which isolate the L2TP secrets from others */
#define L2TP_FENCE            "#L2TP secret tests\n"

/**
 * Default amount of memory allocated for list methods
 * of l2tp subtreee
 */
#define L2TP_SERVER_LIST_SIZE 1024

/** Default buffer size for command-line construction */
#define L2TP_CMDLINE_LENGTH   1024

/** Default size for the pid storage*/
#define L2TP_MAX_PID_LENGTH 5

/**< pid of xl2tpd */
static int l2tp_pid = -1;

/** Authentication type */
enum l2tp_secret_prot {
    L2TP_SECRET_PROT_CHAP,    /**< CHAP authentication */
    L2TP_SECRET_PROT_PAP      /**< PAP authentication */
};

/** The class of the options */
enum l2tp_option_type {
    L2TP_OPTION_TYPE_PPP,    /**< PPP options class */
    L2TP_OPTION_TYPE_L2TP,   /**< L2TP options class */
    L2TP_OPTION_TYPE_SECRET, /**< SECRET options */
};

/** CHAP|PAP secret's field */
enum l2tp_secret_field {
    L2TP_SECRET_FIELD_SERVER,   /**< Server field */
    L2TP_SECRET_FIELD_SECRET,   /**< Secret field  */
    L2TP_SECRET_FIELD_IPV4,     /**< IPv4 field  */
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

typedef struct te_l2tp_connected {
    SLIST_ENTRY(te_l2tp_connected)  list;  /**< Next element in list */
    char                           *cname; /**< Client name */
} te_l2tp_connected;

/** L2TP server configuration structure */
typedef struct te_l2tp_server {
    SLIST_HEAD(, te_l2tp_section)   section;     /**< Section of the L2TP
                                                      server structure */
    SLIST_HEAD(, te_l2tp_connected) client;      /**< Connected clients to
                                                      the L2TP server */
    te_bool                         initialized; /**< Flag to determine whether
                                                      the structure is initialized */
    te_bool                         started;     /**< Admin status for the
                                                      L2TP server */
    te_bool                         changed;     /**< Configuration changed flag,
                                                      used to detect if
                                                      L2TP-server restart
                                                      is required */
} te_l2tp_server;

static te_bool
l2tp_is_running(te_l2tp_server *l2tp);

/** Static L2TP server structure */
static te_l2tp_server l2tp_server;

/**
 * Initialize L2TP server structure with default values
 *
 * @param l2tp         L2TP server structure to initialize
 *
 */
static void
l2tp_server_init(te_l2tp_server *l2tp)
{
    ENTRY("%s()", __FUNCTION__);
    SLIST_INIT(&l2tp->section);
    SLIST_INIT(&l2tp->client);
    l2tp->started = l2tp_is_running(l2tp);
    l2tp->changed = l2tp->started;
    l2tp->initialized = TRUE;
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
    if (!l2tp->initialized)
        l2tp_server_init(l2tp);
    return l2tp;
}

/**
 * Recover the original CHAP|PAP secrets file
 *
 * @param outname     L2TP_CHAP_SECRETS or L2TP_PAP_SECRETS
 *
 * @return Status code
 */
static te_errno
l2tp_secrets_recover(char *secretfname)
{
    char    *backupfname;
    char    *directory = "/etc/ppp";
    char    *secprefix = "sec";
    char     line [256];
    FILE*    secrets_file;
    FILE*    backup_file;

    backupfname = tempnam(directory, secprefix);

    secrets_file = fopen(secretfname, "r");
    if(secrets_file == NULL)
    {
        ERROR("Failed to open file: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    backup_file = fopen(backupfname, "w");
    if( backup_file == NULL)
    {
        if (fclose(backup_file) != 0)
        {
            ERROR("%s(): fclose() failed: %s",
                  __FUNCTION__, strerror(errno));

            return  TE_OS_RC(TE_TA_UNIX, errno);
        }
        ERROR("Failed to open file: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    while( fgets(line, sizeof(line), secrets_file) != NULL )
    {
        if ((strcmp(line, L2TP_FENCE) != 0))
        {
            fputs(line, backup_file);
        }
    }

    if (fclose(backup_file) != 0)
    {
        ERROR("%s(): fclose() failed: %s",
              __FUNCTION__, strerror(errno));

        return  TE_OS_RC(TE_TA_UNIX, errno);
    }

    if (fclose(secrets_file) != 0)
    {
        ERROR("%s(): fclose() failed: %s",
              __FUNCTION__, strerror(errno));

        return  TE_OS_RC(TE_TA_UNIX, errno);
    }

    if (rename(backupfname, secretfname) != 0)
    {
        ERROR("Failed to rename %s to %s: %s",
              backupfname, secretfname, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
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
    FILE            *chap_secret_file;
    FILE            *pap_secret_file;
    FILE            *l2tp_file;
    FILE            *ppp_file = NULL;
    te_l2tp_option  *l2tp_option;
    te_l2tp_option  *ppp_option;
    te_l2tp_section *l2tp_section;
    char            *pppfilename = NULL;
    char            *l2tp_conf = malloc(strlen(L2TP_SERVER_CONF_BASIS
                                               + L2TP_MAX_PID_LENGTH));

    ENTRY("%s()", __FUNCTION__);
    TE_SPRINTF(l2tp_conf, L2TP_SERVER_CONF_BASIS ".%i", (int) getpid());

    l2tp_file = fopen(l2tp_conf, "w");
    if (l2tp_file == NULL)
    {
        ERROR("Failed to open '%s' file for writing: %s",
              l2tp_conf, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    chap_secret_file = fopen(L2TP_CHAP_SECRETS, "a+");
    if (chap_secret_file == NULL)
    {
        if (fclose(l2tp_file) != 0)
        {
            ERROR("%s(): fclose() failed: %s",
                  __FUNCTION__, strerror(errno));

            return  TE_OS_RC(TE_TA_UNIX, errno);
        }
        ERROR("Failed to open '%s' file for writing: %s",
              L2TP_CHAP_SECRETS, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    pap_secret_file = fopen(L2TP_PAP_SECRETS, "a+");
    if (pap_secret_file == NULL)
    {
        if (fclose(l2tp_file) != 0 ||
            fclose(chap_secret_file) != 0)
        {
            ERROR("%s(): fclose() failed: %s",
                  __FUNCTION__, strerror(errno));

            return  TE_OS_RC(TE_TA_UNIX, errno);
        }
        ERROR("Failed to open '%s' file for writing: %s",
              L2TP_PAP_SECRETS, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    /** Add a line to indicate the beginning of the secrets for L2TP tests */
    fprintf(chap_secret_file, "\n%s", L2TP_FENCE);
    fprintf(pap_secret_file, "\n%s", L2TP_FENCE);

    for (l2tp_section = SLIST_FIRST(&l2tp->section); l2tp_section != NULL;
         l2tp_section = SLIST_NEXT(l2tp_section, list))
    {
        if (strcmp(l2tp_section->secname, L2TP_GLOBAL) == 0)
            fprintf(l2tp_file, "[ %s ]\n", l2tp_section->secname);
        else
            fprintf(l2tp_file, "[ lns %s ]\n", l2tp_section->secname);

        for (l2tp_option = SLIST_FIRST(&l2tp_section->l2tp_option);
             l2tp_option != NULL; l2tp_option = SLIST_NEXT(l2tp_option, list))
        {
            if (l2tp_option->type == L2TP_OPTION_TYPE_L2TP)
            {
                if (strcmp(l2tp_option->name, PPP_OPTIONS) == 0)
                {
                    pppfilename = l2tp_option->value;
                }
                else
                {
                    fprintf(l2tp_file, "%s = %s\n", l2tp_option->name,
                            l2tp_option->value);
                }
            }
        }

        if (pppfilename != NULL)
        {
            ppp_file = fopen(pppfilename, "w");
            if (ppp_file == NULL)
            {
                if (fclose(l2tp_file) != 0 ||
                    fclose(chap_secret_file) != 0 ||
                    fclose(pap_secret_file) != 0)
                {
                    ERROR("%s(): fclose() failed: %s",
                          __FUNCTION__, strerror(errno));

                    return  TE_OS_RC(TE_TA_UNIX, errno);
                }
                ERROR("Failed to open '%s' for writing: %s", ppp_file,
                      strerror(errno));
                return TE_OS_RC(TE_TA_UNIX, errno);
            }
        }

        for (ppp_option = SLIST_FIRST(&l2tp_section->l2tp_option);
             ppp_option != NULL;
             ppp_option = SLIST_NEXT(ppp_option, list))
        {
            FILE *temp;
            switch(ppp_option->type)
            {
                case L2TP_OPTION_TYPE_SECRET:
                {
                    switch (ppp_option->secret->type)
                    {
                        case L2TP_SECRET_PROT_CHAP:
                        {
                            temp = chap_secret_file;
                            break;
                        }

                        case L2TP_SECRET_PROT_PAP:
                        {
                            temp = chap_secret_file;
                            break;
                        }

                        default:
                            temp = NULL;
                            break;
                    }
                    if (temp != NULL)
                    {
                        fprintf(temp, "%s %s %s %s\n",
                                ppp_option->name,
                                ppp_option->secret->server,
                                ppp_option->secret->secret,
                                ppp_option->secret->sipv4);
                    }

                }
                case L2TP_OPTION_TYPE_PPP:
                {
                    fprintf(ppp_file, "%s %s\n",
                            ppp_option->name, ppp_option->value);
                    break;
                }

                default:
                    break;
            }
        }
    }
    /** Add a line to indicate the end of the secrets for L2TP tests */
    fprintf(chap_secret_file, "%s", L2TP_FENCE);
    fprintf(pap_secret_file, "%s", L2TP_FENCE);

    if (fclose(l2tp_file) != 0 || fclose(ppp_file) != 0
        || fclose(chap_secret_file) != 0 || fclose(pap_secret_file) != 0)
    {
        ERROR("%s(): fclose() failed: %s", __FUNCTION__, strerror(errno));
        return  TE_OS_RC(TE_TA_UNIX, errno);
    }
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
l2tp_is_running(te_l2tp_server *l2tp)
{
    te_bool  is_running;
    char     l2tp_ta_pidfile[128];
    FILE    *f;

    UNUSED(l2tp);

    TE_SPRINTF(l2tp_ta_pidfile, "%s%i" , L2TP_TA_PIDFILE, (int) getpid());

    if ((f = fopen(L2TP_SERVER_PIDFILE, "r")) != NULL)
    {
        if (fscanf(f, "%u", &l2tp_pid) == 0)
        {
            fclose(f);
            assert(0);
        }
        kill(l2tp_pid, SIGTERM);
        l2tp_pid = 0;
    }
    else if ((f = fopen(l2tp_ta_pidfile, "r")) != NULL)
    {
        if (fscanf(f, "%u", &l2tp_pid) == 0)
        {
            fclose(f);
            assert(0);
        }
    }
    is_running = l2tp_pid > 0;

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
    char     stop[L2TP_CMDLINE_LENGTH];

    ENTRY("%s()", __FUNCTION__);

    if (l2tp_pid > 0)
    {
        kill(l2tp_pid, SIGTERM);
        l2tp_pid = 0;
    }
    else if (l2tp_is_running(l2tp))
    {
        if (!access(L2TP_INIT_SCRIPT, X_OK))
        {
            TE_SPRINTF(stop, "%s stop", L2TP_INIT_SCRIPT);
            if (ta_system(stop) != 0)
            {
                ERROR("Command %s failed", stop);
                return TE_RC(TE_TA_UNIX, TE_ESHCMD);
            }
        }
        else
        {
            kill(l2tp_pid, SIGTERM);
            l2tp_pid = 0;
        }
    }

    l2tp_secrets_recover(L2TP_CHAP_SECRETS);
    l2tp_secrets_recover(L2TP_PAP_SECRETS);

    return 0;
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
    char     buf[L2TP_CMDLINE_LENGTH];
    te_errno res;
    int      ta_pid = (int) getpid();

    l2tp_server_stop(l2tp);

    ENTRY("%s()", __FUNCTION__);

    res = l2tp_server_save_conf(l2tp);
    if (res != 0)
    {
        ERROR("Failed to save L2TP server configuration file");
        return res;
    }

    TE_SPRINTF(buf, "%s -D -c %s%i -p %s%i", L2TP_SERVER_EXEC,
               L2TP_SERVER_CONF_BASIS, ta_pid, L2TP_TA_PIDFILE, ta_pid);
    if ((l2tp_pid = te_shell_cmd(buf, (uid_t) -1, NULL, NULL, NULL)) < 0)
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

    ENTRY("%s()", __FUNCTION__);

    strcpy(value, l2tp_is_running(l2tp) ? "1" : "0");
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

    ENTRY("%s()", __FUNCTION__);

    l2tp->started = (strcmp(value, "1") == 0);
    if (l2tp->started != l2tp_is_running(l2tp));
    {
        l2tp->changed = TRUE;
    }

    return 0;
};

/**
 * Commit changes in L2TP server configuration.
 * (Re)start/stop L2TP server if required
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

    ENTRY("%s()", __FUNCTION__);

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
         l2tp_section != NULL;
         l2tp_section = SLIST_NEXT(l2tp_section, list))
    {
        if (strcmp(l2tp_section->secname, name) == 0)
            break;
    };

    return l2tp_section;
}

/**
 * Find l2tp server option in options list
 *
 * @param l2tp          l2tp server structure
 * @param section       lns section where certain option is located
 * @param name          option name to look for
 *
 * @return l2tp server option structure
 */
static te_l2tp_option *
l2tp_find_option(te_l2tp_server *l2tp, const char *section, const char *name)
{
    te_l2tp_option  *opt = NULL;
    te_l2tp_section *sec = l2tp_section_find(l2tp, section);

    if (strcmp(sec->secname, section) == 0)
    {
        for (opt = SLIST_FIRST(&sec->l2tp_option);
             opt != NULL;
             opt = SLIST_NEXT(opt, list))
        {
            if (strcmp(opt->name, name) == 0)
                break;
        };
    }

    return opt;
}

/**
 * Find l2tp server's client in options list
 *
 * @param l2tp          l2tp server structure
 * @param section       lns section where certain option is located
 * @param name          client name to look for
 *
 * @return l2tp server option structure
 */
static te_l2tp_option *
l2tp_find_client(te_l2tp_server *l2tp, const char *section,
                 const char *name, enum l2tp_secret_prot type)
{
    te_l2tp_option  *client = NULL;
    te_l2tp_section *sec = l2tp_section_find(l2tp, section);

    if (strcmp(sec->secname, section) == 0)
    {
        for (client = SLIST_FIRST(&sec->l2tp_option);
             client != NULL;
             client = SLIST_NEXT(client, list))
        {
            if (strcmp(client->name, name) == 0
                && client->secret != NULL
                && client->secret->type == type)
            {
                break;
            }
        };
    }

    return client;
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
l2tp_find_secret(te_l2tp_section *l2tp, const char *cname,
                 enum l2tp_secret_prot type)
{
    te_l2tp_option *client;

    for (client = SLIST_FIRST(&l2tp->l2tp_option);
         client != NULL; client = SLIST_NEXT(client, list))
    {
        if ((strcmp(client->name, cname) == 0) && (client->type == type))
        {
            return client->secret;
        }
    }
    return NULL;
}

/**
 * Get method for /agent/l2tp/listen or /agent/l2tp/port node.
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
    te_l2tp_option *opt = l2tp_find_option(l2tp, L2TP_GLOBAL, option);;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if (opt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(value, opt->value);
    return 0;
}

/**
 * Set method for /agent/l2tp/listen or /agent/l2tp/port nodes.
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
    te_l2tp_option *opt  = l2tp_find_option(l2tp, L2TP_GLOBAL, option);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if (opt == NULL)
    {
        return TE_RC(TE_TA_UNIX,TE_ENOENT);
    }

    free(opt->value);
    opt->value = strdup(value);
    l2tp->changed = TRUE;
    return 0;
}


/**
 * Method for adding a section to /agent/l2tp/lns.
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
    te_l2tp_section *l2tp_section = l2tp_section_find(l2tp, name);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);
    UNUSED(value);

    if (l2tp_section != NULL)
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
 * Method for deleting a section from /agent/l2tp/lns.
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
    te_l2tp_section *l2tp_section = l2tp_section_find(l2tp, name);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if (l2tp_section == NULL)
    {
        return TE_RC(TE_TA_UNIX,  TE_ENOENT);
    }

    SLIST_REMOVE(&l2tp->section, l2tp_section, te_l2tp_section, list);
    free(l2tp_section);
    l2tp->changed = TRUE;

    return 0;
}

/**
 * Method for obtaining the list of sections from /agent/l2tp/lns.
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
    te_string        str = TE_STRING_INIT;
    uint32_t         list_size;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    list_size = L2TP_SERVER_LIST_SIZE;
    if ((*list = (char *)calloc(1, list_size)) == NULL)
    {
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    SLIST_FOREACH(l2tp_section, &l2tp->section, list)
    {
        te_string_append(&str, "%s ",l2tp_section->secname);
    }
    *list = str.ptr;
    return 0;

}

/**
 * Get method for /agent/l2tp/lns/ip_range|lac_range|bit|auth/refuse|require.
 * or /agent/l2tp/lns/pppopt/mtu|mru|lcp-echo-interval|lcp-echo-failure
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
    te_l2tp_option *option = l2tp_find_option(l2tp, secname, optname);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if (option == NULL)
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    strcpy(value, option->value);
    return 0;
};

/**
 * Set method for
 * /agent/l2tp/lns/ip_range|lac_range|bit|auth/refuse|require.
 * or /agent/l2tp/lns/pppopt/mtu|mru|lcp-echo-interval|lcp-echo-failure.
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
    te_l2tp_option *option = NULL;

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

/** Сheck the uniqueness of the pppoptfile's name
 *
 *  @param l2tp     l2tp server structure
 *  @param value    pppoptfile's name
 *
 *  @return TRUE if belong
            otherwise FALSE
 */
static te_bool
check_pppoptfile(te_l2tp_server *l2tp, const char *value)
{
    te_l2tp_section *l2tp_section;
    te_l2tp_option  *opt;

    for (l2tp_section = SLIST_FIRST(&l2tp->section);
         l2tp_section != NULL;
         l2tp_section = SLIST_NEXT(l2tp_section, list))
    {
        for (opt = SLIST_FIRST(&l2tp_section->l2tp_option);
             opt != NULL;
             opt = SLIST_NEXT(opt, list))
        {
            if ((strcmp(opt->name, PPP_OPTIONS) == 0)
                && (strcmp(opt->value, value) == 0))
                return FALSE;
        }
    }
    return TRUE;
}

/**
 * Method for adding an option to
 * /agent/l2tp/lns/ip_range|lac_range|bit|auth/refuse|require
 * or /agent/l2tp/pppopt/option.
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
    te_l2tp_option  *l2tp_option;
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *l2tp_section = l2tp_section_find(l2tp, secname);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if (strcmp(optname, PPP_OPTIONS) == 0 && !check_pppoptfile(l2tp, value))
    {
        ERROR("Attempt to assign the same pppoptfile "
              "for the different lnses: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    if (l2tp_section == NULL)
        return TE_RC(TE_TA_UNIX,  TE_ENOENT);

    if ((l2tp_option = l2tp_find_option(l2tp, secname, optname)) != NULL)
    {
        return TE_RC(TE_TA_UNIX,  TE_EEXIST);
    }
    l2tp_option = (te_l2tp_option *)calloc(1, sizeof(te_l2tp_option));
    if (l2tp_option == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    l2tp_option->name = strdup(optname);
    l2tp_option->type = option_type;
    l2tp_option->value = strdup(value);
    SLIST_INSERT_HEAD(&l2tp_section->l2tp_option, l2tp_option, list);
    l2tp->changed = TRUE;
    return 0;
}

/**
 * Method for deleting an option from
 * /agent/l2tp/lns/ip_range|lac_range|bit|auth/refuse|require.
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
    te_l2tp_section *l2tp_section = l2tp_section_find(l2tp, secname);
    te_l2tp_option  *l2tp_option  = l2tp_find_option(l2tp, secname, optname);

    UNUSED(gid);
    UNUSED(oid);

    if (l2tp_section == NULL || l2tp_option == NULL)
        return TE_RC(TE_TA_UNIX,  TE_ENOENT);

    SLIST_REMOVE(&l2tp_section->l2tp_option, l2tp_option,
                 te_l2tp_option, list);

    free(l2tp_option->name);
    free(l2tp_option->value);
    free(l2tp_option);
    l2tp->changed = TRUE;
    return 0;
}

/**
 * Method for adding a client to /agent/l2tp/lns/auth/client
 *
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
    te_l2tp_option  *client;
    te_l2tp_server  *l2tp = l2tp_server_find();
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
 * Method for deleting a client from /agent/l2tp/lns/auth/client
 *
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
    te_l2tp_option  *client;
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *section = l2tp_section_find(l2tp, secname);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((client = l2tp_find_client(l2tp, secname, cname, type)) == NULL)
    {
        return TE_RC(TE_TA_UNIX,  TE_ENOENT);
    }
    SLIST_REMOVE(&section->l2tp_option, client, te_l2tp_option, list);
    free(client->secret->secret);
    free(client->secret->server);
    free(client->secret->sipv4);
    free(client->secret);
    free(client);
    l2tp->changed = TRUE;
    return 0;
}

/**
 * Get method for /agent/l2tp/lns/auth/client/secret|ipv4|server
 *
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
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *section = l2tp_section_find(l2tp, secname);
    te_l2tp_secret  *secret = l2tp_find_secret(section, cname, type);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if (secret == NULL)
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

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
            return TE_RC(TE_TA_UNIX, TE_ELIBBAD);
    }

    return 0;
};

/**
 * Set method for /agent/l2tp/lns/auth/client/secret|ipv4|server
 *
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
    te_l2tp_server  *l2tp = l2tp_server_find();
    te_l2tp_section *section = l2tp_section_find(l2tp, secname);
    te_l2tp_secret  *secret = l2tp_find_secret(section, cname, type);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if (secret == NULL)
    {
        return TE_RC(TE_TA_UNIX,TE_ENOENT);
    }
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
    l2tp->changed = TRUE;
    return 0;
};

static te_bool
l2tp_check_range(char *range)
{
    regex_t  regex;
    int      retval;
    char    *temprange = strdup(range);
    char    *pch = strtok(temprange, ",");

    retval = regcomp(&regex, "^" L2TP_SEMI_IP_PATTERN
            "($|-" L2TP_SEMI_IP_PATTERN ")$", REG_EXTENDED);

    if (retval != 0)
    {
        regfree(&regex);
        free(temprange);
        ERROR("Could not compile regex: %s", strerror(errno));
        return FALSE;
    }

    do {
        retval = regexec(&regex, pch, 0, NULL, 0);
        if (retval != 0)
        {
            free(temprange);
            regfree(&regex);
            return FALSE;
        }
        pch = strtok(NULL, ",");

    } while (pch != NULL);


    free(temprange);
    regfree(&regex);
    return TRUE;
}

/** Helper for te_l2tp_check_accessory function
 *
 *  @param range    possible range of owning the ip
 *  @param ip       ip address for checking
 *
 *  @return         TRUE if ip belongs to range
 *                  Otherwise false
 */
te_bool
te_l2tp_ip_compare(char *range, char *ip)
{
    struct in_addr      start;
    struct in_addr      center;
    struct in_addr      end;
    char               *temp_range;
    char               *range_token;
    char               *ip_token;
    char               *end_range;

    int                 ip_len = 15;

    if (!l2tp_check_range(range))
        return FALSE;

    if (inet_aton(ip, &center) == 0)
        return FALSE;

    temp_range = strdup(range);
    range_token = strtok_r(temp_range, ",", &end_range);
    do {
        char *end_hyphen;
        if (strlen(range_token) <= ip_len)
        {
            if (inet_aton(range_token, &start) == 0)
            {
                free(temp_range);
                return FALSE;
            }
            else if (ntohl(start.s_addr) == ntohl(center.s_addr))
            {
                free(temp_range);
                return TRUE;
            }
        }
        else
        {
            ip_token = strtok_r(range_token, "-", &end_hyphen);
            if (inet_aton(ip_token, &start) == 0)
            {
                free(temp_range);
                return FALSE;
            }
            ip_token = strtok_r(NULL, "-", &end_hyphen);
            if (inet_aton(ip_token, &end) == 0)
            {
                free(temp_range);
                return FALSE;
            }
            if (ntohl(start.s_addr) <= ntohl(center.s_addr)
                && ntohl(center.s_addr) <= ntohl(end.s_addr))
            {
                free(temp_range);
                return TRUE;
            }
        }
    } while ((range_token = strtok_r(NULL, ",", &end_range)) != NULL);

    free(temp_range);
    return FALSE;
}

/** Check address belong to any ip range
 *
 *  @param l2tp     l2tp server structure
 *  @param cip      address for checking
 *
 *  @return         TRUE if belong
                    otherwise FALSE
 */
static te_bool
te_l2tp_check_accessory(te_l2tp_server *l2tp, char *cip)
{
    te_l2tp_option     *option;
    te_l2tp_section    *section;

    SLIST_FOREACH(section, &l2tp->section, list)
    {
        SLIST_FOREACH(option, &section->l2tp_option, list)
        {
            if (strcmp(option->name, "ip range") == 0 &&
                te_l2tp_ip_compare(option->value, cip))
                return TRUE;
        }
    }
    return FALSE;
}

/**
 * Check the ip address is equal to any existing local ip
 *
 * @param local_ip   testing ip
 * @param ranges     array of ip ranges
 *
 * @return TRUE if local_ip is equal to
 *         local ip from ranges' array
 *         otherwise it returns FALSE
 */
static te_bool
l2tp_check_lns_ip(char *local_ip, te_l2tp_server *l2tp)
{
    te_l2tp_section  *section;
    te_l2tp_option   *opt;

    SLIST_FOREACH(section, &l2tp->section, list)
    {
        SLIST_FOREACH(opt, &section->l2tp_option, list)
        {
            if (strcmp(opt->name, "local ip") == 0 &&
                strcmp(opt->value, local_ip) == 0)
                return TRUE;
        }
    }
    return FALSE;
};

/**
 * Add connected clients to the te_l2tp_server.
 *
 * @return status code
 */
static te_errno
te_l2tp_clients_add(te_l2tp_server *l2tp)
{
    te_l2tp_connected *client;
    struct ifaddrs    *iter;
    struct ifaddrs    *perm;
    char               cip[INET6_ADDRSTRLEN];

    if (getifaddrs(&perm))
    {
        ERROR("getifaddrs: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    else
    {
        for (iter = perm; iter != NULL; iter = iter->ifa_next)
        {
            if (iter->ifa_addr && iter->ifa_addr->sa_family == AF_INET
                && l2tp_check_lns_ip(inet_ntoa(
                   ((struct sockaddr_in *) iter->ifa_addr)->sin_addr), l2tp))
            {
                inet_ntop(AF_INET, &((struct sockaddr_in *)
                                  iter->ifa_ifu.ifu_dstaddr)->sin_addr,
                          cip, INET_ADDRSTRLEN);
                client = (te_l2tp_connected *)
                         calloc(1, sizeof(te_l2tp_connected));
                if (client == NULL)
                    return TE_RC(TE_TA_UNIX, TE_ENOMEM);
                else if (te_l2tp_check_accessory(l2tp, cip))
                {
                    client->cname = strdup(cip);
                    SLIST_INSERT_HEAD(&l2tp->client, client, list);
                }
            }
        }
    }
    freeifaddrs(perm);
    l2tp->changed = TRUE;
    return 0;
}

/**
 * Method for obtaining the list of connected clients from
 * /agent/l2tp/lns/connected.
 *
 * @param gid           group identifier
 * @param oid           full identifier of the father instance
 * @param list          location of the connected clients
 * @param l2tp_name     name of the l2tp instance is always empty

 * @return status code
 */
static te_errno
l2tp_lns_connected_list(unsigned int gid, const char *oid,
                        char **list, const char *l2tp_name)
{
    te_l2tp_connected *l2tp_connected;
    uint32_t           list_size;
    te_string          str = TE_STRING_INIT;
    te_l2tp_server    *l2tp = l2tp_server_find();

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if (te_l2tp_clients_add(l2tp) == 0)
    {
        list_size = L2TP_SERVER_LIST_SIZE;
        if ((*list = (char *)calloc(1, list_size)) == NULL)
        {
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
        SLIST_FOREACH(l2tp_connected, &l2tp->client, list)
        {
            te_string_append(&str, "%s ", l2tp_connected->cname);
        }
        *list = str.ptr;
        return 0;
    }
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}


static rcf_pch_cfg_object node_l2tp;

static rcf_pch_cfg_object node_l2tp_lns_pppoption =
        { "option", 0, NULL, NULL, NULL, NULL,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_einterval =
        { "lcp-echo-interval", 0, NULL, &node_l2tp_lns_pppoption,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_efailure =
        { "lcp-echo-failure", 0, NULL, &node_l2tp_lns_einterval,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_mru =
        { "mru", 0, NULL, &node_l2tp_lns_efailure,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_mtu =
        { "mtu", 0, NULL, &node_l2tp_lns_mru,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          NULL, NULL, NULL, NULL, &node_l2tp};

static rcf_pch_cfg_object node_l2tp_lns_ppp =
        { "pppopt", 0, &node_l2tp_lns_mtu, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_unix_auth =
        { "unix_auth", 0, NULL, &node_l2tp_lns_ppp,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_challenge =
        { "use_challenge", 0, NULL, &node_l2tp_lns_unix_auth,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
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
        { "client", 0, &node_l2tp_lns_ssecret, NULL,
          NULL, NULL,
          (rcf_ch_cfg_add)l2tp_lns_client_add,
          (rcf_ch_cfg_del)l2tp_lns_client_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_refuse =
        { "refuse", 0, NULL, &node_l2tp_lns_sclient,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_require =
        { "require", 0, NULL, &node_l2tp_lns_refuse,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_auth =
        { "auth", 0, &node_l2tp_lns_require, &node_l2tp_lns_challenge,
          NULL, NULL, NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_bit =
        { "bit", 0, NULL, &node_l2tp_lns_auth,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_local_ip =
        { "local_ip", 0, NULL, &node_l2tp_lns_bit,
          (rcf_ch_cfg_get)l2tp_global_opt_get,
          (rcf_ch_cfg_set)l2tp_global_opt_set,
          NULL, NULL, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_connected =
        { "connected", 0, NULL, &node_l2tp_lns_local_ip,
          NULL, NULL, NULL, NULL,
          (rcf_ch_cfg_list)l2tp_lns_connected_list,
          NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_lac_range =
        { "lac_range", 0, NULL, &node_l2tp_connected,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_ip_range =
        { "ip_range", 0, NULL, &node_l2tp_lns_lac_range,
          (rcf_ch_cfg_get)l2tp_lns_option_get,
          (rcf_ch_cfg_set)l2tp_lns_option_set,
          (rcf_ch_cfg_add)l2tp_lns_option_add,
          (rcf_ch_cfg_del)l2tp_lns_option_del, NULL, NULL, &node_l2tp };

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

static rcf_pch_cfg_object node_l2tp_lns =
        { "lns", 0, &node_l2tp_lns_ip_range, &node_l2tp_port,
          NULL, NULL,
          (rcf_ch_cfg_add)l2tp_lns_section_add,
          (rcf_ch_cfg_del)l2tp_lns_section_del,
          (rcf_ch_cfg_list)l2tp_lns_section_list, NULL, &node_l2tp };


static rcf_pch_cfg_object node_l2tp =
        { "l2tp", 0, &node_l2tp_lns, NULL,
          (rcf_ch_cfg_get)l2tp_server_get,
          (rcf_ch_cfg_set)l2tp_server_set,
          NULL, NULL, NULL,
          (rcf_ch_cfg_commit)l2tp_server_commit};

/**
 * Grab method for l2tp server resource
 *
 * @param name  dummy name of l2tp server
 *
 * @return status code
 */
te_errno
l2tp_grab(const char *name)
{
    te_l2tp_server *l2tp = l2tp_server_find();
    const char     *l2tp_paths[] = {L2TP_SERVER_EXEC};
    te_errno        retval = 0;

    UNUSED(name);

    INFO("%s()", __FUNCTION__);

    if (access(L2TP_SERVER_EXEC, X_OK) != 0)
    {
        ERROR("No L2TP server executable was not found");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if ((retval = rcf_pch_add_node("/agent", &node_l2tp)) != 0)
    {
        return retval;
    }

    if ((retval = l2tp_server_stop(l2tp)) != 0)
    {
        ERROR("Failed to stop L2TP server");
        rcf_pch_del_node(&node_l2tp);
        return retval;
    }

    l2tp->started = FALSE;

    return 0;
}

/**
 * Release method for l2tp server resource
 *
 * @param name  dummy name of l2tp server
 *
 * @return status code
 */
te_errno
l2tp_release(const char *name)
{
    te_l2tp_server *l2tp = l2tp_server_find();
    te_errno        retval;

    UNUSED(name);

    INFO("%s()", __FUNCTION__);

    if ((retval = rcf_pch_del_node(&node_l2tp)) != 0)
        return retval;

    if ((retval = l2tp_server_stop(l2tp)) != 0)
    {
        ERROR("Failed to stop l2tp server");
        return retval;
    }

    return 0;
}