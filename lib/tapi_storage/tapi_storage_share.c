/** @file
 * @brief Test API to storage share routines
 *
 * Functions for convenient work with storage share.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Storage Share"

#include "te_defs.h"
#include "tapi_storage_share.h"


/* See description in tapi_storage_share.h. */
void
tapi_storage_share_list_free(tapi_storage_share_list *share)
{
    tapi_storage_share_le *s;

    while (!SLIST_EMPTY(share))
    {
        s = SLIST_FIRST(share);
        SLIST_REMOVE_HEAD(share, next);
        free(s->storage);
        free(s->path);
        free(s);
    }
}
