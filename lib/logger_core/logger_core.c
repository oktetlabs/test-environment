/** @file
 * @brief Logger subsystem API - core
 *
 * Core library which must have no TE dependencies and required
 * for any entity which would like to use TE logging.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#include "te_config.h"

#include <stdio.h>

#include "te_defs.h"
#include "logger_api.h"


/* See logger_api.h for the description */
const char *te_lgr_entity = "UNSPECIFIED";

/* See logger_api.h for the description */
void
te_log_init(const char *lgr_entity)
{
    te_lgr_entity = lgr_entity;
}
