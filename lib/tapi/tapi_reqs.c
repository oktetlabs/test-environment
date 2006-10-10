/** @file
 * @brief API to modify target requirements from prologues
 *
 * Implementation of API to modify target requirements from prologues.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Reqs"

#include "te_config.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#include "te_errno.h"
#include "conf_api.h"
#include "tapi_test.h"


/* See the description from tapi_reqs.h */
te_errno
tapi_reqs_exclude(const char *reqs)
{
    te_errno        rc;
    cfg_val_type    type = CVT_STRING;
    char           *old = NULL;
    char           *val = NULL;
    int             len;
    int             p;

    if (reqs == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_get_instance_fmt(&type, &old, "/local:/reqs:%u",
                              te_test_id);
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        ERROR("%s(): cfg_get_instance_fmt(/local:/reqs:%u) failed: %r",
              __FUNCTION__, te_test_id, rc);
        return rc;
    }
    
    if (old == NULL)
        len = 0;
    else
        len = strlen(old) + strlen("()&");
    len += strlen(reqs) + strlen("!()") + 1;

    val = TE_ALLOC(len);
    if (val == NULL)
    {
        free(old);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    if (old == NULL)
        p = snprintf(val, len, "!(%s)", reqs);
    else
        p = snprintf(val, len, "(%s)&!(%s)", old, reqs);

    free(old);

    if (p != len - 1)
    {
        ERROR("%s(): Unexpected number of characters is printed: "
              "%d vs %d", __FUNCTION__, p, len - 1);
        free(val);
        return TE_RC(TE_TAPI, TE_EFAULT);
    }

    if (old == NULL)
        rc = cfg_add_instance_fmt(NULL, type, val,
                                  "/local:/reqs:%u", te_test_id);
    else
        rc = cfg_set_instance_fmt(type, val,
                                  "/local:/reqs:%u", te_test_id);

    if (rc != 0)
    {
        ERROR("%s(): Failed to populate a new target requirements: %r",
              __FUNCTION__, rc);
    }

    free(val);

    return rc;
}
