/** @file
 * @brief PCI support
 *
 * PCI configuration tree support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
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

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_string.h"
#include "te_str.h"
#include "te_file.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "conf_common.h"
#include "te_alloc.h"

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
    unsigned devno;               /**< Device instance number
                                   *   among all the devices
                                   *   with same vendor/device ID
                                   */
    uint16_t subsystem_vendor;    /**< Subsystem vendor ID */
    uint16_t subsystem_device;    /**< Subsystem device ID */
    uint32_t device_class;        /**< PCI device class */
    unsigned lock;                /**< TE resource lock counter */
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
static pci_vendors *vendor_list;

/** Whole PCI tree TE resource lock */
static unsigned global_pci_lock;

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
    if (fscanf(f, "%x", &result) != 1)
    {
        ERROR("Cannot parse PCI '%s' hex attribute '%s' value: %s",
              name, attr, strerror(errno));
    }
    fclose(f);
    return result;
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
find_vendor(const pci_vendors *list, unsigned vendor_id)
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
free_vendor_list(pci_vendors *list)
{
    static int callnum = 0;
    callnum++;

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
    unsigned i;

    vendor_list = calloc(1, sizeof(*vendor_list));
    if (vendor_list == NULL)
        return vendor_list;
    LIST_INIT(vendor_list);

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
                return NULL;
            }

            vendor->id = devs[i].vendor_id;
            LIST_INIT(&vendor->vendor_devices);
            LIST_INSERT_HEAD(vendor_list, vendor, next);
        }

        vendor_device = find_vendor_device(vendor->vendor_devices,
                                           devs[i].device_id);
        if (vendor_device == NULL)
        {
            vendor_device = calloc(1, sizeof(*vendor_device));
            if (vendor_device == NULL)
            {
                free_vendor_list(vendor_list);
                return NULL;
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

    static int callnum = 0;
    callnum++;

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
    te_string_free(&result);
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

    LIST_FOREACH(vendor, vendor_list, next)
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

        LIST_FOREACH(vendor, vendor_list, next)
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
    te_errno rc;
    te_string buf = TE_STRING_INIT_STATIC(PATH_MAX);
    struct stat statbuf;
    struct stat dev_statbuf;

    rc = te_string_append(&buf, SYSFS_PCI_DEVICES_TREE "/%s", name);
    if (rc != 0)
        return rc;

    if (stat(buf.ptr, &dev_statbuf) != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    te_string_reset(&buf);
    rc = te_string_append(&buf, "/sys/dev/char/%d:%d", major, minor);
    if (rc != 0)
        return rc;

    if (stat(buf.ptr, &statbuf) == 0 && statbuf.st_ino == dev_statbuf.st_ino)
    {
        *type = S_IFCHR;
        return 0;
    }

    te_string_reset(&buf);
    rc = te_string_append(&buf, "/sys/dev/block/%d:%d", major, minor);
    if (rc != 0)
        return rc;

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

        rc = te_string_append(&subdir, "%s", namelist[i]->d_name);
        if (rc == 0 && dhl->subdir != NULL)
            rc = te_string_append(&subdir, "/%s", dhl->subdir);

        if (rc != 0)
            break;

        rc = te_string_append(&devdir, "%s/%s", buf.ptr, subdir.ptr);
        if (rc != 0)
            break;

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

    rc = te_string_append(&buf, "/%s/%s", subdir, device);
    if (rc != 0)
        return rc;

    rc = sysfs_read_dev_major_minor(buf.ptr, "dev", &maj, &min);

    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        return 0;

    if (rc != 0)
        return rc;

    dev = makedev(maj, min);

    rc = te_string_append(&device_path, "/dev/%s", device);
    if (rc != 0)
        return rc;

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
pci_driver_set(unsigned int gid, const char *oid, const char *value,
               const char *unused1, const char *unused2,
               const char *addr_str)
{
    const pci_device *dev;
    te_errno rc;
    char driver_name[PATH_MAX];
    unsigned int n_vfs;

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
        te_bool is_gen_driver = FALSE;
        unsigned int i;

        rc = maybe_load_driver(value);
        if (rc != 0)
            return rc;
        for (i = 0; i < TE_ARRAY_LEN(generic_drivers) && !is_gen_driver; i++)
            is_gen_driver = (strcmp(generic_drivers[i], value) == 0);

        if (is_gen_driver)
        {
            rc = let_generic_driver_know_pci_device(dev, value);
            if (rc != 0)
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

    return 0;
}

static te_errno
append_list_callback(const pci_device *pci_dev, const char *subdir,
                        const char *device, void *user)
{
    te_string *list = (te_string*)user;

    UNUSED(pci_dev);
    UNUSED(subdir);

    return te_string_append(list, "%s ", device);
}

static te_errno
pci_dev_list(unsigned int gid, const char *oid, const char *sub_id,
             char **list, const char *unused1, const char *unused2,
             const char *addr_str)
{
    te_errno rc;
    const pci_driver_dev_list_helper *dlh;
    const pci_device *dev;
    char driver_name[PATH_MAX];
    te_string result = TE_STRING_INIT;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    rc = get_driver_name(dev, driver_name, sizeof(driver_name));
    if (rc != 0)
        return rc;

    dlh = pci_driver_dev_list_get(dev_list_helper,
                                  TE_ARRAY_LEN(dev_list_helper), driver_name);

    if (dlh == NULL)
        return string_empty_list(list);

    rc = pci_driver_dev_list_for_each(dlh, dev, append_list_callback, &result);
    if (rc != 0)
        return rc;

    if (result.ptr == NULL)
        return string_empty_list(list);

    te_string_cut(&result, 1);
    *list = result.ptr;

    return 0;
}

static te_errno
pci_net_list(unsigned int gid, const char *oid, const char *sub_id,
             char **list, const char *unused1, const char *unused2,
             const char *addr_str)
{
    te_string buf = TE_STRING_INIT;
    pci_device *dev;
    char *net_list = NULL;
    te_errno rc;
    char driver_name[PATH_MAX];
    struct dirent **namelist;
    unsigned int i;
    int n;

    UNUSED(sub_id);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

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
        n = scandir(buf.ptr, &namelist, filter_virtio, alphasort);

        if (n < 0)
        {
            te_string_free(&buf);
            ERROR("Failed to scan directory '%s': %s",
                  buf.ptr, strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        if (n == 0)
        {
            te_string_free(&buf);
            *list = strdup("");
            if (*list == NULL)
                return TE_OS_RC(TE_TA_UNIX, TE_ENOMEM);

            free(dev->net_list);
            dev->net_list = NULL;
            return 0;
        }

        rc = te_string_append(&buf, "%s", namelist[0]->d_name);
        if (rc != 0)
            return rc;

        while (n--)
            free(namelist[n]);

        free(namelist);
    }

    rc = te_string_append(&buf, "%s", "/net");
    if (rc != 0)
        return rc;

    net_list = calloc(1, RCF_MAX_VAL);
    if (net_list == NULL)
    {
        te_string_free(&buf);
        ERROR("Out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    rc = get_dir_list(buf.ptr, net_list, RCF_MAX_VAL, TRUE, NULL, NULL);
    te_string_reset(&buf);
    if (rc != 0)
    {
        te_string_free(&buf);
        free(net_list);
        return rc;
    }

    if (*net_list == '\0')
    {
        free(net_list);
        net_list = NULL;
    }

    rc = string_replace(&dev->net_list, net_list);
    if (rc != 0)
    {
        te_string_free(&buf);
        free(net_list);
        return rc;
    }

    if (net_list == NULL)
    {
        te_string_free(&buf);
        *list = NULL;
        return 0;
    }

    for (n = 0, i = 0; i < strlen(net_list); i++)
    {
        if (net_list[i] != ' ')
            continue;

        rc = te_string_append(&buf, "%u ", n++);
        if (rc != 0)
        {
            te_string_free(&buf);
            free(net_list);
            return rc;
        }
    }
    free(net_list);

    if (n == 1)
    {
        te_string_reset(&buf);
        /* The string must contain at least 2 chars */
        (void)te_string_append(&buf, " ");
    }

    *list = buf.ptr;

    return 0;
}

static te_errno
pci_net_get(unsigned int gid, const char *oid, char *value,
            const char *unused1, const char *unused2,
            const char *addr_str, const char *net_id_str)
{
    const pci_device *dev;
    unsigned int net_id;
    const char *next;
    unsigned int n;
    unsigned int i;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

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

    snprintf(value, RCF_MAX_VAL, "%.*s", (int)(next - &dev->net_list[i]),
             &dev->net_list[i]);

    return 0;
}

static te_errno
pci_numa_node_get(unsigned int gid, const char *oid, char *value,
               const char *unused1, const char *unused2,
               const char *addr_str)
{
    te_errno rc;
    int result;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = read_pci_int_attr(addr_str, "numa_node", &result);
    if (rc != 0 || result < 0)
    {
        /* Default to empty value (no defined NUMA node) on failure */
        value[0] = '\0';
        return 0;
    }

    snprintf(value, RCF_MAX_VAL, "/agent:%s/hardware:/node:%d", ta_name,
             result);
    return 0;
}

static te_errno
pci_sriov_num_vfs_get(unsigned int gid, const char *oid, char *value,
                      const char *unused1, const char *unused2,
                      const char *addr_str)
{
    te_errno rc;
    int result;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = read_pci_int_attr(addr_str, "sriov_numvfs", &result);
    if (rc != 0)
        return rc;

    snprintf(value, RCF_MAX_VAL, "%d", result);
    return 0;
}

static te_errno
pci_sriov_num_vfs_set(unsigned int gid, const char *oid, char *value,
                      const char *unused1, const char *unused2,
                      const char *addr_str)
{
    te_errno rc;
    unsigned int n_vfs;
    const pci_device *dev;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    rc = te_strtoui(value, 10, &n_vfs);
    if (rc != 0)
        return rc;

    rc = pci_current_num_vfs_set(dev, n_vfs);
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
pci_sriov_pf_get(unsigned int gid, const char *oid, char *value,
                 const char *unused1, const char *unused2,
                 const char *addr_str)
{
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
        int rc = errno;
        te_string_free(&buf);

        if (rc == ENOENT)
        {
            *value = '\0';
            return 0;
        }

        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    te_string_free(&buf);

    base = strrchr(link, '/');
    if (base == NULL)
        base = link;
    else
        base++;

    snprintf(value, RCF_MAX_VAL, "/agent:%s/hardware:/pci:/device:%s", ta_name,
             base);
    return 0;
}

static te_errno
pci_sriov_vf_get(unsigned int gid, const char *oid, char *value,
                 const char *unused1, const char *unused2,
                 const char *addr_str, const char *unused3,
                 const char *virtfn_id)
{
    te_string buf = TE_STRING_INIT;
    char link[PATH_MAX] = "";
    char *vf_addr;
    const pci_device *dev;
    const pci_device *vf;
    te_errno rc;
    char *agent;
    int n;

    UNUSED(gid);
    UNUSED(unused1);
    UNUSED(unused2);
    UNUSED(unused3);

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        return rc;

    rc = format_sysfs_device_name(&buf, dev, "/");
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    rc = te_string_append(&buf, PCI_VIRTFN_PREFIX "%s", virtfn_id);
    if (rc != 0)
    {
        te_string_free(&buf);
        return rc;
    }

    rc = readlink(buf.ptr, link, sizeof(link) - 1);
    te_string_free(&buf);
    if (rc < 0)
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

    agent = cfg_oid_str_get_inst_name(oid, 1);
    if (agent == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    n = snprintf(value, RCF_MAX_VAL,
                 "/agent:%s/hardware:/pci:/vendor:%04x/device:%04x/instance:%u",
                 agent, vf->vendor_id, vf->device_id, vf->devno);
    free(agent);
    if (n < 0 || n >= RCF_MAX_VAL)
        return TE_RC(TE_TA_UNIX, TE_EFAIL);

    return 0;
}

static int
filter_virtfn(const struct dirent *de)
{
    return (strcmp_start(PCI_VIRTFN_PREFIX, de->d_name) == 0);
}

static te_errno
pci_sriov_vf_list(unsigned int gid, const char *oid, const char *sub_id,
                  char **list, const char *unused1, const char *unused2,
                  const char *addr_str)
{
    size_t result_size = RCF_MAX_VAL;
    te_string buf = TE_STRING_INIT;
    const pci_device *dev;
    struct dirent **names = NULL;
    char *result = NULL;
    te_errno rc = 0;
    size_t off;
    int n = 0;
    int i;

    UNUSED(sub_id);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = find_device_by_addr_str(addr_str, (pci_device **)&dev);
    if (rc != 0)
        goto out;

    rc = format_sysfs_device_name(&buf, dev, NULL);
    if (rc != 0)
        goto out;

    n = scandir(buf.ptr, &names, filter_virtfn, alphasort);
    if (n < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Failed to get a list of PCI virtual functions");
        goto out;
    }

    result = TE_ALLOC(result_size);
    if (result == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto out;
    }

    for (off = 0, i = 0; i < n; i++)
    {
        int ret;

        if (strlen(names[i]->d_name) <= strlen(PCI_VIRTFN_PREFIX))
        {
            ERROR("Malformed virtfn link");
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
            goto out;
        }

        ret = snprintf(result + off, result_size - off, "%s ",
                       names[i]->d_name + strlen(PCI_VIRTFN_PREFIX));
        if (ret < 0 || (size_t)ret >= result_size - off)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("snprintf() failed or string truncated");
            goto out;
        }

        off += ret;
    }

    *list = result;

out:
    te_string_free(&buf);
    for (i = 0; i < n; i++)
        free(names[i]);
    free(names);

    if (rc != 0)
        free(result);

    return rc;
}

static te_errno
pci_sriov_get(unsigned int gid, const char *oid, char *value,
              const char *unused1, const char *unused2,
              const char *addr_str)
{
    te_errno rc;
    int result;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = read_pci_int_attr(addr_str, "sriov_totalvfs", &result);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        result = 0;
    else if (rc != 0)
        return rc;

    snprintf(value, RCF_MAX_VAL, "%d", result);
    return 0;
}

/* Obtain PCI device serial number */
static te_errno
pci_serialno_get(unsigned int gid, const char *oid, char *value,
                 const char *unused1, const char *unused2,
                 const char *addr_str)
{
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

#ifndef USE_LIBNETCONF
    UNUSED(gid);
    UNUSED(addr_str);

    *value = '\0';
    return 0;
#else
    te_errno rc = 0;
    netconf_list *list = NULL;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

    rc = netconf_devlink_get_info(nh_genl, "pci", addr_str, &list);
    if (rc != 0)
    {
        if (rc == TE_ENODEV || rc == TE_ENOENT)
        {
            *value = '\0';
            return 0;
        }

        return TE_RC(TE_TA_UNIX, rc);
    }

    if (list->length == 0)
    {
        *value = '\0';
        goto out;
    }

    rc = te_snprintf(value, RCF_MAX_VAL, "%s",
                     list->tail->data.devlink_info.serial_number);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed, rc=%r", __FUNCTION__, rc);
        rc = TE_RC(TE_TA_UNIX, rc);
    }

out:

    netconf_list_free(list);
    return rc;
#endif
}

/* List parameter names of a PCI device */
static te_errno
pci_param_list(unsigned int gid, const char *oid,
               const char *sub_id, char **list,
               const char *unused1, const char *unused2,
               const char *addr_str)
{
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);
    UNUSED(unused2);

#ifndef USE_LIBNETCONF
    UNUSED(gid);
    UNUSED(addr_str);

    *list = NULL;
    return 0;
#else
    te_errno rc = 0;
    netconf_node *node;
    netconf_devlink_param *param;
    te_string str = TE_STRING_INIT;

    rc = update_dev_params(gid);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            *list = NULL;
            return 0;
        }

        return TE_RC(TE_TA_UNIX, rc);
    }

    for (node = dev_params->head; node != NULL; node = node->next)
    {
        param = &node->data.devlink_param;
        if (strcmp(param->bus_name, "pci") != 0 ||
            strcmp(param->dev_name, addr_str) != 0)
            continue;

        rc = te_string_append(&str, "%s ", param->name);
        if (rc != 0)
            goto cleanup;
    }

cleanup:

    if (rc != 0)
        te_string_free(&str);
    else
        *list = str.ptr;

    return rc;
#endif
}

/* Show whether device parameter is driver-specific or generic */
static te_errno
param_drv_specific_get(unsigned int gid, const char *oid, char *value,
                       const char *unused1, const char *unused2,
                       const char *addr_str, const char *param_name)
{
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

#ifndef USE_LIBNETCONF
    UNUSED(gid);
    UNUSED(value);
    UNUSED(addr_str);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    te_errno rc;
    netconf_devlink_param *param;

    rc = find_dev_param(gid, addr_str, param_name, &param);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    rc = te_snprintf(value, RCF_MAX_VAL, "%d",
                     (param->generic ? 0 : 1));
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed, rc=%r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
#endif
}

/* Get type of device parameter */
static te_errno
param_type_get(unsigned int gid, const char *oid, char *value,
               const char *unused1, const char *unused2,
               const char *addr_str, const char *param_name)
{
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

#ifndef USE_LIBNETCONF
    UNUSED(gid);
    UNUSED(value);
    UNUSED(addr_str);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    te_errno rc;
    netconf_devlink_param *param;

    rc = find_dev_param(gid, addr_str, param_name, &param);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    rc = te_snprintf(value, RCF_MAX_VAL, "%s",
                     netconf_nla_type2str(param->type));
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed, rc=%r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
#endif
}

/*
 * List configuration modes for which parameter value is available.
 * Such as "runtime", "driverinit" and "permanent".
 */
static te_errno
param_value_list(unsigned int gid, const char *oid,
                 const char *sub_id, char **list,
                 const char *unused1, const char *unused2,
                 const char *addr_str, const char *param_name)
{
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);
    UNUSED(unused2);

#ifndef USE_LIBNETCONF
    UNUSED(gid);
    UNUSED(addr_str);
    UNUSED(param_name);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    te_errno rc;
    netconf_devlink_param *param;
    te_string str = TE_STRING_INIT;
    int i;

    rc = find_dev_param(gid, addr_str, param_name, &param);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    for (i = 0; i < NETCONF_DEVLINK_PARAM_CMODES; i++)
    {
        if (param->values[i].defined)
        {
            rc = te_string_append(&str, "%s ",
                                  devlink_param_cmode_netconf2str(i));
            if (rc != 0)
                goto cleanup;
        }
    }

cleanup:

    if (rc != 0)
        te_string_free(&str);
    else
        *list = str.ptr;

    return rc;
#endif
}

/* Get device parameter value stored in specified configuration mode */
static te_errno
param_value_get(unsigned int gid, const char *oid, char *value,
                const char *unused1, const char *unused2,
                const char *addr_str, const char *param_name,
                const char *cmode_name)
{
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

#ifndef USE_LIBNETCONF
    UNUSED(gid);
    UNUSED(addr_str);
    UNUSED(param_name);
    UNUSED(cmode_name);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
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
            rc = te_snprintf(value, RCF_MAX_VAL, "%d",
                             param_value->data.flag ? 1 : 0);
            break;

        case NETCONF_NLA_U8:
            rc = te_snprintf(value, RCF_MAX_VAL, "%u",
                             (unsigned int)(param_value->data.u8));
            break;

        case NETCONF_NLA_U16:
            rc = te_snprintf(value, RCF_MAX_VAL, "%u",
                             (unsigned int)(param_value->data.u16));
            break;

        case NETCONF_NLA_U32:
            rc = te_snprintf(value, RCF_MAX_VAL, "%u",
                             (unsigned int)(param_value->data.u32));
            break;

        case NETCONF_NLA_U64:
            rc = te_snprintf(
                     value, RCF_MAX_VAL, "%llu",
                     (long long unsigned int)(param_value->data.u64));
            break;

        case NETCONF_NLA_STRING:
            rc = te_snprintf(value, RCF_MAX_VAL, "%s",
                             param_value->data.str);
            break;

        default:
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed, rc=%r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
#endif
}

/* Set parameter value in specified configuration mode */
static te_errno
param_value_set(unsigned int gid, const char *oid, const char *value,
                const char *unused1, const char *unused2,
                const char *addr_str, const char *param_name,
                const char *cmode_name)
{
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);

#ifndef USE_LIBNETCONF
    UNUSED(gid);
    UNUSED(addr_str);
    UNUSED(param_name);
    UNUSED(cmode_name);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
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
        rc = te_strtoul(value, 10, &uint_val);
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

RCF_PCH_CFG_NODE_RW_COLLECTION(node_pci_param_value, "value",
                               NULL, NULL,
                               param_value_get,
                               param_value_set,
                               NULL, NULL, param_value_list,
                               NULL);

RCF_PCH_CFG_NODE_RO(node_pci_param_type, "type",
                    NULL, &node_pci_param_value, param_type_get);

RCF_PCH_CFG_NODE_RO(node_pci_param_drv_spec, "driver_specific",
                    NULL, &node_pci_param_type,
                    param_drv_specific_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_param, "param",
                               &node_pci_param_drv_spec, NULL,
                               NULL, pci_param_list);

RCF_PCH_CFG_NODE_RO(node_pci_serialno, "serialno",
                    NULL, &node_pci_param, pci_serialno_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_dev, "dev",
                               NULL, &node_pci_serialno,
                               NULL, pci_dev_list);
RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_net, "net",
                               NULL, &node_pci_dev,
                               pci_net_get, pci_net_list);
RCF_PCH_CFG_NODE_RO(node_pci_numa_node, "node",
                    NULL, &node_pci_net,
                    pci_numa_node_get);
RCF_PCH_CFG_NODE_RW(node_pci_driver, "driver",
                    NULL, &node_pci_numa_node,
                    pci_driver_get, pci_driver_set);

RCF_PCH_CFG_NODE_RW(node_pci_sriov_numvfs, "num_vfs",
                    NULL, NULL,
                    pci_sriov_num_vfs_get, pci_sriov_num_vfs_set);
RCF_PCH_CFG_NODE_RO(node_pci_sriov_pf, "pf",
                    NULL, &node_pci_sriov_numvfs,
                    pci_sriov_pf_get);
RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_sriov_vf, "vf",
                               NULL, &node_pci_sriov_pf,
                               pci_sriov_vf_get, pci_sriov_vf_list);
RCF_PCH_CFG_NODE_RO(node_pci_sriov, "sriov",
                    &node_pci_sriov_vf, &node_pci_driver,
                    pci_sriov_get);


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

PCI_ID_NODE_RO(class, device_class, "%08x", &node_pci_sriov);
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

    return rcf_pch_add_node("/agent/hardware", &node_pci);
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
