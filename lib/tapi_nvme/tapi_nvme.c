/** @file
 * @brief Control NVMeOF
 *
 * API for control NVMe Over Fabrics
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "te_defs.h"
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

#define PORT_SIZE           (6)
#define DIRECTORY_SIZE      (32)
#define SUBNQN_SIZE         (256)
#define NAMESPACE_SIZE      (32)
#define MAX_NAMESPACES      (32)
#define MAX_ADMIN_DEVICES   (32)
#define TRANPORT_SIZE       (16)
#define ADDRESS_INFO_SIZE   (128)

#define DEVICE_WAIT_ATTEMPTS (5)

static int
run_command_generic(rcf_rpc_server *rpcs, te_string *str_stdout,
                    te_string *str_stderr, const char *command)
{
    tarpc_pid_t pid;
    int fd_stdout;
    int fd_stderr;
    rpc_wait_status status;

    pid = rpc_te_shell_cmd(rpcs, command, (tarpc_uid_t) -1, NULL,
                           &fd_stdout, &fd_stderr);

    if (pid == -1)
        TEST_FAIL("Cannot run command: %s", command);

    RPC_AWAIT_IUT_ERROR(rpcs);
    rpcs->timeout = 1000;
    pid = rpc_waitpid(rpcs, pid, &status, 0);

    if (pid == -1)
        TEST_FAIL("waitpid: %s", command);

    if (str_stdout != NULL)
        rpc_read_fd2te_string(rpcs, fd_stdout, 100, 0, str_stdout);
    if (str_stderr != NULL)
        rpc_read_fd2te_string(rpcs, fd_stderr, 100, 0, str_stderr);

    if (status.flag != RPC_WAIT_STATUS_EXITED)
        TEST_FAIL("Process is %s", wait_status_flag_rpc2str(status.flag));


    return status.value;
}

static int
run_command_output(rcf_rpc_server *rpcs, te_string *str_stdour,
                   te_string *str_stderr, const char *format_cmd, ...)
{
    va_list arguments;
    char command[RPC_SHELL_CMDLINE_MAX];

    va_start(arguments, format_cmd);
    vsnprintf(command, sizeof(command), format_cmd, arguments);
    va_end(arguments);

    return run_command_generic(rpcs, str_stdour, str_stderr, command);
}

static int
run_command(rcf_rpc_server *rpcs, const char *format_cmd, ...)
{
    va_list arguments;
    char command[RPC_SHELL_CMDLINE_MAX];

    va_start(arguments, format_cmd);
    vsnprintf(command, sizeof(command), format_cmd, arguments);
    va_end(arguments);

    return run_command_generic(rpcs, NULL, NULL, command);
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
    if (count <= 0)
        return READ_ERROR;

    return strcpy(info->namespace, names[0].name) != NULL ?
           READ_SUCCESS: READ_ERROR;
}

static read_result
parse_address(const char *str, char *address, unsigned short *port)
{
    const char *traddr = "traddr=";
    const char *trsvcid = "trsvcid=";
    char *delimeter = strchr(str, ',');
    char *start_traddr = strstr(str, traddr);
    char *start_trsvcid = strstr(str, trsvcid);
    char buffer[32];

    if (delimeter == NULL || start_traddr == NULL || start_trsvcid == NULL ||
        start_trsvcid < start_traddr || start_traddr > delimeter)
        return READ_ERROR;

    strncpy(buffer, str, delimeter - str);
    if (sscanf(buffer, "traddr=%s", address) != 1)
        return READ_ERROR;

    strcpy(buffer, delimeter + 1);
    if (sscanf(buffer, "trsvcid=%hu", port) != 1)
        return READ_ERROR;

    return READ_SUCCESS;
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

    if (parse_address(buffer, address, &port) != 0)
    {
        ERROR("Cannot parse address info");
        return READ_ERROR;
    }

    memset(&info->addr, 0, sizeof(info->addr));
    info->addr.sin_family = AF_INET;
    info->addr.sin_addr.s_addr = inet_addr(address);
    info->addr.sin_port = htons(port);

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
        sprintf(path, "%s/%s/%s", BASE_NVME_FABRICS,
                admin_dev, actions[i].file);
        if ((rc = actions[i].function(rpcs, info, path)) != READ_SUCCESS)
            return rc;
    }

    return READ_SUCCESS;
}

static void
debug_target_print(const tapi_nvme_target *target)
{
    const struct sockaddr_in *addr1 = SIN(target->addr);
    INFO("target->nqn='%s'; target->addr='%s:%hu'",
         target->subnqn,
         te_sockaddr_get_ipstr(SA(addr1)),
         te_sockaddr_get_port(SA(addr1)));
}

static void
debug_fabric_info_print(const nvme_fabric_info *info)
{
    const struct sockaddr_in *addr2 = SIN(&info->addr);
    INFO("info->nqn='%s'; info->addr='%s:%hu'",
         info->subnqn,
         te_sockaddr_get_ipstr(SA(addr2)),
         te_sockaddr_get_port(SA(addr2)));
}

static te_bool
is_target_eq(const tapi_nvme_target *target, const nvme_fabric_info *info)
{
    const struct sockaddr_in *addr1 = SIN(target->addr);
    const struct sockaddr_in *addr2 = SIN(&info->addr);
    te_bool subnqn_eq;
    te_bool addr_eq;
    te_bool transport_eq;

    debug_target_print(target);
    debug_fabric_info_print(info);

    transport_eq = target->transport == info->transport;
    subnqn_eq = strcmp(target->subnqn, info->subnqn) == 0;
    addr_eq = addr2->sin_addr.s_addr == addr1->sin_addr.s_addr;

    return transport_eq && subnqn_eq && addr_eq;
}

static char *
get_new_device(const tapi_nvme_host_ctrl *host_ctrl,
               const tapi_nvme_target *target)
{
    int i, count;
    read_result rc;
    char *device = NULL;

    nvme_fabric_info info;
    directory_name names[MAX_ADMIN_DEVICES];

    count = filter_directory(host_ctrl->rpcs, BASE_NVME_FABRICS,
                             "nvme", names, TE_ARRAY_LEN(names));

    if (count < 0)
        return NULL;

    for (i = 0; i < count; i++)
    {
        rc = read_nvme_fabric_info(host_ctrl->rpcs, &info, names[i].name);
        if (rc == READ_ERROR)
            return NULL;
        else if (rc == READ_CONTINUE)
            continue;

        if (is_target_eq(target, &info) == TRUE)
        {
            device = tapi_malloc(256);
            sprintf(device, "/dev/%s", info.namespace);
            break;
        }
    }

    return device;
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

    if (host_ctrl == NULL)
        return TE_EINVAL;

    if (host_ctrl->connected_target != NULL)
        return TE_EISCONN;

    if (target == NULL ||
        target->addr == NULL ||
        target->addr->sa_family != AF_INET ||
        target->subnqn == NULL)
        return TE_EINVAL;

    rc = run_command_output(host_ctrl->rpcs, &str_stdout, &str_stderr,
                            strcat(cmd, opts),
                            te_sockaddr_get_ipstr(target->addr),
                            te_sockaddr_get_port(target->addr),
                            tapi_nvme_transport_str(target->transport),
                            target->subnqn);

    if (rc == 0)
    {
        host_ctrl->connected_target = target;
        RING("Success connection to target");
    }
        RING("stdout:\n%s\nstderr:\n%s", str_stdout.ptr, str_stderr.ptr);


    (void)tapi_nvme_initiator_list(host_ctrl);

    for (i = 0; i < DEVICE_WAIT_ATTEMPTS; i++)
    {
        host_ctrl->device = get_new_device(host_ctrl, target);
        if (host_ctrl->device != NULL)
            break;
        te_motivated_sleep(1, "Waiting device...");
    }

    if (host_ctrl->device == NULL)
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

    rc = run_command_output(host_ctrl->rpcs, &str_stdout, &str_stderr,
                            "nvme list");
    if (rc != 0)
        WARN("stderr: \n%s", str_stderr.ptr);
    else
        RING("stdout:\n%s\nstderr:\n%s", str_stdout.ptr, str_stderr.ptr);

    return rc == 0 ? 0 : TE_EFAULT;
}


/* See description in tapi_nvme.h */
void
tapi_nvme_initiator_disconnect(tapi_nvme_host_ctrl *host_ctrl)
{
    te_string str_stdout = TE_STRING_INIT;
    te_string str_stderr = TE_STRING_INIT;
    const char *template = "nvme disconnect --nqn=%s";

    if (host_ctrl == NULL ||
        host_ctrl->rpcs == NULL ||
        host_ctrl->connected_target == NULL ||
        host_ctrl->connected_target->subnqn == NULL)
        return;

    run_command_output(host_ctrl->rpcs, &str_stdout, &str_stderr, template,
                  host_ctrl->connected_target->subnqn);

    RING("stdout: %s\nstderr: %s", str_stdout.ptr, str_stderr.ptr);

    host_ctrl->connected_target = NULL;
    free(host_ctrl->device);
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

    sprintf(port_str, "%d", nvmet_port);
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

    for (i = 0; i < TE_ARRAY_LEN(directories); i++)
    {
        replace_port(directories[i], nvmet_port, buffer);
        snprintf(path, sizeof(path), buffer, BASE_NVMET_CONFIG, nqn);
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

    snprintf(port, sizeof(port), "%u", te_sockaddr_get_port(addr));

    for (i = 0; i < TE_ARRAY_LEN(configs); i++)
    {
        replace_port(configs[i].prefix, nvmet_port, buffer);
        snprintf(path, sizeof(path), buffer, BASE_NVMET_CONFIG, nqn);
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

    snprintf(path1, sizeof(path1),
             "%s/subsystems/%s", BASE_NVMET_CONFIG, nqn);
    snprintf(path2, sizeof(path2),
             "%s/ports/%d/subsystems/%s", BASE_NVMET_CONFIG, nvmet_port, nqn);

    RPC_AWAIT_IUT_ERROR(rpcs);
    return rpc_symlink(rpcs, path1, path2) == 0 ? 0: rpcs->_errno;
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_target_setup(tapi_nvme_target *target)
{
    te_errno rc;

    if (target == NULL ||
        target->rpcs == NULL ||
        target->addr == NULL ||
        target->subnqn == NULL ||
        target->addr->sa_family != AF_INET)
        return TE_EINVAL;

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
is_dir_exist(rcf_rpc_server *rpcs, const char *path)
{
    rpc_dir_p dir;

    RPC_AWAIT_IUT_ERROR(rpcs);
    if ((dir = rpc_opendir(rpcs, path)) != 0)
    {
        RPC_AWAIT_IUT_ERROR(rpcs);
        rpc_closedir(rpcs, dir);
        return TRUE;
    }

    return FALSE;
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

    RING("Target cleanup start");

    snprintf(path, sizeof(path), "%s/ports/%d/subsystems/%s",
             BASE_NVMET_CONFIG, target->nvmet_port, target->subnqn);

    RPC_AWAIT_IUT_ERROR(target->rpcs);
    if (rpc_access(target->rpcs, path, RPC_F_OK) == 0)
    {
        RPC_AWAIT_IUT_ERROR(target->rpcs);
        rpc_unlink(target->rpcs, path);
    }

    for (i = 0; i < TE_ARRAY_LEN(directories); i++)
    {
        replace_port(directories[i], target->nvmet_port, buffer);
        snprintf(path, sizeof(path), buffer, BASE_NVMET_CONFIG, target->subnqn);
        try_rmdir(target->rpcs, path);
    }

    RING("Target cleanup end");
}

/* See description in tapi_nvme.h */
te_errno
tapi_nvme_target_format(tapi_nvme_target *target)
{
    if (target == NULL ||
        target->rpcs == NULL ||
        target->device == NULL)
        return TE_EINVAL;

    run_command(target->rpcs, "nvme format --ses=%d --namespace-id=%d %s",
                0, 1, target->device);

    return 0;
}
