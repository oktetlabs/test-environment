/** @file
 * @brief CPU support
 *
 * CPU configuration tree support
 *
 * Copyright (C) 2018-2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#define TE_LGR_USER     "Conf CPU"

#include "te_config.h"
#include "config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "ctype.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_string.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "conf_common.h"
#include "te_str.h"
#include "te_alloc.h"

typedef enum cpu_item_type {
    CPU_ITEM_NODE = 0,
    CPU_ITEM_PACKAGE,
    CPU_ITEM_CORE,
    CPU_ITEM_THREAD,
} cpu_item_type;

typedef LIST_HEAD(cpu_item_list, cpu_item) cpu_item_list;

typedef union cpu_properties {
    struct {
        te_bool isolated;
    } thread;
} cpu_properties;

typedef struct cpu_item {
    LIST_ENTRY(cpu_item) next;

    cpu_item_type type;
    unsigned int id;
    cpu_properties prop;

    cpu_item_list children;
} cpu_item;

static cpu_item_list global_cpu_item_root;

#define SYSFS_SYSTEM_TREE "/sys/devices/system"

static cpu_item *
find_cpu_item(cpu_item_list *root, cpu_item_type type, const unsigned int *ids)
{
    cpu_item *found_item;
    cpu_item *item;
    cpu_item_type type_i;

    if (root == NULL || LIST_EMPTY(root) || ids == NULL)
        return NULL;

    for (type_i = CPU_ITEM_NODE; type_i <= CPU_ITEM_THREAD; type_i++)
    {
        found_item = NULL;

        LIST_FOREACH(item, root, next)
        {
            if (item->id == ids[type_i])
                found_item = item;
        }

        if (found_item == NULL)
            return NULL;

        if (type_i == type)
            return found_item;
        else
            root = &found_item->children;
    }

    return NULL;
}

static te_errno
init_item(cpu_item_type type, unsigned int id, cpu_properties prop,
          cpu_item **item)
{
    cpu_item *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        ERROR("Failed to allocate a CPU item");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    result->type = type;
    result->id = id;
    result->prop = prop;
    LIST_INIT(&result->children);

    *item = result;

    return 0;
}

/*
 * Add a CPU item to a tree, creating its parent items if they are not present
 */
static te_errno
add_cpu_item(cpu_item_list *root, cpu_item_type type, unsigned int *ids,
             cpu_properties *props)
{
    cpu_item *item = NULL;
    cpu_item_type type_i;
    cpu_item_list *list = root;
    te_errno rc;

    for (type_i = CPU_ITEM_NODE; type_i <= CPU_ITEM_THREAD; type_i++)
    {
        item = find_cpu_item(root, type_i, ids);

        if (item != NULL && type_i == type)
        {
            ERROR("Failed to add a CPU item - already exists");
            return TE_RC(TE_TA_UNIX, TE_EEXIST);
        }

        if (item == NULL)
        {
            rc = init_item(type_i, ids[type_i], props[type_i], &item);

            if (rc != 0)
            {
                ERROR("Failed to initialize a CPU item");
                return rc;
            }

            LIST_INSERT_HEAD(list, item, next);
        }

        list = &item->children;
    }

    return 0;
}

static te_errno
get_index(const char *num, unsigned long *index)
{
    return te_strtoul(num, 10, index);
}

static te_errno
open_system_file(const char *path, FILE **result)
{
    te_errno rc = 0;
    te_string buf = TE_STRING_INIT;

    if ((rc = te_string_append(&buf, SYSFS_SYSTEM_TREE "/%s", path)) != 0)
    {
        ERROR("Failed to create sysfs path '%s'", path);
        te_string_free(&buf);
        return rc;
    }

    *result = fopen(buf.ptr, "r");
    if (*result == NULL)
    {
        if (errno == ENOENT)
        {
            INFO("Sysfs file '%s' does not exist", path);
        }
        else
        {
            ERROR("Failed to open sysfs file '%s', error: %s",
                  path, strerror(errno));
        }
        rc = TE_OS_RC(TE_TA_UNIX, errno);
    }
    te_string_free(&buf);
    return rc;
}

static te_errno
read_cpu_topology_dec_attr(const char *name, const char *attr,
                           unsigned int *result)
{
    FILE *f;
    te_errno rc = 0;

    te_string buf = TE_STRING_INIT;

    if ((rc = te_string_append(&buf, "cpu/%s/topology/%s", name, attr)) != 0)
    {
        ERROR("Failed to create cpu topology path");
        te_string_free(&buf);
        return rc;
    }

    rc = open_system_file(buf.ptr, &f);
    te_string_free(&buf);
    if (rc != 0)
        return rc;

    if (fscanf(f, "%u", result) == EOF)
    {
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        ERROR("Failed to read '%s' attribute for CPU '%s'", attr, name);
    }

    fclose(f);
    return rc;
}

static te_errno
read_isolated(char **isolated)
{
    FILE *f = NULL;
    char *result = NULL;
    te_errno rc = 0;

    result = TE_ALLOC(RCF_MAX_VAL);
    if (result == NULL)
    {
        ERROR("Out of memory");
        rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto out;
    }

    rc = open_system_file("cpu/isolated", &f);
    switch (TE_RC_GET_ERROR(rc))
    {
        case 0:
            break;
        case TE_ENOENT:
            INFO("Could not open sysfs CPUs isolated file, fallback to empty");
            *isolated = result;
            rc = 0;
            goto out;
        default:
            goto out;
    }

    if (fgets(result, RCF_MAX_VAL, f) == NULL)
    {
        ERROR("Failed to read sysfs CPU isolated file");
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto out;
    }

    /* Replace newline in the end of file with null-terminator */
    result[strlen(result) - 1] = '\0';

    *isolated = result;

out:
    if (f != NULL)
        fclose(f);
    if (rc != 0)
        free(result);

    return rc;
}

static te_errno
get_suffix_index(const char *name, unsigned long *index)
{
    size_t i;
    size_t len = strlen(name);
    size_t begin = len;
    te_errno rc;

    for (i = len - 1; i + 1 != 0; i--)
    {
        if (!isdigit(name[i]))
            break;

        begin = i;
    }

    if (begin == len)
    {
        ERROR("Failed to get index from name '%s'", name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rc = get_index(name + begin, index)) != 0)
    {
        ERROR("Failed to get suffix index from name '%s'", name);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

static int
filter_prefix_with_index(const struct dirent *de, const char *prefix)
{
    size_t i;
    size_t entry_len = strlen(de->d_name);
    size_t prefix_len = strlen(prefix);

    if (entry_len <= prefix_len)
        return 0;

    if (strncmp(de->d_name, prefix, prefix_len) != 0)
        return 0;

    for (i = prefix_len; i < entry_len; i++)
    {
        if (!isdigit(de->d_name[i]))
            return 0;
    }

    return 1;
}

static int
filter_node(const struct dirent *de)
{
    return filter_prefix_with_index(de, "node");
}

static int
filter_cpu(const struct dirent *de)
{
    return filter_prefix_with_index(de, "cpu");
}

static te_errno
get_node(const char *cpu_name, unsigned long *node_id)
{
    te_string buf = TE_STRING_INIT;
    struct dirent **names = NULL;
    te_errno rc = 0;
    int n_nodes = 0;

    if ((rc = te_string_append(&buf, SYSFS_SYSTEM_TREE "/cpu/%s", cpu_name)) != 0)
        goto out;

    n_nodes = scandir(buf.ptr, &names, filter_node, NULL);
    if (n_nodes < 0)
    {
        ERROR("Could not get CPU NUMA node, rc=%d", n_nodes);
        rc = TE_OS_RC(TE_TA_UNIX, n_nodes);
        goto out;
    }

    switch (n_nodes)
    {
        case 0:
            INFO("Could not find CPU NUMA node for '%s', fallback to node 0",
                 cpu_name);
            *node_id = 0;
            break;
        case 1:
            if ((rc = get_suffix_index(names[0]->d_name, node_id)) != 0)
                ERROR("Could not get CPU NUMA index");
            break;
        default:
            ERROR("More than 1 NUMA node for a CPU");
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
            break;
    }

out:
    if (names != NULL)
    {
        int i;

        for (i = 0; i < n_nodes; i++)
            free(names[i]);
    }
    free(names);
    te_string_free(&buf);

    return rc;
}

static te_errno
is_thread_isolated(const char *isolated_str, unsigned long thread_id,
                   te_bool *isolated)
{
    unsigned long first;
    unsigned long last;
    const char *str = isolated_str;
    char *endptr = (char *)str;

    while (*str != '\0')
    {
        errno = 0;
        first = strtoul(str, &endptr, 10);
        if (errno != 0)
        {
            ERROR("Failed to parse sysfs CPU isolated");
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        if (*endptr == '-')
        {
            str = endptr + 1;
            errno = 0;
            last = strtoul(str, &endptr, 10);
            if (errno != 0)
            {
                ERROR("Failed to parse sysfs CPU isolated");
                return TE_OS_RC(TE_TA_UNIX, errno);
            }
        }
        else
        {
            last = first;
        }

        if (first <= thread_id && thread_id <= last)
        {
            *isolated = TRUE;
            return 0;
        }

        if (*endptr == ',')
            str = endptr + 1;
        else
            str = endptr;
    }

    *isolated = FALSE;

    return 0;
}

static te_errno
populate_cpu(cpu_item_list *root, const char *name, const char *isolated_str)
{
    unsigned long thread_id;
    unsigned long node_id;
    unsigned int core_id;
    unsigned int package_id;
    te_bool isolated = FALSE;
    te_errno rc = 0;

    if ((rc = get_suffix_index(name, &thread_id)) != 0)
        return rc;

    if ((rc = get_node(name, &node_id)) != 0)
        return rc;

    if ((rc = read_cpu_topology_dec_attr(name, "core_id", &core_id)) != 0)
        return rc;

    if ((rc = read_cpu_topology_dec_attr(name, "physical_package_id",
                                         &package_id)) != 0)
    {
        return rc;
    }

    if ((rc = is_thread_isolated(isolated_str, thread_id, &isolated)) != 0)
        return rc;

    {
        unsigned int ids[] = {node_id, package_id, core_id, thread_id};
        cpu_properties props[] = {{{0}}, {{0}}, {{0}}, {{isolated}}};

        rc = add_cpu_item(root, CPU_ITEM_THREAD, ids, props);
    }

    return rc;
}

static void
free_list(cpu_item_list *root)
{
    if (root == NULL)
        return;

    while (!LIST_EMPTY(root))
    {
        cpu_item *item = LIST_FIRST(root);

        free_list(&item->children);
        LIST_REMOVE(item, next);
        free(item);
    }
}

static te_errno
scan_system(cpu_item_list *root)
{
    struct dirent **names = NULL;
    char *isolated_str = NULL;
    cpu_item_list result = LIST_HEAD_INITIALIZER(&result);
    int n_cpus = 0;
    te_errno rc = 0;
    int i;

    if ((rc = read_isolated(&isolated_str)) != 0)
        goto out;

    n_cpus = scandir(SYSFS_SYSTEM_TREE "/cpu", &names, filter_cpu, NULL);
    if (n_cpus <= 0)
    {
        ERROR("Could not get a list of CPUs, rc=%d", n_cpus);
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto out;
    }

    for (i = 0; i < n_cpus; i++)
    {
        if ((rc = populate_cpu(&result, names[i]->d_name, isolated_str)) != 0)
        {
            ERROR("Only %d CPUs populated, could not get info about '%s'",
                 i, names[i]->d_name);
            goto out;
        }
    }

    memcpy(root, &result, sizeof(cpu_item_list));

out:
    if (names != NULL)
    {
        for (i = 0; i < n_cpus; i++)
            free(names[i]);
    }
    free(names);
    free(isolated_str);
    if (rc != 0)
        free_list(&result);

    return rc;
}

static te_errno
update_cpu_info()
{
    cpu_item_list root = LIST_HEAD_INITIALIZER(&root);
    te_errno rc;

    if ((rc = scan_system(&root)) != 0)
    {
        ERROR("Failed to get CPU information");
        return rc;
    }

    if (LIST_EMPTY(&root))
    {
        ERROR("No information is found about CPU");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    free_list(&global_cpu_item_root);
    global_cpu_item_root = root;

    return 0;
}

static te_errno
cpu_thread_isolated_get(unsigned int gid, const char *oid, char *value,
                        const char *unused1, const char *node_str,
                        const char *package_str, const char *core_str,
                        const char *thread_str)
{
    unsigned long node_id;
    unsigned long package_id;
    unsigned long core_id;
    unsigned long thread_id;
    cpu_item *thread = NULL;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

    if ((rc = get_index(node_str, &node_id)) != 0)
        return rc;
    if ((rc = get_index(package_str, &package_id)) != 0)
        return rc;
    if ((rc = get_index(core_str, &core_id)) != 0)
        return rc;
    if ((rc = get_index(thread_str, &thread_id)) != 0)
        return rc;

    {
        unsigned int ids[] = {node_id, package_id, core_id, thread_id};

        thread = find_cpu_item(&global_cpu_item_root, CPU_ITEM_THREAD, ids);
    }

    if (thread == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%u", thread->prop.thread.isolated);

    return 0;
}

static te_errno
numa_list(unsigned int gid, const char *oid, const char *sub_id,
               char **list)
{
    te_string result = TE_STRING_INIT;
    te_bool first = TRUE;
    cpu_item *node;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    LIST_FOREACH(node, &global_cpu_item_root, next)
    {
        rc = te_string_append(&result, "%s%u", first ? "" : " ", node->id);
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

static te_errno
cpu_list(unsigned int gid, const char *oid, const char *sub_id,
         char **list, const char *unused1, const char *node_str)
{
    te_string result = TE_STRING_INIT;
    unsigned long node_id;
    te_bool first = TRUE;
    cpu_item *node = NULL;
    cpu_item *package;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);

    if ((rc = get_index(node_str, &node_id)) != 0)
        return rc;

    {
        unsigned int ids[] = {node_id};

        node = find_cpu_item(&global_cpu_item_root, CPU_ITEM_NODE, ids);
    }
    if (node == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(package, &node->children, next)
    {
        rc = te_string_append(&result, "%s%u", first ? "" : " ", package->id);
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

static te_errno
cpu_core_list(unsigned int gid, const char *oid, const char *sub_id,
              char **list, const char *unused1, const char *node_str,
              const char *package_str)
{
    te_string result = TE_STRING_INIT;
    unsigned long node_id;
    unsigned long package_id;
    te_bool first = TRUE;
    cpu_item *package = NULL;
    cpu_item *core;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);

    if ((rc = get_index(node_str, &node_id)) != 0)
        return rc;
    if ((rc = get_index(package_str, &package_id)) != 0)
        return rc;

    {
        unsigned int ids[] = {node_id, package_id};
        package = find_cpu_item(&global_cpu_item_root, CPU_ITEM_PACKAGE, ids);
    }
    if (package == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(core, &package->children, next)
    {
        rc = te_string_append(&result, "%s%u", first ? "" : " ", core->id);
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

static te_errno
cpu_thread_list(unsigned int gid, const char *oid, const char *sub_id,
                char **list, const char *unused1, const char *node_str,
                const char *package_str, const char *core_str)
{
    te_string result = TE_STRING_INIT;
    unsigned long node_id;
    unsigned long package_id;
    unsigned long core_id;
    te_bool first = TRUE;
    cpu_item *core = NULL;
    cpu_item *thread;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);

    if ((rc = get_index(node_str, &node_id)) != 0)
        return rc;
    if ((rc = get_index(package_str, &package_id)) != 0)
        return rc;
    if ((rc = get_index(core_str, &core_id)) != 0)
        return rc;

    {
        unsigned int ids[] = {node_id, package_id, core_id};
        core = find_cpu_item(&global_cpu_item_root, CPU_ITEM_CORE, ids);
    }
    if (core == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(thread, &core->children, next)
    {
        rc = te_string_append(&result, "%s%u", first ? "" : " ", thread->id);
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

static te_errno
cpu_thread_grab(const char *name)
{
    cfg_oid *oid = cfg_convert_oid_str(name);
    unsigned long node_id;
    unsigned long package_id;
    unsigned long core_id;
    unsigned long thread_id;
    cpu_item *thread = NULL;
    te_errno rc = 0;

    if (oid == NULL || !oid->inst || oid->len != 7)
    {
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto out;
    }

    if ((rc = get_index(CFG_OID_GET_INST_NAME(oid, 3), &node_id)) != 0)
        goto out;
    if ((rc = get_index(CFG_OID_GET_INST_NAME(oid, 4), &package_id)) != 0)
        goto out;
    if ((rc = get_index(CFG_OID_GET_INST_NAME(oid, 5), &core_id)) != 0)
        goto out;
    if ((rc = get_index(CFG_OID_GET_INST_NAME(oid, 6), &thread_id)) != 0)
        goto out;

    {
        unsigned int ids[] = {node_id, package_id, core_id, thread_id};
        thread = find_cpu_item(&global_cpu_item_root, CPU_ITEM_THREAD, ids);
    }

    if (thread == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
        goto out;
    }

out:
    cfg_free_oid(oid);

    return rc;
}

RCF_PCH_CFG_NODE_RO(node_thread_isolated, "isolated",
                    NULL, NULL, cpu_thread_isolated_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_cpu_thread, "thread",
                               &node_thread_isolated, NULL,
                               NULL, cpu_thread_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_cpu_core, "core",
                               &node_cpu_thread, NULL,
                               NULL, cpu_core_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_cpu, "cpu",
                               &node_cpu_core, NULL,
                               NULL, cpu_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_numa_node, "node",
                               &node_cpu, NULL,
                               NULL, numa_list);

te_errno
ta_unix_conf_cpu_init()
{
    te_errno rc = update_cpu_info();
    if (rc != 0)
        return rc;

    rc = rcf_pch_add_node("/agent/hardware", &node_numa_node);

    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/hardware/node/cpu/core/thread",
                             cpu_thread_grab,
                             rcf_pch_rsrc_release_dummy);
}
