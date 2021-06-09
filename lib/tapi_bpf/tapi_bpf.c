/** @file
 * @brief Test API to configure BPF/XDP programs.
 *
 * Test API to configure BPF/XDP programs (implementation).
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Damir Mansurov <Damir.Mansurov@oktetlabs.ru>
 */

#include "te_config.h"
#include "te_string.h"
#include "te_str.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "rcf_common.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_mem.h"
#include "tapi_bpf.h"
#include "tapi_cfg_qdisc.h"

static const char *tapi_bpf_states[] =
{
    [TAPI_BPF_STATE_UNLOADED] =          "unloaded",
    [TAPI_BPF_STATE_LOADED] =            "loaded"
};

static const char *tapi_bpf_prog_types[] =
{
    [TAPI_BPF_PROG_TYPE_UNSPEC] =        "UNSPEC",
    [TAPI_BPF_PROG_TYPE_SOCKET_FILTER] = "SOCKET_FILTER",
    [TAPI_BPF_PROG_TYPE_KPROBE] =        "KPROBE",
    [TAPI_BPF_PROG_TYPE_SCHED_CLS] =     "SCHED_CLS",
    [TAPI_BPF_PROG_TYPE_SCHED_ACT] =     "SCHED_ACT",
    [TAPI_BPF_PROG_TYPE_TRACEPOINT] =    "TRACEPOINT",
    [TAPI_BPF_PROG_TYPE_XDP] =           "XDP",
    [TAPI_BPF_PROG_TYPE_PERF_EVENT] =    "PERF_EVENT",
    [TAPI_BPF_PROG_TYPE_UNKNOWN] =       "<UNKNOWN>"
};

static const char *tapi_bpf_map_types[] =
{
    [TAPI_BPF_MAP_TYPE_UNSPEC] =              "UNSPEC",
    [TAPI_BPF_MAP_TYPE_HASH] =                "HASH",
    [TAPI_BPF_MAP_TYPE_ARRAY] =               "ARRAY",
    [TAPI_BPF_MAP_TYPE_PROG_ARRAY] =          "PROG_ARRAY",
    [TAPI_BPF_MAP_TYPE_PERF_EVENT_ARRAY] =    "PERF_EVENT_ARRAY",
    [TAPI_BPF_MAP_TYPE_PERCPU_HASH] =         "PERCPU_HASH",
    [TAPI_BPF_MAP_TYPE_PERCPU_ARRAY] =        "PERCPU_ARRAY",
    [TAPI_BPF_MAP_TYPE_STACK_TRACE] =         "STACK_TRACE",
    [TAPI_BPF_MAP_TYPE_CGROUP_ARRAY] =        "CGROUP_ARRAY",
    [TAPI_BPF_MAP_TYPE_LRU_HASH] =            "LRU_HASH",
    [TAPI_BPF_MAP_TYPE_LRU_PERCPU_HASH] =     "LRU_PERCPU_HASH",
    [TAPI_BPF_MAP_TYPE_LPM_TRIE] =            "LPM_TRIE",
    [TAPI_BPF_MAP_TYPE_ARRAY_OF_MAPS] =       "ARRAY_OF_MAPS",
    [TAPI_BPF_MAP_TYPE_HASH_OF_MAPS] =        "HASH_OF_MAPS",
    [TAPI_BPF_MAP_TYPE_DEVMAP] =              "DEVMAP",
    [TAPI_BPF_MAP_TYPE_SOCKMAP] =             "SOCKMAP",
    [TAPI_BPF_MAP_TYPE_CPUMAP] =              "CPUMAP",
    [TAPI_BPF_MAP_TYPE_XSKMAP] =              "XSKMAP",
    [TAPI_BPF_MAP_TYPE_SOCKHASH] =            "SOCKHASH",
    [TAPI_BPF_MAP_TYPE_CGROUP_STORAGE] =      "CGROUP_STORAGE",
    [TAPI_BPF_MAP_TYPE_REUSEPORT_SOCKARRAY] = "REUSEPORT_SOCKARRAY",
    [TAPI_BPF_MAP_TYPE_UNKNOWN] =             "<UNKNOWN>"
};

/* See description in tapi_bpf.h */
char *
tapi_bpf_build_bpf_obj_path(const char *ta, const char *bpf_prog_name)
{
    char *ta_dir = NULL;
    cfg_val_type val_type = CVT_STRING;
    te_string buf = TE_STRING_INIT;
    te_errno rc;

    rc = cfg_get_instance_fmt(&val_type, &ta_dir, "/agent:%s/dir:", ta);
    if (rc != 0)
    {
        ERROR("%s(): Failed to get /agent:%s/dir : %r",
               __FUNCTION__, ta, rc);
        return NULL;
    }

    rc = te_string_append(&buf, "%s/%s.o", ta_dir, bpf_prog_name);
    if (rc != 0)
    {
        ERROR("%s(): Failed to build path to BPF object: %r",
              __FUNCTION__, rc);
        free(ta_dir);
        return NULL;
    }
    free(ta_dir);
    return buf.ptr;
}

/**
 * Get list of instances names for given pattern
 *
 * @param[in]   ptrn        Pattern, i.e. '/agent:Agt_A/bpf:*'
 * @param[out]  names       Array of names with @c NULL item at the end,
 *                          do not forget to free it after use by
 *                          @ref te_str_free_array
 * @param[out]  count       Pointer to store number of names in array,
 *                          unused if @c NULL
 *
 * @return                  Status code
 */
static te_errno
tapi_bpf_get_inst_list(const char *ptrn, char ***names,
                       unsigned int *names_count)
{
    cfg_handle     *hdl_names = NULL;
    unsigned int    count;
    unsigned int    i, j;
    char           *name = NULL;
    char          **str = NULL;
    te_errno        rc = 0;

    if ((rc = cfg_find_pattern(ptrn, &count, &hdl_names)) != 0)
    {
        ERROR("%s(): Failed to get list for %s: %r",
               __FUNCTION__, ptrn, rc);
        return rc;
    }

    if (names_count != NULL)
        *names_count = count;

    if (count == 0)
    {
        free(hdl_names);
        return 0;
    }

    if ((str = TE_ALLOC(sizeof(char *) * (count + 1))) == NULL)
    {
        free(hdl_names);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    str[count] = NULL;

    for (i = 0; i < count; i++)
    {
        if ((rc = cfg_get_inst_name(hdl_names[i], &name)) != 0)
        {
            ERROR("%s(): Failed to get instance name: %r", __FUNCTION__, rc);
            for (j = 0; j < i; j++)
                free(str[i]);
            free(str);
            free(hdl_names);
            return rc;
        }
        str[i] = name;
    }
    *names = str;
    return 0;
}

/**
 * Get list of instances names for pattern format
 *
 * @param[out]  names       Array of names
 * @param[out]  names_count Pointer to store number of names in array,
 *                          unused if @c NULL
 * @param[in]   ptrn_fmt    Pattern format
 * @param[in]   ...         Values
 *
 * @return                  Status code
 */
static te_errno
tapi_bpf_get_inst_list_fmt(char ***names, unsigned int *names_count,
                           const char *ptrn_fmt, ...)
{
    va_list ap;
    char    ptrn[CFG_OID_MAX];

    va_start(ap, ptrn_fmt);
    vsnprintf(ptrn, sizeof(ptrn), ptrn_fmt, ap);
    va_end(ap);

    return tapi_bpf_get_inst_list(ptrn, names, names_count);
}

/**
 * Generate unique ID (name) for BPF object
 *
 * @param[in] ta        Test Agent name
 * @param[out] bpf_id   BPF object ID
 *
 * @return              Status code
 */
static te_errno
tapi_bpf_gen_id(const char *ta, unsigned int *bpf_id)
{
    char          **names = NULL;
    unsigned int    count;
    unsigned int    i;
    unsigned int    id;
    unsigned int    max_id = 0;
    te_errno        rc = 0;

    if ((rc = tapi_bpf_get_inst_list_fmt(&names, &count,
                                         "/agent:%s/bpf:*", ta)) != 0)
    {
        return rc;
    }

    for (i = 0; i < count; i++)
    {
        if ((rc = te_strtoui(names[i], 10, &id)) != 0)
        {
            unsigned int j;

            for (j = i; j < count; j++)
                free(names[j]);

            free(names);
            return rc;
        }

        if (max_id < id)
            max_id = id;

        free(names[i]);
    }
    free(names);

    *bpf_id = ++max_id;
    return 0;
}

/**
 * Check key/values sizes to real key/value sizes in the map
 *
 * @param ta        Test Agent name
 * @param bpf_id    Bpf ID
 * @param map       Map name
 * @param key_size  Key size to check in the @p map
 * @param val_size  Value size to check in the @p map
 *
 * @return          Status code
 */
static te_errno
tapi_bpf_map_check_kvpair_size(const char *ta, unsigned int bpf_id,
                               const char *map, unsigned int key_size,
                               unsigned int val_size)
{
    unsigned int real_key_size;
    unsigned int real_val_size;
    te_errno     rc;

    if ((rc = tapi_bpf_map_get_key_size(ta, bpf_id, map, &real_key_size)) != 0)
        return rc;

    if ((rc = tapi_bpf_map_get_val_size(ta, bpf_id, map, &real_val_size)) != 0)
        return rc;

    if (key_size != real_key_size || val_size != real_val_size)
    {
        ERROR("%s(): arguments did not match to real values: provided size of "
              "key/value %u/%u, but map %s has key/value size %u/%u",
              __FUNCTION__, key_size, val_size, map, real_key_size,
              real_val_size);

        return TE_RC(TE_TAPI, TE_EFAIL);
    }
    return 0;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_add(const char *ta, const char *fname, unsigned int *bpf_id)
{
    te_errno     rc;
    unsigned int id;

    assert(ta != NULL);
    assert(fname != NULL);
    assert(bpf_id != NULL);

    if ((rc = tapi_bpf_gen_id(ta, &id)) != 0)
        return rc;

    if ((rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                                   "/agent:%s/bpf:%u", ta, id)) != 0)
    {
        ERROR("%s(): Failed to add BPF object /agent:%s/bpf:%u: %r",
              __FUNCTION__, ta, id, rc);
        return rc;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, fname),
                                   "/agent:%s/bpf:%u/filepath:",
                                   ta, id)) != 0)
    {
        ERROR("%s(): Failed to set filepath value %s to /agent:%s/bpf:%u: %r",
              __FUNCTION__, fname, ta, id, rc);
    }
    *bpf_id = id;
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_del(const char *ta, unsigned int bpf_id)
{
    te_errno rc;

    assert(ta != NULL);

    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/bpf:%u", ta, bpf_id)) != 0)
    {
        ERROR("%s(): Failed to delete BPF object /agent:%s/bpf:%u: %r",
              __FUNCTION__, ta, bpf_id, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_load(const char *ta, unsigned int bpf_id)
{
    te_errno rc;

    assert(ta != NULL);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, "loaded"),
                                   "/agent:%s/bpf:%u/state:",
                                   ta, bpf_id)) != 0)
    {
        ERROR("%s(): Failed to load BPF object /agent:%s/bpf:%u: %r",
              __FUNCTION__, ta, bpf_id, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_get_state(const char *ta, unsigned int bpf_id,
                       tapi_bpf_state *bpf_state)
{
    cfg_val_type val = CVT_STRING;
    char        *str = NULL;
    te_errno     rc;

    assert(ta != NULL);
    assert(bpf_state != NULL);

    if ((rc = cfg_get_instance_fmt(&val, &str, "/agent:%s/bpf:%u/state:",
                                   ta, bpf_id)) != 0)
    {
        ERROR("%s(): Failed to get state of BPF object /agent:%s/bpf:%u: %r",
              __FUNCTION__, ta, bpf_id, rc);
        return rc;
    }

    rc = te_str_find_index(str, tapi_bpf_states, TE_ARRAY_LEN(tapi_bpf_states),
                           (unsigned int *)bpf_state);
    free(str);
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_unload(const char *ta, unsigned int bpf_id)
{
    te_errno    rc;
    const char *str = NULL;

    assert(ta != NULL);

    str = tapi_bpf_states[TAPI_BPF_STATE_UNLOADED];

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING,  str),
                                   "/agent:%s/bpf:%u/state:",
                                   ta, bpf_id)) != 0)
    {
        ERROR("%s(): Failed to unload BPF object /agent:%s/bpf:%u: %r",
              __FUNCTION__, ta, bpf_id, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_get_type(const char *ta, unsigned int bpf_id,
                      tapi_bpf_prog_type *type)
{
    cfg_val_type        val_type = CVT_STRING;
    tapi_bpf_prog_type  prog_type;
    char               *str = NULL;
    te_errno            rc;

    assert(ta != NULL);
    assert(type != NULL);

    if ((rc = cfg_get_instance_fmt(&val_type, &str,
                                   "/agent:%s/bpf:%u/type:",
                                   ta, bpf_id)) == 0)
    {
        if ((rc = te_str_find_index(str, tapi_bpf_prog_types,
                                    TE_ARRAY_LEN(tapi_bpf_prog_types),
                                    (unsigned int *)&prog_type)) == 0)
        {
            *type = prog_type;
            return 0;
        }
    }
    else
    {
        ERROR("%s(): Failed to get value for /agent:%s/bpf:%u/type: %r",
              __FUNCTION__, ta, bpf_id, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_set_type(const char *ta, unsigned int bpf_id,
                      tapi_bpf_prog_type type)
{
    te_errno      rc;
    const char   *str = NULL;

    assert(ta != NULL);

    str = tapi_bpf_prog_types[type];

    if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, str),
                                   "/agent:%s/bpf:%u/type:", ta, bpf_id)) != 0)
    {
        ERROR("%s(): Failed to set type %s to /agent:%s/bpf:%u/type: %r",
              __FUNCTION__, type, ta, bpf_id, rc);
    }
    return rc;
}

/************** Functions to work with programs ****************/

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_prog_get_list(const char *ta, unsigned int bpf_id,
                       char ***prog, unsigned int *prog_count)
{
    assert(ta != NULL);
    assert(prog != NULL);
    assert(prog_count != NULL);

    return tapi_bpf_get_inst_list_fmt(prog, prog_count,
                                      "/agent:%s/bpf:%u/program:*", ta, bpf_id);
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_prog_link(const char *ta, const char *ifname,
                   unsigned int bpf_id, tapi_bpf_link_point link_type,
                   const char *prog)
{
    te_string   str = TE_STRING_INIT_STATIC(RCF_MAX_VAL);
    te_errno    rc;

    assert(ta != NULL);
    assert(ifname != NULL);
    assert(prog != NULL);

    if ((rc = te_string_append(&str, "/agent:%s/bpf:%u/prog:%s",
                               ta, bpf_id, prog)) < 0)
    {
        return rc;
    }

    switch (link_type)
    {
        case TAPI_BPF_LINK_XDP:
            rc = cfg_set_instance_fmt(CFG_VAL(STRING, str.ptr),
                                      "/agent:%s/interface:%s/xdp:",
                                      ta, ifname);
            break;

        case TAPI_BPF_LINK_TC_INGRESS:
        {
            te_bool qdisc_is_enabled = FALSE;
            tapi_cfg_qdisc_kind_t qdisc_kind = TAPI_CFG_QDISC_KIND_UNKNOWN;

            if ((rc = tapi_cfg_qdisc_get_enabled(ta, ifname,
                                                 &qdisc_is_enabled)) != 0)
            {
                ERROR("Failed to get qdisc status");
                break;
            }

            if (!qdisc_is_enabled)
            {
                ERROR("qdisc is disabled");
                rc = TE_EINVAL;
                break;
            }

            if ((rc = tapi_cfg_qdisc_get_kind(ta, ifname, &qdisc_kind)) != 0)
            {
                ERROR("Failed to get qdisc kind");
                break;
            }

            if (qdisc_kind != TAPI_CFG_QDISC_KIND_CLSACT)
            {
                ERROR("qdisc has invalid kind, %s instead of %s",
                      tapi_cfg_qdisc_kind2str(qdisc_kind),
                      tapi_cfg_qdisc_kind2str(TAPI_CFG_QDISC_KIND_CLSACT));
                rc = TE_EINVAL;
                break;
            }

            if ((rc = tapi_cfg_qdisc_set_param(ta, ifname, "bpf_ingress",
                                               str.ptr)) != 0)
            {
                ERROR("Failed to set qdisc parameter \"bpf_ingress\"");
                break;
            }
            break;
        }

        default:
            ERROR("Link point is not supported");
            rc = TE_EINVAL;
            break;
    }

    if (rc != 0)
    {
        ERROR("%s(): Failed to link program %s to agent %s interface %s"
              ": %r", __FUNCTION__, str.ptr, ta, ifname, rc);
    }

    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_prog_unlink(const char *ta, const char *ifname,
                     tapi_bpf_link_point link_type)
{
    te_errno    rc;

    assert(ta != NULL);
    assert(ifname != NULL);

    switch (link_type)
    {
        case TAPI_BPF_LINK_TC_INGRESS:
            if ((rc = tapi_cfg_qdisc_set_param(ta, ifname,
                                               "bpf_ingress", "")) != 0)
            {
                ERROR("%s(): Failed to unlink BPF TC program: %r",
                      __FUNCTION__, rc);
            }
            break;

        case TAPI_BPF_LINK_XDP:
            if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                                           "/agent:%s/interface:%s/xdp:",
                                           ta, ifname)) != 0)
            {
                ERROR("%s(): Failed to unlink xdp program from "
                      "/agent:%s/interface:%s/xdp: %r",
                      __FUNCTION__, ta, ifname, rc);
            }
            break;

        default:
            ERROR("%s(): link point type is not specified", __FUNCTION__);
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            break;
    }

    return rc;
}

/************** Functions for working with maps ****************/

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_get_list(const char *ta, unsigned int bpf_id,
                      char ***map, unsigned int *map_count)
{
    assert(ta != NULL);
    assert(map != NULL);

    return tapi_bpf_get_inst_list_fmt(map, map_count, "/agent:%s/bpf:%u/map:*",
                                      ta, bpf_id);
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_get_type(const char *ta, unsigned int bpf_id, const char *map,
                      tapi_bpf_map_type *type)
{
    cfg_val_type        val_type = CVT_STRING;
    tapi_bpf_map_type   map_type;
    char               *str = NULL;
    te_errno            rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(type != NULL);

    if ((rc = cfg_get_instance_fmt(&val_type, &str,
                                   "/agent:%s/bpf:%u/map:%s/type:",
                                   ta, bpf_id, map)) == 0)
    {
        if ((rc = te_str_find_index(str, tapi_bpf_map_types,
                                    TE_ARRAY_LEN(tapi_bpf_map_types),
                                    (unsigned int *)&map_type)) == 0)
        {
            *type = map_type;
            return 0;
        }
    }
    else
    {
        ERROR("%s(): Failed to get value for /agent:%s/bpf:%u/map:%s/type: %r",
              __FUNCTION__, ta, bpf_id, map, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_get_key_size(const char *ta, unsigned int bpf_id,
                          const char *map, unsigned int *key_size)
{
    cfg_val_type val_type = CVT_INTEGER;
    int          val;
    te_errno     rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(key_size != NULL);

    if ((rc = cfg_get_instance_fmt(&val_type, &val,
                                   "/agent:%s/bpf:%u/map:%s/key_size:",
                                   ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to get value for /agent:%s/bpf:%u/map:%s/key_size:"
              " %r", __FUNCTION__, ta, bpf_id, map, rc);
    }
    else
    {
        *key_size = val;
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_get_val_size(const char *ta, unsigned int bpf_id,
                          const char *map, unsigned int *val_size)
{
    cfg_val_type val_type = CVT_INTEGER;
    int          val;
    te_errno     rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(val_size != NULL);

    if ((rc = cfg_get_instance_fmt(&val_type, &val,
                                   "/agent:%s/bpf:%u/map:%s/value_size:",
                                   ta, bpf_id, map)) == 0)
    {
        *val_size = val;
    }
    else
    {
        ERROR("%s(): Failed to get value for "
              "/agent:%s/bpf:%u/map:%s/value_size: %r",
              __FUNCTION__, ta, bpf_id, map, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_get_max_entries(const char *ta, unsigned int bpf_id,
                             const char *map, unsigned int *max_entries)
{
    cfg_val_type val_type = CVT_INTEGER;
    int          val;
    te_errno     rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(max_entries != NULL);

    if ((rc = cfg_get_instance_fmt(&val_type, &val,
                                   "/agent:%s/bpf:%u/map:%s/max_entries:",
                                   ta, bpf_id, map)) == 0)
    {
        *max_entries = val;
    }
    else
    {
        ERROR("%s(): Failed to get value for "
              "/agent:%s/bpf:%u/map:%s/max_entries: %r",
              __FUNCTION__, ta, bpf_id, map, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_get_writable_state(const char *ta, unsigned int bpf_id,
                                const char *map, te_bool *is_writable)
{
    cfg_val_type val_type = CVT_INTEGER;
    int          val;
    te_errno     rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(is_writable != NULL);

    if ((rc = cfg_get_instance_fmt(&val_type, &val,
                                   "/agent:%s/bpf:%u/map:%s/writable:",
                                   ta, bpf_id, map)) == 0)
    {
        *is_writable = val == 0 ? FALSE : TRUE;
    }
    else
    {
        ERROR("%s(): Failed to get value /agent:%s/bpf:%u/map:%s/writable: %r",
              __FUNCTION__, ta, bpf_id, map, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_set_writable(const char *ta, unsigned int bpf_id, const char *map)
{
    const int   value = 1;
    te_errno    rc;

    assert(ta != NULL);
    assert(map != NULL);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                   "/agent:%s/bpf:%u/map:%s/writable:",
                                   ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to set value %d to "
              "/agent:%s/bpf:%u/map:%s/writable: %r",
              __FUNCTION__, value, ta, bpf_id, map, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_unset_writable(const char *ta, unsigned int bpf_id, const char *map)
{
    const int   value = 0;
    te_errno    rc;

    assert(ta != NULL);
    assert(map != NULL);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                   "/agent:%s/bpf:%u/map:%s/writable:",
                                   ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to unset value %d to "
              "/agent:%s/bpf:%u/map:%s/writable: %r",
              __FUNCTION__, value, ta, bpf_id, map, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_update_kvpair(const char *ta, unsigned int bpf_id,
                           const char *map,
                           const uint8_t *key, unsigned int key_size,
                           const uint8_t *val, unsigned int val_size)
{
    te_string    key_str = TE_STRING_INIT_STATIC(RCF_MAX_VAL);
    te_string    val_str = TE_STRING_INIT_STATIC(RCF_MAX_VAL);
    te_bool      wrtbl;
    te_errno     rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(key != NULL);
    assert(val != NULL);

    if ((rc = tapi_bpf_map_get_writable_state(ta, bpf_id, map, &wrtbl)) != 0)
        return rc;

    if (!wrtbl)
    {
        ERROR("%s(): map:%s is not writable", __FUNCTION__, map);
        return TE_RC(TE_TAPI, TE_EPERM);
    }

    if((rc = tapi_bpf_map_check_kvpair_size(ta, bpf_id, map,
                                            key_size, val_size)) != 0)
    {
        return rc;
    }

    if ((rc = te_str_hex_raw2str(key, key_size, &key_str)) != 0)
        return rc;
    if ((rc = te_str_hex_raw2str(val, val_size, &val_str)) != 0)
        return rc;

    /*
     * Some types of XDP maps (e.g. hash and lpm_trie) have no elements
     * on creation. Hence there is no key instances in configurator DB.
     * In these cases we need to add a new instance for specified key-value.
     */
    rc = cfg_find_fmt(NULL, "/agent:%s/bpf:%u/map:%s/writable:/key:%s",
                            ta, bpf_id, map, key_str.ptr);
    if (rc == 0)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, val_str.ptr),
                                  "/agent:%s/bpf:%u/map:%s/writable:/key:%s",
                                  ta, bpf_id, map, key_str.ptr);
    }
    else if (rc == TE_RC(TE_CS, TE_ENOENT))
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, val_str.ptr),
                                  "/agent:%s/bpf:%u/map:%s/writable:/key:%s",
                                  ta, bpf_id, map, key_str.ptr);
    }
    else
    {
        ERROR("%s(): cfg_find_fmt() returned unexpected result: %r",
                  __FUNCTION__, rc);
        return rc;
    }

    if (rc != 0)
    {
        ERROR("%s(): Failed to set value %s to "
              "/agent:%s/bpf:%u/map:%s/writable/key:%s: %r",
              __FUNCTION__, val_str.ptr, ta, bpf_id, map, key_str.ptr, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_lookup_kvpair(const char *ta,
                           unsigned int bpf_id,
                           const char *map,
                           const uint8_t *key, unsigned int key_size,
                           uint8_t *val, unsigned int val_size)
{
    cfg_val_type    val_type = CVT_STRING;
    te_string       key_str = TE_STRING_INIT_STATIC(RCF_MAX_VAL);
    char           *val_str = NULL;
    te_bool         wrtbl;
    te_errno        rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(key != NULL);
    assert(val != NULL);

    if((rc = tapi_bpf_map_check_kvpair_size(ta, bpf_id, map,
                                            key_size, val_size)) != 0)
    {
        return rc;
    }

    if ((rc = te_str_hex_raw2str(key, key_size, &key_str)) != 0)
        return rc;

    if ((rc = tapi_bpf_map_get_writable_state(ta, bpf_id, map, &wrtbl)) != 0)
        return rc;

    if ((rc = cfg_get_instance_fmt(&val_type, &val_str,
                                   "/agent:%s/bpf:%u/map:%s/%s:/key:%s",
                                   ta, bpf_id, map,
                                   wrtbl ? "writable" : "read_only",
                                   key_str.ptr)) != 0)
    {
        ERROR("%s(): Failed to get value for "
              "/agent:%s/bpf:%u/map:%s/%s:/key:%s: %r",
              __FUNCTION__, ta, bpf_id, map,
              wrtbl ? "writable" : "read_only", key_str.ptr, rc);
        return rc;
    }

    return te_str_hex_str2raw(val_str, val, val_size);
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_delete_kvpair(const char *ta, unsigned int bpf_id, const char *map,
                           const uint8_t *key, unsigned int key_size)
{
    te_string    key_str = TE_STRING_INIT_STATIC(RCF_MAX_VAL);
    unsigned int real_key_size;
    te_bool      wrtbl;
    te_errno     rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(key != NULL);

    if ((rc = tapi_bpf_map_get_writable_state(ta, bpf_id, map, &wrtbl)) != 0)
        return rc;

    if (!wrtbl)
    {
        ERROR("%s(): map:%s is not writable", __FUNCTION__, map);
        return TE_RC(TE_TAPI, TE_EPERM);
    }

    if ((rc == tapi_bpf_map_get_key_size(ta, bpf_id, map, &real_key_size)) != 0)
        return rc;

    if (key_size != real_key_size)
    {
        ERROR("%s(): arguments did not match to real key value: provided size "
              "%u, but map %s has key size %u", __FUNCTION__, key_size, map,
              real_key_size);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if ((rc = te_str_hex_raw2str(key, key_size, &key_str)) != 0)
        return rc;

    if ((rc = cfg_del_instance_fmt(FALSE,
                                   "/agent:%s/bpf:%u/map:%s/writable:/key:%s",
                                   ta, bpf_id, map, key_str.ptr)) != 0)
    {
        ERROR("%s(): Failed to delete "
              "/agent:%s/bpf:%u/map:%s/writable:/key:%s: %r",
              __FUNCTION__, ta, bpf_id, map, key_str.ptr, rc);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_get_key_list(const char *ta, unsigned int bpf_id, const char *map,
                          unsigned int *key_size, uint8_t ***key,
                          unsigned int *count)
{
    char          **k_str = NULL;
    uint8_t       **k_keys = NULL;
    unsigned int    k_count;
    unsigned int    k_size;
    unsigned int    i;
    te_bool         wrtbl;
    te_errno        rc;

    assert(ta != NULL);
    assert(map != NULL);
    assert(key_size != NULL);
    assert(key != NULL);
    assert(count != NULL);

    if ((rc = tapi_bpf_map_get_key_size(ta, bpf_id, map, &k_size)) != 0)
        return rc;

    if ((rc = tapi_bpf_map_get_writable_state(ta, bpf_id, map, &wrtbl)) != 0)
        return rc;

    if ((rc = tapi_bpf_get_inst_list_fmt(&k_str, &k_count,
                                         "/agent:%s/bpf:%u/map:%s/%s:/key:*",
                                         ta, bpf_id, map,
                                         wrtbl ?
                                         "writable" : "read_only")) != 0)
    {
        return rc;
    }

    *key_size = k_size;
    *count = k_count;
    if (k_count == 0)
        return 0;

    if ((k_keys = TE_ALLOC(sizeof(uint8_t *) * k_count)) == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto cleanup;
    }

    for (i = 0; i < k_count; i++)
    {
        if ((k_keys[i] = TE_ALLOC(k_size)) == NULL)
        {
            rc = TE_RC(TE_TAPI, TE_ENOMEM);
            goto cleanup;
        }
        if ((rc = te_str_hex_str2raw(k_str[i], k_keys[i], k_size) != 0))
            goto cleanup;
    }
    *key = k_keys;
    rc = 0;

cleanup:
    if (k_str != NULL)
    {
        for (i = 0; i < k_count; i++)
            free(k_str[i]);
        free(k_str);
    }

    if (k_keys != NULL && rc != 0)
    {
        for (i = 0; i < k_count; i++)
            free(k_keys[i]);
        free(k_keys);
    }
    return rc;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_perf_event_init(const char *ta, unsigned int bpf_id,
                         const char *map, unsigned int event_size)
{
    te_errno        rc = 0;
    cfg_val_type    val_type = CVT_INTEGER;
    int             val;

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, event_size),
                                   "/agent:%s/bpf:%u/perf_map:%s/event_size:",
                                   ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to set event size %u to "
              "/agent:%s/bpf:%u/perf_map:%s (%r)",
              __FUNCTION__, event_size, ta, bpf_id, map, rc);
        return rc;
    }

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                  "/agent:%s/bpf:%u/perf_map:%s"
                                  "/events_enable:",
                                  ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to enable events "
              "/agent:%s/bpf:%u/perf_map:%s (%r)",
              __FUNCTION__, ta, bpf_id, map, rc);
        return rc;
    }

    if ((rc = cfg_get_instance_sync_fmt(&val_type, &val,
                                        "/agent:%s/bpf:%u/perf_map:%s"
                                        "/events_enable:",
                                        ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to get instance /agent:%s/bpf:%u/perf_map:%s"
              "/events_enable: (%r)",
              __FUNCTION__, ta, bpf_id, map, rc);
        return rc;
    }

    if (val == 0)
    {
        ERROR("%s(): Initialization of perf event map failed.", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    return 0;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_perf_event_deinit(const char *ta, unsigned int bpf_id,
                           const char *map)
{
    te_errno rc = 0;

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                  "/agent:%s/bpf:%u/perf_map:%s"
                                  "/events_enable:",
                                  ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to disable events "
              "/agent:%s/bpf:%u/perf_map:%s (%r)",
              __FUNCTION__, ta, bpf_id, map, rc);
    }

    return rc;
}

te_errno
tapi_bpf_perf_get_events(const char *ta, unsigned int bpf_id, const char *map,
                         unsigned int *num, uint8_t **data)
{
    cfg_val_type    val_type = CVT_INTEGER;
    te_errno        rc = 0;
    unsigned int    num_events = 0;
    unsigned int    event_data_size = 0;
    unsigned int    event_id_num = 0;
    cfg_handle     *event_id_hdls = NULL;
    unsigned int    i = 0;

    if (data == NULL || num == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((rc = cfg_get_instance_fmt(&val_type, &event_data_size,
                              "/agent:%s/bpf:%u/perf_map:%s/event_size:",
                              ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to get event size from "
              "/agent:%s/bpf:%u/perf_map:%s (%r)",
              __FUNCTION__, ta, bpf_id, map, rc);
        return rc;
    }

    if ((rc = cfg_get_instance_fmt(&val_type, &num_events,
                              "/agent:%s/bpf:%u/perf_map:%s/num_events:",
                              ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to get number of events from "
              "/agent:%s/bpf:%u/perf_map:%s (%r)",
              __FUNCTION__, ta, bpf_id, map, rc);
        return rc;
    }

    if ((rc = cfg_find_pattern_fmt(&event_id_num, &event_id_hdls,
                                   "/agent:%s/bpf:%u/perf_map:%s/id:*",
                                   ta, bpf_id, map)) != 0)
    {
        ERROR("%s(): Failed to get event data list from "
              "/agent:%s/bpf:%u/perf_map:%s (%r)",
              __FUNCTION__, ta, bpf_id, map, rc);
        return rc;
    }

    if (num_events != event_id_num)
    {
        ERROR("%s(): Number of events in id list is invalid", __FUNCTION__);
        free(event_id_hdls);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    *data = tapi_calloc(event_id_num, event_data_size);

    val_type = CVT_STRING;
    for (i = 0; i < event_id_num; ++i)
    {
        char    *val_str = NULL;
        uint8_t *data_ptr = *data + i * event_data_size;

        if ((rc = cfg_get_instance(event_id_hdls[i],
                                   &val_type,
                                   &val_str)) != 0)
        {
            ERROR("%s(): Failed to get event data instance", __FUNCTION__);
            free(event_id_hdls);
            return rc;
        }

        if ((rc = te_str_hex_str2raw(val_str, data_ptr, event_data_size)) != 0)
        {
            ERROR("%s(): Failed to convert hex-string to raw data",
                  __FUNCTION__);
            free(val_str);
            free(event_id_hdls);
            return rc;
        }

        free(val_str);
    }

    *num = event_id_num;
    free(event_id_hdls);

    return 0;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_perf_map_get_list(const char *ta, unsigned int bpf_id,
                           char ***map, unsigned int *map_count)
{
    assert(ta != NULL);
    assert(map != NULL);

    return tapi_bpf_get_inst_list_fmt(map, map_count, "/agent:%s/bpf:%u/perf_map:*",
                                      ta, bpf_id);
}

/************** Auxiliary functions  ****************/

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_init(const char *ta, const char *path, tapi_bpf_prog_type type,
                  unsigned int *bpf_id)
{
    te_errno     rc;
    unsigned int id;

    if ((rc = tapi_bpf_obj_add(ta, path, &id)) != 0)
        return rc;

    if ((rc = tapi_bpf_obj_set_type(ta, id, type)) != 0)
        return rc;

    if ((rc = tapi_bpf_obj_load(ta, id)) != 0)
        return rc;

    *bpf_id = id;
    return 0;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_obj_fini(const char *ta, unsigned int bpf_id)
{
    te_errno rc;

    if ((rc = tapi_bpf_obj_unload(ta, bpf_id)) != 0)
        return rc;

    return tapi_bpf_obj_del(ta, bpf_id);
}

/**
 * Check that object with @p name is exists
 *
 * @param obj_name  The name of object to search
 *                      i.e. /agent:Agt_A/bpf:1/program:xdp1
 *
 * @return              Status code
 */
static te_errno
tapi_bpf_check_name_exists(const char *obj_name)
{
    char          **names = NULL;
    unsigned int    count;
    te_errno        rc = 0;

    if ((rc = tapi_bpf_get_inst_list(obj_name, &names, &count) != 0))
    {
        return rc;
    }

    if (count == 0)
    {
        return TE_RC(TE_TAPI, TE_ENOENT);
    }
    else if (count > 1)
    {
        ERROR("%s(): unexpected count of instances '%s', expected 1, but "
              "obtained %u", __FUNCTION__, obj_name, count);
        rc = TE_RC(TE_TAPI, TE_EEXIST);
    }

    te_str_free_array(names);

    return 0;
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_prog_name_check(const char *ta, unsigned int bpf_id,
                         const char *prog_name)
{
    te_string   str = TE_STRING_INIT_STATIC(RCF_MAX_ID);
    te_errno    rc = 0;

    assert(ta != NULL);
    assert(prog_name != NULL);

    if ((rc = te_string_append(&str, "/agent:%s/bpf:%u/program:%s",
                               ta, bpf_id, prog_name)) != 0)
    {
        return rc;
    }

    return tapi_bpf_check_name_exists(str.ptr);
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_type_name_check(const char *ta, unsigned int bpf_id,
                             const char *map_name,
                             tapi_bpf_map_type map_type)
{
    te_string   str = TE_STRING_INIT_STATIC(RCF_MAX_ID);
    te_errno    rc = 0;

    assert(ta != NULL);
    assert(map_name != NULL);

    if ((rc = te_string_append(&str, "/agent:%s/bpf:%u/%s:%s",
                               ta, bpf_id,
                               map_type == TAPI_BPF_MAP_TYPE_PERF_EVENT_ARRAY ?
                               "perf_map" : "map",
                               map_name)) != 0)
    {
        return rc;
    }

    return tapi_bpf_check_name_exists(str.ptr);
}

/* See description in tapi_bpf.h */
te_errno
tapi_bpf_map_check_type(const char *ta,
                        unsigned int bpf_id,
                        const char *map_name,
                        tapi_bpf_map_type exp_map_type)
{
    tapi_bpf_map_type type;
    te_errno          rc = 0;

    assert(ta != NULL);
    assert(map_name != NULL);

    if ((rc = tapi_bpf_map_get_type(ta, bpf_id, map_name, &type)) != 0)
        return rc;

    if (exp_map_type != type)
    {
        ERROR("%s(): The specified type %s does not match the expected type %s",
              __FUNCTION__, tapi_bpf_map_types[type], tapi_bpf_map_types[exp_map_type]);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}
