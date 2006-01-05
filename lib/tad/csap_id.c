/** @file
 * @brief TAD CSAP IDs
 *
 * Traffic Application Domain Command Handler.
 * Implementation of CSAP IDs database functions.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD CSAP IDs"

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
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#else
#error sys/queue.h is required for CSAP IDs database
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "tad_common.h"
#include "csap_id.h"


/* FIXME: Put it in rcf_pch.h and include it here */
extern char *ta_name;


/** Compilation flag, if true - start CSAP IDs from 1 */
#define SIMPLE_CSAP_IDS 1


/** CSAP ID database entry. */
typedef struct csap_id_entry {
    LIST_ENTRY(csap_id_entry)   links;  /**< List links */

    csap_handle_t   id;     /**< CSAP ID */
    void           *ptr;    /**< Associated pointer */
} csap_id_entry;


/** Head of the CSAP ID database */
static LIST_HEAD(csap_id_list, csap_id_entry) csap_id_db;

/** The first CSAP ID */
static unsigned int csap_id_start;


/* See description in csap_id.h */
void
csap_id_init(void)
{
    LIST_INIT(&csap_id_db);

#if SIMPLE_CSAP_IDS
    csap_id_start = CSAP_INVALID_HANDLE + 1;
#else
    /* 
     * Sometimes there was necessity to 'almost unique' CSAP IDs on
     * all TA 
     */
    unsigned int    seed;
    unsigned int    i;

    for (seed = time(NULL), i = 0;
         i < strlen(ta_name);
         seed += i * ta_name[i], ++i);

    srandom(seed);

    do {
        csap_id_start = random();
    } while (csap_id_start == CSAP_INVALID_HANDLE);

    INFO("Initialized with seed %u, start position is %u", 
         seed, csap_id_start);
#endif
}

/* See description in csap_id.h */
void
csap_id_destroy(void)
{
    /* FIXME */
}


/* See description in csap_id.h */
csap_handle_t
csap_id_new(void *ptr)
{
    te_bool         was_invalid = FALSE;
    csap_id_entry  *curr, *prev;
    csap_handle_t   new_id;
    csap_id_entry  *new_entry;

    if (ptr == NULL)
    {
        ERROR("It is not allowed to associate NULL pointer with CSAP ID");
        return CSAP_INVALID_HANDLE;
    }

    for (curr = csap_id_db.lh_first, prev = NULL, new_id = csap_id_start; 
         (curr != NULL) && (new_id >= curr->id);
         prev = curr, curr = curr->links.le_next)
    {
        new_id++;
        if (new_id == CSAP_INVALID_HANDLE)
        {
            if (was_invalid)
            {
                ERROR("All CSAP IDs are used");
                return CSAP_INVALID_HANDLE;
            }

            was_invalid = TRUE;
            new_id++;
        }
    }

    new_entry = malloc(sizeof(*new_entry));
    if (new_entry == NULL)
        return CSAP_INVALID_HANDLE;

    new_entry->id = new_id;
    new_entry->ptr = ptr;
    if (prev == NULL)
    {
        LIST_INSERT_HEAD(&csap_id_db, new_entry, links);
    }
    else
    {
        LIST_INSERT_AFTER(prev, new_entry, links);
    }

    return new_id;
}

/**
 * Find CSAP IDs DB entry with specified ID.
 *
 * @param csap_id   CSAP ID
 *
 * @return Pointer to CSAP IDs DB entry or NULL
 */
static csap_id_entry *
csap_id_find(csap_handle_t csap_id)
{
    csap_id_entry  *p;

    for (p = csap_id_db.lh_first;
         p != NULL && p->id < csap_id;
         p = p->links.le_next);

    return (p != NULL && p->id == csap_id) ? p : NULL;
}

/* See description in csap_id.h */
void *
csap_id_get(csap_handle_t csap_id)
{
    csap_id_entry  *p;

    p = csap_id_find(csap_id);
    if (p != NULL)
    {
        assert(p->ptr != NULL);
        return p->ptr;
    }
    else
    {
        return NULL;
    }
}

/* See description in csap_id.h */
void *
csap_id_delete(csap_handle_t csap_id)
{
    csap_id_entry  *entry;
    void           *ptr;

    entry = csap_id_find(csap_id);
    if (entry != NULL)
    {
        LIST_REMOVE(entry, links);
        ptr = entry->ptr;
        free(entry);

        assert(ptr != NULL);
        return ptr;
    }
    else
    {
        return NULL;
    }
}


/* See description in csap_id.h */
void
csap_id_enum(csap_id_enum_cb cb)
{
    UNUSED(cb);
    ERROR("%s() is not implemented yet", __FUNCTION__);
}
