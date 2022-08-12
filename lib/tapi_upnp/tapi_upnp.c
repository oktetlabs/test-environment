/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Implementation of Test API DLNA UPnP commons.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TAPI UPnP"

#include "te_config.h"
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "tapi_upnp.h"
#include "te_str.h"


/* See description in tapi_upnp.h. */
char *
tapi_upnp_get_st_uuid(const char *uuid)
{
    return te_str_concat("uuid:", uuid);
}

/* See description in tapi_upnp.h. */
char *
tapi_upnp_get_st_device_type(const char *domain, const char *device_type)
{
    char *urn = te_str_concat("urn:", domain);
    char *device = te_str_concat(":device:", device_type);
    char *st = te_str_concat(urn, device);

    free(urn);
    free(device);
    return st;
}

/* See description in tapi_upnp.h. */
char *
tapi_upnp_get_st_service_type(const char *domain, const char *service_type)
{
    char *urn = te_str_concat("urn:", domain);
    char *service = te_str_concat(":service:", service_type);
    char *st = te_str_concat(urn, service);

    free(urn);
    free(service);
    return st;
}
