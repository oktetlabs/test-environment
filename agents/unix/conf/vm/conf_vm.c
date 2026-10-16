/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA virtual machines support
 *
 * Copyright (C) 2019-2023 OKTET Labs Ltd. All rights reserved.
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
#include "te_kvpair.h"
#include "te_sleep.h"
#include "te_string.h"
#include "te_shell_cmd.h"
#include "te_str.h"
#include "te_vector.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "agentlib.h"
#include "conf_common.h"
#include "conf_vm.h"

#if __linux__

/** KVM device to check */
#define DEV_KVM     "/dev/kvm"

/** Default QEMU system emulator to use */
#define VM_QEMU_DEFAULT "qemu-system-x86_64"

/** Default QEMU emulated machine */
#define VM_MACHINE_DEFAULT  \
    "pc,usb=off,vmport=off,dump-guest-core=off"

/** Default management network device */
#define VM_MGMT_NET_DEVICE_DEFAULT  "virtio-net-pci"

#define VM_NET_NETDEV_OPT "netdev_opt"
#define VM_NET_DEVICE_OPT "device_opt"

typedef struct vm_cpu {
    te_string       model;
    unsigned int    num;
} vm_cpu;

struct vm_drive_entry {
    SLIST_ENTRY(vm_drive_entry)     links;
    char                           *name;
    te_string                       file;
    bool snapshot;
    bool cdrom;
};

struct vm_virtfs_entry {
    SLIST_ENTRY(vm_virtfs_entry)    links;
    char                           *name;
    char                           *fsdriver;
    char                           *path;
    char                           *security_model;
    char                           *mount_tag;
};

struct vm_chardev_entry {
    SLIST_ENTRY(vm_chardev_entry)   links;
    char                           *name;
    char                           *path;
    bool server;
};

struct vm_net_entry {
    SLIST_ENTRY(vm_net_entry)   links;
    char                       *name;
    char                       *type;
    char                       *type_spec;
    char                       *mac_addr;
    te_kvpair_h                 netdev_opts;
    te_kvpair_h                 device_opts;
};

struct vm_pci_pt_entry {
    SLIST_ENTRY(vm_pci_pt_entry)    links;
    char                           *name;
    char                           *vf_token;
    te_string                       pci_addr;
};

struct vm_device_entry {
    SLIST_ENTRY(vm_device_entry)    links;
    char                           *name;
    te_string                       device;
};


typedef SLIST_HEAD(vm_chardev_list_t, vm_chardev_entry) vm_chardev_list_t;
typedef SLIST_HEAD(vm_net_list_t, vm_net_entry) vm_net_list_t;
typedef SLIST_HEAD(vm_drive_list_t, vm_drive_entry) vm_drive_list_t;
typedef SLIST_HEAD(vm_virtfs_list_t, vm_virtfs_entry) vm_virtfs_list_t;
typedef SLIST_HEAD(vm_pci_pt_list_t, vm_pci_pt_entry) vm_pci_pt_list_t;
typedef SLIST_HEAD(vm_device_list_t, vm_device_entry) vm_device_list_t;

struct vm_entry {
    SLIST_ENTRY(vm_entry)   links;
    char                   *name;
    char                   *qemu;
    char                   *machine;
    char                   *mgmt_net_device;
    bool kvm;
    uint16_t                host_ssh_port;
    uint16_t                guest_ssh_port;
    uint16_t                rcf_port;
    vm_cpu                  cpu;
    unsigned int            mem_size;
    char                   *mem_path;
    bool mem_prealloc;
    te_string               cmd;
    pid_t                   pid;
    vm_chardev_list_t       chardevs;
    vm_net_list_t           nets;
    vm_drive_list_t         drives;
    vm_virtfs_list_t        virtfses;
    vm_pci_pt_list_t        pci_pts;
    vm_device_list_t        devices;
    char                   *kernel;
    char                   *ker_cmd;
    char                   *ker_initrd;
    char                   *ker_dtb;
    char                   *serial;
};

static struct vm_chardev_entry * vm_chardev_find(const struct vm_entry *vm,
                                                 const char *name);

static SLIST_HEAD(, vm_entry) vms = SLIST_HEAD_INITIALIZER(vms);

static uint16_t guest_ssh_port; /* SSH port in host byte order */
static bool kvm_supported;

static bool
vm_is_running(struct vm_entry *vm)
{
    pid_t ret;

    if (vm->pid == -1)
        return false;

    do {
        ret = ta_waitpid(vm->pid, NULL, WNOHANG);
    } while (ret == -1 && errno == EINTR);

    if (ret != 0)
    {
        /*
         * Either an error occurred or the process terminated.
         * In both cases we can forget about the child process.
         */
        vm->pid = -1;
        return false;
    }

    return true;
}

static te_errno
vm_append_virtio_dev_cmd(te_string *cmd, const char *mac_addr,
                         unsigned int interface_id, te_kvpair_h *kvp)
{
    te_string opts = TE_STRING_INIT;

    te_kvpair_to_str_gen(kvp, ",", &opts);
    te_string_append(cmd, "virtio-net-pci,netdev=netdev%u%s%s%s%s",
                     interface_id,
                     mac_addr == NULL ? "" : ",mac=",
                     mac_addr == NULL ? "" : mac_addr,
                     te_str_is_null_or_empty(opts.ptr) ? "" : ",",
                     te_str_is_null_or_empty(opts.ptr) ? "" : opts.ptr);
    te_string_free(&opts);

    return 0;
}

static te_errno
vm_append_dgram_interface_cmd(te_string *cmd, struct vm_net_entry *net,
                              unsigned int interface_id)
{
    te_string netdev = TE_STRING_INIT;
    te_string device = TE_STRING_INIT;
    te_string opts = TE_STRING_INIT;
    te_errno rc;

    te_kvpair_to_str_gen(&net->netdev_opts, ",", &opts);
    te_string_append(&netdev, "dgram,id=netdev%u%s%s", interface_id,
                     te_str_is_null_or_empty(opts.ptr) ? "" : ",",
                     te_str_is_null_or_empty(opts.ptr) ? "" : opts.ptr);

    rc = vm_append_virtio_dev_cmd(&device, net->mac_addr, interface_id,
                                  &net->device_opts);
    if (rc == 0)
        te_string_append_shell_args_as_is(cmd, "-netdev", netdev.ptr,
                                          "-device", device.ptr, NULL);

    te_string_free(&netdev);
    te_string_free(&device);
    te_string_free(&opts);

    return rc;
}

static te_errno
vm_append_socket_interface_cmd(te_string *cmd, struct vm_net_entry *net,
                               unsigned int interface_id)
{
    te_string netdev = TE_STRING_INIT;
    te_string device = TE_STRING_INIT;
    te_string opts = TE_STRING_INIT;
    te_errno rc;

    if (net->type_spec != NULL && strchr(net->type_spec, '=') == NULL)
    {
        ERROR("type_spec must be a key=value pair for socket net interface '%s'",
              te_str_empty_if_null(net->name));
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    te_kvpair_to_str_gen(&net->netdev_opts, ",", &opts);
    te_string_append(&netdev, "socket,id=netdev%u%s%s%s%s", interface_id,
                     net->type_spec == NULL ? "" : ",",
                     net->type_spec == NULL ? "" : net->type_spec,
                     te_str_is_null_or_empty(opts.ptr) ? "" : ",",
                     te_str_is_null_or_empty(opts.ptr) ? "" : opts.ptr);

    rc = vm_append_virtio_dev_cmd(&device, net->mac_addr, interface_id,
                                  &net->device_opts);
    if (rc == 0)
    {
        te_string_append_shell_args_as_is(cmd, "-netdev", netdev.ptr,
                                          "-device", device.ptr, NULL);
    }

    te_string_free(&netdev);
    te_string_free(&device);
    te_string_free(&opts);

    return rc;
}

static te_errno
vm_append_tap_interface_cmd(te_string *cmd, struct vm_net_entry *net,
                            unsigned int interface_id)
{
    te_string netdev = TE_STRING_INIT;
    te_string device = TE_STRING_INIT;
    te_string opts = TE_STRING_INIT;
    te_errno rc = 0;

    te_kvpair_to_str_gen(&net->netdev_opts, ",", &opts);
    te_string_append(&netdev,
            "tap,script=no,downscript=no,id=netdev%u%s%s%s%s", interface_id,
            net->type_spec == NULL ? "" : ",ifname=",
            net->type_spec == NULL ? "" : net->type_spec,
            te_str_is_null_or_empty(opts.ptr) ? "" : ",",
            te_str_is_null_or_empty(opts.ptr) ? "" : opts.ptr);

    rc = vm_append_virtio_dev_cmd(&device, net->mac_addr, interface_id,
                                  &net->device_opts);
    if (rc == 0)
        te_string_append_shell_args_as_is(cmd, "-netdev", netdev.ptr,
                                          "-device", device.ptr, NULL);

    te_string_free(&netdev);
    te_string_free(&device);
    te_string_free(&opts);

    return rc;
}

static te_errno
vm_append_vhost_user_interface_cmd(te_string *cmd, struct vm_net_entry *net,
                                   unsigned int interface_id,
                                   const struct vm_entry *vm)
{
    te_string netdev = TE_STRING_INIT;
    te_string device = TE_STRING_INIT;
    te_string opts = TE_STRING_INIT;
    struct vm_chardev_entry *chardev;
    te_errno rc = 0;

    if (net->type_spec == NULL)
    {
        ERROR("Attribute type_spec is required for vhost-user net interface");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    chardev = vm_chardev_find(vm, net->type_spec);
    if (chardev == NULL)
    {
        ERROR("Failed to find chardev pointed to by vhost-user net interface");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (!chardev->server)
        WARN("Probably vhost-user net interface expects server chardev");

    if (vm->mem_path == NULL)
    {
        ERROR("Huge pages filesystem is required for vhost-user net interface");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    te_kvpair_to_str_gen(&net->netdev_opts, ",", &opts);
    te_string_append(&netdev,
                     "type=vhost-user,id=netdev%u,chardev=%s,vhostforce%s%s",
                     interface_id, net->type_spec,
                     te_str_is_null_or_empty(opts.ptr) ? "" : ",",
                     te_str_is_null_or_empty(opts.ptr) ? "" : opts.ptr);

    rc = vm_append_virtio_dev_cmd(&device, net->mac_addr, interface_id,
                                  &net->device_opts);
    if (rc == 0)
        te_string_append_shell_args_as_is(cmd, "-netdev", netdev.ptr,
                                          "-device", device.ptr, NULL);

    te_string_free(&netdev);
    te_string_free(&device);
    te_string_free(&opts);

    return rc;
}

static te_errno
vm_append_chardevs_cmd(te_string *cmd, vm_chardev_list_t *chardevs)
{
    te_string chardev_args = TE_STRING_INIT;
    struct vm_chardev_entry *chardev;
    te_errno rc = 0;

    SLIST_FOREACH(chardev, chardevs, links)
    {
        te_string chardev_arg = TE_STRING_INIT;

        /* Only Unix socket backend for character devices is supported yet */
        if (chardev->path == NULL)
        {
            ERROR("Unix socket character device must have path attribute");
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
            goto exit;
        }

        te_string_append(&chardev_arg, "socket,id=%s,path=%s%s",
                         chardev->name, chardev->path,
                         chardev->server ? ",server" : "");

        te_string_append_shell_args_as_is(&chardev_args, "-chardev",
                                          chardev_arg.ptr, NULL);
        te_string_free(&chardev_arg);
    }

    if (chardev_args.ptr != NULL)
        te_string_append(cmd, " %s", chardev_args.ptr);

exit:
    te_string_free(&chardev_args);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_net_interfaces_cmd(te_string *cmd, vm_net_list_t *nets,
                             const struct vm_entry *vm)
{
    te_string interface_args = TE_STRING_INIT;
    unsigned int interface_id = 0;
    struct vm_net_entry *net;
    te_errno rc = 0;

    SLIST_FOREACH(net, nets, links)
    {
        if (net->type == NULL)
        {
            ERROR("Cannot append empty interface type to VM (line %u)",
                  __LINE__);
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        else if (strcmp(net->type, "dgram") == 0)
        {
            rc = vm_append_dgram_interface_cmd(&interface_args, net,
                                               interface_id);
        }
        else if (strcmp(net->type, "tap") == 0)
        {
            rc = vm_append_tap_interface_cmd(&interface_args, net,
                                             interface_id);
        }
        else if (strcmp(net->type, "socket") == 0)
        {
            rc = vm_append_socket_interface_cmd(&interface_args, net,
                                                interface_id);
        }
        else if (strcmp(net->type, "vhost-user") == 0)
        {
            rc = vm_append_vhost_user_interface_cmd(&interface_args, net,
                                                    interface_id, vm);
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
        te_string_append(cmd, " %s", interface_args.ptr);

exit:
    te_string_free(&interface_args);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_kernel_cmd(te_string *cmd, struct vm_entry *vm)
{
    if (vm->kernel != NULL)
    {
        te_string_append_shell_args_as_is(cmd, "-kernel", vm->kernel, NULL);

        if (vm->ker_cmd != NULL)
        {
            te_string_append_shell_args_as_is(cmd, "-append",
                                              vm->ker_cmd, NULL);
        }

        if (vm->ker_initrd != NULL)
        {
            te_string_append_shell_args_as_is(cmd, "-initrd",
                                              vm->ker_initrd, NULL);
        }

        if (vm->ker_dtb != NULL)
        {
            te_string_append_shell_args_as_is(cmd, "-dtb",
                                              vm->ker_dtb, NULL);
        }
    }

    return 0;
}

static te_errno
vm_append_drive_cmd(te_string *cmd, vm_drive_list_t *drives)
{
    te_string drive_args = TE_STRING_INIT;
    struct vm_drive_entry *drive;

    SLIST_FOREACH(drive, drives, links)
    {
        te_string_append(&drive_args, "file=%s,media=%s,snapshot=%s",
                         drive->file.ptr,
                         drive->cdrom ? "cdrom" : "disk",
                         drive->snapshot ? "on" : "off");

        te_string_append_shell_args_as_is(cmd, "-drive", drive_args.ptr, NULL);

        te_string_reset(&drive_args);
    }

    te_string_free(&drive_args);

    return 0;
}

static te_errno
vm_append_virtfs_cmd(te_string *cmd, vm_virtfs_list_t *virtfses)
{
    te_string virtfs_args = TE_STRING_INIT;
    struct vm_virtfs_entry *virtfs;

    SLIST_FOREACH(virtfs, virtfses, links)
    {
        te_string_append(&virtfs_args,
                         "%s,path=%s,security_model=%s,mount_tag=%s",
                         virtfs->fsdriver,
                         virtfs->path,
                         virtfs->security_model,
                         virtfs->mount_tag);

        te_string_append_shell_args_as_is(cmd, "-virtfs",
                                          virtfs_args.ptr, NULL);

        te_string_reset(&virtfs_args);
    }

    te_string_free(&virtfs_args);

    return 0;
}

static te_errno
vm_append_pci_pt_cmd(te_string *cmd, vm_pci_pt_list_t *pt_list)
{
    te_string args = TE_STRING_INIT;
    struct vm_pci_pt_entry *pt = NULL;

    SLIST_FOREACH(pt, pt_list, links)
    {
        te_string_append(&args, "vfio-pci,host=%s%s%s", pt->pci_addr.ptr,
                         pt->vf_token == NULL ? "" : ",vf_token=",
                         te_str_empty_if_null(pt->vf_token));

        te_string_append_shell_args_as_is(cmd, "-device", args.ptr, NULL);

        te_string_reset(&args);
    }

    te_string_free(&args);

    return 0;
}

static te_errno
vm_append_devices_cmd(te_string *cmd, vm_device_list_t *dev_list)
{
    struct vm_device_entry *dev;

    SLIST_FOREACH(dev, dev_list, links)
    {
        te_string_append_shell_args_as_is(cmd, "-device",
                                          dev->device.ptr, NULL);
    }

    return 0;
}

static te_errno
vm_append_cpu_cmd(te_string *cmd, struct vm_entry *vm)
{
    te_string num_arg = TE_STRING_INIT;

    te_string_append_shell_args_as_is(cmd, "-cpu", vm->cpu.model.ptr, NULL);

    te_string_append(&num_arg, "%u", vm->cpu.num);

    te_string_append_shell_args_as_is(cmd, "-smp", num_arg.ptr, NULL);

    te_string_free(&num_arg);

    return 0;
}

static te_errno
vm_append_mem_file_cmd(te_string *cmd, struct vm_entry *vm)
{
    te_string mem_file = TE_STRING_INIT;

    if (vm->mem_size == 0)
    {
        ERROR("Memory size must be set to use memory file");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Attribute 'share' is non configurable yet */
    te_string_append(&mem_file,
                     "memory-backend-file,id=mem,size=%uM,"
                     "mem-path=%s,share=on",
                     vm->mem_size, vm->mem_path);

    te_string_append_shell_args_as_is(cmd, "-object", mem_file.ptr, NULL);
    te_string_free(&mem_file);

    return 0;
}

static te_errno
vm_start(struct vm_entry *vm)
{
    in_addr_t local_ip = htonl(INADDR_LOOPBACK);
    char local_ip_str[INET_ADDRSTRLEN];
    te_string name_str = TE_STRING_INIT;
    te_string machine_str = TE_STRING_INIT;
    te_string net_mgmt_str = TE_STRING_INIT;
    te_string net_mgmt_dev_str = TE_STRING_INIT;
    te_errno rc;

    if (inet_ntop(AF_INET, &local_ip, local_ip_str, sizeof(local_ip_str)) == NULL)
    {
        ERROR("Cannot make local IP address string");
        rc = TE_RC(TE_TA_UNIX, TE_EFAULT);
        goto exit;
    }

    te_string_append(&name_str, "guest=%s", vm->name);

    te_string_append(&net_mgmt_str,
                     "user,id=mgmt,restrict=on,hostfwd=tcp:%s:%hu-:%hu,"
                     "hostfwd=tcp:%s:%hu-:%hu",
                     local_ip_str, vm->host_ssh_port, vm->guest_ssh_port,
                     local_ip_str, vm->rcf_port, vm->rcf_port);

    te_string_append(&net_mgmt_dev_str,
                     "%s,netdev=mgmt,romfile=,addr=0x3",
                     vm->mgmt_net_device);

    te_string_free(&vm->cmd);
    te_string_append_shell_args_as_is(&vm->cmd, vm->qemu,
                                      "-name", name_str.ptr,
                                      "-no-user-config",
                                      "-nodefaults",
                                      "-nographic",
                                      NULL);

    if (vm->kvm)
        te_string_append_shell_args_as_is(&vm->cmd, "-enable-kvm", NULL);

    te_string_append(&machine_str, "%s%s",
                     vm->machine, vm->kvm ? ",accel=kvm" : "");
    te_string_append_shell_args_as_is(&vm->cmd, "-machine", machine_str.ptr,
                                      NULL);

    rc = vm_append_cpu_cmd(&vm->cmd, vm);
    if (rc != 0)
        goto exit;

    if (vm->mem_size != 0)
    {
        te_string mem_size_str = TE_STRING_INIT;

        te_string_append(&mem_size_str, "%uM", vm->mem_size);

        te_string_append_shell_args_as_is(&vm->cmd, "-m", mem_size_str.ptr,
                                          NULL);
        te_string_free(&mem_size_str);
    }

    te_string_append_shell_args_as_is(&vm->cmd,
                                      "-netdev", net_mgmt_str.ptr,
                                      "-device", net_mgmt_dev_str.ptr,
                                      NULL);

    rc = vm_append_chardevs_cmd(&vm->cmd, &vm->chardevs);
    if (rc != 0)
        goto exit;

    rc = vm_append_net_interfaces_cmd(&vm->cmd, &vm->nets, vm);
    if (rc != 0)
        goto exit;

    rc = vm_append_drive_cmd(&vm->cmd, &vm->drives);
    if (rc != 0)
        goto exit;

    rc = vm_append_virtfs_cmd(&vm->cmd, &vm->virtfses);
    if (rc != 0)
        goto exit;

    rc = vm_append_pci_pt_cmd(&vm->cmd, &vm->pci_pts);
    if (rc != 0)
        goto exit;

    rc = vm_append_devices_cmd(&vm->cmd, &vm->devices);
    if (rc != 0)
        goto exit;

    rc = vm_append_kernel_cmd(&vm->cmd, vm);
    if (rc != 0)
        goto exit;

    if (vm->serial != NULL)
    {
        te_string_append_shell_args_as_is(&vm->cmd, "-serial", vm->serial,
                                          NULL);
    }

    if (vm->mem_path != NULL)
    {
        rc = vm_append_mem_file_cmd(&vm->cmd, vm);
        if (rc != 0)
            goto exit;

        te_string_append_shell_args_as_is(&vm->cmd, "-numa",
                                          "node,memdev=mem", NULL);
        if (vm->mem_prealloc)
            te_string_append_shell_args_as_is(&vm->cmd, "-mem-prealloc", NULL);
    }

    RING("VM %s command-line: %s", vm->name, vm->cmd.ptr);

    vm->pid = te_shell_cmd(vm->cmd.ptr, -1, NULL, NULL, NULL);
    if (vm->pid == -1)
    {
        ERROR("Cannot start VM: %s", vm->cmd.ptr);
        rc = TE_RC(TE_TA_UNIX, TE_EFAULT);
        goto exit;
    }

    /*
     * VMs are created and started from CS configuration files which
     * have no ways to wait for configuration changes yet. Add a delay
     * here to let process to start and create interfaces which are
     * typically required for further processing.
     */
    te_msleep(200);

exit:
    te_string_free(&name_str);
    te_string_free(&machine_str);
    te_string_free(&net_mgmt_str);
    te_string_free(&net_mgmt_dev_str);
    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_stop(struct vm_entry *vm)
{
    te_errno rc;

    if (ta_kill_death(vm->pid) == 0)
        rc = 0;
    else
        rc = TE_ENOENT;

    return TE_RC(TE_TA_UNIX, rc);
}


/*
 * Configuration model implementation
 */

static void
vm_chardev_free(struct vm_chardev_entry *chardev)
{
    free(chardev->name);
    free(chardev->path);
    free(chardev);
}

static void
vm_net_free(struct vm_net_entry *net)
{
    te_kvpair_fini(&net->netdev_opts);
    te_kvpair_fini(&net->device_opts);
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

static struct vm_chardev_entry *
vm_chardev_find(const struct vm_entry *vm, const char *name)
{
    struct vm_chardev_entry *p;

    if (vm == NULL)
        return NULL;

    SLIST_FOREACH(p, &vm->chardevs, links)
    {
        if (strcmp(name, p->name) == 0)
            return p;
    }

    return NULL;
}

static void
vm_drive_free(struct vm_drive_entry *drive)
{
    free(drive->name);
    te_string_free(&drive->file);
    free(drive);
}

static void
vm_virtfs_free(struct vm_virtfs_entry *vfs)
{
    free(vfs->fsdriver);
    free(vfs->mount_tag);
    free(vfs->path);
    free(vfs->security_model);
    free(vfs->name);
    free(vfs);
}

static void
vm_pci_pt_free(struct vm_pci_pt_entry *pt)
{
    free(pt->name);
    te_string_free(&pt->pci_addr);
    free(pt->vf_token);
    free(pt);
}

static void
vm_device_free(struct vm_device_entry *dev)
{
    free(dev->name);
    te_string_free(&dev->device);
    free(dev);
}

static void
vm_free(struct vm_entry *vm)
{
    struct vm_chardev_entry *chardev;
    struct vm_chardev_entry *chardev_tmp;
    struct vm_net_entry *net;
    struct vm_net_entry *net_tmp;
    struct vm_drive_entry *drive;
    struct vm_drive_entry *drive_tmp;
    struct vm_virtfs_entry *virtfs;
    struct vm_virtfs_entry *virtfs_tmp;
    struct vm_pci_pt_entry *pci_pt;
    struct vm_pci_pt_entry *pci_pt_tmp;
    struct vm_device_entry *device;
    struct vm_device_entry *device_tmp;

    SLIST_FOREACH_SAFE(chardev, &vm->chardevs, links, chardev_tmp)
    {
        SLIST_REMOVE(&vm->chardevs, chardev, vm_chardev_entry, links);
        vm_chardev_free(chardev);
    }

    SLIST_FOREACH_SAFE(net, &vm->nets, links, net_tmp)
    {
        SLIST_REMOVE(&vm->nets, net, vm_net_entry, links);
        vm_net_free(net);
    }

    SLIST_FOREACH_SAFE(drive, &vm->drives, links, drive_tmp)
    {
        SLIST_REMOVE(&vm->drives, drive, vm_drive_entry, links);
        vm_drive_free(drive);
    }

    SLIST_FOREACH_SAFE(virtfs, &vm->virtfses, links, virtfs_tmp)
    {
        SLIST_REMOVE(&vm->virtfses, virtfs, vm_virtfs_entry, links);
        vm_virtfs_free(virtfs);
    }

    SLIST_FOREACH_SAFE(pci_pt, &vm->pci_pts, links, pci_pt_tmp)
    {
        SLIST_REMOVE(&vm->pci_pts, pci_pt, vm_pci_pt_entry, links);
        vm_pci_pt_free(pci_pt);
    }

    SLIST_FOREACH_SAFE(device, &vm->devices, links, device_tmp)
    {
        SLIST_REMOVE(&vm->devices, device, vm_device_entry, links);
        vm_device_free(device);
    }

    agent_free_l4_port(vm->rcf_port);
    agent_free_l4_port(vm->host_ssh_port);

    te_string_free(&vm->cmd);
    te_string_free(&vm->cpu.model);
    free(vm->mem_path);
    free(vm->mgmt_net_device);
    free(vm->machine);
    free(vm->qemu);
    free(vm->name);
    free(vm->kernel);
    free(vm->ker_cmd);
    free(vm->ker_initrd);
    free(vm->ker_dtb);
    free(vm->serial);
    free(vm);
}

static struct vm_drive_entry *
vm_drive_find(const struct vm_entry *vm, const char *name)
{
    struct vm_drive_entry *p;

    if (vm == NULL)
        return NULL;

    SLIST_FOREACH(p, &vm->drives, links)
    {
        if (strcmp(name, p->name) == 0)
            return p;
    }

    return NULL;
}

static struct vm_virtfs_entry *
vm_virtfs_find(const struct vm_entry *vm, const char *name)
{
    struct vm_virtfs_entry *p;

    if (vm == NULL)
        return NULL;

    SLIST_FOREACH(p, &vm->virtfses, links)
    {
        if (strcmp(name, p->name) == 0)
            return p;
    }

    return NULL;
}

static struct vm_pci_pt_entry *
vm_pci_pt_find(const struct vm_entry *vm, const char *name)
{
    struct vm_pci_pt_entry *p;

    if (vm == NULL)
        return NULL;

    SLIST_FOREACH(p, &vm->pci_pts, links)
    {
        if (strcmp(name, p->name) == 0)
            return p;
    }

    return NULL;
}

static struct vm_device_entry *
vm_device_find(const struct vm_entry *vm, const char *name)
{
    struct vm_device_entry *p;

    if (vm == NULL)
        return NULL;

    SLIST_FOREACH(p, &vm->devices, links)
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

/**
 * Put a string value into the get buffer, truncating it to what the
 * legacy fixed-size buffer could hold.
 */
static void
vm_value_put(te_string *val, const char *str)
{
    te_string_append(val, "%.*s", RCF_MAX_VAL - 1, str);
}

static te_errno
vm_list(const ta_conf_ctx *ctx, te_vec *names)
{
    struct vm_entry *vm;

    UNUSED(ctx);

    SLIST_FOREACH(vm, &vms, links)
    {
        char *name = TE_STRDUP(vm->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
vm_add(const ta_conf_ctx *ctx)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;
    te_errno rc;

    ENTRY("%s", vm_name);

    if (vm_find(vm_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    vm = TE_ALLOC(sizeof(*vm));

    SLIST_INIT(&vm->nets);
    SLIST_INIT(&vm->drives);
    SLIST_INIT(&vm->virtfses);
    SLIST_INIT(&vm->pci_pts);
    SLIST_INIT(&vm->devices);
    vm->cmd = (te_string)TE_STRING_INIT;

    vm->name = strdup(vm_name);
    if (vm->name == NULL)
    {
        vm_free(vm);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    vm->qemu = strdup(VM_QEMU_DEFAULT);
    if (vm->qemu == NULL)
    {
        vm_free(vm);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    vm->machine = strdup(VM_MACHINE_DEFAULT);
    if (vm->machine == NULL)
    {
        vm_free(vm);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    vm->cpu.model = (te_string)TE_STRING_INIT;
    te_string_append(&vm->cpu.model, "host");

    vm->mgmt_net_device = strdup(VM_MGMT_NET_DEVICE_DEFAULT);
    if (vm->mgmt_net_device == NULL)
    {
        vm_free(vm);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    rc = agent_alloc_l4_port(AF_INET, SOCK_STREAM, &vm->host_ssh_port);
    if (rc != 0)
    {
        vm_free(vm);
        return TE_RC(TE_TA_UNIX, rc);
    }

    rc = agent_alloc_l4_port(AF_INET, SOCK_STREAM, &vm->rcf_port);
    if (rc != 0)
    {
        vm_free(vm);
        return TE_RC(TE_TA_UNIX, rc);
    }

    vm->cpu.num = 1;

    vm->kvm = kvm_supported;
    vm->guest_ssh_port = guest_ssh_port;

    vm->pid = -1;
    vm->serial = strdup("stdio");
    if (vm->serial == NULL)
    {
        vm_free(vm);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    SLIST_INSERT_HEAD(&vms, vm, links);

    return 0;
}

static te_errno
vm_del(const ta_conf_ctx *ctx)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vms, vm, vm_entry, links);

    if (vm_is_running(vm))
        (void)vm_stop(vm);

    vm_free(vm);

    return 0;
}

static te_errno
vm_qemu_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, vm->qemu);

    return 0;
}

static te_errno
vm_qemu_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    if (te_str_is_null_or_empty(value))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->qemu, value);
}

static te_errno
vm_status_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = vm_is_running(vm);

    return 0;
}

static te_errno
vm_status_set(const ta_conf_ctx *ctx, int32_t val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;
    bool enable = (val != 0);
    te_errno rc;

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

    return TE_RC(TE_TA_UNIX, rc);
}

static uint16_t *
vm_port_ptr_by_oid(struct vm_entry *vm, const char *oid)
{
    cfg_oid *coid = cfg_convert_oid_str(oid);
    uint16_t *result = NULL;

    if (coid == NULL)
        return NULL;

    if (strcmp(cfg_oid_inst_subid(coid, 3), "rcf_port") == 0)
    {
        result = &vm->rcf_port;
    }
    else if (strcmp(cfg_oid_inst_subid(coid, 3), "ssh_port") == 0)
    {
        if (strcmp(cfg_oid_inst_subid(coid, 4), "host") == 0)
            result = &vm->host_ssh_port;
        else if (strcmp(cfg_oid_inst_subid(coid, 4), "guest") == 0)
            result = &vm->guest_ssh_port;
    }

    cfg_free_oid(coid);

    return result;
}

static te_errno
vm_port_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    const char *oid = ta_conf_ctx_oid(ctx);
    struct vm_entry *vm;
    uint16_t *port;

    ENTRY("%s oid=%s", vm_name, oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    port = vm_port_ptr_by_oid(vm, oid);
    if (port == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = *port;

    return 0;
}

static te_errno
vm_port_set(const ta_conf_ctx *ctx, int32_t val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;
    uint16_t *port;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    port = vm_port_ptr_by_oid(vm, ta_conf_ctx_oid(ctx));
    if (port == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (val < 0 || val > UINT16_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *port = val;

    return 0;
}


static te_errno
vm_kvm_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = vm->kvm;

    return 0;
}

static te_errno
vm_kvm_set(const ta_conf_ctx *ctx, int32_t value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;
    bool val;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    val = (value != 0);

    if (!kvm_supported && val)
        WARN("KVM is not supported, but requested");

    vm->kvm = val;

    return 0;
}

static te_errno
vm_machine_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, vm->machine);

    return 0;
}

static te_errno
vm_machine_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    if (te_str_is_null_or_empty(value))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->machine, value);
}

static te_errno
vm_mem_size_get(const ta_conf_ctx *ctx, int32_t *val)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = vm->mem_size;

    return 0;
}

static te_errno
vm_mem_size_set(const ta_conf_ctx *ctx, int32_t val)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (val < 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm->mem_size = val;

    return 0;
}

static te_errno
vm_mem_path_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(vm->mem_path));

    return 0;
}

static te_errno
vm_mem_path_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->mem_path, value);
}

static te_errno
vm_mem_prealloc_get(const ta_conf_ctx *ctx, int32_t *val)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = vm->mem_prealloc;

    return 0;
}

static te_errno
vm_mem_prealloc_set(const ta_conf_ctx *ctx, int32_t val)
{
    struct vm_entry *vm;
    bool enable = (val != 0);

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    vm->mem_prealloc = enable;

    return 0;
}

static te_errno
vm_chardev_add(const ta_conf_ctx *ctx)
{
    const char *chardev_name = ta_conf_ctx_inst(ctx, "chardev");
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;

    if ((vm = vm_find(ta_conf_ctx_inst(ctx, "vm"))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_chardev_find(vm, chardev_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    chardev = TE_ALLOC(sizeof(*chardev));

    chardev->name = strdup(chardev_name);
    if (chardev->name == NULL)
    {
        free(chardev);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    SLIST_INSERT_HEAD(&vm->chardevs, chardev, links);

    return 0;
}

static te_errno
vm_chardev_del(const ta_conf_ctx *ctx)
{
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    chardev = vm_chardev_find(vm, ta_conf_ctx_inst(ctx, "chardev"));
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->chardevs, chardev, vm_chardev_entry, links);

    vm_chardev_free(chardev);

    return 0;
}

static te_errno
vm_chardev_list(const ta_conf_ctx *ctx, te_vec *names)
{
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(chardev, &vm->chardevs, links)
    {
        char *name = TE_STRDUP(chardev->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
vm_mgmt_net_device_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, vm->mgmt_net_device);

    return 0;
}

static te_errno
vm_mgmt_net_device_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    if (te_str_is_null_or_empty(value))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->mgmt_net_device, value);
}

static te_errno
vm_net_add(const ta_conf_ctx *ctx)
{
    const char *net_name = ta_conf_ctx_inst(ctx, "net");
    struct vm_entry *vm;
    struct vm_net_entry *net;

    if ((vm = vm_find(ta_conf_ctx_inst(ctx, "vm"))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_net_find(vm, net_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    net = TE_ALLOC(sizeof(*net));

    net->name = strdup(net_name);
    if (net->name == NULL)
    {
        free(net);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    te_kvpair_init(&net->netdev_opts);
    te_kvpair_init(&net->device_opts);

    SLIST_INSERT_HEAD(&vm->nets, net, links);

    return 0;
}

static te_errno
vm_net_del(const ta_conf_ctx *ctx)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    net = vm_net_find(vm, ta_conf_ctx_inst(ctx, "net"));
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->nets, net, vm_net_entry, links);

    vm_net_free(net);

    return 0;
}

static te_errno
vm_net_list(const ta_conf_ctx *ctx, te_vec *names)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(net, &vm->nets, links)
    {
        char *name = TE_STRDUP(net->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
vm_chardev_server_get(const ta_conf_ctx *ctx, int32_t *val)
{
    struct vm_chardev_entry *chardev;

    chardev = vm_chardev_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                              ta_conf_ctx_inst(ctx, "chardev"));
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = chardev->server;

    return 0;
}

static te_errno
vm_chardev_server_set(const ta_conf_ctx *ctx, int32_t val)
{
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;
    bool is_server = (val != 0);

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    chardev = vm_chardev_find(vm, ta_conf_ctx_inst(ctx, "chardev"));
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    chardev->server = is_server;

    return 0;
}

static te_errno
vm_chardev_path_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_chardev_entry *chardev;

    chardev = vm_chardev_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                              ta_conf_ctx_inst(ctx, "chardev"));
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(chardev->path));

    return 0;
}

static te_errno
vm_chardev_path_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    chardev = vm_chardev_find(vm, ta_conf_ctx_inst(ctx, "chardev"));
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&chardev->path, value);
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

static te_kvpair_h *
vm_net_opts_by_subid(struct vm_net_entry *net, const char *sub_id)
{
    if (strcmp(sub_id, VM_NET_NETDEV_OPT) == 0)
        return &net->netdev_opts;
    else if (strcmp(sub_id, VM_NET_DEVICE_OPT) == 0)
        return &net->device_opts;

    return NULL;
}

static te_errno
vm_net_opt_add(const ta_conf_ctx *ctx, const char *sub_id,
               const char *value)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;
    te_kvpair_h *kvp;
    te_errno rc;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    net = vm_net_find(vm, ta_conf_ctx_inst(ctx, "net"));
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    kvp = vm_net_opts_by_subid(net, sub_id);
    if (kvp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_kvpair_add(kvp, ta_conf_ctx_inst(ctx, sub_id), "%s", value);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_net_opt_del(const ta_conf_ctx *ctx, const char *sub_id)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;
    te_kvpair_h *kvp;
    te_errno rc;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    net = vm_net_find(vm, ta_conf_ctx_inst(ctx, "net"));
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    kvp = vm_net_opts_by_subid(net, sub_id);
    if (kvp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_kvpairs_del(kvp, ta_conf_ctx_inst(ctx, sub_id));

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_net_opt_list(const ta_conf_ctx *ctx, const char *sub_id, te_vec *names)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;
    te_kvpair_h *kvp;
    te_kvpair *p;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    net = vm_net_find(vm, ta_conf_ctx_inst(ctx, "net"));
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    kvp = vm_net_opts_by_subid(net, sub_id);
    if (kvp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    /* Same order as te_kvpair_keys_to_str() */
    TAILQ_FOREACH_REVERSE(p, kvp, te_kvpair_h, links)
    {
        char *name = TE_STRDUP(p->key);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
vm_net_opt_get(const ta_conf_ctx *ctx, const char *sub_id, te_string *val)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;
    te_kvpair_h *kvp;
    const char *opt;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    net = vm_net_find(vm, ta_conf_ctx_inst(ctx, "net"));
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    kvp = vm_net_opts_by_subid(net, sub_id);
    if (kvp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    opt = te_kvpairs_get(kvp, ta_conf_ctx_inst(ctx, sub_id));
    if (opt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, opt);

    return 0;
}

static te_errno
vm_net_netdev_opt_get(const ta_conf_ctx *ctx, te_string *val)
{
    return vm_net_opt_get(ctx, VM_NET_NETDEV_OPT, val);
}

static te_errno
vm_net_netdev_opt_add(const ta_conf_ctx *ctx, const char *value)
{
    return vm_net_opt_add(ctx, VM_NET_NETDEV_OPT, value);
}

static te_errno
vm_net_netdev_opt_del(const ta_conf_ctx *ctx)
{
    return vm_net_opt_del(ctx, VM_NET_NETDEV_OPT);
}

static te_errno
vm_net_netdev_opt_list(const ta_conf_ctx *ctx, te_vec *names)
{
    return vm_net_opt_list(ctx, VM_NET_NETDEV_OPT, names);
}

static te_errno
vm_net_device_opt_get(const ta_conf_ctx *ctx, te_string *val)
{
    return vm_net_opt_get(ctx, VM_NET_DEVICE_OPT, val);
}

static te_errno
vm_net_device_opt_add(const ta_conf_ctx *ctx, const char *value)
{
    return vm_net_opt_add(ctx, VM_NET_DEVICE_OPT, value);
}

static te_errno
vm_net_device_opt_del(const ta_conf_ctx *ctx)
{
    return vm_net_opt_del(ctx, VM_NET_DEVICE_OPT);
}

static te_errno
vm_net_device_opt_list(const ta_conf_ctx *ctx, te_vec *names)
{
    return vm_net_opt_list(ctx, VM_NET_DEVICE_OPT, names);
}

static te_errno
vm_net_property_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_net_entry *net;
    char **property;

    net = vm_net_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                      ta_conf_ctx_inst(ctx, "net"));
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    property = vm_net_property_ptr_by_oid(net, ta_conf_ctx_oid(ctx));
    if (property == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(*property));

    return 0;
}

static te_errno
vm_net_property_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;
    struct vm_net_entry *net;
    char **property;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    net = vm_net_find(vm, ta_conf_ctx_inst(ctx, "net"));
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    property = vm_net_property_ptr_by_oid(net, ta_conf_ctx_oid(ctx));
    if (property == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(property, value);
}

static te_errno
vm_kernel_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(vm->kernel));

    return 0;
}

static te_errno
vm_kernel_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->kernel, value);
}

static te_errno
vm_ker_cmd_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(vm->ker_cmd));

    return 0;
}

static te_errno
vm_ker_cmd_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->ker_cmd, value);
}

static te_errno
vm_ker_initrd_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(vm->ker_initrd));

    return 0;
}

static te_errno
vm_ker_initrd_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->ker_initrd, value);
}

static te_errno
vm_ker_dtb_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(vm->ker_dtb));

    return 0;
}

static te_errno
vm_ker_dtb_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->ker_dtb, value);
}

static te_errno
vm_serial_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(vm->serial));

    return 0;
}

static te_errno
vm_serial_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *vm_name = ta_conf_ctx_inst(ctx, "vm");
    struct vm_entry *vm;

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->serial, value);
}

static te_errno
vm_drive_add(const ta_conf_ctx *ctx)
{
    const char *drive_name = ta_conf_ctx_inst(ctx, "drive");
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    if ((vm = vm_find(ta_conf_ctx_inst(ctx, "vm"))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_drive_find(vm, drive_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    drive = TE_ALLOC(sizeof(*drive));

    drive->name = strdup(drive_name);
    if (drive->name == NULL)
    {
        free(drive);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    drive->file = (te_string)TE_STRING_INIT;

    SLIST_INSERT_HEAD(&vm->drives, drive, links);

    return 0;
}

static te_errno
vm_drive_del(const ta_conf_ctx *ctx)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    drive = vm_drive_find(vm, ta_conf_ctx_inst(ctx, "drive"));
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->drives, drive, vm_drive_entry, links);

    vm_drive_free(drive);

    return 0;
}

static te_errno
vm_drive_list(const ta_conf_ctx *ctx, te_vec *names)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(drive, &vm->drives, links)
    {
        char *name = TE_STRDUP(drive->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
vm_file_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_drive_entry *drive;

    drive = vm_drive_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                          ta_conf_ctx_inst(ctx, "drive"));
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_string_value(&drive->file));

    return 0;
}

static te_errno
vm_file_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    drive = vm_drive_find(vm, ta_conf_ctx_inst(ctx, "drive"));
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_free(&drive->file);
    te_string_append(&drive->file, "%s",  value);

    return 0;
}

static te_errno
vm_snapshot_get(const ta_conf_ctx *ctx, int32_t *val)
{
    struct vm_drive_entry *drive;

    drive = vm_drive_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                          ta_conf_ctx_inst(ctx, "drive"));
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = drive->snapshot;

    return 0;
}

static te_errno
vm_snapshot_set(const ta_conf_ctx *ctx, int32_t val)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    drive = vm_drive_find(vm, ta_conf_ctx_inst(ctx, "drive"));
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    drive->snapshot = (val != 0);

    return 0;
}

static te_errno
vm_drive_cdrom_get(const ta_conf_ctx *ctx, int32_t *val)
{
    struct vm_drive_entry *drive;

    drive = vm_drive_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                          ta_conf_ctx_inst(ctx, "drive"));
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = drive->cdrom;

    return 0;
}

static te_errno
vm_drive_cdrom_set(const ta_conf_ctx *ctx, int32_t val)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    drive = vm_drive_find(vm, ta_conf_ctx_inst(ctx, "drive"));
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    drive->cdrom = (val != 0);

    return 0;
}

static te_errno
vm_virtfs_add(const ta_conf_ctx *ctx)
{
    const char *virtfs_name = ta_conf_ctx_inst(ctx, "virtfs");
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    if ((vm = vm_find(ta_conf_ctx_inst(ctx, "vm"))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_virtfs_find(vm, virtfs_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    virtfs = TE_ALLOC(sizeof(*virtfs));

    virtfs->name = strdup(virtfs_name);
    if (virtfs->name == NULL)
    {
        free(virtfs);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    SLIST_INSERT_HEAD(&vm->virtfses, virtfs, links);

    return 0;
}

static te_errno
vm_virtfs_del(const ta_conf_ctx *ctx)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->virtfses, virtfs, vm_virtfs_entry, links);

    vm_virtfs_free(virtfs);

    return 0;
}

static te_errno
vm_virtfs_list(const ta_conf_ctx *ctx, te_vec *names)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *vfs;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(vfs, &vm->virtfses, links)
    {
        char *name = TE_STRDUP(vfs->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
vm_virtfs_fsdriver_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_virtfs_entry *virtfs;

    virtfs = vm_virtfs_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                            ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(virtfs->fsdriver));

    return 0;
}

static te_errno
vm_virtfs_fsdriver_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&virtfs->fsdriver, value);
}

static te_errno
vm_virtfs_mount_tag_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_virtfs_entry *virtfs;

    virtfs = vm_virtfs_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                            ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(virtfs->mount_tag));

    return 0;
}

static te_errno
vm_virtfs_mount_tag_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&virtfs->mount_tag, value);
}

static te_errno
vm_virtfs_path_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_virtfs_entry *virtfs;

    virtfs = vm_virtfs_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                            ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(virtfs->path));

    return 0;
}

static te_errno
vm_virtfs_path_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&virtfs->path, value);
}

static te_errno
vm_virtfs_security_model_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_virtfs_entry *virtfs;

    virtfs = vm_virtfs_find(vm_find(ta_conf_ctx_inst(ctx, "vm")),
                            ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(virtfs->security_model));

    return 0;
}

static te_errno
vm_virtfs_security_model_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, ta_conf_ctx_inst(ctx, "virtfs"));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&virtfs->security_model, value);
}


static te_errno
vm_pci_pt_token_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *pci_pt_name = ta_conf_ctx_inst(ctx, "pci_pt");
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt = NULL;

    if ((vm = vm_find(ta_conf_ctx_inst(ctx, "vm"))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(pt, &vm->pci_pts, links)
    {
        if (strcmp(pci_pt_name, pt->name) == 0)
            break;
    }
    if (pt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, te_str_empty_if_null(pt->vf_token));

    return 0;
}

static te_errno
vm_pci_pt_token_set(const ta_conf_ctx *ctx, const char *value)
{
    const char *pci_pt_name = ta_conf_ctx_inst(ctx, "pci_pt");
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt = NULL;

    if ((vm = vm_find(ta_conf_ctx_inst(ctx, "vm"))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    SLIST_FOREACH(pt, &vm->pci_pts, links)
    {
        if (strcmp(pci_pt_name, pt->name) == 0)
            break;
    }
    if (pt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&pt->vf_token, value);
}

static te_errno
vm_pci_pt_add(const ta_conf_ctx *ctx, const char *value)
{
    const char *pci_pt_name = ta_conf_ctx_inst(ctx, "pci_pt");
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt = NULL;

    if (rcf_pch_rsrc_accessible("%s", value))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_pci_pt_find(vm, pci_pt_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    pt = TE_ALLOC(sizeof(*pt));

    pt->name = strdup(pci_pt_name);
    if (pt->name == NULL)
    {
        free(pt);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    pt->pci_addr = (te_string)TE_STRING_INIT;
    te_string_append(&pt->pci_addr, "%s",  value);

    SLIST_INSERT_HEAD(&vm->pci_pts, pt, links);

    return 0;
}

static te_errno
vm_pci_pt_del(const ta_conf_ctx *ctx)
{
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    pt = vm_pci_pt_find(vm, ta_conf_ctx_inst(ctx, "pci_pt"));
    if (pt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->pci_pts, pt, vm_pci_pt_entry, links);

    vm_pci_pt_free(pt);

    return 0;
}

static te_errno
vm_pci_pt_list(const ta_conf_ctx *ctx, te_vec *names)
{
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(pt, &vm->pci_pts, links)
    {
        char *name = TE_STRDUP(pt->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
vm_pci_pt_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *pci_pt_name = ta_conf_ctx_inst(ctx, "pci_pt");
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt = NULL;

    if ((vm = vm_find(ta_conf_ctx_inst(ctx, "vm"))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(pt, &vm->pci_pts, links)
    {
        if (strcmp(pci_pt_name, pt->name) == 0)
            break;
    }

    if (pt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, pt->pci_addr.ptr);

    return 0;
}



static te_errno
vm_device_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_entry *vm;
    struct vm_device_entry *dev;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    dev = vm_device_find(vm, ta_conf_ctx_inst(ctx, "device"));
    if (dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, dev->device.ptr);

    return 0;
}

static te_errno
vm_device_add(const ta_conf_ctx *ctx, const char *value)
{
    const char *device_name = ta_conf_ctx_inst(ctx, "device");
    struct vm_entry *vm;
    struct vm_device_entry *dev;

    if (rcf_pch_rsrc_accessible("%s", value))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_device_find(vm, device_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    dev = TE_ALLOC(sizeof(*dev));

    dev->name = strdup(device_name);
    if (dev->name == NULL)
    {
        vm_device_free(dev);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    dev->device = (te_string)TE_STRING_INIT;
    te_string_append(&dev->device, "%s",  value);

    SLIST_INSERT_HEAD(&vm->devices, dev, links);

    return 0;
}

static te_errno
vm_device_del(const ta_conf_ctx *ctx)
{
    struct vm_entry *vm;
    struct vm_device_entry *dev;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    dev = vm_device_find(vm, ta_conf_ctx_inst(ctx, "device"));
    if (dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->devices, dev, vm_device_entry, links);

    vm_device_free(dev);

    return 0;
}

static te_errno
vm_device_list(const ta_conf_ctx *ctx, te_vec *names)
{
    struct vm_entry *vm;
    struct vm_device_entry *dev;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(dev, &vm->devices, links)
    {
        char *name = TE_STRDUP(dev->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
vm_cpu_model_get(const ta_conf_ctx *ctx, te_string *val)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    vm_value_put(val, vm->cpu.model.ptr);

    return 0;
}

static te_errno
vm_cpu_model_set(const ta_conf_ctx *ctx, const char *value)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    te_string_free(&vm->cpu.model);
    te_string_append(&vm->cpu.model, "%s",  value);

    return 0;
}

static te_errno
vm_cpu_num_get(const ta_conf_ctx *ctx, int32_t *val)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = vm->cpu.num;

    return 0;
}

static te_errno
vm_cpu_num_set(const ta_conf_ctx *ctx, int32_t val)
{
    struct vm_entry *vm;

    vm = vm_find(ta_conf_ctx_inst(ctx, "vm"));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (val < 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm->cpu.num = val;

    return 0;
}

static const ta_conf_node *const node_vm =
    TA_CONF_COLL("vm", vm_add, vm_del, vm_list,
        TA_CONF_RW_STR("qemu", vm_qemu_get, vm_qemu_set),
        TA_CONF_RW_INT32("status", vm_status_get, vm_status_set),
        TA_CONF_NA("ssh_port",
            TA_CONF_RO_INT32("host", vm_port_get),
            TA_CONF_RW_INT32("guest", vm_port_get, vm_port_set)),
        TA_CONF_RW_INT32("rcf_port", vm_port_get, vm_port_set),
        TA_CONF_RW_INT32("kvm", vm_kvm_get, vm_kvm_set),
        TA_CONF_RW_STR("machine", vm_machine_get, vm_machine_set),
        TA_CONF_NA("mem",
            TA_CONF_RW_INT32("size", vm_mem_size_get, vm_mem_size_set),
            TA_CONF_RW_STR("path", vm_mem_path_get, vm_mem_path_set),
            TA_CONF_RW_INT32("prealloc", vm_mem_prealloc_get,
                             vm_mem_prealloc_set)),
        TA_CONF_COLL("chardev", vm_chardev_add, vm_chardev_del,
                     vm_chardev_list,
            TA_CONF_RW_STR("path", vm_chardev_path_get, vm_chardev_path_set),
            TA_CONF_RW_INT32("server", vm_chardev_server_get,
                             vm_chardev_server_set)),
        TA_CONF_RW_STR("mgmt_net_device", vm_mgmt_net_device_get,
                       vm_mgmt_net_device_set),
        TA_CONF_COLL("net", vm_net_add, vm_net_del, vm_net_list,
            TA_CONF_RW_STR("type", vm_net_property_get,
                           vm_net_property_set),
            TA_CONF_RW_STR("type_spec", vm_net_property_get,
                           vm_net_property_set),
            TA_CONF_RW_STR("mac_addr", vm_net_property_get,
                           vm_net_property_set),
            TA_CONF_COLL_STR(VM_NET_NETDEV_OPT, vm_net_netdev_opt_get,
                             vm_net_netdev_opt_add, vm_net_netdev_opt_del,
                             vm_net_netdev_opt_list),
            TA_CONF_COLL_STR(VM_NET_DEVICE_OPT, vm_net_device_opt_get,
                             vm_net_device_opt_add, vm_net_device_opt_del,
                             vm_net_device_opt_list)),
        TA_CONF_RW_STR("kernel", vm_kernel_get, vm_kernel_set,
            TA_CONF_RW_STR("dtb", vm_ker_dtb_get, vm_ker_dtb_set),
            TA_CONF_RW_STR("initrd", vm_ker_initrd_get, vm_ker_initrd_set),
            TA_CONF_RW_STR("cmdline", vm_ker_cmd_get, vm_ker_cmd_set)),
        TA_CONF_COLL("drive", vm_drive_add, vm_drive_del, vm_drive_list,
            TA_CONF_RW_STR("file", vm_file_get, vm_file_set),
            TA_CONF_RW_INT32("snapshot", vm_snapshot_get, vm_snapshot_set),
            TA_CONF_RW_INT32("cdrom", vm_drive_cdrom_get,
                             vm_drive_cdrom_set)),
        TA_CONF_COLL("virtfs", vm_virtfs_add, vm_virtfs_del, vm_virtfs_list,
            TA_CONF_RW_STR("security_model", vm_virtfs_security_model_get,
                           vm_virtfs_security_model_set),
            TA_CONF_RW_STR("mount_tag", vm_virtfs_mount_tag_get,
                           vm_virtfs_mount_tag_set),
            TA_CONF_RW_STR("path", vm_virtfs_path_get, vm_virtfs_path_set),
            TA_CONF_RW_STR("fsdriver", vm_virtfs_fsdriver_get,
                           vm_virtfs_fsdriver_set)),
        TA_CONF_NA("cpu",
            TA_CONF_RW_STR("model", vm_cpu_model_get, vm_cpu_model_set),
            TA_CONF_RW_INT32("num", vm_cpu_num_get, vm_cpu_num_set)),
        TA_CONF_COLL_STR("pci_pt", vm_pci_pt_get, vm_pci_pt_add,
                         vm_pci_pt_del, vm_pci_pt_list,
            TA_CONF_RW_STR("vf_token", vm_pci_pt_token_get,
                           vm_pci_pt_token_set)),
        TA_CONF_COLL_STR("device", vm_device_get, vm_device_add,
                         vm_device_del, vm_device_list),
        TA_CONF_RW_STR("serial", vm_serial_get, vm_serial_set));


static bool
check_kvm(void)
{
    bool supported = false;
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
            supported = true;
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

    return ta_conf_register("/agent", node_vm);
}

#else /* !__linux__ */

te_errno
ta_unix_conf_vm_init(void)
{
    WARN("Virtual machines configuration is not supported");
    return 0;
}

#endif /* !__linux__ */
