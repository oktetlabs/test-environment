/** @file
 * @brief Unix Test Agent
 *
 * Unix TA virtual machines support
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#define TE_LGR_USER     "TA unix VM"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "te_string.h"
#include "te_shell_cmd.h"
#include "rcf_pch.h"
#include "agentlib.h"
#include "conf_vm.h"

#if __linux__

/** KVM device to check */
#define DEV_KVM     "/dev/kvm"

struct vm_net_entry {
    SLIST_ENTRY(vm_net_entry)   links;
    char                       *name;
    char                       *type;
    char                       *type_spec;
    char                       *mac_addr;
};

typedef SLIST_HEAD(vm_net_list_t, vm_net_entry) vm_net_list_t;

struct vm_entry {
    SLIST_ENTRY(vm_entry)   links;
    char                   *name;
    te_bool                 kvm;
    uint16_t                host_ssh_port;
    uint16_t                guest_ssh_port;
    uint16_t                rcf_port;
    te_string               cmd;
    pid_t                   pid;
    vm_net_list_t           nets;
};


static SLIST_HEAD(, vm_entry) vms = SLIST_HEAD_INITIALIZER(vms);

static uint16_t guest_ssh_port; /* SSH port in host byte order */
static te_bool kvm_supported;

static uint16_t
vm_alloc_tcp_port(void)
{
    static uint16_t tcp_port_next = 9320;

    return tcp_port_next++;
}

static te_bool
vm_is_running(struct vm_entry *vm)
{
    return (vm->pid == -1) ? FALSE : (ta_waitpid(vm->pid, NULL, WNOHANG) == 0);
}

static te_errno
vm_string_replace(const char *src, char **dst)
{
    char *new_dst;

    new_dst = strdup(src);
    if (new_dst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free(*dst);
    *dst = new_dst;

    return 0;
}

static te_errno
vm_append_virtio_dev_cmd(te_string *cmd, const char *mac_addr,
                         unsigned int interface_id)
{
    te_errno rc;

    rc = te_string_append(cmd, "virtio-net-pci,netdev=netdev%u%s%s",
                          interface_id,
                          mac_addr == NULL ? "" : ",mac=",
                          mac_addr == NULL ? "" : mac_addr);
    if (rc != 0)
    {
        ERROR("Cannot compose VM device argument (line %u)", __LINE__);
        return rc;
    }

    return 0;
}

static te_errno
vm_append_tap_interface_cmd(te_string *cmd, struct vm_net_entry *net,
                            unsigned int interface_id, te_bool vhost)
{
    te_string netdev = TE_STRING_INIT;
    te_string device = TE_STRING_INIT;
    te_errno rc = 0;

    rc = te_string_append(&netdev, "tap,id=netdev%u%s%s%s", interface_id,
                          vhost ? ",vhost=on" : "",
                          net->type_spec == NULL ? "" : ",ifname=",
                          net->type_spec == NULL ? "" : net->type_spec);
    if (rc != 0)
    {
        ERROR("Cannot compose VM netdev argument (line %u)", __LINE__);
        goto exit;
    }

    rc = vm_append_virtio_dev_cmd(&device, net->mac_addr, interface_id);
    if (rc != 0)
        goto exit;

    rc = te_string_append_shell_args_as_is(cmd, "-netdev", netdev.ptr,
                                           "-device", device.ptr, NULL);
    if (rc != 0)
    {
        ERROR("Cannot compose VM net interface command line (line %u)", __LINE__);
        goto exit;
    }

exit:
    te_string_free(&netdev);
    te_string_free(&device);

    return rc;
}

static te_errno
vm_append_net_interfaces_cmd(te_string *cmd, vm_net_list_t *nets)
{
    te_string interface_args = TE_STRING_INIT;
    unsigned int interface_id = 0;
    struct vm_net_entry *net;
    te_errno rc = 0;

    SLIST_FOREACH(net, nets, links)
    {
        if (net->type == NULL || *net->type == '\0')
        {
            ERROR("Cannot append empty interface type to VM (line %u)",
                  __LINE__);
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        else if (strcmp(net->type, "tap") == 0)
        {
            rc = vm_append_tap_interface_cmd(&interface_args, net,
                                             interface_id, FALSE);
        }
        else if (strcmp(net->type, "tap-vhost") == 0)
        {
            rc = vm_append_tap_interface_cmd(&interface_args, net,
                                             interface_id, TRUE);
        }
        else
        {
            ERROR("Cannot append unknown interface type to VM (line %u)",
                  __LINE__);
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if (rc != 0)
            goto exit;

        interface_id++;
    }

    if (interface_args.ptr != NULL)
    {
        rc = te_string_append(cmd, " %s", interface_args.ptr);
        if (rc != 0)
        {
            ERROR("Cannot compose VM net interface list command line (line %u)",
                  __LINE__);
            goto exit;
        }
    }

exit:
    te_string_free(&interface_args);

    return rc;
}

static te_errno
vm_start(struct vm_entry *vm)
{
    in_addr_t local_ip = htonl(INADDR_LOOPBACK);
    char local_ip_str[INET_ADDRSTRLEN];
    te_string name_str = TE_STRING_INIT;
    te_string net_mgmt_str = TE_STRING_INIT;
    te_errno rc;

    if (inet_ntop(AF_INET, &local_ip, local_ip_str, sizeof(local_ip_str)) == NULL)
    {
        ERROR("Cannot make local IP address string");
        rc = TE_RC(TE_TA_UNIX, TE_EFAULT);
        goto exit;
    }

    rc = te_string_append(&name_str, "guest=%s", vm->name);
    if (rc != 0)
    {
        ERROR("Cannot compose VM name parameter");
        goto exit;
    }

    rc = te_string_append(&net_mgmt_str,
             "user,id=mgmt,restrict=on,hostfwd=tcp:%s:%hu-:%hu,"
             "hostfwd=tcp:%s:%hu-:%hu",
             local_ip_str, vm->host_ssh_port, vm->guest_ssh_port,
             local_ip_str, vm->rcf_port, vm->rcf_port);
    if (rc != 0)
    {
        ERROR("Cannot compose management network config");
        goto exit;
    }

    te_string_free(&vm->cmd);
    rc = te_string_append_shell_args_as_is(&vm->cmd,
             "qemu-system-x86_64",
             "-name", name_str.ptr,
             "-no-user-config",
             "-nodefaults",
             "-nographic",
             NULL);
    if (rc != 0)
    {
        ERROR("Cannot compose VM start command line (line %u)", __LINE__);
        goto exit;
    }

    if (vm->kvm)
    {
        rc = te_string_append_shell_args_as_is(&vm->cmd,
                 "-enable-kvm",
                 "-cpu", "host",
                 "-machine",
                 "pc-i440fx-2.8,accel=kvm,usb=off,vmport=off,dump-guest-core=off",
                 NULL);
    }
    else
    {
        rc = te_string_append_shell_args_as_is(&vm->cmd,
                 "-machine", "pc-i440fx-2.8,usb=off,vmport=off,dump-guest-core=off",
                 NULL);
    }
    if (rc != 0)
    {
        ERROR("Cannot compose VM start command line (line %u)", __LINE__);
        goto exit;
    }

    rc = te_string_append_shell_args_as_is(&vm->cmd,
             "-drive", "file=/srv/virtual/testvm.qcow2,snapshot=on",
             "-netdev", net_mgmt_str.ptr,
             "-device", "virtio-net-pci,netdev=mgmt,romfile=,bus=pci.0,addr=0x3",
             NULL);
    if (rc != 0)
    {
        ERROR("Cannot compose VM start command line (line %u)", __LINE__);
        goto exit;
    }

    rc = vm_append_net_interfaces_cmd(&vm->cmd, &vm->nets);
    if (rc != 0)
        goto exit;

    INFO("VM %s command-line: %s", vm->name, vm->cmd.ptr);

    vm->pid = te_shell_cmd(vm->cmd.ptr, -1, NULL, NULL, NULL);
    if (vm->pid == -1)
    {
        ERROR("Cannot start VM: %s", vm->cmd.ptr);
        rc = TE_RC(TE_TA_UNIX, TE_EFAULT);
        goto exit;
    }


exit:
    te_string_free(&name_str);
    te_string_free(&net_mgmt_str);
    return rc;
}

static te_errno
vm_stop(struct vm_entry *vm)
{
    te_errno rc;

    if (ta_kill_death(vm->pid) == 0)
        rc = 0;
    else
        rc = TE_RC(TE_TA_UNIX, TE_ENOENT);

    return rc;
}


/*
 * Configuration model implementation
 */

static void
vm_net_free(struct vm_net_entry *net)
{
    free(net->name);
    free(net->type);
    free(net->type_spec);
    free(net->mac_addr);
    free(net);
}

static struct vm_net_entry *
vm_net_find(const struct vm_entry *vm, const char *name)
{
    struct vm_net_entry *p;

    if (vm == NULL)
        return NULL;

    SLIST_FOREACH(p, &vm->nets, links)
    {
        if (strcmp(name, p->name) == 0)
            return p;
    }

    return NULL;
}

static struct vm_entry *
vm_find(const char *name)
{
    struct vm_entry *p;

    SLIST_FOREACH(p, &vms, links)
    {
        if (strcmp(name, p->name) == 0)
            return p;
    }

    return NULL;
}

static te_errno
vm_list(unsigned int gid, const char *oid, const char *sub_id, char **list)
{
    struct vm_entry *vm;
    size_t len = 0;
    char *p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    SLIST_FOREACH(vm, &vms, links)
        len += strlen(vm->name) + 1;

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    *list = TE_ALLOC(len);
    if (*list == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    p = *list;
    SLIST_FOREACH(vm, &vms, links)
    {
        if (p != *list)
            *p++ = ' ';
        len = strlen(vm->name);
        memcpy(p, vm->name, len);
        p += len;
    }
    *p = '\0';

    return 0;
}

static te_errno
vm_add(unsigned int gid, const char *oid, const char *value,
       const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    ENTRY("%s", vm_name);

    if (vm_find(vm_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    vm = TE_ALLOC(sizeof(*vm));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    vm->name = strdup(vm_name);
    if (vm->name == NULL)
    {
        free(vm);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    vm->kvm = kvm_supported;
    vm->host_ssh_port = vm_alloc_tcp_port();
    vm->guest_ssh_port = guest_ssh_port;
    vm->rcf_port = vm_alloc_tcp_port();

    vm->pid = -1;

    SLIST_INIT(&vm->nets);

    SLIST_INSERT_HEAD(&vms, vm, links);

    return 0;
}

static te_errno
vm_del(unsigned int gid, const char *oid,
       const char *vm_name)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;
    struct vm_net_entry *net_tmp;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vms, vm, vm_entry, links);

    if (vm_is_running(vm))
        (void)vm_stop(vm);

    SLIST_FOREACH_SAFE(net, &vm->nets, links, net_tmp)
    {
        vm_net_free(net);
    }

    te_string_free(&vm->cmd);
    free(vm->name);
    free(vm);

    return 0;
}

static te_errno
vm_status_get(unsigned int gid, const char *oid, char *value,
              const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%d", vm_is_running(vm));

    return 0;
}

static te_errno
vm_status_set(unsigned int gid, const char *oid, const char *value,
              const char *vm_name)
{
    struct vm_entry *vm;
    te_bool enable = !!atoi(value);
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (enable == vm_is_running(vm))
        return 0;

    if (enable)
        rc = vm_start(vm);
    else
        rc = vm_stop(vm);

    return rc;
}

static uint16_t *
vm_port_ptr_by_oid(struct vm_entry *vm, const char *oid)
{
    cfg_oid *coid = cfg_convert_oid_str(oid);

    if (coid == NULL)
        return NULL;

    if (strcmp(cfg_oid_inst_subid(coid, 3), "rcf_port") == 0)
    {
        return &vm->rcf_port;
    }
    else if (strcmp(cfg_oid_inst_subid(coid, 3), "ssh_port") == 0)
    {
        if (strcmp(cfg_oid_inst_subid(coid, 4), "host") == 0)
            return &vm->host_ssh_port;
        else if (strcmp(cfg_oid_inst_subid(coid, 4), "guest") == 0)
            return &vm->guest_ssh_port;
        else
            return NULL;
    }
    else
        return NULL;
}

static te_errno
vm_port_get(unsigned int gid, const char *oid, char *value,
            const char *vm_name)
{
    struct vm_entry *vm;
    uint16_t *port;

    UNUSED(gid);

    ENTRY("%s oid=%s", vm_name, oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    port = vm_port_ptr_by_oid(vm, oid);
    if (port == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%hu", *port);

    return 0;
}

static te_errno
vm_port_set(unsigned int gid, const char *oid, const char *value,
            const char *vm_name)
{
    struct vm_entry *vm;
    uint16_t *port;
    int val;

    UNUSED(gid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    port = vm_port_ptr_by_oid(vm, oid);
    if (port == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    val = atoi(value);
    if (val < 0 || val > UINT16_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *port = val;

    return 0;
}


static te_errno
vm_kvm_get(unsigned int gid, const char *oid, char *value,
           const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%d", vm->kvm);

    return 0;
}

static te_errno
vm_kvm_set(unsigned int gid, const char *oid, const char *value,
           const char *vm_name)
{
    struct vm_entry *vm;
    te_bool val;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    val = !!atoi(value);

    if (!kvm_supported && val)
        WARN("KVM is not supported, but requested");

    vm->kvm = val;

    return 0;
}

static te_errno
vm_net_add(unsigned int gid, const char *oid, const char *value,
           const char *vm_name, const char *net_name)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((vm = vm_find(vm_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    if (vm_net_find(vm, net_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    net = TE_ALLOC(sizeof(*net));
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    net->name = strdup(net_name);
    if (net->name == NULL)
    {
        free(net);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    SLIST_INSERT_HEAD(&vm->nets, net, links);

    return 0;
}

static te_errno
vm_net_del(unsigned int gid, const char *oid,
           const char *vm_name, const char *net_name)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    net = vm_net_find(vm, net_name);
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->nets, net, vm_net_entry, links);

    vm_net_free(net);

    return 0;
}

static te_errno
vm_net_list(unsigned int gid, const char *oid, const char *sub_id, char **list,
            const char *vm_name)
{
    te_string result = TE_STRING_INIT;
    struct vm_entry *vm;
    struct vm_net_entry *net;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(net, &vm->nets, links)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ", net->name);
        first = FALSE;
        if (rc != 0)
        {
            te_string_free(&result);
            return rc;
        }
    }

    *list = result.ptr;
    return 0;
}

static char **
vm_net_property_ptr_by_oid(struct vm_net_entry *net, const char *oid)
{
    cfg_oid *coid = cfg_convert_oid_str(oid);
    char **result = NULL;
    char *prop_subid;

    if (coid == NULL)
        goto exit;

    prop_subid = cfg_oid_inst_subid(coid, 4);
    if (prop_subid == NULL)
        goto exit;

    if (strcmp(prop_subid, "type") == 0)
        result = &net->type;
    else if (strcmp(prop_subid, "type_spec") == 0)
        result = &net->type_spec;
    else if (strcmp(prop_subid, "mac_addr") == 0)
        result = &net->mac_addr;

exit:
    cfg_free_oid(coid);

    return result;
}

static te_errno
vm_net_property_get(unsigned int gid, const char *oid, char *value,
                    const char *vm_name, const char *net_name)
{
    struct vm_net_entry *net;
    char **property;

    UNUSED(gid);

    net = vm_net_find(vm_find(vm_name), net_name);
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    property = vm_net_property_ptr_by_oid(net, oid);
    if (property == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", *property == NULL ? "" : *property);

    return 0;
}

static te_errno
vm_net_property_set(unsigned int gid, const char *oid, const char *value,
                const char *vm_name, const char *net_name)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;
    char **property;

    UNUSED(gid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, ETXTBSY);

    net = vm_net_find(vm, net_name);
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    property = vm_net_property_ptr_by_oid(net, oid);
    if (property == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return vm_string_replace(value, property);
}

RCF_PCH_CFG_NODE_RW(node_vm_net_mac_addr, "mac_addr", NULL, NULL,
                    vm_net_property_get, vm_net_property_set);

RCF_PCH_CFG_NODE_RW(node_vm_net_type_spec, "type_spec", NULL,
                    &node_vm_net_mac_addr, vm_net_property_get,
                    vm_net_property_set);

RCF_PCH_CFG_NODE_RW(node_vm_net_type, "type", NULL, &node_vm_net_type_spec,
                    vm_net_property_get, vm_net_property_set);

RCF_PCH_CFG_NODE_COLLECTION(node_vm_net, "net", &node_vm_net_type, NULL,
                            vm_net_add, vm_net_del, vm_net_list, NULL);

RCF_PCH_CFG_NODE_RW(node_vm_kvm, "kvm", NULL, &node_vm_net,
                    vm_kvm_get, vm_kvm_set);

RCF_PCH_CFG_NODE_RW(node_vm_rcf_port, "rcf_port", NULL, &node_vm_kvm,
                    vm_port_get, vm_port_set);

RCF_PCH_CFG_NODE_RW(node_vm_ssh_port_guest, "guest", NULL, NULL,
                    vm_port_get, vm_port_set);

RCF_PCH_CFG_NODE_RO(node_vm_ssh_port_host, "host", NULL, &node_vm_ssh_port_guest,
                    vm_port_get);

RCF_PCH_CFG_NODE_NA(node_vm_ssh_port, "ssh_port",
                    &node_vm_ssh_port_host, &node_vm_rcf_port);

RCF_PCH_CFG_NODE_RW(node_vm_status, "status", NULL, &node_vm_ssh_port,
                    vm_status_get, vm_status_set);

RCF_PCH_CFG_NODE_COLLECTION(node_vm, "vm", &node_vm_status, NULL,
                            vm_add, vm_del, vm_list, NULL);


static te_bool
check_kvm(void)
{
    te_bool supported = FALSE;
    struct stat st;

    if (stat(DEV_KVM, &st) != 0)
    {
        if (errno == ENOENT)
            WARN("KVM is not supported");
        else
            WARN("KVM check failed: %s", strerror(errno));
    }
    else if (access(DEV_KVM, R_OK | W_OK) != 0)
    {
        WARN("KVM is not accessible: %s", strerror(errno));
    }
    else
    {
        int fd = open(DEV_KVM, O_RDWR);

        if (fd < 0)
        {
            ERROR("Cannot open %s to read-write: %s",
                  DEV_KVM, strerror(errno));
        }
        else
        {
            close(fd);
            RING("KVM is supported");
            supported = TRUE;
        }
    }

    return supported;
}


te_errno
ta_unix_conf_vm_init(void)
{
    struct servent *se = getservbyname("ssh", "tcp");

    if (se == NULL)
    {
        ERROR("Cannot get ssh service entry: %s", strerror(errno));
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    guest_ssh_port = ntohs(se->s_port);

    endservent();

    kvm_supported = check_kvm();

    return rcf_pch_add_node("/agent", &node_vm);
}

#else /* !__linux__ */

te_errno
ta_unix_conf_vm_init(void)
{
    WARN("Virtual machines configuration is not supported");
    return 0;
}

#endif /* !__linux__ */
