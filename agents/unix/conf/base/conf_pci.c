/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief PCI support
 *
 * PCI configuration tree support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */


#define TE_LGR_USER     "Conf PCI"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if HAVE_SEARCH_H
#include <search.h>
#endif

#include <limits.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_string.h"
#include "te_str.h"
#include "te_file.h"
#include "te_vector.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "conf_common.h"
#include "te_alloc.h"
#include "te_pci.h"

#ifdef USE_LIBNETCONF
#include "netconf.h"
#endif

#define PCI_VIRTFN_PREFIX "virtfn"

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
    unsigned int devno;           /**< Device instance number
                                   *   among all the devices
                                   *   with same vendor/device ID
                                   */
    uint16_t subsystem_vendor;    /**< Subsystem vendor ID */
    uint16_t subsystem_device;    /**< Subsystem device ID */
    uint32_t device_class;        /**< PCI device class */
    unsigned int lock;            /**< TE resource lock counter */
    char *net_list;               /**< Space separated list of network
                                       interfaces */
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
    unsigned int id;                      /** Device ID */
    unsigned int next_devno;              /** Next instance number for a PCI
                                           *   device
                                           */
    unsigned int lock;                    /**< TE resource lock counter */
    pci_devices devices;                  /**< List of device instances */
} pci_vendor_device;

/** List of device sets with the same vendor/device ID */
typedef LIST_HEAD(pci_vendor_devices, pci_vendor_device)
    pci_vendor_devices;

/** A set of devices with the same vendor ID */
typedef struct pci_vendor {
    LIST_ENTRY(pci_vendor) next;         /**< Next vendor ID */
    unsigned int id;                     /**< Vendor ID */
    unsigned int lock;                   /**< TE resource lock counter */
    pci_vendor_devices vendor_devices;   /**< List of device IDs */
} pci_vendor;

/** List of same vendor ID device sets */
typedef LIST_HEAD(pci_vendors, pci_vendor) pci_vendors;

/** Number of all PCI devices in the system */
static size_t      n_all_devices = 0;

/** An array of all PCI devices in the system */
static pci_device *all_devices = NULL;

/** The list of all vendor IDs in the system */
static pci_vendors *vendor_list;

/** Whole PCI tree TE resource lock */
static unsigned int global_pci_lock;

#ifdef USE_LIBNETCONF

/*
 * Netconf session used to work with devlink over
 * Generic Netlink API
 */
static netconf_handle nh_genl = NULL;
/* Cache of device parameters */
static netconf_list *dev_params = NULL;
/* Group ID for which the cache of parameters was obtained */
static unsigned int dev_params_gid = 0;

/* Update device parameters cache */
static te_errno
update_dev_params(unsigned int gid)
{
    te_errno rc = 0;

    if (dev_params != NULL)
    {
        if (gid != dev_params_gid)
        {
            netconf_list_free(dev_params);
            dev_params = NULL;
        }
    }

    if (dev_params == NULL)
    {
        rc = netconf_devlink_param_dump(nh_genl, &dev_params);
        if (rc == TE_ENODEV)
            rc = TE_ENOENT;
    }

    if (rc == 0)
        dev_params_gid = gid;

    return rc;
}

/* Retrieve device parameter by PCI address and parameter name */
static te_errno
find_dev_param(unsigned int gid, const char *pci_addr,
               const char *param_name,
               netconf_devlink_param **param_out)
{
    te_errno rc;
    netconf_node *node;
    netconf_devlink_param *param;

    rc = update_dev_params(gid);
    if (rc != 0)
        return rc;

    for (node = dev_params->head; node != NULL; node = node->next)
    {
        param = &node->data.devlink_param;
        if (strcmp(param->bus_name, "pci") == 0 &&
            strcmp(param->dev_name, pci_addr) == 0 &&
            strcmp(param->name, param_name) == 0)
        {
            *param_out = param;
            return 0;
        }
    }

    return TE_ENOENT;
}

#endif

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

    te_string_append(&buf, SYSFS_PCI_DEVICES_TREE "/%s/%s", name, attr);

    *result = fopen(buf.ptr, "r");
    if (*result == NULL)
        rc = TE_OS_RC(TE_TA_UNIX, errno);
    te_string_free(&buf);
    return rc;
}

static te_errno
read_pci_hex_attr(const char *name, const char *attr, unsigned int *value)
{
    FILE *f;
    te_errno rc = open_pci_attr(name, attr, &f);
    unsigned int result = 0;

    if (rc != 0)
    {
        WARN("Cannot open '%s' for PCI device '%s', rc=%x",
             attr, name, rc);
        return rc;
    }
    if (fscanf(f, "%x", &result) != 1)
    {
        ERROR("Cannot parse PCI '%s' hex attribute '%s' value: %s",
              name, attr, strerror(errno));
        fclose(f);
        return TE_RC(TE_TA_UNIX, te_rc_os2te(errno));
    }
    fclose(f);
    *value = result;
    return 0;
}

static te_errno
read_pci_int_attr(const char *name, const char *attr, int *result)
{
    FILE *f;
    te_errno rc = open_pci_attr(name, attr, &f);

    if (rc != 0)
        return rc;

    if (fscanf(f, "%d", result) != 1)
    {
        WARN("Cannot parse PCI '%s' decimal attribute '%s' value: %s",
              name, attr, strerror(errno));
        fclose(f);
        return TE_EIO;
    }

    fclose(f);

    return 0;
}

static bool
populate_pci_device(pci_device *dst, const char *name)
{
    te_errno rc;
    unsigned int value;

    rc = parse_pci_address(name, &dst->address);
    if (rc != 0)
    {
        ERROR("Error parsing PCI device name '%s', rc = %d", name, rc);
        return false;
    }

    rc = read_pci_hex_attr(name, "vendor", &value);
    if (rc != 0)
    {
        ERROR("Unknown vendor ID for '%s'", name);
        return false;
    }
    dst->vendor_id = value;

    rc = read_pci_hex_attr(name, "device", &value);
    if (rc != 0)
    {
        ERROR("Unknown device ID for '%s'", name);
        return false;
    }
    dst->device_id = value;

    rc = read_pci_hex_attr(name, "subsystem_vendor", &value);
    dst->subsystem_vendor = rc == 0 ? value : 0;
    rc = read_pci_hex_attr(name, "subsystem_device", &value);
    dst->subsystem_device = rc == 0 ? value : 0;
    rc = read_pci_hex_attr(name, "class", &value);
    dst->device_class = rc == 0 ? value : 0;

    return true;
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

    result = TE_ALLOC(rc * sizeof(*result));

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
find_vendor(const pci_vendors *list, unsigned int vendor_id)
{
    pci_vendor *vendor;

    LIST_FOREACH(vendor, list, next)
    {
        if (vendor->id == vendor_id)
            return vendor;
    }

    return NULL;
}

static pci_vendor_device *
find_vendor_device(const pci_vendor_devices list, unsigned int device_id)
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
find_device_byno(const pci_devices list, unsigned int devno)
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
find_device_by_id(unsigned int vendor_id, unsigned int device_id,
                  unsigned int devno)
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
free_vendor_list(pci_vendors *list)
{
    while (!LIST_EMPTY(list))
    {
        pci_vendor *vendor = LIST_FIRST(list);

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
    free(list);
}


static pci_vendors *
make_vendor_list(pci_device *devs, size_t n_devs)
{
    pci_vendors *vendor_list;
    unsigned int i;

    vendor_list = TE_ALLOC(sizeof(*vendor_list));
    LIST_INIT(vendor_list);

    for (i = 0; i < n_devs; i++)
    {
        pci_vendor *vendor = find_vendor(vendor_list, devs[i].vendor_id);
        pci_vendor_device *vendor_device;

        if (vendor == NULL)
        {
            vendor = TE_ALLOC(sizeof(*vendor));

            vendor->id = devs[i].vendor_id;
            LIST_INIT(&vendor->vendor_devices);
            LIST_INSERT_HEAD(vendor_list, vendor, next);
        }

        vendor_device = find_vendor_device(vendor->vendor_devices,
                                           devs[i].device_id);
        if (vendor_device == NULL)
        {
            vendor_device = TE_ALLOC(sizeof(*vendor_device));

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
transfer_locking(pci_vendors *dest, pci_vendors *src)
{
    pci_vendor *src_vendor;

    LIST_FOREACH(src_vendor, src, next)
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

/* Release memory allocated by list of devices */
static void
free_device_list(void)
{
    size_t i;

    if (all_devices == NULL)
        return;

    for (i = 0; i < n_all_devices; i++)
        free(all_devices[i].net_list);

    free(all_devices);
    all_devices = NULL;
    n_all_devices = 0;
}

static te_errno
update_device_list(void)
{
    size_t n_devs;
    pci_device *devs = scan_pci_bus(&n_devs);
    pci_vendors *vendors;

    if (devs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENODEV);

    vendors = make_vendor_list(devs, n_devs);
    if (n_devs > 0 && (vendors == NULL || LIST_EMPTY(vendors)))
    {
        free(devs);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if (vendor_list != NULL) {
        transfer_locking(vendors, vendor_list);
        free_vendor_list(vendor_list);
    }

    vendor_list = vendors;

    free_device_list();

    all_devices = devs;
    n_all_devices = n_devs;

    return 0;
}

static te_errno
format_device_address(te_string *dest, const pci_address *dev)
{
    te_string_append(dest, "%04x:%02x:%02x.%1.1o",
                     dev->domain, dev->bus, dev->slot, dev->fn);
    return 0;
}

static bool
is_device_accessible(const pci_device *dev)
{
    return global_pci_lock > 0 || dev->lock > 0;
}

static bool
is_vendor_device_accessible(const pci_vendor_device *devid)
{
    pci_device *dev;

    if (global_pci_lock > 0 || devid->lock > 0)
        return true;

    TAILQ_FOREACH(dev, &devid->devices, next)
    {
        if (is_device_accessible(dev))
            return true;
    }

    return false;
}

static bool
is_vendor_accessible(const pci_vendor *vendor)
{
    pci_vendor_device *vd;

    if (global_pci_lock > 0 || vendor->lock > 0)
        return true;

    LIST_FOREACH(vd, &vendor->vendor_devices, next)
    {
        if (is_vendor_device_accessible(vd))
            return true;
    }

    return false;
}


static te_errno
pci_device_list(const ta_conf_ctx *ctx, te_vec *names)
{
    unsigned int i;
    const pci_device *iter = all_devices;
    te_errno rc;

    UNUSED(ctx);

    for (i = 0; i < n_all_devices; i++, iter++)
    {
        te_string addr = TE_STRING_INIT;

        if (!is_device_accessible(iter))
            continue;

        rc = format_device_address(&addr, &iter->address);
        if (rc != 0)
        {
            te_string_free(&addr);
            return rc;
        }
        TE_VEC_APPEND(names, addr.ptr);
    }

    return 0;
}

static unsigned int
get_hex_id(const char *id)
{
    const char *end = id;
    unsigned long result = strtoul(id, (char **)&end, 16);

    if (*end != '\0' || result > UINT16_MAX)
        return 0;

    return result;
}

static unsigned int
get_devno(const char *id)
{
    const char *end = id;
    unsigned long result = strtoul(id, (char **)&end, 10);

    if (*end != '\0')
        return (unsigned int)(-1);

    return result;
}

static te_errno
pci_device_instance_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *venid = ta_conf_ctx_inst(ctx, "vendor");
    const char *devid = ta_conf_ctx_inst(ctx, "device");
    const char *inst = ta_conf_ctx_inst(ctx, "instance");
    unsigned int vendor_id = get_hex_id(venid);
    unsigned int device_id = get_hex_id(devid);
    unsigned int devno = get_devno(inst);
    te_errno rc;
    const pci_device *dev;

    if (vendor_id == 0 || device_id == 0 || devno == (unsigned int)(-1))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    dev = find_device_by_id(vendor_id, device_id, devno);
    if (dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (!is_device_accessible(dev))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    rc = te_string_append_chk(val, "/agent:%s/hardware:/pci:/device:",
                              ta_name);
    if (rc != 0)
        return rc;

    return format_device_address(val, &dev->address);
}

static te_errno
pci_device_instance_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *venid = ta_conf_ctx_inst(ctx, "vendor");
    const char *devid = ta_conf_ctx_inst(ctx, "device");
    unsigned int vendor_id = get_hex_id(venid);
    unsigned int device_id = get_hex_id(devid);
    pci_vendor *vendor;
    pci_vendor_device *vd;
    pci_device *dev;

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
        if (!is_device_accessible(dev))
            continue;

        char *name = te_string_fmt("%u", dev->devno);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
pci_vendor_device_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *venid = ta_conf_ctx_inst(ctx, "vendor");
    unsigned int vendor_id = get_hex_id(venid);
    pci_vendor *vendor;
    pci_vendor_device *vd;

    if (vendor_id == 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vendor = find_vendor(vendor_list, vendor_id);
    if (vendor == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(vd, &vendor->vendor_devices, next)
    {
        if (!is_vendor_device_accessible(vd))
            continue;

        char *name = te_string_fmt("%04x", vd->id);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
pci_vendor_list(const ta_conf_ctx *ctx, te_vec *names)
{
    pci_vendor *vendor;

    UNUSED(ctx);

    LIST_FOREACH(vendor, vendor_list, next)
    {
        if (!is_vendor_accessible(vendor))
            continue;

        char *name = te_string_fmt("%04x", vendor->id);

        TE_VEC_APPEND(names, name);
    }

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


static bool
check_pci_lock(void)
{
    te_string str = TE_STRING_INIT;
    bool result;

    te_string_append(&str, "/agent:%s/hardware:/pci:", ta_name);
    result = (rcf_pch_rsrc_check_locks(str.ptr) == 0);
    te_string_free(&str);

    return result;
}

static bool
check_device_lock(const pci_device *dev)
{
    te_string str = TE_STRING_INIT;
    bool result;

    te_string_append(&str,
                     "/agent:%s/hardware:/pci:/vendor:%04x/device:%04x/instance:%u",
                     ta_name, dev->vendor_id, dev->device_id, dev->devno);
    result = (rcf_pch_rsrc_check_locks(str.ptr) == 0);
    te_string_free(&str);

    return result;

}


static bool
check_vd_lock(const pci_vendor *vendor, const pci_vendor_device *vd,
              bool recursive)
{
    te_string str = TE_STRING_INIT;
    bool result;

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
                return false;
        }
    }

    return result;

}

static bool
check_vendor_lock(const pci_vendor *vendor, bool recursive)
{
    te_string str = TE_STRING_INIT;
    bool result;

    te_string_append(&str, "/agent:%s/hardware:/pci:/vendor:%04x", ta_name,
                     vendor->id);
    result = (rcf_pch_rsrc_check_locks(str.ptr) == 0);
    te_string_free(&str);

    if (result && recursive)
    {
        pci_vendor_device *vd;

        LIST_FOREACH(vd, &vendor->vendor_devices, next)
        {
            if (!check_vd_lock(vendor, vd, true))
                return false;
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

        LIST_FOREACH(vendor, vendor_list, next)
        {
            if (!check_vendor_lock(vendor, true))
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
    unsigned int vendor_id = get_hex_id(CFG_OID_GET_INST_NAME(oid, 4));

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
        unsigned int device_id;

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
            unsigned int devno;

            assert(oid->len > 6);

            devno = get_devno(CFG_OID_GET_INST_NAME(oid, 6));
            if (devno == (unsigned int)(-1))
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
lock_vendor_device(pci_vendor_device *vd, bool lock)
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
lock_vendor(pci_vendor *vendor, bool lock)
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
            if (!check_vd_lock(vendor, vd, true))
            {
                return TE_RC(TE_TA_UNIX, TE_EPERM);
            }
        }
    }

    lock_vendor(vendor, true);
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
    lock_vendor(vendor, false);

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

        if (!check_pci_lock() || !check_vendor_lock(vendor, false))
            return TE_RC(TE_TA_UNIX, TE_EPERM);

        TAILQ_FOREACH(dev, &vd->devices, next)
        {
            if (!check_device_lock(dev))
            {
                return TE_RC(TE_TA_UNIX, TE_EPERM);
            }
        }
    }

    lock_vendor_device(vd, true);
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

    lock_vendor_device(vd, false);

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
        if (!check_pci_lock() || !check_vendor_lock(vendor, false) ||
            !check_vd_lock(vendor, vd, false))
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

static te_errno
find_device_by_addr_str_ignore_permission(const char *addr_str,
                                          pci_device **devp)
{
    pci_address addr;
    te_errno rc;

    rc = parse_pci_address(addr_str, &addr);
    if (rc != 0)
        return rc;

    *devp = find_device_by_addr(&addr);
    if (*devp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}

static te_errno
find_device_by_addr_str(const char *addr_str, pci_device **devp)
{
    te_errno rc;

    rc = find_device_by_addr_str_ignore_permission(addr_str, devp);
    if (rc != 0)
        return rc;

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

    te_string_append(dest, SYSFS_PCI_DEVICES_TREE "/");

    rc = format_device_address(dest, &dev->address);
    if (rc != 0)
        return rc;

    if (suffix != NULL)
        te_string_append(dest, "%s", suffix);

    return 0;
}


static te_errno
get_driver_name(const pci_device *dev, char *name, size_t namesize)
{
    te_errno rc;
    te_string buf = TE_STRING_INIT;
    char link[PATH_MAX] = "";
    char *base;

    rc = format_sysfs_device_name(&buf, dev, "/driver");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    if (readlink(buf.ptr, link, sizeof(link) - 1) < 0)
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
    te_string_free(&buf);

    base = strrchr(link, '/');
    if (base == NULL)
        base = link;
    else
        base++;

    te_strlcpy(name, base, namesize);
    return 0;
}

static te_errno
pci_driver_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const pci_device *dev;
    char name[RCF_MAX_VAL];
    te_errno rc;

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    rc = get_driver_name(dev, name, sizeof(name));
    if (rc != 0)
        return rc;

    return te_string_append_chk(val, "%s", name);
}

static te_errno
pci_current_num_vfs_fopen(const pci_device *dev, const char *flags, FILE **fd)
{
    te_errno rc;
    te_string buf = TE_STRING_INIT;

    rc = format_sysfs_device_name(&buf, dev, "/max_vfs");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }
    *fd = fopen(buf.ptr, flags);
    te_string_free(&buf);
    if (*fd != NULL)
        return 0;

    /* Error occurred, try another sysfs path for current VFs number */
    te_string_reset(&buf);
    rc = format_sysfs_device_name(&buf, dev, "/sriov_numvfs");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    *fd = fopen(buf.ptr, flags);
    te_string_free(&buf);
    if (*fd == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

static te_errno
pci_current_num_vfs_get(const pci_device *dev, unsigned int *num)
{
    te_errno rc;
    FILE *fd;
    int n_parsed;

    rc = pci_current_num_vfs_fopen(dev, "r", &fd);
    if (rc != 0)
        return rc;

    n_parsed = fscanf(fd, "%u", num);
    fclose(fd);
    if (n_parsed != 1)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot parse current number of VFs for a PCI device, %r", rc);
    }

    return rc;
}

static te_errno
pci_current_num_vfs_set_try(FILE *fd, unsigned int num)
{
    int n_required;
    int n_printed;

    n_required = snprintf(NULL, 0, "%u", num);
    n_printed = fprintf(fd, "%u", num);
    if (n_printed < 0)
    {
        ERROR("%s: write failed: %r", __FUNCTION__, te_rc_os2te(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    else if (n_required != n_printed)
    {
        ERROR("%s: tried to write %d bytes (value %u), but only %d bytes were actually written",
              __FUNCTION__, n_required, num, n_printed);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    return 0;
}

static te_errno
pci_current_num_vfs_set(const pci_device *dev, unsigned int num)
{
    te_errno rc;
    FILE *fd;

    rc = pci_current_num_vfs_fopen(dev, "w", &fd);
    if (rc != 0)
        return rc;

    /* Disable buffering to be able to handle write errors */
    setvbuf (fd, NULL, _IONBF, 0);

    rc = pci_current_num_vfs_set_try(fd, num);
    if (TE_RC_GET_ERROR(rc) == TE_EBUSY)
    {
        /*
         * It's possible that the number of VFs cannot be changed from non-zero
         * to non-zero. In these cases, the number of VFs needs to be set to
         * zero first.
         */
        rc = pci_current_num_vfs_set_try(fd, 0);
        if (rc == 0)
            rc = pci_current_num_vfs_set_try(fd, num);
    }

    fclose(fd);

    if (rc != 0)
        ERROR("Cannot set current number of VFs for a PCI device, %r", rc);

    return rc;
}

static te_errno
unbind_pci_device(const pci_device *dev)
{
    te_errno rc;
    te_string buf = TE_STRING_INIT;
    int fd;

    /*
     * Try to remove existing VFs since some drivers may leave
     * existing VFs after unbind.
     */
    (void)pci_current_num_vfs_set(dev, 0);

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
    te_errno rc = 0;
    int status;
    te_string buf = TE_STRING_INIT;

    te_string_append(&buf, "/sys/bus/pci/drivers/%s", drvname);

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
    te_string_append(&buf, "/sbin/modprobe %s", drvname);

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
 * On the other hand, e.g. vfio-pci seems to ignore remove_id completely.
 * Another issue is that writing a new vendor/device IDs to new_id causes
 * the driver to probe all the devices with these IDs and bind them if they
 * are not bound to some other driver (however, if the device is already
 * known, the explicit bind is still necessary).
 * Writing 0 to /sys/bus/pci/drivers/drivers_autoprobe should disable such
 * behaviour, but vfio-pci seems to ignore this.
 */

static te_errno
let_generic_driver_know_pci_device(const pci_device *dev, const char *drvname)
{
    te_errno rc = 0;
    te_string buf = TE_STRING_INIT;
    int fd;

    te_string_append(&buf, "/sys/bus/pci/drivers/%s/new_id", drvname);

    fd = open(buf.ptr, O_WRONLY);
    if (fd < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        te_string_free(&buf);
        return rc;
    }
    te_string_cut(&buf, buf.len);
    te_string_append(&buf, "%04x %04x", dev->vendor_id, dev->device_id);

    /* Do not fail if ID is already added */
    if (write(fd, buf.ptr, buf.len) < 0 && errno != EEXIST)
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

    te_string_append(&buf, "/sys/bus/pci/drivers/%s/bind", drvname);

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
         * Also, if a device is already bound on a driver, e.g. auto bound,
         * the binding it again brings the EBUSY code.
         */
        if (errno != ENODEV && errno != EBUSY)
            rc = TE_OS_RC(TE_TA_UNIX, errno);
        else
        {
            int eno = errno;

            te_string_cut(&buf, buf.len);
            te_string_append(&buf, "/sys/bus/pci/drivers/%s/", drvname);
            rc = format_device_address(&buf, &dev->address);
            if (rc == 0 && access(buf.ptr, F_OK) != 0)
                rc = TE_RC(TE_TA_UNIX, eno);
        }
    }
    close(fd);
    te_string_free(&buf);
    return rc;
}

static te_errno
sysfs_read_dev_major_minor(const char *name, const char *attr,
                           int *major, int *minor)
{
    FILE *f;
    te_errno rc = open_pci_attr(name, attr, &f);

    if (rc != 0)
        return rc;

    if (fscanf(f, "%d:%d", major, minor) != 2)
    {
        ERROR("Cannot parse PCI '%s' major:minor attribute '%s' value: %s",
              name, attr, strerror(errno));
    }
    fclose(f);
    return 0;
}

static te_errno
sysfs_read_dev_type(const char *name, int major, int minor, mode_t *type)
{
    te_string buf = TE_STRING_INIT_STATIC(PATH_MAX);
    struct stat statbuf;
    struct stat dev_statbuf;

    te_string_append(&buf, SYSFS_PCI_DEVICES_TREE "/%s", name);

    if (stat(buf.ptr, &dev_statbuf) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    te_string_reset(&buf);
    te_string_append(&buf, "/sys/dev/char/%d:%d", major, minor);

    if (stat(buf.ptr, &statbuf) == 0 && statbuf.st_ino == dev_statbuf.st_ino)
    {
        *type = S_IFCHR;
        return 0;
    }

    te_string_reset(&buf);
    te_string_append(&buf, "/sys/dev/block/%d:%d", major, minor);

    if (stat(buf.ptr, &statbuf) == 0 && statbuf.st_ino == dev_statbuf.st_ino)
    {
        *type = S_IFBLK;
        return 0;
    }

    ERROR("%s: Failed to get device type for '%d:%d'",
          __FUNCTION__, major, minor);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

typedef int (*filter_func)(const struct dirent *de);

typedef struct pci_driver_dev_list_helper {
    const char *driver;
    filter_func filter;
    const char *subdir;
} pci_driver_dev_list_helper;

static int
filter_uio(const struct dirent *de)
{
    return (strcmp_start("uio", de->d_name) == 0);
}

static int
filter_virtio(const struct dirent *de)
{
    return (strcmp_start("virtio", de->d_name) == 0);
}

static const pci_driver_dev_list_helper dev_list_helper[] = {
    {
        .driver = "igb_uio",
        .filter = filter_uio,
        .subdir = NULL,
    },
        {
        .driver = "uio_pci_generic",
        .filter = filter_uio,
        .subdir = NULL,
    },
    {
        .driver = "virtio-pci",
        .filter = filter_virtio,
        .subdir = "block",
    },
};

static const struct pci_driver_dev_list_helper *
pci_driver_dev_list_get(const struct pci_driver_dev_list_helper *dlhs,
                        size_t count, const char *driver_name)
{
    size_t i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(dlhs[i].driver, driver_name) == 0)
            return dlhs + i;
    }

    return NULL;
}

typedef te_errno (*for_each_dev_callback)(const pci_device *dev,
                                          const char *subdir,
                                          const char *device, void *user);

static te_errno
pci_driver_dev_list_for_each(const pci_driver_dev_list_helper *dhl,
                             const pci_device *dev,
                             for_each_dev_callback callback, void *user)
{
    te_errno rc;
    int i, dirs;
    struct dirent **namelist = NULL;
    te_string buf = TE_STRING_INIT_STATIC(PATH_MAX);

    rc = format_sysfs_device_name(&buf, dev, "/");
    if (rc != 0)
        return rc;

    dirs = scandir(buf.ptr, &namelist, dhl->filter, alphasort);
    if (dirs < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    for (i = 0; i < dirs && rc == 0; i++)
    {
        int j, devdirs;
        struct dirent **devlist = NULL;
        te_string subdir = TE_STRING_INIT_STATIC(PATH_MAX);
        te_string devdir = TE_STRING_INIT_STATIC(PATH_MAX);

        te_string_append(&subdir, "%s", namelist[i]->d_name);
        if (dhl->subdir != NULL)
            te_string_append(&subdir, "/%s", dhl->subdir);

        te_string_append(&devdir, "%s/%s", buf.ptr, subdir.ptr);

        devdirs = scandir(devdir.ptr, &devlist, NULL, alphasort);
        if (devdirs < 0)
        {
            if (errno == ENOENT)
            {
                /* It is normal, requested subdir is not device, continue */
                continue;
            }
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            break;
        }

        for (j = 0; j < devdirs && rc == 0; j++)
        {
            if (devlist[j]->d_name[0] == '.')
                continue;

            rc = callback(dev, subdir.ptr, devlist[j]->d_name, user);
        }

        for (j = 0; j < devdirs; j++)
            free(devlist[j]);
        free(devlist);
    }

    for (i = 0; i < dirs; i++)
        free(namelist[i]);
    free(namelist);

    return rc;
}

static te_errno
create_device_callback(const pci_device *pci_dev, const char *subdir,
                       const char *device, void *user)
{
    dev_t dev;
    mode_t dev_type = 0;
    int maj, min;
    te_errno rc;
    struct stat statbuf;
    te_string buf = TE_STRING_INIT_STATIC(PATH_MAX);
    te_string device_path = TE_STRING_INIT_STATIC(PATH_MAX);

    UNUSED(user);

    rc = format_device_address(&buf, &pci_dev->address);
    if (rc != 0)
        return rc;

    te_string_append(&buf, "/%s/%s", subdir, device);

    rc = sysfs_read_dev_major_minor(buf.ptr, "dev", &maj, &min);

    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        return 0;

    if (rc != 0)
        return rc;

    dev = makedev(maj, min);

    te_string_append(&device_path, "/dev/%s", device);

    if (stat(device_path.ptr, &statbuf) == 0)
    {
        if (statbuf.st_dev == dev)
            return 0;

        if (unlink(device_path.ptr) != 0)
        {
            ERROR("%s: Could not remove old device '%s': %s",
                  __FUNCTION__, device_path.ptr, strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
    }

    rc = sysfs_read_dev_type(buf.ptr, maj, min, &dev_type);
    if (rc != 0)
        return rc;

    RING("Creating '%s' with '%d:%d' as %s dev", device_path.ptr, maj, min,
         dev_type == S_IFBLK ? "block": "char");

    if (mknod(device_path.ptr, dev_type, dev) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
}

static te_errno
maybe_create_device(const pci_device *dev, const char *drvname)
{
    const pci_driver_dev_list_helper *dlh;

    dlh = pci_driver_dev_list_get(dev_list_helper,
                                  TE_ARRAY_LEN(dev_list_helper), drvname);

    if (dlh == NULL)
        return 0;

    return pci_driver_dev_list_for_each(dlh, dev, create_device_callback, NULL);
}

static te_errno
try_override(const pci_device *dev, const char *drv)
{
    te_string sys = TE_STRING_INIT;
    te_string id = TE_STRING_INIT;
    te_errno rc;
    ssize_t ret;
    int fd;

    rc = format_device_address(&id, &dev->address);
    if (rc != 0)
    {
        te_string_free(&id);
        return rc;
    }

    te_string_append(&sys, "/sys/bus/pci/devices/%s/driver_override", id.ptr);
    te_string_free(&id);

    if (access(sys.ptr, F_OK) != 0)
    {
        rc = (errno == ENOENT) ? TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP) :
                                 TE_OS_RC(TE_TA_UNIX, errno);
        te_string_free(&sys);
        return rc;
    }

    fd = open(sys.ptr, O_WRONLY);

    rc = TE_OS_RC(TE_TA_UNIX, errno);
    te_string_free(&sys);

    if (fd < 0)
        return rc;

    /* Driver override is cleared only when a newline symbol is written. */
    if (*drv == '\0')
        drv = "\n";

    ret = write(fd, drv, strlen(drv));
    rc = (ret < 0) ? TE_OS_RC(TE_TA_UNIX, errno) : 0;

    close(fd);
    return rc;
}

static te_errno
pci_driver_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const pci_device *dev;
    te_errno rc;
    char driver_name[PATH_MAX];
    unsigned int n_vfs;

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;
    rc = get_driver_name(dev, driver_name, sizeof(driver_name));
    if (rc != 0)
        return rc;

    if (strcmp(driver_name, value) == 0)
        return maybe_create_device(dev, value);

    rc = pci_current_num_vfs_get(dev, &n_vfs);
    if (rc != 0)
        n_vfs = 0;

    if (*driver_name != '\0')
    {
        rc = unbind_pci_device(dev);
        if (rc != 0)
            return rc;
    }

    if (*value != '\0')
    {
        const char *generic_drivers[] = {"uio_pci_generic", "igb_uio",
                                         "vfio-pci", "virtio-pci"};
        bool is_gen_driver = false;
        unsigned int i;

        rc = maybe_load_driver(value);
        if (rc != 0)
            return rc;

        /* Available on kernels >= 3.15. */
        rc = try_override(dev, value);

        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
        {
            for (i = 0;
                 i < TE_ARRAY_LEN(generic_drivers) && !is_gen_driver;
                 i++)
            {
                is_gen_driver = (strcmp(generic_drivers[i], value) == 0);
            }

            if (is_gen_driver)
            {
                rc = let_generic_driver_know_pci_device(dev, value);
                if (rc != 0)
                    return rc;
            }
        }
        else if (rc != 0)
        {
            return rc;
        }

        rc = bind_pci_device(dev, value);
        if (rc != 0)
            return rc;

        if (n_vfs != 0)
        {
            rc = pci_current_num_vfs_set(dev, n_vfs);
            if (rc != 0)
                return rc;
        }

        rc = maybe_create_device(dev, value);
        if (rc != 0)
            return rc;
    }
    else
    {
        /* Clear the driver override we might have set previously. */
        rc = try_override(dev, value);
        if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_EOPNOTSUPP)
            return rc;
    }

    return 0;
}

static te_errno
append_list_callback(const pci_device *pci_dev, const char *subdir,
                        const char *device, void *user)
{
    te_vec *names = user;
    char *name;

    UNUSED(pci_dev);
    UNUSED(subdir);

    name = TE_STRDUP(device);
    TE_VEC_APPEND(names, name);
    return 0;
}

static te_errno
pci_dev_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_errno rc;
    const pci_driver_dev_list_helper *dlh;
    const pci_device *dev;
    char driver_name[PATH_MAX];

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    rc = get_driver_name(dev, driver_name, sizeof(driver_name));
    if (rc != 0)
        return rc;

    dlh = pci_driver_dev_list_get(dev_list_helper,
                                  TE_ARRAY_LEN(dev_list_helper), driver_name);

    if (dlh == NULL)
        return 0;

    return pci_driver_dev_list_for_each(dlh, dev, append_list_callback, names);
}

static te_errno
pci_net_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_string buf = TE_STRING_INIT;
    te_vec net_names = TE_VEC_INIT_AUTOPTR(char *);
    te_string net_list = TE_STRING_INIT;
    pci_device *dev;
    te_errno rc;
    char driver_name[PATH_MAX];
    struct dirent **namelist;
    unsigned int i;
    size_t base;
    size_t n;

    rc = find_device_by_addr_str(addr_str, &dev);
    if (rc != 0)
        return rc;

    rc = get_driver_name(dev, driver_name, sizeof(driver_name));
    if (rc != 0)
        return rc;

    rc = format_sysfs_device_name(&buf, dev, "/");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    if (strcmp_start("virtio-pci", driver_name) == 0)
    {
        int nvirtio = scandir(buf.ptr, &namelist, filter_virtio, alphasort);

        if (nvirtio < 0)
        {
            te_string_free(&buf);
            ERROR("Failed to scan directory '%s': %s",
                  buf.ptr, strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        if (nvirtio == 0)
        {
            te_string_free(&buf);

            free(dev->net_list);
            dev->net_list = NULL;
            return 0;
        }

        te_string_append(&buf, "%s", namelist[0]->d_name);

        while (nvirtio--)
            free(namelist[nvirtio]);

        free(namelist);
    }

    te_string_append(&buf, "%s", "/net");

    rc = get_dir_list_vec(buf.ptr, &net_names, true, NULL, NULL, alphasort);
    te_string_free(&buf);
    if (rc != 0)
    {
        te_vec_free(&net_names);
        return rc;
    }

    n = te_vec_size(&net_names);
    if (n == 0)
    {
        te_vec_free(&net_names);

        free(dev->net_list);
        dev->net_list = NULL;
        return 0;
    }

    /*
     * dev->net_list caches the interface names in the space-separated
     * form pci_net_get() parses: every name is followed by a single
     * space, including the last one.
     */
    te_string_join_vec(&net_list, &net_names, " ");
    te_string_append(&net_list, " ");
    te_vec_free(&net_names);

    rc = string_replace(&dev->net_list, net_list.ptr);
    te_string_free(&net_list);
    if (rc != 0)
        return rc;

    base = te_vec_size(names);
    for (i = 0; i < n; i++)
    {
        char buf_idx[sizeof("4294967295")];
        char *name;

        te_snprintf(buf_idx, sizeof(buf_idx), "%u", i);
        name = TE_STRDUP(buf_idx);
        TE_VEC_APPEND(names, name);
    }

    if (n == 1)
    {
        /*
         * Per doc/cm/cm_pci.yml: a device with a single network
         * interface names that instance with an empty string
         * instead of a sequence number. The ta_conf list encoding
         * can carry an empty instance name, so the lone sequence
         * number is simply replaced by one.
         */
        free(TE_VEC_GET(char *, names, base));
        TE_VEC_GET(char *, names, base) = TE_STRDUP("");
    }

    return 0;
}

static te_errno
pci_net_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const char *net_id_str = ta_conf_ctx_inst(ctx, "net");
    const pci_device *dev;
    unsigned int net_id;
    const char *next;
    unsigned int n;
    unsigned int i;
    te_errno rc;

    if (*net_id_str == '\0')
    {
        net_id = 0;
    }
    else
    {
        rc = te_strtoui(net_id_str, 0, &net_id);
        if (rc != 0)
            return rc;
    }

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    if (dev->net_list == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    for (n = 0, i = 0; n < net_id && i < strlen(dev->net_list); i++)
    {
        if (dev->net_list[i] == ' ')
            n++;
    }
    if (n != net_id)
    {
        ERROR("Failed to find %u network in list '%s'", net_id, dev->net_list);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    next = strchr(&dev->net_list[i], ' ');
    if (next == NULL)
        next = &dev->net_list[strlen(dev->net_list)];

    return te_string_append_chk(val, "%.*s", (int)(next - &dev->net_list[i]),
                                &dev->net_list[i]);
}

static te_errno
pci_numa_node_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_errno rc;
    int result;

    rc = read_pci_int_attr(addr_str, "numa_node", &result);
    if (rc != 0 || result < 0)
    {
        /* Default to empty value (no defined NUMA node) on failure */
        return 0;
    }

    return te_string_append_chk(val, "/agent:%s/hardware:/node:%d", ta_name,
                                result);
}

static te_errno
pci_sriov_num_vfs_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_errno rc;
    int result;

    rc = read_pci_int_attr(addr_str, "sriov_numvfs", &result);
    if (rc != 0)
        return rc;

    *val = result;
    return 0;
}

static te_errno
pci_sriov_num_vfs_set(const ta_conf_ctx *ctx, int32_t val)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const pci_device *dev;
    te_errno rc;

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    /* Match te_strtoui()'s base-10, unsigned int range. */
    if (val < 0 || val > UINT_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = pci_current_num_vfs_set(dev, (unsigned int)val);
    if (rc != 0)
        return rc;

    rc = update_device_list();
    if (rc != 0) {
        ERROR("%s(%s): failed to update device list: %r", __func__, addr_str, rc);
        return rc;
    }

    return 0;
}

static te_errno
pci_sriov_pf_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    char *base;
    te_errno rc;
    te_string buf = TE_STRING_INIT;
    char link[PATH_MAX] = "";
    const pci_device *dev;

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    rc = format_sysfs_device_name(&buf, dev, "/physfn");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    if (readlink(buf.ptr, link, sizeof(link) - 1) < 0)
    {
        int err = errno;

        te_string_free(&buf);

        if (err == ENOENT)
            return 0;

        return TE_OS_RC(TE_TA_UNIX, err);
    }
    te_string_free(&buf);

    base = strrchr(link, '/');
    if (base == NULL)
        base = link;
    else
        base++;

    return te_string_append_chk(val, "/agent:%s/hardware:/pci:/device:%s",
                                ta_name, base);
}

static te_errno
pci_sriov_vf_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const char *virtfn_id = ta_conf_ctx_inst(ctx, "vf");
    te_string buf = TE_STRING_INIT;
    char link[PATH_MAX] = "";
    char *vf_addr;
    const pci_device *dev;
    const pci_device *vf;
    te_errno rc;
    char *agent;
    ssize_t n;

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    rc = format_sysfs_device_name(&buf, dev, "/");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    te_string_append(&buf, PCI_VIRTFN_PREFIX "%s", virtfn_id);

    n = readlink(buf.ptr, link, sizeof(link) - 1);
    te_string_free(&buf);
    if (n < 0)
        return TE_RC(TE_TA_UNIX, TE_EFAIL);

    vf_addr = te_basename(link);
    if (vf_addr == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    /*
     * Caller may not have a permission to access a VF (it requires grabbing
     * it as a resource). But the information that a VF exists might be
     * provided (it does not grant any permissions to access the VF,
     * subsequent resource grab is required).
     */
    rc = find_device_by_addr_str_ignore_permission(vf_addr, (pci_device **)&vf);
    if (rc != 0) {
        free(vf_addr);
        return rc;
    }
    free(vf_addr);

    agent = cfg_oid_str_get_inst_name(ta_conf_ctx_oid(ctx), 1);
    if (agent == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    rc = te_string_append_chk(val,
             "/agent:%s/hardware:/pci:/vendor:%04x/device:%04x/instance:%u",
             agent, vf->vendor_id, vf->device_id, vf->devno);
    free(agent);

    return rc;
}

static int
filter_virtfn(const struct dirent *de)
{
    return (strcmp_start(PCI_VIRTFN_PREFIX, de->d_name) == 0);
}

static te_errno
pci_sriov_vf_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_string buf = TE_STRING_INIT;
    const pci_device *dev;
    struct dirent **namelist = NULL;
    te_errno rc = 0;
    int n = 0;
    int i;

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        goto out;

    rc = format_sysfs_device_name(&buf, dev, NULL);
    if (rc != 0)
        goto out;

    n = scandir(buf.ptr, &namelist, filter_virtfn, alphasort);
    if (n < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Failed to get a list of PCI virtual functions");
        goto out;
    }

    for (i = 0; i < n; i++)
    {
        char *name;

        if (strlen(namelist[i]->d_name) <= strlen(PCI_VIRTFN_PREFIX))
        {
            ERROR("Malformed virtfn link");
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
            goto out;
        }

        name = TE_STRDUP(namelist[i]->d_name + strlen(PCI_VIRTFN_PREFIX));
        TE_VEC_APPEND(names, name);
    }

out:
    te_string_free(&buf);
    for (i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);

    return rc;
}

static te_errno
pci_sriov_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_errno rc;
    int result;

    rc = read_pci_int_attr(addr_str, "sriov_totalvfs", &result);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        result = 0;
    else if (rc != 0)
        return rc;

    *val = result;
    return 0;
}

/* Obtain PCI device serial number */
static te_errno
pci_serialno_get(const ta_conf_ctx *ctx, te_string *val)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);
    UNUSED(val);

    return 0;
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_errno rc = 0;
    netconf_list *list = NULL;

    rc = netconf_devlink_get_info(nh_genl, "pci", addr_str, &list);
    if (rc != 0)
    {
        if (rc == TE_ENODEV || rc == TE_ENOENT || rc == TE_EOPNOTSUPP)
            return 0;

        return TE_RC(TE_TA_UNIX, rc);
    }

    if (list->length != 0)
    {
        rc = te_string_append_chk(val, "%s",
                 list->tail->data.devlink_info.serial_number);
    }

    netconf_list_free(list);
    return rc;
#endif
}

/* Get PCI device eswitch mode */
static te_errno
pci_eswitch_mode_get(const ta_conf_ctx *ctx, te_string *val)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);
    UNUSED(val);

    return 0;
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_errno rc = 0;
    netconf_list *list = NULL;

    rc = netconf_devlink_get_eswitch(nh_genl, "pci", addr_str, &list);
    if (rc != 0)
    {
        if (rc == TE_ENODEV || rc == TE_ENOENT || rc == TE_EOPNOTSUPP ||
            rc == TE_EPERM)
            return 0;

        return TE_RC(TE_TA_UNIX, rc);
    }

    if (list->length != 0)
    {
        rc = te_string_append_chk(val, "%s",
                 netconf_devlink_eswitch_mode_netconf2str(
                     list->tail->data.devlink_eswitch.mode));
    }

    netconf_list_free(list);
    return rc;
#endif
}

/* Set PCI device eswitch mode */
static te_errno
pci_eswitch_mode_set(const ta_conf_ctx *ctx, const char *value)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);
    UNUSED(value);

    return 0;
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_errno rc = 0;
    netconf_devlink_eswitch_mode mode;

    mode = netconf_devlink_eswitch_mode_str2netconf(value);
    if (mode == NETCONF_DEVLINK_ESWITCH_MODE_UNDEF)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = netconf_devlink_eswitch_mode_set(nh_genl, "pci", addr_str, mode);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
#endif
}

/* Get PCI device eswitch support */
static te_errno
pci_eswitch_get(const ta_conf_ctx *ctx, bool *val)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);

    *val = false;
    return 0;
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    te_errno rc = 0;
    netconf_list *list = NULL;

    rc = netconf_devlink_get_eswitch(nh_genl, "pci", addr_str, &list);
    netconf_list_free(list);

    *val = (rc == 0);
    return 0;
#endif
}

/* List parameter names of a PCI device */
static te_errno
pci_param_list(const ta_conf_ctx *ctx, te_vec *names)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);
    UNUSED(names);

    return 0;
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    te_errno rc = 0;
    netconf_node *node;
    netconf_devlink_param *param;

    rc = update_dev_params(gid);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            return 0;

        return TE_RC(TE_TA_UNIX, rc);
    }

    for (node = dev_params->head; node != NULL; node = node->next)
    {
        char *name;

        param = &node->data.devlink_param;
        if (strcmp(param->bus_name, "pci") != 0 ||
            strcmp(param->dev_name, addr_str) != 0)
            continue;

        name = TE_STRDUP(param->name);
        TE_VEC_APPEND(names, name);
    }

    return 0;
#endif
}

/* Show whether device parameter is driver-specific or generic */
static te_errno
param_drv_specific_get(const ta_conf_ctx *ctx, int32_t *val)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    te_errno rc;
    netconf_devlink_param *param;

    rc = find_dev_param(gid, addr_str, param_name, &param);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    *val = param->generic ? 0 : 1;
    return 0;
#endif
}

/* Get type of device parameter */
static te_errno
param_type_get(const ta_conf_ctx *ctx, te_string *val)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    te_errno rc;
    netconf_devlink_param *param;

    rc = find_dev_param(gid, addr_str, param_name, &param);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return te_string_append_chk(val, "%s", netconf_nla_type2str(param->type));
#endif
}

/*
 * List configuration modes for which parameter value is available.
 * Such as "runtime", "driverinit" and "permanent".
 */
static te_errno
param_value_list(const ta_conf_ctx *ctx, te_vec *names)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);
    UNUSED(names);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    te_errno rc;
    netconf_devlink_param *param;
    int i;

    rc = find_dev_param(gid, addr_str, param_name, &param);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    for (i = 0; i < NETCONF_DEVLINK_PARAM_CMODES; i++)
    {
        if (param->values[i].defined)
        {
            char *name = TE_STRDUP(devlink_param_cmode_netconf2str(i));

            TE_VEC_APPEND(names, name);
        }
    }

    return 0;
#endif
}

/* Get device parameter value stored in specified configuration mode */
static te_errno
param_value_get(const ta_conf_ctx *ctx, te_string *val)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    const char *cmode_name = ta_conf_ctx_inst(ctx, "value");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    te_errno rc;
    netconf_devlink_param *param;
    netconf_devlink_param_value *param_value;
    netconf_devlink_param_cmode cmode;

    rc = find_dev_param(gid, addr_str, param_name, &param);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    cmode = devlink_param_cmode_str2netconf(cmode_name);
    if (cmode == NETCONF_DEVLINK_PARAM_CMODE_UNDEF)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    param_value = &param->values[cmode];

    if (!param_value->defined)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    switch (param->type)
    {
        case NETCONF_NLA_FLAG:
            return te_string_append_chk(val, "%d",
                                        param_value->data.flag ? 1 : 0);

        case NETCONF_NLA_U8:
            return te_string_append_chk(val, "%u",
                     (unsigned int)(param_value->data.u8));

        case NETCONF_NLA_U16:
            return te_string_append_chk(val, "%u",
                     (unsigned int)(param_value->data.u16));

        case NETCONF_NLA_U32:
            return te_string_append_chk(val, "%u",
                     (unsigned int)(param_value->data.u32));

        case NETCONF_NLA_U64:
            return te_string_append_chk(
                     val, "%llu",
                     (long long unsigned int)(param_value->data.u64));

        case NETCONF_NLA_STRING:
            return te_string_append_chk(val, "%s", param_value->data.str);

        default:
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
#endif
}

/* Set parameter value in specified configuration mode */
static te_errno
param_value_set(const ta_conf_ctx *ctx, const char *value)
{
#ifndef USE_LIBNETCONF
    UNUSED(ctx);
    UNUSED(value);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    const char *addr_str = ta_conf_ctx_inst(ctx, "device");
    const char *param_name = ta_conf_ctx_inst(ctx, "param");
    const char *cmode_name = ta_conf_ctx_inst(ctx, "value");
    unsigned int gid = ta_conf_ctx_gid(ctx);
    te_errno rc;
    netconf_devlink_param *param;
    netconf_devlink_param_cmode cmode;
    netconf_devlink_param_value_data value_data;
    uint64_t uint_val;
    char *str_val = NULL;

    rc = find_dev_param(gid, addr_str, param_name, &param);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    cmode = devlink_param_cmode_str2netconf(cmode_name);
    if (cmode == NETCONF_DEVLINK_PARAM_CMODE_UNDEF)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    memset(&value_data, 0, sizeof(value_data));

    if (param->type == NETCONF_NLA_STRING)
    {
        str_val = strdup(value);
        if (str_val == NULL)
        {
            ERROR("%s(): out of memory", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }

        value_data.str = str_val;
    }
    else if (param->type == NETCONF_NLA_U8 ||
             param->type == NETCONF_NLA_U16 ||
             param->type == NETCONF_NLA_U32 ||
             param->type == NETCONF_NLA_U64 ||
             param->type == NETCONF_NLA_FLAG)
    {
        rc = te_str_to_uint64(value, 10, &uint_val);
        if (rc != 0)
        {
            ERROR("%s(): invalid value '%s'", __FUNCTION__, value);
            return TE_RC(TE_TA_UNIX, rc);
        }

        if ((param->type == NETCONF_NLA_U8 && uint_val > UINT8_MAX) ||
            (param->type == NETCONF_NLA_U16 && uint_val > UINT16_MAX) ||
            (param->type == NETCONF_NLA_U32 && uint_val > UINT32_MAX) ||
            (param->type == NETCONF_NLA_FLAG && uint_val > 1))
        {
            ERROR("%s(): too big value '%s'", __FUNCTION__, value);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        switch (param->type)
        {
            case NETCONF_NLA_U8:
                value_data.u8 = uint_val;
                break;

            case NETCONF_NLA_U16:
                value_data.u16 = uint_val;
                break;

            case NETCONF_NLA_U32:
                value_data.u32 = uint_val;
                break;

            case NETCONF_NLA_U64:
                value_data.u64 = uint_val;
                break;

            case NETCONF_NLA_FLAG:
                value_data.flag = (uint_val != 0);
                break;

            default:
                break;
        }
    }
    else
    {
        ERROR("%s(): not supported parameter type %d", __FUNCTION__,
              param->type);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = netconf_devlink_param_set(nh_genl, "pci", addr_str,
                                   param_name, param->type,
                                   cmode, &value_data);
    if (rc != 0)
    {
        free(str_val);
        return rc;
    }

    /* Update cached parameter value after successful change */
    netconf_devlink_param_value_data_mv(param->type,
                                        &param->values[cmode].data,
                                        &value_data);
    return 0;
#endif
}

#define PCI_SPDK_CONFIG_PATTERN_FMT "te_spdk_config_%s_*.json"
#define PCI_SPDK_CONFIG_NAME_FMT "%ste_spdk_config_%s_%s.json"
#define PCI_SPDK_CONFIG_NAME_ARGS(_pci_addr, _cfg_name) \
    ta_tmp_dir, (_pci_addr), (_cfg_name)

static te_errno
pci_spdk_config_add(const ta_conf_ctx *ctx)
{
    const char *pci_addr = ta_conf_ctx_inst(ctx, "device");
    const char *cfg_name = ta_conf_ctx_inst(ctx, "spdk_config");
    te_errno rc = 0;
    const pci_device *dev = NULL;
    FILE *json_file;

    rc = find_device_by_addr_str(pci_addr, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    if (dev->device_class != TE_PCI_PROG_INTERFACE_NVM_CONTROLLER_NVME)
    {
        ERROR("SPDK configs are only meaningful for NVMe devices");
        return TE_RC(TE_TA_UNIX, TE_ENOTBLK);
    }

    if (te_access_fmt(F_OK, PCI_SPDK_CONFIG_NAME_FMT,
                      PCI_SPDK_CONFIG_NAME_ARGS(pci_addr, cfg_name)) == 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    json_file = te_fopen_fmt("w", PCI_SPDK_CONFIG_NAME_FMT,
                             PCI_SPDK_CONFIG_NAME_ARGS(pci_addr, cfg_name));
    if (json_file == NULL)
        return TE_RC(TE_TA_UNIX, TE_EIO);

    fprintf(json_file,
            "{\n"
            "   \"subsystems\": [\n"
            "    {\n"
            "       \"subsystem\": \"bdev\",\n"
            "       \"config\": [\n"
            "           {\n"
            "              \"method\": \"bdev_nvme_attach_controller\"\n"
            "              \"params\": {\n"
            "                 \"trtype\": \"PCIe\",\n"
            "                 \"name\":\"%s\",\n"
            "                 \"traddr\":\"%04x:%02x:%02x.%x\"\n"
            "               }\n"
            "           }\n"
            "       ]\n"
            "    }\n"
            "   ]\n"
            "}\n",
            cfg_name, dev->address.domain, dev->address.bus,
            dev->address.slot, dev->address.fn);
    fclose(json_file);

    return 0;
}

static te_errno
pci_spdk_config_del(const ta_conf_ctx *ctx)
{
    const char *pci_addr = ta_conf_ctx_inst(ctx, "device");
    const char *cfg_name = ta_conf_ctx_inst(ctx, "spdk_config");
    te_errno rc;

    rc = te_unlink_fmt(PCI_SPDK_CONFIG_NAME_FMT,
                       PCI_SPDK_CONFIG_NAME_ARGS(pci_addr, cfg_name));
    return TE_RC_UPSTREAM(TE_TA_UNIX, rc);
}

static te_file_scandir_callback add_spdk_config;
static te_errno add_spdk_config(const char *pattern, const char *pathname,
                                void *data)
{
    te_vec *names = data;
    char *base = te_file_extract_glob(pathname, pattern, true);

    if (base == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    TE_VEC_APPEND(names, base);

    return 0;
}

static te_errno
pci_spdk_config_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char *pci_addr = ta_conf_ctx_inst(ctx, "device");

    return te_file_scandir(ta_tmp_dir, add_spdk_config, names,
                           PCI_SPDK_CONFIG_PATTERN_FMT, pci_addr);
}

static te_errno
pci_spdk_config_filename_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *pci_addr = ta_conf_ctx_inst(ctx, "device");
    const char *cfg_name = ta_conf_ctx_inst(ctx, "spdk_config");

    if (te_access_fmt(R_OK, PCI_SPDK_CONFIG_NAME_FMT,
                      PCI_SPDK_CONFIG_NAME_ARGS(pci_addr, cfg_name)) != 0)
    {
        ERROR("There is no SPDK config '%s' for PCI device %s",
              cfg_name, pci_addr);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return te_string_append_chk(val, PCI_SPDK_CONFIG_NAME_FMT,
                                PCI_SPDK_CONFIG_NAME_ARGS(pci_addr, cfg_name));
}

/*
 * The 9 PCI id leaves print unpadded/padded hex or plain decimal with
 * no "0x" prefix, matching the raw sysfs attributes byte for byte;
 * hence they stay string-typed instead of using a fixed-width numeric
 * type.
 */
#define PCI_ID_GET(_name, _field, _fmt)                              \
    static te_errno                                                      \
    pci_##_name##_get(const ta_conf_ctx *ctx, te_string *val)                  \
    {                                                                    \
        const char *addr_str = ta_conf_ctx_inst(ctx, "device");          \
        const pci_device *dev;                                           \
        te_errno rc;                                                     \
                                                                         \
        rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);     \
        if (rc != 0)                                                     \
            return rc;                                                   \
                                                                         \
        return te_string_append_chk(val, _fmt, dev->_field);             \
    }

PCI_ID_GET(domain, address.domain, "%04x");
PCI_ID_GET(bus, address.bus, "%02x");
PCI_ID_GET(slot, address.slot, "%02x");
PCI_ID_GET(fn, address.fn, "%u");
PCI_ID_GET(vendor_id, vendor_id, "%04x");
PCI_ID_GET(device_id, device_id, "%04x");
PCI_ID_GET(subsystem_vendor, subsystem_vendor, "%04x");
PCI_ID_GET(subsystem_device, subsystem_device, "%04x");
PCI_ID_GET(class, device_class, "%08x");

#undef PCI_ID_GET

/* PCI id leaf: RO_STR node on the matching PCI_ID_GET() handler */
#define PCI_ID_NODE_RO(_name) TA_CONF_RO_STR(#_name, pci_##_name##_get)

static const ta_conf_node *const node_pci =
    TA_CONF_NA("pci",
        TA_CONF_LIST("device", pci_device_list,
            PCI_ID_NODE_RO(domain), PCI_ID_NODE_RO(bus),
            PCI_ID_NODE_RO(slot), PCI_ID_NODE_RO(fn),
            PCI_ID_NODE_RO(vendor_id), PCI_ID_NODE_RO(device_id),
            PCI_ID_NODE_RO(subsystem_vendor),
            PCI_ID_NODE_RO(subsystem_device), PCI_ID_NODE_RO(class),
            TA_CONF_RO_BOOL("eswitch", pci_eswitch_get,
                TA_CONF_RW_STR("mode", pci_eswitch_mode_get,
                               pci_eswitch_mode_set)),
            TA_CONF_RO_INT32("sriov", pci_sriov_get,
                TA_CONF_RO_COLL("vf", pci_sriov_vf_get, pci_sriov_vf_list),
                TA_CONF_RO_STR("pf", pci_sriov_pf_get),
                TA_CONF_RW_INT32("num_vfs", pci_sriov_num_vfs_get,
                               pci_sriov_num_vfs_set)),
            TA_CONF_RW_STR("driver", pci_driver_get, pci_driver_set),
            TA_CONF_RO_STR("node", pci_numa_node_get),
            TA_CONF_RO_COLL("net", pci_net_get, pci_net_list),
            TA_CONF_LIST("dev", pci_dev_list),
            TA_CONF_RO_STR("serialno", pci_serialno_get),
            TA_CONF_LIST("param", pci_param_list,
                TA_CONF_RO_INT32("driver_specific", param_drv_specific_get),
                TA_CONF_RO_STR("type", param_type_get),
                TA_CONF_RW_COLL_STR("value", param_value_get,
                                    param_value_set, param_value_list)),
            TA_CONF_COLL("spdk_config", pci_spdk_config_add,
                         pci_spdk_config_del, pci_spdk_config_list,
                TA_CONF_RO_STR("filename",
                               pci_spdk_config_filename_get))),
        TA_CONF_LIST("vendor", pci_vendor_list,
            TA_CONF_LIST("device", pci_vendor_device_list,
                TA_CONF_RO_COLL("instance", pci_device_instance_get,
                                pci_device_instance_list))));


#undef PCI_ID_NODE_RO

te_errno
ta_unix_conf_pci_init()
{
    te_errno rc = update_device_list();

    if (rc != 0)
        return rc;

#ifdef USE_LIBNETCONF
    if (netconf_open(&nh_genl, NETLINK_GENERIC) != 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): failed to open netconf session, errno=%r",
              __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }
#endif

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

    rcf_pch_rsrc_info("/agent/hardware/pci/vendor/device/instance/netdev",
                      pci_device_grab,
                      pci_device_release);

    return ta_conf_register("/agent/hardware", node_pci);
}

/* Release resources */
te_errno
ta_unix_conf_pci_cleanup(void)
{
#ifdef USE_LIBNETCONF
    if (nh_genl != NULL)
    {
        netconf_close(nh_genl);
        nh_genl = NULL;
    }

    if (dev_params != NULL)
    {
        netconf_list_free(dev_params);
        dev_params = NULL;
    }
#endif

    free_device_list();

    return 0;
}
