/** @file
 * @brief Testing Results Comparator: common
 *
 * Helper functions to work with TRC tags.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TRC_TAGS_H__
#define __TE_TRC_TAGS_H__

#include "te_errno.h"
#include "tq_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add TRC tag into the list.
 *
 * @param tags          List with TRC tags
 * @param name          Name of the tag to add
 *
 * @return Status code.
 */
extern te_errno trc_add_tag(tqh_strings *tags, const char *name);

/**
 * Parse string with TRC tags and add them into the list.
 *
 * @param tags          List to add tags
 * @param tags_str      String with tags
 *
 * @return Status code.
 */
extern te_errno trc_tags_str_to_list(tqh_strings *tags, char *tags_str);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_TAGS_H__ */
