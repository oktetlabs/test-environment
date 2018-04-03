/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * @defgroup tapi_upnp_common Test API for DLNA UPnP commons
 * @ingroup tapi_upnp
 * @{
 *
 * Definition of Test API DLNA UPnP commons.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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

/** Search for Content Directory service only. */
#define TAPI_UPNP_ST_CONTENT_DIRECTORY \
    "urn:upnp-org:serviceId:ContentDirectory"


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

/**@} <!-- END tapi_upnp_common --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_UPNP_H__ */
