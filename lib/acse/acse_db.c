/** @file
 * @brief ACSE CWMP Dispatcher
 *
 * ACS Emulator support
 *
 *
 * Copyright (C) 2009-2010 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stddef.h>
#include<string.h>

#include "acse_internal.h"

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"

/** The list af acs instances */
acs_list_t acs_list = LIST_HEAD_INITIALIZER(&acs_list); 


/**
 * Add an ACS object to internal DB
 *
 * @param acs_name      Name of the ACS
 *
 * @return              status code
 */
extern te_errno
db_add_acs(const char *acs_name)
{
    acs_t *acs_item;

    if (acs_name == NULL || strlen(acs_name) == 0)
        return TE_RC(TE_ACSE, TE_EINVAL);

    acs_item = db_find_acs(acs_name);

    if (acs_item != NULL)
        return TE_RC(TE_ACSE, TE_EEXIST);

    if ((acs_item = malloc(sizeof(*acs_item))) == NULL)
        return TE_RC(TE_ACSE, TE_ENOMEM);

    memset(acs_item, 0, sizeof(*acs_item));
    acs_item->name = strdup(acs_name);

    LIST_INIT(&acs_item->cpe_list);
    LIST_INSERT_HEAD(&acs_list, acs_item, links);

    return 0;
}


/**
 * Add a CPE record for particular ACS object to internal DB
 *
 * @param acs_name      Name of the ACS
 * @param cpe_name      Name of the CPE 
 *
 * @return              status code
 */
extern te_errno
db_add_cpe(const char *acs_name, const char *cpe_name)
{
    acs_t *acs_item;
    cpe_t *cpe_item;

    if (acs_name == NULL || strlen(acs_name) == 0 ||
        cpe_name == NULL || strlen(cpe_name) == 0)
        return TE_RC(TE_ACSE, TE_EINVAL);

    if ((acs_item = db_find_acs(acs_name)) == NULL)
        return TE_RC(TE_ACSE, TE_ENOENT);

    if ((cpe_item = db_find_cpe(acs_item, acs_name, cpe_name)) != NULL)
        return TE_RC(TE_ACSE, TE_EEXIST);

    if ((cpe_item = malloc(sizeof(*cpe_item))) == NULL)
        return TE_RC(TE_ACSE, TE_ENOMEM);

    memset(cpe_item, 0, sizeof(*cpe_item));
    cpe_item->name = strdup(cpe_name);

    TAILQ_INIT(&(cpe_item->rpc_queue));
    TAILQ_INIT(&(cpe_item->rpc_results));

    LIST_INIT(&cpe_item->inform_list);

    LIST_INSERT_HEAD(&(acs_item->cpe_list), cpe_item, links);

    return 0;
}

/**
 * Find an acs instance from the acs list
 *
 * @param acs_name      Name of the acs instance
 *
 * @return              Acs instance address or NULL if not found
 */
extern acs_t *
db_find_acs(const char *acs_name)
{
    acs_t *item;

    LIST_FOREACH(item, &acs_list, links)
    {
        if (strcmp(item->name, acs_name) == 0)
            return item;
    }

    return NULL;
}

/**
 * Find a cpe instance from the cpe list of an acs instance
 *
 * @param acs_name      Name of the acs instance
 * @param cpe_name      Name of the cpe instance
 *
 * @return              Cpe instance address or NULL if not found
 */
extern cpe_t *
db_find_cpe(acs_t *acs_item, const char *acs_name, const char *cpe_name)
{
    cpe_t *cpe_item;

    if (acs_item == NULL)
    {
        LIST_FOREACH(acs_item, &acs_list, links)
        {
            if (strcmp(acs_item->name, acs_name) == 0)
                break;
        }
    }

    if (acs_item != NULL)
    {
        LIST_FOREACH(cpe_item, &acs_item->cpe_list, links)
        {
            if (strcmp(cpe_item->name, cpe_name) == 0)
                return cpe_item;
        }
    }

    return NULL;
}

te_errno
db_remove_acs(acs_t *acs_item)
{
    /* TODO */
    return 0;
}

te_errno
db_remove_cpe(cpe_t *cpe_item)
{
    /* TODO */
    return 0;
}
