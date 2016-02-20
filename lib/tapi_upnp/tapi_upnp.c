/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Implementation of Test API DLNA UPnP commons.
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

#define TE_LGR_USER     "TAPI UPnP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#include "logger_api.h"
#include "tapi_upnp.h"


/**
 * Concatenate two strings. User should care about freeing memory allocated
 * for output string.
 *
 * @param first     First string to concatenate.
 * @param seconf    Second string to concatenate.
 *
 * @return Concatenated string, or @c NULL on error.
 */
static char *
string_concat(const char *first, const char *second)
{
    char  *str;
    size_t len1;

    if (first == NULL || second == NULL)
    {
        ERROR("input strings must not be NULL");
        return NULL;
    }
    len1 = strlen(first);
    str = malloc(len1 + strlen(second) + 1);
    if (str == NULL)
    {
        ERROR("%s:%d: cannot allocate memory", __FUNCTION__, __LINE__);
        return NULL;
    }
    strcpy(str, first);
    strcpy(&str[len1], second);
    return str;
}


/* See description in tapi_upnp.h. */
char *
tapi_upnp_get_st_uuid(const char *uuid)
{
    return string_concat("uuid:", uuid);
}

/* See description in tapi_upnp.h. */
char *
tapi_upnp_get_st_device_type(const char *domain, const char *device_type)
{
    char *urn = string_concat("urn:", domain);
    char *device = string_concat(":device:", device_type);
    char *st = string_concat(urn, device);

    free(urn);
    free(device);
    return st;
}

/* See description in tapi_upnp.h. */
char *
tapi_upnp_get_st_service_type(const char *domain, const char *service_type)
{
    char *urn = string_concat("urn:", domain);
    char *service = string_concat(":service:", service_type);
    char *st = string_concat(urn, service);

    free(urn);
    free(service);
    return st;
}
