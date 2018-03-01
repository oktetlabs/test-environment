/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * @defgroup tapi_upnp_cd_resources Test API to operate the DLNA UPnP Content Directory Resources
 * @ingroup tapi_upnp
 * @{
 *
 * Definition of Test API for DLNA UPnP Content Directory Resources
 * features.
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TAPI_UPNP_RESOURCES_H__
#define __TAPI_UPNP_RESOURCES_H__

#include "rcf_rpc.h"
#include "tapi_upnp_content_directory.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Service Info. */
/** UPnP media resources URL. */
typedef struct tapi_upnp_resources_uri_node {
    tapi_upnp_cd_resource_type type;
    const char *uri;
    SLIST_ENTRY(tapi_upnp_resources_uri_node) next;
} tapi_upnp_resources_uri_node;

/** Head of the UPnP media resources list. */
SLIST_HEAD(tapi_upnp_media_uri, tapi_upnp_resources_uri_node);
typedef struct tapi_upnp_media_uri tapi_upnp_media_uri;


/**
 * Get URLs of existed media resources, which satisfy to particular resource
 * type. Note, @p media should be freed with
 * @b tapi_upnp_resources_free_media_uri when it is no longer needed.
 *
 * @param[in]  container    Subtree of containers.
 * @param[in]  type         Resource type of media to find.
 * @param[out] media        Media URLs list to collect obtained data.
 *
 * @return Status code. On success, @c 0.
 *
 * @sa tapi_upnp_resources_free_media_uri
 */
extern te_errno tapi_upnp_resources_get_media_uri(
                                    tapi_upnp_cd_container_node *container,
                                    tapi_upnp_cd_resource_type   type,
                                    tapi_upnp_media_uri         *media);

/**
 * Empty the list of media URLs (free allocated memory) which was obtained
 * with @b tapi_upnp_resources_get_media_uri.
 *
 * @param media     Media URLs list.
 *
 * @sa tapi_upnp_resources_get_media_uri
 */
extern void tapi_upnp_resources_free_media_uri(tapi_upnp_media_uri *media);

/**
 * Print UPnP Content Directory object resource context using RING function.
 * This function should be used for debugging purpose.
 *
 * @param res           Content Directory object resource.
 */
extern void tapi_upnp_print_resource_info(const tapi_upnp_cd_resource *res);

/**@} <!-- END tapi_upnp_cd_resources --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_UPNP_RESOURCES_H__ */
