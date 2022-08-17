/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA virtual machines support
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
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
#include "te_sleep.h"
#include "te_string.h"
#include "te_shell_cmd.h"
#include "te_str.h"
#include "rcf_pch.h"
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
    "pc-i440fx-2.8,usb=off,vmport=off,dump-guest-core=off"

/** Default managment network device */
#define VM_MGMT_NET_DEVICE_DEFAULT  "virtio-net-pci"

typedef struct vm_cpu {
    te_string       model;
    unsigned int    num;
} vm_cpu;

struct vm_drive_entry {
    SLIST_ENTRY(vm_drive_entry)     links;
    char                           *name;
    te_string                       file;
    te_bool                         snapshot;
    te_bool                         cdrom;
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
    te_bool                         server;
};

struct vm_net_entry {
    SLIST_ENTRY(vm_net_entry)   links;
    char                       *name;
    char                       *type;
    char                       *type_spec;
    char                       *mac_addr;
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
    te_bool                 kvm;
    uint16_t                host_ssh_port;
    uint16_t                guest_ssh_port;
    uint16_t                rcf_port;
    vm_cpu                  cpu;
    unsigned int            mem_size;
    char                   *mem_path;
    te_bool                 mem_prealloc;
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
static te_bool kvm_supported;

static te_bool
vm_is_running(struct vm_entry *vm)
{
    pid_t ret;

    if (vm->pid == -1)
        return FALSE;

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
        return FALSE;
    }

    return TRUE;
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
        return TE_RC(TE_TA_UNIX, rc);
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

    rc = te_string_append(&netdev,
            "tap,script=no,downscript=no,id=netdev%u%s%s%s",
            interface_id, vhost ? ",vhost=on" : "",
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

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_vhost_user_interface_cmd(te_string *cmd, struct vm_net_entry *net,
                                   unsigned int interface_id,
                                   const struct vm_entry *vm)
{
    te_string netdev = TE_STRING_INIT;
    te_string device = TE_STRING_INIT;
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

    rc = te_string_append(&netdev,
                          "type=vhost-user,id=netdev%u,chardev=%s,vhostforce",
                          interface_id, net->type_spec);
    if (rc != 0)
    {
        ERROR("Cannot compose VM netdev argument: %r", rc);
        goto exit;
    }

    rc = vm_append_virtio_dev_cmd(&device, net->mac_addr, interface_id);
    if (rc != 0)
        goto exit;

    rc = te_string_append_shell_args_as_is(cmd, "-netdev", netdev.ptr,
                                           "-device", device.ptr, NULL);
    if (rc != 0)
    {
        ERROR("Cannot compose VM net interface command line: %r", rc);
        goto exit;
    }

exit:
    te_string_free(&netdev);
    te_string_free(&device);

    return TE_RC(TE_TA_UNIX, rc);
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

        rc = te_string_append(&chardev_arg, "socket,id=%s,path=%s%s",
                              chardev->name, chardev->path,
                              chardev->server ? ",server" : "");
        if (rc != 0)
        {
            ERROR("Cannot compose VM chardev argument: %r", rc);
            goto exit;
        }

        rc = te_string_append_shell_args_as_is(&chardev_args, "-chardev",
                                               chardev_arg.ptr, NULL);
        te_string_free(&chardev_arg);
        if (rc != 0)
        {
            ERROR("Cannot compose VM character device list: %r", rc);
            goto exit;
        }
    }

    if (chardev_args.ptr != NULL)
    {
        rc = te_string_append(cmd, " %s", chardev_args.ptr);
        if (rc != 0)
            ERROR("Cannot append character device list to VM cmdline: %r", rc);
    }

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

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_kernel_cmd(te_string *cmd, struct vm_entry *vm)
{
    te_errno rc;

    if (vm->kernel != NULL)
    {
        rc = te_string_append_shell_args_as_is(cmd, "-kernel", vm->kernel, NULL);
        if (rc != 0)
        {
            ERROR("Cannot compose kernel command line (line %u): %r",
                  __LINE__, rc);
            return TE_RC(TE_TA_UNIX, rc);
        }

        if (vm->ker_cmd != NULL)
        {
            rc = te_string_append_shell_args_as_is(cmd, "-append",
                            vm->ker_cmd, NULL);
            if (rc != 0)
            {
                ERROR("Cannot compose kernel command line (line %u): %r",
                      __LINE__, rc);
                return TE_RC(TE_TA_UNIX, rc);
            }
        }

        if (vm->ker_initrd != NULL)
        {
            rc = te_string_append_shell_args_as_is(cmd, "-initrd",
                            vm->ker_initrd, NULL);
            if (rc != 0)
            {
                ERROR("Cannot compose kernel command line (line %u): %r",
                      __LINE__, rc);
                return TE_RC(TE_TA_UNIX, rc);
            }
        }

        if (vm->ker_dtb != NULL)
        {
            rc = te_string_append_shell_args_as_is(cmd, "-dtb",
                            vm->ker_dtb, NULL);
            if (rc != 0)
            {
                ERROR("Cannot compose kernel command line (line %u): %r",
                      __LINE__, rc);
                return TE_RC(TE_TA_UNIX, rc);
            }
        }
    }

    return 0;
}

static te_errno
vm_append_drive_cmd(te_string *cmd, vm_drive_list_t *drives)
{
    te_string drive_args = TE_STRING_INIT;
    struct vm_drive_entry *drive;
    te_errno rc = 0;

    SLIST_FOREACH(drive, drives, links)
    {
        rc = te_string_append(&drive_args, "file=%s,media=%s,snapshot=%s",
                              drive->file.ptr,
                              drive->cdrom ? "cdrom" : "disk",
                              drive->snapshot ? "on" : "off");
        if (rc != 0)
        {
            ERROR("Cannot compose VM drive command line (line %u)", __LINE__);
            goto exit;
        }

        rc = te_string_append_shell_args_as_is(cmd, "-drive",
                                               drive_args.ptr, NULL);
        if (rc != 0)
        {
            ERROR("Cannot compose VM drive command line (line %u)", __LINE__);
            goto exit;
        }

        te_string_reset(&drive_args);
    }

exit:
    te_string_free(&drive_args);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_virtfs_cmd(te_string *cmd, vm_virtfs_list_t *virtfses)
{
    te_string virtfs_args = TE_STRING_INIT;
    struct vm_virtfs_entry *virtfs;
    te_errno rc = 0;

    SLIST_FOREACH(virtfs, virtfses, links)
    {
        rc = te_string_append(&virtfs_args,
                              "%s,path=%s,security_model=%s,mount_tag=%s",
                              virtfs->fsdriver,
                              virtfs->path,
                              virtfs->security_model,
                              virtfs->mount_tag);
        if (rc != 0)
        {
            ERROR("Cannot compose VM drive command line (line %u)", __LINE__);
            goto exit;
        }

        rc = te_string_append_shell_args_as_is(cmd, "-virtfs",
                                               virtfs_args.ptr, NULL);
        if (rc != 0)
        {
            ERROR("Cannot compose VM drive command line (line %u)", __LINE__);
            goto exit;
        }

        te_string_reset(&virtfs_args);
    }

exit:
    te_string_free(&virtfs_args);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_pci_pt_cmd(te_string *cmd, vm_pci_pt_list_t *pt_list)
{
    te_string args = TE_STRING_INIT;
    struct vm_pci_pt_entry *pt = NULL;
    te_errno rc = 0;

    SLIST_FOREACH(pt, pt_list, links)
    {
        rc = te_string_append(&args, "vfio-pci,host=%s%s%s", pt->pci_addr.ptr,
                              pt->vf_token == NULL ? "" : ",vf_token=",
                              te_str_empty_if_null(pt->vf_token));
        if (rc != 0)
        {
            ERROR("Cannot compose PCI function pass-through command line (line %u): %r", __LINE__, rc);
            goto exit;
        }

        rc = te_string_append_shell_args_as_is(cmd, "-device",
                                               args.ptr, NULL);
        if (rc != 0)
        {
            ERROR("Cannot compose PCI function pass-through command line (line %u): %r", __LINE__, rc);
            goto exit;
        }

        te_string_reset(&args);
    }

exit:
    te_string_free(&args);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_devices_cmd(te_string *cmd, vm_device_list_t *dev_list)
{
    struct vm_device_entry *dev;
    te_errno rc = 0;

    SLIST_FOREACH(dev, dev_list, links)
    {
        rc = te_string_append_shell_args_as_is(cmd, "-device",
                                               dev->device.ptr, NULL);
        if (rc != 0)
        {
            ERROR("Cannot compose -device command line (line %u): %r",
                  __LINE__, rc);
            break;
        }
    }

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_cpu_cmd(te_string *cmd, struct vm_entry *vm)
{
    te_string num_arg = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_append_shell_args_as_is(cmd, "-cpu", vm->cpu.model.ptr, NULL);
    if (rc != 0)
    {
        ERROR("Cannot compose CPU command line (line %u): %r",
              __LINE__, rc);
        goto exit;
    }

    rc = te_string_append(&num_arg, "%u", vm->cpu.num);
    if (rc != 0)
    {
        ERROR("Cannot compose CPU command line (line %u): %r",
              __LINE__, rc);
        goto exit;
    }

    rc = te_string_append_shell_args_as_is(cmd, "-smp", num_arg.ptr, NULL);
    if (rc != 0)
    {
        ERROR("Cannot compose CPU command line (line %u): %r",
              __LINE__, rc);
        goto exit;
    }

exit:
    te_string_free(&num_arg);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_append_mem_file_cmd(te_string *cmd, struct vm_entry *vm)
{
    te_string mem_file = TE_STRING_INIT;
    te_errno rc;

    if (vm->mem_size == 0)
    {
        ERROR("Memory size must be set to use memory file");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Attribute 'share' is non configurable yet */
    rc = te_string_append(&mem_file,
                          "memory-backend-file,id=mem,size=%uM,"
                          "mem-path=%s,share=on",
                          vm->mem_size, vm->mem_path);
    if (rc != 0)
        ERROR("Failed to append memory file argument: %r", rc);

    rc = te_string_append_shell_args_as_is(cmd, "-object", mem_file.ptr, NULL);
    te_string_free(&mem_file);

    if (rc != 0)
        ERROR("Failed to append memory object argument: %r", rc);

    return rc;
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

    rc = te_string_append(&net_mgmt_dev_str,
             "%s,netdev=mgmt,romfile=,addr=0x3",
             vm->mgmt_net_device);
    if (rc != 0)
    {
        ERROR("Cannot compose management network device config");
        goto exit;
    }

    te_string_free(&vm->cmd);
    rc = te_string_append_shell_args_as_is(&vm->cmd, vm->qemu,
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
        rc = te_string_append_shell_args_as_is(&vm->cmd, "-enable-kvm", NULL);
        if (rc != 0)
        {
            ERROR("Failed to add -enable-kvm: %r", rc);
            goto exit;
        }
    }

    rc = te_string_append(&machine_str, "%s%s",
                          vm->machine, vm->kvm ? ",accel=kvm" : "");
    if (rc != 0)
    {
        ERROR("Failed to make VM machine string: %r", rc);
        goto exit;
    }
    rc = te_string_append_shell_args_as_is(&vm->cmd,
                                           "-machine", machine_str.ptr,
                                           NULL);
    if (rc != 0)
    {
        ERROR("Failed to add -machine option: %r", rc);
        goto exit;
    }

    rc = vm_append_cpu_cmd(&vm->cmd, vm);
    if (rc != 0)
        goto exit;

    if (vm->mem_size != 0)
    {
        te_string mem_size_str = TE_STRING_INIT;

        rc = te_string_append(&mem_size_str, "%uM", vm->mem_size);
        if (rc != 0)
        {
            ERROR("Cannot compose VM mem_size argument (line %u)", __LINE__);
            goto exit;
        }

        rc = te_string_append_shell_args_as_is(&vm->cmd,
                                               "-m", mem_size_str.ptr, NULL);
        te_string_free(&mem_size_str);
        if (rc != 0)
        {
            ERROR("Cannot compose VM mem_size command line (line %u)", __LINE__);
            goto exit;
        }
    }

    rc = te_string_append_shell_args_as_is(&vm->cmd,
             "-netdev", net_mgmt_str.ptr,
             "-device", net_mgmt_dev_str.ptr,
             NULL);
    if (rc != 0)
    {
        ERROR("Cannot compose VM start command line (line %u)", __LINE__);
        goto exit;
    }

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
        rc = te_string_append_shell_args_as_is(&vm->cmd, "-serial",
                                               vm->serial, NULL);

        if (rc != 0)
        {
            ERROR("Cannot compose VM start command line (line %u)", __LINE__);
            goto exit;
        }
    }

    if (vm->mem_path != NULL)
    {
        rc = vm_append_mem_file_cmd(&vm->cmd, vm);
        if (rc != 0)
            goto exit;

        rc = te_string_append_shell_args_as_is(&vm->cmd, "-numa",
                                               "node,memdev=mem", NULL);
        if (rc == 0 && vm->mem_prealloc)
        {
            rc = te_string_append_shell_args_as_is(&vm->cmd, "-mem-prealloc",
                                                   NULL);
        }
        if (rc != 0)
        {
            ERROR("Failed to append additional arguments for memory object");
            goto exit;
        }
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
        SLIST_REMOVE(&vm->virtfses, drive, vm_virtfs_entry, links);
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
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    ENTRY("%s", vm_name);

    if (vm_find(vm_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    vm = TE_ALLOC(sizeof(*vm));
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    SLIST_INIT(&vm->nets);
    SLIST_INIT(&vm->drives);
    SLIST_INIT(&vm->virtfses);
    SLIST_INIT(&vm->pci_pts);
    SLIST_INIT(&vm->devices);

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

    rc = te_string_append(&vm->cpu.model, "host");
    if (rc != 0)
    {
        vm_free(vm);
        return TE_RC(TE_TA_UNIX, rc);
    }

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
vm_del(unsigned int gid, const char *oid,
       const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

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
vm_qemu_get(unsigned int gid, const char *oid, char *value,
            const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->qemu);

    return 0;
}

static te_errno
vm_qemu_set(unsigned int gid, const char *oid, const char *value,
            const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

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

    return TE_RC(TE_TA_UNIX, rc);
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
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

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
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    val = !!atoi(value);

    if (!kvm_supported && val)
        WARN("KVM is not supported, but requested");

    vm->kvm = val;

    return 0;
}

static te_errno
vm_machine_get(unsigned int gid, const char *oid, char *value,
               const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->machine);

    return 0;
}

static te_errno
vm_machine_set(unsigned int gid, const char *oid, const char *value,
               const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

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
vm_mem_size_get(unsigned int gid, const char *oid, char *value,
                const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%u", vm->mem_size);

    return 0;
}

static te_errno
vm_mem_size_set(unsigned int gid, const char *oid, const char *value,
                const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (te_strtoui(value, 0, &vm->mem_size) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    return 0;
}

static te_errno
vm_mem_path_get(unsigned int gid, const char *oid, char *value,
                const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_strlcpy(value, te_str_empty_if_null(vm->mem_path), RCF_MAX_VAL);

    return 0;
}

static te_errno
vm_mem_path_set(unsigned int gid, const char *oid, const char *value,
                const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->mem_path, value);
}

static te_errno
vm_mem_prealloc_get(unsigned int gid, const char *oid, char *value,
                    const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%d", vm->mem_prealloc);

    return 0;
}

static te_errno
vm_mem_prealloc_set(unsigned int gid, const char *oid, const char *value,
                    const char *vm_name)
{
    struct vm_entry *vm;
    te_bool enable = !!atoi(value);

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    vm->mem_prealloc = enable;

    return 0;
}

static te_errno
vm_chardev_add(unsigned int gid, const char *oid, const char *value,
               const char *vm_name, const char *chardev_name)
{
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((vm = vm_find(vm_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_chardev_find(vm, chardev_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    chardev = TE_ALLOC(sizeof(*chardev));
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

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
vm_chardev_del(unsigned int gid, const char *oid,
           const char *vm_name, const char *chardev_name)
{
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    chardev = vm_chardev_find(vm, chardev_name);
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->chardevs, chardev, vm_chardev_entry, links);

    vm_chardev_free(chardev);

    return 0;
}

static te_errno
vm_chardev_list(unsigned int gid, const char *oid, const char *sub_id, char **list,
            const char *vm_name)
{
    te_string result = TE_STRING_INIT;
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(chardev, &vm->chardevs, links)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ", chardev->name);
        first = FALSE;
        if (rc != 0)
        {
            te_string_free(&result);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
vm_mgmt_net_device_get(unsigned int gid, const char *oid, char *value,
                       const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->mgmt_net_device);

    return 0;
}

static te_errno
vm_mgmt_net_device_set(unsigned int gid, const char *oid, const char *value,
                       const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

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
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

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
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

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
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
vm_chardev_server_get(unsigned int gid, const char *oid, char *value,
                      const char *vm_name, const char *chardev_name)
{
    struct vm_chardev_entry *chardev;

    UNUSED(gid);
    UNUSED(oid);

    chardev = vm_chardev_find(vm_find(vm_name), chardev_name);
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%d", chardev->server);

    return 0;
}

static te_errno
vm_chardev_server_set(unsigned int gid, const char *oid, const char *value,
                      const char *vm_name, const char *chardev_name)
{
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;
    te_bool is_server = !!atoi(value);

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    chardev = vm_chardev_find(vm, chardev_name);
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    chardev->server = is_server;

    return 0;
}

static te_errno
vm_chardev_path_get(unsigned int gid, const char *oid, char *value,
                    const char *vm_name, const char *chardev_name)
{
    struct vm_chardev_entry *chardev;

    UNUSED(gid);
    UNUSED(oid);

    chardev = vm_chardev_find(vm_find(vm_name), chardev_name);
    if (chardev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_strlcpy(value, te_str_empty_if_null(chardev->path), RCF_MAX_VAL);

    return 0;
}

static te_errno
vm_chardev_path_set(unsigned int gid, const char *oid, const char *value,
                    const char *vm_name, const char *chardev_name)
{
    struct vm_entry *vm;
    struct vm_chardev_entry *chardev;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    chardev = vm_chardev_find(vm, chardev_name);
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
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    net = vm_net_find(vm, net_name);
    if (net == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    property = vm_net_property_ptr_by_oid(net, oid);
    if (property == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(property, value);
}

static te_errno
vm_kernel_get(unsigned int gid, const char *oid, char *value,
              const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->kernel);

    return 0;
}

static te_errno
vm_kernel_set(unsigned int gid, const char *oid, const char *value,
              const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->kernel, value);
}

static te_errno
vm_ker_cmd_get(unsigned int gid, const char *oid, char *value,
               const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->ker_cmd);

    return 0;
}

static te_errno
vm_ker_cmd_set(unsigned int gid, const char *oid, const char *value,
               const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->ker_cmd, value);
}

static te_errno
vm_ker_initrd_get(unsigned int gid, const char *oid, char *value,
                  const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->ker_initrd);

    return 0;
}

static te_errno
vm_ker_initrd_set(unsigned int gid, const char *oid, const char *value,
                  const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->ker_initrd, value);
}

static te_errno
vm_ker_dtb_get(unsigned int gid, const char *oid, char *value,
               const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->ker_dtb);

    return 0;
}

static te_errno
vm_ker_dtb_set(unsigned int gid, const char *oid, const char *value,
              const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->ker_dtb, value);
}

static te_errno
vm_serial_get(unsigned int gid, const char *oid, char *value,
              const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->serial);

    return 0;
}

static te_errno
vm_serial_set(unsigned int gid, const char *oid, const char *value,
              const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", vm_name);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    return string_replace(&vm->serial, value);
}

static te_errno
vm_drive_add(unsigned int gid, const char *oid, const char *value,
             const char *vm_name, const char *drive_name)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((vm = vm_find(vm_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_drive_find(vm, drive_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    drive = TE_ALLOC(sizeof(*drive));
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    drive->name = strdup(drive_name);
    if (drive->name == NULL)
    {
        free(drive);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    SLIST_INSERT_HEAD(&vm->drives, drive, links);

    return 0;
}

static te_errno
vm_drive_del(unsigned int gid, const char *oid,
             const char *vm_name, const char *drive_name)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    drive = vm_drive_find(vm, drive_name);
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->drives, drive, vm_drive_entry, links);

    vm_drive_free(drive);

    return 0;
}

static te_errno
vm_drive_list(unsigned int gid, const char *oid, const char *sub_id, char **list,
              const char *vm_name)
{
    te_string result = TE_STRING_INIT;
    struct vm_entry *vm;
    struct vm_drive_entry *drive;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(drive, &vm->drives, links)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ", drive->name);
        first = FALSE;
        if (rc != 0)
        {
            te_string_free(&result);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
vm_file_get(unsigned int gid, const char *oid, char *value,
            const char *vm_name, const char *drive_name)
{
    struct vm_drive_entry *drive;

    UNUSED(gid);
    UNUSED(oid);

    drive = vm_drive_find(vm_find(vm_name), drive_name);
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", drive->file.ptr);

    return 0;
}

static te_errno
vm_file_set(unsigned int gid, const char *oid, char *value,
            const char *vm_name, const char *drive_name)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;
    int rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    drive = vm_drive_find(vm, drive_name);
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_free(&drive->file);
    rc = te_string_append(&drive->file, "%s",  value);

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_snapshot_get(unsigned int gid, const char *oid, char *value,
                const char *vm_name, const char *drive_name)
{
    struct vm_drive_entry *drive;

    UNUSED(gid);
    UNUSED(oid);

    drive = vm_drive_find(vm_find(vm_name), drive_name);
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%d", drive->snapshot);

    return 0;
}

static te_errno
vm_snapshot_set(unsigned int gid, const char *oid, char *value,
                const char *vm_name, const char *drive_name)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    drive = vm_drive_find(vm, drive_name);
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    drive->snapshot = (atoi(value) != 0);

    return 0;
}

static te_errno
vm_drive_cdrom_get(unsigned int gid, const char *oid, char *value,
                   const char *vm_name, const char *drive_name)
{
    struct vm_drive_entry *drive;

    UNUSED(gid);
    UNUSED(oid);

    drive = vm_drive_find(vm_find(vm_name), drive_name);
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%d", drive->cdrom);

    return 0;
}

static te_errno
vm_drive_cdrom_set(unsigned int gid, const char *oid, char *value,
                   const char *vm_name, const char *drive_name)
{
    struct vm_entry *vm;
    struct vm_drive_entry *drive;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    drive = vm_drive_find(vm, drive_name);
    if (drive == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    drive->cdrom = (atoi(value) != 0);

    return 0;
}

static te_errno
vm_virtfs_add(unsigned int gid, const char *oid, const char *value,
              const char *vm_name, const char *virtfs_name)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((vm = vm_find(vm_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    if (vm_virtfs_find(vm, virtfs_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    virtfs = TE_ALLOC(sizeof(*virtfs));
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

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
vm_virtfs_del(unsigned int gid, const char *oid,
              const char *vm_name, const char *virtfs_name)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->virtfses, virtfs, vm_virtfs_entry, links);

    vm_virtfs_free(virtfs);

    return 0;
}

static te_errno
vm_virtfs_list(unsigned int gid, const char *oid, const char *sub_id,
               char **list, const char *vm_name)
{
    te_string result = TE_STRING_INIT;
    struct vm_entry *vm;
    struct vm_virtfs_entry *vfs;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(vfs, &vm->virtfses, links)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ", vfs->name);
        first = FALSE;
        if (rc != 0)
        {
            te_string_free(&result);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
vm_virtfs_fsdriver_get(unsigned int gid, const char *oid, char *value,
                       const char *vm_name, const char *virtfs_name)
{
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    virtfs = vm_virtfs_find(vm_find(vm_name), virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", virtfs->fsdriver);

    return 0;
}

static te_errno
vm_virtfs_fsdriver_set(unsigned int gid, const char *oid, char *value,
                       const char *vm_name, const char *virtfs_name)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&virtfs->fsdriver, value);
}

static te_errno
vm_virtfs_mount_tag_get(unsigned int gid, const char *oid, char *value,
                        const char *vm_name, const char *virtfs_name)
{
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    virtfs = vm_virtfs_find(vm_find(vm_name), virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", virtfs->mount_tag);

    return 0;
}

static te_errno
vm_virtfs_mount_tag_set(unsigned int gid, const char *oid, char *value,
                        const char *vm_name, const char *virtfs_name)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&virtfs->mount_tag, value);
}

static te_errno
vm_virtfs_path_get(unsigned int gid, const char *oid, char *value,
                   const char *vm_name, const char *virtfs_name)
{
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    virtfs = vm_virtfs_find(vm_find(vm_name), virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", virtfs->path);

    return 0;
}

static te_errno
vm_virtfs_path_set(unsigned int gid, const char *oid, char *value,
                   const char *vm_name, const char *virtfs_name)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&virtfs->path, value);
}

static te_errno
vm_virtfs_security_model_get(unsigned int gid, const char *oid, char *value,
                             const char *vm_name, const char *virtfs_name)
{
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    virtfs = vm_virtfs_find(vm_find(vm_name), virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", virtfs->security_model);

    return 0;
}

static te_errno
vm_virtfs_security_model_set(unsigned int gid, const char *oid, char *value,
                             const char *vm_name, const char *virtfs_name)
{
    struct vm_entry *vm;
    struct vm_virtfs_entry *virtfs;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    virtfs = vm_virtfs_find(vm, virtfs_name);
    if (virtfs == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return string_replace(&virtfs->security_model, value);
}


static te_errno
vm_pci_pt_token_get(unsigned int gid, const char *oid, char *value,
                    const char *vm_name, const char *pci_pt_name)
{
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if ((vm = vm_find(vm_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(pt, &vm->pci_pts, links)
    {
        if (strcmp(pci_pt_name, pt->name) == 0)
            break;
    }
    if (pt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", te_str_empty_if_null(pt->vf_token));

    return 0;
}

static te_errno
vm_pci_pt_token_set(unsigned int gid, const char *oid, const char *value,
                    const char *vm_name, const char *pci_pt_name)
{
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if ((vm = vm_find(vm_name)) == NULL)
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
vm_pci_pt_add(unsigned int gid, const char *oid, char *value,
              const char *vm_name, const char *pci_pt_name)
{
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt = NULL;
    int rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (rcf_pch_rsrc_accessible("%s", value))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    pt = TE_ALLOC(sizeof(*pt));
    if (pt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    pt->name = strdup(pci_pt_name);
    if (pt->name == NULL)
    {
        free(pt);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if ((rc = te_string_append(&pt->pci_addr, "%s",  value)) != 0)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    SLIST_INSERT_HEAD(&vm->pci_pts, pt, links);

    return 0;
}

static te_errno
vm_pci_pt_del(unsigned int gid, const char *oid,
              const char *vm_name, const char *pci_pt_name)
{
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    pt = vm_pci_pt_find(vm, pci_pt_name);
    if (pt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->pci_pts, pt, vm_pci_pt_entry, links);

    vm_pci_pt_free(pt);

    return 0;
}

static te_errno
vm_pci_pt_list(unsigned int gid, const char *oid, const char *sub_id,
               char **list, const char *vm_name)
{
    te_string result = TE_STRING_INIT;
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(pt, &vm->pci_pts, links)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ", pt->name);
        first = FALSE;
        if (rc != 0)
        {
            te_string_free(&result);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
vm_pci_pt_get(unsigned int gid, const char *oid, char *value,
              const char *vm_name, const char *pci_pt_name)
{
    struct vm_entry *vm;
    struct vm_pci_pt_entry *pt = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if ((vm = vm_find(vm_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(pt, &vm->pci_pts, links)
    {
        if (strcmp(pci_pt_name, pt->name) == 0)
            break;
    }

    if (pt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", pt->pci_addr.ptr);

    return 0;
}



static te_errno
vm_device_get(unsigned int gid, const char *oid, char *value,
              const char *vm_name, const char *device_name)
{
    struct vm_entry *vm;
    struct vm_device_entry *dev;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    dev = vm_device_find(vm, device_name);
    if (dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", dev->device.ptr);

    return 0;
}

static te_errno
vm_device_add(unsigned int gid, const char *oid, const char *value,
              const char *vm_name, const char *device_name)
{
    struct vm_entry *vm;
    struct vm_device_entry *dev;
    int rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (rcf_pch_rsrc_accessible("%s", value))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    dev = TE_ALLOC(sizeof(*dev));
    if (dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    dev->name = strdup(device_name);
    if (dev->name == NULL)
    {
        vm_device_free(dev);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if ((rc = te_string_append(&dev->device, "%s",  value)) != 0)
    {
        vm_device_free(dev);
        return TE_RC(TE_TA_UNIX, rc);
    }

    SLIST_INSERT_HEAD(&vm->devices, dev, links);

    return 0;
}

static te_errno
vm_device_del(unsigned int gid, const char *oid,
              const char *vm_name, const char *device_name)
{
    struct vm_entry *vm;
    struct vm_device_entry *dev;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    dev = vm_device_find(vm, device_name);
    if (dev == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&vm->devices, dev, vm_device_entry, links);

    vm_device_free(dev);

    return 0;
}

static te_errno
vm_device_list(unsigned int gid, const char *oid, const char *sub_id,
               char **list, const char *vm_name)
{
    te_string result = TE_STRING_INIT;
    struct vm_entry *vm;
    struct vm_device_entry *dev;
    te_bool first = TRUE;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(dev, &vm->devices, links)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ", dev->name);
        first = FALSE;
        if (rc != 0)
        {
            te_string_free(&result);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = result.ptr;
    return 0;
}

static te_errno
vm_cpu_model_get(unsigned int gid, const char *oid, char *value,
               const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%s", vm->cpu.model.ptr);

    return 0;
}

static te_errno
vm_cpu_model_set(unsigned int gid, const char *oid, const char *value,
                const char *vm_name)
{
    struct vm_entry *vm;
    te_errno rc;
    te_string save = TE_STRING_INIT;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    rc = te_string_append(&save, "%s", vm->cpu.model.ptr);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    te_string_free(&vm->cpu.model);
    rc = te_string_append(&vm->cpu.model, "%s",  value);
    if (rc == 0)
        te_string_free(&save);
    else
        vm->cpu.model = save;

    return TE_RC(TE_TA_UNIX, rc);
}

static te_errno
vm_cpu_num_get(unsigned int gid, const char *oid, char *value,
               const char *vm_name)
{
    struct vm_entry *vm;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%u", vm->cpu.num);

    return 0;
}

static te_errno
vm_cpu_num_set(unsigned int gid, const char *oid, const char *value,
               const char *vm_name)
{
    struct vm_entry *vm;
    unsigned int save;

    UNUSED(gid);
    UNUSED(oid);

    vm = vm_find(vm_name);
    if (vm == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (vm_is_running(vm))
        return TE_RC(TE_TA_UNIX, TE_EBUSY);

    save = vm->cpu.num;
    if (te_strtoui(value, 0, &vm->cpu.num) != 0)
    {
        vm->cpu.num = save;
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

RCF_PCH_CFG_NODE_RW(node_vm_serial, "serial", NULL, NULL,
                    vm_serial_get, vm_serial_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_vm_device, "device", NULL, &node_vm_serial,
                               vm_device_get, NULL, vm_device_add,
                               vm_device_del, vm_device_list, NULL);

RCF_PCH_CFG_NODE_RW(node_vm_pci_pt_token, "vf_token", NULL, NULL,
                    vm_pci_pt_token_get, vm_pci_pt_token_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_vm_pci_pt, "pci_pt",
                               &node_vm_pci_pt_token, &node_vm_device,
                               vm_pci_pt_get, NULL, vm_pci_pt_add,
                               vm_pci_pt_del, vm_pci_pt_list, NULL);

RCF_PCH_CFG_NODE_RW(node_vm_cpu_num, "num", NULL, NULL,
                    vm_cpu_num_get, vm_cpu_num_set);

RCF_PCH_CFG_NODE_RW(node_vm_cpu_model, "model", NULL, &node_vm_cpu_num,
                    vm_cpu_model_get, vm_cpu_model_set);

RCF_PCH_CFG_NODE_NA(node_vm_cpu, "cpu", &node_vm_cpu_model, &node_vm_pci_pt);

RCF_PCH_CFG_NODE_RW(node_vm_virtfs_fsdriver, "fsdriver", NULL, NULL,
                    vm_virtfs_fsdriver_get, vm_virtfs_fsdriver_set);

RCF_PCH_CFG_NODE_RW(node_vm_virtfs_path, "path", NULL, &node_vm_virtfs_fsdriver,
                    vm_virtfs_path_get, vm_virtfs_path_set);

RCF_PCH_CFG_NODE_RW(node_vm_virtfs_mount_tag, "mount_tag",
                    NULL, &node_vm_virtfs_path,
                    vm_virtfs_mount_tag_get, vm_virtfs_mount_tag_set);

RCF_PCH_CFG_NODE_RW(node_vm_virtfs_security_model, "security_model",
                    NULL, &node_vm_virtfs_mount_tag,
                    vm_virtfs_security_model_get, vm_virtfs_security_model_set);

RCF_PCH_CFG_NODE_COLLECTION(node_vm_virtfs, "virtfs",
                            &node_vm_virtfs_security_model,
                            &node_vm_cpu, vm_virtfs_add, vm_virtfs_del,
                            vm_virtfs_list, NULL);

RCF_PCH_CFG_NODE_RW(node_vm_drive_cdrom, "cdrom", NULL, NULL,
                    vm_drive_cdrom_get, vm_drive_cdrom_set);

RCF_PCH_CFG_NODE_RW(node_vm_snapshot, "snapshot", NULL, &node_vm_drive_cdrom,
                    vm_snapshot_get, vm_snapshot_set);

RCF_PCH_CFG_NODE_RW(node_vm_file, "file", NULL, &node_vm_snapshot,
                    vm_file_get, vm_file_set);

RCF_PCH_CFG_NODE_COLLECTION(node_vm_drive, "drive", &node_vm_file,
                            &node_vm_virtfs, vm_drive_add, vm_drive_del,
                            vm_drive_list, NULL);

RCF_PCH_CFG_NODE_RW(node_vm_kernel_cmdline, "cmdline", NULL, NULL,
                    vm_ker_cmd_get, vm_ker_cmd_set);

RCF_PCH_CFG_NODE_RW(node_vm_kernel_initrd, "initrd", NULL,
                    &node_vm_kernel_cmdline, vm_ker_initrd_get,
                    vm_ker_initrd_set);

RCF_PCH_CFG_NODE_RW(node_vm_kernel_dtb, "dtb", NULL, &node_vm_kernel_initrd,
                    vm_ker_dtb_get, vm_ker_dtb_set);

RCF_PCH_CFG_NODE_RW(node_vm_kernel, "kernel", &node_vm_kernel_dtb,
                    &node_vm_drive, vm_kernel_get, vm_kernel_set);

RCF_PCH_CFG_NODE_RW(node_vm_net_mac_addr, "mac_addr", NULL, NULL,
                    vm_net_property_get, vm_net_property_set);

RCF_PCH_CFG_NODE_RW(node_vm_net_type_spec, "type_spec", NULL,
                    &node_vm_net_mac_addr, vm_net_property_get,
                    vm_net_property_set);

RCF_PCH_CFG_NODE_RW(node_vm_net_type, "type", NULL, &node_vm_net_type_spec,
                    vm_net_property_get, vm_net_property_set);

RCF_PCH_CFG_NODE_COLLECTION(node_vm_net, "net", &node_vm_net_type,
                            &node_vm_kernel, vm_net_add, vm_net_del,
                            vm_net_list, NULL);

RCF_PCH_CFG_NODE_RW(node_vm_mgmt_net_device, "mgmt_net_device", NULL,
                    &node_vm_net,
                    vm_mgmt_net_device_get, vm_mgmt_net_device_set);

RCF_PCH_CFG_NODE_RW(node_vm_chardev_server, "server", NULL, NULL,
                    vm_chardev_server_get, vm_chardev_server_set);

RCF_PCH_CFG_NODE_RW(node_vm_chardev_path, "path", NULL, &node_vm_chardev_server,
                    vm_chardev_path_get, vm_chardev_path_set);

RCF_PCH_CFG_NODE_COLLECTION(node_vm_chardev, "chardev", &node_vm_chardev_path,
                            &node_vm_mgmt_net_device,
                            vm_chardev_add, vm_chardev_del,
                            vm_chardev_list, NULL);

RCF_PCH_CFG_NODE_RW(node_vm_mem_prealloc, "prealloc", NULL, NULL,
                    vm_mem_prealloc_get, vm_mem_prealloc_set);

RCF_PCH_CFG_NODE_RW(node_vm_mem_path, "path", NULL, &node_vm_mem_prealloc,
                    vm_mem_path_get, vm_mem_path_set);

RCF_PCH_CFG_NODE_RW(node_vm_mem_size, "size", NULL, &node_vm_mem_path,
                    vm_mem_size_get, vm_mem_size_set);

RCF_PCH_CFG_NODE_NA(node_vm_mem, "mem", &node_vm_mem_size, &node_vm_chardev);

RCF_PCH_CFG_NODE_RW(node_vm_machine, "machine", NULL, &node_vm_mem,
                    vm_machine_get, vm_machine_set);

RCF_PCH_CFG_NODE_RW(node_vm_kvm, "kvm", NULL, &node_vm_machine,
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

RCF_PCH_CFG_NODE_RW(node_vm_qemu, "qemu", NULL, &node_vm_status,
                    vm_qemu_get, vm_qemu_set);

RCF_PCH_CFG_NODE_COLLECTION(node_vm, "vm", &node_vm_qemu, NULL,
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
