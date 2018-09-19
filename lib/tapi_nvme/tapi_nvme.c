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
#include "tapi_test_log.h"
#include "te_sleep.h"
#include "tapi_mem.h"
#include "tapi_nvme.h"

#define DEFAULT_MODE \
    (RPC_S_IRWXU | RPC_S_IRGRP | RPC_S_IXGRP | RPC_S_IROTH | RPC_S_IXOTH)

#define BASE_NVMET_CONFIG   "/sys/kernel/config/nvmet"
#define BASE_NVME_FABRICS   "/sys/class/nvme-fabrics/ctl"
#define ENABLE              "1"

#define PORT_SIZE           NAME_MAX
#define DIRECTORY_SIZE      NAME_MAX
#define SUBNQN_SIZE         NAME_MAX
#define NAMESPACE_SIZE      (32)
#define MAX_NAMESPACES      (32)
#define MAX_ADMIN_DEVICES   (32)
#define TRANPORT_SIZE       (16)
#define ADDRESS_INFO_SIZE   (128)

#define BUFFER_SIZE         (128)

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

typedef struct directory_name {
    char name[DIRECTORY_SIZE];
} directory_name;

static int
filter_directory(rcf_rpc_server *rpcs,
                 const char * path,
                 const char * start_from,
                 directory_name *directory_names,
                 int count_names)
{
    int count = 0;
    rpc_dir_p dir;
    rpc_dirent *dirent = NULL;

    RPC_AWAIT_IUT_ERROR(rpcs);
    if ((dir = rpc_opendir(rpcs, path)) == RPC_NULL)
        return -1;

    while (count < count_names) {
        RPC_AWAIT_IUT_ERROR(rpcs);
        if ((dirent = rpc_readdir(rpcs, dir)) == NULL)
            break;

        if (strstr(dirent->d_name, start_from) == dirent->d_name)
            strcpy(directory_names[count++].name, dirent->d_name);
    }

    if (rpc_readdir(rpcs, dir) != NULL)
        WARN("Too many of the directories found");

    rpc_closedir(rpcs, dir);
    return count;
}

static int
read_file(rcf_rpc_server *rpcs, const char *path, char *buffer, size_t size)
{
    int fd;

    RPC_AWAIT_IUT_ERROR(rpcs);
    if ((fd = rpc_open(rpcs, path, RPC_O_RDONLY, DEFAULT_MODE)) == -1)
        return -1;


    RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_read(rpcs, fd, buffer, size) == -1)
    {
        ERROR("Cannot read file %s", path);
        RPC_AWAIT_IUT_ERROR(rpcs);
        rpc_close(rpcs, fd);
        return -1;
    }
    else
    {
        strtok(buffer, "\n");

        RPC_AWAIT_IUT_ERROR(rpcs);
        return rpc_close(rpcs, fd);
    }
}

typedef struct nvme_fabric_info {
    struct sockaddr_in addr;
    tapi_nvme_transport transport;
    char subnqn[SUBNQN_SIZE];
    char namespace[NAMESPACE_SIZE];    /* TODO: it is list in future */
} nvme_fabric_info;

typedef enum read_result {
    READ_SUCCESS = 0,
    READ_ERROR = -1,
    READ_CONTINUE = -2
} read_result;

typedef read_result (*read_info)(rcf_rpc_server *rpcs,
                                 nvme_fabric_info *info,
                                 const char *filepath);

static read_result
read_nvme_fabric_info_namespace(rcf_rpc_server *rpcs,
                                nvme_fabric_info *info,
                                const char *filepath)
{
    int count;
    directory_name names[MAX_NAMESPACES];

    count = filter_directory(rpcs, filepath, "nvme",
                             names, TE_ARRAY_LEN(names));
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

    return strcpy(info->namespace, names[0].name) != NULL ?
           READ_SUCCESS: READ_ERROR;
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

    if (read_file(rpcs, filepath, buffer, TE_ARRAY_LEN(buffer)))
    {
        ERROR("Cannot read address info");
        return READ_ERROR;
    }

    if (parse_endpoint(buffer, address, &port) != 0)
    {
        ERROR("Cannot parse address info");
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

    if (read_file(rpcs, filepath, buffer, TE_ARRAY_LEN(buffer)) != 0)
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
    if (read_file(rpcs, filepath, info->subnqn,
                  TE_ARRAY_LEN(info->subnqn)) == -1)
    {
        ERROR("Cannot read subnqn");
        return READ_ERROR;
    }

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
        { read_nvme_fabric_info_namespace, "" },
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

    nvme_fabric_info info;
    directory_name names[MAX_ADMIN_DEVICES];

    count = filter_directory(host_ctrl->rpcs, BASE_NVME_FABRICS,
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
            host_ctrl->admin_dev = tapi_strdup(names[i].name);
            host_ctrl->device = tapi_malloc(NAME_MAX);
            snprintf(host_ctrl->device, NAME_MAX, "/dev/%s", info.namespace);

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
        RING("nvme-cli output\n"
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

te_errno tapi_nvme_initiator_list(tapi_nvme_host_ctrl *host_ctrl)
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
        WARN("stderr: \n%s", str_stderr.ptr);
    else
        RING("stdout:\n%s\nstderr:\n%s", str_stdout.ptr, str_stderr.ptr);

    return rc == 0 ? 0 : TE_EFAULT;
}

static te_errno
file_append(rcf_rpc_server *rpcs, const char *path, const char *string)
{
    int fd;

    RPC_AWAIT_IUT_ERROR(rpcs);
    fd = rpc_open(rpcs, path, RPC_O_CREAT | RPC_O_APPEND | RPC_O_WRONLY,
                  DEFAULT_MODE);

    if (fd == -1)
        return rpcs->_errno;

    RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_write(rpcs, fd, string, strlen(string)) == -1)
        return rpcs->_errno;

    RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_close(rpcs, fd) == -1)
        return rpcs->_errno;

    return 0;
}

static te_bool
is_dir_exist(rcf_rpc_server *rpcs, const char *path)
{
    rpc_dir_p dir;

    RPC_AWAIT_IUT_ERROR(rpcs);
    dir = rpc_opendir(rpcs, path);
    if (dir != RPC_NULL)
    {
        RPC_AWAIT_IUT_ERROR(rpcs);
        rpc_closedir(rpcs, dir);
        return TRUE;
    }

    return FALSE;
}

static te_bool
is_disconnected(rcf_rpc_server *rpcs, const char *admin_dev)
{
    char path[NAME_MAX];

    TE_SPRINTF(path, BASE_NVME_FABRICS "/%s", admin_dev);

    return !is_dir_exist(rpcs, path);
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
    char delete_controller[NAME_MAX];

    if (host_ctrl == NULL ||
        host_ctrl->rpcs == NULL ||
        host_ctrl->device == NULL ||
        host_ctrl->admin_dev == NULL ||
        host_ctrl->connected_target == NULL)
        return 0;

    TE_SPRINTF(delete_controller, BASE_NVME_FABRICS "/%s/delete_controller",
               host_ctrl->admin_dev);

    RING("Device '%s' tries to disconnect", host_ctrl->admin_dev);

    rc = file_append(host_ctrl->rpcs, delete_controller, ENABLE);
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

static char *
strrep(const char *source, const char *template,
       const char *replace, char *result)
{
    const char *start = source;
    const char *found = NULL;
    char *current = result;

    while ((found = strstr(start, template)) != NULL)
    {
        strncpy(current, start, found - start);
        current += found - start;
        strcpy(current, replace);
        current += strlen(replace);
        start = found + strlen(template);
    }

    strcpy(current, start);

    return result;
}

static char *
replace_port(const char *source, int nvmet_port, char *buffer)
{
    char port_str[16];

    TE_SPRINTF(port_str, "%d", nvmet_port);
    strrep(source, "{port}", port_str, buffer);

    return buffer;
}

static te_bool
try_mkdir(rcf_rpc_server *rpcs, const char *path)
{
    int rc;

    RING("Creating dir '%s'", path);
    RPC_AWAIT_IUT_ERROR(rpcs);
    rc = rpc_mkdir(rpcs, path, DEFAULT_MODE);

    if (rc == -1 && rpcs->_errno == TE_EEXIST)
        return TRUE;

    return rc == 0;
}

static te_errno
create_directories(rcf_rpc_server *rpcs, tapi_nvme_subnqn nqn, int nvmet_port)
{
    size_t i;
    char path[RCF_MAX_PATH];
    char buffer[RCF_MAX_PATH];
    const char *directories[] = {
        "%s/subsystems/%s",
        "%s/subsystems/%s/namespaces/{port}",
        "%s/ports/{port}",
    };

    te_log_stack_push("Create target directories for nqn=%s port=%d",
                      nqn, nvmet_port);

    for (i = 0; i < TE_ARRAY_LEN(directories); i++)
    {
        replace_port(directories[i], nvmet_port, buffer);
        TE_SPRINTF(path, buffer, BASE_NVMET_CONFIG, nqn);
        if (try_mkdir(rpcs, path) == FALSE)
            return rpcs->_errno;
    }

    return 0;
}

typedef struct nvme_target_config {
    const char *prefix;
    const char *value;
} nvme_target_config;

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

static te_errno
write_config(rcf_rpc_server *rpcs, tapi_nvme_transport transport,
             const char *device, const struct sockaddr *addr,
             int nvmet_port, tapi_nvme_subnqn nqn)
{
    size_t i;
    te_errno rc;
    char port[PORT_SIZE];
    char path[RCF_MAX_PATH];
    char buffer[RCF_MAX_PATH];
    const char *transport_str = tapi_nvme_transport_str(transport);
    nvme_target_config configs[] = {
        { "%s/subsystems/%s/namespaces/{port}/device_path", device },
        { "%s/subsystems/%s/namespaces/{port}/enable", ENABLE },
        { "%s/subsystems/%s/attr_allow_any_host", ENABLE },
        { "%s/ports/{port}/addr_adrfam", "ipv4" },
        { "%s/ports/{port}/addr_trtype", transport_str},
        { "%s/ports/{port}/addr_trsvcid", port },
        { "%s/ports/{port}/addr_traddr", te_sockaddr_get_ipstr(addr) }
    };

    te_log_stack_push("Writing target config for device=%s port=%d",
                      device, nvmet_port);

    TE_SPRINTF(port, "%hu", ntohs(te_sockaddr_get_port(addr)));

    for (i = 0; i < TE_ARRAY_LEN(configs); i++)
    {
        replace_port(configs[i].prefix, nvmet_port, buffer);
        TE_SPRINTF(path, buffer, BASE_NVMET_CONFIG, nqn);
        if ((rc = file_append(rpcs, path, configs[i].value)) != 0)
            return rc;
    }

    return 0;
}

static te_errno
make_namespace_target_available(rcf_rpc_server *rpcs,
                                tapi_nvme_subnqn nqn, int nvmet_port)
{
    char path1[RCF_MAX_PATH];
    char path2[RCF_MAX_PATH];

    TE_SPRINTF(path1, BASE_NVMET_CONFIG "/subsystems/%s", nqn);
    TE_SPRINTF(path2, BASE_NVMET_CONFIG "/ports/%d/subsystems/%s",
               nvmet_port, nqn);

    RPC_AWAIT_IUT_ERROR(rpcs);
    return rpc_symlink(rpcs, path1, path2) == 0 ? 0: rpcs->_errno;
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_target_setup(tapi_nvme_target *target)
{
    te_errno rc;

    assert(target);
    if (target->rpcs == NULL ||
        target->addr == NULL ||
        target->subnqn == NULL ||
        target->device == NULL ||
        target->addr->sa_family != AF_INET)
    {
        ERROR("Target can't be setup: rpcs=%p addr=%p subnqn=%s",
              target->rpcs, target->addr, target->subnqn);
        return TE_EINVAL;
    }

    tapi_nvme_target_cleanup(target);

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

static te_bool
try_rmdir(rcf_rpc_server *rpcs, const char *path)
{
    if (!is_dir_exist(rpcs, path))
        return FALSE;


    RPC_AWAIT_IUT_ERROR(rpcs);
    return rpc_rmdir(rpcs, path) == 0;
}

/* See description in tapi_nvme.h */
void
tapi_nvme_target_cleanup(tapi_nvme_target *target)
{
    size_t i;
    char path[RCF_MAX_PATH];
    char buffer[RCF_MAX_PATH];
    const char *directories[] = {
        "%s/subsystems/%s/namespaces/{port}",
        "%s/subsystems/%s",
        "%s/ports/{port}"
    };

    if (target == NULL ||
        target->rpcs == NULL ||
        target->addr == NULL ||
        target->subnqn == NULL)
        return;

    te_log_stack_push("Target cleanup start");

    TE_SPRINTF(path, BASE_NVMET_CONFIG "/ports/%d/subsystems/%s",
               target->nvmet_port, target->subnqn);

    RPC_AWAIT_IUT_ERROR(target->rpcs);
    if (rpc_access(target->rpcs, path, RPC_F_OK) == 0)
    {
        RPC_AWAIT_IUT_ERROR(target->rpcs);
        rpc_unlink(target->rpcs, path);
    }

    for (i = 0; i < TE_ARRAY_LEN(directories); i++)
    {
        replace_port(directories[i], target->nvmet_port, buffer);
        TE_SPRINTF(path, buffer, BASE_NVMET_CONFIG, target->subnqn);
        try_rmdir(target->rpcs, path);
    }
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
