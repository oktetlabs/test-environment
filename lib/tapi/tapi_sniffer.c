/** @file
 * @brief Test API to configure sniffers.
 *
 * Implementation of API to configure sniffers.
 *
 * Copyright (C) 2004-2011 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id: 
 */

#define TE_LGR_USER     "TAPI Sniffer"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "conf_api.h"
#include "logger_api.h"
#include "tapi_cfg_base.h"
#include "tapi_sniffer.h"

#include "te_stdint.h"
#include "te_raw_log.h"
#include "logger_int.h"
#include "ipc_client.h"
#include "logger_ten.h"

#define TE_CFG_SNIF_FMT   "/agent:%s/interface:%s/sniffer:%s"

/**
 * Enable the sniffer and sync to get ssn from the Agent.
 * 
 * @param ta        Agent name
 * @param snif_id   The sniffer ID
 * 
 * @return Status code
 * @retval 0    succes
 */
static te_errno
sniffer_enable_sync(tapi_sniffer_id *snif_id)
{
    cfg_val_type    type;
    te_errno        rc;
    char            sn_oid[CFG_OID_MAX];

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                              TE_CFG_SNIF_FMT "/enable:", 
                              snif_id->ta, snif_id->ifname,
                              snif_id->snifname);
    if (rc != 0)
    {
        ERROR("Failed to enable the sniffer");
        return rc;
    }

    snprintf(sn_oid, sizeof(sn_oid), TE_CFG_SNIF_FMT, snif_id->ta,
             snif_id->ifname, snif_id->snifname);
    if ((rc = cfg_synchronize(sn_oid, TRUE)) != 0)
    {
        ERROR("Failted to sync");
        return rc;
    }

    type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&type, &snif_id->ssn, TE_CFG_SNIF_FMT,
                              snif_id->ta, snif_id->ifname,
                              snif_id->snifname);
    if (rc != 0)
    {
        ERROR("Failed to get the sniffer ssn");
        return rc;
    }
    return 0;
}

/**
 * Check sniffer name exists
 * 
 * @param ta        Agent name
 * @param iface     Interface name
 * @param name      Sniffer name
 * @c     NULL      Generate a new name
 * @param newname   Pointer to new name if incoming exists or NULL (OUT)
 * @c     NULL      If incoming name is correct
 * 
 * @retval 0        Success
 */
static te_errno
sniffer_check_name(const char *ta, const char *iface, const char *name,
                   char **newname)
{
    cfg_handle           *handles;
    unsigned int          n_handles;
    int                   rc;
    char                 *rec_name;

    *newname = NULL;

    if (name == NULL)
    {
        *newname = malloc(CFG_SUBID_MAX);
        if (*newname == NULL)
            return TE_RC(TE_RCF_API, TE_ENOMEM);
        rc = snprintf(*newname, CFG_SUBID_MAX, "default_%u", rand()%1000);
        if (rc > CFG_SUBID_MAX)
            return TE_RC(TE_RCF_API, TE_EINVAL);
        rc = 0;
    }
    else
    {
        rc = cfg_find_pattern_fmt(&n_handles, &handles,
                                  "/agent:%s/interface:%s/sniffer:%s",
                                  ta, iface, name);
        if (rc == 0 && n_handles > 0)
        {
            *newname = malloc(CFG_SUBID_MAX);
            if (*newname == NULL)
                return TE_RC(TE_RCF_API, TE_ENOMEM);
            rc = snprintf(*newname, CFG_SUBID_MAX, "%s_%u", name,
                          rand()%1000);
            if (rc > CFG_SUBID_MAX)
                return TE_RC(TE_RCF_API, TE_EINVAL);
            rc = 0;
        }
    }

    /* Recursion to check new name */
    if (rc == 0 && *newname != NULL)
    {
        rc = sniffer_check_name(ta, iface, *newname, &rec_name);
        if (rc == 0 && rec_name != NULL)
        {
            free(*newname);
            *newname = rec_name;
        }
        else if (rc != 0)
        {
            free(*newname);
            *newname = NULL;
        }
    }

    return rc;
}

/* See description in tapi_sniffer.h */
tapi_sniffer_id *
tapi_sniffer_add(const char *ta, const char *iface, const char *name,
                 const char *filter, te_bool ofill)
{
    tapi_sniffer_id      *newsnid = NULL;
    int                   rc;
    char                 *sniffname;

    sniffer_check_name(ta, iface, name, &sniffname);
    if (sniffname == NULL)
        sniffname = strdup(name);

    if ((iface == NULL) || (ta == NULL))
    {
        ERROR("Wrong incoming arguments interface name or ta is null");
        return NULL;
    }

    if ((newsnid = malloc(sizeof(tapi_sniffer_id))) == NULL)
    {
        ERROR("Malloc error");
        return NULL;
    }
    memset(newsnid, 0, sizeof(newsnid));

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL, TE_CFG_SNIF_FMT,
                              ta, iface, sniffname);
    if (rc != 0)
    {
        ERROR("Failed to add sniffer");
        free(newsnid);
        return NULL;
    }

    strncpy(newsnid->ta, ta, CFG_SUBID_MAX);
    strncpy(newsnid->snifname, sniffname, CFG_SUBID_MAX);
    strncpy(newsnid->ifname, iface, CFG_SUBID_MAX);
    if (strlen(newsnid->snifname) == 0 || strlen(newsnid->ifname) == 0 ||
        strlen(newsnid->ta) == 0)
    {
        ERROR("Wrong agent, interface or sniffer name.");
        goto clean_sniffer_add;
    }

    if (filter != NULL)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, filter),
                                  TE_CFG_SNIF_FMT "/filter_exp_str:",
                                  ta, iface, sniffname);
        if (rc != 0)
        {
            ERROR("Failed to change the filter expression to %s", filter);
            goto clean_sniffer_add;
        }
    }

    if (ofill == TRUE)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1), TE_CFG_SNIF_FMT
                                  "/tmp_logs:/overfill_meth:",
                                  ta, iface, sniffname);
        if (rc != 0)
        {
            ERROR("Failed to change overfill handle method to tail drop");
            goto clean_sniffer_add;
        }
    }

    if((rc = sniffer_enable_sync(newsnid)) != 0)
        goto clean_sniffer_add;

    return newsnid;

clean_sniffer_add:
    if (sniffname == NULL)
        free(sniffname);
    if (newsnid != NULL)
        free(newsnid);
    return NULL;
}

/* See description in tapi_sniffer.h */
te_errno
tapi_sniffer_add_mult(const char *ta, const char *iface, const char *name,
                      const char *filter, te_bool ofill, sniffl_h_t *snif_h)
{
    tapi_sniffer_id      *newsnid = NULL;
    tapi_sniff_list_s    *newsn_l = NULL;
    cfg_handle           *handles;
    unsigned int          n_handles;
    int                   rc;
    int                   i;
    char                 *newiface;

    if (iface == NULL)
    {
        rc = cfg_find_pattern_fmt(&n_handles, &handles,
                                  "/agent:%s/interface:*", ta);
        if (rc != 0)
            return rc;
        for (i = 0; i < n_handles; i++)
        {
            rc = cfg_get_inst_name(handles[i], &newiface);
            if (rc == 0 && strcmp(newiface, "lo") != 0)
            {
                newsnid = tapi_sniffer_add(ta, newiface, name, filter,
                                           ofill);
                if (newsnid != NULL)
                {
                    newsn_l = malloc(sizeof(tapi_sniff_list_s));
                    newsn_l->sniff = newsnid;
                    SLIST_INSERT_HEAD(snif_h, newsn_l, tapi_sniff_ent);
                }
            }
        }
    }
    else
    {
        newsnid = tapi_sniffer_add(ta, iface, name, filter, ofill);
        if (newsnid != NULL)
        {
            newsn_l = malloc(sizeof(tapi_sniff_list_s));
            newsn_l->sniff = newsnid;
            SLIST_INSERT_HEAD(snif_h, newsn_l, tapi_sniff_ent);
        }
    }

    return 0;
}

/* See description in tapi_sniffer.h */
te_errno
tapi_sniffer_del(tapi_sniffer_id *id)
{
    int rc;

    if (id == NULL)
    {
        ERROR("Sniffer ID is null");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_del_instance_fmt(FALSE, TE_CFG_SNIF_FMT, id->ta, 
                              id->ifname, id->snifname);
    if (rc != 0)
    {
        ERROR("Failed to delete sniffer");
        return TE_RC(TE_TAPI, rc);
    }

    return 0;
}

/* See description in tapi_sniffer.h */
te_errno
tapi_sniffer_stop(tapi_sniffer_id *id)
{
    if (id == NULL)
    {
        ERROR("Sniffer ID is null");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                TE_CFG_SNIF_FMT "/enable:", 
                                id->ta, id->ifname, id->snifname);
}

/* See description in tapi_sniffer.h */
te_errno
tapi_sniffer_start(tapi_sniffer_id *id)
{
    int rc;

    if (id == NULL)
    {
        ERROR("Sniffer ID is null");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = sniffer_enable_sync(id);

    return rc;
}

/* See description in tapi_sniffer.h */
te_errno
tapi_sniffer_mark(const char *ta, tapi_sniffer_id *id,
                  const char *description)
{
    te_log_nfl          nfl;
    te_log_nfl          nfl_net;
    size_t              buf_len;
    char               *mess;
    te_errno            rc         = 0;
    struct ipc_client  *log_client = NULL;

    if (description == NULL)
        description = "";

    /* Prepare message */
    buf_len = SNIFFER_MIN_MARK_SIZE;
    mess = malloc(buf_len);
    if (mess == NULL)
    {
        ERROR("Malloc error");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (id != NULL)
        nfl = snprintf(mess + sizeof(te_log_nfl),
                       buf_len - sizeof(te_log_nfl),
                       "%s0%s %s %s %u;%s", LGR_SRV_SNIFFER_MARK, id->ta,
                       id->snifname, id->ifname, id->ssn, description);
    else if (ta != NULL)
        nfl = snprintf(mess + sizeof(te_log_nfl),
                       buf_len - sizeof(te_log_nfl),
                       "%s1%s;%s", LGR_SRV_SNIFFER_MARK, ta, description);
    else
        return TE_RC(TE_TAPI, TE_EINVAL);
    if (nfl > buf_len - sizeof(te_log_nfl))
    {
        do {
            /* Double size of the buffer */
            buf_len = buf_len << 1;
        } while (buf_len < nfl);
        /* Reallocate the buffer */
        mess = realloc(mess, buf_len);
        if (mess == NULL)
        {
            ERROR("%s(): realloc(p, %u) failed",
                  __FUNCTION__, buf_len);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
        if (id != NULL)
            nfl = snprintf(mess + sizeof(te_log_nfl),
                           buf_len - sizeof(te_log_nfl),
                           "%s0%s %s %s %u;%s", LGR_SRV_SNIFFER_MARK,
                           id->ta, id->snifname, id->ifname,
                           id->ssn, description);
        else
            nfl = snprintf(mess + sizeof(te_log_nfl),
                           buf_len - sizeof(te_log_nfl),
                           "%s1%s;%s", LGR_SRV_SNIFFER_MARK, ta,
                           description);
    }

    nfl_net = htons(nfl);
    memcpy(mess, &nfl_net, sizeof(nfl_net));

    rc = ipc_init_client("LOGGER_SNIFFER_MARK", LOGGER_IPC, &log_client);
    if (rc != 0)
    {
        ERROR("ipc_init_client() failed: 0x%X", rc);
        goto exit_tapi_sniffer_mark;
    }
    assert(log_client != NULL);
    if ((rc = ipc_send_message(log_client, LGR_SRV_NAME, mess,
                               nfl + sizeof(nfl_net))) != 0)
    {
        ERROR("ipc_send_message() failed");        
        ipc_close_client(log_client);
    }
    else if ((rc = ipc_close_client(log_client)) != 0)
        ERROR("ipc_close_client() failed");

exit_tapi_sniffer_mark:
    free(mess);

    return rc;
}
