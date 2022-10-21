/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief CPU support
 *
 * CPU configuration tree support
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
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

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SCHED_H
#include <sched.h>
/*
 * cpu_set_t is used to save the ids of CPUs connected to cpu_items.
 * Information about caches available to corresponding cpu_items is
 * added using these CPUs ids.
 */
#ifdef CPU_SETSIZE
#define SUPPORT_CACHES 1
#endif
#endif

#ifndef SUPPORT_CACHES
#define SUPPORT_CACHES 0
#endif

#include "ctype.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_queue.h"
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

#include "te_intset.h"

#if SUPPORT_CACHES
#include "te_file.h"
#include "te_numeric.h"
#include "te_units.h"

typedef LIST_HEAD(cache_item_list, cache_item) cache_item_list;

/*
 * id field contains id for a cache_item in a cache_list.
 * sys_id field contains id for cache in sysfs.
 */
typedef struct cache_item {
    LIST_ENTRY(cache_item) next;

    unsigned int id;
    cpu_set_t shared_cpu_set;
    uintmax_t sys_id;
    char *type;
    uintmax_t level;
    uintmax_t linesize;
    uintmax_t size;
} cache_item;

typedef enum cache_item_field {
    CACHE_ITEM_LEVEL = 0,
    CACHE_ITEM_LINESIZE,
    CACHE_ITEM_SIZE,
} cache_item_field;
#endif

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

#if SUPPORT_CACHES
    cpu_set_t vcpus;
#endif
    cpu_item_type type;
    unsigned int id;
    cpu_properties prop;

#if SUPPORT_CACHES
    cache_item_list cache_list;
#endif

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

#if SUPPORT_CACHES
/*
 * By construction, a set of CPU ids of a CPU thread contains only this thread
 * and a set of CPU ids of higher level CPU sets is a union of CPU id sets of
 * their children.
 *
 * Normally, we search for a CPU item that is uniquely identified by a shared
 * cpu set. However, there may be cases when a provided CPU id set falls
 * between the layers, in which case we're going to associate it with the parent
 * cpu set.
 *
 * Therefore:
 * - if a CPU id set of the current item is equal to the requested id set -
 *   the item is found;
 * - if it's a superset of the requested id set, then we should look in
 *   descendants;
 * - if it's a subset of the requested id set, we shall stick with the parent.
 */
static cpu_item *
find_item_by_cpu_set(cpu_item_list *root, cpu_set_t *shared_cpu_set)
{
    cpu_item *container_item = NULL;
    cpu_item *item;

    while (root != NULL)
    {
        LIST_FOREACH(item, root, next)
        {
            if (CPU_EQUAL(shared_cpu_set, &item->vcpus))
                return item;

            if (te_cpuset_is_subset(shared_cpu_set, &item->vcpus) != 0)
            {
                container_item = item;
                root = &container_item->children;
                break;
            }

            if (te_cpuset_is_subset(&item->vcpus, shared_cpu_set) != 0)
                return container_item;

            root = NULL;
        }
    }

    return NULL;
}
#endif

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

#if SUPPORT_CACHES
    CPU_ZERO(&result->vcpus);
#endif
    result->type = type;
    result->id = id;
    result->prop = prop;
#if SUPPORT_CACHES
    LIST_INIT(&result->cache_list);
#endif
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

#if SUPPORT_CACHES
        if (type == CPU_ITEM_THREAD)
            CPU_SET(ids[type], &item->vcpus);
#endif

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
        int error = errno;

        ERROR("Could not get CPU NUMA node, rc=%d", error);
        rc = TE_OS_RC(TE_TA_UNIX, error);
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
is_cpu_online(const char *name, te_bool *is_online)
{
    FILE *f = NULL;
    te_errno rc = 0;
    unsigned int result;
    unsigned long thread_id;

    te_string buf = TE_STRING_INIT;

    if ((rc = get_suffix_index(name, &thread_id)) != 0)
        return rc;

    /* cpu0 is always online */
    if (thread_id == 0)
    {
        *is_online = TRUE;
        return rc;
    }

    if ((rc = te_string_append(&buf, "cpu/%s/online", name)) != 0)
    {
        ERROR("Failed to create online cpu path");
        goto out;
    }

    rc = open_system_file(buf.ptr, &f);
    switch (TE_RC_GET_ERROR(rc))
    {
        case 0:
            break;
        case TE_ENOENT:
            INFO("Could not open sysfs CPUs online file, fallback to empty");
            *is_online = TRUE;
            rc = 0;
            goto out;
        default:
            goto out;
    }

    if (fscanf(f, "%u", &result) == EOF)
    {
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        ERROR("Failed to read online attribute for CPU '%s'", name);
        goto out;
    }

    *is_online = (result == 1) ? TRUE : FALSE;

out:
    if (f != NULL)
        fclose(f);
    te_string_free(&buf);
    return rc;
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

#if SUPPORT_CACHES
static void
free_cache_item(cache_item *cache)
{
    free(cache->type);
    free(cache);
}

static void
free_cache_list(cache_item_list *cache_list)
{
    if (cache_list == NULL)
        return;

    while (!LIST_EMPTY(cache_list))
    {
        cache_item *cache = LIST_FIRST(cache_list);

        LIST_REMOVE(cache, next);
        free_cache_item(cache);
    }
}
#endif

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
#if SUPPORT_CACHES
        free_cache_list(&item->cache_list);
#endif
        free(item);
    }
}

#if SUPPORT_CACHES
static te_bool
compare_cache_items(cache_item *item1, cache_item *item2)
{
    if (item1->sys_id != item2->sys_id)
        return FALSE;
    if (strcmp(item1->type, item2->type) != 0)
        return FALSE;
    if (item1->level != item2->level)
        return FALSE;

    return TRUE;
}

static void
add_cache_to_cpu_item(cpu_item_list *root, cache_item *cache)
{
    cpu_item *item;
    cache_item *cache_item = NULL;

    item = find_item_by_cpu_set(root, &cache->shared_cpu_set);

    if (item == NULL)
        TE_FATAL_ERROR("Cache item does not belong to any CPU");

    LIST_FOREACH(cache_item, &item->cache_list, next)
    {
        if (compare_cache_items(cache_item, cache))
        {
            free_cache_item(cache);
            return;
        }
    }

    if (LIST_FIRST(&item->cache_list) != NULL)
        cache->id = LIST_FIRST(&item->cache_list)->id + 1;
    LIST_INSERT_HEAD(&item->cache_list, cache, next);
}

static te_errno
read_shared_cpu_list(const char *cpu_name, const char *index_name,
                     cpu_set_t *shared_cpu_set)
{
    char shared_cpu_list[RCF_MAX_VAL];
    te_errno rc = 0;

    rc = read_sys_value(shared_cpu_list, sizeof(shared_cpu_list), FALSE,
                        SYSFS_SYSTEM_TREE "/cpu/%s/cache/%s/shared_cpu_list",
                        cpu_name, index_name);

    if (rc != 0)
    {
        ERROR("Failed to read shared_cpu_list system file for cache %s of %s",
              index_name, cpu_name);
        return rc;
    }

    rc = te_cpuset_parse(shared_cpu_list, shared_cpu_set);

    return rc;
}

static te_errno
get_cache_dim(const char *cpu_name, const char *index_name,
              const char *item_name, uintmax_t *dim)
{
    char result[RCF_MAX_VAL];
    te_unit unit;
    te_errno rc = 0;

    rc = read_sys_value(result, sizeof(result), FALSE, SYSFS_SYSTEM_TREE
                        "/cpu/%s/cache/%s/%s", cpu_name,
                        index_name, item_name);

    if (rc != 0)
    {
        ERROR("Failed to read %s system file for cache %s of %s",
              item_name, index_name, cpu_name);
        return rc;
    }

    if ((rc = te_unit_from_string(result, &unit)) != 0)
    {
        ERROR("Failed to create %s unit from string for cache %s of %s",
              item_name, index_name, cpu_name);
        return rc;
    }

    rc = te_double2uint_safe(te_unit_bin_unpack(unit), UINTMAX_MAX, dim);

    return rc;
}

static te_errno
get_type(const char *cpu_name, const char *index_name, char **type)
{
    char result[RCF_MAX_VAL];
    te_errno rc = 0;

    rc = read_sys_value(result, sizeof(result), FALSE, SYSFS_SYSTEM_TREE
                        "/cpu/%s/cache/%s/type", cpu_name, index_name);

    if (rc != 0)
    {
        ERROR("Failed to read type system file for cache %s of %s", index_name,
              cpu_name);
        return rc;
    }

    *type = strdup(result);

    return 0;
}

static cache_item *
init_cache_item(uintmax_t sys_id, char *type, uintmax_t level,
                uintmax_t linesize, uintmax_t size)
{
    cache_item *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        TE_FATAL_ERROR("Failed to allocate a cache item");

    result->id = 0;
    result->sys_id = sys_id;
    result->type = type;
    result->level = level;
    result->linesize = linesize;
    result->size = size;

    return result;
}

static te_errno
get_cache_info(const char *cpu_name, const char *index_name,
               cache_item **cache)
{
    uintmax_t sys_id;
    char *type;
    uintmax_t level;
    uintmax_t linesize;
    uintmax_t size;
    te_errno rc;

    rc = get_cache_dim(cpu_name, index_name, "id", &sys_id);
    if (rc != 0)
        return rc;

    rc = get_type(cpu_name, index_name, &type);
    if (rc != 0)
        return rc;

    rc = get_cache_dim(cpu_name, index_name, "level", &level);
    if (rc != 0)
        return rc;

    rc = get_cache_dim(cpu_name, index_name, "coherency_line_size", &linesize);
    if (rc != 0)
        return rc;

    rc = get_cache_dim(cpu_name, index_name, "size", &size);
    if (rc != 0)
        return rc;

    *cache = init_cache_item(sys_id, type, level, linesize, size);

    return 0;
}

typedef struct cache_location {
    cpu_item_list *root;
    const char *name;
} cache_location;

static te_file_scandir_callback add_index_name;
static te_errno
add_index_name(const char *pattern, const char *pathname, void *data)
{
    cache_location *location = data;
    cpu_item_list *root = location->root;
    const char *name = location->name;
    char *index_name = strrchr(pathname, '/') + 1;
    cache_item *cache = NULL;
    te_errno rc = 0;

    rc = get_cache_info(name, index_name, &cache);
    if (cache == NULL)
    {
        ERROR("Could not get information about cache %s for %s", index_name,
              name);
        return rc;
    }

    rc = read_shared_cpu_list(name, index_name, &cache->shared_cpu_set);
    if (rc != 0)
        return rc;

    add_cache_to_cpu_item(root, cache);

    return rc;
}

static te_errno
insert_cache_info(cpu_item_list *root, const char *name)
{
    te_errno rc = 0;
    char buf[RCF_MAX_VAL];
    cache_location data = {root, name};

    TE_SPRINTF(buf, SYSFS_SYSTEM_TREE "/cpu/%s/cache", name);
    rc = te_file_scandir(buf, add_index_name, &data, "index*");

    if (rc != 0)
        ERROR("Could not scan cache directory for %s", name);

    return rc;
}
#endif

static te_errno
scan_system(cpu_item_list *root)
{
    struct dirent **names = NULL;
    char *isolated_str = NULL;
    cpu_item_list result = LIST_HEAD_INITIALIZER(&result);
    int n_cpus = 0;
    te_errno rc = 0;
    int i;
    te_bool is_online = TRUE;

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
        if ((rc = is_cpu_online(names[i]->d_name, &is_online)) != 0)
        {
            ERROR("Could not get info about online/offline status of '%s'",
                  names[i]->d_name);
            goto out;
        }

        if (!is_online)
            continue;

        if ((rc = populate_cpu(&result, names[i]->d_name, isolated_str)) != 0)
        {
            ERROR("Only %d CPUs populated, could not get info about '%s'",
                 i, names[i]->d_name);
            goto out;
        }
    }

#if SUPPORT_CACHES
    for (i = 0; i < n_cpus; i++)
    {
        if ((rc = insert_cache_info(&result, names[i]->d_name)) != 0)
        {
            ERROR("Could not get info about cache available to '%s'",
                  names[i]->d_name);
            goto out;
        }
    }
#endif

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
update_cpu_info(void)
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

#if SUPPORT_CACHES
static cache_item *
find_cache(const char *node_id_str, const char *package_id_str,
           const char *core_id_str, const char *cache_id_str)
{
    unsigned long node_id;
    unsigned long package_id;
    unsigned long core_id;
    unsigned long cache_id;
    cpu_item *item = NULL;
    cache_item *cache = NULL;
    te_errno rc;

    if ((rc = get_index(node_id_str, &node_id)) != 0)
        return NULL;

    if ((rc = get_index(package_id_str, &package_id)) != 0)
        return NULL;

    if (core_id_str != NULL)
    {
        if ((rc = get_index(core_id_str, &core_id)) != 0)
            return NULL;
    }

    if (core_id_str != NULL)
    {
        item = find_cpu_item(&global_cpu_item_root, CPU_ITEM_CORE,
                             (unsigned int[]){node_id, package_id, core_id});
    }
    else
    {
        item = find_cpu_item(&global_cpu_item_root, CPU_ITEM_PACKAGE,
                             (unsigned int[]){node_id, package_id});
    }
    if (item == NULL)
        return NULL;

    if ((rc = get_index(cache_id_str, &cache_id)) != 0)
        return NULL;

    LIST_FOREACH(cache, &item->cache_list, next)
    {
        if (cache->id == cache_id)
            return cache;
    }

    return NULL;
}

static void
copy_cache_item_field(cache_item *cache, char *value, cache_item_field field)
{
    uintmax_t numval;

    switch (field)
    {
        case CACHE_ITEM_LEVEL:
            numval = cache->level;
            break;

        case CACHE_ITEM_LINESIZE:
            numval = cache->linesize;
            break;

        case CACHE_ITEM_SIZE:
            numval = cache->size;
            break;

        default:
            TE_FATAL_ERROR("No such field in cache_item structure");
            break;
    }

    snprintf(value, RCF_MAX_VAL, "%ju", numval);
}

static te_errno
cpu_cache_get_value(const char *node_str, const char *package_str,
                    const char *core_str, const char *cache_str,
                    char *value, cache_item_field field)
{
    cache_item *cache = NULL;

    cache = find_cache(node_str, package_str, core_str, cache_str);
    if (cache == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    copy_cache_item_field(cache, value, field);

    return 0;
}
#endif

static te_errno
cpu_core_cache_level_get(unsigned int gid, const char *oid, char *value,
                         const char *unused1, const char *node_str,
                         const char *package_str, const char *core_str,
                         const char *cache_str)
{
    te_errno rc = TE_EOPNOTSUPP;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

#if SUPPORT_CACHES
    rc = cpu_cache_get_value(node_str, package_str, core_str, cache_str, value,
                             CACHE_ITEM_LEVEL);
#else
    UNUSED(node_str);
    UNUSED(package_str);
    UNUSED(core_str);
    UNUSED(cache_str);
#endif

    return rc;
}

static te_errno
cpu_package_cache_level_get(unsigned int gid, const char *oid, char *value,
                         const char *unused1, const char *node_str,
                         const char *package_str, const char *cache_str)
{
    te_errno rc = TE_EOPNOTSUPP;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

#if SUPPORT_CACHES
    rc = cpu_cache_get_value(node_str, package_str, NULL, cache_str, value,
                             CACHE_ITEM_LEVEL);
#else
    UNUSED(node_str);
    UNUSED(package_str);
    UNUSED(cache_str);
#endif

    return rc;
}

static te_errno
cpu_core_cache_linesize_get(unsigned int gid, const char *oid, char *value,
                            const char *unused1, const char *node_str,
                            const char *package_str, const char *core_str,
                            const char *cache_str)
{
    te_errno rc = TE_EOPNOTSUPP;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

#if SUPPORT_CACHES
    rc = cpu_cache_get_value(node_str, package_str, core_str, cache_str, value,
                             CACHE_ITEM_LINESIZE);
#else
    UNUSED(node_str);
    UNUSED(package_str);
    UNUSED(core_str);
    UNUSED(cache_str);
#endif

    return rc;
}

static te_errno
cpu_package_cache_linesize_get(unsigned int gid, const char *oid, char *value,
                               const char *unused1, const char *node_str,
                               const char *package_str, const char *cache_str)
{
    te_errno rc = TE_EOPNOTSUPP;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

#if SUPPORT_CACHES
    rc = cpu_cache_get_value(node_str, package_str, NULL, cache_str, value,
                             CACHE_ITEM_LINESIZE);
#else
    UNUSED(node_str);
    UNUSED(package_str);
    UNUSED(cache_str);
#endif

    return rc;
}

static te_errno
cpu_core_cache_size_get(unsigned int gid, const char *oid, char *value,
                        const char *unused1, const char *node_str,
                        const char *package_str, const char *core_str,
                        const char *cache_str)
{
    te_errno rc = TE_EOPNOTSUPP;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

#if SUPPORT_CACHES
    rc = cpu_cache_get_value(node_str, package_str, core_str, cache_str, value,
                             CACHE_ITEM_SIZE);
#else
    UNUSED(node_str);
    UNUSED(package_str);
    UNUSED(core_str);
    UNUSED(cache_str);
#endif

    return rc;
}

static te_errno
cpu_package_cache_size_get(unsigned int gid, const char *oid, char *value,
                           const char *unused1, const char *node_str,
                           const char *package_str, const char *cache_str)
{
    te_errno rc = TE_EOPNOTSUPP;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

#if SUPPORT_CACHES
    rc = cpu_cache_get_value(node_str, package_str, NULL, cache_str, value,
                             CACHE_ITEM_SIZE);
#else
    UNUSED(node_str);
    UNUSED(package_str);
    UNUSED(cache_str);
#endif

    return rc;
}

#if SUPPORT_CACHES
static te_errno
cpu_cache_type_get(char *value, const char *node_id_str,
                   const char *package_id_str, const char *core_id_str,
                   const char *cache_id_str)
{
    cache_item *cache = NULL;

    cache = find_cache(node_id_str, package_id_str, core_id_str, cache_id_str);
    if (cache == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(value, cache->type);

    return 0;
}
#endif

static te_errno
cpu_core_cache_type_get(unsigned int gid, const char *oid, char *value,
                        const char *unused1, const char *node_id_str,
                        const char *package_id_str, const char *core_id_str,
                        const char *cache_id_str)
{
    te_errno rc = TE_EOPNOTSUPP;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

#if SUPPORT_CACHES
    rc = cpu_cache_type_get(value, node_id_str, package_id_str, core_id_str,
                            cache_id_str);
#else
    UNUSED(node_id_str);
    UNUSED(package_id_str);
    UNUSED(core_id_str);
    UNUSED(cache_id_str);
#endif

    return rc;
}

static te_errno
cpu_package_cache_type_get(unsigned int gid, const char *oid, char *value,
                           const char *unused1, const char *node_id_str,
                           const char *package_id_str, const char *cache_id_str)
{
    te_errno rc = TE_EOPNOTSUPP;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);

#if SUPPORT_CACHES
    rc = cpu_cache_type_get(value, node_id_str, package_id_str, NULL,
                            cache_id_str);
#else
    UNUSED(node_id_str);
    UNUSED(package_id_str);
    UNUSED(cache_id_str);
#endif

    return rc;
}

static te_errno
cpu_core_cache_list(unsigned int gid, const char *oid, const char *sub_id,
                    char **list, const char *unused1, const char *node_str,
                    const char *package_str, const char *core_str)
{
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);

#if SUPPORT_CACHES
    te_string result = TE_STRING_INIT;
    unsigned long node_id;
    unsigned long package_id;
    unsigned long core_id;
    te_bool first = TRUE;
    cpu_item *core = NULL;
    cache_item *cache;

    if ((rc = get_index(node_str, &node_id)) != 0)
        return rc;
    if ((rc = get_index(package_str, &package_id)) != 0)
        return rc;
    if ((rc = get_index(core_str, &core_id)) != 0)
        return rc;

    core = find_cpu_item(&global_cpu_item_root, CPU_ITEM_CORE,
                         (unsigned int[]){node_id, package_id, core_id});
    if (core == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(cache, &core->cache_list, next)
    {
        te_string_append(&result, "%s%u", first ? "" : " ", cache->id);
        first = FALSE;
    }

    *list = result.ptr;
#else
    UNUSED(node_str);
    UNUSED(package_str);
    UNUSED(core_str);

    rc = string_empty_list(list);
#endif

    return rc;
}

static te_errno
cpu_package_cache_list(unsigned int gid, const char *oid, const char *sub_id,
                       char **list, const char *unused1, const char *node_str,
                       const char *package_str)
{
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);

#if SUPPORT_CACHES
    te_string result = TE_STRING_INIT;
    unsigned long node_id;
    unsigned long package_id;
    te_bool first = TRUE;
    cpu_item *package = NULL;
    cache_item *cache;

    if ((rc = get_index(node_str, &node_id)) != 0)
        return rc;
    if ((rc = get_index(package_str, &package_id)) != 0)
        return rc;

    package = find_cpu_item(&global_cpu_item_root, CPU_ITEM_PACKAGE,
                            (unsigned int[]){node_id, package_id});
    if (package == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(cache, &package->cache_list, next)
    {
        te_string_append(&result, "%s%u", first ? "" : " ", cache->id);
        first = FALSE;
    }

    *list = result.ptr;
#else
    UNUSED(node_str);
    UNUSED(package_str);

    rc = string_empty_list(list);
#endif

    return rc;
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

static te_errno
memory_get(unsigned int gid, const char *oid, char *value)
{
  UNUSED(gid);
  UNUSED(oid);
  uint64_t mem = 0;

#ifdef _SC_PHYS_PAGES
  long n_pages, page_size;

  errno = 0;

  n_pages = sysconf(_SC_PHYS_PAGES);
  if (n_pages == -1)
  {
      ERROR("Failed to get sysconf number of memory pages");
      return TE_OS_RC(TE_TA_UNIX, errno);
  }

  page_size = sysconf(_SC_PAGESIZE);
  if (page_size == -1)
  {
      ERROR("Failed to get sysconf memory page size");
      return TE_OS_RC(TE_TA_UNIX, errno);
  }

  /* Total memory in bytes */
  mem = (uint64_t)n_pages * (uint64_t)page_size;
#endif

  snprintf(value, RCF_MAX_VAL, "%" PRIu64, mem);
  return 0;
}

static te_errno
avail_memory_get(unsigned int gid, const char *oid, char *value)
{
  UNUSED(gid);
  UNUSED(oid);
  uint64_t avail_mem = 0;

#ifdef _SC_AVPHYS_PAGES
  long avail_pages, page_size;

  errno = 0;

  avail_pages = sysconf(_SC_AVPHYS_PAGES);
  if (avail_pages == -1)
  {
      ERROR("Failed to get sysconf number of available pages");
      return TE_OS_RC(TE_TA_UNIX, errno);
  }

  page_size = sysconf(_SC_PAGESIZE);
  if (page_size == -1)
  {
      ERROR("Failed to get sysconf memory page size");
      return TE_OS_RC(TE_TA_UNIX, errno);
  }

  /* Available memory in bytes */
  avail_mem = (uint64_t)avail_pages * (uint64_t)page_size;
#endif

  snprintf(value, RCF_MAX_VAL, "%" PRIu64, avail_mem);
  return 0;
}


RCF_PCH_CFG_NODE_RO(node_thread_isolated, "isolated",
                    NULL, NULL, cpu_thread_isolated_get);

RCF_PCH_CFG_NODE_RO(node_cpu_core_cache_size, "size", NULL, NULL,
                    cpu_core_cache_size_get);

RCF_PCH_CFG_NODE_RO(node_cpu_core_cache_linesize, "linesize", NULL,
                    &node_cpu_core_cache_size, cpu_core_cache_linesize_get);

RCF_PCH_CFG_NODE_RO(node_cpu_core_cache_level, "level", NULL,
                    &node_cpu_core_cache_linesize, cpu_core_cache_level_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_cpu_core_cache, "cache",
                               &node_cpu_core_cache_level, NULL,
                               cpu_core_cache_type_get, cpu_core_cache_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_cpu_thread, "thread", &node_thread_isolated,
                               &node_cpu_core_cache, NULL, cpu_thread_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_cpu_core, "core",
                               &node_cpu_thread, NULL,
                               NULL, cpu_core_list);

RCF_PCH_CFG_NODE_RO(node_cpu_package_cache_size, "size", NULL, NULL,
                    cpu_package_cache_size_get);

RCF_PCH_CFG_NODE_RO(node_cpu_package_cache_linesize, "linesize", NULL,
                    &node_cpu_package_cache_size,
                    cpu_package_cache_linesize_get);

RCF_PCH_CFG_NODE_RO(node_cpu_package_cache_level, "level", NULL,
                    &node_cpu_package_cache_linesize,
                    cpu_package_cache_level_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_cpu_package_cache, "cache",
                               &node_cpu_package_cache_level, &node_cpu_core,
                               cpu_package_cache_type_get,
                               cpu_package_cache_list);

RCF_PCH_CFG_NODE_RO(node_avail_memory, "free", NULL, NULL,
                    avail_memory_get);

RCF_PCH_CFG_NODE_RO(node_memory, "memory", &node_avail_memory, NULL,
                    memory_get);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_cpu, "cpu", &node_cpu_package_cache,
                               &node_memory, NULL, cpu_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_numa_node, "node",
                               &node_cpu, NULL,
                               NULL, numa_list);

te_errno
ta_unix_conf_cpu_init(void)
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
