/** @file
 * @brief Test API to configure virtual machines.
 *
 * @defgroup tapi_conf_vm Virtual machines configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of TAPI to configure virtual machines.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_VM_H__
#define __TE_TAPI_CFG_VM_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add virtual machine.
 *
 * @param ta            Test Agent.
 * @param vm_name       Virtual machine name.
 * @param tmpl          @c NULL or virtual machine configuration template.
 * @param start         Start it just after addition and template apply
 *
 * @return Status code
 */
extern te_errno tapi_cfg_vm_add(const char *ta, const char *vm_name,
                                const char *tmpl, te_bool start);

/**
 * Delete virtual machine.
 *
 * @param ta            Test Agent.
 * @param vm_name       Virtual machine name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_vm_del(const char *ta, const char *vm_name);

/**
 * Start virtual machine.
 *
 * @param ta            Test Agent.
 * @param vm_name       Virtual machine name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_vm_start(const char *ta, const char *vm_name);

/**
 * Stop virtual machine.
 *
 * @param ta            Test Agent.
 * @param vm_name       Virtual machine name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_vm_stop(const char *ta, const char *vm_name);

/**
 * Add drive subtree.
 *
 * @param ta            Test Agent.
 * @param vm_name       Virtual machine name.
 * @param drive_name    Drive name.
 * @param file          File option.
 * @param snapshot      Snapshot option.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_vm_add_drive(const char *ta, const char *vm_name,
                                      const char *drive_name, const char *file,
                                      te_bool snapshot);

/**
 * Pass PCI function to virtual machine.
 *
 * @param ta            Test Agent.
 * @param vm_name       Virtual machine name.
 * @param pci_pt_name   PCI function name.
 * @param vendor        Vendor component of PCI address.
 * @param device        Device component of PCI address.
 * @param instance      Instance component of PCI address.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_vm_pass_pci(const char *ta, const char *vm_name,
                                     const char *pci_pt_name,
                                     unsigned long vendor, unsigned long device,
                                     unsigned long instance);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_VM_H__ */

/**@} <!-- END tapi_conf_vm --> */
