/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to get/set parameters in /proc/sys/
 *
 * Implementation of TAPI to get/set Linux system parameters
 * in /proc/sys/.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "/proc/sys TAPI"

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_cfg_base.h"
#include "tapi_host_ns.h"
#include "tapi_cfg_sys.h"
#include "te_string.h"
#include "te_enum.h"

/**
 * Check whether a given object requires an instance name.
 *
 * @param obj_name    Object name.
 *
 * @return @c true if this is an object which requires an instance name,
 *         @c false otherwise.
 */
static bool
req_instance(const char *obj_name)
{
    const char    *names[] = { "conf/", "neigh/" };
    size_t         name_len;
    unsigned int   i;

    for (i = 0; i < TE_ARRAY_LEN(names); i++)
    {
        name_len = strlen(names[i]);

        if (strncmp(obj_name, names[i], name_len) == 0)
            return true;
    }

    return false;
}

/**
 * Parse path passed to one of the TAPI functions, construct
 * valid OID from it. For example, "net/ipv4/tcp_retries2"
 * is turned into "net:/ipv4:/tcp_retries2:".
 *
 * @note If "neigh" or "conf" are encountered in path, the next path
 *       element is treated as instance name (for example, "neigh/default"
 *       turns into "neigh:default").
 *
 * @param buf         Buffer where to save OID.
 * @param len         Length of the buffer.
 * @param fmt         Format string of the path.
 * @param ap          List of arguments for format string.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_sys_parse_path_va(char *buf, size_t len,
                           const char *fmt, va_list ap)
{
    char        buf_aux[CFG_OID_MAX] = "";
    te_string   str = TE_STRING_BUF_INIT(buf_aux);
    char       *obj_name = NULL;

    size_t    i = 0;
    size_t    j = 0;
    bool instance_name = false;

    if (len == 0 || buf == NULL)
    {
        ERROR("%s(): NULL or buffer of zero length passed",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_ESMALLBUF);
    }

    te_string_append_va(&str, fmt, ap);

    obj_name = buf_aux;
    for (i = 0; ; i++)
    {
        if (j >= len)
            break;

        if ((buf_aux[i] != '/' && buf_aux[i] != '\0') || instance_name)
        {
            /*
             * If this is not end of an object name, simply copy
             * the current symbol.
             */
            buf[j++] = buf_aux[i];

            if (buf_aux[i] == '/')
            {
                instance_name = false;
                obj_name = buf_aux + i + 1;
            }
            else if (buf_aux[i] == ':')
            {
                instance_name = true;
            }
        }
        else
        {
            /* If this is end of an object name, insert ":". */
            buf[j++] = ':';

            if (buf_aux[i] == '\0')
            {
                if (j >= len)
                    break;

                buf[j++] = '\0';
                break;
            }

            if (req_instance(obj_name))
            {
                /*
                 * If the object name was "conf" or "neigh", treat the
                 * next name in the path as an instance name rather than
                 * an object name.
                 */
                instance_name = true;
            }
            else if (buf_aux[i] == '/')
            {
                if (j >= len)
                    break;

                buf[j++] = '/';
                obj_name = buf_aux + i + 1;
            }
        }

        if (buf_aux[i] == '\0')
            break;
    }

    assert(j > 0);
    if (buf[j - 1] != '\0')
    {
        ERROR("%s(): not enough space for OID", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_ESMALLBUF);
    }

    return 0;
}

/**
 * Get value of specified parameter from /sys: subtree.
 *
 * @param ta            Test agent name.
 * @param type          Parameter type.
 * @param val           Where to save obtained value.
 * @param fmt           Format string for parameter's path in /proc/sys/.
 * @param ap            Arguments for the format string.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_sys_get_va(const char *ta, cfg_val_type type, void *val,
                    const char *fmt, va_list ap)
{
    char      buf[CFG_OID_MAX];
    te_errno  rc;

    rc = tapi_cfg_sys_parse_path_va(buf, sizeof(buf), fmt, ap);
    if (rc != 0)
        return rc;

    return cfg_get_instance_fmt(&type, val, "/agent:%s/sys:/%s", ta, buf);
}

/**
 * Set value of specified parameter in /sys: subtree.
 *
 * @param ta            Test agent name.
 * @param type          Parameter type.
 * @param val           Value to set.
 * @param old_val       Where to save previous value (if not @c NULL).
 * @param fmt           Format string for parameter's path in /proc/sys/.
 * @param ap            Arguments for the format string.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_sys_set_va(const char *ta, cfg_val_type type, const void *val,
                    void *old_val, const char *fmt, va_list ap)
{
    char      buf[CFG_OID_MAX];
    te_errno  rc;

    rc = tapi_cfg_sys_parse_path_va(buf, sizeof(buf), fmt, ap);
    if (rc != 0)
        return rc;

    if (old_val != NULL)
    {
        rc = cfg_get_instance_fmt(&type, old_val,
                                  "/agent:%s/sys:/%s", ta, buf);
        if (rc != 0)
            return rc;
    }

    return cfg_set_instance_fmt(type, val, "/agent:%s/sys:/%s", ta, buf);
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_get_int(const char *ta, int *val, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_get_va(ta, CVT_INT32, val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_set_int(const char *ta, int val, int *old_val,
                     const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_set_va(ta, CFG_VAL(INT32, val), old_val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_get_uint64(const char *ta, uint64_t *val, const char *fmt, ...)
{
    va_list ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_get_va(ta, CVT_UINT64, val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_set_uint64(const char *ta, uint64_t val, uint64_t *old_val,
                        const char *fmt, ...)
{
    va_list ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_set_va(ta, CFG_VAL(UINT64, val), old_val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_get_str(const char *ta, char **val, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_get_va(ta, CVT_STRING, val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_set_str(const char *ta, const char *val, char **old_val,
                     const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_set_va(ta, CFG_VAL(STRING, val), old_val, fmt, ap);
    va_end(ap);

    return rc;
}

/**
 * Get value of specified parameter from /sys: subtree. Use test agent in
 * default net namespace if the parameter does not exist in current namespace.
 *
 * @param ta            Test agent name.
 * @param type          Parameter type.
 * @param val           Where to save obtained value.
 * @param fmt           Format string for parameter's path in /proc/sys/.
 * @param ap            Arguments for the format string.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_sys_ns_get_va(const char *ta, cfg_val_type type, void *val,
                       const char *fmt, va_list ap)
{
    te_errno    rc;
    char       *ta_def;

    rc = tapi_cfg_sys_get_va(ta, type, val, fmt, ap);
    if (rc == 0)
        return 0;
    else if (rc != TE_RC(TE_CS, TE_ENOENT))
        return rc;

    rc = tapi_host_ns_agent_default(ta, &ta_def);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_sys_get_va(ta_def, type, val, fmt, ap);
    free(ta_def);

    return rc;
}

/**
 * Set value of specified parameter in /sys: subtree. Use test agent in
 * default net namespace if the parameter does not exist in current namespace.
 *
 * @param ta            Test agent name.
 * @param type          Parameter type.
 * @param val           Value to set.
 * @param old_val       Where to save previous value (if not @c NULL).
 * @param fmt           Format string for parameter's path in /proc/sys/.
 * @param ap            Arguments for the format string.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_sys_ns_set_va(const char *ta, cfg_val_type type, const void *val,
                       void *old_val, const char *fmt, va_list ap)
{
    te_errno rc;
    char    *ta_def;

    rc = tapi_cfg_sys_set_va(ta, type, val, old_val, fmt, ap);
    if (rc == 0)
        return 0;
    else if (rc != TE_RC(TE_CS, TE_ENOENT))
        return rc;

    rc = tapi_host_ns_agent_default(ta, &ta_def);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_sys_set_va(ta_def, type, val, old_val, fmt, ap);
    free(ta_def);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_ns_get_int(const char *ta, int *val, const char *fmt, ...)
{
    va_list     ap;
    te_errno    rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_ns_get_va(ta, CVT_INT32, val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_ns_set_int(const char *ta, int val, int *old_val,
                        const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_ns_set_va(ta, CFG_VAL(INT32, val), old_val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_ns_get_uint64(const char *ta, uint64_t *val,
                           const char *fmt, ...)
{
    va_list ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_ns_get_va(ta, CVT_UINT64, val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_ns_set_uint64(const char *ta, uint64_t val, uint64_t *old_val,
                           const char *fmt, ...)
{
    va_list ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_ns_set_va(ta, CFG_VAL(UINT64, val), old_val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_ns_get_str(const char *ta, char **val, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_cfg_sys_ns_get_va(ta, CVT_STRING, val, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_ns_set_str(const char *ta, const char *val, char **old_val,
                     const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;
    va_start(ap, fmt);
    rc = tapi_cfg_sys_ns_set_va(ta, CFG_VAL(STRING, val), old_val, fmt, ap);
    va_end(ap);

    return rc;
}

/* Mapping of IP-related sysctl subtrees to their string representations. */
static const te_enum_map ip_net_subtree_map[] = {
    { "conf",  TAPI_CFG_SYS_IP_SUBTREE_CONF },
    { "neigh", TAPI_CFG_SYS_IP_SUBTREE_NEIGH },
    TE_ENUM_MAP_END
};

/* Mapping of sysctl instance kinds to their string representations. */
static const te_enum_map ip_instance_kind_map[] = {
    { "all",     TAPI_CFG_SYS_IP_INST_ALL },
    { "default", TAPI_CFG_SYS_IP_INST_DEFAULT },
    TE_ENUM_MAP_END
};

/* Convert address family to the string representation of IP protocol. */
static const char *
af2str(int af, bool only_suffix)
{
    switch (af)
    {
       case AF_INET:
           return only_suffix ? "" : "ipv4";

       case AF_INET6:
           return only_suffix ? "6" : "ipv6";

       default:
           TE_FATAL_ERROR("Unsupported address family: %d", af);
    }
}

/* Build resource name for an IP sysctl subtree. */
static char *
tapi_cfg_sys_ip_rsrc_name(int af, tapi_cfg_sys_ip_net_subtree subtree,
                          tapi_cfg_sys_ip_instance_kind inst)
{
    const char *suffix = af2str(af, true);
    const char *name;
    const char *sub;

    sub = te_enum_map_from_any_value(ip_net_subtree_map, subtree, NULL);
    name = te_enum_map_from_any_value(ip_instance_kind_map, inst, NULL);

    if (sub == NULL || name == NULL)
        return NULL;

    return te_string_fmt("%s%s_%s", sub, suffix, name);
}

/* Build OID for an IP-related sysctl subtree. */
static char *
tapi_cfg_sys_ip_oid(const char *ta, int af,
                    tapi_cfg_sys_ip_net_subtree subtree,
                    tapi_cfg_sys_ip_instance_kind inst)
{
    const char *ver = af2str(af, false);
    const char *name;
    const char *sub;

    sub = te_enum_map_from_any_value(ip_net_subtree_map, subtree, NULL);
    name = te_enum_map_from_any_value(ip_instance_kind_map, inst, NULL);

    if (ta == NULL || sub == NULL || name == NULL)
        return NULL;

    return te_string_fmt("/agent:%s/sys:/net:/%s:/%s:%s", ta, ver, sub, name);
}

/* Create or reuse a Configurator resource and bind it to a given OID. */
static te_errno
tapi_cfg_sys_rsrc_grab_oid(const char *ta, const char *rsrc_name,
                          const char *oid, cs_rsrc_lock_type lock_type)
{
    static const int grab_timeout_ms = 3000;
    char *old_oid = NULL;
    bool set_oid = true;
    int actual_shared;
    bool shared;
    te_errno rc;

    if (ta == NULL || rsrc_name == NULL || oid == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    switch (lock_type)
    {
       case CS_RSRC_LOCK_TYPE_EXCLUSIVE:
           shared = false;
           break;

       case CS_RSRC_LOCK_TYPE_SHARED:
           shared = true;
           break;

       default:
           ERROR("Unsupported resource lock type");
           return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_get_instance_string_fmt(&old_oid,
                                     "/agent:%s/rsrc:%s", ta, rsrc_name);
    if (rc == 0)
    {
        if (old_oid == NULL)
            return TE_RC(TE_TAPI, TE_EFAULT);

        if (*old_oid != '\0' && strcmp(old_oid, oid) != 0)
        {
            ERROR("Resource '/agent:%s/rsrc:%s' points to '%s' instead of '%s'",
                  ta, rsrc_name, old_oid, oid);
            free(old_oid);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
        set_oid = (*old_oid == '\0');
        free(old_oid);
    }
    else
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, ""),
                                  "/agent:%s/rsrc:%s", ta, rsrc_name);
        if (rc != 0)
            return rc;
    }

    /*
     * Bind the resource to the OID before requesting lock mode,
     * so that the resource subsystem can acquire the lock.
     */
    if (set_oid)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, oid),
                                  "/agent:%s/rsrc:%s", ta, rsrc_name);
        if (rc != 0)
            return rc;
    }

    rc = cfg_set_instance_fmt(CFG_VAL(INT32, 1),
                              "/agent:%s/rsrc:%s/fallback_shared:",
                              ta, rsrc_name);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(INT32, grab_timeout_ms),
                              "/agent:%s/rsrc:%s/acquire_attempts_timeout:",
                              ta, rsrc_name);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(INT32, shared ? 1 : 0),
                              "/agent:%s/rsrc:%s/shared:",
                              ta, rsrc_name);
    if (rc != 0)
        return rc;

    rc = cfg_get_instance_int_fmt(&actual_shared,
                                  "/agent:%s/rsrc:%s/shared:",
                                  ta, rsrc_name);
    if (rc != 0)
        return rc;

    if ((actual_shared != 0) != shared)
    {
        ERROR("Failed to acquire %s lock for '%s' on %s (oid=%s): got %s",
              shared ? "shared" : "exclusive",
              rsrc_name, ta, oid,
              (actual_shared != 0) ? "shared" : "exclusive");
        return TE_RC(TE_TAPI, TE_EBUSY);
    }

    return 0;
}

/* See description in tapi_cfg_sys.h */
te_errno
tapi_cfg_sys_ip_grab(const char *ta, int af,
                     tapi_cfg_sys_ip_net_subtree subtree,
                     tapi_cfg_sys_ip_instance_kind inst,
                     cs_rsrc_lock_type lock_type)
{
    char *rsrc_name = NULL;
    te_errno rc;
    char *oid = NULL;

    if (ta == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rsrc_name = tapi_cfg_sys_ip_rsrc_name(af, subtree, inst);
    oid = tapi_cfg_sys_ip_oid(ta, af, subtree, inst);
    if (rsrc_name == NULL || oid == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    rc = tapi_cfg_sys_rsrc_grab_oid(ta, rsrc_name, oid, lock_type);
    if (rc != 0)
    {
        ERROR("%s(): failed to grab sys resource '%s' on %s (oid=%s): %r",
              __FUNCTION__, rsrc_name, ta, oid, rc);
        goto out;
    }

out:
    free(rsrc_name);
    free(oid);

    return rc;
}
