/** @file
 * @brief Unix Test Agent
 *
 * IEEE 802.1x supplicants configuring (xsupplicant, wpa_supplicant)
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
 * @author Boris Misenov <Boris.Misenov@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef ENABLE_8021X
#include <stddef.h>
#include "conf_daemons.h"

/** Identifiers for supplicant parameters */
typedef enum {
    SP_NETWORK,             /**< Network name, usually ESSID */
    SP_METHOD,              /**< EAP method: "eap-md5", "eap-tls" etc. */
    SP_IDENTITY,            /**< EAP identity */
    SP_PROTO,               /**< Protocol: "", "WPA", "WPA2", "RSN" */
    SP_KEY_MGMT,            /**< Key management: "NONE", "WPA-PSK",
                             *   "WPA-EAP" */
    SP_WEP_KEY0,            /**< WEP keys: double-quoted ASCII strings
                             *   or strings of 10/13/16 hexdigits
                             *   without quotation */
    SP_WEP_KEY1,
    SP_WEP_KEY2,
    SP_WEP_KEY3,
    SP_WEP_TX_KEYIDX,       /**< Default WEP key index: 0..3 */
    SP_AUTH_ALG,            /**< Authentication algorithm OPEN|SHARED|LEAP */
    SP_GROUP,               /**< Group cipher: "TKIP", "CCMP", "TKIP CCMP" */
    SP_PAIRWISE,            /**< Pairwise cipher: "TKIP", "CCMP",
                             *  "TKIP CCMP" */
    SP_PSK,                 /**< Preshared key */
    SP_MD5_USERNAME,        /**< EAP-MD5 username */
    SP_MD5_PASSWORD,        /**< EAP-MD5 password */
    SP_TLS_CERT_PATH,       /**< EAP-TLS path to user certificate file */
    SP_TLS_KEY_PATH,        /**< EAP-TLS path to user private key file */
    SP_TLS_KEY_PASSWD,      /**< EAP-TLS password for user private key */
    SP_TLS_ROOT_CERT_PATH,  /**< EAP-TLS path to root certificate file */
    SP_MAX                  /**< Last value to determine the size of array */
} supp_param_t;

struct supplicant;

/** Callbacks for handling supplicant implementation */
typedef struct supplicant_impl
{
    te_bool  (*get)(const char *ifname);
    te_errno (*start)(const char *ifname, const char *confname);
    te_errno (*stop)(const char *ifname);
    te_errno (*reload)(const char *ifname);
    void     (*write_config)(FILE *f, const struct supplicant *supp);
} supplicant_impl;

/** A supplicant <-> interface correspondence */
typedef struct supplicant
{
    char              *ifname;     /**< interface name */
    char               confname[256];
                                   /**< Name of configuration file */
    te_bool            started;    /**< Supplicant was started and
                                        is supposed to be running */
    te_bool            changed;    /**< Configuration is changed
                                        but not commited into file yet */
    char              *params[SP_MAX];
                                   /**< Supplicant parameters,
                                        according to supp_param_t */
    const supplicant_impl *impl;
    struct supplicant *next;       /**< chain link */
} supplicant;

/** List of all available supplicants */
static supplicant *supplicant_list = NULL;

static const char *supp_get_param(const supplicant *supp, supp_param_t id);
static te_errno supp_set_param(supplicant *supp, supp_param_t id,
                               const char *value);
static supplicant *supp_create(const char *ifname);
static void supp_destroy(supplicant *supp);
static supplicant *supp_find(const char *ifname);
static te_errno supp_update(supplicant *supp);

extern te_errno wifi_essid_get(unsigned int gid, const char *oid,
                               char *value, const char *ifname);

/**
 * XSupplicant service control functions
 *
 * We do not use /etc/init.d/ scripts because XSupplicant is not common
 * package in all of distributions. Instead functions provided below are
 * used.
 *
 * We cannot determine pid of XSupplicant because it forks from the initial
 * process while daemonize and does not create pid files in /var/run.
 * The presence of XSupplicant is detected by Unix socket named
 *    /tmp/xsupplicant.sock.<ifname>
 * that is used by XSupplicant itself for its own IPC.
 *
 * If another instance of XSupplicant is started IPC socket may be lost
 * so additional check via 'ps' is performed.
 */

/** Prefix of XSupplicant socket name */
#define XSUPPLICANT_SOCK_NAME "/tmp/xsupplicant.sock."

/**
 * XSupplicant daemon presence check - any instance fits
 *
 * @param ifname   Name of interface where XSupplicant should listen
 *
 * @return TRUE if XSupplicant is found, FALSE otherwise.
 */
static te_bool
xsupplicant_get(const char *ifname)
{
    char buf[128];

    snprintf(buf, sizeof(buf),
             PS_ALL_COMM " | grep xsupplicant | grep -v grep | grep -q %s",
             ifname);
    if (ta_system(buf) == 0)
        return TRUE;

    return FALSE;
}

/**
 * XSupplicant daemon presence check - only active instance (that
 * owns IPC socket) fits
 *
 * @param ifname   Name of interface where XSupplicant should listen
 *
 * @return TRUE if XSupplicant is found, FALSE otherwise.
 */
static te_bool
xsupplicant_get_valid(const char *ifname)
{
    char buf[128];

    snprintf(buf, sizeof(buf), "fuser -s %s%s >/dev/null 2>&1",
             XSUPPLICANT_SOCK_NAME, ifname);
    if (ta_system(buf) == 0)
        return TRUE;

    return FALSE;
}

/**
 * XSupplicant daemon stop
 *
 * @param ifname    Name of interface listened
 *
 * @return Status code.
 */
static te_errno
xsupplicant_stop(const char *ifname)
{
    char buf[128];

    if (!xsupplicant_get(ifname))
    {
        WARN("%s: XSupplicant on %s is not running", __FUNCTION__, ifname);
        return 0;
    }
    RING("Stopping xsupplicant on %s", ifname);

    /* Kill acting instance */
    snprintf(buf, sizeof(buf),
             "fuser -k -TERM %s%s >/dev/null 2>&1 && rm -f %s%s",
             XSUPPLICANT_SOCK_NAME, ifname,
             XSUPPLICANT_SOCK_NAME, ifname);
    RING("Running '%s'", buf);
    if (ta_system(buf) != 0)
        WARN("Command '%s' failed", buf);

    /* Kill stale instances not owning IPC socket */
    if (xsupplicant_get(ifname))
    {
        snprintf(buf, sizeof(buf),
                 "kill `ps ax | grep xsupplicant | grep %s | grep -v grep"
                 "| awk ' { print $1 }'`", ifname);
        if (ta_system(buf) != 0)
            WARN("Command '%s' failed", buf);
    }
    return 0;
}

/**
 * XSupplicant daemon start
 *
 * @param ifname      Name of interface to listen
 * @param conf_fname  Name of configuration file
 *
 * @return Status code.
 */
static te_errno
xsupplicant_start(const char *ifname, const char *conf_fname)
{
    char  buf[128];

    RING("%s('%s', '%s')", __FUNCTION__, ifname, conf_fname);
    if (xsupplicant_get(ifname))
    {
        if (xsupplicant_get_valid(ifname))
        {
            WARN("%s: XSupplicant on %s is already running, doing nothing",
                 __FUNCTION__, ifname);
            return 0;
        }
        else
        {
            WARN("%s: XSupplicant on %s is already running, but seems "
                 "not valid, restarting",
                 __FUNCTION__, ifname);
            xsupplicant_stop(ifname);
        }
    }
    RING("Starting xsupplicant on %s", ifname);
    snprintf(buf, sizeof(buf), "xsupplicant -i %s -c %s -dA >/dev/null 2>&1", 
             ifname, conf_fname);
    if (ta_system(buf) != 0)
    {
        ERROR("Command <%s> failed", buf);
        return TE_ESHCMD;
    }
    if (!xsupplicant_get(ifname))
    {
        ERROR("Failed to start XSupplicant on %s", ifname);
        return TE_EFAIL;
    }
    return 0;
}

static void
xsupplicant_write_config(FILE *f, const supplicant *supp)
{
    const char template[] = 
        "network_list = all\n"
        "default_netname = %s\n"
        "logfile = /tmp/te_supp_%s.log\n"
        "%s {\n"
        "  identity = \"%s\"\n"
        "  allow_types = %s\n"
        "  eap-md5 {\n"
        "    username = \"%s\"\n"
        "    password = \"%s\"\n"
        "  }\n"
        "  eap-tls {\n"
        "    user_cert = \"%s\"\n"
        "    user_key = \"%s\"\n"
        "    user_key_pass = \"%s\"\n"
        "    root_cert = \"%s\"\n"
        "  }\n"
        "}\n";
    const char *method = supp_get_param(supp, SP_METHOD);

    fprintf(f, template,
            supp_get_param(supp, SP_NETWORK),
            supp->ifname,
            supp_get_param(supp, SP_NETWORK),
            supp_get_param(supp, SP_IDENTITY),
            method[0] == '\0' ? "all" : method,
            supp_get_param(supp, SP_MD5_USERNAME),
            supp_get_param(supp, SP_MD5_PASSWORD),
            supp_get_param(supp, SP_TLS_CERT_PATH),
            supp_get_param(supp, SP_TLS_KEY_PATH),
            supp_get_param(supp, SP_TLS_KEY_PASSWD),
            supp_get_param(supp, SP_TLS_ROOT_CERT_PATH)
            );
}

/** Callbacks for xsupplicant */
const supplicant_impl xsupplicant = {
    xsupplicant_get,
    xsupplicant_start,
    xsupplicant_stop,
    NULL,
    xsupplicant_write_config
};

static te_bool
wpa_supp_get(const char *ifname)
{
    char buf[128];

#if 0
    snprintf(buf, sizeof(buf),
             PS_ALL_COMM " | grep wpa_supplicant | grep -v grep | "
             "grep -q '\-i %s'",
             ifname);
#else
    snprintf(buf, sizeof(buf),
             "/sbin/wpa_cli -i %s status 1>/dev/null 2>&1", ifname);
#endif
    INFO("WPA supplicant status on interface %s", ifname);

    if (ta_system(buf) == 0)
    {
        RING("WPA supplicant on interface %s is running.", ifname);
        return TRUE;
    }
    else
        INFO("WPA supplicant on interface %s is not running", ifname);

    return FALSE;
}

static te_errno
wpa_supp_start(const char *ifname, const char *conf_fname)
{
    char  buf[128];

    RING("%s('%s', '%s')", __FUNCTION__, ifname, conf_fname);
    if (wpa_supp_get(ifname))
    {
        WARN("%s: wpa_supplicant on %s is already running, doing nothing",
             __FUNCTION__, ifname);
        return 0;
    }

    WARN("Starting wpa_supplicant on %s", ifname);

#if 0
    snprintf(buf, sizeof(buf),
             "wpa_supplicant -i %s -c %s -D "
             "wext -B >/dev/null 2>&1", ifname, conf_fname);
#else
    snprintf(buf, sizeof(buf),
             "/sbin/wpa_supplicant -i %s -c %s -D "
             "wext -B", ifname, conf_fname);
#endif

    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_ESHCMD;
    }

    if (!wpa_supp_get(ifname))
    {
        ERROR("Failed to start wpa_supplicant on %s", ifname);
        return TE_EFAIL;
    }
    return 0;
}

static te_errno
wpa_supp_stop(const char *ifname)
{
    char buf[256];

    if (!wpa_supp_get(ifname))
    {
        WARN("%s: wpa_supplicant on %s is not running", __FUNCTION__, ifname);
        return 0;
    }

    WARN("Stopping wpa_supplicant on %s", ifname);

    snprintf(buf, sizeof(buf), "/sbin/wpa_cli -i %s disconnect", ifname);
    if (ta_system(buf) != 0)
        WARN("Command '%s' failed", buf);

    snprintf(buf, sizeof(buf), "/sbin/wpa_cli -i %s terminate", ifname);
    if (ta_system(buf) != 0)
        WARN("Command '%s' failed", buf);

    snprintf(buf, sizeof(buf), "/sbin/ifconfig %s up", ifname);
    if (ta_system(buf) != 0)
        WARN("Command '%s' failed", buf);

    return 0;
}

static te_errno
wpa_supp_reload(const char *ifname)
{
    char buf[256];

    if (!wpa_supp_get(ifname))
    {
        WARN("%s: wpa_supplicant on %s is not running", __FUNCTION__, ifname);
    }
    else
    {
        RING("Reloading wpa_supplicant configuration on %s", ifname);

        snprintf(buf, sizeof(buf), "/sbin/wpa_cli -i %s disconnect", ifname);
        if (ta_system(buf) != 0)
            WARN("Command '%s' failed", buf);

        snprintf(buf, sizeof(buf), "/sbin/wpa_cli -i %s reconfigure", ifname);
        if (ta_system(buf) != 0)
            WARN("Command '%s' failed", buf);

        snprintf(buf, sizeof(buf), "/sbin/wpa_cli -i %s reassociate", ifname);
        if (ta_system(buf) != 0)
            WARN("Command '%s' failed", buf);
    }

    return 0;
}

/**
 * Create configuration file for wpa_supplicant
 *
 * @param f     File for writing config (already opened)
 * @param supp  Supplicant to process
 */
static void
wpa_supp_write_config(FILE *f, const supplicant *supp)
{
    const char *s_method = supp_get_param(supp, SP_METHOD);
    const char *s_proto = supp_get_param(supp, SP_PROTO);
    const char *s_identity = supp_get_param(supp, SP_IDENTITY);
    const char *s_key_mgmt = supp_get_param(supp, SP_KEY_MGMT);
    const char *s_wep_key0 = supp_get_param(supp, SP_WEP_KEY0);
    const char *s_wep_key1 = supp_get_param(supp, SP_WEP_KEY1);
    const char *s_wep_key2 = supp_get_param(supp, SP_WEP_KEY2);
    const char *s_wep_key3 = supp_get_param(supp, SP_WEP_KEY3);
    const char *s_wep_tx_keyidx = supp_get_param(supp, SP_WEP_TX_KEYIDX);
    const char *s_group = supp_get_param(supp, SP_GROUP);
    const char *s_pairwise = supp_get_param(supp, SP_PAIRWISE);
    const char *s_psk = supp_get_param(supp, SP_PSK);
    const char *s_auth_alg = supp_get_param(supp, SP_AUTH_ALG);

    fprintf(f, "ctrl_interface=/var/run/wpa_supplicant\n"
               "network={\n"
               "  ssid=\"%s\"\n",
            supp_get_param(supp, SP_NETWORK));

    if (s_identity[0] != '\0')
        fprintf(f, "  identity=\"%s\"\n", s_identity);

    if (s_key_mgmt[0] != '\0')
        fprintf(f, "  key_mgmt=%s\n", s_key_mgmt);

    /* Writing WEP keys' settings into configuration file */
    if (s_wep_key0[0] != '\0')
        fprintf(f, "  wep_key0=%s\n", s_wep_key0);

    if (s_wep_key1[0] != '\0')
        fprintf(f, "  wep_key1=%s\n", s_wep_key1);

    if (s_wep_key2[0] != '\0')
        fprintf(f, "  wep_key2=%s\n", s_wep_key2);

    if (s_wep_key3[0] != '\0')
        fprintf(f, "  wep_key3=%s\n", s_wep_key3);

    if (s_wep_tx_keyidx[0] != '\0')
        fprintf(f, "  wep_tx_keyidx=%s\n", s_wep_tx_keyidx);

    /* Authentication algorithm OPEN|SHARED|LEAP */
    if (s_auth_alg[0] != '\0')
        fprintf(f, "  auth_alg=%s\n", s_auth_alg);
    /**/

    if (s_proto[0] != '\0')
        fprintf(f, "  proto=%s\n", s_proto);
    else if (strcmp(s_key_mgmt, "IEEE8021X") == 0)
        fprintf(f, "  eapol_flags=0\n");

    if (s_pairwise[0] != '\0')
        fprintf(f, "  pairwise=%s\n", s_pairwise);

    if (s_group[0] != '\0')
        fprintf(f, "  group=%s\n", s_group);

    if (s_psk[0] != '\0')
        fprintf(f, "  psk=\"%s\"\n", s_psk);

    if (s_method[0] != '\0')
    {
        if (strcmp(s_method, "eap-md5") == 0)
        {
            fprintf(f, "  eap=MD5\n"
                       "  password=\"%s\"\n",
                    supp_get_param(supp, SP_MD5_PASSWORD));
        }
        else if (strcmp(s_method, "eap-tls") == 0)
        {
            fprintf(f, "  eap=TLS\n"
                       "  ca_cert=\"%s\"\n"
                       "  client_cert=\"%s\"\n"
                       "  private_key=\"%s\"\n"
                       "  private_key_passwd=\"%s\"\n",
                       supp_get_param(supp, SP_TLS_ROOT_CERT_PATH),
                       supp_get_param(supp, SP_TLS_CERT_PATH),
                       supp_get_param(supp, SP_TLS_KEY_PATH),
                       supp_get_param(supp, SP_TLS_KEY_PASSWD));
        }
        else
        {
            ERROR("%s(): unknown EAP method '%s'",
                  __FUNCTION__, s_method);
        }
    }

    fprintf(f, "}\n");
}

/** Callbacks for wpa_supplicant */
const supplicant_impl wpa_supplicant = {
    wpa_supp_get,
    wpa_supp_start,
    wpa_supp_stop,
    wpa_supp_reload,
    wpa_supp_write_config
};

/**
 * Get supplicant parameter value
 *
 * @param supp     Supplicant to query
 * @param id       Identifier of parameter to query
 *
 * @return Value of specified parameter of the supplicant, or empty
 *         string if parameter is not set or @p id is invalid.
 */
static const char *
supp_get_param(const supplicant *supp, supp_param_t id)
{
    if (id >= SP_MAX)
    {
        ERROR("%s(): invalid supplicant parameter number %u",
              __FUNCTION__, id);
        return "";
    }
    return (supp->params[id] == NULL) ? "" : supp->params[id];
}

/**
 * Set supplicant parameter value
 *
 * @param supp      Supplicant to change
 * @param id        Identifier of parameter to set
 * @param value     New value of the supplicant parameter (may be NULL)
 *
 * @return Status code.
 */
static te_errno
supp_set_param(supplicant *supp, supp_param_t id, const char *value)
{
    if (id >= SP_MAX)
    {
        ERROR("%s(): invalid supplicant parameter number %u",
              __FUNCTION__, id);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    if (value == NULL)
    {
        if (supp->params[id] != NULL)
        {
            free(supp->params[id]);
            supp->changed = TRUE;
            return 0;
        }
        return 0;
    }
    if (supp->params[id] != NULL)
    {
        if (strcmp(supp->params[id], value) == 0)
            return 0;

        free(supp->params[id]);
    }
    if ((supp->params[id] = strdup(value)) == NULL)
    {
        ERROR("%s(): failed to allocate memory for value '%s'",
              __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    supp->changed = TRUE;
    return 0;
}

/**
 * Create new supplicant structure for the interface
 *
 * @param ifname   Name of interface to create supplicant at
 *
 * @return Pointer to new supplicant structure, or NULL if creation failed.
 */
static supplicant *
supp_create(const char *ifname)
{
    supplicant   *ns;
    unsigned int  i;

    if ((ns = calloc(1, sizeof(supplicant))) == NULL)
        return NULL;

    if ((ns->ifname = strdup(ifname)) == NULL)
    {
        free(ns);
        return NULL;
    }
    ns->started = FALSE;
    ns->changed = TRUE;
#ifdef USE_XSUPPLICANT
    ns->impl = &xsupplicant;
#else
    ns->impl = &wpa_supplicant;
#endif

    snprintf(ns->confname, sizeof(ns->confname),
             "/tmp/te_supp_%s.conf", ifname);

    for (i = 0; i < SP_MAX; i++)
        ns->params[i] = NULL;

    supp_set_param(ns, SP_NETWORK, "tester");

    ns->next = supplicant_list;
    supplicant_list = ns;

    return ns;
}

/**
 * Free the memory allocated for the supplicant structure 
 * and its parameters
 *
 * @param supp      Supplicant to destroy
 */
static void
supp_destroy(supplicant *supp)
{
    unsigned int i;

    for (i = 0; i < SP_MAX; i++)
        free(supp->params[i]);

    remove(supp->confname);
    free(supp->ifname);
    free(supp);
}

/**
 * Find the supplicant for specified interface in the list of available
 * supplicants
 *
 * @param ifname  Name of interface to find
 *
 * @return Pointer to supplicant structure or NULL if there is no supplicant
 *         configured at the specified interface.
 */
static supplicant *
supp_find(const char *ifname)
{
    supplicant *found;

    for (found = supplicant_list; found != NULL; found = found->next)
    {
        if (strcmp(found->ifname, ifname) == 0)
            return found;
    }
    return NULL;
}

/**
 * Check the changed made in supplicant configuration,
 * create new configuration file and restart supplicant if needed.
 *
 * @param  supp  Supplicant to process
 *
 * @return Status code.
 */
static te_errno
supp_update(supplicant *supp)
{
    FILE     *conf;
    const supplicant_impl *new_impl;
    const char *proto;
    const char *method;

    if (supp == NULL || !supp->changed)
        return 0;

    if ((conf = fopen(supp->confname, "w")) == NULL)
        return TE_RC(TE_TA_UNIX, errno);

    /* Check protocol value and detect the type of supplicant */
    proto = supp_get_param(supp, SP_PROTO);
    method = supp_get_param(supp, SP_METHOD);
    if (proto[0] == '\0' || method[0] == '\0')
#ifdef USE_XSUPPLICANT
        new_impl = &xsupplicant;
#else
        new_impl = &wpa_supplicant;
#endif
    else if (strcmp(proto, "WPA") == 0 || strcmp(proto, "RSN") == 0 ||
             strcmp(proto, "WPA2") == 0)
        new_impl = &wpa_supplicant;
    else
    {
        ERROR("%s(): unknown proto '%s'", __FUNCTION__, proto);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    new_impl->write_config(conf, supp);
    fclose(conf);

    supp->changed = FALSE;
    if (supp->started)
    {
        if (supp->impl == new_impl && supp->impl->reload != NULL)
            supp->impl->reload(supp->ifname);
        else
        {
            supp->impl->stop(supp->ifname);
            new_impl->start(supp->ifname, supp->confname);
        }
    }

    supp->impl = new_impl;
    return 0;
}

static te_errno
ds_supplicant_commit(unsigned int gid, const cfg_oid *p_oid)
{
    supplicant *supp;
    const char *instance;

    UNUSED(gid);

    /* oid has 4 elements: [root, agent, interface, supplicant] */
    instance = CFG_OID_GET_INST_NAME(p_oid, 2);
    RING("%s('%s')", __FUNCTION__, instance);
    if ((supp = supp_find(instance)) == NULL)
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    if (supp->changed)
        return supp_update(supp);
    else if (supp->started)
        return supp->impl->start(supp->ifname, supp->confname);
    else
        return supp->impl->stop(supp->ifname);
}

static te_errno
ds_supplicant_get(unsigned int gid, const char *oid,
                  char *value, const char *instance, ...)
{
    supplicant *supp;

    UNUSED(gid);
    UNUSED(oid);

    if ((supp = supp_find(instance)) == NULL)
    {
        if ((supp = supp_create(instance)) == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if (supp->impl->get(supp->ifname))
        strcpy(value, "1");
    else
        strcpy(value, "0");

    return 0;
}

static te_errno
ds_supplicant_set(unsigned int gid, const char *oid,
                  const char *value, const char *instance, ...)
{
    supplicant *supp = supp_find(instance);

    UNUSED(gid);
    UNUSED(oid);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    supp->started = (*value == '0') ? FALSE : TRUE;

    return 0;
}

/**
 * Set value of ESSID for supplicant
 *
 * Note: function is not static to be called from conf_wifi.c
 * when changing ESSID
 */
te_errno
ds_supplicant_network_set(unsigned int gid, const char *oid,
                          const char *value, const char *instance, ...)
{
    supplicant *supp = supp_find(instance);
    te_errno    rc;

    UNUSED(gid);
    UNUSED(oid);

    RING("%s('%s','%s')", __FUNCTION__, instance, value);
    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((rc = supp_set_param(supp, SP_NETWORK, value)) != 0)
        return rc;

    return supp_update(supp);
}

#define DS_SUPP_PARAM_GET(_func_name, _param_name) \
    static te_errno                                                     \
    _func_name(unsigned int gid, const char *oid,                       \
               char *value, const char *instance, ...)                  \
    {                                                                   \
        supplicant *supp = supp_find(instance);                         \
                                                                        \
        UNUSED(gid);                                                    \
        UNUSED(oid);                                                    \
                                                                        \
        if (supp == NULL)                                               \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                        \
                                                                        \
        strcpy(value, supp_get_param(supp, (_param_name)));             \
        return 0;                                                       \
    }


#define DS_SUPP_PARAM_SET(_func_name, _param_name) \
    static te_errno                                                     \
    _func_name(unsigned int gid, const char *oid,                       \
               const char *value, const char *instance, ...)            \
    {                                                                   \
        supplicant *supp = supp_find(instance);                         \
                                                                        \
        UNUSED(gid);                                                    \
        UNUSED(oid);                                                    \
                                                                        \
        if (supp == NULL)                                               \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                        \
                                                                        \
        return supp_set_param(supp, (_param_name), value);              \
    }

/**
 * EAP-MD5 support
 * Parameters:
 *      SP_MD5_USERNAME   User name
 *      SP_MD5_PASSWORD   User password
 */
DS_SUPP_PARAM_GET(ds_supp_eapmd5_username_get, SP_MD5_USERNAME)
DS_SUPP_PARAM_SET(ds_supp_eapmd5_username_set, SP_MD5_USERNAME)
DS_SUPP_PARAM_GET(ds_supp_eapmd5_passwd_get, SP_MD5_PASSWORD)
DS_SUPP_PARAM_SET(ds_supp_eapmd5_passwd_set, SP_MD5_PASSWORD)

RCF_PCH_CFG_NODE_RW(node_ds_supp_eapmd5_passwd, "passwd",
                    NULL, NULL,
                    ds_supp_eapmd5_passwd_get, 
                    ds_supp_eapmd5_passwd_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_eapmd5_username, "username",
                    NULL, &node_ds_supp_eapmd5_passwd,
                    ds_supp_eapmd5_username_get, 
                    ds_supp_eapmd5_username_set);

RCF_PCH_CFG_NODE_RO(node_ds_supp_eapmd5, "eap-md5",
                    &node_ds_supp_eapmd5_username, NULL, NULL);

/**
 * EAP-TLS support
 * Parameters:
 *      SP_TLS_CERT_PATH       Path to user certificate file
 *      SP_TLS_KEY_PATH        Path to user private key file
 *      SP_TLS_KEY_PASSWD      Password for user private key
 *      SP_TLS_ROOT_CERT_PATH  Path to root certificate file
 */
DS_SUPP_PARAM_GET(ds_supp_eaptls_cert_get, SP_TLS_CERT_PATH)
DS_SUPP_PARAM_SET(ds_supp_eaptls_cert_set, SP_TLS_CERT_PATH)
DS_SUPP_PARAM_GET(ds_supp_eaptls_key_get, SP_TLS_KEY_PATH)
DS_SUPP_PARAM_SET(ds_supp_eaptls_key_set, SP_TLS_KEY_PATH)
DS_SUPP_PARAM_GET(ds_supp_eaptls_key_passwd_get, SP_TLS_KEY_PASSWD)
DS_SUPP_PARAM_SET(ds_supp_eaptls_key_passwd_set, SP_TLS_KEY_PASSWD)
DS_SUPP_PARAM_GET(ds_supp_eaptls_root_cert_get, SP_TLS_ROOT_CERT_PATH)
DS_SUPP_PARAM_SET(ds_supp_eaptls_root_cert_set, SP_TLS_ROOT_CERT_PATH)

RCF_PCH_CFG_NODE_RW(node_ds_supp_eaptls_cert, "cert",
                    NULL, NULL,
                    ds_supp_eaptls_cert_get,
                    ds_supp_eaptls_cert_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_eaptls_key, "key",
                    NULL, &node_ds_supp_eaptls_cert,
                    ds_supp_eaptls_key_get,
                    ds_supp_eaptls_key_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_eaptls_key_passwd, "key_passwd",
                    NULL, &node_ds_supp_eaptls_key,
                    ds_supp_eaptls_key_passwd_get,
                    ds_supp_eaptls_key_passwd_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_eaptls_root_cert, "root_cert",
                    NULL, &node_ds_supp_eaptls_key_passwd,
                    ds_supp_eaptls_root_cert_get,
                    ds_supp_eaptls_root_cert_set);

RCF_PCH_CFG_NODE_RO(node_ds_supp_eaptls, "eap-tls",
                    &node_ds_supp_eaptls_root_cert,
                    &node_ds_supp_eapmd5, NULL);

/**
 * Common EAP parameters
 */
DS_SUPP_PARAM_GET(ds_supp_identity_get, SP_IDENTITY)
DS_SUPP_PARAM_SET(ds_supp_identity_set, SP_IDENTITY)
DS_SUPP_PARAM_GET(ds_supp_method_get, SP_METHOD)
DS_SUPP_PARAM_SET(ds_supp_method_set, SP_METHOD)
DS_SUPP_PARAM_GET(ds_supp_proto_get, SP_PROTO)
DS_SUPP_PARAM_SET(ds_supp_proto_set, SP_PROTO)

/* NEW_BROTHERS_IN_OUR_FAMILY */
DS_SUPP_PARAM_GET(ds_supp_key_mgmt_get, SP_KEY_MGMT)
DS_SUPP_PARAM_SET(ds_supp_key_mgmt_set, SP_KEY_MGMT)
/* NODES_TO_MANAGE_WEP_KEYS */
DS_SUPP_PARAM_GET(ds_supp_wep_key0_get, SP_WEP_KEY0)
DS_SUPP_PARAM_SET(ds_supp_wep_key0_set, SP_WEP_KEY0)
DS_SUPP_PARAM_GET(ds_supp_wep_key1_get, SP_WEP_KEY1)
DS_SUPP_PARAM_SET(ds_supp_wep_key1_set, SP_WEP_KEY1)
DS_SUPP_PARAM_GET(ds_supp_wep_key2_get, SP_WEP_KEY2)
DS_SUPP_PARAM_SET(ds_supp_wep_key2_set, SP_WEP_KEY2)
DS_SUPP_PARAM_GET(ds_supp_wep_key3_get, SP_WEP_KEY3)
DS_SUPP_PARAM_SET(ds_supp_wep_key3_set, SP_WEP_KEY3)
DS_SUPP_PARAM_GET(ds_supp_wep_tx_keyidx_get, SP_WEP_TX_KEYIDX)
DS_SUPP_PARAM_SET(ds_supp_wep_tx_keyidx_set, SP_WEP_TX_KEYIDX)
/* !NODES_TO_MANAGE_WEP_KEYS */
DS_SUPP_PARAM_GET(ds_supp_group_get, SP_GROUP)
DS_SUPP_PARAM_SET(ds_supp_group_set, SP_GROUP)
DS_SUPP_PARAM_GET(ds_supp_pairwise_get, SP_PAIRWISE)
DS_SUPP_PARAM_SET(ds_supp_pairwise_set, SP_PAIRWISE)
DS_SUPP_PARAM_GET(ds_supp_psk_get, SP_PSK)
DS_SUPP_PARAM_SET(ds_supp_psk_set, SP_PSK)
DS_SUPP_PARAM_GET(ds_supp_auth_alg_get, SP_AUTH_ALG)
DS_SUPP_PARAM_SET(ds_supp_auth_alg_set, SP_AUTH_ALG)

RCF_PCH_CFG_NODE_RW(node_ds_supp_auth_alg, "auth_alg",
                    NULL, &node_ds_supp_eaptls,
                    ds_supp_auth_alg_get,
                    ds_supp_auth_alg_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_psk, "psk",
                    NULL, &node_ds_supp_auth_alg,
                    ds_supp_psk_get,
                    ds_supp_psk_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_pairwise, "pairwise",
                    NULL, &node_ds_supp_psk,
                    ds_supp_pairwise_get,
                    ds_supp_pairwise_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_group, "group",
                    NULL, &node_ds_supp_pairwise,
                    ds_supp_group_get,
                    ds_supp_group_set);

/* NODES_TO_MANAGE_WEP_KEYS */
RCF_PCH_CFG_NODE_RW(node_ds_supp_wep_tx_keyidx, "wep_tx_keyidx",
                    NULL, &node_ds_supp_group,
                    ds_supp_wep_tx_keyidx_get,
                    ds_supp_wep_tx_keyidx_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_wep_key3, "wep_key3",
                    NULL, &node_ds_supp_wep_tx_keyidx,
                    ds_supp_wep_key3_get,
                    ds_supp_wep_key3_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_wep_key2, "wep_key2",
                    NULL, &node_ds_supp_wep_key3,
                    ds_supp_wep_key2_get,
                    ds_supp_wep_key2_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_wep_key1, "wep_key1",
                    NULL, &node_ds_supp_wep_key2,
                    ds_supp_wep_key1_get,
                    ds_supp_wep_key1_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_wep_key0, "wep_key0",
                    NULL, &node_ds_supp_wep_key1,
                    ds_supp_wep_key0_get,
                    ds_supp_wep_key0_set);
/* !NODES_TO_MANAGE_WEP_KEYS */

RCF_PCH_CFG_NODE_RW(node_ds_supp_key_mgmt, "key_mgmt",
                    NULL, &node_ds_supp_wep_key0,
                    ds_supp_key_mgmt_get,
                    ds_supp_key_mgmt_set);
/* !NEW_BROTHERS_IN_OUR_FAMILY */

RCF_PCH_CFG_NODE_RW(node_ds_supp_proto, "proto",
                    NULL, &node_ds_supp_key_mgmt,
                    ds_supp_proto_get,
                    ds_supp_proto_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_method, "cur_method",
                    NULL, &node_ds_supp_proto,
                    ds_supp_method_get,
                    ds_supp_method_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_identity, "identity",
                    NULL, &node_ds_supp_method,
                    ds_supp_identity_get,
                    ds_supp_identity_set);

static rcf_pch_cfg_object node_ds_supplicant = {
                    "supplicant", 0, &node_ds_supp_identity, NULL,
                    ds_supplicant_get, ds_supplicant_set,
                    NULL, NULL, NULL, ds_supplicant_commit, NULL };

te_errno
ta_unix_conf_supplicant_init()
{
    return rcf_pch_add_node("/agent/interface", &node_ds_supplicant);
}

/**
 * Get the name of interface from the name of interface resource
 * e.g. "/agent:Agt_A/interface:eth0" -> "eth0"
 *
 * @param name  Name of interface resource
 *
 * @return Name of interface.
 */
const char *
supplicant_get_name(const char *name)
{
    const char *instance = strrchr(name, ':');

    if (instance == NULL || *instance == '\0')
    {
        ERROR("%s(): invalid interface resource name '%s'",
              __FUNCTION__, name);
        return NULL;
    }
    return instance + 1;
}

te_errno
supplicant_grab(const char *name)
{
    const char *instance = supplicant_get_name(name);

    if (instance == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (supp_find(instance) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (supp_create(instance) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

te_errno
supplicant_release(const char *name)
{
    const char *instance = supplicant_get_name(name);
    supplicant *supp;
    supplicant *prev = NULL;
    supplicant *iter;

    if (instance == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((supp = supp_find(instance)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    for (iter = supplicant_list; iter != NULL; prev = iter, iter = iter->next)
    {
        if (supp == iter)
            break;
    }
    if (iter != NULL)
    {
        if (prev != NULL)
            prev->next = iter->next;
        else
            supplicant_list = iter->next;
    }

    if (supp->started)
        supp->impl->stop(supp->ifname);

    supp_destroy(supp);
    return 0;
}
#endif
