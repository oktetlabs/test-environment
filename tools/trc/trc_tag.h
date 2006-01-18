/** @file
 * @brief Testing Results Comparator
 *
 * Definition of TRC tag types and related routines.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TRC_TAG_H__
#define __TE_TRC_TAG_H__

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#else
#error sys/queue.h is required for TRC tool
#endif

#include "te_defs.h"


/** Named tag */
typedef struct trc_tag {
    TAILQ_ENTRY(trc_tag)    links;  /**< List links */

    char                   *name;   /**< Tag name */
} trc_tag;

/** List of named tags */
typedef TAILQ_HEAD(trc_tags, trc_tag)   trc_tags;   /**< List of tags */

/** Entry in the list of list of named tags */
typedef struct trc_tags_entry {
    TAILQ_ENTRY(trc_tags_entry) links;  /**< List links */

    unsigned int    id;         /**< ID of the list */
    trc_tags        tags;       /**< List of tags */
    char           *name;       /**< Name of the set */
    te_bool         show_keys;  /**< Show table with keys which explain
                                     differences */
} trc_tags_entry;

/** List of lists of named tags */
typedef TAILQ_HEAD(trc_tags_list, trc_tags_entry) trc_tags_list;


/** List of tags to get specific expected result */
extern trc_tags tags;

/** Lists of tags to get specific expected result to compare. */
extern trc_tags_list tags_diff;


/**
 * Add tag in the end of the TRC tags list.
 *
 * @param tags      List of tags to be updated
 * @param name      Name of the tag
 *
 * @return Status code.
 */
extern int trc_add_tag(trc_tags *tags, const char *name);

/**
 * Free TRC tags list.
 *
 * @param tags      List of tags to be freed
 */
extern void trc_free_tags(trc_tags *tags);


/**
 * Set name of the TRC tags set with specified ID.
 *
 * @param tags      List of sets of tags
 * @param id        Identifier of the list to be used
 * @param name      Name of the tags set
 *
 * @return Status code.
 */
extern int trc_diff_set_name(trc_tags_list *tags, unsigned int id,
                             const char *name);

/**
 * Enable showing keys of the TRC tags set with specified ID.
 *
 * @param tags      List of sets of tags
 * @param id        Identifier of the list to be used
 *
 * @return Status code.
 */
extern int trc_diff_show_keys(trc_tags_list *tags, unsigned int id);

/**
 * Add tag in the end of the TRC tags set with specified ID.
 *
 * @param tags      List of lists of tags
 * @param id        Identifier of the list to be used
 * @param name      Name of the tag
 *
 * @return Status code.
 */
extern int trc_diff_add_tag(trc_tags_list *tags, unsigned int id,
                            const char *name);

/**
 * Free TRC tags lists.
 *
 * @param tags      List of list of tags to be freed
 */
extern void trc_diff_free_tags(trc_tags_list *tags);


#endif /* !__TE_TRC_TAG_H__ */
