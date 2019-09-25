/** @file
 * @brief Test API to configure virtual machines.
 *
 * @defgroup tapi_conf_vm Virtual machines configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of TAPI to configure virtual machines.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#include "te_config.h"

#include "te_defs.h"
#include "te_errno.h"
#include "logger_ten.h"
#include "conf_api.h"
#include "te_string.h"

#include "tapi_cfg_vm.h"


#define TE_CFG_TA_VM    "/agent:%s/vm:%s"

static te_errno
tapi_cfg_vm_copy_local(const char *ta, const char *vm_name, const char *tmpl)
{
    te_errno rc;
    te_string agent = TE_STRING_INIT;

    rc = te_string_append(&agent, "/agent:%s/vm:%s", ta, vm_name);
    if (rc != 0)
        goto out;

    rc = cfg_copy_subtree_fmt(agent.ptr, "%s", tmpl);

out:
    te_string_free(&agent);

    return rc;
}

/* See descriptions in tapi_cfg_vm.h */
te_errno
tapi_cfg_vm_add(const char *ta, const char *vm_name,
                const char *tmpl, te_bool start)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL, TE_CFG_TA_VM, ta, vm_name);
    if (rc != 0)
    {
        ERROR("Cannot add VM %s to TA %s: %r", vm_name, ta, rc);
        goto fail_vm_add;
    }

    if (tmpl != NULL)
    {
        rc = tapi_cfg_vm_copy_local(ta, vm_name, tmpl);
        if (rc != 0)
        {
            ERROR("Failed to apply template %s", tmpl);
            goto fail_vm_tmpl;
        }
    }

    if (start)
    {
        rc = tapi_cfg_vm_start(ta, vm_name);
        if (rc != 0)
            goto fail_vm_start;
    }

    return 0;

fail_vm_start:
fail_vm_tmpl:
    (void)cfg_del_instance_fmt(FALSE, TE_CFG_TA_VM, ta, vm_name);

fail_vm_add:
    return rc;
}

/* See descriptions in tapi_cfg_vm.h */
te_errno
tapi_cfg_vm_del(const char *ta, const char *vm_name)
{
    te_errno rc;

    rc = cfg_del_instance_fmt(FALSE, TE_CFG_TA_VM, ta, vm_name);
    if (rc != 0)
        ERROR("Cannot delete VM %s from TA %s: %r", vm_name, ta, rc);

    return rc;
}

/* See descriptions in tapi_cfg_vm.h */
te_errno
tapi_cfg_vm_start(const char *ta, const char *vm_name)
{
    te_errno rc;

    rc  = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                               TE_CFG_TA_VM "/status:", ta, vm_name);
    if (rc != 0)
        ERROR("Cannot start VM %s on TA %s: %r", vm_name, ta, rc);

    return rc;
}

/* See descriptions in tapi_cfg_vm.h */
te_errno
tapi_cfg_vm_stop(const char *ta, const char *vm_name)
{
    te_errno rc;

    rc  = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                               TE_CFG_TA_VM "/status:", ta, vm_name);
    if (rc != 0)
        ERROR("Cannot stop VM %s on TA %s: %r", vm_name, ta, rc);

    return rc;
}

/* See descriptions in tapi_cfg_vm.h */
te_errno
tapi_cfg_vm_add_drive(const char *ta, const char *vm_name,
                      const char *drive_name, const char *file,
                      te_bool snapshot)
{
    te_errno rc = 0;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL, TE_CFG_TA_VM "/drive:%s",
                              ta, vm_name, drive_name);
    if (rc != 0)
    {
        ERROR("Cannot add drive %s (VM %s, TA %s): %r", drive_name,
              vm_name, ta, rc);
        return rc;
    }

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, snapshot),
                               TE_CFG_TA_VM "/drive:%s/snapshot:",
                               ta, vm_name, drive_name);
    if (rc != 0)
    {
        ERROR("Cannot add snapshot for drive %s (VM %s, TA %s): %r",
              drive_name, vm_name, ta, rc);
        goto fail_vm_add_drive;
    }

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, file),
                              TE_CFG_TA_VM "/drive:%s/file:",
                              ta, vm_name, drive_name);
    if (rc != 0)
    {
        ERROR("Cannot add file for drive %s (VM %s, TA %s): %r",
              drive_name, vm_name, ta, rc);
        goto fail_vm_add_drive;
    }

    return 0;

fail_vm_add_drive:
    (void)cfg_del_instance_fmt(TRUE, TE_CFG_TA_VM "/drive:%s",
                               ta, vm_name, drive_name);

    return rc;
}
