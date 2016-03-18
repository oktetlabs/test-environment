/** @file
 * @brief Test API to local file routines
 *
 * Functions for convinient work with the files on the engine and TA.
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

#define TE_LGR_USER     "TAPI Local File"

#include "te_config.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "tapi_local_file.h"


/* See description in tapi_local_file.h. */
const char *
tapi_local_file_get_name(const tapi_local_file *file)
{
    const char *basename;

    assert(file != NULL);
    if (file->pathname == NULL)
        return NULL;

    basename = strrchr(file->pathname, '/');
    return (basename != NULL ? basename + 1 : file->pathname);
}

/* See description in tapi_local_file.h. */
te_bool
tapi_local_file_cmp(const tapi_local_file *file1,
                    const tapi_local_file *file2)
{
    assert(file1 != NULL && file2 != NULL);
    return (file1->type == file2->type &&
            file1->property.size == file2->property.size &&
            ((file1->pathname != NULL && file2->pathname != NULL &&
              strcmp(file1->pathname, file2->pathname) == 0) ||
             (file1->pathname == NULL && file2->pathname == NULL)));
}
