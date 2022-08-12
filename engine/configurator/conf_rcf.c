/** @file
 * @brief Configurator
 *
 * RCF interaction auxiliary routines
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#include "te_errno.h"

#include "te_string.h"
#include "rcf_api.h"
#include "conf_db.h"
#include "conf_ta.h"

/**
 * Find son with specified subid and optional name.
 */
static cfg_instance *
cfg_db_find_son(cfg_instance *father, const char *subid, const char *name)
{
    cfg_instance *inst;

    for (inst = father->son; inst != NULL; inst = inst->brother)
    {
         if (strcmp(inst->obj->subid, subid) != 0)
             continue;

         if (name == NULL || strcmp(inst->name, name) == 0)
             break;
    }

    return inst;
}


/** rcfunix parameters to string convertion rules */
struct cfg_rcfunix_conf_param {
    const char *name;       /**< Parameter name */
    te_bool required;       /**< Is parameter required? */
    te_bool has_value;      /**< Does it have a value? */
};

static te_errno
cfg_rcfunix_make_confstr(te_string *confstr, cfg_instance *ta)
{
    static const struct cfg_rcfunix_conf_param params[] = {
        { "host",         FALSE,  TRUE  },
        { "port",         TRUE,   TRUE  },
        { "user",         FALSE,  TRUE  },
        { "key",          FALSE,  TRUE  },
        { "ssh_port",     FALSE,  TRUE  },
        { "ssh_proxy",    FALSE,  TRUE  },
        { "copy_timeout", FALSE,  TRUE  },
        { "copy_tries",   FALSE,  TRUE  },
        { "kill_timeout", FALSE,  TRUE  },
        { "notcopy",      FALSE,  FALSE },
        { "sudo",         FALSE,  FALSE },
        { "connect",      FALSE,  TRUE  },
        { "opaque",       FALSE,  TRUE  },
        { NULL,           FALSE,  FALSE },
    };
    const struct cfg_rcfunix_conf_param *p;
    cfg_instance *inst;
    te_errno rc = TE_ENOENT;

    for (p = params; p->name != NULL; p++)
    {
        inst = cfg_db_find_son(ta, "conf", p->name);
        if (inst == NULL)
        {
            if (p->required)
            {
                rc = TE_EINVAL;
                break;
            }

            if (confstr->len == 0)
            {
                rc = te_string_append(confstr, ":");
                if (rc != 0)
                    break;
            }
            continue;
        }

        if (inst->obj->type != CVT_STRING)
        {
            rc = TE_EBADTYPE;
            break;
        }

        if (!p->has_value && inst->val.val_str[0] != '\0')
        {
            ERROR("rcfunix parameter %s without value has it '%s'",
                  p->name, inst->val.val_str);
            rc = TE_EINVAL;
            break;
        }

        /* Skip optional parameters which has value, but value is empty */
        if (confstr->len != 0 && p->has_value && !p->required &&
            inst->val.val_str[0] == '\0')
            continue;

        rc = te_string_append(confstr, "%s%s%s:",
                              p->name,
                              p->has_value ? "=" : "",
                              p->has_value ? inst->val.val_str : "");
        if (rc != 0)
            break;
    }

    if (rc != 0)
        te_string_free(confstr);

    return TE_RC(TE_CS, rc);
}

static te_errno
cfg_rcf_ta_sync(const char *ta_name)
{
    char oid[CFG_INST_NAME_MAX + strlen("/agent:")];
    int ret;

    ret = snprintf(oid, sizeof(oid), "/agent:%s", ta_name);
    if (ret >= (int)sizeof(oid))
        return TE_RC(TE_CS, TE_ESMALLBUF);

    return cfg_ta_sync(oid, TRUE);
}

static te_errno
cfg_rcf_add_ta(cfg_instance *ta)
{
    te_errno rc;
    unsigned int flags = RCF_TA_NO_SYNC_TIME;
    cfg_instance *rcflib = cfg_db_find_son(ta, "rcflib", "");
    cfg_instance *synch_time;
    cfg_instance *rebootable;
    te_string confstr = TE_STRING_INIT;

    if (rcflib == NULL)
    {
        ERROR("Cannot add TA %s: rcflib unspecified", ta->name);
        return TE_RC(TE_CS, TE_EINVAL);
    }
    if (rcflib->obj->type != CVT_STRING)
    {
        ERROR("Cannot add TA %s: rcflib value is not string", ta->name);
        return TE_RC(TE_CS, TE_EINVAL);
    }

    /* confstr composition is rcfunix-specific */
    if (strcmp(rcflib->val.val_str, "rcfunix") != 0)
    {
        ERROR("Cannot add TA %s: rcflib %s is not supported",
              ta->name, rcflib->val.val_str);
        return TE_RC(TE_CS, TE_EINVAL);
    }
    rc = cfg_rcfunix_make_confstr(&confstr, ta);
    if (rc != 0)
        return rc;

    if ((synch_time = cfg_db_find_son(ta, "synch_time", "")) != NULL &&
        synch_time->val.val_int != 0)
        flags &= ~RCF_TA_NO_SYNC_TIME;
    if ((rebootable = cfg_db_find_son(ta, "rebootable", "")) != NULL &&
        rebootable->val.val_int != 0)
        flags |= RCF_TA_REBOOTABLE;

    rc = rcf_add_ta(ta->name, ta->val.val_str, rcflib->val.val_str,
                    confstr.ptr, flags);
    if (rc == 0)
    {
        rc = cfg_rcf_ta_sync(ta->name);
        if (rc != 0)
        {
            te_errno rc2;

            ERROR("Added test agent '%s' configuration sync failed: %r - "
                  "delete it", ta->name, rc);
            if ((rc2 = rcf_del_ta(ta->name)) != 0)
                ERROR("Cannot delete just created test agent '%s': %r",
                      ta->name, rc2);
        }
    }

    te_string_free(&confstr);

    return rc;
}

static te_errno
cfg_rcf_del_ta(cfg_instance *ta)
{
    te_errno rc;
    te_errno rc_sync;

    rc = rcf_del_ta(ta->name);
    if (rc != 0)
        ERROR("Cannot delete test agent '%s': %r", ta->name, rc);

    rc_sync = cfg_rcf_ta_sync(ta->name);
    if (rc_sync != 0)
        ERROR("Deleted test agent '%s' configuration sync failed: %r",
              ta->name, rc_sync);

    /*
     * We do not want to rollback deletion back because of sync failure.
     * So, we should ignore sync failure return code (except logging
     * done above).
     */
    return rc;
}

/**
 * Check RCF subtree instance and get corresponding RCF agent instance.
 */
static te_errno
cfg_rcf_agent(cfg_instance * const inst, cfg_instance **inst_agent,
              cfg_instance **inst_agent_status)
{
    cfg_instance *inst_rcf;
    cfg_instance *inst_son = NULL;

    for (inst_rcf = inst;
         inst_rcf->father != &cfg_inst_root;
         inst_son = inst_rcf, inst_rcf = inst_rcf->father);

    assert(strcmp(inst_rcf->obj->oid, "/rcf") == 0);
    /* RCF root must have empty name */
    if (inst_rcf->name[0] != '\0')
    {
        ERROR("Invalid RCF OID '%s': non-empty RCF name", inst->oid);
        return TE_RC(TE_CS, TE_EINVAL);
    }

    /* RCF root access itself - no problem */
    if (inst_son == NULL)
    {
        *inst_agent = NULL;
        return 0;
    }

    if (strcmp(inst_son->obj->oid, "/rcf/agent") != 0)
    {
        ERROR("Invalid RCF OID '%s': not agent", inst->oid);
        return TE_RC(TE_CS, TE_EINVAL);
    }

    if (inst_son->name[0] == '\0')
    {
        ERROR("Invalid RCF OID '%s': empty agent name", inst->oid);
        return TE_RC(TE_CS, TE_EINVAL);
    }

    *inst_agent = inst_son;
    *inst_agent_status = cfg_db_find_son(inst_son, "status", "");

    /* Deny any changes if its not a status leaf and the agent is running */
    if (*inst_agent_status != NULL && inst != *inst_agent_status &&
        (*inst_agent_status)->val.val_int != 0)
    {
        ERROR("Cannot reconfigure running RCF agent '%s'", inst->oid);
        return TE_RC(TE_CS, TE_EPERM);
    }

    return 0;
}

te_errno
cfg_rcf_add(cfg_instance *inst)
{
    cfg_instance *inst_agent;
    cfg_instance *inst_agent_status;
    te_errno rc;

    rc = cfg_rcf_agent(inst, &inst_agent, &inst_agent_status);
    if (rc != 0 || inst_agent == NULL || inst == inst_agent)
        return rc;

    if (inst == inst_agent_status && inst->val.val_int != 0)
        rc = cfg_rcf_add_ta(inst_agent);

    return rc;
}

te_errno
cfg_rcf_del(cfg_instance *inst)
{
    cfg_instance *inst_agent;
    cfg_instance *inst_agent_status;
    te_errno rc;

    rc = cfg_rcf_agent(inst, &inst_agent, &inst_agent_status);
    if (rc != 0 || inst_agent == NULL || inst == inst_agent)
        return rc;

    if (inst == inst_agent_status || inst == inst_agent)
        rc = cfg_rcf_del_ta(inst_agent);

    return rc;
}

te_errno
cfg_rcf_set(cfg_instance *inst)
{
    cfg_instance *inst_agent;
    cfg_instance *inst_agent_status;
    te_errno rc;

    rc = cfg_rcf_agent(inst, &inst_agent, &inst_agent_status);
    if (rc != 0 || inst_agent == NULL || inst == inst_agent)
        return rc;

    if (inst == inst_agent_status) {
        if (inst->val.val_int != 0)
            rc = cfg_rcf_add_ta(inst_agent);
        else
            rc = cfg_rcf_del_ta(inst_agent);
    }

    return rc;
}
