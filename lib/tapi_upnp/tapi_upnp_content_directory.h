/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * @defgroup tapi_upnp_cd_service Test API to operate the DLNA UPnP Content Directory Service
 * @ingroup tapi_upnp
 * @{
 *
 * Definition of Test API for DLNA UPnP Content Directory Service features.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_UPNP_CONTENT_DIRECTORY_H__
#define __TAPI_UPNP_CONTENT_DIRECTORY_H__

#include "te_queue.h"
#include "rcf_rpc.h"
#include "tapi_upnp_device_info.h"
#include "tapi_upnp_service_info.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Multimedia content types which can be returned as a resource. */
typedef enum tapi_upnp_cd_resource_type {
    TAPI_UPNP_CD_RESOURCE_AUDIO = 0,
    TAPI_UPNP_CD_RESOURCE_IMAGE,
    TAPI_UPNP_CD_RESOURCE_VIDEO,
    TAPI_UPNP_CD_RESOURCE_OTHER
} tapi_upnp_cd_resource_type;

/**
 * Object type that is either container (i.e contains other objects) or
 * item.
 */
typedef enum tapi_upnp_cd_object_type {
    TAPI_UPNP_CD_OBJECT_CONTAINER = 0,
    TAPI_UPNP_CD_OBJECT_ITEM
} tapi_upnp_cd_object_type;

/** Parameters of a contributors resource in ContentDirectory service. */
typedef struct tapi_upnp_cd_contributor {
    const char *name;
    const char *role;
} tapi_upnp_cd_contributor;

typedef struct tapi_upnp_cd_contributor_node {
    tapi_upnp_cd_contributor contributor;
    SLIST_ENTRY(tapi_upnp_cd_contributor_node) next;
} tapi_upnp_cd_contributor_node;

/** Parameters of a resource in ContentDirectory service. */
typedef struct tapi_upnp_cd_resource {
    tapi_upnp_cd_resource_type type;
#ifdef LIBGUPNP_VER_0_12
    /* Now it is available only in unstable version libgupnp. */
    uint32_t    update_count;
#endif /* LIBGUPNP_VER_0_12 */
    const char *protection;
    const char *uri;
    const char *import_uri;
    const char *protocol_info;
    int64_t     size;
    int64_t     duration;
    int         bitrate;
    int         sample_freq;
    int         bits_per_sample;
    int         audio_channels;
    int         width;
    int         height;
    int         color_depth;
} tapi_upnp_cd_resource;

/** Node of the @b tapi_upnp_cd_resource list. */
typedef struct tapi_upnp_cd_resource_node {
    tapi_upnp_cd_resource res;
    SLIST_ENTRY(tapi_upnp_cd_resource_node) next;
} tapi_upnp_cd_resource_node;

/**
 * Parameters of an object (common for items and containers) in
 * ContentDirectory service.
 */
typedef struct tapi_upnp_cd_object {
    const char *id;
    const char *parent_id;
    te_bool     restricted;
    const char *title;
    const char *class;
    const char *creator;
    SLIST_HEAD(, tapi_upnp_cd_resource_node) resources;
    const char *write_status;
    /*
     * Next elements belong to no Object Class according ContentDirectory
     * Specification, but in generally they are common for items and
     * containers.
     */
#ifdef LIBGUPNP_VER_0_12
    /* Now it is available only in unstable version libgupnp. */
    uint32_t object_update_id;
#endif /* LIBGUPNP_VER_0_12 */
    SLIST_HEAD(, tapi_upnp_cd_contributor_node) artists;
    SLIST_HEAD(, tapi_upnp_cd_contributor_node) authors;
    const char *album;
    const char *album_art;
    const char *genre;
    const char *description;
    const char *date;
    int         track_number;
} tapi_upnp_cd_object;

/** A container context in ContentDirectory service. */
typedef struct tapi_upnp_cd_container {
    tapi_upnp_cd_object_type type;
    tapi_upnp_cd_object base;
    union {
        struct {
            int      child_count;
            te_bool  searchable;
#ifndef LIBGUPNP_DIDLLITE_CONTAINER_BUG
            int64_t  storage_used;
#endif /* !LIBGUPNP_DIDLLITE_CONTAINER_BUG */
#ifdef LIBGUPNP_VER_0_12
            uint32_t container_update_id;
            uint32_t total_deleted_child_count;
#endif /* LIBGUPNP_VER_0_12 */
        } container;
        struct {
            const char *ref_id;
#ifdef LIBGUPNP_VER_0_12
            /* Now it is available only in unstable version libgupnp. */
            int64_t     lifetime;
#endif /* LIBGUPNP_VER_0_12 */
        } item;
    };
} tapi_upnp_cd_container;

/** Node of the @b tapi_upnp_cd_container list. */
typedef struct tapi_upnp_cd_container_node {
    tapi_upnp_cd_container data;
    SLIST_HEAD(, tapi_upnp_cd_container_node) children;
    struct tapi_upnp_cd_container_node *parent;
    SLIST_ENTRY(tapi_upnp_cd_container_node) next;
} tapi_upnp_cd_container_node;


/**
 * Prototype of function of handler for using in @sa tapi_upnp_cd_tree_dfs.
 *
 * @param container     Container context
 */
typedef void (*cd_handler)(tapi_upnp_cd_container_node *, void *);


/**
 * Recursively call callbacks on each container in UPnP ContentDirectory
 * tree using depth-first search on tree.
 *
 * @param container     UPnP ContentDirectory subtree root element.
 * @param presearch     Pre search handler that is called on subtree node.
 * @param postsearch    Post search handler that is called on subtree node.
 * @param user_data     Additional data to post to @p presearch and
 *                      @p postsearch handlers.
 */
void tapi_upnp_cd_tree_dfs(tapi_upnp_cd_container_node *container,
                           cd_handler presearch, cd_handler postsearch,
                           void *user_data);

/**
 * Retrieve information about a container parent object.
 * It does not invoke action on UPnP Control Point just gets parent from
 * child context.
 *
 * @param container  The container location which contains parent content.
 *
 * @return Parent container, or @c NULL if @p container has no parent, i.e.
 *         it is a root object.
 */
static inline tapi_upnp_cd_container_node *
tapi_upnp_cd_get_parent(const tapi_upnp_cd_container_node *container)
{
    return container->parent;
}

/**
 * Retrieve information about a root container.
 *
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  service      ContentDirectory service context.
 * @param[out] container    The container location where information will be
 *                          saved.
 *
 * @return Status code. On success, @c 0.
 */
extern te_errno tapi_upnp_cd_get_root(
                                rcf_rpc_server               *rpcs,
                                const tapi_upnp_service_info *service,
                                tapi_upnp_cd_container_node  *container);

/**
 * Retrieve information about a container child objects. Returned data about
 * the children will be saved inside the children list of @p container.
 *
 * @param[in]    rpcs       RPC server handle.
 * @param[in]    service    ContentDirectory service context.
 * @param[in,out] container The container location which contains parent
 *                          content.
 *
 * @return Status code. On success, @c 0.
 */
extern te_errno tapi_upnp_cd_get_children(
                                rcf_rpc_server               *rpcs,
                                const tapi_upnp_service_info *service,
                                tapi_upnp_cd_container_node  *container);

/**
 * Retrieve tree structure and data of Content Directory which placement is
 * matched to @p path_filter, for example, Video/Folders. This function
 * releases @p container before retrieve a new data. Note, @p container
 * should be released with either @b tapi_upnp_cd_remove_container or
 * @b tapi_upnp_cd_remove_tree when it is no longer needed.
 *
 * @param[in]    rpcs           RPC server handle.
 * @param[in]    service        ContentDirectory service context.
 * @param[in]    path_filter    Root path to retrieve data from. May be
                                @c NULL or empty string to retrieve all
                                content.
 * @param[in,out] container     Container to put data to.
 *
 * @return Status code. On success, @c 0, on error @p container will be not
 *         changed.
 *
 * @retval TE_ENODATA   There are no data which is satisfied to
 *                      @p path_filter.
 */
extern te_errno tapi_upnp_cd_get_tree(
                                rcf_rpc_server               *rpcs,
                                const tapi_upnp_service_info *service,
                                const char                   *path_filter,
                                tapi_upnp_cd_container_node  *container);

/**
 * Remove the UPnP ContentDirectory container from the tree with all of it
 * children, free content memory and update parent. User should care about
 * freeing @p container if needed.
 *
 * @param container     Container context.
 */
extern void tapi_upnp_cd_remove_container(
                                tapi_upnp_cd_container_node *container);

/**
 * Remove the all UPnP ContentDirectory containers: go through all parents
 * to the root node and then remove all of it children. User should care
 * about freeing @p root if needed.
 *
 * @param root      Container context.
 *
 * @sa tapi_upnp_cd_remove_container
 */
extern void tapi_upnp_cd_remove_tree(tapi_upnp_cd_container_node *root);

/**
 * Print UPnP Content Directory context using RING function.
 * This function should be used for debugging purpose.
 *
 * @param container     Container context.
 */
extern void tapi_upnp_print_content_directory(
                            const tapi_upnp_cd_container_node *container);

/**
 * Get a number of UPnP Content Directory objects.
 *
 * @param container     Container context.
 * @param type          Content Directory object type.
 *
 * @return Number of UPnP Content Directory objects.
 */
extern size_t tapi_upnp_cd_get_objects_count(
                            const tapi_upnp_cd_container_node *container,
                            tapi_upnp_cd_object_type           type);

/**@} <!-- END tapi_upnp_cd_service --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_UPNP_CONTENT_DIRECTORY_H__ */
