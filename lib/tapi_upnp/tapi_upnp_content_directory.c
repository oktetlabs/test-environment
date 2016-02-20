/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Implementation of Test API for DLNA UPnP Content Directory Service
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

#define TE_LGR_USER     "TAPI UPnP Content Directory"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#include <libgupnp-av/gupnp-av.h>
#include "tapi_upnp_content_directory.h"


/**
 * Aggregator of data to post them to the DIDL-Lite parser callbacks.
 */
typedef struct {
    tapi_upnp_cd_container_node *container;
    te_errno error;
} on_didl_param_t;

/**
 * Aggregator of data to post them to the @b cd_handler callbacks for
 * search new children.
 */
typedef struct {
    rcf_rpc_server         *rpcs;
    tapi_upnp_service_info *service;
    te_errno                error;
} search_target_service_t;

/**
 * Specifies a browse option BrowseFlag to be used for browsing the
 * ContentDirectory service.
 */
typedef enum {
    BROWSE_FLAG_METADATA,
    BROWSE_FLAG_DIRECT_CHILDREN
} browse_flag_t;

/**
 * String representation of BrowseFlag
 */
static const char *browse_flag_string[] = {
    [BROWSE_FLAG_METADATA] = "BrowseMetadata",
    [BROWSE_FLAG_DIRECT_CHILDREN] = "BrowseDirectChildren"
};

/**
 * Initialise the GLib type system.
 * Warning: it is require to call this function before calling
 * @sa tapi_upnp_cd_get_root and @sa tapi_upnp_cd_get_children functions.
 */
static void
init_glib_type(void)
{
    static te_bool single = FALSE;

    if (single)
        return;

    /* Required initialisation. */
#if !GLIB_CHECK_VERSION(2,35,0)
    /*
     * Initialise the type system only for GLib version < 2.35.
     * Since GLib 2.35, the type system is initialised automatically and
     * g_type_init() is deprecated.
     */
    g_type_init();
#endif /* !GLIB_CHECK_VERSION(2,35,0) */
    single = TRUE;
}

#if !GLIB_CHECK_VERSION(2,28,0)
/* Only for GLib version < 2.28.*/
/**
 * Remove the all elements from the @p list and unref them.
 *
 * @param list      List to free.
 *
 * @sa g_object_unref, g_list_free
 */
static void
glist_free_with_unref(GList *list)
{
    GList *l;

    for (l = list; l != NULL; l = l->next)
        g_object_unref(l->data);
    g_list_free(list);
}
#else /* !GLIB_CHECK_VERSION(2,28,0) */
/**
 * Remove the all elements from the @p list and unref them.
 *
 * @param list      List to free.
 *
 * @sa g_object_unref, g_list_free_full
 */
static inline void
glist_free_with_unref(GList *list)
{
    g_list_free_full(list, g_object_unref);
}
#endif /* !GLIB_CHECK_VERSION(2,28,0) */

/**
 * Invoke browse action on ContentDirectory service.
 *
 * @param[in]  rpcs             RPC server handle.
 * @param[in]  service          ContentDirectory service context.
 * @param[in]  object_id        ID of object, for root object must be "0".
 * @param[in]  browse_flag      BrowseFlag argument for Browse action.
 * @param[out] objects_number   Number of objects returned in the @p result
 *                              argument.
 * @param[out] result           DIDL-Lite XML Document contains the returned
 *                              objects. Do not free it after use!
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
browse_cd(rcf_rpc_server                *rpcs,
          const tapi_upnp_service_info  *service,
          const char                    *object_id,
          browse_flag_t                  browse_flag,
          size_t                        *objects_number,
          char                         **result)
{
    const char         *service_id;
    tapi_upnp_action   *action = NULL;
    tapi_upnp_argument *argument = NULL;
    tapi_upnp_argument *argument_number = NULL;
    tapi_upnp_argument *argument_result = NULL;
    te_errno            rc = 0;

    service_id = strstr(tapi_upnp_get_service_id(service),
                        "ContentDirectory");
    if (service_id == NULL)
    {
        ERROR("Service \"%s\" is incompatible with ContentDirectory",
              tapi_upnp_get_service_id(service));
        return TE_ECANCELED;
    }
    SLIST_FOREACH(action, &service->actions, next)
    {
        if (strcmp(action->name, "Browse") == 0)
            break;
    }
    if (action == NULL)
    {
        ERROR("Service \"%s\" does not provide \"Browse\" action",
              tapi_upnp_get_service_id(service));
        return TE_ECANCELED;
    }

    /* Prepare input arguments and save output ones. */
    SLIST_FOREACH(argument, &action->arguments, next)
    {
        const char *name;

        name = tapi_upnp_get_argument_name(argument);
        if (strcmp(name, "ObjectID") == 0)
        {
            tapi_upnp_set_argument_value(argument, object_id);
        }
        else if (strcmp(name, "BrowseFlag") == 0)
        {
            if (browse_flag < TE_ARRAY_LEN(browse_flag_string))
            {
                tapi_upnp_set_argument_value(argument,
                                        browse_flag_string[browse_flag]);
            }
            else
            {
                ERROR("BrowseFlag %u is not supported", browse_flag);
                return TE_EINVAL;
            }
        }
        else if (strcmp(name, "Filter") == 0)
        {
            /*
             * '*' indicates request all supported properties, both
             * required and allowed, from all namespaces.
             */
            tapi_upnp_set_argument_value(argument, "*");
        }
        else if (strcmp(name, "StartingIndex") == 0)
        {
            tapi_upnp_set_argument_value(argument, "0");
        }
        else if (strcmp(name, "RequestedCount") == 0)
        {
            /* 0 (zero) indicates request all entries. */
            tapi_upnp_set_argument_value(argument, "0");
        }
        else if (strcmp(name, "SortCriteria") == 0)
        {
            tapi_upnp_set_argument_value(argument, "");
        }
        /* Output arguments. */
        else if (strcmp(name, "NumberReturned") == 0)
        {
            argument_number = argument;
        }
        else if (strcmp(name, "Result") == 0)
        {
            argument_result = argument;
        }
    }
    /* Invoke action. */
    rc = tapi_upnp_invoke_action(rpcs, service, action);
    if (rc == 0)
    {
        /* Extract action result to return. */
        size_t value;
        char  *end;

        errno = 0;
        value = strtol(tapi_upnp_get_argument_value(argument_number),
                       &end, 10);
        if (errno == ERANGE)
        {
            ERROR("Too much objects was returned");
            errno = 0;
            return TE_EOVERFLOW;
        }
        if (errno == EINVAL)
        {
            ERROR("Invalid value of objects number: %s", strerror(errno));
            errno = 0;
            return TE_EINVAL;
        }
        *objects_number = value;
        *result = (char *)tapi_upnp_get_argument_value(argument_result);
    }
    return rc;
}

/**
 * Free base part of ContentDirectory container.
 *
 * @param base      Common part for item and container.
 */
static void
free_cd_base(tapi_upnp_cd_object *base)
{
    tapi_upnp_cd_resource_node *res, *tmp_res;
    tapi_upnp_cd_contributor_node *con, *tmp_con;

    free((char *)base->id);
    free((char *)base->parent_id);
    free((char *)base->title);
    free((char *)base->class);
    free((char *)base->creator);
    free((char *)base->write_status);
    free((char *)base->album);
    free((char *)base->album_art);
    free((char *)base->genre);
    free((char *)base->description);
    free((char *)base->date);
    SLIST_FOREACH_SAFE(res, &base->resources, next, tmp_res)
    {
        SLIST_REMOVE(&base->resources, res, tapi_upnp_cd_resource_node,
                     next);
        free((char *)res->res.protection);
        free((char *)res->res.uri);
        free((char *)res->res.import_uri);
        free((char *)res->res.protocol_info);
        free(res);
    }
    SLIST_FOREACH_SAFE(con, &base->artists, next, tmp_con)
    {
        SLIST_REMOVE(&base->artists, con, tapi_upnp_cd_contributor_node,
                     next);
        free((char *)con->contributor.name);
        free((char *)con->contributor.role);
        free(con);
    }
    SLIST_FOREACH_SAFE(con, &base->authors, next, tmp_con)
    {
        SLIST_REMOVE(&base->authors, con, tapi_upnp_cd_contributor_node,
                     next);
        free((char *)con->contributor.name);
        free((char *)con->contributor.role);
        free(con);
    }
    memset(base, 0, sizeof(*base));
}

/**
 * Free ContentDirectory container.
 *
 * @param container     Container context.
 */
static void
free_cd_container(tapi_upnp_cd_container *container)
{
    free_cd_base(&container->base);
    if (container->type == TAPI_UPNP_CD_OBJECT_CONTAINER)
    {
        memset(&container->container, 0, sizeof(container->container));
    }
    else /* if (container->type == TAPI_UPNP_CD_OBJECT_ITEM) */
    {
        free((char *)container->item.ref_id);
        memset(&container->item, 0, sizeof(container->item));
    }
}

/**
 * Remove container contents. Warning: it removes only itself data and near
 * children, i.e. it children must not have children. It does not update
 * neither parent reference nor it sibs.
 *
 * @param container     Container context.
 * @param user_data     Unused additional parameter.
 */
static void
remove_container(tapi_upnp_cd_container_node *container, void *user_data)
{
    UNUSED(user_data);
    tapi_upnp_cd_container_node *child, *tmp_child;

    free_cd_container(&container->data);
    SLIST_FOREACH_SAFE(child, &container->children, next, tmp_child)
    {
        SLIST_REMOVE(&container->children, child,
                     tapi_upnp_cd_container_node, next);
        free(child);
    }
}

/**
 * Search and add new children to the @p container's children list.
 * It is used as @b presearch handler in @sa tapi_upnp_cd_get_tree.
 *
 * @param container     Container context.
 * @param user_data     Unused additional parameter.
 */
static void
search_new_children(tapi_upnp_cd_container_node *container, void *user_data)
{
    search_target_service_t *sts = (search_target_service_t *)user_data;

    if (sts->error != 0)
        return;

    if (container->data.type == TAPI_UPNP_CD_OBJECT_CONTAINER)
        sts->error = tapi_upnp_cd_get_children(sts->rpcs, sts->service,
                                               container);
}

/**
 * Extract a metadata and save it in the base class part of the container
 * context.
 *
 * @param upnp_object   GUPnPDIDLLiteObject extracted from the DIDL-Lite XML
 *                      Document.
 * @param base          Common part for item and container.
 *
 * @return Status code. On success, @c 0.
 *
 * @todo Find and determine a particular resource type.
 */
static te_errno
extract_base_class_data(GUPnPDIDLLiteObject *upnp_object,
                        tapi_upnp_cd_object *base)
{
    typedef struct {
        const char **value;
        const char *(*get_value)(GUPnPDIDLLiteObject *);
    } upnp_cd_base_string_property_t;
    typedef struct {
        const char **value;
        const char *(*get_value)(GUPnPDIDLLiteResource *);
    } upnp_cd_resource_string_property_t;
    typedef struct {
        const char **value;
        const char *(*get_value)(GUPnPDIDLLiteContributor *);
    } upnp_cd_contributor_string_property_t;
    upnp_cd_base_string_property_t base_string[] = {
        { &base->id, gupnp_didl_lite_object_get_id },
        { &base->parent_id, gupnp_didl_lite_object_get_parent_id },
        { &base->title, gupnp_didl_lite_object_get_title },
        { &base->class, gupnp_didl_lite_object_get_upnp_class },
        { &base->creator, gupnp_didl_lite_object_get_creator },
        { &base->write_status, gupnp_didl_lite_object_get_write_status },
        { &base->album, gupnp_didl_lite_object_get_album },
        { &base->album_art, gupnp_didl_lite_object_get_album_art },
        { &base->genre, gupnp_didl_lite_object_get_genre },
        { &base->description, gupnp_didl_lite_object_get_description },
        { &base->date, gupnp_didl_lite_object_get_date }
    };
    upnp_cd_resource_string_property_t resource_string[] = {
        { NULL, gupnp_didl_lite_resource_get_protection },
        { NULL, gupnp_didl_lite_resource_get_uri },
        { NULL, gupnp_didl_lite_resource_get_import_uri }
    };
    upnp_cd_contributor_string_property_t contributor_string[] = {
        { NULL, gupnp_didl_lite_contributor_get_name },
        { NULL, gupnp_didl_lite_contributor_get_role }
    };
    GList      *resources = NULL;
    GList      *artists = NULL;
    GList      *authors = NULL;
    GList      *l;
    size_t      i;
    te_errno    rc = 0;
    const char *tmp_str;

    for (i = 0; i < TE_ARRAY_LEN(base_string); i++)
    {
        tmp_str = base_string[i].get_value(upnp_object);
        if (tmp_str != NULL)
        {
            *base_string[i].value = strdup(tmp_str);
            if (*base_string[i].value == NULL)
            {
                ERROR("%s:%d: strdup is failed", __FUNCTION__, __LINE__);
                rc = TE_ENOMEM;
                goto extract_base_class_data_cleanup;
            }
        }
    }
    base->restricted =
        (te_bool)gupnp_didl_lite_object_get_restricted(upnp_object);

#ifdef LIBGUPNP_VER_0_12
    /* Now it is available only in unstable version libgupnp. */
    base->object_update_id =
        gupnp_didl_lite_object_get_update_id(upnp_object);
#endif /* LIBGUPNP_VER_0_12 */

    base->track_number =
        gupnp_didl_lite_object_get_track_number(upnp_object);

    /* List of resources. */
    SLIST_INIT(&base->resources);
    resources = gupnp_didl_lite_object_get_resources(upnp_object);
    for (l = resources; l; l = l->next)
    {
        GUPnPDIDLLiteResource      *resource;
        tapi_upnp_cd_resource_node *res_node;

        res_node = calloc(1, sizeof(*res_node));
        if (res_node == NULL)
        {
            ERROR("%s:%d: cannot allocate memory", __FUNCTION__, __LINE__);
            rc = TE_ENOMEM;
            goto extract_base_class_data_cleanup;
        }
        resource_string[0].value = &res_node->res.protection;
        resource_string[1].value = &res_node->res.uri;
        resource_string[2].value = &res_node->res.import_uri;

        resource = (GUPnPDIDLLiteResource *)l->data;
        res_node->res.type = TAPI_UPNP_CD_RESOURCE_OTHER;   /**< @todo
                                * Determine a particular resource type. */
#ifdef LIBGUPNP_VER_0_12
        /* Now it is available only in unstable version libgupnp. */
        res_node->res.update_count =
            gupnp_didl_lite_resource_get_update_count(resource);
#endif /* LIBGUPNP_VER_0_12 */

        res_node->res.size = gupnp_didl_lite_resource_get_size(resource);
        res_node->res.size64 =
            gupnp_didl_lite_resource_get_size64(resource);
        res_node->res.duration =
            gupnp_didl_lite_resource_get_duration(resource);
        res_node->res.bitrate =
            gupnp_didl_lite_resource_get_bitrate(resource);
        res_node->res.sample_freq =
            gupnp_didl_lite_resource_get_sample_freq(resource);
        res_node->res.bits_per_sample =
            gupnp_didl_lite_resource_get_bits_per_sample(resource);
        res_node->res.audio_channels =
            gupnp_didl_lite_resource_get_audio_channels(resource);
        res_node->res.width = gupnp_didl_lite_resource_get_width(resource);
        res_node->res.height =
            gupnp_didl_lite_resource_get_height(resource);
        res_node->res.color_depth =
            gupnp_didl_lite_resource_get_color_depth(resource);
        for (i = 0; i < TE_ARRAY_LEN(resource_string); i++)
        {
            tmp_str = resource_string[i].get_value(resource);
            if (tmp_str != NULL)
            {
                *resource_string[i].value = strdup(tmp_str);
                if (*resource_string[i].value == NULL)
                {
                    ERROR("%s:%d: strdup is failed",
                          __FUNCTION__, __LINE__);
                    rc = TE_ENOMEM;
                    goto extract_base_class_data_cleanup;
                }
            }
        }
        /**
         * @todo perhaps gupnp_protocol_info_to_string will be better than
         * gupnp_protocol_info_get_protocol
         */
        tmp_str = gupnp_protocol_info_get_protocol(
                    gupnp_didl_lite_resource_get_protocol_info(resource));
        if (tmp_str != NULL)
        {
            res_node->res.protocol_info = strdup(tmp_str);
            if (res_node->res.protocol_info == NULL)
            {
                ERROR("%s:%d: strdup is failed", __FUNCTION__, __LINE__);
                rc = TE_ENOMEM;
                goto extract_base_class_data_cleanup;
            }
        }
        SLIST_INSERT_HEAD(&base->resources, res_node, next);
    }

    /* List of artists. */
    SLIST_INIT(&base->artists);
    artists = gupnp_didl_lite_object_get_artists(upnp_object);
    for (l = artists; l; l = l->next)
    {
        GUPnPDIDLLiteContributor      *artist;
        tapi_upnp_cd_contributor_node *artist_node;

        artist_node = calloc(1, sizeof(*artist_node));
        if (artist_node == NULL)
        {
            ERROR("%s:%d: cannot allocate memory", __FUNCTION__, __LINE__);
            rc = TE_ENOMEM;
            goto extract_base_class_data_cleanup;
        }
        contributor_string[0].value = &artist_node->contributor.name;
        contributor_string[1].value = &artist_node->contributor.role;

        artist = (GUPnPDIDLLiteContributor *)l->data;
        for (i = 0; i < TE_ARRAY_LEN(contributor_string); i++)
        {
            tmp_str = contributor_string[i].get_value(artist);
            if (tmp_str != NULL)
            {
                *contributor_string[i].value = strdup(tmp_str);
                if (*contributor_string[i].value == NULL)
                {
                    ERROR("%s:%d: strdup is failed",
                          __FUNCTION__, __LINE__);
                    rc = TE_ENOMEM;
                    goto extract_base_class_data_cleanup;
                }
            }
        }
        SLIST_INSERT_HEAD(&base->artists, artist_node, next);
    }

    /* List of authors. */
    SLIST_INIT(&base->authors);
    authors = gupnp_didl_lite_object_get_authors(upnp_object);
    for (l = authors; l; l = l->next)
    {
        GUPnPDIDLLiteContributor *author;
        tapi_upnp_cd_contributor_node *author_node;

        author_node = calloc(1, sizeof(*author_node));
        if (author_node == NULL)
        {
            ERROR("%s:%d: cannot allocate memory", __FUNCTION__, __LINE__);
            rc = TE_ENOMEM;
            goto extract_base_class_data_cleanup;
        }
        contributor_string[0].value = &author_node->contributor.name;
        contributor_string[1].value = &author_node->contributor.role;

        author = (GUPnPDIDLLiteContributor *)l->data;
        for (i = 0; i < TE_ARRAY_LEN(contributor_string); i++)
        {
            tmp_str = contributor_string[i].get_value(author);
            if (tmp_str != NULL)
            {
                *contributor_string[i].value = strdup(tmp_str);
                if (*contributor_string[i].value == NULL)
                {
                    ERROR("%s:%d: strdup is failed",
                          __FUNCTION__, __LINE__);
                    rc = TE_ENOMEM;
                    goto extract_base_class_data_cleanup;
                }
            }
        }
        SLIST_INSERT_HEAD(&base->authors, author_node, next);
    }

extract_base_class_data_cleanup:
    glist_free_with_unref(resources);
    glist_free_with_unref(artists);
    glist_free_with_unref(authors);
    if (rc != 0)
        free_cd_base(base);
    return rc;
}

/**
 * Extract an item metadata and save it in the the container context.
 *
 * @param upnp_item     GUPnPDIDLLiteItem extracted from the DIDL-Lite XML
 *                      Document.
 * @param container     Container context to save data in.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
extract_item_data(GUPnPDIDLLiteItem           *upnp_item,
                  tapi_upnp_cd_container_node *container)
{
    te_errno    rc = 0;
    const char *tmp_str;

    rc = extract_base_class_data((GUPnPDIDLLiteObject *)upnp_item,
                                 &container->data.base);
    if (rc != 0)
        return rc;

    container->data.type = TAPI_UPNP_CD_OBJECT_ITEM;

    tmp_str = gupnp_didl_lite_item_get_ref_id(upnp_item);
    if (tmp_str != NULL)
    {
        container->data.item.ref_id = strdup(tmp_str);
        if (container->data.item.ref_id == NULL)
        {
            ERROR("%s:%d: strdup is failed", __FUNCTION__, __LINE__);
            return TE_ENOMEM;
        }
    }
#ifdef LIBGUPNP_VER_0_12
    container->data.item.lifetime =
        gupnp_didl_lite_item_get_lifetime(upnp_item);
#endif /* LIBGUPNP_VER_0_12 */

    return rc;
}

/**
 * Extract a container metadata and save it in the the container context.
 *
 * @param upnp_container    GUPnPDIDLLiteContainer extracted from the
 *                          DIDL-Lite XML Document.
 * @param container         Container context to save data in.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
extract_container_data(GUPnPDIDLLiteContainer *upnp_container,
                       tapi_upnp_cd_container_node *container)
{
    te_errno rc = 0;

    rc = extract_base_class_data((GUPnPDIDLLiteObject *)upnp_container,
                                 &container->data.base);
    if (rc != 0)
        return rc;

    container->data.type = TAPI_UPNP_CD_OBJECT_CONTAINER;

#ifdef LIBGUPNP_VER_0_12
    container->data.container.container_update_id =
        gupnp_didl_lite_container_get_container_update_id(upnp_container);
    container->data.container.total_deleted_child_count =
        gupnp_didl_lite_container_get_total_deleted_child_count(
                                                            upnp_container);
#endif /* LIBGUPNP_VER_0_12 */

    container->data.container.child_count =
        gupnp_didl_lite_container_get_child_count(upnp_container);
    container->data.container.searchable =
        (te_bool)gupnp_didl_lite_container_get_searchable(upnp_container);
    container->data.container.storage_used =
        gupnp_didl_lite_container_get_storage_used(upnp_container);

    return rc;
}

/**
 * Extract metadata and save it in the container context.
 * It is called each time an "item-available" signal is emitted, i.e. an
 * item is found in the DIDL-Lite XML being parsed.
 *
 * @param parser        The GUPnPDIDLLiteParser that received the signal.
 * @param item          The now available GUPnPDIDLLiteItem.
 * @param user_data     The on_didl_param_t data.
 */
static void
on_didl_item_available(GUPnPDIDLLiteParser *parser,
                       GUPnPDIDLLiteItem   *item,
                       gpointer             user_data)
{
    UNUSED(parser);
    on_didl_param_t *on_didl_param = (on_didl_param_t *)user_data;

#if UPNP_DEBUG > 2
    VERB("* item available");
#endif /* UPNP_DEBUG */

    on_didl_param->error =
        extract_item_data(item, on_didl_param->container);
}

/**
 * Extract metadata and save it in the container context.
 * It is called each time an "container-available" signal is emitted, i.e. a
 * container is found in the DIDL-Lite XML being parsed.
 *
 * @param parser        The GUPnPDIDLLiteParser that received the signal.
 * @param container     The now available GUPnPDIDLLiteContainer.
 * @param user_data     The on_didl_param_t data.
 */
static void
on_didl_container_available(GUPnPDIDLLiteParser    *parser,
                            GUPnPDIDLLiteContainer *container,
                            gpointer                user_data)
{
    UNUSED(parser);
    on_didl_param_t *on_didl_param = (on_didl_param_t *)user_data;

#if UPNP_DEBUG > 2
    VERB("* container available");
#endif /* UPNP_DEBUG */

    on_didl_param->error =
        extract_container_data(container, on_didl_param->container);
}

/**
 * Extract metadata and save it in the child container context.
 * It is called each time an "item-available" signal is emitted, i.e. an
 * item is found in the DIDL-Lite XML being parsed.
 *
 * @param parser        The GUPnPDIDLLiteParser that received the signal.
 * @param item          The now available GUPnPDIDLLiteItem.
 * @param user_data     The on_didl_param_t data with parent container
 *                      context contains children list to save data in.
 */
static void
on_didl_child_item_available(GUPnPDIDLLiteParser *parser,
                             GUPnPDIDLLiteItem   *item,
                             gpointer             user_data)
{
    UNUSED(parser);
    on_didl_param_t *on_didl_param = (on_didl_param_t *)user_data;
    tapi_upnp_cd_container_node *con;

#if UPNP_DEBUG > 2
    VERB("* child item available");
#endif /* UPNP_DEBUG */

    if (on_didl_param->error != 0)  /* Previous parsing was failed. */
        return;

    con = calloc(1, sizeof(*con));
    if (con == NULL)
    {
        ERROR("%s:%d: cannot allocate memory", __FUNCTION__, __LINE__);
        on_didl_param->error = TE_ENOMEM;
        return;
    }
    on_didl_param->error = extract_item_data(item, con);
    if (on_didl_param->error == 0)
    {
        SLIST_INSERT_HEAD(&on_didl_param->container->children, con, next);
        con->parent = on_didl_param->container;
    }
    else
        free(con);
}

/**
 * Extract metadata and save it in the child container context.
 * It is called each time an "container-available" signal is emitted, i.e. a
 * container is found in the DIDL-Lite XML being parsed.
 *
 * @param parser        The GUPnPDIDLLiteParser that received the signal.
 * @param container     The now available GUPnPDIDLLiteContainer.
 * @param user_data     The on_didl_param_t data with parent container
 *                      context contains children list to save data in.
 */
static void
on_didl_child_container_available(GUPnPDIDLLiteParser    *parser,
                                  GUPnPDIDLLiteContainer *container,
                                  gpointer                user_data)
{
    UNUSED(parser);
    on_didl_param_t *on_didl_param = (on_didl_param_t *)user_data;
    tapi_upnp_cd_container_node *con;

#if UPNP_DEBUG > 2
    VERB("* child container available");
#endif /* UPNP_DEBUG */

    if (on_didl_param->error != 0)  /* Previous parsing was failed. */
        return;

    con = calloc(1, sizeof(*con));
    if (con == NULL)
    {
        ERROR("%s:%d: cannot allocate memory", __FUNCTION__, __LINE__);
        on_didl_param->error = TE_ENOMEM;
        return;
    }
    on_didl_param->error = extract_container_data(container, con);
    if (on_didl_param->error == 0)
    {
        SLIST_INSERT_HEAD(&on_didl_param->container->children, con, next);
        con->parent = on_didl_param->container;
    }
    else
        free(con);
}

/**
 * Extract a metadata of ContentDirectory object from the DIDL-Lite XML
 * Document.
 *
 * @param[in]  data         DIDL-Lite XML Document contains the object
 *                          metadata.
 * @param[out] container    Container context to save metadata to.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_metadata(const char *data, tapi_upnp_cd_container_node *container)
{
    GUPnPDIDLLiteParser *parser;
    GError              *error = NULL;
    te_errno             rc = 0;
    on_didl_param_t      on_didl_param = {
        .container = container,
        .error = 0
    };

    parser = gupnp_didl_lite_parser_new();
    g_signal_connect(parser, "item-available",
                     G_CALLBACK (on_didl_item_available),
                     (gpointer)&on_didl_param);

    g_signal_connect(parser, "container-available",
                     G_CALLBACK (on_didl_container_available),
                     (gpointer)&on_didl_param);

    if (!gupnp_didl_lite_parser_parse_didl(parser, data, &error))
    {
        if (error)
        {
            ERROR("Fail to parse DIDL-Lite XML Document: %s",
                  error->message);
            g_error_free(error);
        }
        rc = TE_EFAIL;
    }
    if (on_didl_param.error != 0)
    {
        ERROR("Fail to parse DIDL-Lite XML Document");
        rc = on_didl_param.error;
    }
    g_object_unref(parser);
    return rc;
}

/**
 * Extract a children with its metadata from the DIDL-Lite XML Document.
 *
 * @param[in]  data         DIDL-Lite XML Document contains the children
 *                          object with its metadata.
 * @param[out] container    Parent container context to save children to.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_children(const char *data, tapi_upnp_cd_container_node *container)
{
    GUPnPDIDLLiteParser *parser;
    GError              *error = NULL;
    te_errno             rc = 0;
    on_didl_param_t      on_didl_param = {
        .container = container,
        .error = 0
    };

    parser = gupnp_didl_lite_parser_new();
    g_signal_connect(parser, "item-available",
                     G_CALLBACK (on_didl_child_item_available),
                     (gpointer)&on_didl_param);

    g_signal_connect(parser, "container-available",
                     G_CALLBACK (on_didl_child_container_available),
                     (gpointer)&on_didl_param);

    if (!gupnp_didl_lite_parser_parse_didl(parser, data, &error))
    {
        if (error)
        {
            ERROR("Fail to parse DIDL-Lite XML Document: %s",
                  error->message);
            g_error_free(error);
        }
        rc = TE_EFAIL;
    }
    if (on_didl_param.error != 0)
    {
        ERROR("Fail to parse DIDL-Lite XML Document");
        rc = on_didl_param.error;
    }
    g_object_unref(parser);
    return rc;
}

/* See description in tapi_upnp_content_directory.h. */
te_errno
tapi_upnp_cd_get_root(rcf_rpc_server               *rpcs,
                      const tapi_upnp_service_info *service,
                      tapi_upnp_cd_container_node  *container)
{
    size_t   objects_number;
    char    *data;
    te_errno rc = 0;

    if (container == NULL)
    {
        ERROR("Container must not be NULL");
        return TE_EINVAL;
    }

    init_glib_type();

    rc = browse_cd(rpcs, service, "0", BROWSE_FLAG_METADATA,
                   &objects_number, &data);
    if (rc == 0)
    {
        if (objects_number == 1)
            rc = parse_metadata(data, container);
        else
            rc = TE_ENODATA;
    }
    return rc;
}

/* See description in tapi_upnp_content_directory.h. */
te_errno
tapi_upnp_cd_get_children(rcf_rpc_server               *rpcs,
                          const tapi_upnp_service_info *service,
                          tapi_upnp_cd_container_node  *container)
{
    size_t   objects_number;
    char    *data;
    te_errno rc = 0;

    if (container == NULL)
    {
        ERROR("Container must not be NULL");
        return TE_EINVAL;
    }

    init_glib_type();

    rc = browse_cd(rpcs, service, container->data.base.id,
                   BROWSE_FLAG_DIRECT_CHILDREN, &objects_number, &data);
    if (rc == 0)
    {
        if (objects_number > 0)
            rc = parse_children(data, container);
        else
            rc = TE_ENODATA;
    }
    return rc;
}

/* See description in tapi_upnp_content_directory.h. */
te_errno
tapi_upnp_cd_get_tree(rcf_rpc_server               *rpcs,
                      const tapi_upnp_service_info *service,
                      tapi_upnp_cd_container_node  *container)
{
    te_errno rc;
    search_target_service_t sts = {
        .rpcs = rpcs,
        .service = service,
        .error = 0
    };

    rc = tapi_upnp_cd_get_root(rpcs, service, container);
    if (rc != 0)
        return rc;

    tapi_upnp_cd_tree_dfs(container, search_new_children, NULL, &sts);

    return sts.error;
}

/* See description in tapi_upnp_content_directory.h. */
void
tapi_upnp_cd_tree_dfs(tapi_upnp_cd_container_node *container,
                      cd_handler presearch, cd_handler postsearch,
                      void *user_data)
{
    tapi_upnp_cd_container_node *child;

    if (container == NULL)
        return;

    if (presearch != NULL)
        presearch(container, user_data);

    SLIST_FOREACH(child, &container->children, next)
        tapi_upnp_cd_tree_dfs(child, presearch, postsearch, user_data);

    if (postsearch != NULL)
        postsearch(container, user_data);
}

/* See description in tapi_upnp_content_directory.h. */
void
tapi_upnp_cd_remove_container(tapi_upnp_cd_container_node *container)
{
    tapi_upnp_cd_container_node *parent;

    tapi_upnp_cd_tree_dfs(container, NULL, remove_container, NULL);

    parent = container->parent;
    if (parent != NULL)
    {
        SLIST_REMOVE(&parent->children, container,
                     tapi_upnp_cd_container_node, next);
        container->parent = NULL;
    }
}

/* See description in tapi_upnp_content_directory.h. */
void
tapi_upnp_cd_remove_tree(tapi_upnp_cd_container_node *root)
{
    while (root->parent != NULL)
        root = root->parent;

    tapi_upnp_cd_remove_container(root);
}
