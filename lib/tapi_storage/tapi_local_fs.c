/** @file
 * @brief Test API to local file system routines
 *
 * Functions for convenient work with the file system mapped on configurator
 * tree.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Local File System"

#include "tapi_local_fs.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "te_string.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "conf_api.h"


/* Format string for storage content in Configuration tree. */
#define TE_CFG_STORAGE_CONTENT_DIR_FMT  "/local:/env:STORAGE_CONTENT_DIR"
/* Format string for local file system entry in Configuration tree. */
#define TE_CFG_LOCAL_FS_FMT   "/local:/fs:"
/* Subid of directory item in Configurator tree. */
#define SUBID_DIR   "/directory:"
/* Subid of file item in Configurator tree. */
#define SUBID_FILE  "/file:"
/* Subid of file property item in Configurator tree. */
#define SUBID_FILE_PROPERTY "/property:"
/* Subid of file metadata item in Configurator tree. */
#define SUBID_FILE_METADATA "/metadata:"


/**
 * User data to pass to @b dump_local_fs as opaque @p user_data
 */
typedef struct dump_local_fs_opaque {
    te_string *dump;    /**< Buffer to string. */
    size_t     num;     /**< Number of files. */
} dump_local_fs_opaque;


/**
 * Dump file info to string buffer.
 *
 * @param file          Local file context.
 * @param user_data     Opaque user data which represented by structure
 *                      @b dump_local_fs_opaque.
 *
 * @return Status code.
 */
static te_errno
dump_local_fs(tapi_local_file *file, void *user_data)
{
    dump_local_fs_opaque *opaque = user_data;

    if (file->type != TAPI_FILE_TYPE_FILE)
        return 0;

    opaque->num++;
    return te_string_append(opaque->dump, " file: %s, size: %llu, tv: %u\n",
                            file->pathname, file->property.size,
                            file->property.date.tv_sec);
}

/**
 * Make pathname (concatenate name with path).
 * Return value should be freed with free when it is no longer needed.
 *
 * @param path          Path to the file.
 * @param name          Name of file.
 *
 * @return Pathname.
 */
static char *
make_pathname(const char *path, const char *name)
{
    char   *pathname;
    size_t  l;

    assert(path != NULL && name != NULL);
    l = strlen(path);
    pathname = TE_ALLOC(l + strlen(name) + 2); /* +2: '/' and '\0' */
    if (pathname != NULL)
    {
        /* Make pathname as 'path/name'. */
        strcpy(pathname, path);
        pathname[l] = '/';
        strcpy(&pathname[l + 1], name);
    }
    return pathname;
}

/**
 * Get properties of local file from configurator.
 *
 * @param[in]    localfs_path   Local file system pathname.
 * @param[in]    name           Name of file.
 * @param[inout] file           Local file.
 *
 * @return Status code.
 */
static te_errno
get_local_file_properties(const char      *localfs_path,
                          const char      *name,
                          tapi_local_file *file)
{
    te_errno  rc;
    char     *property;

    errno = 0;
    /* Retrieve a size property. */
    rc = cfg_get_instance_fmt(NULL, &property, "%s" SUBID_FILE
                              "%s" SUBID_FILE_PROPERTY "size",
                              localfs_path, name);
    if (rc != 0)
        return rc;

    file->property.size = strtoull(property, NULL, 10);
    rc = TE_OS_RC(TE_TAPI, errno);
    free(property);
    if (rc != 0)
        return rc;

    /* Retrieve a date property. */
    rc = cfg_get_instance_fmt(NULL, &property, "%s" SUBID_FILE
                              "%s" SUBID_FILE_PROPERTY "date",
                              localfs_path, name);
    if (rc != 0)
        return rc;

    file->property.date.tv_sec = strtol(property, NULL, 10);
    rc = TE_OS_RC(TE_TAPI, errno);
    free(property);
    if (rc != 0)
        return rc;

    return 0;
}

/**
 * Free local file list entry.
 *
 * @param file      Local file list entry.
 */
static void
free_local_file_le(tapi_local_file_le *file)
{
    free((char *)file->file.pathname);
    free(file);
}

/**
 * Convert regular pathname to local file system pathname.
 * For example, transform directory '/foo' to '/local:/fs:/directory:foo',
 * but file '/foo/bar' to '/local:/fs:/directory:foo/file:bar'
 * Return value should be freed with free when it is no longer needed.
 *
 * @param type          Type of local file.
 * @param pathname      Regular pathname.
 *
 * @return Local file system pathname, of @c NULL in case of failure.
 *
 * FIXME: Add escaping since configurator will get ability to escape special
 * symbols.
 */
static char *
local_fs_get_pathname(tapi_local_file_type type, const char *pathname)
{
    te_errno    rc;
    te_string   localfs = TE_STRING_INIT;
    char       *str;
    char       *token;
    char       *basename = NULL;

    str = TE_ALLOC(strlen(pathname) + 1);
    if (str == NULL)
        return NULL;
    strcpy(str, pathname);
    if (type == TAPI_FILE_TYPE_FILE)
    {
        basename = strrchr(str, '/');
        if (basename != NULL)
            *basename++ = '\0';
    }

    rc = te_string_append(&localfs, "%s", TE_CFG_LOCAL_FS_FMT);
    if (rc != 0)
    {
        free(str);
        te_string_free(&localfs);
        return NULL;
    }

    for (token = strtok(str, "/");
         token != NULL;
         token = strtok(NULL, "/"))
    {
        rc = te_string_append(&localfs, "%s%s", SUBID_DIR, token);
        if (rc != 0)
        {
            free(str);
            te_string_free(&localfs);
            return NULL;
        }
    }
    if (type == TAPI_FILE_TYPE_FILE && basename != NULL &&
        *basename != '\0')
    {
        rc = te_string_append(&localfs, "%s%s", SUBID_FILE, basename);
        if (rc != 0)
        {
            free(str);
            te_string_free(&localfs);
            return NULL;
        }
    }
    free(str);
    return localfs.ptr;
}

/**
 * Get list of local files from particular path of configurator.
 *
 * @param[in]  type             Local file type.
 * @param[in]  localfs_path     Local file system pathname.
 * @param[in]  path             Regular path to the file, it needed for
 *                              creation of pathname to avoid extract
 *                              regular path from @p localfs_path.
 * @param[out] files            Files list.
 *
 * @return Status code.
 *
 * FIXME: Add unescaping since configurator will get ability to escape
 * special symbols.
 */
static te_errno
get_local_files(tapi_local_file_type  type,
                const char           *localfs_path,
                const char           *path,
                tapi_local_file_list *files)
{
    te_errno     rc;
    cfg_handle  *handles;
    unsigned int n_handles;
    char        *name;
    size_t       i;
    tapi_local_file_le *file;

    if (type == TAPI_FILE_TYPE_DIRECTORY)
        rc = cfg_find_pattern_fmt(&n_handles, &handles, "%s" SUBID_DIR "*",
                                  localfs_path);
    else if (type == TAPI_FILE_TYPE_FILE)
        rc = cfg_find_pattern_fmt(&n_handles, &handles, "%s" SUBID_FILE "*",
                                  localfs_path);
    else
        return TE_RC(TE_TAPI, TE_EINVAL);

    if (rc != 0)
        return rc;

    for (i = 0; i < n_handles; i++)
    {
        rc = cfg_get_inst_name(handles[i], &name);
        if (rc != 0)
            return rc;
        file = TE_ALLOC(sizeof(*file));
        if (file == NULL)
            return TE_RC(TE_TAPI, TE_ENOMEM);
        file->file.type = type;
        file->file.pathname = make_pathname(path, name);
        if (file->file.pathname == NULL)
        {
            free_local_file_le(file);
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }
        if (type == TAPI_FILE_TYPE_FILE)
        {
            rc = get_local_file_properties(localfs_path, name, &file->file);
            if (rc != 0)
            {
                ERROR("Failed to get local file properties");
                free_local_file_le(file);
                return rc;
            }
        }
        SLIST_INSERT_HEAD(files, file, next);
    }
    return 0;
}


/* See description in tapi_local_fs.h. */
te_errno
tapi_local_fs_ls(const char *pathname, tapi_local_file_list *files)
{
    te_errno  rc;
    char     *localfs_path;

    assert(pathname != NULL);
    SLIST_INIT(files);
    localfs_path = local_fs_get_pathname(TAPI_FILE_TYPE_DIRECTORY,
                                         pathname);
    rc = get_local_files(TAPI_FILE_TYPE_DIRECTORY, localfs_path, pathname,
                         files);
    if (rc == 0)
        rc = get_local_files(TAPI_FILE_TYPE_FILE, localfs_path, pathname,
                             files);
    if (rc != 0)
        tapi_local_fs_list_free(files);

    free(localfs_path);
    return rc;
}

/* See description in tapi_local_fs.h. */
void
tapi_local_fs_list_free(tapi_local_file_list *files)
{
    tapi_local_file_le *file;

    while (!SLIST_EMPTY(files))
    {
        file = SLIST_FIRST(files);
        SLIST_REMOVE_HEAD(files, next);
        free_local_file_le(file);
    }
}

/* See description in tapi_local_fs.h. */
te_errno
tapi_local_fs_traverse(const char                *pathname,
                       tapi_local_fs_traverse_cb  cb_pre,
                       tapi_local_fs_traverse_cb  cb_post,
                       void                      *user_data)
{
    tapi_local_file_list  files;
    tapi_local_file_le   *f;
    te_errno              rc = 0;

    rc = tapi_local_fs_ls(pathname, &files);
    if (rc == 0)
        SLIST_FOREACH(f, &files, next)
        {
            if (cb_pre != NULL)
            {
                rc = cb_pre(&f->file, user_data);
                if (rc != 0)
                    break;
            }

            if (f->file.type == TAPI_FILE_TYPE_DIRECTORY)
            {
                rc = tapi_local_fs_traverse(f->file.pathname, cb_pre,
                                            cb_post, user_data);
                if (rc != 0)
                    break;
            }

            if (cb_post != NULL)
            {
                rc = cb_post(&f->file, user_data);
                if (rc != 0)
                    break;
            }
        }
    tapi_local_fs_list_free(&files);
    return rc;
}

/* See description in tapi_local_fs.h. */
te_errno
tapi_local_fs_get_file_metadata(const char  *pathname,
                                const char  *metaname,
                                char       **metadata)
{
    te_errno  rc;
    char     *localfs_path;

    assert(pathname != NULL && metaname != NULL);
    localfs_path = local_fs_get_pathname(TAPI_FILE_TYPE_FILE, pathname);
    if (localfs_path == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = cfg_get_instance_fmt(NULL, metadata, "%s" SUBID_FILE_METADATA
                              "%s", localfs_path, metaname);
    if (rc != 0)
        ERROR("Failed to get metadata \"%s\": %s",
              metaname, te_rc_err2str(rc));
    return rc;
}

/* See description in tapi_local_fs.h. */
char *
tapi_local_fs_get_file_real_pathname(const tapi_local_file *file,
                                     const char            *mapping_pfx)
{
    te_errno    rc = 0;
    char       *content_dir = NULL;
    char       *real_path;
    const char *sep;

    if (mapping_pfx == NULL)
    {
        /* Read STORAGE_CONTENT_DIR from the Configurator. */
        rc = cfg_get_instance_fmt(NULL, &content_dir,
                                  TE_CFG_STORAGE_CONTENT_DIR_FMT);
        if (rc != 0)
        {
            ERROR("Failed to get value of " TE_CFG_STORAGE_CONTENT_DIR_FMT);
            return NULL;
        }
        mapping_pfx = content_dir;
    }
    sep = (mapping_pfx[strlen(mapping_pfx) - 1] == '/' ||
           file->pathname[0] == '/' ? "" : "/");
    real_path = TE_ALLOC(strlen(mapping_pfx) + strlen(sep) +
                         strlen(file->pathname) + 1);
    if (real_path != NULL)
    {
        strcpy(real_path, mapping_pfx);
        strcat(real_path, sep);
        strcat(real_path, file->pathname);
    }
    free(content_dir);
    return real_path;
}

/* See description in tapi_local_fs.h. */
void
tapi_local_fs_ls_print(const char *pathname)
{
    te_string            dump = TE_STRING_INIT;
    dump_local_fs_opaque opaque = {
        .dump = &dump,
        .num = 0
    };

    te_string_append(&dump, "Local fs dump:\n");
    TAPI_LOCAL_FS_FOREACH_RECURSIVE(pathname, dump_local_fs, &opaque);
    te_string_append(&dump, "Total number of files: %u\n", opaque.num);
    RING(dump.ptr);
    te_string_free(&dump);
}
