/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief iptables Configuration Model TAPI
 *
 * Definition of test API for iptables configuration model
 * (storage/cm/cm_iptables.xml).
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

#include "tapi_cfg_iptables.h"

/* See description in tapi_cfg_iptables.h */
te_errno
tapi_cfg_iptables_chain_set(const char *ta,
                            const char *ifname,
                            const char *table,
                            const char *chain,
                            te_bool     enable)
{
    int rc;

    assert(ta != NULL);
    assert(ifname != NULL);
    assert(table != NULL);
    assert(chain != NULL);

    INFO("Set iptables chain (TA:%s, ifname:%s, table:%s, chain:%s) %s",
         ta, ifname, table, chain, enable ? "ON" : "OFF");

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, enable),
                              "/agent:%s/interface:%s/iptables:"
                              "/table:%s/chain:%s",
                              ta, ifname, table, chain);
    if (rc != 0)
    {
        ERROR("Error while executing iptables rule: %r", rc);
        return rc;
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s"
                                  "/iptables:/table:%s",
                                  ta, ifname, table)) != 0)
    {
        ERROR("Error while synchronizing iptables rules on %s Agent, rc=%r",
              ta, rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_iptables.h */
te_errno
tapi_cfg_iptables_chain_add(const char *ta,
                            const char *ifname,
                            const char *table,
                            const char *chain,
                            te_bool     enable)
{
    int rc;
    cfg_handle handle;

    INFO("Add/Set iptables chain (TA:%s, ifname:%s, table:%s, chain:%s) %s",
         ta, ifname, table, chain, enable ? "ON" : "OFF");

    if ((rc = cfg_find_fmt(&handle, "/agent:%s/interface:%s/iptables:"
                           "/table:%s/chain:%s", ta, ifname,
                           table, chain)) == 0)
    {
        if ((rc = cfg_set_instance(handle, CFG_VAL(INTEGER, enable))) != 0)
        {
            ERROR("Failed to setup iptables chain %s for %s table on %s, "
                  "rc=%r", chain, table, ifname, rc);
            return rc;
        }
    }
    else if ((rc = cfg_add_instance_fmt(&handle, CFG_VAL(INTEGER, enable),
                                        "/agent:%s/interface:%s/iptables:"
                                        "/table:%s/chain:%s",
                                        ta, ifname, table, chain)) != 0)
    {
        ERROR("Failed to add iptables chain %s for %s table on %s, rc=%r",
              chain, table, ifname, rc);
        return rc;
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s"
                                  "/iptables:/table:%s",
                                  ta, ifname, table)) != 0)
    {
        ERROR("Error while synchronizing iptables rules on %s Agent, rc=%r",
              ta, rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_iptables.h */
te_errno
tapi_cfg_iptables_chain_del(const char *ta,
                            const char *ifname,
                            const char *table,
                            const char *chain)
{
    int rc;

    INFO("Delete iptables chain (TA:%s, ifname:%s, table:%s, chain:%s)",
         ta, ifname, table, chain);

    if ((rc = cfg_del_instance_fmt(FALSE,
                                   "/agent:%s/interface:%s/iptables:"
                                   "/table:%s/chain:%s",
                                   ta, ifname, table, chain)) != 0)
    {
        ERROR("Failed to delete chain, rc=%r", rc);
        return rc;
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s"
                                  "/iptables:/table:%s",
                                  ta, ifname, table)) != 0)
    {
        ERROR("Error while synchronizing iptables rules on %s Agent, rc=%r",
              ta, rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_iptables.h */
te_errno
tapi_cfg_iptables_cmd(const char *ta,
                      const char *ifname,
                      const char *table,
                      const char *chain,
                      const char *rule)
{
    int rc;
    cfg_handle handle;

    INFO("Execute iptables rule (TA:%s, ifname:%s, table:%s, chain:%s): %s",
         ta, ifname, table, chain, rule);

    if ((rc = cfg_find_fmt(&handle, "/agent:%s/interface:%s"
                           "/iptables:/table:%s/chain:%s",
                           ta, ifname, table, chain)) != 0)
    {
        ERROR("Chain %s_%s not found, rc=%r", chain, ifname, rc);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, rule),
                                   "/agent:%s/interface:%s/iptables:"
                                   "/table:%s/chain:%s/cmd:",
                                   ta, ifname, table, chain)) != 0)
    {
        ERROR("Error while executing iptables rule, rc=%r", rc);
        return rc;
    }

    if ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s"
                                  "/iptables:/table:%s/chain:%s",
                                  ta, ifname, table, chain)) != 0)
    {
        ERROR("Error while synchronizing iptables rules on %s Agent, rc=%r",
              ta, rc);
        return rc;
    }

    return 0;
}


/* See description in tapi_cfg_iptables.h */
te_errno
tapi_cfg_iptables_cmd_fmt(const char *ta, const char *ifname,
                          const char *table, const char *chain,
                          const char *rule, ...)
{
    char    str[TAPI_CFG_IPTABLES_CMD_LEN_MAX];
    va_list ap;
    int     rc;

    va_start(ap, rule);
    rc = vsnprintf(str, sizeof(str), rule, ap);
    va_end(ap);

    if (rc < 0)
    {
        ERROR("Cannot compose the rule string from the arguments: %r",
              te_rc_os2te(errno));
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((size_t)rc >= sizeof(str))
    {
        ERROR("Too long iptables rule");
        return TE_RC(TE_TAPI, TE_ESMALLBUF);
    }

    return tapi_cfg_iptables_cmd(ta, ifname, table, chain, str);
}
