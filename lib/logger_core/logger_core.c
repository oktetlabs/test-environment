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

/**
 * Default logging backed to avoid bad crashes if the backend is unset.
 */
static void
te_log_message_def(const char      *file,
                   unsigned int     line,
                   te_log_ts_sec    sec,
                   te_log_ts_usec   usec,
                   unsigned int     level,
                   const char      *entity,
                   const char      *user,
                   const char      *fmt,
                   va_list          ap)
{
    UNUSED(sec);
    UNUSED(usec);
    UNUSED(level);
    UNUSED(ap);

    fprintf(stderr,
            "BUG: Logging backend is unset at %s:%u by %s:%s for '%s'\n",
            file, line, entity, user, fmt);
}

/* See logger_api.h for the description */
te_log_message_f *te_log_message_va = te_log_message_def;

/* See logger_api.h for the description */
void
te_log_init(const char *lgr_entity, te_log_message_f *log_message)
{
    if (lgr_entity != NULL)
        te_lgr_entity = lgr_entity;
    if (log_message != NULL)
        te_log_message_va = log_message;
}
