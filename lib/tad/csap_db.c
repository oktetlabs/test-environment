/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Implementation of CSAP dynamic DB methods * 
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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "tad.h"
#include "te_errno.h"
#include "logger_ta.h"
#include "logger_api.h"

#ifndef INSQUE
/* macros to insert element p into queue _after_ element q */
#define INSQUE(p, q) {(p)->prev = q; (p)->next = (q)->next; \
                      (q)->next = p; (p)->next->prev = p; }
/* macros to remove element p from queue  */
#define REMQUE(p) {(p)->prev->next = (p)->next; (p)->next->prev = (p)->prev; \
                   (p)->next = (p)->prev = p; }
#endif


/*
 * Forward declarations of local functions.
 */
static void csap_free(csap_p csap_descr);

struct csap_db_entry;
typedef struct csap_db_entry *csap_db_entry_p;
typedef struct csap_db_entry 
{
    csap_db_entry_p next; 
    csap_db_entry_p prev; 
    csap_p          rec;
} csap_db_entry;

static csap_db_entry root_csap_db;
static int start_position;

extern char *ta_name;


/**
 * Initialize CSAP database.
 *
 * @return zero on success, otherwise error code 
 */ 
int 
csap_db_init()
{
    int seed;
    root_csap_db.next = root_csap_db.prev = &root_csap_db;
    root_csap_db.rec = NULL;

    strncpy ((char *)&seed, ta_name, sizeof (seed));
    srandom(seed);
    #if 1
    start_position = 1;
    #else
    start_position = random();
    #endif
    VERB("Init with seed %d, start_position %d", 
                        seed, start_position);
    return 0;
}

/**
 * Clear CSAP database.
 *
 * @return zero on success, otherwise error code 
 */ 
int 
csap_db_clear()
{
    return 1; /* i think, it is not necessary now.. */ 
}

#define MAX_CSAP_DEPTH 200
/**
 * Create new CSAP. 
 *      This method does not perform any actions related to CSAP functionality,
 *      neither processing of CSAP init parameters, nor initialyzing some 
 *      communication media units (for example, sockets, etc.).
 *      It only allocates memory for csap_instance sturture, set fields
 *      'id', 'depth' and 'proto' in it and allocates memory for 'layer_data'.
 * 
 * @param type  Type of CSAP: dot-separated sequence of textual layer labels.
 *
 * @return identifier of new CSAP or zero if error occured.
 */ 
int 
csap_create(const char *type)
{
    char *csap_type = strdup(type);
    int depth = 0, i, next_id = start_position;
    csap_p new_csap = calloc(1, sizeof(csap_instance));
    csap_db_entry_p dp, new_dp = calloc(1, sizeof(csap_db_entry)); 
    char *layer_protos[MAX_CSAP_DEPTH];
    VERB("In csap_create: %s\n", csap_type);

    if ((new_csap == NULL) || (new_dp == NULL)) 
    { 
        errno = ENOMEM;
        return 0;
    }
    new_dp->rec = new_csap;

    /* Find free identifier, set it to new CSAP and insert structure to DB. */
    for (dp = root_csap_db.next; dp!=&root_csap_db; dp = dp->next, next_id++)
        if ( next_id < dp->rec->id) break;

    VERB("In csap_create, new id: %d\n", next_id);
    new_csap->id = next_id;
    INSQUE(new_dp, dp->prev);

    {
        pthread_mutex_t da_lock_mutex = PTHREAD_MUTEX_INITIALIZER;
        new_csap->data_access_lock = da_lock_mutex;
    }

    if (strncmp(csap_type, "data", 4) != 0)
    {
        layer_protos[0] = csap_type;
        depth++; 
    }
    else
        new_csap->type = TAD_DATA_CSAP;

    for (i = 0; csap_type[i]; i++) 
    {
        if (csap_type[i] == '.') 
        {
            char *next_layer = &csap_type[i + 1];
            csap_type[i] = 0;
            layer_protos[depth] = next_layer;
            depth++; 
            VERB("In csap_create, next_layer: %s\n", next_layer);
        }
    }

    new_csap->depth = depth;

    /* Allocate memory for stack arrays */ 
    if ((new_csap->proto        = calloc(depth, sizeof(char*)))==NULL ||
        (new_csap->layer_data   = calloc(depth, sizeof(void*)))==NULL ||
        (new_csap->get_param_cb = calloc(depth, sizeof(csap_get_param_cb_t) 
                                                       ))==NULL 
       )
    { 
        errno = ENOMEM;
        REMQUE(new_dp); free(new_dp); csap_free(new_csap); 
        return 0; 
    }

    memcpy ( new_csap->proto, layer_protos, sizeof(char *) * depth);

    for (i = 0; i < depth; i++)
        VERB("In csap_create, layer %d: %s\n", i, new_csap->proto[i]);


    return new_csap->id;
}

/**
 * Find CSAP DB entry by CSAP identifier.
 *
 * @param csap_id       Identifier of CSAP 
 *
 * @return 
 *    Pointer to structure with CSAP DB entry or NULL if not found. 
 */ 
static csap_db_entry_p
csap_db_entry_find (int id)
{
    csap_db_entry_p dp;

    for ( dp = root_csap_db.next; ; dp = dp->next)
        if ((dp == &root_csap_db) || (dp->rec->id > id)) return NULL; 
        else if (dp->rec->id == id) return dp;

    return NULL;
}


/** 
 * Free all memory allocatad for all common CSAP data.
 *
 * @param csap_descr    pointer to CSAP descriptor.
 *
 * @return nothing
 */
static void
csap_free(csap_p csap_descr)
{
    if (csap_descr == NULL)
        return;

    VERB("called for # %d, ptrs pr %x, l %x, g_p %x", 
            csap_descr->id, csap_descr->proto, csap_descr->layer_data, 
            csap_descr->get_param_cb);
    
    /* memory for protocol layer sub-ids was allocated
       in csap_create as single interval by strdup */ 
    free (csap_descr->proto[0]);

    free (csap_descr->proto); 

    if (csap_descr->layer_data)
    {
        int i;
        for (i = 0; i < csap_descr->depth; i++)
            if (csap_descr->layer_data[i]) 
                free(csap_descr->layer_data[i]);

        free (csap_descr->layer_data);
    }

    free (csap_descr->get_param_cb);

    free(csap_descr);
}

/**
 * Destroy CSAP.
 *      Before call this DB method, all protocol-specific data in 'layer-data'
 *      and underground media resources should be freed. 
 *      This method will free all non-NULL pointers in 'layer-data', but 
 *      does not know nothing about what structures are pointed by them, 
 *      therefore if there are some more pointers in that structures, 
 *      memory may be lost. 
 *
 * @param csap_id       Identifier of CSAP to be destroyed.
 *
 * @return zero on success, otherwise error code 
 */ 
int
csap_destroy (int csap_id)
{
    csap_p c_p;
    csap_db_entry_p dp = csap_db_entry_find (csap_id); 

    VERB("called for #1", csap_id);

    if (dp == NULL) return 1; /* correct error code should be returned! */ 

    c_p = dp->rec;
    csap_free(c_p);

    REMQUE(dp);
    free(dp);

    return 0;
}

/**
 * Find CSAP by its identifier.
 *
 * @param csap_id       Identifier of CSAP 
 *
 * @return 
 *    Pointer to structure with internal CSAP information or NULL if not found. 
 *    Change data in this structure if you really know what does it mean!
 */ 
csap_p
csap_find (int csap_id)
{
    csap_db_entry_p dp = csap_db_entry_find (csap_id);
    if (dp == NULL) return NULL;
    return dp->rec;
}



