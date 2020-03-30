/** @file
 * @brief PCI Configuration Model TAPI
 *
 * Definition of test API for network configuration model
 * (doc/cm/cm_pci.yml).
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_PCI_H__
#define __TE_TAPI_CFG_PCI_H__

#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_pci PCI devices configuration of Test Agents
 * @ingroup tapi_conf
 * @{
 */

/**
 * Get vendor and device identifiers of a PCI device.
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  pci_addr     PCI device address (BDF notation)
 * @param[out] vendor       Vendor identifier (may be @c NULL to ignore)
 * @param[out] device       Device identifier (may be @c NULL to ignore)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_get_pci_vendor_device(const char *ta,
                                                   const char *pci_addr,
                                                   char **vendor,
                                                   char **device);

/**
 * Get VFs by a PCI PF object identifier.
 *
 * @param[in]  pf_oid        PF OID in string representation
 * @param[in]  pci_device    @c TRUE - get PCI device OID
 *                           (/agent/hardware/pci/device),
 *                           @c FALSE - get PCI instance OID
 *                           (/agent/hardware/pci/vendor/device/instance)
 * @param[out] n_pci_vfs     Number of discovered VFs (must not be @c NULL)
 * @param[out] pci_vfs       VF object identifiers (may be @c NULL to ignore)
 * @param[out] pci_vf_ids    Indices of the VFs (may be @c NULL to ignore)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_get_vfs_of_pf(const char *pf_oid,
                                           te_bool pci_device,
                                           unsigned int *n_pci_vfs,
                                           cfg_oid ***pci_vfs,
                                           unsigned int **pci_vf_ids);

/**
 * Get PCI address (BDF notation) by PCI device OID (/agent/hardware/pci/device)
 *
 * @param[in]  pci_device       PCI device
 * @param[out] pci_addr         PCI address (must not be @c NULL)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_addr_by_oid(const cfg_oid *pci_device,
                                         char **pci_addr);

/**
 * Call tapi_cfg_pci_addr_by_oid() on an array of PCI device OIDs
 *
 * @param[in]  n_devices        Number of PCI devices
 * @param[in]  pci_devices      PCI device OIDs
 * @param[out] pci_addrs        PCI addresses (must not be @c NULL)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_addr_by_oid_array(unsigned int n_devices,
                                               const cfg_oid **pci_devices,
                                               char ***pci_addrs);
/**
 * Allocate a string with resource name for grabbing a PCI instance.
 *
 * @param[in]  pci_device       PCI instance OID
 *                              (/agent/hardware/pci/vendor/device/instance)
 *
 * @return Allocated resource name string or @c NULL
 */
extern char * tapi_cfg_pci_rsrc_name(const cfg_oid *pci_instance);

/**
 * Grab a PCI device as a resource.
 *
 * @param[in]  pci_device       PCI instance OID
 *                              (/agent/hardware/pci/vendor/device/instance)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_grab(const cfg_oid *pci_instance);

/**@} <!-- END tapi_conf_pci --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PCI_H__ */
