/** @file
 * @brief Queuing Discipline configuration
 *
 * Queuing Discipline configuration
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */


#include "conf_api.h"
#include "logger_api.h"
#include "tapi_cfg.h"

#include "tapi_cfg_qdisc.h"

#define QDISC_FMT   "/agent:%s/interface:%s/tc:/qdisc:"

/* See description in the tapi_cfg_qdisc.h */
te_errno
tapi_cfg_qdisc_get_enabled(const char *ta, const char *if_name,
                           te_bool *enabled)
{
    te_errno rc;
    int int_enabled = 0;

    rc = tapi_cfg_get_int_fmt(&int_enabled, QDISC_FMT "/enabled:",
                              ta, if_name);
    if (rc != 0)
        return rc;

    *enabled = (te_bool)int_enabled;
    return 0;
}

/* See description in the tapi_cfg_qdisc.h */
te_errno
tapi_cfg_qdisc_set_enabled(const char *ta, const char *if_name,
                           te_bool enabled)
{
    te_errno rc;
    te_bool already_enabled = FALSE;

    if ((rc = tapi_cfg_qdisc_get_enabled(ta, if_name, &already_enabled)) != 0)
        return rc;

    if (enabled == already_enabled)
        return 0;

    return tapi_cfg_set_int_fmt(enabled == TRUE ? 1: 0, NULL,
                                QDISC_FMT "/enabled:", ta, if_name);
}

static const char *kinds[] = {
    [TAPI_CFG_QDISC_KIND_NETEM] = "netem",
};

/* See description in the tapi_cfg_qdisc.h */
const char *
tapi_cfg_qdisc_kind2str(tapi_cfg_qdisc_kind_t kind)
{
    if (kind >= 0 && kind < TE_ARRAY_LEN(kinds))
        return kinds[kind];

    return NULL;
}

/* See description in the tapi_cfg_qdisc.h */
tapi_cfg_qdisc_kind_t
tapi_cfg_qdisc_str2kind(const char *string)
{
    size_t kind;

    if (string == NULL)
        return TAPI_CFG_QDISC_KIND_UNKNOWN;

    for (kind = 0; kind < TE_ARRAY_LEN(kinds); kind++)
        if (kinds[kind] != NULL && strcmp(kinds[kind], string) == 0)
            return (tapi_cfg_qdisc_kind_t)kind;

    return TAPI_CFG_QDISC_KIND_UNKNOWN;
}

/* See description in the tapi_cfg_qdisc.h */
te_errno
tapi_cfg_qdisc_set_kind(const char *ta, const char *if_name,
                        tapi_cfg_qdisc_kind_t kind)
{
    te_errno rc;
    const char * kind_str = tapi_cfg_qdisc_kind2str(kind);

    if (kind_str == NULL)
    {
        ERROR("Unknown kind='%d'", kind);
        return TE_EINVAL;
    }

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, kind_str),
                              QDISC_FMT "/kind:", ta, if_name);

    if (rc != 0)
        ERROR("Failed to set kind of qdisc on %s Agent, rc=%r", ta, rc);

    return rc;
}


/* See description in the tapi_cfg_qdisc.h */
te_errno
tapi_cfg_qdisc_get_kind(const char *ta, const char *if_name,
                        tapi_cfg_qdisc_kind_t *kind)
{
    te_errno rc;
    char *kind_str;
    tapi_cfg_qdisc_kind_t k;

    rc = cfg_get_instance_fmt(NULL, &kind_str, QDISC_FMT "/kind:", ta, if_name);
    if (rc != 0)
    {
        ERROR("Failed to get kind of qdisc on %s Agent, rc=%r", ta, rc);
        return rc;
    }

    if ((k = tapi_cfg_qdisc_str2kind(kind_str)) == TAPI_CFG_QDISC_KIND_UNKNOWN)
        rc = TE_EINVAL;
    else
        *kind = k;

    free(kind_str);
    return rc;
}
