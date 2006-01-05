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

#include <stdlib.h>
#include <string.h>

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "te_errno.h"
#include "logger_api.h"


struct csap_spt_entry;
typedef struct csap_spt_entry *csap_spt_entry_p;

/**
 * CSAP protocol layer support DB entry
 */
typedef struct csap_spt_entry { 
    csap_spt_entry_p  next;     /**< Next descr in queue */
    csap_spt_entry_p  prev;     /**< Prev layer in queue */
    csap_spt_type_p   spt_data; /**< Pointer to support descriptor */ 
} csap_spt_entry_t;

static csap_spt_entry_t csap_spt_root = {
    &csap_spt_root,
    &csap_spt_root,
    NULL
};

/**
 * Init CSAP support database
 *
 * @return zero on success, otherwise error code. 
 */
int 
csap_spt_init(void)
{
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
    csap_spt_entry_p new_spt_entry = malloc(sizeof(*new_spt_entry));

    if (new_spt_entry == NULL) 
        return TE_ENOMEM;

    new_spt_entry->spt_data = spt_descr;
    INSQUE(new_spt_entry, &csap_spt_root);

    INFO("Registered '%s' protocol support", spt_descr->proto);

    return 0;
}

/**
 * Find structure for CSAP support respective to passed protocol label.
 *
 * @param proto      protocol label.
 *
 * @return pointer to structure or NULL if not found. 
 */
csap_spt_type_p 
csap_spt_find(const char *proto)
{
    csap_spt_entry_p spt_entry;

    VERB("%s(): asked proto %s", __FUNCTION__, proto);

    for (spt_entry = csap_spt_root.next; 
         spt_entry!= &csap_spt_root; 
         spt_entry = spt_entry->next)
    { 
        if (spt_entry->spt_data)
            VERB("test proto %s", spt_entry->spt_data->proto);

        if (spt_entry->spt_data && 
            (strcmp(spt_entry->spt_data->proto, proto) == 0))
            return spt_entry->spt_data;
    } 
    return NULL;
}


