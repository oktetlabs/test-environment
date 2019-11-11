/** @file
 * @brief Logger subsystem API
 *
 * Logger library to be used by standalone TE off-line applications.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Logger File"

#include "te_config.h"

#include "logger_api.h"

#include "logger_file.h"

/**
 * Initialize logging backend for standalone TE off-line applications.
 */
__attribute__((constructor))
static void
logger_file_init(void)
{
    te_log_message_va = te_log_message_file;
}
