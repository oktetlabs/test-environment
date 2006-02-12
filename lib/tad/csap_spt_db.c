/** @file
 * @brief TAD CSAP Support Database
 *
 * Traffic Application Domain Command Handler.
 * Implementation of CSAP support DB methods.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "CSAP support"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#include "te_errno.h"
#include "logger_api.h"

#include "tad_csap_support.h"


/**
 * CSAP protocol layer support DB entry
 */
typedef struct csap_spt_entry { 
    TAILQ_ENTRY(csap_spt_entry) links;  /**< List links */

    csap_spt_type_p   spt_data; /**< Pointer to support descriptor */ 

} csap_spt_entry_t;


/** Head of the CSAP protocol support list */
static TAILQ_HEAD(, csap_spt_entry) csap_spt_root;


/* See the description in tad_csap_support.h */
te_errno
csap_spt_init(void)
{
    TAILQ_INIT(&csap_spt_root);
    return 0;
}

/**
 * Add structure for CSAP support for respective protocol.
 *
 * @param spt_descr     CSAP layer support structure. 
 *
 * @return Zero on success, otherwise error code. 
 *
 * @todo check for uniqueness of protocol labels. 
 */
te_errno
csap_spt_add(csap_spt_type_p spt_descr)
{
    csap_spt_entry_t   *new_spt_entry;

    if (spt_descr == NULL)
        return TE_EINVAL;

    new_spt_entry = malloc(sizeof(*new_spt_entry));
    if (new_spt_entry == NULL) 
        return TE_ENOMEM;

    new_spt_entry->spt_data = spt_descr;
    TAILQ_INSERT_TAIL(&csap_spt_root, new_spt_entry, links);

    INFO("Registered '%s' protocol support", spt_descr->proto);

    return 0;
}

/* See the description in tad_csap_support.h */
csap_spt_type_p 
csap_spt_find(const char *proto)
{
    csap_spt_entry_t   *spt_entry;

    VERB("%s(): asked proto %s", __FUNCTION__, proto);

    for (spt_entry = csap_spt_root.tqh_first; 
         spt_entry != NULL; 
         spt_entry = spt_entry->links.tqe_next)
    { 
        assert(spt_entry->spt_data != NULL);
        VERB("%s(): test proto %s", __FUNCTION__,
             spt_entry->spt_data->proto);

        if (strcmp(spt_entry->spt_data->proto, proto) == 0)
            return spt_entry->spt_data;
    } 
    return NULL;
}

/* See the description in tad_csap_support.h */
void
csap_spt_destroy(void)
{
    csap_spt_entry_t   *entry;

    while ((entry = csap_spt_root.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&csap_spt_root, entry, links);

        assert(entry->spt_data != NULL);
        if (entry->spt_data->unregister_cb != NULL)
            entry->spt_data->unregister_cb();

        free(entry);
    }
}
