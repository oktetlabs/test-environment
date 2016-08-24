/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Implementation of Test API for DLNA UPnP Device features.
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

#define TE_LGR_USER     "TAPI UPnP Device Info"

#include "te_config.h"

#include <jansson.h>
#include "logger_api.h"
#include "tapi_upnp_cp.h"
#include "tapi_upnp_device_info.h"
#include "te_string.h"


/** Device property accessors. */
typedef struct {
    const char *name;
    const char *(*get_value)(const tapi_upnp_device_info *);
    te_errno    (*set_value)(tapi_upnp_device_info *, const json_t *);
} upnp_device_property_t;

/**
 * Set a device property string value.
 *
 * @param device            Device.
 * @param property_idx      Index of certain property in properties array.
 * @param value             JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
tapi_upnp_set_device_property(tapi_upnp_device_info *device,
                              te_upnp_device_property_idx property_idx,
                              const json_t *value)
{
    const char *property = json_string_value(value);

    if (property == NULL)
    {
        ERROR("Invalid property. JSON string was expected");
        return TE_EINVAL;
    }
    if (property_idx >= DPROPERTY_MAX)
    {
        ERROR("Invalid array index");
        return TE_EINVAL;
    }
    device->properties[property_idx] = strdup(property);
    return (device->properties[property_idx] == NULL ? TE_ENOMEM : 0);
}

/**
 * Set Unique Device Name property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_udn(tapi_upnp_device_info *device,
                         const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_UDN, value);
}

/**
 * Set Device Type property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_type(tapi_upnp_device_info *device,
                          const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_TYPE, value);
}

/**
 * Set Device Location property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_location(tapi_upnp_device_info *device,
                              const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_LOCATION, value);
}

/**
 * Set Device Friendly Name property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_friendly_name(tapi_upnp_device_info *device,
                                   const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_FRIENDLY_NAME,
                                         value);
}

/**
 * Set Device Manufacturer property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_manufacturer(tapi_upnp_device_info *device,
                                  const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_MANUFACTURER,
                                         value);
}

/**
 * Set Device Manufacturer URL property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_manufacturer_url(tapi_upnp_device_info *device,
                                      const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_MANUFACTURER_URL,
                                         value);
}

/**
 * Set Device Model Description property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_model_description(tapi_upnp_device_info *device,
                                       const json_t          *value)
{
    return tapi_upnp_set_device_property(device,
                                         DPROPERTY_MODEL_DESCRIPTION,
                                         value);
}

/**
 * Set Device Model Name property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_model_name(tapi_upnp_device_info *device,
                                const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_MODEL_NAME,
                                         value);
}

/**
 * Set Device Model Number property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_model_number(tapi_upnp_device_info *device,
                                  const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_MODEL_NUMBER,
                                         value);
}

/**
 * Set Device Model URL property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_model_url(tapi_upnp_device_info *device,
                               const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_MODEL_URL,
                                         value);
}

/**
 * Set Device Serial Number property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_serial_number(tapi_upnp_device_info *device,
                                   const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_SERIAL_NUMBER,
                                         value);
}

/**
 * Set Device Universal Product Code property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_upc(tapi_upnp_device_info *device,
                         const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_UPC, value);
}

/**
 * Set Device Icon URL property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_icon_url(tapi_upnp_device_info *device,
                              const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_ICON_URL, value);
}

/**
 * Set Device Presentation URL property.
 *
 * @param device    Device.
 * @param value     JSON string value.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_device_presentation_url(tapi_upnp_device_info *device,
                                      const json_t          *value)
{
    return tapi_upnp_set_device_property(device, DPROPERTY_PRESENTATION_URL,
                                         value);
}

/** Properties accessors (for internal use). */
static upnp_device_property_t property[] = {
    [DPROPERTY_UDN] = { "UDN", tapi_upnp_get_device_udn,
                               tapi_upnp_set_device_udn },
    [DPROPERTY_TYPE] = { "Type", tapi_upnp_get_device_type,
                                 tapi_upnp_set_device_type },
    [DPROPERTY_LOCATION] = { "Location",
                             tapi_upnp_get_device_location,
                             tapi_upnp_set_device_location },
    [DPROPERTY_FRIENDLY_NAME] = { "Friendly Name",
                                  tapi_upnp_get_device_friendly_name,
                                  tapi_upnp_set_device_friendly_name },
    [DPROPERTY_MANUFACTURER] = { "Manufacturer",
                                 tapi_upnp_get_device_manufacturer,
                                 tapi_upnp_set_device_manufacturer },
    [DPROPERTY_MANUFACTURER_URL] = { "Manufacturer URL",
                            tapi_upnp_get_device_manufacturer_url,
                            tapi_upnp_set_device_manufacturer_url },
    [DPROPERTY_MODEL_DESCRIPTION] = { "Model Description",
                            tapi_upnp_get_device_model_description,
                            tapi_upnp_set_device_model_description },
    [DPROPERTY_MODEL_NAME] = { "Model Name",
                               tapi_upnp_get_device_model_name,
                               tapi_upnp_set_device_model_name },
    [DPROPERTY_MODEL_NUMBER] = { "Model Number",
                                 tapi_upnp_get_device_model_number,
                                 tapi_upnp_set_device_model_number },
    [DPROPERTY_MODEL_URL] = { "Model URL",
                              tapi_upnp_get_device_model_url,
                              tapi_upnp_set_device_model_url },
    [DPROPERTY_SERIAL_NUMBER] = { "Serial Number",
                                  tapi_upnp_get_device_serial_number,
                                  tapi_upnp_set_device_serial_number },
    [DPROPERTY_UPC] = { "UPC", tapi_upnp_get_device_upc,
                               tapi_upnp_set_device_upc },
    [DPROPERTY_ICON_URL] = { "Icon URL",
                             tapi_upnp_get_device_icon_url,
                             tapi_upnp_set_device_icon_url },
    [DPROPERTY_PRESENTATION_URL] = { "Presentation URL",
                            tapi_upnp_get_device_presentation_url,
                            tapi_upnp_set_device_presentation_url }
};

/**
 * Extract UPnP device's properties from JSON array.
 *
 * @param[in]  jarray   JSON array contains UPnP device.
 * @param[out] device   Device context.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_device_properties(const json_t *jarray, tapi_upnp_device_info *device)
{
    int    rc = 0;
    size_t i;

    if (jarray == NULL || !json_is_array(jarray))
    {
        ERROR("Invalid input data. JSON array was expected");
        return TE_EINVAL;
    }
    for (i = 0; i < TE_ARRAY_LEN(property); i++)
    {
        rc = property[i].set_value(device, json_array_get(jarray, i));
        if (rc != 0)
            break;
    }
    if (rc != 0)
    {
        for (i = 0; i < TE_ARRAY_LEN(device->properties); i++)
        {
            free(device->properties[i]);
            device->properties[i] = NULL;
        }
    }
    return rc;
}

/**
 * Extract UPnP devices from JSON array.
 *
 * @param[in]  jarray       JSON array contains UPnP devices.
 * @param[out] devices      Devices context list.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_devices(const json_t *jarray, tapi_upnp_devices *devices)
{
    int     rc = 0;
    ssize_t num_devices;
    ssize_t i;
    tapi_upnp_device_info *device;

    if (jarray == NULL || !json_is_array(jarray))
    {
        ERROR("Invalid input data. JSON array was expected");
        return TE_EINVAL;
    }
    if (!SLIST_EMPTY(devices))
        VERB("Devices list is not empty");
    num_devices = json_array_size(jarray);
    for (i = 0; i < num_devices; i++)
    {
        device = calloc(1, sizeof(*device));
        if (device == NULL)
        {
            ERROR("Cannot allocate memory");
            return TE_ENOMEM;
        }
        rc = parse_device_properties(json_array_get(jarray, i), device);
        if (rc != 0)
        {
            ERROR("Fail to extract properties");
            free(device);
            return rc;
        }
        SLIST_INSERT_HEAD(devices, device, next);
    }
    return 0;
}

/* See description in tapi_upnp_device_info.h. */
const char *
tapi_upnp_get_device_property(const tapi_upnp_device_info *device,
                              te_upnp_device_property_idx  property_idx)
{
    return (property_idx < DPROPERTY_MAX
            ? (const char *)device->properties[property_idx]
            : NULL);
}

/* See description in tapi_upnp_device_info.h. */
te_errno
tapi_upnp_get_device_info(rcf_rpc_server    *rpcs,
                          const char        *name,
                          tapi_upnp_devices *devices)
{
    json_error_t error;
    json_t      *jrequest = NULL;
    json_t      *jreply = NULL;
    char        *request;
    char        *reply = NULL;
    size_t       reply_len = 0;
    te_upnp_cp_request_type reply_type;
    te_errno     rc = 0;

    if (name == NULL)
        name = "";

    /* Prepare request. */
    jrequest = json_pack("[i,s]", UPNP_CP_REQUEST_DEVICE, name);
    if (jrequest == NULL)
    {
        ERROR("json_pack fails");
        return TE_ENOMEM;
    }
    request = json_dumps(jrequest, JSON_ENCODING_FLAGS);
    if (request == NULL)
    {
        ERROR("json_dumps fails");
        json_decref(jrequest);
        return TE_ENOMEM;
    }

    /* Send request. */
    rc = rpc_upnp_cp_action(rpcs, request, strlen(request) + 1,
                            (void **)&reply, &reply_len);
    if (rc != 0)
        goto get_device_info_cleanup;

    /* Parse reply. */
    jreply = json_loads(reply, 0, &error);
    if (jreply == NULL)
    {
        ERROR("json_loads fails with massage: \"%s\", position: %u",
              error.text, error.position);
        rc = TE_EINVAL;
        goto get_device_info_cleanup;
    }
    if (!json_is_integer(json_array_get(jreply, 0)))
    {
        ERROR("Invalid reply type. JSON integer was expected");
        rc = TE_EINVAL;
        goto get_device_info_cleanup;
    }
    reply_type = json_integer_value(json_array_get(jreply, 0));
    if (reply_type != UPNP_CP_REQUEST_DEVICE)
    {
        ERROR("Unexpected reply type");
        rc = TE_EINVAL;
        goto get_device_info_cleanup;
    }

    rc = parse_devices(json_array_get(jreply, 1), devices);
    if (rc != 0)
    {
        ERROR("parse_devices fails");
        tapi_upnp_free_device_info(devices);
    }

get_device_info_cleanup:
    free(reply);
    free(request);
    json_decref(jreply);
    json_decref(jrequest);
    return rc;
}

/* See description in tapi_upnp_device_info.h. */
void
tapi_upnp_free_device_info(tapi_upnp_devices *devices)
{
    tapi_upnp_device_info *device, *dev_tmp;
    size_t i;

    SLIST_FOREACH_SAFE(device, devices, next, dev_tmp)
    {
        for (i = 0; i < TE_ARRAY_LEN(device->properties); i++)
        {
            free(device->properties[i]);
        }
        SLIST_REMOVE(devices, device, tapi_upnp_device_info, next);
        free(device);
    }
}

/* See description in tapi_upnp_device_info.h. */
void
tapi_upnp_print_device_info(const tapi_upnp_devices *devices)
{
    size_t    i;
    size_t    num_devices = 0;
    te_string dump = TE_STRING_INIT;
    tapi_upnp_device_info *device;

    if (SLIST_EMPTY(devices))
    {
        RING("List of devices is empty");
        return;
    }

    SLIST_FOREACH(device, devices, next)
    {
        num_devices++;
        te_string_append(&dump, "[\n");
        for (i = 0; i < TE_ARRAY_LEN(property); i++)
            te_string_append(&dump, " %s: %s\n",
                             property[i].name,
                             property[i].get_value(device));
        te_string_append(&dump, "],\n");
    }
    te_string_append(&dump, "---\n");
    te_string_append(&dump, "Total number of devices: %u\n", num_devices);
    RING(dump.ptr);
    te_string_free(&dump);
}
