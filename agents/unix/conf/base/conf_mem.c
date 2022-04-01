/** @file
 * @brief System module configuration support
 *
 * Implementation of configuration nodes for memory manipulation
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#define TE_LGR_USER "Unix Conf Memory Module"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#if HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#include "rcf_pch.h"
#include "rcf_ch_api.h"
#include "conf_common.h"
#include "unix_internal.h"
#include "logger_api.h"
#include "te_string.h"
#include "te_units.h"
#include "te_alloc.h"
#include "te_vector.h"

/** Path to system hugepage directory */
#define SYS_HUGEPAGES "/sys/kernel/mm/hugepages"
/** Name of the file to store size of the pool of the hugepages */
#define HUGEPAGES_FILENAME_NR "nr_hugepages"
/** Name of the file to store free hugepages */
#define HUGEPAGES_FILENAME_FREE "free_hugepages"
/** Name of the file to store reserved hugepages */
#define HUGEPAGES_FILENAME_RESV "resv_hugepages"

/** Delimiter used in the mountpount name instead of '/' */
#define PATH_DELIMITER "$"

/** Information about mountpoint */
struct mountpoint_info
{
    /** Linked list of mountpoints */
    LIST_ENTRY(mountpoint_info)
    link;
    /** Directory name */
    char *name;
    /** Value from the 'pagesize' option of hugetlbfs in kB */
    unsigned int hp_size;
    /** Is the directory already created */
    te_bool is_created;
    /** Is the directory mounted by the agent */
    te_bool is_mounted;
};

/** List of the mountpoints */
typedef LIST_HEAD(mount_dirs_list_t, mountpoint_info) mount_dirs_list_t;

/** Information about hugepage */
struct hugepage_info
{
    /** Linked list of hugepages info */
    LIST_ENTRY(hugepage_info) link;
    /** Hugepage size in kB */
    unsigned int size;
    /** Size of the pool of hugepages */
    unsigned int nr_hugepages;
    /** Number of hugepages in the pool that are not yet allocated */
    unsigned int free_hugepages;
    /**
     * Number of hugepages to allocate from the pool,
     * but no allocation has yet been made.
     */
    unsigned int resv_hugepages;
    /** Mountpoints for the hugepages */
    mount_dirs_list_t mount_dirs;
};

/** List of supported hugepage sizes */
static LIST_HEAD(, hugepage_info) hugepages = LIST_HEAD_INITIALIZER(hugepages);

static te_errno
add_hugepage_info(unsigned int size)
{
    struct hugepage_info *hp_info;

    hp_info = TE_ALLOC(sizeof(*hp_info));
    if (hp_info == NULL)
        return TE_ENOMEM;

    hp_info->size = size;

    LIST_INIT(&hp_info->mount_dirs);

    LIST_INSERT_HEAD(&hugepages, hp_info, link);

    return 0;
}

static te_errno
find_hugepage_info(const char *size_str, struct hugepage_info **hp_info,
                   te_bool quiet)
{
    struct hugepage_info *tmp;
    long int size;
    te_errno rc;

    if (!rcf_pch_rsrc_accessible("/agent:%s/mem:/hugepages:%s",
                                 ta_name, size_str))
    {
        if (quiet)
        {
            INFO("%s(): Hugepage with size %s is not grabbed as resourse: %r",
                  __FUNCTION__, size_str, TE_EPERM);
        }
        else
        {
            ERROR("%s(): Hugepage with size %s is not grabbed as resourse: %r",
                  __FUNCTION__, size_str, TE_EPERM);
        }
        return TE_EPERM;
    }

    rc = te_strtol(size_str, 10, &size);
    if (rc != 0)
        return rc;

    LIST_FOREACH(tmp, &hugepages, link)
    {
        if (tmp->size == size)
        {
            *hp_info = tmp;
            return 0;
        }
    }

    ERROR("%s(): Hugepage with size %s is not supported", __FUNCTION__, size_str);
    return TE_ENOENT;
}

static void
free_hugepage_info_list(void)
{
    struct hugepage_info *hp_info;
    struct hugepage_info *hp_info_tmp;
    struct mountpoint_info *mp_info;
    struct mountpoint_info *mp_info_tmp;

    LIST_FOREACH_SAFE(hp_info, &hugepages, link, hp_info_tmp)
    {
        LIST_FOREACH_SAFE(mp_info, &hp_info->mount_dirs, link, mp_info_tmp)
        {
            LIST_REMOVE(mp_info, link);
            free(mp_info->name);
            free(mp_info);
        }

        LIST_REMOVE(hp_info, link);
        free(hp_info);
    }
}

static te_errno
read_hugepage_attr_value(const char *filename, unsigned int hp_size,
                         unsigned int *value)
{
    FILE *file;
    te_string buf = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_append(&buf, SYS_HUGEPAGES "/hugepages-%ukB/%s",
                          hp_size, filename);
    if (rc != 0)
        return rc;

    file = fopen(buf.ptr, "r");
    if (file == NULL)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): Failed to open file %s: %r", __FUNCTION__, buf.ptr, rc);
        te_string_free(&buf);
        return rc;
    }

    if (fscanf(file, "%u", value) != 1)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): Cannot parse file %s: %r", __FUNCTION__, buf.ptr, rc);
    }

    fclose(file);
    te_string_free(&buf);

    return rc;
}

static te_errno
scan_hugepage_info(struct hugepage_info *hp_info)
{
    te_errno rc;

    rc = read_hugepage_attr_value(HUGEPAGES_FILENAME_NR, hp_info->size,
                                  &hp_info->nr_hugepages);
    if (rc != 0)
        return rc;

    rc = read_hugepage_attr_value(HUGEPAGES_FILENAME_FREE, hp_info->size,
                                  &hp_info->free_hugepages);
    if (rc != 0)
        return rc;

    rc = read_hugepage_attr_value(HUGEPAGES_FILENAME_RESV, hp_info->size,
                                  &hp_info->resv_hugepages);

    return rc;
}

static te_errno
write_value(const struct hugepage_info *hp_info, unsigned int number)
{
    FILE *file;
    te_string buf = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_append(&buf, SYS_HUGEPAGES "/hugepages-%ukB/%s",
                          hp_info->size, HUGEPAGES_FILENAME_NR);
    if (rc != 0)
        return rc;

    file = fopen(buf.ptr, "r+");
    if (file == NULL)
    {
        rc = te_rc_os2te(errno);
        te_string_free(&buf);
        ERROR("%s(): Failed to open file %s: %r", __FUNCTION__, buf.ptr, rc);
        return rc;
    }

    if (fprintf(file, "%u", number) < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): Failed to write a new value to %s: %r",
              __FUNCTION__, HUGEPAGES_FILENAME_NR, rc);
    }

    fclose(file);
    te_string_free(&buf);

    return rc;
}

static te_errno
alloc_hugepages(struct hugepage_info *hp_info, unsigned int number)
{
    te_errno rc = 0;
    size_t old_nr_nugepages;

    rc = scan_hugepage_info(hp_info);
    if (rc != 0)
    {
        ERROR("%s(): Failed to scan information about hugepages: %r",
              __FUNCTION__, rc);
        return rc;
    }

    old_nr_nugepages = hp_info->nr_hugepages;

    rc = write_value(hp_info, number);
    if (rc != 0)
        return rc;

    rc = scan_hugepage_info(hp_info);
    if (rc != 0)
    {
        ERROR("%s(): Failed to scan information about hugepages: %r",
              __FUNCTION__, rc);
        return rc;
    }

    if (hp_info->nr_hugepages != number)
    {
        ERROR("%s(): Failed to allocate hugepages", __FUNCTION__);
        rc = write_value(hp_info, old_nr_nugepages);
        if (rc != 0)
            return rc;

        rc = TE_ENOSPC;
    }

    return rc;
}

/* TODO: this function should be improved and moved to te_vector.h */
static te_errno
split_proc_mounts_line(char *str, te_vec *vec)
{
    char *token;
    char *saveptr;
    te_errno rc;

    te_vec_reset(vec);

    token = strtok_r(str, " ", &saveptr);
    while (token != NULL)
    {
        rc = te_vec_append_str_fmt(vec, "%s", token);
        if (rc != 0)
            break;

        token = strtok_r(NULL, " ", &saveptr);
    }

    return rc;
}

static te_errno
convert_mountpoint_name(const char *mountpoint, te_string *name, te_bool decode)
{
    te_errno rc;

    te_string_reset(name);

    rc = te_string_append(name, "%s", mountpoint);
    if (rc != 0)
        return rc;

    if (decode)
        rc = te_string_replace_all_substrings(name, "/", PATH_DELIMITER);
    else
        rc = te_string_replace_all_substrings(name, PATH_DELIMITER, "/");

    return rc;
}

static te_errno
mount_hugepage_dir(const struct hugepage_info *hp_info,
                   struct mountpoint_info *mp_info)
{
    te_string options = TE_STRING_INIT;
    te_errno rc;
    int rv;

    rv = mkdir(mp_info->name, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (rv != 0 && errno != EEXIST)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): Failed to create directory %s: %r", __FUNCTION__,
              mp_info->name, rc);
        return rc;
    }

    mp_info->is_created = (rv == 0) ? FALSE : TRUE;

    rc = te_string_append(&options, "pagesize=%ukB", hp_info->size);
    if (rc != 0)
        return rc;

    if (mount("hugetlbfs", mp_info->name, "hugetlbfs", 0, options.ptr) != 0)
    {
        rc = te_rc_os2te(errno);
        te_string_free(&options);
        ERROR("%s(): Failed to mount hugetlbfs: %r", __FUNCTION__, rc);
        return rc;
    }

    RING("Mount hugetlbfs on %s type hugetlbfs (%s)",
         mp_info->name, options.ptr);

    te_string_free(&options);

    return 0;
}

static struct mountpoint_info *
find_mountpoint_info(const struct hugepage_info *hp_info, const char *name)
{
    struct mountpoint_info *mp_info;

    if (hp_info == NULL)
        return NULL;

    LIST_FOREACH(mp_info, &hp_info->mount_dirs, link)
    {
        if (strcmp(name, mp_info->name) == 0)
            return mp_info;
    }

    return NULL;
}

static te_errno
add_mountpoint_info(struct hugepage_info *hp_info, const char *name,
                    te_bool do_mount)
{
    struct mountpoint_info *mp_info;
    te_errno rc;

    mp_info = TE_ALLOC(sizeof(*mp_info));
    if (mp_info == NULL)
        return TE_ENOMEM;

    mp_info->name = strdup(name);
    if (mp_info->name == NULL)
    {
        free(mp_info);
        return TE_ENOMEM;
    }

    mp_info->is_mounted = do_mount;
    mp_info->is_created = FALSE;

    if (do_mount)
    {
        rc = mount_hugepage_dir(hp_info, mp_info);
        if (rc != 0)
        {
            ERROR("%s(): Failed to mount %s: %r", __FUNCTION__,
                  mp_info->name, rc);
            free(mp_info->name);
            free(mp_info);
            return rc;
        }
    }

    LIST_INSERT_HEAD(&hp_info->mount_dirs, mp_info, link);

    return 0;
}

static te_errno
update_mountpoint_info(struct hugepage_info *hp_info, te_vec *mount_args)
{
    te_string target_option = TE_STRING_INIT;
    te_string encode_name = TE_STRING_INIT;
    char *option;
    char *name;
    struct mountpoint_info *mp_info;
    te_errno rc;

    name = TE_VEC_GET(char *, mount_args, 1);

    mp_info = find_mountpoint_info(hp_info, name);
    if (mp_info != NULL)
        return 0;

    rc = convert_mountpoint_name(name, &encode_name, FALSE);
    if (rc != 0)
    {
        ERROR("%s(): Cannot encode mountpoint name: %r", __FUNCTION__, rc);
        return rc;
    }

    if (!rcf_pch_rsrc_accessible("/agent:%s/mem:/hugepages:%u/mountpoint:%s",
                                 ta_name, hp_info->size, encode_name.ptr))
    {
        goto out;
    }

    option = TE_VEC_GET(char *, mount_args, 3);
    rc = te_string_append(&target_option, "pagesize=%uM", hp_info->size / 1024);
    if (rc != 0)
        goto out;

    if (strstr(option, target_option.ptr) == NULL)
        goto out;

    rc = add_mountpoint_info(hp_info, name, FALSE);
    if (rc != 0)
    {
        ERROR("%s(): Failed to add mountpoint info: %r",
              __FUNCTION__, rc);
    }

out:
    te_string_free(&encode_name);
    te_string_free(&target_option);

    return rc;
}

static te_errno
scan_mounts_file(struct hugepage_info *hp_info)
{
    FILE *file;
    const char *proc_mounts = "/proc/mounts";
    char *line = NULL;
    size_t len = 0;
    te_errno rc;
    te_vec mount_args = TE_VEC_INIT(char *);

    file = fopen(proc_mounts, "r");
    if (file == NULL)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): Failed to open %s: %r", __FUNCTION__, proc_mounts, rc);
        return rc;
    }

    while (getline(&line, &len, file) != -1)
    {
        rc = split_proc_mounts_line(line, &mount_args);
        if (rc != 0)
            break;

        if (strcmp(TE_VEC_GET(char *, &mount_args, 2), "hugetlbfs") != 0)
            continue;

        rc = update_mountpoint_info(hp_info, &mount_args);
        if (rc != 0)
            break;
    }

    fclose(file);
    free(line);
    te_vec_deep_free(&mount_args);

    return rc;
}

static te_errno
hugepages_list(unsigned int gid, const char *oid,
               const char *sub_id, char **list, ...)
{
    struct hugepage_info *hp_info;
    te_string buf = TE_STRING_INIT;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    LIST_FOREACH(hp_info, &hugepages, link)
    {
        rc = te_string_append(&buf, "%u ", hp_info->size);
        if (rc != 0)
            break;
    }

    if (rc == 0)
        *list = buf.ptr;
    else
        te_string_free(&buf);

    return rc;
}

static te_errno
hugepages_set(unsigned int gid, const char *oid, const char *value,
              const char *unused, const char *hugepage_size)
{
    struct hugepage_info *hp_info;
    long int number;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused);

    rc = find_hugepage_info(hugepage_size, &hp_info, FALSE);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    rc = te_strtol(value, 10, &number);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (number < 0)
    {
        ERROR("%s(): Number of hugepages should be a non-negative value",
              __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = alloc_hugepages(hp_info, number);
    if (rc != 0)
    {
        ERROR("%s(): Failed to allocate hugepages: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

static te_errno
hugepages_get(unsigned int gid, const char *oid, char *value,
              const char *unused, const char *hugepage_size)
{
    struct hugepage_info *hp_info;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused);

    rc = find_hugepage_info(hugepage_size, &hp_info, TRUE);
    if (rc != 0)
    {
        if (rc == TE_EPERM)
        {
            rc = te_snprintf(value, RCF_MAX_VAL, "-1");
            if (rc != 0)
                ERROR("%s(): Failed to write value: %r", __FUNCTION__, rc);
        }

        return TE_RC(TE_TA_UNIX, rc);
    }

    rc = scan_hugepage_info(hp_info);
    if (rc != 0)
    {
        ERROR("%s(): Failed to scan information about hugepage: %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = te_snprintf(value, RCF_MAX_VAL, "%u", hp_info->nr_hugepages);
    if (rc != 0)
    {
        ERROR("%s(): Failed to write value: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

static te_errno
hugepages_mountpoint_add(unsigned int gid, const char *oid, const char *value,
                         const char *unused, const char *hugepage_size,
                         const char *mountpoint)
{
    struct hugepage_info *hp_info;
    struct mountpoint_info *mp_info;
    te_string decode_name = TE_STRING_INIT;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused);
    UNUSED(value);

    rc = find_hugepage_info(hugepage_size, &hp_info, FALSE);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (!rcf_pch_rsrc_accessible("/agent:%s/mem:/hugepages:%u/mountpoint:%s",
                                 ta_name, hp_info->size, mountpoint))
    {
        ERROR("%s(): Failed to find the lock for %s", __FUNCTION__, mountpoint);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    rc = convert_mountpoint_name(mountpoint, &decode_name, TRUE);
    if (rc != 0)
    {
        ERROR("%s(): Failed to decode mountpoint name: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    rc = scan_mounts_file(hp_info);
    if (rc != 0)
    {
        ERROR("%s(): Failed to scan mounts file: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    mp_info = find_mountpoint_info(hp_info, decode_name.ptr);
    if (mp_info != NULL)
    {
        ERROR("%s(): The mountpoint %s already exists", __FUNCTION__,
              decode_name.ptr);
        te_string_free(&decode_name);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    rc = add_mountpoint_info(hp_info, decode_name.ptr, TRUE);
    if (rc != 0)
    {
        ERROR("%s(): Failed to add mountpoint info: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

static te_errno
hugepages_mountpoint_del(unsigned int gid, const char *oid, const char *unused,
                         const char *hugepage_size, const char *mountpoint)
{
    te_errno rc;
    struct hugepage_info *hp_info;
    struct mountpoint_info *mp_info;
    te_string decode_name = TE_STRING_INIT;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused);

    rc = find_hugepage_info(hugepage_size, &hp_info, FALSE);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    rc = convert_mountpoint_name(mountpoint, &decode_name, TRUE);
    if (rc != 0)
    {
        ERROR("%s(): Failed to decode mountpoint name: %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    mp_info = find_mountpoint_info(hp_info, decode_name.ptr);
    te_string_free(&decode_name);
    if (mp_info == NULL)
    {
        ERROR("%s(): Failed to find mountpoint %s", __FUNCTION__, mountpoint);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (mp_info->is_mounted && umount2(mp_info->name, MNT_DETACH) != 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): Failed to umount %s: %r", __FUNCTION__,
              mp_info->name, rc);
    }

    if (!mp_info->is_created && rmdir(mp_info->name) != 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): Failed to delete directory %s: %r", __FUNCTION__,
              mp_info->name, rc);
    }

    LIST_REMOVE(mp_info, link);

    free(mp_info->name);
    free(mp_info);

    return 0;
}

static te_errno
hugepages_mountpoint_list(unsigned int gid, const char *oid, const char *sub_id,
                          char **list, const char *unused,
                          const char *hugepage_size, ...)
{
    te_string result = TE_STRING_INIT;
    struct hugepage_info *hp_info;
    struct mountpoint_info *mp_info;
    te_errno rc;

    rc = find_hugepage_info(hugepage_size, &hp_info, TRUE);
    if (rc != 0)
    {
        if (rc == TE_EPERM)
            rc = 0;

        return TE_RC(TE_TA_UNIX, rc);
    }

    rc = scan_mounts_file(hp_info);
    if (rc != 0)
    {
        ERROR("Failed to scan mounts file: %r", rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    LIST_FOREACH(mp_info, &hp_info->mount_dirs, link)
    {
        rc = te_string_append(&result, "%s ", mp_info->name);
        if (rc != 0)
            break;
    }

    if (rc == 0)
    {
        rc = te_string_replace_all_substrings(&result, PATH_DELIMITER, "/");
        if (rc != 0)
            return rc;

        *list = result.ptr;
    }
    else
    {
        te_string_free(&result);
    }

    return rc;
}

te_errno
get_supported_hugepages_sizes(void)
{
    struct dirent **names;
    unsigned int hp_size;
    te_errno rc = 0;
    int n = 0;
    int i;

    n = scandir(SYS_HUGEPAGES, &names, NULL, alphasort);
    if (n <= 0)
    {
        if (errno == ENOENT)
            return 0;

        rc = te_rc_os2te(errno);
        ERROR("%s(): Cannot get a list of available hugepages size: %r",
              __FUNCTION__, rc);
        return rc;
    }

    for (i = 0; i < n; i++)
    {
        if (strcmp(names[i]->d_name, ".") == 0 ||
            strcmp(names[i]->d_name, "..") == 0)
        {
            continue;
        }

        if (sscanf(names[i]->d_name, "hugepages-%ukB", &hp_size) != 1)
        {
            rc = te_rc_os2te(errno);
            ERROR("%s(): Cannot parse available hugepages size: %r",
                  __FUNCTION__, rc);
            break;
        }

        rc = add_hugepage_info(hp_size);
        if (rc != 0)
            break;
    }

    for (i = 0; i < n; i++)
        free(names[i]);
    free(names);

    return rc;
}

static te_errno
hugepages_mountpoint_grab(const char *name)
{
    cfg_oid *oid = cfg_convert_oid_str(name);
    struct hugepage_info *hp_info;
    te_errno rc;

    if (oid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = find_hugepage_info(CFG_OID_GET_INST_NAME(oid, 3), &hp_info, FALSE);

    cfg_free_oid(oid);
    return TE_RC(TE_TA_UNIX, rc);
}

RCF_PCH_CFG_NODE_COLLECTION(node_hugepage_mountpoint, "mountpoint",
                            NULL, NULL,
                            hugepages_mountpoint_add, hugepages_mountpoint_del,
                            hugepages_mountpoint_list,
                            NULL);
RCF_PCH_CFG_NODE_RW_COLLECTION(node_hugepages, "hugepages",
                               &node_hugepage_mountpoint, NULL,
                               hugepages_get, hugepages_set, NULL, NULL,
                               hugepages_list,
                               NULL);
RCF_PCH_CFG_NODE_NA(node_mem, "mem", &node_hugepages, NULL);

te_errno
ta_unix_conf_memory_init(void)
{
    te_errno rc;

    rc = get_supported_hugepages_sizes();
    if (rc != 0)
        return rc;

    rc = rcf_pch_add_node("/agent", &node_mem);
    if (rc != 0)
        return rc;

    rc = rcf_pch_rsrc_info("/agent/mem/hugepages",
                           rcf_pch_rsrc_grab_dummy,
                           rcf_pch_rsrc_release_dummy);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/mem/hugepages/mountpoint",
                             hugepages_mountpoint_grab,
                             rcf_pch_rsrc_release_dummy);
}

te_errno
ta_unix_conf_memory_cleanup(void)
{
    free_hugepage_info_list();
    return 0;
}