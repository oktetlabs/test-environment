/** @file
 * @brief ARL table Configuration Model TAPI
 *
 * Implementation of test API for ARL table configuration model
 * (storage/cm/cm_poesw.xml).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER      "Configuration TAPI"

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "te_queue.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_cfg.h"

#include "tapi_cfg_arl.h"


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
    int rc;
    int val;

    rc = tapi_cfg_get_son_mac(oid, "mac", "", p->mac);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get ARL entry %s MAC", rc, oid);
        return rc;
    }

    rc = cfg_get_instance_int_fmt(&val, "%s/port:", oid);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get ARL entry %s MAC", rc, oid);
        return rc;
    }
    p->port = (unsigned int)val;

    rc = cfg_get_instance_int_fmt(&val, "%s/type:", oid);
    if (rc != 0)
    {
        ERROR("Failed(%x) to get ARL entry %s MAC", rc, oid);
        return rc;
    }
    /* Direct mapping */
    p->type = (arl_entry_type)val;

    rc = cfg_get_instance_string_fmt(&p->vlan, "%s/vlan:", oid);
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

    while ((p = TAILQ_FIRST(p_table)) != NULL)
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
    TAILQ_FOREACH(p, p_table, links)
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

    TAILQ_FOREACH(p, p_table, links)
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

