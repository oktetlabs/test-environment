/** @file
 * @brief Test API to storage device routines
 *
 * @defgroup tapi_local_storage_device Test API to control the local storage device
 * @ingroup tapi_storage
 * @{
 *
 * Functions for convenient work with storage device.
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

#ifndef __TAPI_LOCAL_STORAGE_DEVICE_H__
#define __TAPI_LOCAL_STORAGE_DEVICE_H__

#include "te_errno.h"
#include "te_queue.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Forward declaration of generic device structure.
 */
struct tapi_local_storage_device;
typedef struct tapi_local_storage_device tapi_local_storage_device;

/**
 * Insert device.
 *
 * @param device        Device handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_local_storage_device_method_insert)(
                                        tapi_local_storage_device *device);

/**
 * Eject device.
 *
 * @param device        Device handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_local_storage_device_method_eject)(
                                        tapi_local_storage_device *device);

/**
 * Methods to operate with device.
 */
typedef struct tapi_local_storage_device_methods {
    tapi_local_storage_device_method_insert insert;
    tapi_local_storage_device_method_eject  eject;
} tapi_local_storage_device_methods;

/**
 * Devices types.
 */
typedef enum tapi_local_storage_device_type {
    TAPI_LOCAL_STORAGE_DEVICE_USB,      /**< USB mass storage device */
} tapi_local_storage_device_type;

/**
 * Device properties and methods to operate.
 */
struct tapi_local_storage_device {
    tapi_local_storage_device_type type;    /**< Type of storage device. */
    const char *name;           /**< Name of storage in configurator. */
    const char *vid;            /**< Disk Vendor ID. */
    const char *pid;            /**< Disk Product ID. */
    const char *serial;         /**< Disk Serial Number. */
    const char *product_name;   /**< Disk Product Name. */
    const char *manufacturer;   /**< Disk Manufacturer. */
    const char *partition;      /**< Disk partition under test. */

    /** Methods to operate the device. */
    const tapi_local_storage_device_methods *methods;
};

/**
 * Devices list entry.
 */
typedef struct tapi_local_storage_device_le {
    tapi_local_storage_device device;
    SLIST_ENTRY(tapi_local_storage_device_le) next;
} tapi_local_storage_device_le;

/**
 * Head of devices list.
 */
SLIST_HEAD(tapi_local_storage_device_list, tapi_local_storage_device_le);
typedef struct tapi_local_storage_device_list
    tapi_local_storage_device_list;


/**
 * Set methods to operate the @p device. @p device should be pre-initialized
 * with, for example, @p tapi_local_storage_device_get
 *
 * @param device        Device context.
 * @param methods       Methods to operate the device.
 *
 * @sa tapi_local_storage_device_get
 */
static inline void
tapi_local_storage_device_set_methods(
                        tapi_local_storage_device               *device,
                        const tapi_local_storage_device_methods *methods)
{
    device->methods = methods;
}

/**
 * Insert device.
 *
 * @param device        Device handle.
 *
 * @return Status code.
 */
static inline te_errno
tapi_local_storage_device_insert(tapi_local_storage_device *device)
{
    return (device->methods != NULL && device->methods->insert != NULL
            ? device->methods->insert(device) : TE_EOPNOTSUPP);
}

/**
 * Eject device.
 *
 * @param device        Device handle.
 *
 * @return Status code.
 */
static inline te_errno
tapi_local_storage_device_eject(tapi_local_storage_device *device)
{
    return (device->methods != NULL && device->methods->eject != NULL
            ? device->methods->eject(device) : TE_EOPNOTSUPP);
}

/**
 * Get all devices from configurator and read them properties.
 * List of @p devices should be released with
 * @p tapi_local_storage_device_list_free when it is no longer needed.
 *
 * @param[out] devices          Devices list.
 *
 * @return Status code.
 *
 * @sa tapi_local_storage_device_list_free
 */
extern te_errno tapi_local_storage_device_list_get(
                                tapi_local_storage_device_list *devices);

/**
 * Release devices list that was obtained with
 * @p tapi_local_storage_device_list_get.
 *
 * @param devices       Devices list.
 *
 * @sa tapi_local_storage_device_list_get
 */
extern void tapi_local_storage_device_list_free(
                                tapi_local_storage_device_list *devices);

/**
 * Get a certain device info from configurator. @p device should be released
 * with @p tapi_local_storage_device_free when it is no longer needed.
 *
 * @param[in]  name     Device name to find.
 * @param[out] device   Device context.
 *
 * @return Status code.
 *
 * @sa tapi_local_storage_device_free
 */
extern te_errno tapi_local_storage_device_get(
                                        const char                 *name,
                                        tapi_local_storage_device **device);

/**
 * Release device context that was obtained with
 * @p tapi_local_storage_device_get.
 *
 * @param device        Device context.
 *
 * @sa tapi_local_storage_device_get
 */
extern void tapi_local_storage_device_free(
                                        tapi_local_storage_device *device);

/**
 * Initialize a device by information obtained from configurator. @p device
 * should be released with @p tapi_local_storage_device_fini when it is no
 * longer needed.
 *
 * @param name          Device name to find.
 * @param device        Device context.
 *
 * @return Status code.
 *
 * @sa tapi_local_storage_device_fini
 */
extern te_errno tapi_local_storage_device_init(
                                        const char                *name,
                                        tapi_local_storage_device *device);

/**
 * Release device context which was initialized with
 * @p tapi_local_storage_device_init.
 *
 * @param device        Device context.
 *
 * @sa tapi_local_storage_device_init
 */
extern void tapi_local_storage_device_fini(
                                        tapi_local_storage_device *device);

/**@} <!-- END tapi_local_storage_device --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_LOCAL_STORAGE_DEVICE_H__ */

