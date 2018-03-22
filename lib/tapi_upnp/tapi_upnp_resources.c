/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Implementation of Test API for DLNA UPnP Content Directory Resources
 * features.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
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

    SLIST_INIT(media);
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

/* See description in tapi_upnp_resources.h. */
void
tapi_upnp_print_resource_info(const tapi_upnp_cd_resource *res)
{
    static const char *res_type_name[] =
    {
        [TAPI_UPNP_CD_RESOURCE_AUDIO] = "audio",
        [TAPI_UPNP_CD_RESOURCE_IMAGE] = "image",
        [TAPI_UPNP_CD_RESOURCE_VIDEO] = "video",
        [TAPI_UPNP_CD_RESOURCE_OTHER] = "unknown/other"
    };

    RING("Content Directory object resource:\n"
         " type: %s (%d)\n"
         " protection: %s\n"
         " uri: %s\n"
         " import_uri: %s\n"
         " protocol_info: %s\n"
         " size: %lld\n"
         " duration: %lld\n"
         " bitrate: %d\n"
         " sample_freq: %d\n"
         " bits_per_sample: %d\n"
         " audio_channels: %d\n"
         " width: %d\n"
         " height: %d\n"
         " color_depth: %d",
         res_type_name[res->type], res->type,
         res->protection,
         res->uri,
         res->import_uri,
         res->protocol_info,
         res->size,
         res->duration,
         res->bitrate,
         res->sample_freq,
         res->bits_per_sample,
         res->audio_channels,
         res->width,
         res->height,
         res->color_depth);
}
