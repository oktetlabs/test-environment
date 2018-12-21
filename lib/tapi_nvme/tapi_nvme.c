/** @file
 * @brief Control NVMeOF
 *
 * API for control NVMe Over Fabrics
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#define TE_LGR_USER "NVME TAPI"

#include "te_defs.h"
#include "te_str.h"
#include "rpc_xdr.h"
#include "te_rpc_sys_stat.h"
#include "te_rpc_types.h"
#include "te_sockaddr.h"
#include "tapi_rpc.h"
#include "tapi_rpc_dirent.h"
#include "tapi_test.h"
#include "tapi_test_log.h"
#include "te_sleep.h"
#include "tapi_mem.h"
#include "tapi_nvme_internal.h"
#include "tapi_nvme.h"

#define BASE_NVME_FABRICS       "/sys/class/nvme-fabrics/ctl"

#define MAX_NAMESPACES          (32)
#define MAX_ADMIN_DEVICES       (32)
#define TRANPORT_SIZE           (16)
#define ADDRESS_INFO_SIZE       (128)
#define BUFFER_SIZE             (128)

#define DEVICE_WAIT_ATTEMPTS    (5)
#define DEVICE_WAIT_TIMEOUT_S   (10)

typedef struct opts_t {
    te_string *str_stdout;
    te_string *str_stderr;
    unsigned int timeout;
} opts_t;

static int
run_command_generic(rcf_rpc_server *rpcs, opts_t opts, const char *command)
{
    tarpc_pid_t pid;
    int fd_stdout;
    int fd_stderr;
    rpc_wait_status status;

    te_log_stack_push("Running remove cmd: '%s'", command);

    pid = rpc_te_shell_cmd(rpcs, command, (tarpc_uid_t) -1, NULL,
                           &fd_stdout, &fd_stderr);

    if (pid == -1)
        TEST_FAIL("Cannot run command: %s", command);

    RPC_AWAIT_IUT_ERROR(rpcs);
    rpcs->timeout = opts.timeout >= 0 ? opts.timeout: TE_SEC2MS(1);
    pid = rpc_waitpid(rpcs, pid, &status, 0);

    if (pid == -1)
        TEST_FAIL("waitpid: %s", command);

    if (opts.str_stdout != NULL)
        rpc_read_fd2te_string(rpcs, fd_stdout, 100, 0, opts.str_stdout );
    if (opts.str_stderr != NULL)
        rpc_read_fd2te_string(rpcs, fd_stderr, 100, 0, opts.str_stderr);

    if (status.flag != RPC_WAIT_STATUS_EXITED)
        TEST_FAIL("Process is %s", wait_status_flag_rpc2str(status.flag));


    return status.value;
}

static int run_command(rcf_rpc_server *rpcs, opts_t opts,
                       const char *format_cmd, ...)
                       __attribute__((format(printf, 3, 4)));

static int
run_command(rcf_rpc_server *rpcs, opts_t opts, const char *format_cmd, ...)
{
    va_list arguments;
    char command[RPC_SHELL_CMDLINE_MAX];

    va_start(arguments, format_cmd);
    vsnprintf(command, sizeof(command), format_cmd, arguments);
    va_end(arguments);

    return run_command_generic(rpcs, opts, command);
}

typedef struct nvme_fabric_namespace_info {
    int admin_device_index;
    int controller_index;
    int namespace_index;
} nvme_fabric_namespace_info;

#define NVME_FABRIC_NAMESPACE_INFO_DEFAULTS (nvme_fabric_namespace_info) {  \
        .admin_device_index = -1,                                           \
        .controller_index = -1,                                             \
        .namespace_index = -1,                                              \
    }

static te_errno
nvme_fabric_namespace_info_string(nvme_fabric_namespace_info *info,
                                  te_string *string)
{
    return te_string_append(string, "nvme%dn%d",
                            info->admin_device_index,
                            info->namespace_index);
}

typedef struct nvme_fabric_info {
    struct sockaddr_in addr;
    tapi_nvme_transport transport;
    char subnqn[NAME_MAX];
    nvme_fabric_namespace_info namespaces[MAX_NAMESPACES];
    int namespaces_count;
} nvme_fabric_info;

typedef enum read_result {
    READ_SUCCESS = 0,
    READ_ERROR = -1,
    READ_CONTINUE = -2
} read_result;

typedef read_result (*read_info)(rcf_rpc_server *rpcs,
                                 nvme_fabric_info *info,
                                 const char *filepath);

static te_bool
parse_with_prefix(const char *prefix, const char **str, int *result,
                  te_bool is_optional)
{
    size_t prefix_len = strlen(prefix);

    if (strncmp(prefix, *str, prefix_len) == 0)
    {
        const char *value_str = *str + prefix_len;
        char *end = NULL;
        long value;

        value = strtol(value_str, &end, 10);
        if (value > INT_MAX || value < INT_MIN || end == value_str)
            return FALSE;

        *result = (int)value;
        *str = end;
    }
    else if (!is_optional)
        return FALSE;

    return TRUE;
}

static te_errno
parse_namespace_info(const char *str,
                     nvme_fabric_namespace_info *namespace_info)
{
    nvme_fabric_namespace_info result = NVME_FABRIC_NAMESPACE_INFO_DEFAULTS;

    if (str == NULL || namespace_info == NULL)
        return TE_EINVAL;

#define PARSE(_prefix, _value, _is_optional)                            \
    do {                                                                \
        if (!parse_with_prefix(_prefix, &str, &_value, _is_optional))   \
            return TE_EINVAL;                                           \
    } while (0)

    PARSE("nvme", result.admin_device_index, FALSE);
    PARSE("c", result.controller_index, TRUE);
    PARSE("n", result.namespace_index, FALSE);

#undef PARSE

    if (*str != '\0')
        return TE_EINVAL;

    *namespace_info = result;
    return 0;
}

static read_result
read_nvme_fabric_info_namespaces(rcf_rpc_server *rpcs,
                                 nvme_fabric_info *info,
                                 const char *filepath)
{
    int i, count;
    tapi_nvme_internal_dirinfo names[MAX_NAMESPACES];

    count = tapi_nvme_internal_filterdir(rpcs, filepath, "nvme", names,
                                         TE_ARRAY_LEN(names));
    if (count < 0)
        return READ_ERROR;

    /* NOTE Bug 9752
     *
     * It is not an error if this particular controller does not have
     * a corresponding namespace. It is a stalled NVMe connection from
     * the previous test and it should not affect current test at all.
     */
    if (count == 0)
        return READ_CONTINUE;

    info->namespaces_count = 0;
    for (i = 0; i < count; i++)
    {
        te_errno rc;

        rc = parse_namespace_info(names[i].name, info->namespaces + i);
        if (rc != 0)
            WARN("Cannot parse namespace '%s'", names[i].name);
        else
            info->namespaces_count++;
    }

    if (info->namespaces_count == 0)
        return READ_CONTINUE;

    return READ_SUCCESS;
}

/**
 * Parse address and port in format:
 * traddr=xxx.xxx.xxx.xxx,trsvcid=xxxxxx
 *
 * @param str[in]           Buffer with string for parsing
 * @param address[out]      Parsed IP Address
 * @param port[out]         Parsed port
 *
 * @note address is string with a capacity of at least
 * INET_ADDRSTRLEN characters
 *
 * @return Status code
 **/
static te_errno
parse_endpoint(char *str, char *address, unsigned short *port)
{
    te_errno rc;

    const char *traddr = "traddr=";
    const char *trsvcid = "trsvcid=";

    char *p;
    long temp_port;
    char *temp_address;

    if (strlen(str) > 0) {
        /* After read file subnqn store with \n, remove it */
        strtok(str, "\n");
    } else {
        return TE_EINVAL;
    }

    if ((p = strtok(str, ",")) == NULL ||
        strncmp(p, traddr, strlen(traddr)) != 0)
        return TE_EINVAL;

    temp_address = p + strlen(traddr);

    if ((p = strtok(NULL, ",")) == NULL ||
        strncmp(p, trsvcid, strlen(trsvcid)) != 0)
        return TE_EINVAL;

    if ((rc = te_strtol(p + strlen(trsvcid), 10, &temp_port)) != 0)
        return rc;

    TE_STRNCPY(address, INET_ADDRSTRLEN, temp_address);
    *port = (unsigned short)temp_port;

    return 0;
}

static read_result
read_nvme_fabric_info_addr(rcf_rpc_server *rpcs,
                           nvme_fabric_info *info,
                           const char *filepath)
{
    char buffer[ADDRESS_INFO_SIZE];
    unsigned short port;
    char address[INET_ADDRSTRLEN];
    int size;

    size = tapi_nvme_internal_file_read(rpcs, buffer, TE_ARRAY_LEN(buffer),
                                        filepath);
    if (size < 0)
    {
        ERROR("Cannot read address info");
        return READ_ERROR;
    }

    if (parse_endpoint(buffer, address, &port) != 0)
    {
        ERROR("Cannot parse address info: %s", buffer);
        return READ_ERROR;
    }

    memset(&info->addr, 0, sizeof(info->addr));
    if (te_sockaddr_str2h(address, SA(&info->addr)) != 0)
        return READ_ERROR;
    te_sockaddr_set_port(SA(&info->addr), htons(port));

    return READ_SUCCESS;
}

typedef struct string_map {
    const char *string;
    tapi_nvme_transport transport;
} string_map;

static read_result
read_nvme_fabric_info_transport(rcf_rpc_server *rpcs,
                                nvme_fabric_info *info,
                                const char *filepath)
{
    size_t i;
    char buffer[TRANPORT_SIZE];
    string_map map[] = {
        TAPI_NVME_TRANSPORT_MAPPING_LIST
    };

    if (tapi_nvme_internal_file_read(rpcs, buffer, TE_ARRAY_LEN(buffer),
                                     filepath) < 0)
    {
        ERROR("Cannot read transport");
        return READ_ERROR;
    }

    for (i = 0; i < TE_ARRAY_LEN(map); i++)
    {
        if (strncmp(map[i].string, buffer, strlen(map[i].string)) == 0)
        {
            info->transport = map[i].transport;
            return READ_SUCCESS;
        }
    }

    ERROR("Unsupported transport");
    return READ_ERROR;
}

static read_result
read_nvme_fabric_info_subnqn(rcf_rpc_server *rpcs,
                             nvme_fabric_info *info,
                             const char *filepath)
{
    if (tapi_nvme_internal_file_read(rpcs,
        info->subnqn, TE_ARRAY_LEN(info->subnqn), filepath) == -1)
    {
        ERROR("Cannot read subnqn");
        return READ_ERROR;
    }

    /* After read file subnqn store with \n, remove it */
    strtok(info->subnqn, "\n");

    return READ_SUCCESS;
}

typedef struct read_nvme_fabric_read_action {
    read_info function;
    const char *file;
} read_nvme_fabric_read_action;

static read_result
read_nvme_fabric_info(rcf_rpc_server *rpcs,
                      nvme_fabric_info *info,
                      const char *admin_dev)
{
    size_t i;
    read_result rc;
    char path[RCF_MAX_PATH];
    read_nvme_fabric_read_action actions[] = {
        { read_nvme_fabric_info_namespaces, "" },
        { read_nvme_fabric_info_addr, "address" },
        { read_nvme_fabric_info_subnqn, "subsysnqn" },
        { read_nvme_fabric_info_transport, "transport" }
    };

    for (i = 0; i < TE_ARRAY_LEN(actions); i++)
    {
        TE_SPRINTF(path, BASE_NVME_FABRICS "/%s/%s",
                   admin_dev, actions[i].file);
        if ((rc = actions[i].function(rpcs, info, path)) != READ_SUCCESS)
            return rc;
    }

    return READ_SUCCESS;
}

static te_bool
is_target_eq(const tapi_nvme_target *target, const nvme_fabric_info *info,
             const char *admin_dev)
{
    const struct sockaddr_in *addr1 = SIN(target->addr);
    const struct sockaddr_in *addr2 = SIN(&info->addr);
    te_bool subnqn_eq;
    te_bool addr_eq;
    te_bool transport_eq;

    RING("Searching for connected device, comparing expected "
         "'%s' with '%s' from %s",
         te_sockaddr2str(SA(addr1)), te_sockaddr2str(SA(addr2)),
         admin_dev);

    transport_eq = target->transport == info->transport;
    subnqn_eq = strcmp(target->subnqn, info->subnqn) == 0;
    addr_eq = te_sockaddrcmp(SA(addr1), sizeof(*addr1),
                             SA(addr2), sizeof(*addr2)) == 0;

    return transport_eq && subnqn_eq && addr_eq;
}

static te_errno
get_new_device(tapi_nvme_host_ctrl *host_ctrl,
               const tapi_nvme_target *target)
{
    int i, count;
    read_result rc;
    te_string device_str = TE_STRING_INIT;

    nvme_fabric_info info;
    tapi_nvme_internal_dirinfo names[MAX_ADMIN_DEVICES];

    count = tapi_nvme_internal_filterdir(host_ctrl->rpcs, BASE_NVME_FABRICS,
                                         "nvme", names, TE_ARRAY_LEN(names));

    for (i = 0; i < count; i++)
    {
        rc = read_nvme_fabric_info(host_ctrl->rpcs, &info, names[i].name);
        if (rc == READ_ERROR)
            return TE_ENODATA;
        else if (rc == READ_CONTINUE)
            continue;
        if (is_target_eq(target, &info, names[i].name) == TRUE)
        {
            te_errno error;

            /* Currently we are configuring only 1 namespace per subsystem
             * while using kernel targets. We have no control over onvme
             * namespace configuration right now, but it also doing in the
             * same way. So appearance of more than 1 namespace per NVMoF
             * device would be very suspicious and highly likely a bug.
             * Fail the entire test for further investigation.
             */
            if (info.namespaces_count > 1)
            {
                TEST_FAIL("We found %d namespaces for %s device, "
                          "but only one was expected",
                          info.namespaces_count, host_ctrl->admin_dev);
            }

            host_ctrl->admin_dev = tapi_strdup(names[i].name);
            host_ctrl->device = tapi_malloc(NAME_MAX);

            error = nvme_fabric_namespace_info_string(&info.namespaces[0],
                                                      &device_str);
            if (error != 0)
                return error;

            snprintf(host_ctrl->device, NAME_MAX, "/dev/%s", device_str.ptr);
            return 0;
        }
    }

    return TE_ENODATA;
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_connect(tapi_nvme_host_ctrl *host_ctrl,
                            const tapi_nvme_target *target)
{
    int i, rc;
    te_string str_stdout = TE_STRING_INIT;
    te_string str_stderr = TE_STRING_INIT;
    char cmd[RPC_SHELL_CMDLINE_MAX] = "nvme connect ";
    const char *opts = "--traddr=%s "
                       "--trsvcid=%d "
                       "--transport=%s "
                       "--nqn=%s ";
    opts_t run_opts = {
        .str_stdout = &str_stdout,
        .str_stderr = &str_stderr,
        .timeout = TE_SEC2MS(30),
    };

    if (host_ctrl == NULL)
        return TE_EINVAL;

    if (host_ctrl->connected_target != NULL)
        return TE_EISCONN;

    if (target == NULL ||
        target->addr == NULL ||
        target->addr->sa_family != AF_INET ||
        target->subnqn == NULL ||
        target->device == NULL)
    {
        te_log_stack_push("%s: something is invalid in our life: target=%p "
                          "target->addr=%p sa_family=%u "
                          "target->subnqn=%p target->device=%p",
                          target, target->addr,
                          target->addr ? target->addr->sa_family : -1,
                          target->subnqn, target->device);
        return TE_EINVAL;
    }

    rc = run_command(host_ctrl->rpcs, run_opts, strcat(cmd, opts),
                     te_sockaddr_get_ipstr(target->addr),
                     ntohs(te_sockaddr_get_port(target->addr)),
                     tapi_nvme_transport_str(target->transport),
                     target->subnqn);

    if (rc != 0)
    {
        ERROR("nvme-cli output\n"
              "stdout:\n%s\n"
              "stderr:\n%s",
              str_stdout.ptr, str_stderr.ptr);
        return rc;
    }

    host_ctrl->connected_target = target;
    RING("Success connection to target");

    (void)tapi_nvme_initiator_list(host_ctrl);

    for (i = 0; i < DEVICE_WAIT_ATTEMPTS; i++)
    {
        rc = get_new_device(host_ctrl, target);
        if (rc == 0)
            return 0;

        te_motivated_sleep(1, "Waiting device...");
    }

    rc = get_new_device(host_ctrl, target);
    if (rc != 0)
    {
        ERROR("Connected device not found");
        return TE_ENOENT;
    }

    return 0;
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_list(tapi_nvme_host_ctrl *host_ctrl)
{
    int rc;
    te_string str_stdout = TE_STRING_INIT;
    te_string str_stderr = TE_STRING_INIT;
    opts_t run_opts = {
        .str_stdout = &str_stdout,
        .str_stderr = &str_stderr,
    };

    rc = run_command(host_ctrl->rpcs, run_opts, "nvme list");

    if (rc != 0)
        WARN("stderr:\n%s", str_stderr.ptr);
    else
        RING("stdout:\n%s\nstderr:\n%s", str_stdout.ptr, str_stderr.ptr);

    return rc == 0 ? 0 : TE_EFAULT;
}

static te_bool
is_disconnected(rcf_rpc_server *rpcs, const char *admin_dev)
{
    char path[NAME_MAX];

    TE_SPRINTF(path, BASE_NVME_FABRICS "/%s", admin_dev);

    return !tapi_nvme_internal_isdir_exist(rpcs, path);
}

static te_errno
wait_device_disapperance(rcf_rpc_server *rpcs, const char *admin_dev)
{
    char why_message[BUFFER_SIZE];
    unsigned int wait_sec = DEVICE_WAIT_TIMEOUT_S;
    unsigned int i;

    if (is_disconnected(rpcs, admin_dev))
        return 0;

    for (i = 1; i <= DEVICE_WAIT_ATTEMPTS; i++)
    {
        TE_SPRINTF(why_message,
                   "[%d/%d] Waiting for disconnecting device '%s'...",
                   i, DEVICE_WAIT_ATTEMPTS, admin_dev);

        te_motivated_sleep(wait_sec, why_message);

        if (is_disconnected(rpcs, admin_dev))
            return 0;

        /* If NVMoF TCP kernel initiator driver will not free the device
         * withing a few seconds interval, then it will hang up for a long
         * time, thus there are no point in high frequency polling. */
        wait_sec += DEVICE_WAIT_TIMEOUT_S;
    }

    return TE_RC(TE_TAPI, TE_ETIMEDOUT);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_disconnect(tapi_nvme_host_ctrl *host_ctrl)
{
    te_errno rc;

    if (host_ctrl == NULL ||
        host_ctrl->rpcs == NULL ||
        host_ctrl->device == NULL ||
        host_ctrl->admin_dev == NULL ||
        host_ctrl->connected_target == NULL)
        return 0;

    RING("Device '%s' tries to disconnect", host_ctrl->admin_dev);

    rc = tapi_nvme_internal_file_append(host_ctrl->rpcs, "1",
        BASE_NVME_FABRICS"/%s/delete_controller", host_ctrl->admin_dev);
    if (rc == 0)
        rc = wait_device_disapperance(host_ctrl->rpcs, host_ctrl->admin_dev);
    if (rc == 0)
        RING("Device '%s' disconnected successfully", host_ctrl->admin_dev);
    else
        ERROR("Disconnection for device '%s' failed", host_ctrl->admin_dev);

    free(host_ctrl->device);
    free(host_ctrl->admin_dev);
    host_ctrl->device = NULL;
    host_ctrl->admin_dev = NULL;
    host_ctrl->connected_target = NULL;

    return rc;
}

/* See description in tapi_nvme.h */
const char *
tapi_nvme_transport_str(tapi_nvme_transport transport) {
    static const char *transports[] = {
        [TAPI_NVME_TRANSPORT_RDMA] = "rdma",
        [TAPI_NVME_TRANSPORT_TCP] = "tcp",
    };

    if (transport < 0 || transport > TE_ARRAY_LEN(transports))
        return NULL;
    return transports[transport];
};

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_target_init(tapi_nvme_target *target, void *opts)
{
    if (target == NULL || target->methods.init == NULL)
        return TE_EINVAL;
    return target->methods.init(target, opts);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_target_setup(tapi_nvme_target *target)
{
    if (target == NULL ||
        target->rpcs == NULL ||
        target->addr == NULL ||
        target->subnqn == NULL ||
        target->device == NULL ||
        target->methods.setup == NULL)
        return TE_EINVAL;

    return target->methods.setup(target);
}

/* See description in tapi_nvme.h */
void
tapi_nvme_target_cleanup(tapi_nvme_target *target)
{
    if (target == NULL ||
        target->rpcs == NULL ||
        target->addr == NULL ||
        target->subnqn == NULL ||
        target->methods.cleanup == NULL)
        return;

    target->methods.cleanup(target);
}

/* See description in tapi_nvme.h */
void
tapi_nvme_target_fini(tapi_nvme_target *target)
{
    tapi_nvme_target_cleanup(target);
    if (target == NULL || target->methods.fini == NULL)
        return;
    target->methods.fini(target);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_target_format(tapi_nvme_target *target)
{
    if (target == NULL ||
        target->rpcs == NULL ||
        target->device == NULL)
        return TE_EINVAL;

    run_command(target->rpcs, (opts_t){},
                "nvme format --ses=%d --namespace-id=%d %s",
                0, 1, target->device);

    return 0;
}
