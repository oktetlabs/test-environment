/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Unix Test Agent
 *
 * IRQ Ethernet interface counters
 */

#define TE_LGR_USER     "IRQ stats Conf"

#include <dirent.h>
#include <sys/stat.h>

#include "te_config.h"
#include "config.h"

#include "logger_api.h"
#include "unix_internal.h"
#include "te_alloc.h"
#include "rcf_pch_ta_cfg.h"

#include "te_printf.h"

#define MAX_IRQ_NAME_LEN 64

struct ta_irq_per_cpu {
    unsigned int cpu;
    uint64_t     num;
};

struct ta_irq_obj {
    unsigned int  irq_num;
    char          name[MAX_IRQ_NAME_LEN];
    te_vec        irq_per_cpu;
    uint64_t      smp_affinity;
};

static void
free_irqs_vec(void *data)
{
    te_vec *vec = data;
    struct ta_irq_obj *irq_obj;

    TE_VEC_FOREACH(vec, irq_obj)
    {
        te_vec_free(&irq_obj->irq_per_cpu);
    }
    te_vec_free(vec);
    free(data);
}

static int
compare_irqs(const void *obj1, const void *obj2)
{
    const struct ta_irq_obj *r1 = obj1;
    const struct ta_irq_obj *r2 = obj2;

    if (r1->irq_num > r2->irq_num)
        return 1;
    else if (r1->irq_num < r2->irq_num)
        return -1;
    else
        return 0;
}

static int
search_irqs(const void *key, const void *elt)
{
    unsigned int k = *(unsigned int *)key;
    struct ta_irq_obj aux_irq_obj;

    aux_irq_obj.irq_num = k;
    return compare_irqs(&aux_irq_obj, elt);
}

static int
compare_ipc(const void *obj1, const void *obj2)
{
    const struct ta_irq_per_cpu *r1 = obj1;
    const struct ta_irq_per_cpu *r2 = obj2;

    if (r1->cpu > r2->cpu)
        return 1;
    else if (r1->cpu < r2->cpu)
        return -1;
    else
        return 0;
}

static int
search_ipc(const void *key, const void *elt)
{
    unsigned int k = *(unsigned int *)key;
    struct ta_irq_per_cpu aux_irq_per_cpu;

    aux_irq_per_cpu.cpu = k;
    return compare_ipc(&aux_irq_per_cpu, elt);
}

/**
 * This function checks that IRQs which were got from
 * /sys/class/net/<iface>/device/msi_irqs and from
 * /sys/class/net/<iface>/device/irq are really registered,
 * so that are really will be in /proc/interrupts file.
 */
static bool
is_irq_registered(unsigned int irq_num)
{
    char path[PATH_MAX];
    struct stat st;
    te_errno rc;

    rc = te_snprintf(path, sizeof(path),"/proc/irq/%u", irq_num);
    if (rc != 0)
        return false;

    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return true;

    return false;
}

static void
add_irq_obj(te_vec *irqs_vec, unsigned int irq_num)
{
    struct ta_irq_obj irq_obj;

    irq_obj.irq_num = irq_num;
    irq_obj.irq_per_cpu = TE_VEC_INIT(struct ta_irq_per_cpu);
    TE_VEC_APPEND(irqs_vec, irq_obj);
}

static te_errno
del_irq_obj(te_vec *irqs_vec, unsigned int irq_num)
{
    unsigned int index = 0;
    bool found = false;

    found = te_vec_search(irqs_vec, &irq_num, &search_irqs, &index, NULL);
    if (!found)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_vec_remove_index(irqs_vec, index);
    return 0;
}

static unsigned int
get_legacy_irq(const char *if_name)
{
    char path[PATH_MAX];
    FILE *f;
    unsigned int irq_num = 0;

    snprintf(path, sizeof(path), "/sys/class/net/%s/device/irq", if_name);

    f = fopen(path, "r");
    if (f == NULL)
        return 0;

    if (fscanf(f, "%u", &irq_num) != 1)
        irq_num = 0;

    fclose(f);
    return irq_num;
}

static te_errno
get_irqs(const char *if_name, te_vec *irqs)
{
    char path[PATH_MAX];
    DIR *dir;
    struct dirent *entry;
    te_errno rc;
    unsigned int irq_num;
    bool old_added = false;

    *irqs = TE_VEC_INIT(struct ta_irq_obj);
    irq_num = get_legacy_irq(if_name);
    if (irq_num != 0 && is_irq_registered(irq_num))
    {
        add_irq_obj(irqs, irq_num);
        old_added = true;
    }

    snprintf(path, sizeof(path), "/sys/class/net/%s/device/msi_irqs", if_name);
    dir = opendir(path);
    if (dir == NULL)
    {
        /* Virtual interfaces may have no own IRQ source. */
        if (errno == ENOENT || errno == ENODEV || errno == ENOTDIR)
            return 0;

        if (old_added)
            return 0;
        else
            return TE_OS_RC(TE_TA_UNIX, errno);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        rc = te_strtoui(entry->d_name, 10, &irq_num);
        if (rc != 0)
        {
            ERROR("%s(): failed to convert %s to integer.", __FUNCTION__,
                  entry->d_name);
            (void)closedir(dir);
            return rc;
        }

        if (is_irq_registered(irq_num))
            add_irq_obj(irqs, irq_num);
    }
    (void)closedir(dir);

    te_vec_sort(irqs, &compare_irqs);

    return 0;
}

static char *
get_ulong_move_next(char *line, const char *pref, unsigned long *val)
{
    char *current = line;
    char *end_digits;

    if (pref != NULL)
    {
        current = strstr(current, pref);
        if (current == NULL)
            return NULL;
        current += strlen(pref);
    }
    else
    {
        while (isspace((unsigned char)*current))
            current++;
    }

    *val = strtoul(current, &end_digits, 10);
    if (current == end_digits || (*val == ULONG_MAX && errno == ERANGE))
        return NULL;

    return end_digits;
}

static te_errno
get_irq_name(const char *line, char *name, size_t max_len)
{
    const char *ptr = line;
    char *back;
    int i;

    if (max_len == 0)
        return 0;

    if (line == NULL || name == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    /* Skip two unneeded words and whitespaces before IRQ name */
    for (i = 0; i < 2; i++)
    {
        while (isspace((unsigned char)*ptr) && *ptr != '\0')
            ptr++;
        while (!isspace((unsigned char)*ptr) && *ptr != '\0')
            ptr++;
        if (*ptr == '\0')
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    while (isspace((unsigned char)*ptr) && *ptr != '\0')
        ptr++;

    te_strlcpy(name, ptr, max_len);

    /* Remove unneeded symbols from the end of line. */
    back = name + strlen(name);
    while (back > name && isspace((unsigned char)*--back))
        *back = '\0';

    return 0;
}

static te_errno
ta_irq_fill_objs(unsigned int gid, const char *if_name)
{
    te_string obj_name = TE_STRING_INIT_STATIC(CFG_OID_MAX);
    ta_cfg_obj_t *obj;
    te_vec *result_irqs = NULL;
    te_vec remove_irqs = TE_VEC_INIT(unsigned int);
    char line[1024];
    te_vec  cpus;
    struct ta_irq_obj *irq_obj;

    FILE *f = NULL;
    char *ptr;
    te_errno rc = 0;
    int  rest_len;
    unsigned long val;
    unsigned int *rem_irq;

    cpus = TE_VEC_INIT(struct ta_irq_per_cpu);

    te_string_append(&obj_name, "%s", if_name);
    obj = ta_obj_find(TA_OBJ_TYPE_IF_IRQ, te_string_value(&obj_name),
                      gid);
    if (obj != NULL)
    {
        ERROR("IRQ object for %s iface already exists.", if_name);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    result_irqs = TE_ALLOC(sizeof(te_vec));
    rc = get_irqs(if_name, result_irqs);
    if (rc != 0)
        goto cleanup;

    if (te_vec_size(result_irqs) == 0)
    {
        rc = ta_obj_add(TA_OBJ_TYPE_IF_IRQ,
                        te_string_value(&obj_name),
                        "", gid, result_irqs, &free_irqs_vec, &obj);
        goto cleanup;
    }

    f = fopen("/proc/interrupts", "r");
    if (f == NULL)
    {
        ERROR("Failed to open /proc/interrupts for reading: %s",
              strerror(errno));
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

    if ((ptr = fgets(line, sizeof(line), f)) == NULL)
    {
        ERROR("Failed to get first line from /proc/interrupts.");
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup;
    }

    /* Get all CPU numbers from /proc/interrupts table. */
    rest_len = strlen(line);
    ptr = line;
    do {
        struct ta_irq_per_cpu ipc;
        ptr = get_ulong_move_next(ptr, "CPU", &val);
        if (ptr == NULL)
            break;

        ipc.cpu = (unsigned int)val;
        TE_VEC_APPEND(&cpus, ipc);
    } while (ptr - line < rest_len);
    te_vec_sort(&cpus, &compare_ipc);

    TE_VEC_FOREACH(result_irqs, irq_obj)
    {
        bool rewind_is_done = false;
        char *current;
        unsigned int irq_val;
        struct ta_irq_per_cpu *irq_per_cpu;
        bool removed_irq = false;
        FILE *affinity_file;
        char *affinity_path;

        /* Check affinity mask */
        affinity_path = te_string_fmt("/proc/irq/%u/smp_affinity",
                                      irq_obj->irq_num);
        affinity_file = fopen(affinity_path, "r");
        free(affinity_path);
        if (affinity_file == NULL)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("Failed to open IRQ %u affinity mask", irq_obj->irq_num);
            goto cleanup;
        }
        if (fgets(line, sizeof(line), affinity_file) == NULL)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            ERROR("Failed to read IRQ %u affinity mask", irq_obj->irq_num);
            fclose(affinity_file);
            goto cleanup;
        }
        fclose(affinity_file);
        if (sscanf(line, "%" TE_PRINTF_64 "x", &irq_obj->smp_affinity) != 1)
        {
            ERROR("Failed to parse IRQ %u affinity mask", irq_obj->irq_num);
            goto cleanup;
        }

        /* Gather interrupt counters */
        while (true)
        {
            if (fgets(line, sizeof(line), f) == NULL)
            {
                if (rewind_is_done)
                {
                    INFO("Failed to find IRQ:%u in /proc/interrupts.",
                         irq_obj->irq_num);
                    TE_VEC_APPEND(&remove_irqs, irq_obj->irq_num);
                    rewind(f);
                    removed_irq = true;
                    break;
                }
                else
                {
                    rewind(f);
                    rewind_is_done = true;
                }
            }
            if (sscanf(line, "%u:", &irq_val) != 1)
                continue;
            if (irq_val == irq_obj->irq_num)
                break;
        }
        if (removed_irq)
            continue;
        current = strchr(line, ':') + 1;

        TE_VEC_FOREACH(&cpus, irq_per_cpu)
        {
            current = get_ulong_move_next(current, NULL, &val);
            if (current == NULL)
            {
                ERROR("Incorrect content of /proc/interrupts");
                rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
                goto cleanup;
            }
            irq_per_cpu->num = (uint64_t)val;
        }
        rc = te_vec_append_vec(&irq_obj->irq_per_cpu, &cpus);
        if (rc != 0)
            goto cleanup;

        rc = get_irq_name(current, irq_obj->name, MAX_IRQ_NAME_LEN);
        if (rc != 0)
            goto cleanup;
    }
    TE_VEC_FOREACH(&remove_irqs, rem_irq)
    {
        rc = del_irq_obj(result_irqs, *rem_irq);
        if (rc != 0)
            goto cleanup;
    }

    rc = ta_obj_add(TA_OBJ_TYPE_IF_IRQ,
                    te_string_value(&obj_name),
                    "", gid, result_irqs, &free_irqs_vec, &obj);

cleanup:
    if (rc != 0)
        free_irqs_vec(result_irqs);
    if (f != NULL)
        fclose(f);
    te_vec_free(&cpus);
    te_vec_free(&remove_irqs);
    te_string_free(&obj_name);

    return rc;
}

static te_errno
get_irqs_vec(unsigned int gid, const char *if_name,
             te_vec **irqs)
{
    te_string obj_name = TE_STRING_INIT_STATIC(CFG_OID_MAX);
    ta_cfg_obj_t *obj;
    te_errno rc;

    te_string_append(&obj_name, "%s", if_name);
    obj = ta_obj_find(TA_OBJ_TYPE_IF_IRQ, te_string_value(&obj_name),
                      gid);
    if (obj != NULL)
    {
        *irqs = (te_vec *)obj->user_data;
        return 0;
    }

    rc = ta_irq_fill_objs(gid, if_name);
    if (rc != 0)
        return rc;

    obj = ta_obj_find(TA_OBJ_TYPE_IF_IRQ, te_string_value(&obj_name),
                      gid);
    if (obj == NULL)
    {
        ERROR("Cannot find IRQ object for iface %s after adding it.", if_name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    *irqs = (te_vec *)obj->user_data;
    return 0;
}

static te_errno
get_irq_obj(unsigned int gid, const char *if_name, const char *irq_num_str,
            struct ta_irq_obj **irq_obj)
{
    te_vec *irqs;
    unsigned int irq_pos;
    unsigned int irq_num;
    bool irq_found = false;
    te_errno rc;

    rc = te_strtoui(irq_num_str, 10, &irq_num);
    if (rc != 0)
    {
        ERROR("%s(): failed to convert %s to integer.", __FUNCTION__,
              irq_num_str);
        return rc;
    }
    rc = get_irqs_vec(gid, if_name, &irqs);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    irq_found = te_vec_search(irqs, &irq_num, &search_irqs, &irq_pos, NULL);
    if (!irq_found)
    {
        ERROR("%s(): failed to find object for IRQ %s", __FUNCTION__,
              irq_num_str);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    *irq_obj = te_vec_get(irqs, irq_pos);
    return 0;
}

static te_errno
irq_smp_affinity_get(unsigned int gid, const char *oid, char *value,
                     const char *if_name, const char *irq_num)
{
    te_errno rc;
    struct ta_irq_obj *irq_obj;

    UNUSED(oid);

    rc = get_irq_obj(gid, if_name, irq_num, &irq_obj);
    if (rc != 0)
        return rc;

    snprintf(value, RCF_MAX_VAL, "%"PRIu64, irq_obj->smp_affinity);
    return 0;
}

static te_errno
irq_smp_affinity_set(unsigned int gid, const char *oid, const char *value,
                     const char *if_name, const char *irq_num)
{
    te_errno rc;
    char *path;
    FILE *f;
    struct ta_irq_obj *irq_obj;

    UNUSED(oid);

    path = te_string_fmt("/proc/irq/%s/smp_affinity", irq_num);
    f = fopen(path, "w");
    free(path);
    if (f == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Failed to open IRQ %s affinity mask for writing: %r",
              irq_num, rc);
        return rc;
    }

    rc = get_irq_obj(gid, if_name, irq_num, &irq_obj);
    if (rc != 0)
        return rc;

    rc = te_str_to_uint64(value, 10, &irq_obj->smp_affinity);
    if (rc != 0)
        return rc;

    if (fprintf(f, "%"PRIx64"\n", irq_obj->smp_affinity) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("Failed to write IRQ %s affinity mask: %r", irq_num, rc);
    }
    fclose(f);
    return rc;
}

static te_errno
irq_cpu_get(unsigned int gid, const char *oid, char *value,
            const char *if_name, const char *irq_num,
            const char *cpu_num_str)
{
    te_errno rc;
    struct ta_irq_obj *irq_obj;
    struct ta_irq_per_cpu *irq_per_cpu;
    unsigned int cpu_num;
    unsigned int cpu_pos;
    bool irq_found = false;

    UNUSED(oid);

    rc = te_strtoui(cpu_num_str, 10, &cpu_num);
    if (rc != 0)
    {
        ERROR("%s(): failed to convert %s to integer.", __FUNCTION__,
              cpu_num_str);
        return rc;
    }
    rc = get_irq_obj(gid, if_name, irq_num, &irq_obj);
    if (rc != 0)
        return rc;

    irq_found = te_vec_search(&irq_obj->irq_per_cpu, &cpu_num, &search_ipc,
                              &cpu_pos, NULL);
    if (!irq_found)
    {
        ERROR("%s(): failed to find object for IRQ %s CPU %s", __FUNCTION__,
              irq_num, cpu_num_str);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    irq_per_cpu = te_vec_get(&irq_obj->irq_per_cpu, cpu_pos);

    rc = te_snprintf(value, RCF_MAX_VAL, "%" TE_PRINTF_64 "d",
                     irq_per_cpu->num);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }
    return 0;
}

static te_errno
irq_cpu_list(unsigned int gid, const char *oid, const char *sub_id,
             char **list, const char *if_name, const char *irq_num)
{
    te_errno rc;
    struct ta_irq_obj *irq_obj;
    struct ta_irq_per_cpu *irq_per_cpu;
    te_string str = TE_STRING_INIT;

    UNUSED(oid);
    UNUSED(sub_id);

    rc = get_irq_obj(gid, if_name, irq_num, &irq_obj);
    if (rc != 0)
        return rc;

    TE_VEC_FOREACH(&irq_obj->irq_per_cpu, irq_per_cpu)
    {
        rc = te_string_append_chk(&str, "%d ", irq_per_cpu->cpu);
        if (rc != 0)
        {
            te_string_free(&str);
            return rc;
        }
    }
    *list = str.ptr;

    return 0;
}

static te_errno
irq_name_get(unsigned int gid, const char *oid, char *value,
             const char *if_name, const char *irq_num)
{
    te_errno rc;
    struct ta_irq_obj *irq_obj;

    UNUSED(oid);

    rc = get_irq_obj(gid, if_name, irq_num, &irq_obj);
    if (rc != 0)
        return rc;

    rc = te_snprintf(value, RCF_MAX_VAL, "%s", irq_obj->name);
    if (rc != 0)
    {
        ERROR("%s(): te_snprintf() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, rc);
    }
    return 0;
}

static te_errno
irq_list(unsigned int gid, const char *oid, const char *sub_id, char **list,
         const char *if_name)
{
    te_errno rc;
    te_vec *irqs;
    struct ta_irq_obj *irq_obj;
    te_string str = TE_STRING_INIT;

    UNUSED(oid);
    UNUSED(sub_id);

    rc = get_irqs_vec(gid, if_name, &irqs);
    if (rc != 0)
        return rc;

    TE_VEC_FOREACH(irqs, irq_obj)
    {
        rc = te_string_append_chk(&str, "%u ", irq_obj->irq_num);
        if (rc != 0)
        {
            te_string_free(&str);
            return rc;
        }
    }
    *list = str.ptr;

    return 0;
}

static rcf_pch_cfg_object node_irq_smp_affinity = {
    .sub_id = "smp_affinity",
    .get = (rcf_ch_cfg_get)irq_smp_affinity_get,
    .set = (rcf_ch_cfg_set)irq_smp_affinity_set,
};

static rcf_pch_cfg_object node_irq_cpu = {
    .sub_id = "cpu",
    .brother = &node_irq_smp_affinity,
    .get = (rcf_ch_cfg_get)irq_cpu_get,
    .list = (rcf_ch_cfg_list)irq_cpu_list,
};

static rcf_pch_cfg_object node_irq_name = {
    .sub_id = "name",
    .brother = &node_irq_cpu,
    .get = (rcf_ch_cfg_get)irq_name_get,
};

static rcf_pch_cfg_object node_irq = {
    .sub_id = "irq",
    .son = &node_irq_name,
    .list = (rcf_ch_cfg_list)irq_list,
};

/**
 * Add a child nodes for IRQ subtree to interface object.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_irq_stats_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_irq);
}
