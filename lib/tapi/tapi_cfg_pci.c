/** @file
 * @brief PCI Configuration Model TAPI
 *
 * Implementation of test API for network configuration model
 * (doc/cm/cm_pci).
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#define TE_LGR_USER      "Configuration TAPI"

#include "te_config.h"

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_str.h"
#include "rcf_api.h"
#include "logger_api.h"
#include "conf_api.h"

#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_pci.h"

#define CFG_PCI_TA_DEVICE_FMT "/agent:%s/hardware:/pci:/device:%s"
#define CFG_PCI_TA_VEND_DEVICE_FMT "/agent:%s/hardware:/pci:/vendor:%s/device:%s"

te_errno
tapi_cfg_pci_get_pci_vendor_device(const char *ta, const char *pci_addr,
                                   char **vendor, char **device)
{
    cfg_val_type val_type = CVT_STRING;
    char *vendor_str;
    char *device_str;
    te_errno rc;

    rc = cfg_get_instance_fmt(&val_type, &device_str,
                              CFG_PCI_TA_DEVICE_FMT "/device_id:",
                              ta, pci_addr);
    if (rc != 0)
    {
        ERROR("Failed to get device ID by PCI addr %s, %r", pci_addr, rc);
        return rc;
    }

    rc = cfg_get_instance_fmt(&val_type, &vendor_str,
                              CFG_PCI_TA_DEVICE_FMT "/vendor_id:",
                              ta, pci_addr);

    if (rc != 0)
    {
        free(device_str);
        ERROR("Failed to get vendor ID by PCI addr %s, %r", pci_addr, rc);
        return rc;
    }

    if (vendor != NULL)
        *vendor = vendor_str;

    if (device != NULL)
        *device = device_str;

    return 0;
}

te_errno
tapi_cfg_pci_get_vfs_of_pf(const char *pf_oid, te_bool pci_device,
                           unsigned int *n_pci_vfs, cfg_oid ***pci_vfs,
                           unsigned int **pci_vf_ids)
{
    cfg_handle *vfs = NULL;
    unsigned int n_vfs;
    cfg_oid **result = NULL;
    unsigned int *ids = NULL;
    unsigned int i;
    te_errno rc;

    if (n_pci_vfs == NULL)
    {
        ERROR("%s: pointer to number of VFs must not be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_find_pattern_fmt(&n_vfs, &vfs, "%s/virtfn:*", pf_oid);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
            ERROR("Failed to get virtual functions of a device");
        return rc;
    }

    result = TE_ALLOC(n_vfs * sizeof(*result));
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    ids = TE_ALLOC(n_vfs * sizeof(*ids));
    if (ids == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    for (i = 0; i < n_vfs; i++)
    {
        char *vf_instance = NULL;
        char *vf_device = NULL;
        cfg_oid *vf_oid;
        cfg_oid *vf_ref_oid;

        rc = cfg_get_oid(vfs[i], &vf_ref_oid);
        if (rc != 0)
        {
            ERROR("Failed to get VF reference from PF");
            goto out;
        }

        rc = te_strtoui(CFG_OID_GET_INST_NAME(vf_ref_oid, 5), 10, &ids[i]);
        cfg_free_oid(vf_ref_oid);
        if (rc != 0)
        {
            ERROR("Failed to parse VF index");
            goto out;
        }

        rc = cfg_get_instance(vfs[i], NULL, &vf_instance);
        if (rc != 0)
        {
            ERROR("Failed to get VF instance");
            goto out;
        }

        if (pci_device)
        {
            rc = cfg_get_instance_str(NULL, &vf_device, vf_instance);
            free(vf_instance);
            vf_instance = NULL;
            if (rc != 0)
            {
                ERROR("Failed to get VF device");
                goto out;
            }
        }

        vf_oid = cfg_convert_oid_str(pci_device ? vf_device : vf_instance);
        free(vf_instance);
        free(vf_device);
        if (vf_oid == NULL)
        {
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            ERROR("Failed to get VF oid");
            goto out;
        }

        result[i] = vf_oid;
    }

    if (pci_vfs != NULL)
        *pci_vfs = result;
    if (pci_vf_ids != NULL)
        *pci_vf_ids = ids;
    *n_pci_vfs = n_vfs;

out:
    free(vfs);
    if (rc != 0 || pci_vfs == NULL)
    {
        for (i = 0; result != NULL && i < n_vfs; i++)
            free(result[i]);
        free(result);
    }

    if (rc != 0 || pci_vf_ids == NULL)
        free(ids);

    return rc;
}

te_errno
tapi_cfg_pci_addr_by_oid(const cfg_oid *pci_device, char **pci_addr)
{
    char *result;

    if (pci_addr == NULL)
    {
        ERROR("%s: output argument must not be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    result = cfg_oid_get_inst_name(pci_device, 4);
    if (result == NULL)
    {
        ERROR("Failed to get PCI addr by oid");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *pci_addr = result;

    return 0;
}

te_errno
tapi_cfg_pci_oid_by_addr(const char *ta, const char *pci_addr,
                         char **pci_oid)
{
    int rc;

    rc = te_asprintf(pci_oid, CFG_PCI_TA_DEVICE_FMT, ta, pci_addr);
    if (rc < 0)
    {
        ERROR("Failed to create a PCI device OID string");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    return 0;
}

te_errno
tapi_cfg_pci_addr_by_oid_array(unsigned int n_devices, const cfg_oid **pci_devices,
                               char ***pci_addrs)
{
    char **result = NULL;
    unsigned int i;
    te_errno rc = 0;

    if (pci_addrs == NULL)
    {
        ERROR("%s: output argument must not be NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    result = TE_ALLOC(sizeof(*result) * n_devices);
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    for (i = 0; i < n_devices; i++)
    {
        rc = tapi_cfg_pci_addr_by_oid(pci_devices[i], &result[i]);
        if (rc != 0)
            goto out;
    }

    *pci_addrs = result;

out:
    if (rc != 0)
    {
        for (i = 0; result != NULL && i < n_devices; i++)
            free(result[i]);
        free(result);
    }

    return rc;
}

char *
tapi_cfg_pci_rsrc_name(const cfg_oid *pci_instance)
{
    char *rsrc_name;

    te_asprintf(&rsrc_name, "pci_fn:%s:%s:%s",
                CFG_OID_GET_INST_NAME(pci_instance, 4),
                CFG_OID_GET_INST_NAME(pci_instance, 5),
                CFG_OID_GET_INST_NAME(pci_instance, 6));

    if (rsrc_name == NULL)
        ERROR("Failed to create PCI function resource string");

    return rsrc_name;
}

te_errno
tapi_cfg_pci_grab(const cfg_oid *pci_instance)
{
    char *rsrc_name = NULL;
    char *oid_str = NULL;
    te_errno rc;

    rsrc_name = tapi_cfg_pci_rsrc_name(pci_instance);
    if (rsrc_name == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    rc = cfg_get_instance_fmt(NULL, NULL, "/agent:%s/rsrc:%s",
                              CFG_OID_GET_INST_NAME(pci_instance, 1), rsrc_name);
    if (rc == 0)
    {
        rc = TE_RC(TE_TAPI, TE_EALREADY);
        goto out;
    }

    oid_str = cfg_convert_oid(pci_instance);
    if (oid_str == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, oid_str),
                              "/agent:%s/rsrc:%s",
                              CFG_OID_GET_INST_NAME(pci_instance, 1), rsrc_name);
    if (rc != 0)
        ERROR("Failed to reserve resource '%s': %r", oid_str, rc);

out:
    free(rsrc_name);
    free(oid_str);

    return rc;
}

te_errno
tapi_cfg_pci_bind_ta_driver_on_device(const char *ta,
                                      enum tapi_cfg_driver_type type,
                                      const char *pci_addr)
{
    char *ta_driver = NULL;
    char *pci_oid = NULL;
    char *pci_driver = NULL;
    te_errno rc;

    rc = tapi_cfg_pci_get_ta_driver(ta, type, &ta_driver);
    if (rc != 0)
        goto out;

    rc = tapi_cfg_pci_oid_by_addr(ta, pci_addr, &pci_oid);
    if (rc != 0)
        goto out;

    rc = tapi_cfg_pci_get_driver(pci_oid, &pci_driver);
    if (rc != 0)
        goto out;

    if (strcmp(ta_driver, pci_driver) != 0)
    {
        rc = tapi_cfg_pci_bind_driver(pci_oid, ta_driver);
        if (rc != 0)
            goto out;
        /*
         * Synchronize possible changes in PCI device configuration after
         * driver bind.
         */
        rc = cfg_synchronize(pci_oid, TRUE);
        if (rc != 0)
            goto out;
    }

out:
    free(ta_driver);
    free(pci_oid);
    free(pci_driver);

    return rc;
}

te_errno
tapi_cfg_pci_get_ta_driver(const char *ta,
                           enum tapi_cfg_driver_type type,
                           char **driver)
{
    const char *driver_prefix = "";
    char *result = NULL;
    te_errno rc;

    switch (type)
    {
        case NET_DRIVER_TYPE_NET:
            driver_prefix = "net";
            break;

        case NET_DRIVER_TYPE_DPDK:
            driver_prefix = "dpdk";
            break;

        default:
            ERROR("Invalid PCI driver type");
            return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    rc = cfg_get_instance_fmt(NULL, &result, "/local:%s/%s_driver:",
                              ta, driver_prefix);
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        ERROR("Failed to get PCI driver of agent %s", ta);
        return rc;
    }

    if (result == NULL || *result == '\0')
    {
        free(result);
        result = NULL;
    }

    *driver = result;

    return 0;
}

te_errno
tapi_cfg_pci_get_net_if(const char *pci_oid, char **interface)
{
    cfg_val_type type = CVT_STRING;
    te_errno rc;

    rc = cfg_get_instance_fmt(&type, interface, "%s/net:", pci_oid);
    if (rc != 0)
        ERROR("Failed to get the only interface of a PCI device: %r", rc);

    return rc;
}

te_errno
tapi_cfg_pci_bind_driver(const char *pci_oid, const char *driver)
{
    te_errno rc;

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, driver), "%s/driver:", pci_oid);
    if (rc != 0)
    {
        ERROR("Failed to bind driver %s on PCI device %s", driver, pci_oid);
        return rc;
    }

    return 0;
}

te_errno
tapi_cfg_pci_get_driver(const char *pci_oid, char **driver)
{
    te_errno rc;
    cfg_val_type type = CVT_STRING;

    rc = cfg_get_instance_fmt(&type, driver, "%s/driver:", pci_oid);
    if (rc != 0)
    {
        ERROR("Failed to get current driver of PCI device %s", pci_oid);
        return rc;
    }

    return 0;
}

te_errno
tapi_cfg_pci_get_devices(const char *pci_oid, unsigned int *count,
                         char ***device_names)
{
    unsigned int n_devices = 0;
    cfg_handle *devices = NULL;
    char **result = NULL;
    unsigned int i;
    te_errno rc;

    rc = cfg_find_pattern_fmt(&n_devices, &devices,
                              "%s/dev:*", pci_oid);
    if (rc != 0)
        goto out;

    result = TE_ALLOC(n_devices * sizeof(*result));
    if (result == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    for (i = 0; i < n_devices; i++)
    {
        rc = cfg_get_inst_name(devices[i], &result[i]);
        if (rc != 0)
            goto out;
    }

    *count = n_devices;
    if (device_names != NULL)
        *device_names = result;

out:
    free(devices);
    if (rc != 0 || device_names == NULL)
    {
        for (i = 0; result != NULL && i < n_devices; i++)
            free(result[i]);

        free(result);
    }

    return rc;
}

te_errno
tapi_cfg_pci_devices_by_vendor_device(const char *ta, const char *vendor,
                                      const char *device, unsigned int *size,
                                      char ***pci_oids)
{
    cfg_handle *instances = NULL;
    unsigned int n_instances = 0;
    char **result = NULL;
    unsigned int i;
    te_errno rc;

    rc = cfg_find_pattern_fmt(&n_instances, &instances,
                              CFG_PCI_TA_VEND_DEVICE_FMT "/instance:*",
                              ta, vendor, device);
    if (rc != 0)
        goto out;

    result = TE_ALLOC(n_instances * sizeof(*result));
    if (result == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    for (i = 0; i < n_instances; i++)
    {
        rc = cfg_get_instance(instances[i], NULL, &result[i]);
        if (rc != 0)
        {
            ERROR("Failed to get PCI device");
            goto out;
        }
    }

    *size = n_instances;
    if (pci_oids != NULL)
        *pci_oids = result;

out:
    free(instances);
    if (rc != 0 || pci_oids == NULL)
    {
        for (i = 0; result != NULL && i < n_instances; i++)
            free(result[i]);

        free(result);
    }

    return rc;
}
