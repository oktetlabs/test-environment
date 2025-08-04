/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Spdk target
 *
 * API for control Spdk target of NVMe Over Fabrics
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "NVME SPDK Target"

#include "te_defs.h"
#include "te_alloc.h"
#include "te_log_stack.h"
#include "te_sockaddr.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_misc.h"
#include "tapi_test.h"
#include "te_vector.h"

#include "tapi_nvme_spdk_target.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_spdk_rpc.h"

#define SPDK_PROC_INIT_TIMEOUT (2)

/* See description in tapi_nvme_spdk_target.h */
te_errno
tapi_nvme_spdk_target_init(struct tapi_nvme_target *target, void *opts)
{
    tapi_spdk_rpc_server_opt    server_opt = tapi_spdk_rpc_server_default_opt;
    tapi_nvme_spdk_target_opts *spdk_tgt_opts = opts;
    tapi_job_factory_t         *factory = NULL;
    tapi_spdk_rpc_app          *app = NULL;
    te_errno                    rc;

    tapi_job_factory_rpc_create(target->rpcs, &factory);
    rc = tapi_spdk_rpc_create(factory, spdk_tgt_opts->rpc_path, &server_opt,
                              &app);
    if (rc != 0)
    {
        ERROR("Failed to create spdk rpc APP");
        return rc;
    }

    target->impl = app;

    return 0;
}

static te_errno
nvme_target2spdk_nvmf_transport(tapi_nvme_transport                in,
                                tapi_spdk_rpc_nvmf_transport_type *out)
{
    te_errno rc = 0;

    switch (in)
    {
        case TAPI_NVME_TRANSPORT_TCP:
            *out = TAPI_SPDK_RPC_NVMF_TRANSPORT_TYPE_TCP;
            break;

        default:
            rc = TE_EOPNOTSUPP;
            break;
    }

    return rc;
}

static te_errno
create_transport(tapi_spdk_rpc_app *app, tapi_nvme_target *target)
{
    te_errno rc;

    tapi_spdk_rpc_nvmf_create_transport_opt opt = {0};

    rc = nvme_target2spdk_nvmf_transport(target->transport, &opt.type);
    if (rc != 0)
    {
        return rc;
    }

    opt.zero_copy_recv = true;

    return tapi_spdk_rpc_nvmf_create_transport(app, &opt);
}

static te_errno
create_subsystem(tapi_spdk_rpc_app *app, tapi_nvme_target *target)
{
    tapi_spdk_rpc_nvmf_create_subsystem_opt opt = {0};

    opt.nqn = (const char *)target->subnqn;
    opt.serial_number = target->serial_number;
    opt.allow_any_host = true;
    opt.ana_reporting = true;

    return tapi_spdk_rpc_nvmf_create_subsystem(app, &opt);
}

static te_errno
delete_subsystem(tapi_spdk_rpc_app *app, tapi_nvme_target *target)
{
    tapi_spdk_rpc_nvmf_delete_subsystem_opt opt = {0};

    opt.nqn = (const char *)target->subnqn;

    return tapi_spdk_rpc_nvmf_delete_subsystem(app, &opt);
}

static te_errno
add_ns(tapi_spdk_rpc_app *app, tapi_nvme_target *target)
{
    tapi_spdk_rpc_nvmf_subsystem_add_ns_opt opt = {0};

    opt.nqn = (const char *)target->subnqn;
    opt.bdev_name = target->device;
    opt.ns_id = target->ns_id;

    return tapi_spdk_rpc_nvmf_subsystem_add_ns(app, &opt);
}

static te_errno
delete_ns(tapi_spdk_rpc_app *app, tapi_nvme_target *target)
{
    tapi_spdk_rpc_nvmf_subsystem_remove_ns_opt opt = {0};

    opt.nqn = (const char *)target->subnqn;
    opt.ns_id = target->ns_id;

    return tapi_spdk_rpc_nvmf_subsystem_remove_ns(app, &opt);
}

static te_errno
add_listener(tapi_spdk_rpc_app *app, tapi_nvme_target *target)
{
    tapi_spdk_rpc_nvmf_subsystem_add_listener_opt opt = {0};
    te_errno                                      rc;

    opt.nqn = (const char *)target->subnqn;

    rc = nvme_target2spdk_nvmf_transport(target->transport, &opt.type);
    if (rc != 0)
    {
        return rc;
    }

    if (target->addr->sa_family != AF_INET)
    {
        return TE_EOPNOTSUPP;
    }

    opt.adrfam = TAPI_SPDK_RPC_NVMF_TRANSPORT_ADRFAM_TYPE_IP4;
    opt.address = te_ip2str(target->addr);
    opt.trsvcid = ntohs(te_sockaddr_get_port(target->addr));

    return tapi_spdk_rpc_nvmf_subsystem_add_listener(app, &opt);
}

static te_errno
remove_listener(tapi_spdk_rpc_app *app, tapi_nvme_target *target)
{
    tapi_spdk_rpc_nvmf_subsystem_remove_listener_opt opt = {0};
    te_errno                                         rc;

    opt.nqn = (const char *)target->subnqn;

    rc = nvme_target2spdk_nvmf_transport(target->transport, &opt.type);
    if (rc != 0)
    {
        return rc;
    }

    if (target->addr->sa_family != AF_INET)
    {
        return TE_EOPNOTSUPP;
    }

    opt.adrfam = TAPI_SPDK_RPC_NVMF_TRANSPORT_ADRFAM_TYPE_IP4;
    opt.address = te_ip2str(target->addr);
    opt.trsvcid = ntohs(te_sockaddr_get_port(target->addr));

    return tapi_spdk_rpc_nvmf_subsystem_remove_listener(app, &opt);
}

/* See description in tapi_nvme_spdk_target.h */
te_errno
tapi_nvme_spdk_target_setup(tapi_nvme_target *target)
{
    te_errno           rc;
    tapi_spdk_rpc_app *app = target->impl;

    te_log_stack_push("Spdk target setup start");

    rc = create_transport(app, target);
    if (rc != 0)
    {
        ERROR("Failed to create SPDK transport");
        return rc;
    }

    rc = create_subsystem(app, target);
    if (rc != 0)
    {
        ERROR("Failed to create SPDK NVMf subsystem");
        return rc;
    }

    rc = add_ns(app, target);
    if (rc != 0)
    {
        ERROR("Failed to add ns to SPDK NVMf");
        return rc;
    }

    rc = add_listener(app, target);
    if (rc != 0)
    {
        ERROR("Failed to add listener to SPDK NVMf");
        return rc;
    }

    te_motivated_sleep(SPDK_PROC_INIT_TIMEOUT,
                       "Give target a while to start");

    return 0;
}

/* See description in tapi_nvme_spdk_target.h */
void
tapi_nvme_spdk_target_cleanup(tapi_nvme_target *target)
{
    te_errno           rc;
    tapi_spdk_rpc_app *app = target->impl;

    rc = remove_listener(app, target);
    if (rc != 0)
    {
        ERROR("Failed to remove listener from SPDK NVMf");
    }

    rc = delete_ns(app, target);
    if (rc != 0)
    {
        ERROR("Failed to delete ns from SPDK NVMf");
    }

    rc = delete_subsystem(app, target);
    if (rc != 0)
    {
        ERROR("Failed to delete SPDK NVMf subsystem");
    }

    return;
}

/* See description in tapi_nvme_spdk_target.h */
void
tapi_nvme_spdk_target_fini(tapi_nvme_target *target)
{
    tapi_spdk_rpc_app *app = target->impl;

    tapi_spdk_rpc_destroy(app);
    return;
}
