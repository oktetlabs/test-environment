/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Implementation of Test API for DLNA UPnP Content Directory Resources
 * features.
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

#define TE_LGR_USER     "TAPI UPnP Content Directory Resources"

#include "te_config.h"
#include "tapi_upnp_resources.h"


/**
 * Aggregator of data to post them to the @b cd_handler callbacks for
 * search particular media.
 */
typedef struct {
    tapi_upnp_media_uri        *media;
    tapi_upnp_cd_resource_type  type;
    te_errno                    error;
} search_target_media_t;


/**
 * Search and add new children to the @p container's children list.
 * It is used as @b presearch handler in @sa tapi_upnp_cd_get_tree.
 *
 * @param container     Container context.
 * @param user_data     Unused additional parameter.
 */
static void
search_media(tapi_upnp_cd_container_node *container, void *user_data)
{
    search_target_media_t *stm = (search_target_media_t *)user_data;
    tapi_upnp_resources_uri_node *media;
    tapi_upnp_cd_resource_node   *resource;

    if (stm->error != 0)
        return;

    if (container->data.type == TAPI_UPNP_CD_OBJECT_ITEM)
    {
        SLIST_FOREACH(resource, &container->data.base.resources, next)
        {
            if (resource->res.type == stm->type)
            {
                media = calloc(1, sizeof(*media));
                if (media == NULL)
                {
                    ERROR("%s:%d: cannot allocate memory",
                          __FUNCTION__, __LINE__);
                    stm->error = TE_ENOMEM;
                    return;
                }
                media->uri = strdup(resource->res.uri);
                if (media->uri == NULL)
                {
                    ERROR("%s:%d: cannot allocate memory",
                          __FUNCTION__, __LINE__);
                    stm->error = TE_ENOMEM;
                    free(media);
                    return;
                }
                SLIST_INSERT_HEAD(stm->media, media, next);
            }
        }
    }
}


/* See description in tapi_upnp_resources.h. */
te_errno
tapi_upnp_resources_get_media_uri(tapi_upnp_cd_container_node *container,
                                  tapi_upnp_cd_resource_type   type,
                                  tapi_upnp_media_uri         *media)
{
    search_target_media_t stm = {
        .media = media,
        .type = type,
        .error = 0
    };

    tapi_upnp_cd_tree_dfs(container, NULL, search_media, &stm);

    return stm.error;
}

/* See description in tapi_upnp_resources.h. */
void
tapi_upnp_resources_free_media_uri(tapi_upnp_media_uri *media)

{
    tapi_upnp_resources_uri_node *m, *tmp;

    SLIST_FOREACH_SAFE(m, media, next, tmp)
    {
        SLIST_REMOVE(media, m, tapi_upnp_resources_uri_node, next);
        free((char *)m->uri);
        free(m);
    }
}
