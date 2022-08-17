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

/**
 * Check whether a given object requires an instance name.
 *
 * @param obj_name    Object name.
 *
 * @return @c TRUE if this is an object which requires an instance name,
 *         @c FALSE otherwise.
 */
static te_bool
req_instance(const char *obj_name)
{
    const char    *names[] = { "conf/", "neigh/" };
    size_t         name_len;
    unsigned int   i;

    for (i = 0; i < TE_ARRAY_LEN(names); i++)
    {
        name_len = strlen(names[i]);

        if (strncmp(obj_name, names[i], name_len) == 0)
            return TRUE;
    }

    return FALSE;
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
    te_errno  rc = 0;
    te_bool   instance_name = FALSE;

    if (len == 0 || buf == NULL)
    {
        ERROR("%s(): NULL or buffer of zero length passed",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_ESMALLBUF);
    }

    rc = te_string_append_va(&str, fmt, ap);
    if (rc != 0)
        return rc;

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
                instance_name = FALSE;
                obj_name = buf_aux + i + 1;
            }
            else if (buf_aux[i] == ':')
            {
                instance_name = TRUE;
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
                instance_name = TRUE;
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
    rc = tapi_cfg_sys_get_va(ta, CVT_INTEGER, val, fmt, ap);
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
    rc = tapi_cfg_sys_set_va(ta, CFG_VAL(INTEGER, val), old_val, fmt, ap);
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
    rc = tapi_cfg_sys_ns_get_va(ta, CVT_INTEGER, val, fmt, ap);
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
    rc = tapi_cfg_sys_ns_set_va(ta, CFG_VAL(INTEGER, val), old_val, fmt, ap);
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
