/** @file
 * @brief ARL table Configuration Model TAPI
 *
 * Implementation of test API for ARL table configuration model
 * (storage/cm/cm_poesw.xml).
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kratsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "conf_api.h"
#include "tapi_cfg.h"

#include "tapi_cfg_arl.h"

#define TE_LGR_USER      "Configuration TAPI"
#include "logger_api.h"


/* See description in tapi_cfg_arl.h */
int
tapi_cfg_arl_get_table(const char *ta, te_bool sync, arl_table_t *p_table)
{
    int             rc;
    cfg_handle     *handles = NULL;
    unsigned int    num = 0;
    unsigned int    i;
    arl_entry_t    *p;


    if ((ta == NULL) || (p_table == NULL))
        return TE_EINVAL;

    if (sync)
    {
        rc = cfg_synchronize_fmt(TRUE, "/agent:%s/arl:", ta);
        if (rc != 0)
        {
            ERROR("Failed(%x) to synchronize ARL table", rc);
            return rc;
        }
    }

    rc = cfg_find_pattern_fmt(&num, &handles, "/agent:%s/arl:/entry:*", ta);
    if (rc != 0)
    {
        ERROR("Failed(%x) to find ARL table entries", rc);
        return rc;
    }
    VERB("ARL table contains %u entries", num);

    TAILQ_INIT(p_table);
    for (i = 0; i < num; ++i)
    {
        char *oid;

        rc = cfg_get_oid_str(handles[i], &oid);
        if (rc != 0)
        {
            ERROR("Failed(%x) to get ARL entry OID by handle", rc);
            tapi_arl_free_table(p_table);
            return rc;
        }
        p = (arl_entry_t *)calloc(1, sizeof(*p));
        if (p == NULL)
        {
            ERROR("Memory allocation failure");
            free(oid);
            tapi_arl_free_table(p_table);
            return TE_ENOMEM;
        }
        rc = tapi_cfg_arl_get_entry(oid, p);
        if (rc != 0)
        {
            ERROR("Failed(%x) to get ARL entry", rc);
            free(p);
            free(oid);
            tapi_arl_free_table(p_table);
            return rc;
        }
        free(oid);
        TAILQ_INSERT_TAIL(p_table, p, links);
    }
    
    return 0;
}

/* See description in tapi_cfg_arl.h */
int
tapi_cfg_arl_del_entry(const char *ta,
                       arl_entry_type type, unsigned int port_num,
                       const uint8_t *mac_addr, const char *vlan_name)
{
    int rc;

    if (vlan_name == NULL)
        vlan_name = DEFAULT_VLAN_NAME;

    rc = cfg_del_instance_fmt(FALSE, "/agent:%s/arl:/entry:"
                              "%d.%u.%02x:%02x:%02x:%02x:%02x:%02x.%s",
                               ta, type, port_num,
                               mac_addr[0], mac_addr[1], mac_addr[2],
                               mac_addr[3], mac_addr[4], mac_addr[5],
                               vlan_name);
    if (rc != 0)
    {
        ERROR("Error while deleting ARL entry: %x", rc);
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/arl:", ta)) != 0)
    {
        ERROR("Error while synchronizing ARL table on %s Agent", ta);
    }

    return rc;
}

/* See description in tapi_cfg_arl.h */
int
tapi_cfg_arl_add_entry(const char *ta,
                       arl_entry_type type, unsigned int port_num,
                       const uint8_t *mac_addr, const char *vlan_name)
{
    cfg_handle  handle;
    int         rc;

    if (vlan_name == NULL)
        vlan_name = DEFAULT_VLAN_NAME;

    /* 
     * We should synchronize ARL entry subtree to make it 
     * available for Configurator
     */
    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/arl:", ta)) != 0)
    {
        ERROR("Error while synchronizing ARL table on %s Agent", ta);
        return rc;
    }

    if ((rc = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                   "/agent:%s/arl:/entry:"
                                   "%d.%u.%02x:%02x:%02x:%02x:%02x:%02x.%s",
                                   ta, type, port_num,
                                   mac_addr[0], mac_addr[1], mac_addr[2],
                                   mac_addr[3], mac_addr[4], mac_addr[5],
                                   vlan_name)) != 0)
    {
        ERROR("Error %x while adding a new ARL entry", rc);
        return rc;
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s", ta)) != 0)
    {
        ERROR("Error while synchronizing ARL table on %s Agent", ta);
        return rc;
    }

    return rc;
}


/* See description in tapi_cfg_arl.h */
int
tapi_cfg_arl_get_entry(const char *oid, arl_entry_t *p)
{
    int             rc;
    cfg_val_type    val_type;
    int             val;

    rc = tapi_cfg_get_son_mac(oid, "mac", "", p->mac);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get ARL entry %s MAC", rc, oid);
        return rc;
    }

    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type,  &val, "%s/port:", oid);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get ARL entry %s MAC", rc, oid);
        return rc;
    }
    p->port = (unsigned int)val;

    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type,  &val, "%s/type:", oid);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get ARL entry %s MAC", rc, oid);
        return rc;
    }
    /* Direct mapping */
    p->type = (arl_entry_type)val;

    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(&val_type,  &p->vlan, "%s/vlan:", oid);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get ARL entry %s VLAN", rc, oid);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_arl.h */
void
tapi_arl_free_entry(arl_entry_t *p)
{
    free(p->vlan);
}

/* See description in tapi_cfg_arl.h */
void
tapi_arl_free_table(arl_table_t *p_table)
{
    arl_entry_t *p;

    while ((p = p_table->tqh_first) != NULL)
    {
        TAILQ_REMOVE(p_table, p, links);
        tapi_arl_free_entry(p);
        free(p);
    }
}


/* See description in tapi_cfg_arl.h */
arl_entry_t *
tapi_arl_find(const arl_table_t *p_table, const uint8_t *mac,
              const char *vlan, unsigned int port,
              enum arl_entry_type type)
{
    arl_entry_t *p;

    VERB("Find %x:%x:%x:%x:%x:%x VLAN=%s port=%u type=%d", mac[0],
        mac[1], mac[2], mac[3], mac[4], mac[5], vlan, port, type);
    for (p = p_table->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        VERB("CMP with %x:%x:%x:%x:%x:%x VLAN=%s port=%u type=%d",
            p->mac[0], p->mac[1], p->mac[2], p->mac[3], p->mac[4],
            p->mac[5], p->vlan, p->port, p->type);
        if ((p->port == port) &&
            (p->type == type) &&
            (memcmp(p->mac, mac, ETHER_ADDR_LEN) == 0) &&
            (strcmp(p->vlan, vlan) == 0))
        {
            VERB("Match!");
            break;
        }
    }

    return p;
}

/* See description in tapi_cfg_arl.h */
void
tapi_arl_print_table(const arl_table_t *p_table)
{
    arl_entry_t *p;

    VERB("ARL Table:");

    for (p = p_table->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        VERB("\tMac: %x:%x:%x:%x:%x:%x\n"
            "\tPort: %d"
            "\tVLAN: %s"
            "\tType: %s",
            p->mac[0], p->mac[1], p->mac[2],
            p->mac[3], p->mac[4], p->mac[5],
            p->port, p->vlan,
            (p->type == ARL_ENTRY_STATIC) ? "static" : "dynamic");
    }
}

