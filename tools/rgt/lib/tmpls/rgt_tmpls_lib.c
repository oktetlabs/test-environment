/** @file
 * @brief Test Environment: Generic interface for working with
 * output templates.
 *
 * The module provides functions for parsing and output templates
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <libgen.h>
#include <unistd.h>
#include "te_errno.h"
#include "te_string.h"

#include "rgt_tmpls_lib.h"
#include "rgt_which.h"

/* External declarations */
static void get_error_point(const char *file, unsigned long offset,
                            int *n_row, int *n_col);

/* Max attribute length in one line */
static int rgt_max_attribute_length = 0;
/* A tag to separate lines */
static const char *rgt_line_separator = NULL;

/** Array of Rgt attributes */
static rgt_attrs_t global_attrs[32];
#define ATTR_NUM (sizeof(global_attrs) / sizeof(global_attrs[0]))

/** Array of saved Rgt attributes */
static rgt_attrs_t saved_attrs[ATTR_NUM];

/** The number of attributes in attribute array */
static unsigned int global_attr_num = 0;
/** Wheter the attribute array is currently used or not */
static te_bool      attr_locked = FALSE;

#define N_BUFS ATTR_NUM
#define BUF_LEN 128
/** The array of buffers for keeping string value attributes */
static char bufs[N_BUFS][BUF_LEN];
/** Currently used buffer */
static unsigned int cur_buf = 0;

/** Type of an output function (like fprintf()) */
typedef int (*write_func)(void *arg, ...);

/* The description see in rgt_tmpls_lib.h */
rgt_attrs_t *
rgt_tmpls_attrs_new(const char **xml_attrs)
{
    unsigned int i, j;

    assert(attr_locked == FALSE);
    attr_locked = TRUE;

    global_attr_num = 0;

    if (xml_attrs == NULL)
    {
        global_attrs[0].type = RGT_ATTR_TYPE_UNKNOWN;
        return global_attrs;
    }

    for (i = 0, j = 0;
         xml_attrs != NULL && xml_attrs[j] != NULL &&
         xml_attrs[j + 1] != NULL;
         i++, j += 2)
    {
        if (i + 2 >= ATTR_NUM)
            assert(0);

        global_attrs[i].type = RGT_ATTR_TYPE_STR;
        global_attrs[i].name = xml_attrs[j];
        global_attrs[i].str_val = xml_attrs[j + 1];
    }

    global_attrs[i].type = RGT_ATTR_TYPE_UNKNOWN;

    global_attr_num = i;
    if (i == ATTR_NUM)
        assert(0);

    return global_attrs;
}

/* The description see in rgt_tmpls_lib.h */
void
rgt_tmpls_attrs_free(rgt_attrs_t *attrs)
{
    assert(attr_locked == TRUE);
    assert(attrs == global_attrs);
    cur_buf = 0;
    attr_locked = FALSE;
}

/* The description see in rgt_tmpls_lib.h */
rgt_attrs_t *
rgt_tmpls_attrs_save(rgt_attrs_t *attrs)
{
    int i;

    assert(attrs == global_attrs);

    for (i = 0; i < ATTR_NUM; i++)
    {
        saved_attrs[i] = attrs[i];
        if (attrs[i].type == RGT_ATTR_TYPE_UNKNOWN)
            break;
        if (attrs[i].type == RGT_ATTR_TYPE_STR && attrs[i].str_val != NULL)
            saved_attrs[i].str_val = strdup(attrs[i].str_val);
    }

    return saved_attrs;
}

/* The description see in rgt_tmpls_lib.h */
void
rgt_tmpls_attrs_saved_free(rgt_attrs_t *attrs)
{
    int i;

    assert(attrs == saved_attrs);

    for (i = 0; i < ATTR_NUM && attrs[i].type != RGT_ATTR_TYPE_UNKNOWN; i++)
    {
        if (attrs[i].type == RGT_ATTR_TYPE_STR)
            free((void*)attrs[i].str_val);
    }
}

/**
 * Adds a new string attribute in the list of attributes.
 *
 * @param attrs    Rgt attributes list
 * @param name     A new attribute name
 * @param fmt_str  Format string for the attribute value
 * @param ap       List of values connected with @p fmt_str
 */
static void
attrs_add_fstr(rgt_attrs_t *attrs, const char *name,
               const char *fmt_str, va_list ap)
{
    assert(attr_locked == TRUE);
    assert(global_attr_num + 2 <= ATTR_NUM);
    assert(cur_buf + 1 <= N_BUFS);
    assert(attrs == global_attrs);

    global_attrs[global_attr_num].type = RGT_ATTR_TYPE_STR;
    global_attrs[global_attr_num].name = name;
    vsnprintf(bufs[cur_buf], BUF_LEN, fmt_str, ap);
    global_attrs[global_attr_num].str_val = bufs[cur_buf];

    cur_buf++;
    global_attr_num++;

    global_attrs[global_attr_num].type = RGT_ATTR_TYPE_UNKNOWN;
}

/* The description see in rgt_tmpls_lib.h */
void
rgt_tmpls_attrs_add_fstr(rgt_attrs_t *attrs, const char *name,
                         const char *fmt_str, ...)
{
    va_list ap;

    va_start(ap, fmt_str);
    attrs_add_fstr(attrs, name, fmt_str, ap);
    va_end(ap);
}

/* The description see in rgt_tmpls_lib.h */
void
rgt_tmpls_attrs_set_fstr(rgt_attrs_t *attrs, const char *name,
                         const char *fmt_str, ...)
{
    va_list  ap;
    uint32_t i;

    assert(attr_locked == TRUE);
    assert(global_attr_num + 2 <= ATTR_NUM);
    assert(attrs == global_attrs);

    va_start(ap, fmt_str);

    for (i = 0; i < global_attr_num; i++)
    {
        if (strcmp(global_attrs[i].name, name) == 0)
        {
            global_attrs[i].type = RGT_ATTR_TYPE_STR;
            vsnprintf((char *)global_attrs[i].str_val,
                      BUF_LEN, fmt_str, ap);
            va_end(ap);
            return;
        }
    }

    /* we haven't found the attribute, use add instead */
    attrs_add_fstr(attrs, name, fmt_str, ap);
    va_end(ap);
}

/* The description see in rgt_tmpls_lib.h */
void
rgt_tmpls_attrs_add_uint32(rgt_attrs_t *attrs, const char *name,
                           uint32_t val)
{
    assert(attr_locked == TRUE);
    assert(global_attr_num + 2 <= ATTR_NUM);
    assert(attrs == global_attrs);

    global_attrs[global_attr_num].type = RGT_ATTR_TYPE_UINT32;
    global_attrs[global_attr_num].name = name;
    global_attrs[global_attr_num].uint32_val = val;

    global_attr_num++;

    global_attrs[global_attr_num].type = RGT_ATTR_TYPE_UNKNOWN;
}

/* The description see in rgt_tmpls_lib.h */
void
rgt_tmpls_attrs_set_uint32(rgt_attrs_t *attrs, const char *name,
                           uint32_t val)
{
    uint32_t i;

    assert(attr_locked == TRUE);
    assert(global_attr_num + 2 <= ATTR_NUM);
    assert(attrs == global_attrs);

    for (i = 0; i < global_attr_num; i++)
    {
        if (strcmp(global_attrs[i].name, name) == 0)
        {
            global_attrs[i].type = RGT_ATTR_TYPE_UINT32;
            global_attrs[i].uint32_val = val;
            return;
        }
    }

    /* we haven't found the attribute, use add instead */
    rgt_tmpls_attrs_add_uint32(attrs, name, val);
}

/**
 * Check the attribute length. In case It is too long, new line tags will
 * be iserted.
 *
 * @param str   Attribute string
 *
 * @return A buffer with new string or NULL if the string was not changed
 */
static char *
rgt_check_string_len(const char *str)
{
    int         max_len = rgt_max_attribute_length;
    int         clen    = 0;
    int         len     = strlen(str);
    int         offt;
    const char *tag     = rgt_line_separator;
    int         tagl;
    char       *buf;

    if (rgt_max_attribute_length == 0)
        return NULL;

    tagl = strlen(tag);

    if (len > max_len)
        buf = malloc(len + len/max_len * tagl + 1);
    else
        return NULL;
    assert(buf != NULL);

    offt = 0;
    while (len - clen > max_len)
    {
        memcpy(buf + offt, str + clen, max_len);
        offt += max_len;
        clen += max_len;
        memcpy(buf + offt, tag, tagl);
        offt += tagl;
    }
    memcpy(buf + offt, str + clen, len - clen);
    offt += len - clen;
    buf[offt] = '\0';
    return buf;
}

/**
 * Output a template block by block with a specified output function.
 *
 * @param func        Output function (like fprintf()).
 * @param dest        Destination (FILE, etc).
 * @param tmpl        Pointer to a template to be output.
 * @param attrs       Pointer to an array of attributes for that template.
 *
 * @return @c 0 on success, @c 1 on failure.
 */
static int
rgt_tmpls_output_gen(write_func func, void *dest, rgt_tmpl_t *tmpl,
                     const rgt_attrs_t *attrs)
{
    int   i;
    char *buf;

    if (dest == NULL)
        return 0;

    for (i = 0; i < tmpl->n_blocks; i++)
    {
        if (tmpl->blocks[i].type == RGT_BLK_TYPE_CSTR)
        {
            func(dest, "%s", tmpl->blocks[i].start_data);
        }
        else
        {
            int j = 0;

            for (j = 0; attrs[j].type != RGT_ATTR_TYPE_UNKNOWN; j++)
            {
                if (strcmp(attrs[j].name, tmpl->blocks[i].var.name) == 0)
                {
                    switch (attrs[j].type)
                    {
                        case RGT_ATTR_TYPE_STR:
                            buf = rgt_check_string_len(attrs[j].str_val);
                            if (buf == NULL)
                            {
                                func(dest, tmpl->blocks[i].var.fmt_str,
                                     attrs[j].str_val);
                            }
                            else
                            {
                                func(dest, tmpl->blocks[i].var.fmt_str,
                                     buf);
                                free(buf);
                            }
                            break;

                        case RGT_ATTR_TYPE_UINT32:
                            func(dest, tmpl->blocks[i].var.fmt_str,
                                 attrs[j].uint32_val);
                            break;

                        default:
                            assert(0);
                    }
                    break;
                }
            }

            if (attrs[j].type == RGT_ATTR_TYPE_UNKNOWN)
            {
                int n_row;
                int n_col;

                get_error_point(tmpl->fname,
                                tmpl->blocks[i].var.name -
                                tmpl->raw_ptr,
                                &n_row, &n_col);
                fprintf(stderr, "Variable %s isn't specified in "
                                "context\n"
                                "%s:%d:%d\n",
                        tmpl->blocks[i].var.name,
                        tmpl->fname, n_row, n_col);
                return 1;
            }
        }
    }

    return 0;
}

/* The description see in rgt_tmpls_lib.h */
int
rgt_tmpls_output(FILE *out_fd, rgt_tmpl_t *tmpl, const rgt_attrs_t *attrs)
{
    return rgt_tmpls_output_gen((write_func)&fprintf, out_fd, tmpl, attrs);
}

/* The description see in rgt_tmpls_lib.h */
int
rgt_tmpls_output_str(te_string *str, rgt_tmpl_t *tmpl,
                     const rgt_attrs_t *attrs)
{
    return rgt_tmpls_output_gen((write_func)&te_string_append, str, tmpl,
                                attrs);
}

/* The description see in rgt_tmpls_lib.h */
void
rgt_tmpls_free(rgt_tmpl_t *tmpls, size_t tmpl_num)
{
    size_t i;

    for (i = 0; i < tmpl_num; i++)
    {
        if (tmpls[i].raw_ptr != NULL)
            free(tmpls[i].raw_ptr);
        if (tmpls[i].fname != NULL)
            free(tmpls[i].fname);
        if (tmpls[i].blocks != NULL)
            free(tmpls[i].blocks);
    }
}

/**
 * Call chdir(), report error if it occurred.
 * This function helps to avoid compiler warning about not checked
 * return value.
 *
 * @param path      Path for chdir().
 *
 * @return Result of chdir().
 */
static int
chdir_report_error(const char *path)
{
    int rc = chdir(path);

    if (rc < 0)
    {
        fprintf(stderr, "Failed to chdir() to '%s', errno=%d ('%s')\n",
                path, errno, strerror(errno));
    }

    return rc;
}

/* The description see in rgt_tmpls_lib.h */
int
rgt_resource_files_prefix_get(const char *util_path, const char *argv0,
                              size_t size, char *prefix)
{
    static char prefix_static[PATH_MAX] = {0};
    static te_bool got_prefix = FALSE;

    const char *getcwd_error = "getcwd failed";
    const char *snprintf_error = "error writing path to buffer";
    int n;

    if (prefix == NULL)
    {
        fprintf(stderr, "%s: error passing NULL to prefix pointer\n",
                __FUNCTION__);
        return 1;
    }

    if (!got_prefix)
    {
        /* relative path from install dir */
        const char *datadir_path = "share";
        char argv0_copy[PATH_MAX] = {0};
        char path_buf[PATH_MAX] = {0};
        char original_path[PATH_MAX] = {0};
        const char *chdir_steps[] = {path_buf, "..", datadir_path, util_path};
        size_t i;

        if (argv0 == NULL || util_path == NULL)
        {
            fprintf(stderr, "%s: error passing NULL argument without "
                    "previous initialization\n", __FUNCTION__);
            return 1;
        }

        if (strchr(argv0, '/') != NULL ||
            rgt_which(argv0, sizeof(path_buf), path_buf))
        {
            n = snprintf(argv0_copy, sizeof(argv0_copy), "%s", argv0);

            if (n < 0 || sizeof(argv0_copy) <= (size_t)n)
            {
                fprintf(stderr, "%s: %s\n", __FUNCTION__, snprintf_error);
                return 1;
            }

            n = snprintf(path_buf, sizeof(path_buf), "%s", dirname(argv0_copy));

            if (n < 0 || sizeof(path_buf) <= (size_t)n)
            {
                fprintf(stderr, "%s: %s\n", __FUNCTION__, snprintf_error);
                return 1;
            }

        }

        if (getcwd(original_path, sizeof(original_path)) == NULL)
        {
            fprintf(stderr, "%s: %s\n", __FUNCTION__, getcwd_error);
            return 1;
        }

        for (i = 0; i < TE_ARRAY_LEN(chdir_steps); ++i)
        {
            if (chdir_report_error(chdir_steps[i]) < 0)
            {
                fprintf(stderr, "%s: error accessing directory: %s "
                        "from path: %s/../%s/%s\n", __FUNCTION__,
                        chdir_steps[i], path_buf, datadir_path, util_path);
                chdir_report_error(original_path);
                return 1;
            }
        }

        if (getcwd(prefix_static, sizeof(prefix_static)) == NULL)
        {
            fprintf(stderr, "%s: %s\n", __FUNCTION__, getcwd_error);
            chdir_report_error(original_path);
            return 1;
        }

        if (chdir_report_error(original_path) < 0)
        {
            fprintf(stderr, "%s: error returning to the original path: %s\n",
                    __FUNCTION__, original_path);
            return 1;
        }

        got_prefix = TRUE;
    }

    n = snprintf(prefix, size, "%s", prefix_static);
    if (n < 0 || size <= (size_t)n)
    {
        fprintf(stderr, "%s: %s\n", __FUNCTION__, snprintf_error);
        return 1;
    }

    return 0;
}

/* The description see in rgt_tmpls_lib.h */
int
rgt_tmpls_parse(const char **files, const char *prefix,
                rgt_tmpl_t *tmpls, size_t tmpl_num)
{
    FILE   *fd;
    size_t  i;
    long    fsize; /* Number of bytes in a template file */
    char   *cur_ptr;
    char   *var_ptr;
    char   *p_tmp;

    memset(tmpls, 0, sizeof(*tmpls) * tmpl_num);

    for (i = 0; i < tmpl_num; i++)
    {
        char tmpl_path[PATH_MAX];
        int n;

        /* Prefix path to a template file */
        n = snprintf(tmpl_path, sizeof(tmpl_path), "%s/tmpls/%s",
                     prefix, files[i]);

        if (n < 0 || (size_t)n >= sizeof(tmpl_path)) {
            fputs("Cannot handle file path of this length\n", stderr);
            rgt_tmpls_free(tmpls, tmpl_num);
            return 1;
        }

        /* Open file with a template and parse it */
        if ((fd = fopen(tmpl_path, "r")) == NULL)
        {
            perror(tmpl_path);
            rgt_tmpls_free(tmpls, tmpl_num);
            return 1;
        }

        fseek(fd, 0L, SEEK_END);

        /*
         * Allocate memory under the template and plus one byte
         * for the trailing '\0' character
         */
        tmpls[i].raw_ptr = (char *)malloc((fsize = ftell(fd)) + 1);
        if (tmpls[i].raw_ptr == NULL ||
            (tmpls[i].fname = strdup(tmpl_path)) == NULL)
        {
            fprintf(stderr, "Not enough memory\n");
            fclose(fd);
            rgt_tmpls_free(tmpls, tmpl_num);
            return 1;
        }
        tmpls[i].raw_ptr[fsize] = '\0';
        strcpy(tmpls[i].fname, tmpl_path);
        fseek(fd, 0L, SEEK_SET);

        if (fread(tmpls[i].raw_ptr, 1, fsize, fd) != (size_t )fsize)
        {
            fprintf(stderr, "Cannot read the whole template %s\n",
                    tmpl_path);
            fclose(fd);
            rgt_tmpls_free(tmpls, tmpl_num);
            return 1;
        }
        fclose(fd);

        tmpls[i].blocks   = NULL;
        tmpls[i].n_blocks = 0;

        /* Parse the content of the template */
        cur_ptr = tmpls[i].raw_ptr;

        do {
            var_ptr = strstr(cur_ptr, RGT_TMPLS_VAR_DELIMETER);

            if (var_ptr != cur_ptr && *cur_ptr != '\0')
            {
                /* Create constant string block */
                p_tmp = (char *)tmpls[i].blocks;
                tmpls[i].blocks = (rgt_blk_t *)realloc(tmpls[i].blocks,
                            sizeof(rgt_blk_t) * (tmpls[i].n_blocks + 1));
                if (tmpls[i].blocks == NULL)
                {
                    fprintf(stderr, "Not enough memory\n");
                    if (p_tmp != NULL)
                        free(p_tmp);
                    rgt_tmpls_free(tmpls, tmpl_num);
                    return 1;
                }
                tmpls[i].blocks[tmpls[i].n_blocks].type = RGT_BLK_TYPE_CSTR;
                tmpls[i].blocks[tmpls[i].n_blocks].start_data = cur_ptr;
                tmpls[i].n_blocks++;
            }

            if (var_ptr != NULL)
            {
                char *end_var_ptr;
                char *fmt_ptr;
                int   n_row;
                int   n_col;

                /* Terminate a constant string on the first "@" character */
                var_ptr[0] = '\0';
                var_ptr += 2;
                end_var_ptr = strstr(var_ptr, RGT_TMPLS_VAR_DELIMETER);

                if (end_var_ptr == NULL)
                {
                    get_error_point(tmpl_path, var_ptr - tmpls[i].raw_ptr,
                                    &n_row, &n_col);

                    fprintf(stderr, "Cannot find trailing %s marker for "
                            "variable started at %s:%d:%d\n",
                            RGT_TMPLS_VAR_DELIMETER, tmpl_path,
                            n_row, n_col);
                    rgt_tmpls_free(tmpls, tmpl_num);
                    return 1;
                }

                /* Terminate variable name with '\0' */
                end_var_ptr[0] = '\0';

                fmt_ptr = var_ptr;

                /* Get variable name */
                var_ptr = strrchr(fmt_ptr, ':');
                if (var_ptr == NULL || var_ptr[1] == '\0')
                {
                    get_error_point(tmpl_path, fmt_ptr - tmpls[i].raw_ptr,
                                    &n_row, &n_col);
                    fprintf(stderr, "Cannot get format string or "
                            "variable name at %s:%d:%d\n",
                            tmpl_path, n_row, n_col);
                    rgt_tmpls_free(tmpls, tmpl_num);
                    return 1;
                }
                *var_ptr = '\0';
                var_ptr++;

                /*
                 * Check that variable name hasn't got any space
                 * characters
                 */
                for (p_tmp = var_ptr; p_tmp != end_var_ptr; p_tmp++)
                {
                    if (isspace(*p_tmp))
                    {
                        get_error_point(tmpl_path, p_tmp - tmpls[i].raw_ptr,
                                        &n_row, &n_col);
                        fprintf(stderr, "Variable name cannot contain any "
                                "space characters\n%s:%d:%d\n",
                                tmpl_path, n_row, n_col);
                        rgt_tmpls_free(tmpls, tmpl_num);
                        return 1;
                    }
                }

                p_tmp = (char *)tmpls[i].blocks;
                tmpls[i].blocks = (rgt_blk_t *)realloc(tmpls[i].blocks,
                            sizeof(rgt_blk_t) * (tmpls[i].n_blocks + 1));
                if (tmpls[i].blocks == NULL)
                {
                    fprintf(stderr, "Not enough memory\n");
                    if (p_tmp != NULL)
                        free(p_tmp);
                    rgt_tmpls_free(tmpls, tmpl_num);
                    return 1;
                }
                tmpls[i].blocks[tmpls[i].n_blocks].type = RGT_BLK_TYPE_VAR;
                tmpls[i].blocks[tmpls[i].n_blocks].var.name = var_ptr;
                tmpls[i].blocks[tmpls[i].n_blocks].var.fmt_str = fmt_ptr;
                tmpls[i].n_blocks++;
                cur_ptr = end_var_ptr + 2;
            }
        } while (var_ptr != NULL);
    }

    return 0;
}

/* The description see in rgt_tmpls_lib.h */
const char *
rgt_tmpls_xml_attrs_get(const char **xml_attrs, const char *name)
{
    int i;

    for (i = 0;
         xml_attrs != NULL && xml_attrs[i] != NULL &&
         xml_attrs[i + 1] != NULL;
         i += 2)
    {
        if (strcmp(xml_attrs[i], name) == 0)
            return xml_attrs[i + 1];
    }

    return NULL;
}

/* The description see in rgt_tmpls_lib.h */
void
rgt_attr_settings_init(const char *sep, int length)
{
    rgt_line_separator = sep;
    rgt_max_attribute_length = length;
}

/**
 * Determines a line and column in a file by offset from the beginning.
 *
 * @param  file    File name
 * @param  offset  Offset from the beginning of the file which line and
 *                 column to be determined
 * @param  n_row   Placeholder for line number
 * @param  n_col   Placeholder for column number
 *
 * @return  Nothing
 */
static void
get_error_point(const char *file, unsigned long offset, int *n_row,
                int *n_col)
{
    unsigned long  n_total = 0;
    int            ch;
    FILE          *fd;

    *n_row = 1;
    *n_col = 0;

    if ((fd = fopen(file, "r")) == NULL)
        return;

    while ((ch = fgetc(fd)) != EOF)
    {
        if (ch == '\n')
        {
            (*n_row)++;
            *n_col = 0;
        }
        else
        {
            (*n_col)++;
        }

        if (n_total++ == offset)
            return;
    }
}
