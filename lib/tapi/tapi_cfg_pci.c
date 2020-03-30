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

te_errno
tapi_cfg_pci_get_pci_vendor_device(const char *ta, const char *pci_addr,
                                   char **vendor, char **device)
{
    cfg_val_type val_type = CVT_STRING;
    char *vendor_str;
    char *device_str;
    te_errno rc;

    rc = cfg_get_instance_fmt(&val_type, &device_str,
                              "/agent:%s/hardware:/pci:/device:%s/device_id:",
                              ta, pci_addr);
    if (rc != 0)
    {
        ERROR("Failed to get device ID by PCI addr %s, %r", pci_addr, rc);
        return rc;
    }

    rc = cfg_get_instance_fmt(&val_type, &vendor_str,
                              "/agent:%s/hardware:/pci:/device:%s/vendor_id:",
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
