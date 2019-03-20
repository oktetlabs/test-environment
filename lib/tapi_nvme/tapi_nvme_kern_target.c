/** @file
 * @brief Kernel target
 *
 * API for control kernel target of NVMe Over Fabrics
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#define TE_LGR_USER "NVME Kernel Target"

#include "te_defs.h"
#include "te_log_stack.h"
#include "te_sockaddr.h"

#include "tapi_rpc_unistd.h"
#include "tapi_nvme_internal.h"
#include "tapi_nvme.h"
#include "tapi_nvme_kern_target.h"

#define BASE_NVMET_CONFIG   "/sys/kernel/config/nvmet"
#define PORT_STRLEN         (11)

te_errno
tapi_nvme_kern_target_init(struct tapi_nvme_target *target, void *opts)
{
    UNUSED(target);
    UNUSED(opts);
    return 0;
}

static te_errno
create_directories(rcf_rpc_server *rpcs, tapi_nvme_subnqn nqn, int nvmet_port)
{
    te_log_stack_push("Create target directories for nqn=%s port=%d",
                      nqn, nvmet_port);

#define MKDIR(_fmt, _args...)                           \
    do {                                                \
        if (tapi_nvme_internal_mkdir(rpcs,              \
            BASE_NVMET_CONFIG _fmt, _args) == FALSE)    \
            return rpcs->_errno;                        \
    } while (0)

    MKDIR("/subsystems/%s", nqn);
    MKDIR("/subsystems/%s/namespaces/%d", nqn, nvmet_port);
    MKDIR("/ports/%d", nvmet_port);
#undef MKDIR

    return 0;
}

static te_errno
write_config(rcf_rpc_server *rpcs, tapi_nvme_transport transport,
             const char *device, const struct sockaddr *addr,
             int port, tapi_nvme_subnqn nqn)
{
    char pstr[PORT_STRLEN];
    const char *tstr = tapi_nvme_transport_str(transport);
    const char *astr = te_sockaddr_get_ipstr(addr);

    te_log_stack_push("Writing target config for device=%s port=%d",
                      device, port);

    TE_SPRINTF(pstr, "%hu", ntohs(te_sockaddr_get_port(addr)));

#define APPEND(_value, _fmt, _args...)                          \
    do {                                                        \
        int rc;                                                 \
        rc = tapi_nvme_internal_file_append(                    \
            rpcs, TAPI_NVME_INTERNAL_DEF_TIMEOUT, _value,       \
            BASE_NVMET_CONFIG _fmt, _args);                     \
        if (rc != 0)                                            \
            return rc;                                          \
    } while (0)

    APPEND(device, "/subsystems/%s/namespaces/%d/device_path", nqn, port);
    APPEND("1",    "/subsystems/%s/namespaces/%d/enable", nqn, port);
    APPEND("1",    "/subsystems/%s/attr_allow_any_host", nqn);
    APPEND("ipv4", "/ports/%d/addr_adrfam", port);
    APPEND(tstr,   "/ports/%d/addr_trtype", port);
    APPEND(pstr,   "/ports/%d/addr_trsvcid", port);
    APPEND(astr,   "/ports/%d/addr_traddr", port);
#undef APPEND

    return 0;
}

static te_errno
make_namespace_target_available(rcf_rpc_server *rpcs,
                                tapi_nvme_subnqn nqn, int port)
{
    char path1[RCF_MAX_PATH];
    char path2[RCF_MAX_PATH];

    TE_SPRINTF(path1, BASE_NVMET_CONFIG "/subsystems/%s", nqn);
    TE_SPRINTF(path2, BASE_NVMET_CONFIG "/ports/%d/subsystems/%s", port, nqn);

    RPC_AWAIT_IUT_ERROR(rpcs);
    return rpc_symlink(rpcs, path1, path2) == 0 ? 0: rpcs->_errno;
}

void
tapi_nvme_kern_target_cleanup(tapi_nvme_target *target)
{
    char path[RCF_MAX_PATH];

    te_log_stack_push("Kernel target cleanup start");
    TE_SPRINTF(path, BASE_NVMET_CONFIG "/ports/%d/subsystems/%s",
        target->nvmet_port, target->subnqn);

    RPC_AWAIT_IUT_ERROR(target->rpcs);
    if (rpc_access(target->rpcs, path, RPC_F_OK) == 0)
    {
        RPC_AWAIT_IUT_ERROR(target->rpcs);
        rpc_unlink(target->rpcs, path);
    }

#define RMDIR(_fmt, _args...)   \
        tapi_nvme_internal_rmdir(target->rpcs, BASE_NVMET_CONFIG _fmt, _args)

    RMDIR("/subsystems/%s/namespaces/%d", target->subnqn, target->nvmet_port);
    RMDIR("/subsystems/%s", target->subnqn);
    RMDIR("/ports/%d", target->nvmet_port);
#undef RMDIR
}

te_errno
tapi_nvme_kern_target_setup(tapi_nvme_target *target)
{
    te_errno rc;

    te_log_stack_push("Kernel target setup start");
    rc = create_directories(target->rpcs, target->subnqn, target->nvmet_port);
    if (rc != 0)
        return rc;

    rc = write_config(target->rpcs, target->transport, target->device,
                      target->addr, target->nvmet_port, target->subnqn);
    if (rc != 0)
        return rc;

    return make_namespace_target_available(target->rpcs, target->subnqn,
                                           target->nvmet_port);
}

void
tapi_nvme_kern_target_fini(struct tapi_nvme_target *target)
{
    UNUSED(target);
}
