/** @file
 * @brief Configurator
 *
 * Configurator main loop
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_CONF_DEFS_H__
#define __TE_CONF_DEFS_H__

#include "config.h"
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
#include "conf_api.h"
#include "conf_messages.h"
#include "conf_types.h"
#include "conf_db.h"
#include "conf_dh.h"
#include "conf_backup.h"
#include "conf_ta.h"

#include "ipc_server.h"
#include "rcf_api.h"

#include "logger_api.h"
#include "logger_ten.h"

/** Check if the instance is volatile */
static inline te_bool
cfg_instance_volatile(cfg_instance *inst)
{
    return strncmp(inst->oid, "/"CFG_VOLATILE":", 
                   strlen("/"CFG_VOLATILE":")) == 0  ||
           strncmp(inst->obj->oid, "/agent/"CFG_VOLATILE, 
                   strlen("/agent/"CFG_VOLATILE)) == 0;
}

#endif /* !__TE_CONF_DEFS_H__ */
