/** @file
 * @brief PCI support
 *
 * PCI configuration tree support
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */


#define TE_LGR_USER     "Conf PCI"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if HAVE_SEARCH_H
#include <search.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_string.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "conf_common.h"

/** PCI device address */
typedef struct pci_address {
    uint16_t domain; /**< PCI domain */
    uint8_t bus;     /**< PCI bus */
    uint8_t slot;    /**< PCI slot (device) */
    uint8_t fn;      /**< PCI function */
} pci_address;

/** PCI device info */
typedef struct pci_device {
    pci_address address;          /**< PCI address */
    uint16_t vendor_id;           /**< Vendor ID */
    uint16_t device_id;           /**< Device ID */
    unsigned devno;               /**< Device instance number
                                   *   among all the devices
                                   *   with same vendor/device ID
                                   */
    uint16_t subsystem_vendor;    /**< Subsystem vendor ID */
    uint16_t subsystem_device;    /**< Subsystem device ID */
    uint32_t device_class;        /**< PCI device class */
    unsigned lock;                /**< TE resource lock counter */
    TAILQ_ENTRY(pci_device) next; /**< Next device with the same
                                   *   vendor/device ID
                                   */
} pci_device;

/** Two-sided list of PCI devices */
typedef TAILQ_HEAD(pci_devices, pci_device) pci_devices;

/** A set of devices with the same vendor/device ID */
typedef struct pci_vendor_device {
    LIST_ENTRY(pci_vendor_device) next;   /**< Next device ID for the same
                                           *   vendor
                                           */
    unsigned id;                          /** Device ID */
    unsigned next_devno;                  /** Next instance number for a PCI
                                           *   device
                                           */
    unsigned lock;                        /**< TE resource lock counter */
    pci_devices devices;                  /**< List of device instances */
} pci_vendor_device;

/** List of device sets with the same vendor/device ID */
typedef LIST_HEAD(pci_vendor_devices, pci_vendor_device)
    pci_vendor_devices;

/** A set of devices with the same vendor ID */
typedef struct pci_vendor {
    LIST_ENTRY(pci_vendor) next;         /**< Next vendor ID */
    unsigned id;                         /**< Vendor ID */
    unsigned lock;                       /**< TE resource lock counter */
    pci_vendor_devices vendor_devices;   /**< List of device IDs */
} pci_vendor;

/** List of same vendor ID device sets */
typedef LIST_HEAD(pci_vendors, pci_vendor) pci_vendors;

/** Number of all PCI devices in the system */
static size_t      n_all_devices = 0;

/** An array of all PCI devices in the system */
static pci_device *all_devices = NULL;

/** The list of all vendor IDs in the system */
static pci_vendors vendor_list;

/** Whole PCI tree TE resource lock */
static unsigned global_pci_lock;

static te_errno
parse_pci_address(const char *str, pci_address *addr)
{
    int end = 0;
    sscanf(str, "%4" SCNx16 ":%2" SCNx8 ":%2" SCNx8 ".%1" SCNo8 "%n",
           &addr->domain, &addr->bus,
           &addr->slot, &addr->fn, &end);
    if (str[end] != '\0')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    return 0;
}

static int
filter_pci_device(const struct dirent *de)
{
    pci_address unused;
    return parse_pci_address(de->d_name, &unused) == 0;
}

#define SYSFS_PCI_DEVICES_TREE "/sys/bus/pci/devices"

static te_errno
open_pci_attr(const char *name, const char *attr, FILE **result)
{
    te_errno rc = 0;
    te_string buf = TE_STRING_INIT;

    if ((rc = te_string_append(&buf, SYSFS_PCI_DEVICES_TREE "/%s/%s",
                               name, attr)) != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    *result = fopen(buf.ptr, "r");
    if (*result == NULL)
        rc = TE_OS_RC(TE_TA_UNIX, errno);
    te_string_free(&buf);
    return rc;
}

static unsigned
read_pci_hex_attr(const char *name, const char *attr)
{
    FILE *f;
    te_errno rc = open_pci_attr(name, attr, &f);
    unsigned result = 0;

    if (rc != 0)
    {
        WARN("Cannot open '%s' for PCI device '%s', rc=%x",
             attr, name, rc);
        return 0;
    }
    fscanf(f, "%x", &result);
    fclose(f);
    return result;
}

static te_bool
populate_pci_device(pci_device *dst, const char *name)
{
    int rc;

    rc = parse_pci_address(name, &dst->address);
    if (rc != 0)
    {
        ERROR("Error parsing PCI device name '%s', rc = %d", name, rc);
        return FALSE;
    }

    dst->vendor_id = read_pci_hex_attr(name, "vendor");
    if (dst->vendor_id == 0)
    {
        ERROR("Unknown vendor ID for '%s'", name);
        return FALSE;
    }

    dst->device_id = read_pci_hex_attr(name, "device");
    if (dst->device_id == 0)
    {
        ERROR("Unknown device ID for '%s'", name);
        return FALSE;
    }

    dst->subsystem_vendor = read_pci_hex_attr(name, "subsystem_vendor");
    dst->subsystem_device = read_pci_hex_attr(name, "subsystem_device");
    dst->device_class = read_pci_hex_attr(name, "class");

    return TRUE;
}

static pci_device *
scan_pci_bus(size_t *n_devices)
{
    pci_device *result;
    struct dirent **names;
    int rc;
    int i;

    rc = scandir(SYSFS_PCI_DEVICES_TREE, &names, filter_pci_device, alphasort);
    if (rc <= 0)
    {
        ERROR("Cannot get a list of PCI devices, rc=%d", rc);
        *n_devices = 0;
        return NULL;
    }

    result = calloc(rc, sizeof(*result));
    if (result == NULL)
    {
        ERROR("Out of memory");
        for (i = 0; i < rc; i++)
            free(names[i]);
        free(names);

        *n_devices = 0;
        return NULL;
    }

    for (i = 0; i < rc; i++)
    {
        if (!populate_pci_device(&result[i], names[i]->d_name))
        {
            for (; i < rc; i++)
                free(names[i]);
            free(names);

            *n_devices = 0;
            free(result);

            return NULL;
        }
        free(names[i]);
    }
    *n_devices = (size_t)rc;
    free(names);

    return result;
}

static int
addr_compar(const void *key, const void *item)
{
    const pci_address *addr = key;
    const pci_device *dev = item;

    if (addr->domain > dev->address.domain)
        return 1;
    if (addr->domain < dev->address.domain)
        return -1;
    if (addr->bus > dev->address.bus)
        return 1;
    if (addr->bus < dev->address.bus)
        return -1;
    if (addr->slot > dev->address.slot)
        return 1;
    if (addr->slot < dev->address.slot)
        return -1;
    if (addr->fn > dev->address.fn)
        return 1;
    if (addr->fn < dev->address.fn)
        return -1;

    return 0;
}

static pci_vendor *
find_vendor(const pci_vendors list, unsigned vendor_id)
{
    pci_vendor *vendor;

    LIST_FOREACH(vendor, &list, next)
    {
        if (vendor->id == vendor_id)
            return vendor;
    }

    return NULL;
}

static pci_vendor_device *
find_vendor_device(const pci_vendor_devices list, unsigned device_id)
{
    pci_vendor_device *vendor_device;

    LIST_FOREACH(vendor_device, &list, next)
    {
        if (vendor_device->id == device_id)
            return vendor_device;
    }

    return NULL;
}

static pci_device *
find_device_byno(const pci_devices list, unsigned devno)
{
    pci_device *device;

    TAILQ_FOREACH(device, &list, next)
    {
        if (device->devno == devno)
            return device;
    }

    return NULL;
}

static pci_device *
find_device_by_id(unsigned vendor_id, unsigned device_id, unsigned devno)
{
    pci_vendor *vendor = find_vendor(vendor_list, vendor_id);
    pci_vendor_device *devid;

    if (vendor == 0)
        return NULL;

    devid = find_vendor_device(vendor->vendor_devices, device_id);
    if (devid == NULL)
        return NULL;

    return find_device_byno(devid->devices, devno);
}

static void
free_vendor_list(pci_vendors list)
{
    while (!LIST_EMPTY(&list))
    {
        pci_vendor *vendor = LIST_FIRST(&list);

        LIST_REMOVE(vendor, next);
        while (!LIST_EMPTY(&vendor->vendor_devices))
        {
            pci_vendor_device *vendor_device = LIST_FIRST(&vendor->vendor_devices);

            LIST_REMOVE(vendor_device, next);

            while (!TAILQ_EMPTY(&vendor_device->devices))
            {
                TAILQ_REMOVE(&vendor_device->devices,
                             TAILQ_FIRST(&vendor_device->devices), next);
            }
            free(vendor_device);
        }
        free(vendor);
    }
}


static pci_vendors
make_vendor_list(pci_device *devs, size_t n_devs)
{
    pci_vendors vendor_list = LIST_HEAD_INITIALIZER(&vendor_list);
    unsigned i;

    for (i = 0; i < n_devs; i++)
    {
        pci_vendor *vendor = find_vendor(vendor_list, devs[i].vendor_id);
        pci_vendor_device *vendor_device;

        if (vendor == NULL)
        {
            vendor = calloc(1, sizeof(*vendor));
            if (vendor == NULL)
            {
                free_vendor_list(vendor_list);
                LIST_INIT(&vendor_list);
                return vendor_list;
            }

            vendor->id = devs[i].vendor_id;
            LIST_INIT(&vendor->vendor_devices);
            LIST_INSERT_HEAD(&vendor_list, vendor, next);
        }

        vendor_device = find_vendor_device(vendor->vendor_devices,
                                           devs[i].device_id);
        if (vendor_device == NULL)
        {
            vendor_device = calloc(1, sizeof(*vendor_device));
            if (vendor_device == NULL)
            {
                free_vendor_list(vendor_list);
                LIST_INIT(&vendor_list);
                return vendor_list;
            }

            vendor_device->id = devs[i].device_id;
            TAILQ_INIT(&vendor_device->devices);
            LIST_INSERT_HEAD(&vendor->vendor_devices,
                             vendor_device, next);
        }
        assert(devs[i].devno == 0);
        assert(TAILQ_NEXT(&devs[i], next) == NULL);
        devs[i].devno = vendor_device->next_devno++;
        TAILQ_INSERT_TAIL(&vendor_device->devices, &devs[i], next);
    }
    return vendor_list;
}

static pci_device *
find_device_by_addr(const pci_address *addr)
{
    return bsearch(addr, all_devices, n_all_devices, sizeof(*all_devices),
                   addr_compar);
}

static void
transfer_locking(pci_vendors dest, pci_vendors src)
{
    pci_vendor *src_vendor;

    LIST_FOREACH(src_vendor, &src, next)
    {
        pci_vendor *dst_vendor = find_vendor(dest, src_vendor->id);

        if (dst_vendor != NULL)
        {
            pci_vendor_device *src_vd;

            dst_vendor->lock = src_vendor->lock;

            LIST_FOREACH(src_vd, &src_vendor->vendor_devices, next)
            {
                pci_vendor_device *dst_vd = find_vendor_device(
                    dst_vendor->vendor_devices,
                    src_vd->id);

                if (dst_vd != NULL)
                {
                    pci_device *src_dev;

                    TAILQ_FOREACH(src_dev, &src_vd->devices, next)
                    {
                        pci_device *dst_dev = find_device_byno(
                            dst_vd->devices,
                            src_dev->devno);

                        if (dst_dev != NULL)
                            dst_dev->lock = src_dev->lock;
                    }

                    dst_vd->lock = src_vd->lock;
                }
                if (src_vd->lock > 0)
                {
                    pci_device *dst_dev;

                    TAILQ_FOREACH(dst_dev, &dst_vd->devices, next)
                    {
                        if (dst_dev->lock == 0)
                            dst_dev->lock = src_vd->lock;
                    }
                }
            }
        }

        if (src_vendor->lock > 0)
        {
            pci_vendor_device *dst_vd;

            LIST_FOREACH(dst_vd, &dst_vendor->vendor_devices, next)
            {
                if (dst_vd->lock == 0)
                {
                    pci_device *dst_dev;

                    dst_vd->lock = src_vendor->lock;
                    TAILQ_FOREACH(dst_dev, &dst_vd->devices, next)
                    {
                        assert(dst_dev->lock == 0);
                        dst_dev->lock = src_vendor->lock;
                    }
                }
            }
        }
    }
}


static te_errno
update_device_list(void)
{
    size_t n_devs;
    pci_device *devs = scan_pci_bus(&n_devs);
    pci_vendors vendors;

    if (devs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENODEV);

    vendors = make_vendor_list(devs, n_devs);
    if (n_devs > 0 && LIST_EMPTY(&vendors))
    {
        free(devs);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    transfer_locking(vendors, vendor_list);

    free(all_devices);
    all_devices = devs;
    n_all_devices = n_devs;

    free_vendor_list(vendor_list);
    vendor_list = vendors;

    return 0;
}

static te_errno
format_device_address(te_string *dest, const pci_address *dev)
{
    return te_string_append(dest, "%04x:%02x:%02x.%1.1o",
                            dev->domain, dev->bus, dev->slot, dev->fn);
}

static te_bool
is_device_accessible(const pci_device *dev)
{
    return global_pci_lock > 0 || dev->lock > 0;
}

static te_bool
is_vendor_device_accessible(const pci_vendor_device *devid)
{
    pci_device *dev;

    if (global_pci_lock > 0 || devid->lock > 0)
        return TRUE;

    TAILQ_FOREACH(dev, &devid->devices, next)
    {
        if (is_device_accessible(dev))
            return TRUE;
    }

    return FALSE;
}

static te_bool
is_vendor_accessible(const pci_vendor *vendor)
{
    pci_vendor_device *vd;

    if (global_pci_lock > 0 || vendor->lock > 0)
        return TRUE;

    LIST_FOREACH(vd, &vendor->vendor_devices, next)
    {
        if (is_vendor_device_accessible(vd))
            return TRUE;
    }

    return FALSE;
}


static te_errno
pci_device_list(unsigned int gid, const char *oid,
                const char *sub_id, char **list)
{
    unsigned i;
    const pci_device *iter = all_devices;
    te_string result = TE_STRING_INIT;
    te_errno rc;
    te_bool first = TRUE;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    for (i = 0; i < n_all_devices; i++, iter++)
    {
        if (is_device_accessible(iter))
        {
            if (!first)
                te_string_append(&result, " ");
            rc = format_device_address(&result, &iter->address);
            if (rc != 0)
            {
                te_string_free(&result);
                return rc;
            }
            first = FALSE;
        }
    }
    *list = result.ptr;
    return 0;
}

static unsigned
get_hex_id(const char *id)
{
    const char *end = id;
    unsigned long result = strtoul(id, (char **)&end, 16);

    if (*end != '\0' || result > UINT16_MAX)
        return 0;

    return result;
}

static unsigned
get_devno(const char *id)
{
    const char *end = id;
    unsigned long result = strtoul(id, (char **)&end, 10);

    if (*end != '\0')
        return (unsigned)(-1);

    return result;
}

static te_errno
pci_device_instance_get(unsigned int gid, const char *oid, char *value,
                        const char *unused1, const char *unused2,
                        const char *venid, const char *devid,
                        const char *inst)
{
    te_string result = TE_STRING_INIT;
    unsigned vendor_id = get_hex_id(venid);
    unsigned device_id = get_hex_id(devid);
    unsigned devno = get_devno(inst);
    te_errno rc;
    const pci_device *dev;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    if (vendor_id == 0 || device_id == 0 || devno == (unsigned)(-1))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    dev = find_device_by_id(vendor_id, device_id, devno);
    if (dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (!is_device_accessible(dev))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    rc = te_string_append(&result, "/agent:%s/hardware:/pci:/device:",
                          ta_name);
    if (rc != 0)
    {
        te_string_free(&result);
        return rc;
    }
    rc = format_device_address(&result, &dev->address);
    if (rc != 0)
    {
        te_string_free(&result);
        return rc;
    }
    strcpy(value, result.ptr);
    return 0;
}

static te_errno
pci_device_instance_list(unsigned int gid, const char *oid,
                         const char *sub_id, char **list,
                         const char *unused1, const char *unused2,
                         const char *venid, const char *devid)
{
    te_string result = TE_STRING_INIT;
    unsigned vendor_id = get_hex_id(venid);
    unsigned device_id = get_hex_id(devid);
    pci_vendor *vendor;
    pci_vendor_device *vd;
    pci_device *dev;
    te_bool first = TRUE;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);
    UNUSED(unused2);

    if (vendor_id == 0 || device_id == 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vendor = find_vendor(vendor_list, vendor_id);
    if (vendor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vd = find_vendor_device(vendor->vendor_devices, device_id);
    if (vd == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_FOREACH(dev, &vd->devices, next)
    {
        if (is_device_accessible(dev))
        {
            te_string_append(&result, "%s%u", first ? "" : " ", dev->devno);
            first = FALSE;
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
pci_vendor_device_list(unsigned int gid, const char *oid,
                       const char *sub_id, char **list,
                       const char *unused1, const char *unused2,
                       const char *venid)
{
    te_string result = TE_STRING_INIT;
    unsigned vendor_id = get_hex_id(venid);
    pci_vendor *vendor;
    pci_vendor_device *vd;
    te_bool first = TRUE;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);
    UNUSED(unused2);

    if (vendor_id == 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vendor = find_vendor(vendor_list, vendor_id);
    if (vendor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(vd, &vendor->vendor_devices, next)
    {
        if (is_vendor_device_accessible(vd))
        {
            te_string_append(&result, "%s%04x", first ? "" : " ",
                             vd->id);
            first = FALSE;
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
pci_vendor_list(unsigned int gid, const char *oid,
                const char *sub_id, char **list,
                const char *unused1, const char *unused2)
{
    te_string result = TE_STRING_INIT;
    pci_vendor *vendor;
    te_bool first = TRUE;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);
    UNUSED(unused2);

    LIST_FOREACH(vendor, &vendor_list, next)
    {
        if (is_vendor_accessible(vendor))
        {
            te_string_append(&result, "%s%04x", first ? "" : " ",
                             vendor->id);
            first = FALSE;
        }
    }

    *list = result.ptr;
    return 0;
}

static cfg_oid *
parse_pci_oid_base(const char *name)
{
    cfg_oid *oid = cfg_convert_oid_str(name);


    if (oid == NULL)
        return NULL;

    if (!oid->inst || oid->len < 4)
    {
        cfg_free_oid(oid);
        return NULL;
    }
    if (*CFG_OID_GET_INST_NAME(oid, 2) != '\0' ||
        *CFG_OID_GET_INST_NAME(oid, 3) != '\0')
    {
        cfg_free_oid(oid);
        return NULL;
    }

    return oid;
}


static te_bool
check_pci_lock(void)
{
    te_string str = TE_STRING_INIT;
    te_bool result;

    te_string_append(&str, "/agent:%s/hardware:/pci:", ta_name);
    result = (rcf_pch_rsrc_check_locks(str.ptr) == 0);
    te_string_free(&str);

    return result;
}

static te_bool
check_device_lock(const pci_device *dev)
{
    te_string str = TE_STRING_INIT;
    te_bool result;

    te_string_append(&str,
                     "/agent:%s/hardware:/pci:/vendor:%04x/device:%04x/instance:%u",
                     ta_name, dev->vendor_id, dev->device_id, dev->devno);
    result = (rcf_pch_rsrc_check_locks(str.ptr) == 0);
    te_string_free(&str);

    return result;

}


static te_bool
check_vd_lock(const pci_vendor *vendor, const pci_vendor_device *vd,
              te_bool recursive)
{
    te_string str = TE_STRING_INIT;
    te_bool result;

    te_string_append(&str, "/agent:%s/hardware:/pci:/vendor:%04x/device:%04x",
                     ta_name, vendor->id, vd->id);
    result = (rcf_pch_rsrc_check_locks(str.ptr) == 0);
    te_string_free(&str);

    if (result && recursive)
    {
        pci_device *dev;

        TAILQ_FOREACH(dev, &vd->devices, next)
        {
            if (!check_device_lock(dev))
                return FALSE;
        }
    }

    return result;

}

static te_bool
check_vendor_lock(const pci_vendor *vendor, te_bool recursive)
{
    te_string str = TE_STRING_INIT;
    te_bool result;

    te_string_append(&str, "/agent:%s/hardware:/pci:/vendor:%04x", ta_name,
                     vendor->id);
    result = (rcf_pch_rsrc_check_locks(str.ptr) == 0);
    te_string_free(&str);

    if (result && recursive)
    {
        pci_vendor_device *vd;

        LIST_FOREACH(vd, &vendor->vendor_devices, next)
        {
            if (!check_vd_lock(vendor, vd, TRUE))
                return FALSE;
        }
    }

    return result;
}


static te_errno
pci_grab(const char *name)
{
    cfg_oid *oid = parse_pci_oid_base(name);

    if (oid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    cfg_free_oid(oid);

    if (global_pci_lock == 0)
    {
        pci_vendor *vendor;

        LIST_FOREACH(vendor, &vendor_list, next)
        {
            if (!check_vendor_lock(vendor, TRUE))
            {
                return TE_RC(TE_TA_UNIX, TE_EPERM);
            }
        }
    }

    global_pci_lock++;
    return 0;
}

static te_errno
pci_release(const char *name)
{
    cfg_oid *oid = parse_pci_oid_base(name);

    if (oid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    cfg_free_oid(oid);

    if (global_pci_lock == 0)
        return TE_RC(TE_TA_UNIX, TE_EPROTO);

    global_pci_lock--;
    return 0;
}


static te_errno
parse_pci_oid(const char *name, pci_vendor **vendor, pci_vendor_device **vd,
              pci_device **dev)
{
    cfg_oid *oid = parse_pci_oid_base(name);
    unsigned vendor_id = get_hex_id(CFG_OID_GET_INST_NAME(oid, 4));

    if (oid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vendor_id = get_hex_id(CFG_OID_GET_INST_NAME(oid, 4));
    if (vendor_id == 0)
    {
        cfg_free_oid(oid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    *vendor = find_vendor(vendor_list, vendor_id);
    if (*vendor == NULL)
    {
        cfg_free_oid(oid);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (vd != NULL)
    {
        unsigned device_id;

        assert(oid->len > 5);

        device_id = get_hex_id(CFG_OID_GET_INST_NAME(oid, 5));
        if (device_id == 0)
        {
            cfg_free_oid(oid);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        *vd = find_vendor_device((*vendor)->vendor_devices, device_id);
        if (*vd == NULL)
        {
            cfg_free_oid(oid);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (dev != NULL)
        {
            unsigned devno;

            assert(oid->len > 6);

            devno = get_devno(CFG_OID_GET_INST_NAME(oid, 6));
            if (devno == (unsigned)(-1))
            {
                cfg_free_oid(oid);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }

            *dev = find_device_byno((*vd)->devices, devno);
            if (*dev == NULL)
            {
                cfg_free_oid(oid);
                return TE_RC(TE_TA_UNIX, TE_ENOENT);
            }
        }
    }
    cfg_free_oid(oid);
    return 0;
}

static void
lock_vendor_device(pci_vendor_device *vd, te_bool lock)
{
    pci_device *dev;

    if (lock)
        vd->lock++;
    else
        vd->lock--;

    TAILQ_FOREACH(dev, &vd->devices, next)
    {
        if (lock)
            dev->lock++;
        else
            dev->lock--;
    }
}


static void
lock_vendor(pci_vendor *vendor, te_bool lock)
{
    pci_vendor_device *vd;

    if (lock)
        vendor->lock++;
    else
        vendor->lock--;

    LIST_FOREACH(vd, &vendor->vendor_devices, next)
    {
        lock_vendor_device(vd, lock);
    }
}

static te_errno
pci_vendor_grab(const char *name)
{
    te_errno rc;
    pci_vendor *vendor;

    rc = parse_pci_oid(name, &vendor, NULL, NULL);
    if (rc != 0)
        return rc;

    if (global_pci_lock == 0 && vendor->lock == 0)
    {
        pci_vendor_device *vd;

        if (!check_pci_lock())
            return TE_RC(TE_TA_UNIX, TE_EPERM);

        LIST_FOREACH(vd, &vendor->vendor_devices, next)
        {
            if (!check_vd_lock(vendor, vd, TRUE))
            {
                return TE_RC(TE_TA_UNIX, TE_EPERM);
            }
        }
    }

    lock_vendor(vendor, TRUE);
    return 0;
}

static te_errno
pci_vendor_release(const char *name)
{
    te_errno rc;
    pci_vendor *vendor;

    rc = parse_pci_oid(name, &vendor, NULL, NULL);
    if (rc != 0)
        return rc;

    if (vendor->lock == 0)
        return TE_RC(TE_TA_UNIX, TE_EPROTO);
    lock_vendor(vendor, FALSE);

    return 0;
}

static te_errno
pci_vendor_device_grab(const char *name)
{
    te_errno rc;
    pci_vendor *vendor;
    pci_vendor_device *vd;

    rc = parse_pci_oid(name, &vendor, &vd, NULL);
    if (rc != 0)
        return rc;

    if (global_pci_lock == 0 && vd->lock == 0)
    {
        pci_device *dev;

        if (!check_pci_lock() || !check_vendor_lock(vendor, FALSE))
            return TE_RC(TE_TA_UNIX, TE_EPERM);

        TAILQ_FOREACH(dev, &vd->devices, next)
        {
            if (!check_device_lock(dev))
            {
                return TE_RC(TE_TA_UNIX, TE_EPERM);
            }
        }
    }

    lock_vendor_device(vd, TRUE);
    return 0;
}

static te_errno
pci_vendor_device_release(const char *name)
{
    te_errno rc;
    pci_vendor *vendor;
    pci_vendor_device *vd;

    rc = parse_pci_oid(name, &vendor, &vd, NULL);
    if (rc != 0)
        return rc;

    if (vd->lock == 0)
        return TE_RC(TE_TA_UNIX, TE_EPROTO);

    lock_vendor_device(vd, FALSE);

    return 0;
}

static te_errno
pci_device_grab(const char *name)
{
    te_errno rc;
    pci_vendor *vendor;
    pci_vendor_device *vd;
    pci_device *dev;

    rc = parse_pci_oid(name, &vendor, &vd, &dev);
    if (rc != 0)
        return rc;

    if (global_pci_lock == 0 && dev->lock == 0)
    {
        if (!check_pci_lock() || !check_vendor_lock(vendor, FALSE) ||
            !check_vd_lock(vendor, vd, FALSE))
        {
            return TE_RC(TE_TA_UNIX, TE_EPERM);
        }
    }

    dev->lock++;
    return 0;
}

static te_errno
pci_device_release(const char *name)
{
    te_errno rc;
    pci_vendor *vendor;
    pci_vendor_device *vd;
    pci_device *dev;

    rc = parse_pci_oid(name, &vendor, &vd, &dev);
    if (rc != 0)
        return rc;

    if (dev->lock == 0)
        return TE_RC(TE_TA_UNIX, TE_EPROTO);
    dev->lock--;

    return 0;
}

RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_device_instance, "instance",
                               NULL, NULL,
                               pci_device_instance_get,
                               pci_device_instance_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_vendor_device, "device",
                               &node_pci_device_instance, NULL,
                               NULL, pci_vendor_device_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_vendor, "vendor",
                               &node_pci_vendor_device, NULL,
                               NULL, pci_vendor_list);



static te_errno
find_device_by_addr_str(const char *addr_str, pci_device **devp)
{
    pci_address addr;
    te_errno rc;

    rc = parse_pci_address(addr_str, &addr);
    if (rc != 0)
        return rc;

    *devp = find_device_by_addr(&addr);
    if (*devp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (!is_device_accessible(*devp))
    {
        ERROR("%s is not ours", addr_str);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    return 0;
}

static te_errno
format_sysfs_device_name(te_string *dest, const pci_device *dev,
                         const char *suffix)
{
    te_errno rc;

    rc = te_string_append(dest, SYSFS_PCI_DEVICES_TREE "/");
    if (rc != 0)
        return rc;

    rc = format_device_address(dest, &dev->address);
    if (rc != 0)
        return rc;

    if (suffix != NULL)
    {
        rc = te_string_append(dest, "%s", suffix);
        if (rc != 0)
            return rc;
    }

    return 0;
}


static te_errno
get_driver_name(const pci_device *dev, char *name, size_t namesize)
{
    te_errno rc;
    te_string buf = TE_STRING_INIT;
    char link[PATH_MAX];
    char *base;

    rc = format_sysfs_device_name(&buf, dev, "/driver");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    if (readlink(buf.ptr, link, sizeof(link)) < 0)
    {
        int rc = errno;
        te_string_free(&buf);

        /* The device is not bound to any driver, not an error */
        if (rc == ENOENT)
        {
            *name = '\0';
            return 0;
        }
        te_string_free(&buf);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    base = strrchr(link, '/');
    if (base == NULL)
        base = link;
    else
        base++;

    strncpy(name, base, namesize - 1);
    return 0;
}

static te_errno
pci_driver_get(unsigned int gid, const char *oid, char *value,
               const char *unused1, const char *unused2,
               const char *addr_str)
{
    const pci_device *dev;
    te_errno rc;


    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    return get_driver_name(dev, value, RCF_MAX_VAL);
}

static te_errno
unbind_pci_device(const pci_device *dev)
{
    te_errno rc;
    te_string buf = TE_STRING_INIT;
    int fd;

    rc = format_sysfs_device_name(&buf, dev, "/driver/unbind");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }
    fd = open(buf.ptr, O_WRONLY);
    if (fd < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        te_string_free(&buf);
        return rc;
    }
    te_string_cut(&buf, buf.len);
    rc = format_device_address(&buf, &dev->address);
    if (rc != 0)
    {
        close(fd);
        te_string_free(&buf);
        return rc;
    }

    if (write(fd, buf.ptr, buf.len) < 0)
        rc = TE_OS_RC(TE_TA_UNIX, errno);
    close(fd);
    te_string_free(&buf);
    return rc;
}

static te_errno
maybe_load_driver(const char *drvname)
{
    te_errno rc;
    int status;
    te_string buf = TE_STRING_INIT;

    rc = te_string_append(&buf, "/sys/bus/pci/drivers/%s", drvname);
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    if (access(buf.ptr, F_OK) == 0)
    {
        te_string_free(&buf);
        return 0;
    }
    if (errno != ENOENT)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        te_string_free(&buf);
        return rc;
    }

    RING("PCI driver '%s' not found, trying to load module", drvname);

    te_string_cut(&buf, buf.len);
    rc = te_string_append(&buf, "/sbin/modprobe %s", drvname);
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }
    status = ta_system(buf.ptr);
    if (status < 0)
        rc = TE_OS_RC(TE_TA_UNIX, errno);
    else if (status != 0)
    {
        ERROR("'%s' returned %d", buf.ptr, status);
        rc = TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    te_string_free(&buf);
    return rc;
}

/* Driver binding in Linux is a rather delicate matter.
 * For a device to be bound to a generic driver like vfio-pci,
 * its vendor and device should be made known to the driver by writing
 * to new_id. In theory, that means that when the device is unbound, its IDs
 * should be removed via writing to remove_id.
 * However, we have no reliable way to know whether the device was not
 * already known to the driver and so writing to remove_id for non-generic
 * driver like a network driver may have adverse effects.
 * On the other hand, e.g. vfio-pci seems to ignore remove_id completely.
 * Another issue is that writing a new vendor/device IDs to new_id causes
 * the driver to probe all the devices with these IDs and bind them if they
 * are not bound to some other driver (however, if the device is already
 * known, the explicit bind is still necessary).
 * Writing 0 to /sys/bus/pci/drivers/drivers_autoprobe should disable such
 * behaviour, but vfio-pci seems to ignore this.
 */

static te_errno
let_driver_know_pci_device(const pci_device *dev, const char *drvname)
{
    te_errno rc;
    te_string buf = TE_STRING_INIT;
    int fd;

    rc = te_string_append(&buf, "/sys/bus/pci/drivers/%s/new_id", drvname);
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }
    fd = open(buf.ptr, O_WRONLY);
    if (fd < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        te_string_free(&buf);
        return rc;
    }
    te_string_cut(&buf, buf.len);
    rc = te_string_append(&buf, "%04x %04x", dev->vendor_id,
                          dev->device_id);
    if (rc != 0)
    {
        close(fd);
        te_string_free(&buf);
        return rc;
    }

    if (write(fd, buf.ptr, buf.len) < 0)
        rc = TE_OS_RC(TE_TA_UNIX, errno);
    close(fd);
    te_string_free(&buf);
    return rc;
}

static te_errno
bind_pci_device(const pci_device *dev, const char *drvname)
{
    te_errno rc;
    te_string buf = TE_STRING_INIT;
    int fd;

    rc = te_string_append(&buf, "/sys/bus/pci/drivers/%s/bind", drvname);
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }
    fd = open(buf.ptr, O_WRONLY);
    if (fd < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        te_string_free(&buf);
        return rc;
    }
    te_string_cut(&buf, buf.len);
    rc = format_device_address(&buf, &dev->address);
    if (rc != 0)
    {
        close(fd);
        te_string_free(&buf);
        return rc;
    }

    if (write(fd, buf.ptr, buf.len) < 0)
    {
        /* For some reason, writing to bind just after
         * writing to new_id may result in ENODEV being reported,
         * but the binding is actually successful, which can be
         * verified by checking the link to device inside the driver
         * directory
         */
        if (errno != ENODEV)
            rc = TE_OS_RC(TE_TA_UNIX, errno);
        else
        {
            te_string_cut(&buf, buf.len);
            rc = te_string_append(&buf, "/sys/bus/pci/drivers/%s/",
                                  drvname);
            if (rc == 0)
            {
                rc = format_device_address(&buf, &dev->address);
                if (rc == 0 && access(buf.ptr, F_OK) != 0)
                    rc = TE_RC(TE_TA_UNIX, TE_ENODEV);
            }
        }
    }
    close(fd);
    te_string_free(&buf);
    return rc;
}


static te_errno
pci_driver_set(unsigned int gid, const char *oid, const char *value,
               const char *unused1, const char *unused2,
               const char *addr_str)
{
    const pci_device *dev;
    te_errno rc;
    char driver_name[PATH_MAX];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;
    rc = get_driver_name(dev, driver_name, sizeof(driver_name));
    if (rc != 0)
        return rc;

    if (strcmp(driver_name, value) == 0)
        return 0;

    if (*driver_name != '\0')
    {
        rc = unbind_pci_device(dev);
        if (rc != 0)
            return rc;
    }

    if (*value != '\0')
    {
        rc = maybe_load_driver(value);
        if (rc != 0)
            return rc;
        rc = let_driver_know_pci_device(dev, value);
        if (rc != 0)
            return rc;
        rc = bind_pci_device(dev, value);
        if (rc != 0)
            return rc;
    }

    return 0;
}

static te_errno
pci_net_list(unsigned int gid, const char *oid, const char *sub_id,
             char **list, const char *unused1, const char *unused2,
             const char *addr_str)
{
    te_string buf = TE_STRING_INIT;
    const pci_device *dev;
    char *result = NULL;
    te_errno rc;

    UNUSED(sub_id);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    rc = format_sysfs_device_name(&buf, dev, "/net");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    result = calloc(1, RCF_MAX_VAL);
    if (result == NULL)
    {
        te_string_free(&buf);
        ERROR("Out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    rc = get_dir_list(buf.ptr, result, RCF_MAX_VAL, TRUE, NULL, NULL);
    te_string_free(&buf);
    if (rc != 0)
    {
        free(result);
        return rc;
    }

    *list = result;
    return rc;
}

RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_net, "net",
                               NULL, NULL,
                               NULL, pci_net_list);
RCF_PCH_CFG_NODE_RW(node_pci_driver, "driver",
                    NULL, &node_pci_net,
                    pci_driver_get, pci_driver_set);


#define PCI_ID_NODE_RO(_name, _field, _fmt, _sibling)                   \
    static te_errno                                                     \
    pci_##_name##_get(unsigned int gid, const char *oid, char *value,   \
                      const char *unused1, const char *unused2,         \
                      const char *addr_str)                             \
    {                                                                   \
        const pci_device *dev;                                          \
        te_errno rc;                                                    \
                                                                        \
        UNUSED(gid);                                                    \
        UNUSED(oid);                                                    \
        UNUSED(unused1);                                                \
        UNUSED(unused2);                                                \
                                                                        \
        rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);    \
        if (rc != 0)                                                    \
            return rc;                                                  \
                                                                        \
        sprintf(value, _fmt, dev->_field);                              \
        return 0;                                                       \
    }                                                                   \
                                                                        \
    RCF_PCH_CFG_NODE_RO(node_pci_##_name, #_name, NULL,                 \
                        (_sibling),                                     \
                        pci_##_name##_get)

PCI_ID_NODE_RO(class, device_class, "%08x", &node_pci_driver);
PCI_ID_NODE_RO(subsystem_device, subsystem_device, "%04x", &node_pci_class);
PCI_ID_NODE_RO(subsystem_vendor, subsystem_vendor, "%04x",
               &node_pci_subsystem_device);
PCI_ID_NODE_RO(device_id, device_id, "%04x",
               &node_pci_subsystem_vendor);
PCI_ID_NODE_RO(vendor_id, vendor_id, "%04x",
               &node_pci_device_id);
PCI_ID_NODE_RO(fn, address.fn, "%u",
               &node_pci_vendor_id);
PCI_ID_NODE_RO(slot, address.slot, "%02x",
               &node_pci_fn);
PCI_ID_NODE_RO(bus, address.bus, "%02x",
               &node_pci_slot);
PCI_ID_NODE_RO(domain, address.domain, "%04x",
               &node_pci_bus);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_device, "device",
                               &node_pci_domain, &node_pci_vendor,
                               NULL, pci_device_list);

RCF_PCH_CFG_NODE_NA(node_pci, "pci", &node_pci_device, NULL);


te_errno
ta_unix_conf_pci_init()
{
    te_errno rc = update_device_list();

    if (rc != 0)
        return rc;

    rcf_pch_rsrc_info("/agent/hardware/pci",
                      pci_grab,
                      pci_release);

    rcf_pch_rsrc_info("/agent/hardware/pci/vendor",
                      pci_vendor_grab,
                      pci_vendor_release);

    rcf_pch_rsrc_info("/agent/hardware/pci/vendor/device",
                      pci_vendor_device_grab,
                      pci_vendor_device_release);

    rcf_pch_rsrc_info("/agent/hardware/pci/vendor/device/instance",
                      pci_device_grab,
                      pci_device_release);

    return rcf_pch_add_node("/agent/hardware", &node_pci);
}
