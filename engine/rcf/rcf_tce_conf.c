/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TE RCF Engine - TCE configuration
 *
 * Internal functions to access the TCE configuration.
 */

#include "te_config.h"

#include <string.h>

#include "rcf_tce_conf.h"

const rcf_tce_comp_conf_t *
rcf_tce_get_next_comp_conf(const rcf_tce_type_conf_t *type,
                           const rcf_tce_comp_conf_t *comp)
{
    if (comp == NULL)
        return SLIST_FIRST(&type->comp);
    else
        return SLIST_NEXT(comp, next);
}

const rcf_tce_type_conf_t *
rcf_tce_get_type_conf(const rcf_tce_conf_t *conf, const char *type)
{
    rcf_tce_type_conf_t *type_conf;

    if (conf == NULL)
        return NULL;

    SLIST_FOREACH(type_conf, &conf->types, next)
    {
        if (strcmp(type, type_conf->name) == 0)
            return type_conf;
    }

    return NULL;
}
