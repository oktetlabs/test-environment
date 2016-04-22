/** @file
 * @brief Test API - RPC
 *
 * Internal definitions
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "tapi_rpc_internal.h"

/* See description in tapi_rpc_internal.h */
te_errno
tapi_rpc_namespace_check(rcf_rpc_server *rpcs, rpc_ptr ptr, const char* ns,
                         const char *function, int line)
{
    char                       *remote_ns;
    te_errno                    rc;
    const rpc_ptr_id_namespace  id = RPC_PTR_ID_GET_NS(ptr);

    if (ptr == 0)
        return 0;

    if (id < rpcs->namespaces_len && rpcs->namespaces[id] != NULL)
    {
        if (strcmp(rpcs->namespaces[id], ns) == 0)
            return 0;
        else
        {
            WARN("%s:%d: Incorrect namespace, "
                 "possible namespace cache is invalid ('%s' != '%s')",
                  function, line, rpcs->namespaces[id], ns);
            rcf_rpc_namespace_free_cache(rpcs);
        }
    }

    rc = rcf_rpc_namespace_id2str(rpcs, id, &remote_ns);
    if (rc)
    {
        rpcs->_errno = rc;
        return rpcs->_errno;
    }

    if (strcmp(remote_ns, ns) != 0)
    {
        ERROR("%s:%d: Incorrect namespace ('%s' != '%s')",
              function, line, remote_ns, ns);
        free(remote_ns);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        return rpcs->_errno;
    }

    if (id >= rpcs->namespaces_len)
    {
        const size_t n = id + 1;
        char **tmp = (char**)realloc(rpcs->namespaces, n * sizeof(char*));

        if (tmp == NULL)
        {
            ERROR("%s:%d: Out of memory!", function, line);
            free(remote_ns);
            rpcs->_errno = TE_RC(TE_TAPI, TE_ENOMEM);
            return rpcs->_errno;
        }
        memset(tmp + rpcs->namespaces_len, 0,
               (n - rpcs->namespaces_len) * sizeof(char*));

        rpcs->namespaces = tmp;
        rpcs->namespaces_len = n;
    }

    rpcs->namespaces[id] = remote_ns;
    return 0;
}

/* See description in tapi_rpc_internal.h */
const char*
tapi_rpc_namespace_get(rcf_rpc_server *rpcs, rpc_ptr ptr)
{
    const rpc_ptr_id_namespace id = RPC_PTR_ID_GET_NS(ptr);
    if (ptr && id < rpcs->namespaces_len && rpcs->namespaces[id] != NULL)
        return rpcs->namespaces[id];
    return NULL;
}
