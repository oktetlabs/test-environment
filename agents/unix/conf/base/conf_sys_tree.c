/** @file
 * @brief System wide settings from /proc/sys/
 *
 * Linux TA system wide settings support (new interface with tree
 * structure)
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#define TE_LGR_USER     "Conf Sys Tree"

#include "te_config.h"
#include "config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "unix_internal.h"
#include "conf_common.h"
#include "te_string.h"
#include "te_str.h"

/** Auxiliary buffer used to construct strings. */
static char buf[4096];

static te_errno sys_if_dir_list_ipv4(unsigned int, const char *,
                                     const char *, char **);

static te_errno sys_if_dir_list_ipv6(unsigned int, const char *,
                                     const char *, char **);

RCF_PCH_CFG_NODE_NA(node_ipv6_icmp, "icmp", NULL, NULL);
RCF_PCH_CFG_NODE_NA(node_ipv6_route, "route", NULL, &node_ipv6_icmp);
RCF_PCH_CFG_NODE_RO_COLLECTION(node_ipv6_neigh, "neigh", NULL,
                               &node_ipv6_route, NULL, sys_if_dir_list_ipv6);
RCF_PCH_CFG_NODE_RO_COLLECTION(node_ipv6_conf, "conf", NULL, &node_ipv6_neigh,
                               NULL, sys_if_dir_list_ipv6);
RCF_PCH_CFG_NODE_NA(node_ipv6, "ipv6", &node_ipv6_conf, NULL);
RCF_PCH_CFG_NODE_NA(node_route, "route", NULL, NULL);
RCF_PCH_CFG_NODE_RO_COLLECTION(node_neigh, "neigh", NULL, &node_route,
                               NULL, sys_if_dir_list_ipv4);
RCF_PCH_CFG_NODE_RO_COLLECTION(node_conf, "conf", NULL, &node_neigh,
                               NULL, sys_if_dir_list_ipv4);
RCF_PCH_CFG_NODE_NA(node_ipv4, "ipv4", &node_conf, &node_ipv6);
RCF_PCH_CFG_NODE_NA(node_core, "core", NULL, &node_ipv4);
RCF_PCH_CFG_NODE_NA(node_net, "net", &node_core, NULL);

static te_errno register_sys_opts(const char *father, const char *path);
static te_errno unregister_sys_opts(const char *father);

/**
 * Create a tree of objects corresponding to directories and files in
 * /proc/sys/, link them to configuration tree.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_sys_tree_init(void)
{
#if __linux__
    CHECK_NZ_RETURN(rcf_pch_add_node("/agent/sys/", &node_net));

    CHECK_NZ_RETURN(register_sys_opts("/agent/sys/net/core",
                                      "/proc/sys/net/core/"));
    CHECK_NZ_RETURN(register_sys_opts("/agent/sys/net/ipv4",
                                      "/proc/sys/net/ipv4/"));
    CHECK_NZ_RETURN(register_sys_opts(
                              "/agent/sys/net/ipv4/conf",
                              "/proc/sys/net/ipv4/conf/default/"));
    CHECK_NZ_RETURN(register_sys_opts(
                              "/agent/sys/net/ipv4/neigh",
                              "/proc/sys/net/ipv4/neigh/default/"));
    CHECK_NZ_RETURN(register_sys_opts("/agent/sys/net/ipv4/route",
                                      "/proc/sys/net/ipv4/route/"));

    CHECK_NZ_RETURN(register_sys_opts("/agent/sys/net/ipv6",
                                      "/proc/sys/net/ipv6/"));
    CHECK_NZ_RETURN(register_sys_opts("/agent/sys/net/ipv6/conf",
                                      "/proc/sys/net/ipv6/conf/default/"));
    CHECK_NZ_RETURN(register_sys_opts("/agent/sys/net/ipv6/neigh",
                                      "/proc/sys/net/ipv6/neigh/default/"));
    CHECK_NZ_RETURN(register_sys_opts("/agent/sys/net/ipv6/route",
                                      "/proc/sys/net/ipv6/route/"));
    CHECK_NZ_RETURN(register_sys_opts("/agent/sys/net/ipv6/icmp",
                                      "/proc/sys/net/ipv6/icmp/"));

    CHECK_NZ_RETURN(rcf_pch_rsrc_info("/agent/sys/net/ipv4/conf",
                                      rcf_pch_rsrc_grab_dummy,
                                      rcf_pch_rsrc_release_dummy));
    CHECK_NZ_RETURN(rcf_pch_rsrc_info("/agent/sys/net/ipv4/neigh",
                                      rcf_pch_rsrc_grab_dummy,
                                      rcf_pch_rsrc_release_dummy));
    CHECK_NZ_RETURN(rcf_pch_rsrc_info("/agent/sys/net/ipv6/conf",
                                      rcf_pch_rsrc_grab_dummy,
                                      rcf_pch_rsrc_release_dummy));
    CHECK_NZ_RETURN(rcf_pch_rsrc_info("/agent/sys/net/ipv6/neigh",
                                      rcf_pch_rsrc_grab_dummy,
                                      rcf_pch_rsrc_release_dummy));
#endif

    return 0;
}

/**
 * Release resources allocated for objects in /agent/sys/ subtree.
 *
 * @return Status code.
 */
te_errno
ta_unix_conf_sys_tree_fini(void)
{
#if __linux__
    CHECK_NZ_RETURN(unregister_sys_opts("/agent/sys/net/core"));
    CHECK_NZ_RETURN(unregister_sys_opts("/agent/sys/net/ipv4"));
    CHECK_NZ_RETURN(unregister_sys_opts("/agent/sys/net/ipv4/conf"));
    CHECK_NZ_RETURN(unregister_sys_opts("/agent/sys/net/ipv4/neigh"));
    CHECK_NZ_RETURN(unregister_sys_opts("/agent/sys/net/ipv4/route"));
    CHECK_NZ_RETURN(unregister_sys_opts("/agent/sys/net/ipv6"));
    CHECK_NZ_RETURN(unregister_sys_opts("/agent/sys/net/ipv6/conf"));
#endif

    return 0;
}

/**
 * Callback used to filter list of instance names corresponding to
 * paths like /proc/sys/net/ipv4/conf/. Directories there
 * are either named by interface names, or have names like "all",
 * "default". Directory name should be appended to a list only
 * if either the interface with such name is grabbed or
 * rsrc instance for corresponding configuration path is added directly.
 *
 * @param dir_name        Directory name.
 * @param data            Pointer to object name
 *                        (a prefix like "ipv4:/<sub_id>").
 *
 * @return @c TRUE if directory name should be included in the list,
 *         @c FALSE otherwise.
 */
static te_bool
sys_if_list_include_callback(const char *dir_name, void *data)
{
    const char *prefix = (const char *)data;
    te_bool     grabbed = FALSE;

    UNUSED(data);

    grabbed = rcf_pch_rsrc_accessible("/agent:%s/interface:%s",
                                      ta_name, dir_name);
    if (!grabbed)
    {
        grabbed = rcf_pch_rsrc_accessible(
                          "/agent:%s/sys:/net:/%s:%s",
                          ta_name, prefix, dir_name);
    }

    return grabbed;
}

/**
 * Get path in /proc/sys/ corresponding to a given configuration object.
 *
 * @param oid             OID (may be OID of parent object).
 * @param sub_id          Object name (may be passed if @p oid ends
 *                        at parent; ignored if @c NULL or empty).
 * @param path            Where to save path.
 * @param path_len        Size of buffer @p path.
 *
 * @return Status code.
 */
static te_errno
sys_opt_get_path(const char *oid, const char *sub_id,
                 char *path, size_t path_len)
{
    ssize_t     i = 0;
    size_t      len = 0;
    size_t      pos = 0;
    const char *s = NULL;

    const char *start_str = "/proc/";

    len = strlen(start_str);
    if (len >= path_len)
    {
        ERROR("%s(): not enough space for path", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
    }
    memcpy(path, start_str, len + 1);

    s = strstr(oid, "/sys:");
    if (s == NULL)
    {
        ERROR("%s(): failed to find /sys: in OID '%s'", __FUNCTION__, oid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    for (i = 0, pos = len; ; i++, pos++)
    {
        if (pos >= path_len)
        {
            ERROR("%s(): not enough space for path", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }

        if (s[i] == ':')
        {
            if (s[i + 1] == '\0')
                path[pos] = '\0';
            else
                path[pos] = '/';
        }
        else
        {
            path[pos] = s[i];
        }

        if (path[pos] == '\0')
            break;
    }

    if (!te_str_is_null_or_empty(sub_id))
    {
        len = strlen(sub_id);
        /*
         * (pos + len + 1) will be the index of final
         * '\0', (path_len - 1) is the maximum index.
         * pos + len + 1 > path_len - 1 is the same
         * as the following condition.
         */
        if (pos + len + 2 > path_len)
        {
            ERROR("%s(): not enough space for path", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }
        path[pos] = '/';
        pos++;
        memcpy(path + pos, sub_id, len + 1);
    }

    return 0;
}

/**
 * Get list of instance names corresponding to locations
 * in /proc/sys/ like /proc/sys/net/ipv4/conf (where subdirectories
 * are either named after interfaces or have names like "all",
 * "default").
 *
 * @param oid           OID of a father object instance.
 * @param prefix        Node prefix like "ipv4:" or "ipv6:"
 * @param sub_id        ID of the object to be listed.
 * @param list          Where to save list of names.
 *
 * @return Status code.
 */
static te_errno
sys_if_dir_list(const char *oid, const char *prefix, const char *sub_id,
                char **list)
{
    te_errno  rc;
    char      path[PATH_MAX] = "";
    te_string prefix_ext = TE_STRING_INIT;

    rc = sys_opt_get_path(oid, sub_id, path, sizeof(path));
    if (rc != 0)
        return rc;

    rc = te_string_append(&prefix_ext, "%s/%s", prefix, sub_id);
    if (rc != 0)
        return rc;

    rc = get_dir_list(path, buf, sizeof(buf), TRUE,
                      &sys_if_list_include_callback, (void *)prefix_ext.ptr);
    te_string_free(&prefix_ext);
    if (rc != 0)
        return rc;

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get list of instance names corresponding to locations
 * in /proc/sys/net/ipv4/ like /proc/sys/net/ipv4/conf (where subdirectories
 * are either named after interfaces or have names like "all",
 * "default").
 *
 * @param gid           Group ID (unused).
 * @param oid           OID of a father object instance.
 * @param sub_id        ID of the object to be listed.
 * @param list          Where to save list of names.
 *
 * @return Status code.
 */
static te_errno
sys_if_dir_list_ipv4(unsigned int gid, const char *oid,
                     const char *sub_id, char **list)
{
    UNUSED(gid);
    return sys_if_dir_list(oid, "ipv4:", sub_id, list);
}

/**
 * Get list of instance names corresponding to locations
 * in /proc/sys/net/ipv6/ like /proc/sys/net/ipv6/conf (where subdirectories
 * are either named after interfaces or have names like "all",
 * "default").
 *
 * @param gid           Group ID (unused).
 * @param oid           OID of a father object instance.
 * @param sub_id        ID of the object to be listed.
 * @param list          Where to save list of names.
 *
 * @return Status code.
 */
static te_errno
sys_if_dir_list_ipv6(unsigned int gid, const char *oid,
                     const char *sub_id, char **list)
{
    UNUSED(gid);
    return sys_if_dir_list(oid, "ipv6:", sub_id, list);
}

/**
 * List instance names corresponding to a single file under /proc/sys/.
 * Usually the instance is only one; in this case " " is returned
 * (which translates to a single instance with empty name). However
 * some files under /proc/sys/ have multiple fields separated by spaces
 * (see for example /proc/sys/net/ipv4/tcp_wmem). In such case a list of
 * field numbers starting with 0 is returned (for example, "0 1 2 ").
 *
 * @param gid           Group ID (unused).
 * @param oid           OID of the parent object instance.
 * @param sub_id        ID of the object to be listed (should be
 *                      the same as name of the file under /proc/sys/).
 * @param list          Where to save the list.
 *
 * @return Status code.
 */
static te_errno
sys_opt_list(unsigned int gid, const char *oid,
             const char *sub_id, char **list)
{
    char      path[PATH_MAX] = "";
    te_errno  rc = 0;
    int       fields_num = 0;
    int       i = 0;

    UNUSED(gid);

    *list = NULL; /* It means no instances. */

    rc = sys_opt_get_path(oid, sub_id, path, sizeof(path));
    if (rc != 0)
        return rc;

    if (access(path, R_OK) != 0)
        return 0;

    rc = read_sys_value(buf, sizeof(buf), FALSE, "%s", path);
    if (rc != 0)
        return rc;

    for (i = 0; buf[i] != '\0'; i++)
    {
        if ((i == 0 || isspace(buf[i - 1])) && !isspace(buf[i]))
            fields_num++;
    }

    if (fields_num > 1)
    {
        te_string str = TE_STRING_BUF_INIT(buf);

        for (i = 0; i < fields_num; i++)
            CHECK_NZ_RETURN(te_string_append(&str, "%d ", i));
    }
    else
    {
        buf[0] = ' ';
        buf[1] = '\0';
    }

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Parse OID of the object instance corresponding to a file under
 * /proc/sys/, obtain path to the file and field number in it.
 *
 * @param oid         OID of the configuration object instance.
 * @param path        Where to save path.
 * @param path_len    Size of buffer @p path.
 * @param field_num   Where to save field number (for files storing
 *                    multiple values separated by spaces, the first
 *                    field has number 0; if a file stores only a
 *                    single value, -1 is returned).
 *
 * @return Status code.
 */
static te_errno
sys_opt_parse_oid(const char *oid, char *path, size_t path_len,
                  int *field_num)
{
    int          len = 0;
    const char  *field = NULL;
    int          i = 0;
    te_errno     rc = 0;

    len = strlen(oid);

    rc = sys_opt_get_path(oid, "", path, path_len);
    if (rc != 0)
        return rc;

    if (oid[len - 1] != ':')
    {
        /*
         * If at the end of OID is field number (for example,
         * ../tcp_wmem:1), remove it from the end of the path.
         */

        for (i = strlen(path) - 1; i > 0; i--)
        {
            if (path[i] == '/')
                break;

            path[i] = '\0';
        }
        path[i] = '\0';

        /*
         * After that obtain pointer to field number string.
         */
        for (i = len - 1; i >= 0; i--)
        {
            if (oid[i] == ':')
            {
                field = oid + i + 1;
                break;
            }
        }
    }

    if (field == NULL)
    {
        *field_num = -1;
    }
    else
    {
        char      *endptr = NULL;
        long int   val;

        val = strtol(field, &endptr, 10);
        if (endptr == NULL || *endptr != '\0' || val < 0 || val > INT_MAX)
        {
            ERROR("%s(): incorrect field number '%s'", __FUNCTION__, field);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        *field_num = val;
    }

    return 0;
}

/**
 * Get value of configuration object instance corresponding to
 * a file under /proc/sys/.
 *
 * @param gid         Group ID (unused).
 * @param oid         OID of the configuration object instance.
 * @param value       Where to save obtained value.
 *
 * @return Status code.
 */
static te_errno
sys_opt_get(unsigned int gid, const char *oid, char *value)
{
    char         path[PATH_MAX] = "";
    int          field_num = 0;
    int          cur_field = 0;
    size_t       len = 0;
    int          i = 0;
    int          j = 0;
    te_errno     rc = 0;

    UNUSED(gid);

    *value = '\0';

    rc = sys_opt_parse_oid(oid, path, sizeof(path), &field_num);
    if (rc != 0)
        return rc;

    rc = read_sys_value(buf, sizeof(buf), FALSE, "%s", path);
    if (rc != 0)
        return rc;

    if (field_num < 0)
    {
        len = strlen(buf);
        if (len >= RCF_MAX_VAL)
        {
            ERROR("%s(): not enough space for value from '%s'",
                  __FUNCTION__, path);
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }
        memcpy(value, buf, len + 1);
    }
    else
    {
        /* Find requested field. */

        for (i = 0, cur_field = 0; buf[i] != '\0'; i++)
        {
            if (!isspace(buf[i]))
            {
                if (cur_field == field_num)
                    break;

                if (isspace(buf[i + 1]))
                    cur_field++;
            }
        }

        if (buf[i] == '\0')
        {
            ERROR("%s(): field %d was not found in '%s'",
                  __FUNCTION__, field_num, path);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        /* Copy the found field to value. */

        for (j = 0; buf[i] != '\0' && j < RCF_MAX_VAL - 1; i++, j++)
        {
            if (isspace(buf[i]))
                break;

            value[j] = buf[i];
        }

        value[j] = '\0';

        if (!isspace(buf[i]) && buf[i] != '\0')
        {
            ERROR("%s(): not enough space for field %d from '%s'",
                  __FUNCTION__, field_num, path);
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }
    }

    return 0;
}

/**
 * Set value of a configuration object instance corresponding to a file
 * under /proc/sys/.
 *
 * @param gid         Group ID (unused).
 * @param oid         OID of the configuration object instance.
 * @param value       Value to set.
 *
 * @return Status code.
 */
static te_errno
sys_opt_set(unsigned int gid, const char *oid, const char *value)
{
    char         path[PATH_MAX] = "";
    char         buf2[RCF_MAX_VAL] = "";
    int          field_num = 0;
    int          cur_field = 0;
    int          i = 0;
    int          j = 0;
    int          k = 0;
    te_errno     rc = 0;
    te_bool      pasted = FALSE;

    UNUSED(gid);

    rc = sys_opt_parse_oid(oid, path, sizeof(path), &field_num);
    if (rc != 0)
        return rc;

    if (field_num < 0)
    {
        rc = write_sys_value(value, "%s", path);
        if (rc != 0)
            return rc;
    }
    else
    {
        if (value[0] == '\0')
        {
            ERROR("%s(): trying to set empty value to "
                  "one of the fields in %s", __FUNCTION__, path);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        rc = read_sys_value(buf, sizeof(buf), FALSE, "%s", path);
        if (rc != 0)
            return rc;

        /*
         * Copy to buf2 everything from buf except for the
         * field which is replaced with specified value.
         */

        for (i = 0, j = 0, cur_field = 0;
             buf[i] != '\0' && j < RCF_MAX_VAL - 1; i++)
        {
            if (isspace(buf[i]))
            {
                buf2[j++] = buf[i];
            }
            else if (cur_field != field_num)
            {
                buf2[j++] = buf[i];

                if (isspace(buf[i + 1]))
                    cur_field++;
            }
            else
            {
                for (k = 0; value[k] != '\0' && j < RCF_MAX_VAL - 1; k++)
                    buf2[j++] = value[k];

                if (value[k] != '\0')
                {
                    ERROR("%s(): not enough space in buffer", __FUNCTION__);
                    return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
                }

                for ( ; !isspace(buf[i]) && buf[i] != '\0'; i++);

                pasted = TRUE;
                break;
            }
        }

        if (!pasted)
        {
            ERROR("%s(): failed to find field %d in %s", __FUNCTION__,
                  field_num, path);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        k = strlen(buf + i);
        if (j + k + 1 > RCF_MAX_VAL)
        {
            ERROR("%s(): not enough space in buffer", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }

        memcpy(buf2 + j, buf + i, k + 1);

        rc = write_sys_value(buf2, "%s", path);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/**
 * Register configuration objects corresponding to files
 * under a specific path in /proc/sys/.
 *
 * @param father          OID of father object.
 * @param path            Path to a directory in /proc/sys/.
 *
 * @return Status code.
 */
static te_errno
register_sys_opts(const char *father, const char *path)
{
    rcf_pch_cfg_object   *obj = NULL;
    struct dirent       **namelist = NULL;
    struct dirent        *de = NULL;
    int                   n = 0;
    int                   i = 0;
    size_t                len = 0;
    te_errno              rc = 0;

    n = scandir(path, &namelist, NULL, NULL);
    if (n < 0)
    {
        rc = te_rc_os2te(errno);

        if (rc == TE_ENOENT)
            return 0;

        ERROR("%s: failed to scan %s directory, errno %r",
              __FUNCTION__, path, rc);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    for (i = 0; i < n; i++)
    {
        de = namelist[i];

        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        if (de->d_type == DT_DIR)
            continue;

        len = strlen(de->d_name);
        if (len >= RCF_MAX_NAME)
        {
            ERROR("%s: too long name '%s'", __FUNCTION__, de->d_name);
            continue;
        }

        obj = calloc(1, sizeof(*obj));
        if (obj == NULL)
        {
            ERROR("%s: out of memory", __FUNCTION__);
            rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
            goto cleanup;
        }

        memcpy(obj->sub_id, de->d_name, len + 1);

        obj->get = (rcf_ch_cfg_get)sys_opt_get;
        obj->set = (rcf_ch_cfg_set)sys_opt_set;
        obj->list = (rcf_ch_cfg_list)sys_opt_list;

        rc = rcf_pch_add_node(father, obj);
        if (rc != 0)
        {
            free(obj);
            goto cleanup;
        }
    }

cleanup:

    for (i = 0; i < n; i++)
        free(namelist[i]);

    free(namelist);

    return rc;
}

/**
 * Unregister configuration objects corresponding to files in
 * some directory in /proc/sys/; release memory allocated for those
 * objects.
 *
 * @param father      OID of the parent object under which
 *                    the configuration objects were registered.
 *
 * @return Status code.
 */
static te_errno
unregister_sys_opts(const char *father)
{
    rcf_pch_cfg_object *father_node = NULL;
    te_errno            rc = 0;
    rcf_pch_cfg_object *tmp = NULL;
    rcf_pch_cfg_object *next = NULL;

    rc = rcf_pch_find_node(father, &father_node);
    if (rc != 0)
    {
        ERROR("%s(): failed to find '%s' in configuration tree",
              __FUNCTION__, father);
        return rc;
    }

    for (tmp = father_node->son; tmp != NULL; tmp = next)
    {
        next = tmp->brother;
        if (tmp->get == (rcf_ch_cfg_get)sys_opt_get)
        {
            rc = rcf_pch_del_node(tmp);
            if (rc != 0)
            {
                ERROR("%s(): rcf_pch_del_node() failed for '%s' "
                      "returning %r", __FUNCTION__, tmp->sub_id, rc);
                return rc;
            }
            free(tmp);
        }
    }

    return 0;
}
