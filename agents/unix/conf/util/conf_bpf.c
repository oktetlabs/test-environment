/** @file
 * @brief Unix Test Agent BPF support.
 *
 * Implementation of unix TA BPF configuring support.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Roman Zhukov <Roman.Zhukov@oktetlabs.ru>
 *
 * $Id:
 */

#define TE_LGR_USER     "Conf BPF"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <linux/if_link.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_string.h"
#include "te_str.h"
#include "te_alloc.h"
#include "cs_common.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "bpf.h"
#include "libbpf.h"

/* Max number of programs or maps in BPF object file */
#define BPF_MAX_ENTRIES 128

/** Structure for the BPF program description */
typedef struct bpf_prog_entry {
    int fd;
    char name[BPF_OBJ_NAME_LEN + 1];
} bpf_prog_entry;

/** Structure for the BPF map description */
typedef struct bpf_map_entry {
    int fd;
    char name[BPF_OBJ_NAME_LEN + 1];
    enum bpf_map_type type;
    unsigned int key_size;
    unsigned int value_size;
    unsigned int max_entries;
    te_bool writable;
} bpf_map_entry;

enum bpf_object_state {
    BPF_STATE_UNLOADED = 0,     /**< BPF object isn't loaded into the kernel */
    BPF_STATE_LOADED,           /**< BPF object is loaded into the kernel */
} bpf_object_state;

/** List of the BPF object descriptions */
typedef struct bpf_entry {
    LIST_ENTRY(bpf_entry) next;

    unsigned int            id;
    enum bpf_object_state   state;
    char                    filepath[RCF_MAX_PATH];
    enum bpf_prog_type      prog_type;
    struct bpf_object      *obj;
    unsigned int            prog_number;
    struct bpf_prog_entry   progs[BPF_MAX_ENTRIES];
    unsigned int            map_number;
    struct bpf_map_entry    maps[BPF_MAX_ENTRIES];
} bpf_entry;

/**< Head of the BPF objects list */
static LIST_HEAD(, bpf_entry) bpf_list_h = LIST_HEAD_INITIALIZER(bpf_list_h);

/** Check that BPF map type is supported */
static te_bool
bpf_map_supported(enum bpf_map_type type)
{
    return (type == BPF_MAP_TYPE_ARRAY ||
            type == BPF_MAP_TYPE_HASH  ||
            type == BPF_MAP_TYPE_LPM_TRIE);
}

/**
 * Searching for the BPF object by object id.
 *
 * @param bpf_id        The BPF object id.
 *
 * @return The BPF object unit or NULL.
 */
static struct bpf_entry *
bpf_find(const char *bpf_id)
{
    struct bpf_entry *p;
    unsigned int id;

    if (*bpf_id == '\0' || te_strtoui(bpf_id, 0, &id) != 0)
        return NULL;

    LIST_FOREACH(p, &bpf_list_h, next)
    {
        if (id == p->id)
            return p;
    }

    return NULL;
}

/**
 * Allocation and default initialization of the BPF object description.
 *
 * @param bpf_id        The BPF object id.
 * @param[out] bpf      The BPF object pointer.
 *
 * @return Status code.
 */
static te_errno
bpf_init(const char *bpf_id, struct bpf_entry **bpf)
{
    struct bpf_entry *result;
    unsigned int id;

    if (*bpf_id == '\0' || te_strtoui(bpf_id, 0, &id) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        ERROR("Failed to allocate a BPF entry");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    result->id = id;
    result->state = BPF_STATE_UNLOADED;
    result->prog_type = BPF_PROG_TYPE_UNSPEC;
    result->obj = NULL;
    memset(result->filepath, 0, sizeof(result->filepath));
    result->prog_number = 0;
    result->map_number = 0;

    *bpf = result;

    return 0;
}

/**
 * Initialize information of the loaded BPF program.
 *
 * @param prog              The BPF program.
 * @param[out] prog_info    The BPF program info.
 *
 * @return Status code.
 */
static te_errno
bpf_init_prog_info(struct bpf_program *prog, struct bpf_prog_entry *prog_info)
{
    int fd;
    struct bpf_prog_info info = {};
    unsigned int info_len = sizeof(info);

    fd = bpf_program__fd(prog);
    if (fd <= 0)
    {
        ERROR("Failed to get fd of loaded BPF program.");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if (bpf_obj_get_info_by_fd(fd, &info, &info_len) != 0)
    {
        ERROR("Failed to get info about loaded BPF program.");
        return TE_RC(TE_TA_UNIX, TE_EBADFD);
    }

    prog_info->fd = fd;
    strncpy(prog_info->name, info.name, sizeof(prog_info->name) - 1);
    prog_info->name[sizeof(prog_info->name) - 1] = '\0';

    return 0;
}

/**
 * Initialize information of the loaded BPF map.
 *
 * @param map           The BPF map.
 * @param[out] map_info The BPF map info.
 *
 * @return Status code.
 */
static te_errno
bpf_init_map_info(struct bpf_map *map, struct bpf_map_entry *map_info)
{
    int fd;
    const struct bpf_map_def *def;

    fd = bpf_map__fd(map);
    if (fd <= 0)
    {
        ERROR("Failed to get fd of loaded BPF map.");
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    def = bpf_map__def(map);
    if (!bpf_map_supported(def->type))
    {
        ERROR("Loaded BPF map isn't supported.");
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }

    map_info->fd = fd;
    map_info->type = def->type;
    map_info->key_size = def->key_size;
    map_info->value_size = def->value_size;
    map_info->max_entries = def->max_entries;
    strncpy(map_info->name, bpf_map__name(map), sizeof(map_info->name) - 1);
    map_info->name[sizeof(map_info->name) - 1] = '\0';
    map_info->writable = FALSE;

    return 0;
}

/**
 * Load the BPF object into the kernel and get information about all loaded
 * maps and programs from this BPF object.
 *
 * @param bpf           The BPF object pointer.
 *
 * @return Status code.
 */
static te_errno
bpf_load(struct bpf_entry *bpf)
{
    int prog_fd;
    struct bpf_program *prog;
    struct bpf_map *map;
    unsigned int prog_number = 0;
    unsigned int map_number = 0;
    int rc = 0;

    if (bpf->state == BPF_STATE_LOADED)
        return 0;

    rc = bpf_prog_load(bpf->filepath, bpf->prog_type, &bpf->obj, &prog_fd);
    if (rc != 0)
    {
        ERROR("BPF object file cannot be loaded.");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    bpf_object__for_each_program(prog, bpf->obj)
    {
        if (prog_number == BPF_MAX_ENTRIES)
        {
            ERROR("Number of BPF programs in object file is too big.");
            return TE_RC(TE_TA_UNIX, TE_EFBIG);
        }

        rc = bpf_init_prog_info(prog, &bpf->progs[prog_number]);
        if (rc != 0)
            return rc;

        prog_number++;
    }

    bpf_object__for_each_map(map, bpf->obj)
    {
        if (map_number == BPF_MAX_ENTRIES)
        {
            ERROR("Number of BPF maps in object file is too big.");
            return TE_RC(TE_TA_UNIX, TE_EFBIG);
        }

        rc = bpf_init_map_info(map, &bpf->maps[map_number]);
        if (rc != 0)
            return rc;

        map_number++;
    }

    bpf->prog_number = prog_number;
    bpf->map_number = map_number;
    bpf->state = BPF_STATE_LOADED;
    return 0;
}

/**
 * Unload the BPF object from the kernel.
 *
 * @param bpf           The BPF object pointer.
 */
static void
bpf_unload(struct bpf_entry *bpf)
{
    if (bpf->state == BPF_STATE_UNLOADED)
        return;

    bpf_object__close(bpf->obj);

    bpf->prog_number = 0;
    bpf->map_number = 0;
    bpf->state = BPF_STATE_UNLOADED;
    return;
}

/**
 * Add a new BPF object.
 *
 * @param gid       Request's group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param value     New value (unused).
 * @param bpf_id    BPF object id.
 *
 * @return Status code.
 */
static te_errno
bpf_add(unsigned int gid, const char *oid, const char *value,
        const char *bpf_id)
{
    struct bpf_entry *bpf;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (bpf_find(bpf_id) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    rc = bpf_init(bpf_id, &bpf);
    if (rc != 0)
        return rc;

    LIST_INSERT_HEAD(&bpf_list_h, bpf, next);

    return 0;
}

/**
 * Delete the BPF object.
 *
 * @param gid       Request's group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param bpf_id    BPF object id.
 *
 * @return Status code.
 */
static te_errno
bpf_del(unsigned int gid, const char *oid, const char *bpf_id)
{
    struct bpf_entry *bpf;

    UNUSED(gid);
    UNUSED(oid);

    bpf = bpf_find(bpf_id);
    if (bpf == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    bpf_unload(bpf);

    LIST_REMOVE(bpf, next);
    free(bpf);

    return 0;
}

/**
 * Get instance list of the BPF objects for object "/agent/bpf".
 *
 * @param gid       Request's group identifier (unused).
 * @param oid       Full parent object instance identifier (unused).
 * @param sub_id    ID of the object to be listed (unused).
 * @param list      Location for the list pointer.
 *
 * @return Status code.
 */
static te_errno
bpf_list(unsigned int gid, const char *oid, const char *sub_id,
         char **list)
{
    te_string result = TE_STRING_INIT;
    te_bool first = TRUE;
    bpf_entry *bpf;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    LIST_FOREACH(bpf, &bpf_list_h, next)
    {
        rc = te_string_append(&result, "%s%u", first ? "" : " ", bpf->id);
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

/**
 * Get list of loaded programs or maps from BPF object.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full identifier of the father instance (unused).
 * @param sub_id    ID of the object to be listed.
 * @param list      Location for the list pointer.
 * @param bpf_id    BPF object id.
 *
 * @return      Status code
 */
static te_errno
bpf_prog_map_list(unsigned int gid, const char *oid, const char *sub_id,
                  char **list,  const char *bpf_id)
{
    te_string result = TE_STRING_INIT;
    te_bool first = TRUE;
    te_bool is_map = FALSE;
    bpf_entry *bpf;
    unsigned int i;
    unsigned int entries_nb = 0;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    bpf = bpf_find(bpf_id);
    if (bpf == NULL)
    {
        te_string_free(&result);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (strcmp(sub_id, "program") == 0)
    {
        entries_nb = bpf->prog_number;
    }
    else if (strcmp(sub_id, "map") == 0)
    {
        is_map = TRUE;
        entries_nb = bpf->map_number;
    }

    for (i = 0; i < entries_nb; i++)
    {
        rc = te_string_append(&result, "%s%s", first ? "" : " ",
                              is_map ? bpf->maps[i].name : bpf->progs[i].name);
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

/* String representations of BPF program types */
static const char* bpf_prog_types_str[] = {
    [BPF_PROG_TYPE_UNSPEC] = "UNSPEC",
    [BPF_PROG_TYPE_SOCKET_FILTER] = "SOCKET_FILTER",
    [BPF_PROG_TYPE_KPROBE] = "KPROBE",
    [BPF_PROG_TYPE_SCHED_CLS] = "SCHED_CLS",
    [BPF_PROG_TYPE_SCHED_ACT] = "SCHED_ACT",
    [BPF_PROG_TYPE_TRACEPOINT] = "TRACEPOINT",
    [BPF_PROG_TYPE_XDP] = "XDP",
    [BPF_PROG_TYPE_PERF_EVENT] = "PERF_EVENT",
};

/** Convert from string to BPF program type */
static te_errno
bpf_type_str2val(const char *prog_type_str, enum bpf_prog_type *prog_type_val)
{
    enum bpf_prog_type result = BPF_PROG_TYPE_UNSPEC;
    te_errno rc;

    if (prog_type_str == NULL || *prog_type_str == '\0')
    {
        *prog_type_val = result;
        return 0;
    }

    rc = te_str_find_index(prog_type_str, bpf_prog_types_str,
                           TE_ARRAY_LEN(bpf_prog_types_str), &result);
    if (rc != 0)
    {
       ERROR("Wrong BPF type value");
       return TE_RC(TE_TA_UNIX, rc);
    }

    *prog_type_val = result;
    return 0;
}

/* String representations of BPF object states */
static const char* bpf_states_str[] = {
    [BPF_STATE_UNLOADED] = "unloaded",
    [BPF_STATE_LOADED] = "loaded",
};

/** Convert from string to BPF object state */
static te_errno
bpf_state_str2val(const char *state_str, unsigned int *state_val)
{
    unsigned int result = BPF_STATE_UNLOADED;
    te_errno rc;

    if (state_str == NULL || *state_str == '\0')
    {
        ERROR("BPF state isn't specified");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = te_str_find_index(state_str, bpf_states_str,
                           TE_ARRAY_LEN(bpf_states_str), &result);
    if (rc != 0)
    {
       ERROR("Wrong BPF state value");
       return TE_RC(TE_TA_UNIX, rc);
    }

    *state_val = result;
    return 0;
}

/**
 * Common get function for the BPF object parameters.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier.
 * @param[out] value    Obtained value.
 * @param bpf_id        BPF object id.
 *
 * @return Status code.
 */
static te_errno
bpf_get_params(unsigned int gid, const char *oid, char *value,
               const char *bpf_id)
{
    struct bpf_entry *bpf;

    UNUSED(gid);

    bpf = bpf_find(bpf_id);
    if (bpf == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strstr(oid, "/filepath:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%s", bpf->filepath);
    else if ((strstr(oid, "/type:") != NULL))
        snprintf(value, RCF_MAX_VAL, "%s", bpf_prog_types_str[bpf->prog_type]);
    else if ((strstr(oid, "/state:") != NULL))
        snprintf(value, RCF_MAX_VAL, "%s", bpf_states_str[bpf->state]);

    return 0;
}

/**
 * Set function for the BPF object filepath.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier.
 * @param value     New value.
 * @param bpf_id    BPF object id.
 *
 * @return Status code.
 */
static te_errno
bpf_set_filepath(unsigned int gid, const char *oid, const char *value,
                 const char *bpf_id)
{
    struct bpf_entry *bpf;

    UNUSED(gid);
    UNUSED(oid);

    bpf = bpf_find(bpf_id);
    if (bpf == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (bpf->state != BPF_STATE_UNLOADED)
    {
        ERROR("Filepath can be changed only in unloaded state.");
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    if (access(value, F_OK) < 0)
    {
        ERROR("BPF object file doesn't exist.");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    strncpy(bpf->filepath, value, sizeof(bpf->filepath));

    return 0;
}

/**
 * Set function for the BPF object type.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier.
 * @param value     New value.
 * @param bpf_id    BPF object id.
 *
 * @return Status code.
 */
static te_errno
bpf_set_type(unsigned int gid, const char *oid, const char *value,
             const char *bpf_id)
{
    struct bpf_entry *bpf;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    bpf = bpf_find(bpf_id);
    if (bpf == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (bpf->state != BPF_STATE_UNLOADED)
    {
        ERROR("Type can be changed only in unloaded state.");
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    rc = bpf_type_str2val(value, &bpf->prog_type);
    if (rc != 0)
        return rc;

    return 0;
}

/**
 * Set function for the BPF object state.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier.
 * @param value     New value.
 * @param bpf_id    BPF object id.
 *
 * @return Status code.
 */
static te_errno
bpf_set_state(unsigned int gid, const char *oid, const char *value,
              const char *bpf_id)
{
    struct bpf_entry *bpf;
    te_errno rc = 0;
    enum bpf_object_state state_to;

    UNUSED(gid);
    UNUSED(oid);

    bpf = bpf_find(bpf_id);
    if (bpf == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = bpf_state_str2val(value, &state_to);
    if (rc != 0)
        return rc;

    if (state_to == BPF_STATE_LOADED)
    {
        rc = bpf_load(bpf);
        if (rc != 0)
            return rc;
    }
    else if (state_to == BPF_STATE_UNLOADED)
    {
        bpf_unload(bpf);
    }

    return 0;
}

/**
 * Searching for the BPF map by object id and map name.
 *
 * @param bpf_id        The BPF object id.
 * @param bpf_id        The map name.
 *
 * @return The BPF map unit or NULL.
 */
static bpf_map_entry *
bpf_find_map(const char *bpf_id, const char *map_name)
{
    unsigned int i;
    struct bpf_entry *bpf;

    bpf = bpf_find(bpf_id);
    if (bpf == NULL)
        return NULL;

    for (i = 0; i < bpf->map_number; i++)
    {
        if (strcmp(map_name, bpf->maps[i].name) == 0)
            return &bpf->maps[i];
    }

    return NULL;
}

/* String representations of BPF map types */
static const char* bpf_map_types_str[] = {
    [BPF_MAP_TYPE_UNSPEC] = "UNSPEC",
    [BPF_MAP_TYPE_HASH] = "HASH",
    [BPF_MAP_TYPE_ARRAY] = "ARRAY",
    [BPF_MAP_TYPE_LPM_TRIE] = "LPM_TRIE",
};

/**
 * Common get function for the BPF map parameters.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier.
 * @param[out] value    Obtained value.
 * @param bpf_id        BPF object id.
 * @param map_name      Map name.
 *
 * @return Status code.
 */
static te_errno
bpf_get_map_params(unsigned int gid, const char *oid, char *value,
                   const char *bpf_id, const char *map_name)
{
    struct bpf_map_entry *map;

    UNUSED(gid);

    map = bpf_find_map(bpf_id, map_name);
    if (map == NULL)
    {
        ERROR("Map %s isn't found.", map_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (strstr(oid, "/type:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%s", bpf_map_types_str[map->type]);
    else if ((strstr(oid, "/key_size:") != NULL))
        snprintf(value, RCF_MAX_VAL, "%u", map->key_size);
    else if ((strstr(oid, "/value_size:") != NULL))
        snprintf(value, RCF_MAX_VAL, "%u", map->value_size);
    else if ((strstr(oid, "/max_entries:") != NULL))
        snprintf(value, RCF_MAX_VAL, "%u", map->max_entries);
    else if ((strstr(oid, "/writable:") != NULL))
        snprintf(value, RCF_MAX_VAL, "%d", map->writable ? 1 : 0);

    return 0;
}

/**
 * Get the value by key from BPF map.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier.
 * @param[out] value    Obtained value.
 * @param bpf_id        BPF object id.
 * @param map_name      Map name.
 * @param view          View of the map (unused).
 * @param key_str       Key from the map.
 *
 * @return Status code.
 */
static te_errno
bpf_get_map_kv_pair(unsigned int gid, const char *oid, char *value_str,
                    const char *bpf_id, const char *map_name, const char *view,
                    const char *key_str)
{
    struct bpf_map_entry *map;
    unsigned char *key, *value;
    te_string value_buf = { value_str, RCF_MAX_VAL, 0, TRUE };
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(view);

    map = bpf_find_map(bpf_id, map_name);
    if (map == NULL)
    {
        ERROR("Map isn't found.");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    key = TE_ALLOC(map->key_size);
    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    value = TE_ALLOC(map->value_size);
    if (value == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto fail_free_key;
    }

    rc = te_str_hex_str2raw(key_str, key, map->key_size);
    if (rc != 0)
        goto fail_free_key_value;

    if (bpf_map_lookup_elem(map->fd, key, value) != 0)
    {
        ERROR("Failed to lookup element.");
        rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
        goto fail_free_key_value;
    }

    rc = te_str_hex_raw2str(value, map->value_size, &value_buf);

fail_free_key_value:
    free(value);

fail_free_key:
    free(key);
    return rc;
}

/**
 * Get list of keys from BPF map.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full identifier of the father instance.
 * @param sub_id        ID of the object to be listed (unused).
 * @param list          Location for the list pointer.
 * @param bpf_id        BPF object id.
 * @param map_name      Map name.
 *
 * @return      Status code
 */
static te_errno
bpf_list_map_kv_pair(unsigned int gid, const char *oid, const char *sub_id,
                     char **list, const char *bpf_id, const char *map_name)
{
    te_string result = TE_STRING_INIT;
    te_bool first = TRUE;
    struct bpf_map_entry *map;
    void *key;
    void *prev_key = NULL;
    te_errno rc = 0;
    unsigned int i;

    UNUSED(gid);
    UNUSED(sub_id);

    map = bpf_find_map(bpf_id, map_name);
    if (map == NULL)
    {
        ERROR("Map isn't found.");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if ((strstr(oid, "/writable:") != NULL) && !map->writable)
    {
        *list = result.ptr;
        return 0;
    }

    key = TE_ALLOC(map->key_size);
    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    prev_key = TE_ALLOC(map->key_size);
    if (prev_key == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto fail;
    }

    i = 0;
    while (bpf_map_get_next_key(map->fd, (i == 0) ? NULL : prev_key, key) != -1)
    {
        te_string buf_str = TE_STRING_INIT_STATIC(RCF_MAX_VAL);

        rc = te_str_hex_raw2str((unsigned char *)key, map->key_size, &buf_str);
        if (rc != 0)
            goto fail;

        rc = te_string_append(&result, "%s%s", first ? "" : " ", buf_str.ptr);
        first = FALSE;
        if (rc != 0)
            goto fail;

        memcpy(prev_key, key, map->key_size);

        if (++i > map->max_entries)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EOVERFLOW);
            goto fail;
        }
    }

    free(key);
    free(prev_key);
    *list = result.ptr;
    return 0;

fail:
    te_string_free(&result);
    free(key);
    free(prev_key);
    return rc;
}

/**
 * Set the writable map view. If writable map view is enabled then the map can
 * be modified by user but it must not be modified by BPF program.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier (unused).
 * @param value         New value.
 * @param bpf_id        BPF object id.
 * @param map_name      Map name.
 *
 * @return Status code.
 */
static te_errno
bpf_set_map_writable(unsigned int gid, const char *oid, const char *value,
                     const char *bpf_id, const char *map_name)
{
    struct bpf_map_entry *map;
    unsigned int state;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    map = bpf_find_map(bpf_id, map_name);
    if (map == NULL)
    {
        ERROR("Map isn't found.");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = te_strtoui(value, 0, &state);
    if (rc != 0)
        return rc;

    map->writable = state;

    return 0;
}

/**
 * Delete key/value pair from the BPF map. For ARRAY map it removes only value
 * because keys are just indexes and can not be added or deleted.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier (unused).
 * @param bpf_id        BPF object id.
 * @param map_name      Map name.
 * @param view          View of the map (unused).
 * @param key_str       Key from the map.
 *
 * @return Status code.
 */
static te_errno
bpf_del_map_writable_kv_pair(unsigned int gid, const char *oid,
                             const char *bpf_id, const char *map_name,
                             const char *view, const char *key_str)
{
    struct bpf_map_entry *map;
    unsigned char *key;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(view);

    map = bpf_find_map(bpf_id, map_name);
    if (map == NULL)
    {
        ERROR("Map isn't found.");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    key = TE_ALLOC(map->key_size);
    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    rc = te_str_hex_str2raw(key_str, key, map->key_size);
    if (rc != 0)
        goto free_key;

    if (map->type == BPF_MAP_TYPE_ARRAY)
    {
        unsigned char *value;

        value = TE_ALLOC(map->value_size);
        if (value == NULL)
            rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);

        memset(value, 0, map->value_size);

        if (bpf_map_update_elem(map->fd, key, value, BPF_ANY) != 0)
        {
            ERROR("Failed to delete element of ARRAY map.");
            rc = TE_RC(TE_TA_UNIX, TE_ENXIO);
        }

        free(value);
        goto free_key;
    }

    if (bpf_map_delete_elem(map->fd, key) != 0)
    {
        ERROR("Failed to delete element.");
        rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

free_key:
    free(key);
    return rc;
}

/**
 * Update existing key/value pair in the BPF map or add new one.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier (unused).
 * @param value_str     New value.
 * @param bpf_id        BPF object id.
 * @param map_name      Map name.
 * @param view          View of the map (unused).
 * @param key_str       Key.
 *
 * @return Status code.
 */
static te_errno
bpf_update_map_writable_kv_pair(unsigned int gid, const char *oid,
                                const char *value_str, const char *bpf_id,
                                const char *map_name, const char *view,
                                const char *key_str)
{
    struct bpf_map_entry *map;
    unsigned char *key, *value;
    unsigned int update_flags = BPF_ANY;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(view);

    if (value_str == NULL)
    {
        ERROR("Value should be specified to update key/value pair in the map.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    map = bpf_find_map(bpf_id, map_name);
    if (map == NULL)
    {
        ERROR("Map isn't found.");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (!map->writable)
    {
        ERROR("Key/value pair can be added only to writable map.");
        return TE_RC(TE_TA_UNIX, TE_EACCES);
    }

    key = TE_ALLOC(map->key_size);
    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    rc = te_str_hex_str2raw(key_str, key, map->key_size);
    if (rc != 0)
        goto fail_free_key;

    value = TE_ALLOC(map->value_size);
    if (value == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto fail_free_key;
    }

    rc = te_str_hex_str2raw(value_str, value, map->value_size);
    if (rc != 0)
        goto fail_free_key_value;

    if (bpf_map_update_elem(map->fd, key, value, update_flags) != 0)
    {
        ERROR("Failed to update element.");
        rc = TE_RC(TE_TA_UNIX, TE_ENXIO);
        goto fail_free_key_value;
    }

fail_free_key_value:
    free(value);

fail_free_key:
    free(key);
    return rc;
}

/*
 * Test Agent /bpf configuration subtree.
 */
RCF_PCH_CFG_NODE_RW_COLLECTION(node_bpf_map_writable_key, "key", NULL,
                               NULL, bpf_get_map_kv_pair,
                               bpf_update_map_writable_kv_pair,
                               bpf_update_map_writable_kv_pair,
                               bpf_del_map_writable_kv_pair,
                               bpf_list_map_kv_pair, NULL);

RCF_PCH_CFG_NODE_RW(node_bpf_map_writable, "writable",
                    &node_bpf_map_writable_key, NULL, bpf_get_map_params,
                    bpf_set_map_writable);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_bpf_map_ro_key, "key", NULL,
                               NULL, bpf_get_map_kv_pair, bpf_list_map_kv_pair);

RCF_PCH_CFG_NODE_NA(node_bpf_map_ro, "read_only", &node_bpf_map_ro_key,
                    &node_bpf_map_writable);

RCF_PCH_CFG_NODE_RO(node_bpf_map_max_entries, "max_entries", NULL,
                    &node_bpf_map_ro, bpf_get_map_params);

RCF_PCH_CFG_NODE_RO(node_bpf_map_value_size, "value_size", NULL,
                    &node_bpf_map_max_entries, bpf_get_map_params);

RCF_PCH_CFG_NODE_RO(node_bpf_map_key_size, "key_size", NULL,
                    &node_bpf_map_value_size, bpf_get_map_params);

RCF_PCH_CFG_NODE_RO(node_bpf_map_type, "type", NULL,
                    &node_bpf_map_key_size, bpf_get_map_params);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_bpf_map, "map", &node_bpf_map_type,
                               NULL, NULL, bpf_prog_map_list);

RCF_PCH_CFG_NODE_RO_COLLECTION(node_bpf_prog, "program", NULL,
                               &node_bpf_map, NULL, bpf_prog_map_list);

RCF_PCH_CFG_NODE_RW(node_bpf_state, "state", NULL,
                    &node_bpf_prog, bpf_get_params, bpf_set_state);

RCF_PCH_CFG_NODE_RW(node_bpf_type, "type", NULL,
                    &node_bpf_state, bpf_get_params, bpf_set_type);

RCF_PCH_CFG_NODE_RW(node_bpf_filepath, "filepath", NULL,
                    &node_bpf_type, bpf_get_params, bpf_set_filepath);

RCF_PCH_CFG_NODE_COLLECTION(node_bpf, "bpf",
                            &node_bpf_filepath, NULL,
                            bpf_add, bpf_del, bpf_list, NULL);

/**
 * Initialization of the BPF configuration subtrees.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_bpf_init(void)
{
    return rcf_pch_add_node("/agent/", &node_bpf);
}

/**
 * Cleanup BPF function.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_bpf_cleanup(void)
{
    struct bpf_entry *bpf;
    struct bpf_entry *tmp;

    LIST_FOREACH_SAFE(bpf, &bpf_list_h, next, tmp)
    {
        bpf_unload(bpf);

        LIST_REMOVE(bpf, next);
        free(bpf);
    }

    return 0;
}

/** Structure for the linkage information of XDP programs */
typedef struct xdp_entry {
    LIST_ENTRY(xdp_entry) next;

    unsigned int            ifindex;
    unsigned int            bpf_id;
    char                    prog[BPF_OBJ_NAME_LEN + 1];
} xdp_entry;

/**< Head of the XDP programs list */
static LIST_HEAD(, xdp_entry) xdp_list_h = LIST_HEAD_INITIALIZER(xdp_list_h);

/** Structure of BPF program OID: /agent:Agt_A/bpf:0/program:foo */
/* Number of levels in BPF program OID */
#define BPF_PROG_OID_LEVELS 4
/* Level of BPF object ID in program OID */
#define BPF_PROG_OID_LEVEL_OBJ_ID 2
/* Level of BPF program name in OID */
#define BPF_PROG_OID_LEVEL_NAME 3

/**
 * Searching for the XDP program linkage information by interface index.
 *
 * @param ifindex       Interface index.
 *
 * @return The XDP program linkage information unit or NULL.
 */
static struct xdp_entry *
xdp_find(unsigned int ifindex)
{
    struct xdp_entry *p;

    LIST_FOREACH(p, &xdp_list_h, next)
    {
        if (ifindex == p->ifindex)
            return p;
    }

    return NULL;
}

/**
 * Get oid of the XDP program that is linked to the interface.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier (unused).
 * @param[out] value    Obtained value.
 * @param ifname        Interface name.
 *
 * @return Status code.
 */
static te_errno
bpf_get_if_xdp(unsigned int gid, const char *oid, char *value,
               const char *ifname)
{
    struct xdp_entry *xdp;
    unsigned int ifindex = if_nametoindex(ifname);

    UNUSED(gid);
    UNUSED(oid);

    xdp = xdp_find(ifindex);
    if (xdp == NULL)
    {
        *value = '\0';
        return 0;
    }

    snprintf(value, RCF_MAX_VAL, "/agent:%s/bpf:%u/program:%s", ta_name, xdp->bpf_id,
             xdp->prog);
    return 0;
}

/**
 * Searching for the BPF program by object id and program name.
 *
 * @param bpf_id        The BPF object id.
 * @param prog_name     The program name.
 *
 * @return The BPF program unit or NULL.
 */
static bpf_prog_entry *
bpf_find_prog(const char *bpf_id, const char *prog_name)
{
    unsigned int i;
    struct bpf_entry *bpf;

    bpf = bpf_find(bpf_id);
    if (bpf == NULL)
        return NULL;

    for (i = 0; i < bpf->prog_number; i++)
    {
        if (strcmp(prog_name, bpf->progs[i].name) == 0)
            return &bpf->progs[i];
    }

    return NULL;
}

/**
 * Add new xdp entry to list and link XDP program to interface.
 *
 * @param oid           OID of XDP program.
 * @param ifindex       Interface index.
 * @param xdp_flags     XDP flags for link.
 *
 * @return Status code.
 */
static te_errno
bpf_add_and_link_xdp(cfg_oid *prog_oid, unsigned int ifindex,
                     unsigned int xdp_flags)
{
    struct bpf_entry *bpf;
    struct bpf_prog_entry *prog;
    char *bpf_id_str;
    struct xdp_entry *xdp;
    te_errno rc = 0;

    if (prog_oid == NULL || !prog_oid->inst ||
        prog_oid->len != BPF_PROG_OID_LEVELS)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bpf_id_str = CFG_OID_GET_INST_NAME(prog_oid, BPF_PROG_OID_LEVEL_OBJ_ID);

    bpf = bpf_find(bpf_id_str);
    if (bpf == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (bpf->prog_type != BPF_PROG_TYPE_XDP)
    {
        ERROR("Only XDP programs can be linked to interface.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    xdp = TE_ALLOC(sizeof(*xdp));
    if (xdp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    xdp->ifindex = ifindex;
    rc = te_strtoui(bpf_id_str, 0, &xdp->bpf_id);
    if (rc != 0)
        goto fail;
    strncpy(xdp->prog, CFG_OID_GET_INST_NAME(prog_oid, BPF_PROG_OID_LEVEL_NAME),
            sizeof(xdp->prog) - 1);
    xdp->prog[sizeof(xdp->prog) - 1] = '\0';


    prog = bpf_find_prog(bpf_id_str, xdp->prog);
    if (prog == NULL)
    {
        rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
        goto fail;
    }

    if (bpf_set_link_xdp_fd(ifindex, prog->fd, xdp_flags) != 0)
    {
        ERROR("Failed to link XDP program.");
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto fail;
    }

    LIST_INSERT_HEAD(&xdp_list_h, xdp, next);
    return 0;

fail:
    free(xdp);
    return rc;
}

/**
 * Set oid of the XDP program that should be linked to the interface. Calls to
 * link/unlink XDP programs.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier (unused).
 * @param value         New value.
 * @param ifname        Interface name.
 *
 * @return Status code.
 */
static te_errno
bpf_set_if_xdp(unsigned int gid, const char *oid, const char *value,
               const char *ifname)
{
    struct xdp_entry *xdp_old = NULL;
    unsigned int ifindex = if_nametoindex(ifname);
    unsigned int xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (!rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
        return TE_RC(TE_TA_UNIX, TE_ENODEV);

    xdp_old = xdp_find(ifindex);

    if (*value == '\0')
    {
        if (bpf_set_link_xdp_fd(ifindex, -1, xdp_flags) != 0)
        {
            ERROR("Failed to unlink XDP program.");
            return TE_RC(TE_TA_UNIX, TE_EFAIL);
        }
    }
    else
    {
        rc = bpf_add_and_link_xdp(cfg_convert_oid_str(value), ifindex,
                                  xdp_flags);
        if (rc != 0)
            return rc;
    }

    if (xdp_old != NULL)
    {
        LIST_REMOVE(xdp_old, next);
        free(xdp_old);
    }

    return 0;
}

/*
 * Test Agent /xdp configuration subtree.
 */
RCF_PCH_CFG_NODE_RW(node_if_xdp, "xdp", NULL,
                    NULL, bpf_get_if_xdp, bpf_set_if_xdp);

/**
 * Initialization of the XDP configuration subtrees.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_if_xdp_init(void)
{
    return rcf_pch_add_node("/agent/interface/", &node_if_xdp);
}

/**
 * Cleanup XDP function. Unlink all XDP programs from interfaces.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_if_xdp_cleanup(void)
{
    struct xdp_entry *xdp;
    struct xdp_entry *tmp;

    LIST_FOREACH_SAFE(xdp, &xdp_list_h, next, tmp)
    {
        if (bpf_set_link_xdp_fd(xdp->ifindex, -1,
                                XDP_FLAGS_UPDATE_IF_NOEXIST) != 0)
        {
            ERROR("Failed to unlink XDP program.");
            return TE_RC(TE_TA_UNIX, TE_EFAIL);
        }

        LIST_REMOVE(xdp, next);
        free(xdp);
    }

    return 0;
}
