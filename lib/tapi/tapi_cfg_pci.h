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

/** Driver type */
enum tapi_cfg_driver_type {
    NET_DRIVER_TYPE_NET = 0, /**< Kernel network interface driver */
    NET_DRIVER_TYPE_DPDK, /**< DPDK-compatible driver */
};

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
 * Get PCI addresses of PCI functions with specified vendor and device
 * identifiers.
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  vendor       Vendor identifier
 * @param[in]  device       Device identifier
 * @param[out] size         Count of @p pci_addrs
 * @param[out] pci_oids     OIDs of found devices (/agent/hardware/pci/device).
 *                          Might be @c NULL to only get the count.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_devices_by_vendor_device(const char *ta,
                                                      const char *vendor,
                                                      const char *device,
                                                      unsigned int *size,
                                                      char ***pci_oids);

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

/**
 * Get PCI device OID by the PCI address
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  pci_addr     PCI device address (BDF notation)
 * @param[out] pci_oid      PCI device OID (/agent/hardware/pci/device)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_oid_by_addr(const char *ta, const char *pci_addr,
                                         char **pci_oid);

/**
 * Get PCI device driver assigned to a Test Agent
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  type         Driver type
 * @param[out] driver       Driver name (on success, if no driver is assigned,
 *                          the pointed data becomes @c NULL). The argument
 *                          itself must not be @c NULL.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_get_ta_driver(const char *ta,
                                           enum tapi_cfg_driver_type type,
                                           char **driver);
/**
 * Get driver of a PCI device
 *
 * @param[in]  pci_oid      PCI device OID (/agent/hardware/pci/device)
 * @param[out] driver       Driver name (must not be @c NULL)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_get_driver(const char *pci_oid,
                                        char **driver);

/**
 * Bind driver to a PCI device
 *
 * @param[in]  pci_oid      PCI device OID (/agent/hardware/pci/device)
 * @param[in]  driver       Driver name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_bind_driver(const char *pci_oid,
                                         const char *driver);

/**
 * Bind driver associated with a Test Agent on a PCI device.
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  type         Driver type
 * @param[in]  pci_addr     PCI address of the device to which the
 *                          driver will be bound
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_bind_ta_driver_on_device(const char *ta,
                                              enum tapi_cfg_driver_type type,
                                              const char *pci_addr);

/**
 * Get the first network interface associated with a PCI device.
 *
 * @note In theory, more than one interface can be associated,
 *       in that case the function produces a warning.
 *
 * @param[in]  pci_oid      PCI device OID (/agent/hardware/pci/device)
 * @param[out] interface    Network interface name (must not be @c NULL)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_pci_get_net_if(const char *pci_oid, char **interface);

/**@} <!-- END tapi_conf_pci --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PCI_H__ */
