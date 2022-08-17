/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * Configurator main loop
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_CONF_DEFS_H__
#define __TE_CONF_DEFS_H__

#include "te_config.h"

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#include <sys/socket.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_str.h"
#include "logger_api.h"
#include "logger_ten.h"
#include "rcf_api.h"
#include "conf_api.h"
#include "conf_messages.h"
#include "conf_types.h"
#include "conf_db.h"
#include "conf_dh.h"
#include "conf_backup.h"
#include "conf_ta.h"

#include "ipc_server.h"

/** Check if the instance is volatile */
static inline te_bool
cfg_instance_volatile(cfg_instance *inst)
{
    return inst->obj->vol;
}

/**
 * Process XML document containing dynamic history and
 * synchronise resulting database with Test Agents.
 *
 * @param root_node     Root node of the input document
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 *
 * @return Status code.
 */
extern te_errno parse_config_dh_sync(xmlNodePtr root_node,
                                     te_kvpair_h *expand_vars);

#endif /* !__TE_CONF_DEFS_H__ */
