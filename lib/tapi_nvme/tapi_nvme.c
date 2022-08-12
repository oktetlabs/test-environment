/** @file
 * @brief Control NVMeOF
 *
 * API for control NVMe Over Fabrics
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
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

#define OPTS_DEFAULTS (opts_t) {    \
    .str_stdout = NULL,             \
    .str_stderr = NULL,             \
    .timeout    = 0                 \
}

#define RUN_COMMAND_DEF_TIMEOUT (0)

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
    rpcs->timeout = opts.timeout == 0 ? TE_SEC2MS(1): opts.timeout;
    pid = rpc_waitpid(rpcs, pid, &status, 0);

    if (pid == -1)
    {
        rpc_close(rpcs, fd_stdout);
        rpc_close(rpcs, fd_stderr);
        TEST_FAIL("waitpid: %s", command);
    }

    if (opts.str_stdout != NULL)
        rpc_read_fd2te_string(rpcs, fd_stdout, 100, 0, opts.str_stdout);
    if (opts.str_stderr != NULL)
        rpc_read_fd2te_string(rpcs, fd_stderr, 100, 0, opts.str_stderr);

    rpc_close(rpcs, fd_stdout);
    rpc_close(rpcs, fd_stderr);

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

static te_errno run_command_dump_output_rc(rcf_rpc_server *rpcs,
                                           unsigned int timeout,
                                           const char *format_cmd, ...)
                                           __attribute__((format(printf, 3, 4)));

static te_errno
run_command_dump_output_rc(rcf_rpc_server *rpcs, unsigned int timeout,
                           const char *format_cmd, ...)
{
    te_errno rc;
    te_string str_stdout = TE_STRING_INIT;
    te_string str_stderr = TE_STRING_INIT;
    char command[RPC_SHELL_CMDLINE_MAX];
    opts_t run_opts = {
        .str_stdout = &str_stdout,
        .str_stderr = &str_stderr,
        .timeout = timeout,
    };
    va_list arguments;

    va_start(arguments, format_cmd);
    vsnprintf(command, sizeof(command), format_cmd, arguments);
    va_end(arguments);

    rc = run_command_generic(rpcs, (opts_t)run_opts, command);

    if (rc != 0)
    {
        ERROR("stdout:\n%s\n"
              "stderr:\n%s\n"
              "return code: %d", str_stdout.ptr, str_stderr.ptr, rc);
        rc = TE_EFAULT;
    }
    else
    {
        RING("stdout:\n%s\n"
             "stderr:\n%s", str_stdout.ptr, str_stderr.ptr);
    }

    te_string_free(&str_stdout);
    te_string_free(&str_stderr);

    return rc;
}

typedef struct initiator_dev {
    int admin_index;
    int controller_index;
    int namespace_index;
} initiator_dev;

#define INITIATOR_DEV_DEFAULTS (initiator_dev) \
{                                              \
    .admin_index = -1,                         \
    .controller_index = -1,                    \
    .namespace_index = -1,                     \
}

typedef struct initiator_dev_info {
    struct sockaddr_in addr;
    tapi_nvme_transport transport;
    char subnqn[NAME_MAX];
} initiator_dev_info;

static const char *
initiator_dev_admin_str(const initiator_dev *dev)
{
    static char admin_str[NAME_MAX];

    TE_SPRINTF(admin_str, "nvme%d", dev->admin_index);

    return admin_str;
}

static const char *
initiator_dev_ns_str(const initiator_dev *dev)
{
    static char ns_str[NAME_MAX];

    if (dev->controller_index == -1)
    {
        TE_SPRINTF(ns_str, "nvme%dn%d", dev->admin_index,
                                        dev->namespace_index);
    }
    else
    {
        TE_SPRINTF(ns_str,  "nvme%dc%dn%d", dev->admin_index,
                                            dev->controller_index,
                                            dev->namespace_index);
    }

    return ns_str;
}

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
initiator_dev_from_string(const char *str, initiator_dev *namespace_info)
{
    initiator_dev result = INITIATOR_DEV_DEFAULTS;

    if (str == NULL || namespace_info == NULL)
        return TE_EINVAL;

#define PARSE(_prefix, _value, _is_optional)                            \
    do {                                                                \
        if (!parse_with_prefix(_prefix, &str, &_value, _is_optional))   \
            return TE_EINVAL;                                           \
    } while (0)

    PARSE("nvme", result.admin_index, FALSE);
    PARSE("c", result.controller_index, TRUE);
    PARSE("n", result.namespace_index, FALSE);

#undef PARSE

    if (*str != '\0')
        return TE_EINVAL;

    *namespace_info = result;
    return 0;
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

    TE_STRLCPY(address, temp_address, INET_ADDRSTRLEN);
    *port = (unsigned short)temp_port;

    return 0;
}

static te_errno
initiator_dev_info_addr_read(rcf_rpc_server *rpcs,
                           initiator_dev_info *info,
                           const char *filepath)
{
    te_errno rc;
    char buffer[ADDRESS_INFO_SIZE];
    unsigned short port;
    char address[INET_ADDRSTRLEN];
    int size;

    size = tapi_nvme_internal_file_read(rpcs, buffer, TE_ARRAY_LEN(buffer),
                                        filepath);
    if (size < 0)
    {
        ERROR("Cannot read address info");
        return RPC_ERRNO(rpcs);
    }

    if (parse_endpoint(buffer, address, &port) != 0)
    {
        ERROR("Cannot parse address info: %s", buffer);
        return RPC_ERRNO(rpcs);
    }

    memset(&info->addr, 0, sizeof(info->addr));
    if ((rc = te_sockaddr_str2h(address, SA(&info->addr))) != 0)
        return rc;

    te_sockaddr_set_port(SA(&info->addr), htons(port));
    return 0;
}

typedef struct string_map {
    const char *string;
    tapi_nvme_transport transport;
} string_map;

static te_errno
initiator_dev_info_transport_read(rcf_rpc_server *rpcs,
                                initiator_dev_info *info,
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
        return RPC_ERRNO(rpcs);
    }

    for (i = 0; i < TE_ARRAY_LEN(map); i++)
    {
        if (strncmp(map[i].string, buffer, strlen(map[i].string)) == 0)
        {
            info->transport = map[i].transport;
            return 0;
        }
    }

    ERROR("Unsupported transport");
    return TE_EOPNOTSUPP;
}

static te_errno
initiator_dev_info_subnqn_read(rcf_rpc_server *rpcs,
                             initiator_dev_info *info,
                             const char *filepath)
{
    if (tapi_nvme_internal_file_read(rpcs,
        info->subnqn, TE_ARRAY_LEN(info->subnqn), filepath) == -1)
    {
        ERROR("Cannot read subnqn");
        return RPC_ERRNO(rpcs);
    }

    /* After read file subnqn store with \n, remove it */
    strtok(info->subnqn, "\n");

    return 0;
}

static te_errno
initiator_dev_admin_list(rcf_rpc_server *rpcs, const char *admin,
                         te_vec *devs)
{
    te_errno rc;
    te_string path = TE_STRING_INIT_STATIC(NAME_MAX);
    te_vec names = TE_VEC_INIT(tapi_nvme_internal_dirinfo);
    tapi_nvme_internal_dirinfo *dirinfo;

    if ((rc = te_string_append(&path, "%s/%s", BASE_NVME_FABRICS, admin)) != 0)
        return rc;

    rc = tapi_nvme_internal_filterdir(rpcs, path.ptr, "nvme", &names);
    if (rc != 0)
    {
        ERROR("Error during reading fabric info from %s (%r)", path.ptr, rc);
        return rc;
    }

    TE_VEC_FOREACH(&names, dirinfo)
    {
        initiator_dev dev = INITIATOR_DEV_DEFAULTS;
        if ((rc = initiator_dev_from_string(dirinfo->name, &dev)) != 0)
            break;
        if ((rc = TE_VEC_APPEND(devs, dev)) != 0)
            break;
    }

    te_vec_free(&names);
    return rc;
}

static te_errno
initiator_dev_list(rcf_rpc_server *rpcs, te_vec *devs)
{
    te_errno rc;
    te_vec names = TE_VEC_INIT(tapi_nvme_internal_dirinfo);
    tapi_nvme_internal_dirinfo *dirinfo;

    rc = tapi_nvme_internal_filterdir(rpcs, BASE_NVME_FABRICS, "nvme", &names);
    if (rc != 0)
        return rc;

    TE_VEC_FOREACH(&names, dirinfo)
    {
        if ((rc = initiator_dev_admin_list(rpcs, dirinfo->name, devs)) != 0)
            break;
    }

    te_vec_free(&names);
    return rc;
}

static te_errno
initiator_dev_info_get(rcf_rpc_server *rpcs, const initiator_dev *dev,
                       initiator_dev_info *info)
{
#define READ(_func, _file, _admin)                                          \
    do {                                                                    \
        te_errno _rc;                                                       \
        te_string _path = TE_STRING_INIT_STATIC(NAME_MAX);                  \
                                                                            \
        _rc = te_string_append(&_path, "%s/%s/%s",                          \
                               BASE_NVME_FABRICS, _admin, _file);           \
                                                                            \
        if (_rc != 0)                                                       \
            return _rc;                                                     \
                                                                            \
        if ((_rc = _func(rpcs, info, _path.ptr)) != 0)                      \
            return _rc;                                                     \
    } while(0)

    const char *admin_str = initiator_dev_admin_str(dev);

    READ(initiator_dev_info_addr_read, "address", admin_str);
    READ(initiator_dev_info_subnqn_read, "subsysnqn", admin_str);
    READ(initiator_dev_info_transport_read, "transport", admin_str);

#undef READ
    return 0;
}

static te_bool
initiator_dev_equal(const initiator_dev *first, const initiator_dev *second)
{
    return first->admin_index == second->admin_index &&
           first->controller_index == second->controller_index &&
           first->namespace_index == second->namespace_index;
}

static te_bool
initiator_dev_contains(const te_vec *devs, const initiator_dev *dev)
{
    const initiator_dev *current_dev;

    TE_VEC_FOREACH(devs, current_dev)
    {
        if (initiator_dev_equal(current_dev, dev))
            return TRUE;
    }

    return FALSE;
}

static te_errno
initiator_dev_list_diff(const te_vec *first, const te_vec *second, te_vec *diff)
{
    te_errno rc;
    const initiator_dev *dev;

    TE_VEC_FOREACH(first, dev)
    {
        if (initiator_dev_contains(second, dev))
            continue;

        if ((rc = TE_VEC_APPEND(diff, *dev)) != 0)
            return rc;
    }

    TE_VEC_FOREACH(second, dev)
    {
        if (initiator_dev_contains(first, dev))
            continue;

        if ((rc = TE_VEC_APPEND(diff, *dev)) != 0)
            return rc;
    }

    return 0;
}

static void
nvme_target2initiator_dev_info(const tapi_nvme_target *target,
                               initiator_dev_info *info)
{
    info->addr = *SIN(target->addr);
    info->transport = target->transport;
    strncpy(info->subnqn, target->subnqn, sizeof(info->subnqn));
}

static te_bool
initiator_dev_info_equal(initiator_dev_info *first, initiator_dev_info *second)
{
    return first->transport == second->transport &&
           strncmp(first->subnqn, second->subnqn, sizeof(first->subnqn)) &&
           te_sockaddrcmp(SA(&first->addr), sizeof(first->addr),
                          SA(&second->addr), sizeof(second->addr)) == 0;
}

static te_errno
get_new_device(tapi_nvme_host_ctrl *host_ctrl, const te_vec *before)
{
    te_errno rc;
    te_vec devs = TE_VEC_INIT(initiator_dev);
    te_vec diff = TE_VEC_INIT(initiator_dev);
    initiator_dev_info target;
    initiator_dev *dev;

    if ((rc = initiator_dev_list(host_ctrl->rpcs, &devs)) != 0)
    {
        te_vec_free(&devs);
        return rc;
    }

    if ((rc = initiator_dev_list_diff(before, &devs, &diff)) != 0)
    {
        te_vec_free(&devs);
        te_vec_free(&diff);
        return rc;
    }

    nvme_target2initiator_dev_info(host_ctrl->connected_target, &target);

    rc = TE_EAGAIN;
    TE_VEC_FOREACH(&diff, dev)
    {
        initiator_dev_info curinfo;

        if ((rc = initiator_dev_info_get(host_ctrl->rpcs, dev, &curinfo)) != 0)
            break;

        RING("Searching for connected device, comparing expected "
             "'%s' with '%s' from %s", te_sockaddr2str(SA(&curinfo.addr)),
             te_sockaddr2str(SA(&target.addr)), initiator_dev_admin_str(dev));

        if (initiator_dev_info_equal(&curinfo, &target) == 0)
        {
            host_ctrl->admin_dev = tapi_strdup(initiator_dev_admin_str(dev));
            host_ctrl->device = tapi_malloc(NAME_MAX);

            snprintf(host_ctrl->device, NAME_MAX, "/dev/%s",
                     initiator_dev_ns_str(dev));

            rc = 0;
            break;
        }

        rc = TE_EAGAIN;
    }

    te_vec_free(&devs);
    te_vec_free(&diff);
    return rc;
}

#define NVME_ADD_OPT(_te_str, args...)                        \
    do {                                                      \
        te_errno rc;                                          \
        if ((rc = te_string_append(_te_str, args)) != 0)      \
        {                                                     \
            te_string_free(_te_str);                          \
            return rc;                                        \
        }                                                     \
    } while(0)

typedef enum nvme_connect_type {
    NVME_CONNECT,
    NVME_CONNECT_ALL,
} nvme_connect_type;

static const char *
nvme_connect_type_str(nvme_connect_type type)
{
    static const char *nvme_connect_types[] = {
        [NVME_CONNECT] = "nvme connect",
        [NVME_CONNECT_ALL] = "nvme connect-all",
    };

    if (type < 0 || type > TE_ARRAY_LEN(nvme_connect_types))
        return NULL;

    return nvme_connect_types[type];
}

typedef struct nvme_connect_generic_opts {
    nvme_connect_type type;
    const tapi_nvme_connect_opts *tapi_opts;
} nvme_connect_generic_opts;

static te_errno
nvme_connect_build_specific_opts(te_string *str_opts,
                                 const tapi_nvme_connect_opts *opts)
{
    if (opts == NULL)
        return 0;

    if (opts->hdr_digest == TRUE)
        NVME_ADD_OPT(str_opts, "--hdr_digest ");

    if (opts->data_digest == TRUE)
        NVME_ADD_OPT(str_opts, "--data_digest ");

    if (opts->duplicate_connection == TRUE)
        NVME_ADD_OPT(str_opts, "--duplicate_connect ");

    return 0;
}

static te_errno
nvme_connect_build_opts(te_string *str_opts,
                        const tapi_nvme_target *target,
                        nvme_connect_generic_opts opts)
{
    const char *nvme_base_cmd = nvme_connect_type_str(opts.type);

    assert(nvme_base_cmd);
    NVME_ADD_OPT(str_opts, "%s ", nvme_base_cmd);
    NVME_ADD_OPT(str_opts, "--traddr=%s ", te_sockaddr_get_ipstr(target->addr));
    NVME_ADD_OPT(str_opts, "--trsvcid=%d ",
                 ntohs(te_sockaddr_get_port(target->addr)));
    NVME_ADD_OPT(str_opts, "--transport=%s ",
                 tapi_nvme_transport_str(target->transport));

    if (opts.type == NVME_CONNECT)
        NVME_ADD_OPT(str_opts, "--nqn=%s ",  target->subnqn);

    return nvme_connect_build_specific_opts(str_opts, opts.tapi_opts);
}

static te_errno
nvme_initiator_wait(tapi_nvme_host_ctrl *host_ctrl, te_vec *before)
{
    size_t i;
    te_errno rc;

    for (i = 0; i < DEVICE_WAIT_ATTEMPTS; i++)
    {
        if ((rc = get_new_device(host_ctrl, before)) != TE_EAGAIN)
            return rc;

        te_motivated_sleep(1, "Waiting device...");
    }

    if ((rc = get_new_device(host_ctrl, before)) == TE_EAGAIN)
    {
        ERROR("Connected device not found");
        return TE_ENOENT;
    }

    return rc;
}

static te_errno
nvme_initiator_connect_generic(tapi_nvme_host_ctrl *host_ctrl,
                               const tapi_nvme_target *target,
                               nvme_connect_generic_opts opts)
{
    te_errno rc;
    te_string str_stdout = TE_STRING_INIT;
    te_string str_stderr = TE_STRING_INIT;
    te_vec devs = TE_VEC_INIT(initiator_dev);
    te_string cmd = TE_STRING_INIT_STATIC(RPC_SHELL_CMDLINE_MAX);
    opts_t run_opts = {
        .str_stdout = &str_stdout,
        .str_stderr = &str_stderr,
        .timeout = TE_SEC2MS(30),
    };

    assert(host_ctrl);
    assert(host_ctrl->connected_target == NULL);
    assert(target);
    assert(target->addr);
    assert(target->addr->sa_family == AF_INET);
    assert(target->subnqn);
    assert(target->device);

    if ((rc = nvme_connect_build_opts(&cmd, target, opts)) != 0)
        return rc;

    if ((rc = initiator_dev_list(host_ctrl->rpcs, &devs)) != 0)
    {
        te_vec_free(&devs);
        return rc;
    }

    if ((rc = run_command(host_ctrl->rpcs, run_opts, "%s", cmd.ptr)) != 0)
    {
        ERROR("nvme-cli output\n"
              "stdout:\n%s\n"
              "stderr:\n%s", str_stdout.ptr, str_stderr.ptr);
        rc = TE_EFAIL;
    }
    else
    {
        host_ctrl->connected_target = target;
        RING("Success connection to target");

        rc = nvme_initiator_wait(host_ctrl, &devs);
    }

    te_string_free(&str_stdout);
    te_string_free(&str_stderr);
    te_vec_free(&devs);

    return rc;
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_connect_opts(tapi_nvme_host_ctrl *host_ctrl,
                                 const tapi_nvme_target *target,
                                 const tapi_nvme_connect_opts *opts)
{
    nvme_connect_generic_opts generic_opts = {
        .type = NVME_CONNECT,
        .tapi_opts = opts,
    };

    return nvme_initiator_connect_generic(host_ctrl, target, generic_opts);
}

te_errno
tapi_nvme_initiator_connect_all_opts(tapi_nvme_host_ctrl *host_ctrl,
                                     const tapi_nvme_target *target,
                                     const tapi_nvme_connect_opts *opts)
{
    nvme_connect_generic_opts generic_opts = {
        .type = NVME_CONNECT_ALL,
        .tapi_opts = opts,
    };

    return nvme_initiator_connect_generic(host_ctrl, target, generic_opts);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_list(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);

    return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "nvme list");
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_id_ctrl(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "nvme id-ctrl %s", host_ctrl->device);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_id_ns(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "nvme id-ns %s", host_ctrl->device);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_get_id_ns(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "nvme get-id-ns %s", host_ctrl->device);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_show_regs(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    return run_command_dump_output_rc(host_ctrl->rpcs, TE_SEC2MS(10),
                                      "nvme show-regs %s", host_ctrl->device);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_fw_log(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "nvme fw-log %s", host_ctrl->device);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_smart_log(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

   return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "nvme smart-log %s", host_ctrl->device);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_error_log(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "nvme error-log %s", host_ctrl->device);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_get_feature(tapi_nvme_host_ctrl *host_ctrl, int feature)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    return run_command_dump_output_rc(host_ctrl->rpcs, TE_SEC2MS(5),
                                      "nvme get-feature %s --feature-id=%d",
                                      host_ctrl->device, feature);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_flush(tapi_nvme_host_ctrl *host_ctrl,
                          const char *namespace)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    te_string cmd = TE_STRING_INIT_STATIC(RPC_SHELL_CMDLINE_MAX);

    NVME_ADD_OPT(&cmd, "nvme flush %s ", host_ctrl->device);
    if (namespace != NULL)
        NVME_ADD_OPT(&cmd, "--namespace-id=%s", namespace);

    return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "%s", cmd.ptr);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_reset(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    assert(host_ctrl->rpcs);
    assert(host_ctrl->device);

    return run_command_dump_output_rc(host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT,
                                      "nvme reset %s", host_ctrl->device);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_discover_from(tapi_nvme_host_ctrl *host_ctrl)
{
    te_string cmd = TE_STRING_INIT;
    const tapi_nvme_target *target;

    assert(host_ctrl);
    assert(host_ctrl->rpcs);

    if ((target = host_ctrl->connected_target) == NULL)
        TEST_FAIL("You're allowed to call discover only if "
                  "target is connected");

    NVME_ADD_OPT(&cmd, "nvme discover ");
    NVME_ADD_OPT(&cmd, "--traddr=%s ", te_sockaddr_get_ipstr(target->addr));
    NVME_ADD_OPT(&cmd, "--trsvcid=%d ",
                 ntohs(te_sockaddr_get_port(target->addr)));
    NVME_ADD_OPT(&cmd, "--transport=%s ",
                 tapi_nvme_transport_str(target->transport));

    return run_command_dump_output_rc(
        host_ctrl->rpcs, RUN_COMMAND_DEF_TIMEOUT, "%s", cmd.ptr);
}

static te_bool
is_disconnected(rcf_rpc_server *rpcs, const char *admin_dev)
{
    char path[NAME_MAX];

    TE_SPRINTF(path, BASE_NVME_FABRICS "/%s", admin_dev);

    return !tapi_nvme_internal_isdir_exist(rpcs, path);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_disconnect(tapi_nvme_host_ctrl *host_ctrl)
{
    te_errno rc;
    const unsigned int timeout_sec = 2 * 60;

    if (host_ctrl == NULL ||
        host_ctrl->rpcs == NULL ||
        host_ctrl->device == NULL ||
        host_ctrl->admin_dev == NULL ||
        host_ctrl->connected_target == NULL)
        return 0;

    RING("Device '%s' tries to disconnect", host_ctrl->admin_dev);

    rc = tapi_nvme_internal_file_append(host_ctrl->rpcs, timeout_sec, "1",
        BASE_NVME_FABRICS"/%s/delete_controller", host_ctrl->admin_dev);

    if (rc == 0 && !is_disconnected(host_ctrl->rpcs, host_ctrl->admin_dev))
        rc = TE_EFAIL;

    free(host_ctrl->device);
    free(host_ctrl->admin_dev);
    host_ctrl->device = NULL;
    host_ctrl->admin_dev = NULL;
    host_ctrl->connected_target = NULL;

    return rc;
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_disconnect_match(rcf_rpc_server *rpcs,
                                     const char *regexp)
{
    if (regexp == NULL)
        return tapi_nvme_initiator_disconnect_all(rpcs);

    return TE_EOPNOTSUPP;
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_initiator_disconnect_all(rcf_rpc_server *rpcs)
{
    assert(rpcs);

    return run_command_dump_output_rc(rpcs, TE_SEC2MS(5),
                                      "nvme disconnect-all");
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
    assert(target);
    assert(target->methods.init);

    return target->methods.init(target, opts);
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_target_setup(tapi_nvme_target *target)
{
    assert(target);
    assert(target->rpcs);
    assert(target->addr);
    assert(target->subnqn);
    assert(target->device);
    assert(target->methods.setup);

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
    assert(target);
    assert(target->rpcs);
    assert(target->device);

    run_command(target->rpcs, OPTS_DEFAULTS,
                "nvme format --ses=%d --namespace-id=%d %s",
                0, 1, target->device);

    return 0;
}
