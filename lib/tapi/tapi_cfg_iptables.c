/** @file
 * @brief iptables Configuration Model TAPI
 *
 * Definition of test API for iptables configuration model
 * (storage/cm/cm_iptables.xml).
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
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

#include "tapi_cfg_iptables.h"

static char *
iptables_rule_to_id(const char *rule)
{
    char *id = strdup(rule);
    char *p  = id;

    for ( ; *p != '\0' ; p++)
    {
        if (*p == ' ')
            *p = '#';
        else if (*p == ':')
            *p = ';';
        else if (*p == '/')
            *p = '|';
    }

    return id;
}

static char *
iptables_id_to_rule(const char *id)
{
    char *rule = strdup(id);
    char *p  = rule;

    for ( ; *p != '\0' ; p++)
    {
        if (*p == '#')
            *p = ' ';
        else if (*p == ';')
            *p = ':';
        else if (*p == '|')
            *p = '/';
    }

    return rule;
}

/* See description in tapi_cfg_iptables.h */
int
tapi_cfg_iptables_rule_add(const char *ta,
                           const char *ifname,
                           const char *table,
                           const char *rule)
{
    int rc;
    cfg_handle handle;
    char *rule_id = NULL;

    assert(ifname != NULL);
    assert(table != NULL);
    assert(rule != NULL);

    RING("Add rule(TA:%s, ifname:%s, table:%s): %s",
         ta, ifname, table, rule);

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s/iptables:",
                                  ta, ifname)) != 0)
    {
        ERROR("Error while synchronizing iptables rules on %s Agent", ta);
    }

    rule_id = iptables_rule_to_id(rule);
    if (rule_id == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);
    rc = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                              "/agent:%s/interface:%s/iptables:"
                              "/table:%s/rule:%s", ta, ifname, table,
                              rule_id);
    free(rule_id);
    if (rc != 0)
    {
        ERROR("Error while adding iptables rule: %x", rc);
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s/iptables:",
                                  ta, ifname)) != 0)
    {
        ERROR("Error while synchronizing iptables rules on %s Agent", ta);
    }

    return rc;
}

/* See description in tapi_cfg_iptables.h */
int
tapi_cfg_iptables_rule_del(const char *ta,
                           const char *ifname,
                           const char *table,
                           const char *rule)
{
    int rc;
    char *rule_id = NULL;

    assert(ifname != NULL);
    assert(table != NULL);
    assert(rule != NULL);

    RING("Delete rule(TA:%s, ifname:%s, table:%s): %s",
         ta, ifname, table, rule);

    rule_id = iptables_rule_to_id(rule);
    if (rule_id == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);
    rc = cfg_del_instance_fmt(FALSE, "/agent:%s/interface:%s/iptables:"
                              "/table:%s/rule:%s", ta, ifname, table,
                              rule_id);
    free(rule_id);
    if (rc != 0)
    {
        ERROR("Error while deleting iptables rule: %x", rc);
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s/iptables:",
                                  ta, ifname)) != 0)
    {
        ERROR("Error while synchronizing iptables rules on %s Agent", ta);
    }

    return rc;
}



