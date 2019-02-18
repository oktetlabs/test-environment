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
#include <netdb.h>

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "rcf_pch.h"
#include "conf_vm.h"

#if __linux__

struct vm_entry {
    SLIST_ENTRY(vm_entry)   links;
    char                   *name;
    uint16_t                host_ssh_port;
    uint16_t                guest_ssh_port;
    uint16_t                rcf_port;
};


static SLIST_HEAD(, vm_entry) vms = SLIST_HEAD_INITIALIZER(vms);

static uint16_t guest_ssh_port; /* SSH port in host byte order */

static uint16_t
vm_alloc_tcp_port(void)
{
    static uint16_t tcp_port_next = 9320;

    return tcp_port_next++;
}

static te_bool
vm_is_running(struct vm_entry *vm)
{
    UNUSED(vm);
    return FALSE;
}

static te_errno
vm_start(struct vm_entry *vm)
{
    UNUSED(vm);
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}

static te_errno
vm_stop(struct vm_entry *vm)
{
    UNUSED(vm);
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}


/*
 * Configuration model implementation
 */

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
        len = strlen(vm->name) + 1;

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

    vm->host_ssh_port = vm_alloc_tcp_port();
    vm->guest_ssh_port = guest_ssh_port;
    vm->rcf_port = vm_alloc_tcp_port();

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


RCF_PCH_CFG_NODE_RW(node_vm_rcf_port, "rcf_port", NULL, NULL,
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
