/** @file
 * @brief Test API to storage device routines
 *
 * Functions for convinient work with storage device.
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

#ifndef __TAPI_LOCAL_STORAGE_DEVICE_H__
#define __TAPI_LOCAL_STORAGE_DEVICE_H__


#ifdef __cplusplus
extern "C" {
#endif

/**
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
typedef te_errno (* tapi_local_storage_device_insert)(
                                        tapi_local_storage_device *device);

/**
 * Eject device.
 *
 * @param device        Device handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_local_storage_device_eject)(
                                        tapi_local_storage_device *device);

/**
 * Methods to operate with device.
 */
typedef struct tapi_local_storage_device_methods {
    tapi_local_storage_device_insert insert;
    tapi_local_storage_device_eject  eject;
} tapi_local_storage_device_methods;

/**
 * Devices types.
 */
typedef enum tapi_local_storage_device_type {
    TAPI_LOCAL_STORAGE_DEVICE_USB,
} tapi_local_storage_device_type;

/**
 * Device properties and methods to operate.
 */
struct tapi_local_storage_device {
    tapi_local_storage_device_type type;
    const char *name;
    const char *vendor;
    const char *product;
    const char *serial;
    const char *partition;
    tapi_local_storage_device_methods methods;
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
 * Get a certain device info from configurator.
 *
 * @param[in]  ta       Test agent name.
 * @param[in]  name     Device name to find.
 * @param[out] device   Device context.
 *
 * @return Status code.
 */
extern te_errno tapi_local_storage_device_get(
                                        const char                *ta,
                                        const char                *name,
                                        tapi_local_storage_device *device);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_LOCAL_STORAGE_DEVICE_H__ */

