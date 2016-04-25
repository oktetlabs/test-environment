/** @file
 * @brief Generic test API to storage routines
 *
 * Generic high level functions to work with storage.
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

#define TE_LGR_USER     "TAPI Storage (generic)"

#include "tapi_storage.h"

#include "logger_api.h"
#include "conf_api.h"


/* Format string for lazy flag in Configuration tree. */
#define TE_CFG_STORAGE_UPLOAD_LAZY_FMT  "/local:/env:STORAGE_UPLOAD_LAZY"


/* See description in tapi_storage.h. */
te_errno
tapi_storage_bootstrap(tapi_storage_client *client)
{
    te_errno con_rc;
    te_errno rc = 0;

    con_rc = tapi_storage_client_connect(client);
    if (con_rc != 0)
        return con_rc;
    rc = tapi_storage_client_rm(client, "/", TRUE);
    con_rc = tapi_storage_client_disconnect(client);
    return (rc != 0 ? rc : con_rc);
}

/* See description in tapi_storage.h. */
te_errno
tapi_storage_setup(tapi_storage_client *client)
{
    te_errno        con_rc;
    te_errno        rc = 0;
    cfg_val_type    type = CVT_INTEGER;
    int             lazy;
    tapi_local_file root_dir = {
        .type = TAPI_FILE_TYPE_DIRECTORY,
        .pathname = "/",
        .property = { 0 }
    };

    /* Read STORAGE_UPLOAD_LAZY from the Configurator. */
    rc = cfg_get_instance_fmt(&type, &lazy, TE_CFG_STORAGE_UPLOAD_LAZY_FMT);
    if (rc != 0)
    {
        ERROR("Failed to get value of " TE_CFG_STORAGE_UPLOAD_LAZY_FMT);
        return rc;
    }
    if (lazy != 0 && lazy != 1)
    {
        ERROR("Invalid value of " TE_CFG_STORAGE_UPLOAD_LAZY_FMT ". "
              "It is expected 0 or 1, but value is %d", lazy);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Copy content to remote storage. */
    con_rc = tapi_storage_client_connect(client);
    if (con_rc != 0)
        return con_rc;
    rc = tapi_storage_client_mput(client, &root_dir, NULL, TRUE, !lazy);
    con_rc = tapi_storage_client_disconnect(client);
    return (rc != 0 ? rc : con_rc);
}
