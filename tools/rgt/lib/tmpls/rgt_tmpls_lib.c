/** @file
 * @brief Test Environment: Generic interface for working with
 * output templates.
 *
 * The module provides functions for parsing and output templates
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#include "rgt_tmpls_lib.h"

/* External declarations */
static void get_error_point(const char *file, unsigned long offset,
                            int *n_row, int *n_col);


/** Array of Rgt attributes */
static rgt_attrs_t global_attrs[20];
#define ATTR_NUM (sizeof(global_attrs) / sizeof(global_attrs[0]))

/** The number of attributes in attribute array */
static unsigned int global_attr_num = 0;
/** Wheter the attribute array is currently used or not */
static te_bool      attr_locked = FALSE;

#define N_BUFS 10
#define BUF_LEN 128
/** The array of buffers for keeping string value attributes */
static char bufs[N_BUFS][BUF_LEN];
/** Currently used buffer */
static int cur_buf = 0;

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
void
rgt_tmpls_attrs_add_fstr(rgt_attrs_t *attrs, const char *name,
                         const char *fmt_str, ...)
{
    va_list ap;

    assert(attr_locked == TRUE);
    assert(global_attr_num + 2 <= ATTR_NUM);
    assert(cur_buf + 1 <= N_BUFS);
    assert(attrs == global_attrs);
    
    va_start(ap, fmt_str);

    global_attrs[global_attr_num].type = RGT_ATTR_TYPE_STR;
    global_attrs[global_attr_num].name = name;
    vsnprintf(bufs[cur_buf], BUF_LEN, fmt_str, ap);
    global_attrs[global_attr_num].str_val = bufs[cur_buf];

    va_end(ap);
    
    cur_buf++;
    global_attr_num++;

    global_attrs[global_attr_num].type = RGT_ATTR_TYPE_UNKNOWN;
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
int
rgt_tmpls_output(FILE *out_fd, rgt_tmpl_t *tmpl, const rgt_attrs_t *attrs)
{
    int i;

    for (i = 0; i < tmpl->n_blocks; i++)
    {
        if (tmpl->blocks[i].type == RGT_BLK_TYPE_CSTR)
        {
            fprintf(out_fd, "%s", tmpl->blocks[i].start_data);
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
                            fprintf(out_fd, tmpl->blocks[i].var.fmt_str,
                                    attrs[j].str_val);
                            break;

                        case RGT_ATTR_TYPE_UINT32:
                            fprintf(out_fd, tmpl->blocks[i].var.fmt_str,
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

/* The description see in rgt_tmpls_lib.h */
int
rgt_tmpls_parse(const char **files, rgt_tmpl_t *tmpls, size_t tmpl_num)
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
        /* Open file with a template and parse it */
        if ((fd = fopen(files[i], "r")) == NULL)
        {
            perror(files[i]);
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
            (tmpls[i].fname = strdup(files[i])) == NULL)
        {
            fprintf(stderr, "Not enough memory\n");
            fclose(fd);
            rgt_tmpls_free(tmpls, tmpl_num);
            return 1;
        }
        tmpls[i].raw_ptr[fsize] = '\0';
        strcpy(tmpls[i].fname, files[i]);
        fseek(fd, 0L, SEEK_SET);

        if (fread(tmpls[i].raw_ptr, 1, fsize, fd) != (size_t )fsize)
        {
            fprintf(stderr, "Cannot read the whole template %s\n",
                    files[i]);
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
                    get_error_point(files[i], var_ptr - tmpls[i].raw_ptr,
                                    &n_row, &n_col);

                    fprintf(stderr, "Cannot find trailing %s marker for "
                            "variable started at %s:%d:%d\n",
                            RGT_TMPLS_VAR_DELIMETER, files[i],
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
                    get_error_point(files[i], fmt_ptr - tmpls[i].raw_ptr,
                                    &n_row, &n_col);
                    fprintf(stderr, "Cannot get format string or "
                            "variable name at %s:%d:%d\n",
                            files[i], n_row, n_col);
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
                        get_error_point(files[i], p_tmp - tmpls[i].raw_ptr,
                                        &n_row, &n_col);
                        fprintf(stderr, "Variable name cannot contain any "
                                "space characters\n%s:%d:%d\n",
                                files[i], n_row, n_col);
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
