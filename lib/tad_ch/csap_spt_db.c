/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Implementation of CSAP support DB methods 
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

#include <stdlib.h>
#include <string.h>

#include "tad.h"
#include "te_errno.h"


#ifndef INSQUE
/* macros to insert element p into queue _after_ element q */
#define INSQUE(p, q) {(p)->prev = q; (p)->next = (q)->next; \
                      (q)->next = p; (p)->next->prev = p; }
/* macros to remove element p from queue  */
#define REMQUE(p) {(p)->prev->next = (p)->next; (p)->next->prev = (p)->prev; \
                   (p)->next = (p)->prev = p; }
#endif


struct csap_spt_entry;
typedef struct csap_spt_entry * csap_spt_entry_p;

typedef struct csap_spt_entry
{ 
    csap_spt_entry_p  next, prev;
    csap_spt_type_p   spt_data;  
} csap_spt_entry_t;

static csap_spt_entry_t csap_spt_root;

/**
 * Init CSAP support database
 *
 * @return zero on success, otherwise error code. 
 */
int 
init_csap_spt (void)
{
    csap_spt_root.next = &csap_spt_root;
    csap_spt_root.prev = &csap_spt_root;
    csap_spt_root.spt_data = NULL;
    return 0;
}

/**
 * Add structure for CSAP support for respective protocol.
 *
 * @param spt_descr     CSAP layer support structure. 
 *
 * @return zero on success, otherwise error code. 
 *
 * @todo check for uniqueness of protocol labels. 
 */
int 
add_csap_spt (csap_spt_type_p spt_descr)
{
    csap_spt_entry_p new_spt_entry = malloc(sizeof(csap_spt_entry_t));

    if (new_spt_entry == NULL) return ENOMEM;

    new_spt_entry->spt_data = spt_descr;
    INSQUE (new_spt_entry, &csap_spt_root);

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
find_csap_spt (const char * proto)
{
    csap_spt_entry_p spt_entry;

    for (spt_entry = csap_spt_root.next; 
         spt_entry!= &csap_spt_root; 
         spt_entry = spt_entry->next)
    {
        if (spt_entry->spt_data && 
            (strcmp (spt_entry->spt_data->proto, proto) == 0))
            return spt_entry->spt_data;
    } 
    return NULL;
}


