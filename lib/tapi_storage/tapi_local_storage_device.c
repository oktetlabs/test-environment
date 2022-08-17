/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to storage device routines
 *
 * Functions for convenient work with storage device.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI Storage Device"

#include "tapi_local_storage_device.h"
#include "logger_api.h"
#include "conf_api.h"
#include "te_alloc.h"


/* SubID of local storage device entry in Configuration tree. */
#define TE_CFG_LOCAL_STORAGE_DEVICE_SUBID "/local:/dut:/storage:/device:"
/* Format string for local storage device entry in Configuration tree. */
#define TE_CFG_LOCAL_STORAGE_DEVICE_FMT \
    TE_CFG_LOCAL_STORAGE_DEVICE_SUBID "%s"

/* Mapping device type to string representation. */
static const char *device_type_map[] = {
    [TAPI_LOCAL_STORAGE_DEVICE_USB] = "usb"
};


/**
 * Get storage device type corresponding to it string representation.
 *
 * @param[in]  type_name        String representation of device type.
 * @param[out] type             Device type.
 *
 * @return Status code.
 */
static te_errno
get_device_type(const char *type_name, tapi_local_storage_device_type *type)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(device_type_map); i++)
    {
        if (strcmp(type_name, device_type_map[i]) == 0)
        {
            *type = (tapi_local_storage_device_type)i;
            return 0;
        }
    }
    ERROR("Unknown type name: %s", type_name);
    return TE_RC(TE_TAPI, TE_ENOSYS);
}

/**
 * Free local storage device context.
 *
 * @param device    Device which context should be freed.
 */
static inline void
free_device_context(tapi_local_storage_device *device)
{
    /* Discard const specifier by explicit type casting. */
    free((char *)device->name);
    free((char *)device->vid);
    free((char *)device->pid);
    free((char *)device->serial);
    free((char *)device->product_name);
    free((char *)device->manufacturer);
    free((char *)device->partition);
}

/**
 * Get device context from configurator. @p device context should be freed
 * with @p free_device_context when it is no longer needed.
 *
 * @param[in]  name     Device name to find.
 * @param[out] device   Device context.
 *
 * @return Status code.
 *
 * @sa free_device_context
 */
static te_errno
get_device_context(const char                *name,
                   tapi_local_storage_device *device)
{
    te_errno  rc;
    char     *property = NULL;

    do {
        rc = cfg_get_instance_fmt(NULL, &property,
                            TE_CFG_LOCAL_STORAGE_DEVICE_FMT "/type:", name);
        if (rc == 0)
        {
            rc = get_device_type(property, &device->type);
            free(property);
        }
        if (rc != 0)
            break;

/*
 * Get string type property.
 *
 * @param subid_        Property SUBID in configurator tree.
 * @param dev_prop_     Device property to save property value to.
 */
#define GET_STRING_PROPERTY(subid_, dev_prop_) \
        rc = cfg_get_instance_fmt(NULL, &property,                         \
                TE_CFG_LOCAL_STORAGE_DEVICE_FMT "/" subid_ ":", name);     \
        if (rc == 0)                                                       \
            dev_prop_ = property;                                          \
        else                                                               \
            break;

        GET_STRING_PROPERTY("vid", device->vid);
        GET_STRING_PROPERTY("pid", device->pid);
        GET_STRING_PROPERTY("serial", device->serial);
        GET_STRING_PROPERTY("product_name", device->product_name);
        GET_STRING_PROPERTY("manufacturer", device->manufacturer);
        GET_STRING_PROPERTY("partition", device->partition);

#undef GET_STRING_PROPERTY
    } while (0);

    device->name = strdup(name);
    if (device->name == NULL)
    {
        ERROR("%s:%d: Failed to duplicate string", __FUNCTION__, __LINE__);
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
    }

    if (rc != 0)
        free_device_context(device);
    return rc;
}


/* See description in tapi_local_storage_device.h. */
te_errno
tapi_local_storage_device_get(const char                 *name,
                              tapi_local_storage_device **device)
{
    tapi_local_storage_device *dev;
    te_errno  rc;

    dev = TE_ALLOC(sizeof(*dev));
    if (dev == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = get_device_context(name, dev);
    if (rc == 0)
        *device = dev;
    else
        free(dev);

    return rc;
}

/* See description in tapi_local_storage_device.h. */
void
tapi_local_storage_device_free(tapi_local_storage_device *device)
{
    if (device != NULL)
    {
        free_device_context(device);
        free(device);
    }
}

/* See description in tapi_local_storage_device.h. */
te_errno
tapi_local_storage_device_list_get(
                                tapi_local_storage_device_list *devices)
{
    tapi_local_storage_device_le *dev;
    te_errno      rc;
    cfg_handle   *handles;
    unsigned int  n_handles;
    char         *name;
    size_t        i;

    SLIST_INIT(devices);
    rc = cfg_find_pattern_fmt(&n_handles, &handles,
                              TE_CFG_LOCAL_STORAGE_DEVICE_SUBID "*");
    if (rc != 0)
        return rc;

    for (i = 0; i < n_handles; i++)
    {
        dev = TE_ALLOC(sizeof(*dev));
        if (dev == NULL)
        {
            rc = TE_RC(TE_TAPI, TE_ENOMEM);
            break;
        }
        rc = cfg_get_inst_name(handles[i], &name);
        if (rc == 0)
        {
            rc = get_device_context(name, &dev->device);
            free(name);
        }
        if (rc != 0)
        {
            free(dev);
            break;
        }
        SLIST_INSERT_HEAD(devices, dev, next);
    }
    free(handles);
    if (rc != 0)
        tapi_local_storage_device_list_free(devices);
    return rc;
}

/* See description in tapi_local_storage_device.h. */
void
tapi_local_storage_device_list_free(
                                tapi_local_storage_device_list *devices)
{
    tapi_local_storage_device_le *dev;

    while (!SLIST_EMPTY(devices))
    {
        dev = SLIST_FIRST(devices);
        SLIST_REMOVE_HEAD(devices, next);
        free_device_context(&dev->device);
        free(dev);
    }
}

/* See description in tapi_local_storage_device.h. */
te_errno
tapi_local_storage_device_init(const char                *name,
                               tapi_local_storage_device *device)
{
    return get_device_context(name, device);
}

/* See description in tapi_local_storage_device.h. */
void
tapi_local_storage_device_fini(tapi_local_storage_device *device)
{
    if (device != NULL)
        free_device_context(device);
}
