/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Definition of Test API DLNA UPnP commons.
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

#ifndef __TAPI_UPNP_H__
#define __TAPI_UPNP_H__

/* TAPI UPnP Search Targets. */
/** Search for all devices and services. */
#define TAPI_UPNP_ST_ALL_RESOURCES "ssdp:all"

/** Search for root devices only. */
#define TAPI_UPNP_ST_ALL_ROOT_DEVICES "upnp:rootdevice"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build Search Target for a particular device. User should care about
 * freeing memory allocated for output string.
 *
 * @param uuid      UUID Specified by UPnP vendor.
 *
 * @return Search Target string, or @c NULL on error.
 */
extern char *tapi_upnp_get_st_uuid(const char *uuid);

/**
 * Build Search Target for any device of particular type and a Vendor Domain
 * Name. User should care about freeing memory allocated for output string.
 *
 * @param domain        Vendor Domain Name.
 * @param device_type   Type of the Device.
 *
 * @return Search Target string, or @c NULL on error.
 */
extern char *tapi_upnp_get_st_device_type(const char *domain,
                                          const char *device_type);

/**
 * Build Search Target for any service of particular type and a Vendor
 * Domain Name. User should care about freeing memory allocated for output
 * string.
 *
 * @param domain        Vendor Domain Name.
 * @param service_type  Type of the Service.
 *
 * @return Search Target string, or @c NULL on error.
 */
extern char *tapi_upnp_get_st_service_type(const char *domain,
                                           const char *service_type);

/**
 * Build Search Target for any device of particular type defined by the UPnP
 * Forum working committee. User should care about freeing memory allocated
 * for output string.
 *
 * @param device_type   Type of the Device.
 *
 * @return Search Target string, or @c NULL on error.
 */
static inline char *
tapi_upnp_get_st_upnp_forum_device_type(const char *device_type)
{
    return tapi_upnp_get_st_device_type("schemas-upnp-org", device_type);
}

/**
 * Build Search Target for any service of particular type defined by the
 * UPnP Forum working committee. User should care about freeing memory
 * allocated for output string.
 *
 * @param service_type  Type of the Service.
 *
 * @return Search Target string, or @c NULL on error.
 */
static inline char *
tapi_upnp_get_st_upnp_forum_service_type(const char *service_type)
{
    return tapi_upnp_get_st_service_type("schemas-upnp-org", service_type);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_UPNP_H__ */
