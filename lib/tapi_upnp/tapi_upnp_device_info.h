/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * @defgroup tapi_upnp_device_info Test API to operate the DLNA UPnP Device information
 * @ingroup tapi_upnp
 * @{
 *
 * Definition of Test API for DLNA UPnP Device features.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#ifndef __TAPI_UPNP_DEVICE_INFO_H__
#define __TAPI_UPNP_DEVICE_INFO_H__

#include "te_queue.h"
#include "rcf_rpc.h"
#include "te_upnp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A UPnP device parameters. */
typedef struct tapi_upnp_device_info {
    void *properties[DPROPERTY_MAX];
    SLIST_ENTRY(tapi_upnp_device_info) next;
} tapi_upnp_device_info;

/** Head of the UPnP devices list. */
SLIST_HEAD(tapi_upnp_devices, tapi_upnp_device_info);
typedef struct tapi_upnp_devices tapi_upnp_devices;


/* Device properties. */

/**
 * Get a device property string value.
 *
 * @param device            Device.
 * @param property_idx      Index of certain property in properties array.
 *
 * @return The string property value or @c NULL if the property is not
 *         found.
 */
const char *tapi_upnp_get_device_property(
                        const tapi_upnp_device_info *device,
                        te_upnp_device_property_idx  property_idx);

/**
 * Get Unique Device Name from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The Unique Device Name or @c NULL in case of missing property.
 */
static inline const char *
tapi_upnp_get_device_udn(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_UDN);
}

/**
 * Get Device Type from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The Device Type or @c NULL in case of missing property.
 */
static inline const char *
tapi_upnp_get_device_type(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_TYPE);
}

/**
 * Get Device Location from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The Device Location or @c NULL in case of missing property.
 */
static inline const char *
tapi_upnp_get_device_location(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_LOCATION);
}

/**
 * Get Device Friendly Name from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The friendly name of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_friendly_name(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_FRIENDLY_NAME);
}

/**
 * Get Device Manufacturer from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The manufacturer of the or @c NULL in case of missing property.
 */
static inline const char *
tapi_upnp_get_device_manufacturer(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_MANUFACTURER);
}

/**
 * Get Device Manufacturer URL from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The manufacturer URL of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_manufacturer_url(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device,
                                         DPROPERTY_MANUFACTURER_URL);
}

/**
 * Get Device Model Description from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The model description of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_model_description(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device,
                                         DPROPERTY_MODEL_DESCRIPTION);
}

/**
 * Get Device Model Name from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The model name of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_model_name(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_MODEL_NAME);
}

/**
 * Get Device Model Number from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The model number of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_model_number(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_MODEL_NUMBER);
}

/**
 * Get Device Model URL from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The model URL of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_model_url(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_MODEL_URL);
}

/**
 * Get Device Serial Number from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The serial number of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_serial_number(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_SERIAL_NUMBER);
}

/**
 * Get Device Universal Product Code from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The Universal Product Code of the device or @c NULL in case of
 *         missing property.
 */
static inline const char *
tapi_upnp_get_device_upc(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_UPC);
}

/**
 * Get Device Icon URL from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The icon URL of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_icon_url(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device, DPROPERTY_ICON_URL);
}

/**
 * Get Device Presentation URL from the UPnP device properties.
 *
 * @param device    Device.
 *
 * @return The presentation URL of the device or @c NULL in case of missing
 *         property.
 */
static inline const char *
tapi_upnp_get_device_presentation_url(const tapi_upnp_device_info *device)
{
    return tapi_upnp_get_device_property(device,
                                         DPROPERTY_PRESENTATION_URL);
}


/* Devices. */

/**
 * Retrieve an information about the UPnP device. Note, @p devices should be
 * freed with @b tapi_upnp_free_device_info when it is no longer needed.
 *
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  name         Device name. Can be @c NULL or zero-length
 *                          string to get the all found devices.
 * @param[out] devices      Devices context list.
 *
 * @return Status code. On success, @c 0.
 *
 * @sa tapi_upnp_free_device_info
 */
extern te_errno tapi_upnp_get_device_info(rcf_rpc_server    *rpcs,
                                          const char        *name,
                                          tapi_upnp_devices *devices);

/**
 * Empty the list of UPnP devices (free allocated memory) which was obtained
 * with @b tapi_upnp_get_device_info.
 *
 * @param devices   List of devices.
 *
 * @sa tapi_upnp_get_device_info
 */
extern void tapi_upnp_free_device_info(tapi_upnp_devices *devices);

/**
 * Print UPnP devices context using RING function.
 * This function should be used for debugging purpose.
 *
 * @param devices    Devices context list.
 */
extern void tapi_upnp_print_device_info(const tapi_upnp_devices *devices);

/**@} <!-- END tapi_upnp_device_info --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_UPNP_DEVICE_INFO_H__ */
