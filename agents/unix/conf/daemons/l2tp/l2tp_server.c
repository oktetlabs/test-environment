#include "logger_api.h"
#include "conf_daemons.h"
#include "te_queue.h"
#include "te_errno.h"
#include "rcf_ch_api.h"

#define L2TP_SERVER_EXEC "/etc/init.d/xl2tpd"
#define L2TP_SERVER_CONF "/etc/xl2tpd/xl2tpd.conf"
#define PPP_OPTIONS "pppoptfile"
#define L2TP_CMDLINE_LENGTH 20

typedef struct te_l2tp_option {
    SLIST_ENTRY(te_l2tp_option) list;
    char                       *name;
    char                       *value;
} te_l2tp_option;

typedef struct te_l2tp_section {
    SLIST_ENTRY(te_l2tp_section) list;
    SLIST_HEAD(, te_l2tp_option) options;
    char *secname;

} te_l2tp_section;

typedef struct te_l2tp_server {
    SLIST_HEAD(, te_l2tp_section)  section;
    SLIST_HEAD(, te_l2tp_option)   l2tp_option;
    SLIST_HEAD(, te_l2tp_option)   ppp_option;
    te_bool                        initialised;
    te_bool                        started;
    te_bool                        changed;
} te_l2tp_server;

static te_bool
l2tp_is_runnning(te_l2tp_server *l2tp);

static te_l2tp_server l2tp_server;

static void
l2tp_server_init(te_l2tp_server *l2tp)
{
    INFO("%s()", __FUNCTION__);

    SLIST_INIT(&l2tp->section);
    SLIST_INIT(&l2tp->l2tp_option);
    SLIST_INIT(&l2tp->ppp_option);
    l2tp->started = l2tp_is_runnning(l2tp);
    l2tp->changed = l2tp->started;
    l2tp->initialised = TRUE;
}

static te_l2tp_server *
l2tp_server_find()
{

}

static te_errno
l2tp_server_save_conf(te_l2tp_server *l2tp)
{
    FILE            *l2tp_file;
    FILE            *ppp_file;
    te_l2tp_option  *l2tp_option;
    te_l2tp_option  *ppp_option;
    te_l2tp_section *l2tp_section;
    char            *pppfilename;


    INFO("%s()", __FUNCTION__);
    l2tp_file = fopen(L2TP_SERVER_CONF, "w");
    if (l2tp_file == NULL)
    {
        ERROR("Failed to open '%s' for writing: %s", L2TP_SERVER_CONF,
              strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    for (l2tp_section = SLIST_FIRST(&l2tp->section); l2tp_section != NULL;
            l2tp_section = SLIST_NEXT(l2tp_section, list))
    {
        fprintf(l2tp_file, "%s", l2tp_section);
        for (l2tp_option = SLIST_FIRST(&l2tp->l2tp_option); l2tp_option != NULL;
             l2tp_option = SLIST_NEXT(l2tp_option, list))
        {
            fprintf(l2tp_file, "%s = %s", l2tp_option->name, l2tp_option->value);
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
        for (ppp_option = SLIST_FIRST(&l2tp->ppp_option); ppp_option != NULL;
             ppp_option = SLIST_NEXT(ppp_option, list))
        {
            fprintf(l2tp_file, "%s %s", ppp_option->name, ppp_option->value);
        }

    }
    if ((fclose(l2tp_file) != 0) || (fclose(ppp_file) != 0))
    {
        ERROR("%s(): fclose() failed: %s", __FUNCTION__, strerror(errno));
        return  TE_OS_RC(TE_TA_UNIX, errno);
    }
    return 0;
}

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


static te_errno
l2tp_listen_get(unsigned gid, const char *oid, char *value,
                const char *l2tp_name, const char *ip_addr)
{

}

static rcf_pch_cfg_object node_l2tp;
static rcf_pch_cfg_object node_l2tp_listen;
static rcf_pch_cfg_object node_l2tp_port;
static rcf_pch_cfg_object node_l2tp_lns;
static rcf_pch_cfg_object node_l2tp_lns_ip;
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
