#include "logger_api.h"
#include "conf_daemons.h"
#include "te_queue.h"
#include "te_errno.h"
#include "rcf_ch_api.h"

/** L2TP global section name */
#define L2TP_GLOBAL           "[global]"

/** L2TP executable file */
#define L2TP_SERVER_EXEC      "/etc/init.d/xl2tpd"

/** L2TP config file */
#define L2TP_SERVER_CONF      "/usr/sbin/xl2tpd.conf"

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
};

/** CHAP|PAP secret structure */
typedef struct te_l2tp_secret {
    SLIST_ENTRY(te_l2tp_secret) list;
    enum l2tp_secret_prot       type;   /**< CHAP|PAP secret */
    char                       *sipv4;  /**< IP address in human dot notation*/
    char                       *client; /**< Client name */
    char                       *server; /**< Server name */
    char                       *secret; /**< Secret name */
} te_l2tp_secret;

/** Options for L2TP config file */
typedef struct te_l2tp_option {
    SLIST_ENTRY(te_l2tp_option) list;
    char                       *name;  /**< Option name */
    char                       *value; /**< Option value */
} te_l2tp_option;

/** The section of L2TP config file */
typedef struct te_l2tp_section {
    SLIST_ENTRY(te_l2tp_section) list;
    SLIST_HEAD(, te_l2tp_option) l2tp_option; /**< L2TP options of the section */
    SLIST_HEAD(, te_l2tp_option) ppp_option;  /**< PPP options of the section */
    char                        *secname;     /**< Section name */

} te_l2tp_section;

/** L2TP server configuration structure */
typedef struct te_l2tp_server {
    SLIST_HEAD(, te_l2tp_section)  section;       /**< Section of the L2TP server structure */
    SLIST_HEAD(, te_l2tp_secret)   secret_option; /**< CHAP|PAP secrets for the authentication */
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
    INFO("%s()", __FUNCTION__);
    SLIST_INIT(&l2tp->section);
    SLIST_INIT(&l2tp->secret_option);
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
    te_l2tp_option  *l2tp_option;
    te_l2tp_option  *ppp_option;
    te_l2tp_secret  *l2tp_secret;
    te_l2tp_section *l2tp_section;
    char            *pppfilename;


    INFO("%s()", __FUNCTION__);
    l2tp_file = fopen(L2TP_SERVER_CONF, "w");
    chap_secret_file = fopen(L2TP_CHAP_SECRETS, "w");
    pap_secret_file = fopen(L2TP_PAP_SECRETS, "w");
    if (l2tp_file == NULL || chap_secret_file == NULL
                          || pap_secret_file == NULL)
    {
        ERROR("Failed to open files for writing: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    //THERE MUST BE DEFAULT OPTIONS ADDING????

    for (l2tp_section = SLIST_FIRST(&l2tp->section); l2tp_section != NULL;
            l2tp_section = SLIST_NEXT(l2tp_section, list))
    {
        fprintf(l2tp_file, "%s", l2tp_section);
        for (l2tp_option = SLIST_FIRST(&l2tp_section->l2tp_option);
             l2tp_option != NULL; l2tp_option = SLIST_NEXT(l2tp_option, list))
        {
            fprintf(l2tp_file, "%s = %s", l2tp_option->name,
                    l2tp_option->value);
            if (strcmp(l2tp_option->name, PPP_OPTIONS) == 0)
            {
                pppfilename = l2tp_option->name;
            }
        }
        ppp_file = fopen(pppfilename, "w");
        if (ppp_file == NULL)
        {
            ERROR("Failed to open '%s' for writing: %s", ppp_file,
                  strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        for (ppp_option = SLIST_FIRST(&l2tp_section->ppp_option); ppp_option != NULL;
             ppp_option = SLIST_NEXT(ppp_option, list))
        {
            fprintf(l2tp_file, "%s %s", ppp_option->name, ppp_option->value);
        }

    }

    for (l2tp_secret = SLIST_FIRST(&l2tp->secret_option); l2tp_secret != NULL;
            l2tp_secret = SLIST_NEXT(l2tp_secret, list))
    {
        if (l2tp_secret->type == L2TP_SECRET_PROT_CHAP)
        {
            fprintf(chap_secret_file,
                    "%s         %s        %s        %s",
                    l2tp_secret->client, l2tp_secret->server,
                    l2tp_secret->secret, l2tp_secret->sipv4);
        }
        else if (l2tp_secret->type == L2TP_SECRET_PROT_PAP)
        {
            fprintf(pap_secret_file,
                    "%s         %s        %s        %s",
                    l2tp_secret->client, l2tp_secret->server,
                    l2tp_secret->secret, l2tp_secret->sipv4);
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

//CHANGE
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

    sprintf(buf, " | grep -v grep | grep -q %s >dev/null 2>&1",
               L2TP_SERVER_EXEC);
    is_running = (ta_system(buf) == 0);
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
    TE_SPRINTF(buf, "%s stop", L2TP_SERVER_EXEC);
    if (ta_system(buf) != 0)
    {
        ERROR("Command %s failed", buf);
        //???
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
    TE_SPRINTF(buf, "%s start", L2TP_SERVER_EXEC);
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

    l2tp->started = (strcmp(value, "1") == 0);
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
l2tp_find_option(te_l2tp_server *l2tp, const char *section,
                 enum l2tp_option_type type, const char *name)
{
    te_l2tp_option  *opt;
    te_l2tp_section *sec;

    for (sec = SLIST_FIRST(&l2tp->section); sec != NULL;
         sec = SLIST_NEXT(sec, list))
    {
        if (strcmp(sec, section) != 0)
            switch (type)
            {
                case L2TP_OPTION_TYPE_L2TP:
                    for (opt = SLIST_FIRST(&sec->l2tp_option);
                         opt != NULL && strcmp(opt->name, name) != 0;
                         opt = SLIST_NEXT(opt, list));
                    break;
                case L2TP_OPTION_TYPE_PPP:
                    for (opt = SLIST_FIRST(&sec->ppp_option);
                         opt != NULL && strcmp(opt->name, name) != 0;
                         opt = SLIST_NEXT(opt, list));
                    break;
                default:
                    break;
            }

    };

    return opt;
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
l2tp_global_opt_get(unsigned gid, const char *oid, const char *value,
                    const char *l2tp_name, const char *option)
{
    te_l2tp_server *l2tp = l2tp_server_find();
    te_l2tp_option *opt;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(l2tp_name);

    if ((opt = l2tp_find_option(l2tp, L2TP_GLOBAL, L2TP_OPTION_TYPE_L2TP,
                                option)) != NULL)
    {
        strcpy(value, opt->value);
        return 0;
    }

    return TE_RC(TE_TA_UNIX,TE_ENOENT);
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

    if ((opt = l2tp_find_option(l2tp, L2TP_GLOBAL, L2TP_OPTION_TYPE_L2TP,
                                option)) != NULL)
    {
        free(opt->value);
        opt->value = strdup(value);
        l2tp->changed = TRUE;
        return 0;
    }

    return TE_RC(TE_TA_UNIX,TE_ENOENT);
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

    l2tp_section = (te_l2tp_section *)malloc(sizeof(te_l2tp_section));
    memset(l2tp_section, '\0', sizeof(sizeof(l2tp_section)));
    l2tp_section->secname = strdup(name);

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

static te_errno
l2tp_ip_range_get()
{

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

static rcf_pch_cfg_object node_l2tp_lns_ip;

static rcf_pch_cfg_object node_l2tp_lns =
        { "lns", 0, &node_l2tp_lns_ip, &node_l2tp_port,
         NULL, NULL,
         (rcf_ch_cfg_add)l2tp_lns_section_add,
         (rcf_ch_cfg_del)l2tp_lns_section_del,
         (rcf_ch_cfg_list)l2tp_lns_section_list, NULL, &node_l2tp };

static rcf_pch_cfg_object node_l2tp_lns_ip_range;
static rcf_pch_cfg_object node_l2tp_lns_lac_range;
static rcf_pch_cfg_object node_l2tp_connected;
static rcf_pch_cfg_object node_l2tp_lns_bit;
static rcf_pch_cfg_object node_l2tp_lns_auth;
static rcf_pch_cfg_object node_l2tp_lns_refuse;
static rcf_pch_cfg_object node_l2tp_lns_require;
static rcf_pch_cfg_object node_l2tp_lns_sclient;
static rcf_pch_cfg_object node_l2tp_lns_sserver;
static rcf_pch_cfg_object node_l2tp_lns_ssecret;
static rcf_pch_cfg_object node_l2tp_lns_sipv4;
static rcf_pch_cfg_object node_l2tp_lns_rauth;
static rcf_pch_cfg_object node_l2tp_lns_ppp;
static rcf_pch_cfg_object node_l2tp_lns_mtu;
static rcf_pch_cfg_object node_l2tp_lns_mru;
static rcf_pch_cfg_object node_l2tp_lns_einterval;
static rcf_pch_cfg_object node_l2tp_lns_efailure;
static rcf_pch_cfg_object node_l2tp_lns_pppopt;

