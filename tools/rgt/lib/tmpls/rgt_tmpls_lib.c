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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rgt_tmpls_lib.h"

/* External declarations */
static void get_error_point(const char *file, unsigned long offset,
                            int *n_row, int *n_col);

/**
 * Parses themplate files and fills in an appropriate data structure.
 *
 * @param  files     An array of template files to be parsed
 * @param  tmpls     An array of internal representation of the templates
 * @param  tmpl_num  Number of templates in the arrays
 *
 * @return  Status of the operation
 *
 * @retval 0  On success
 * @retval 1  On failure
 *
 * @se  If an error occures the function output error message into stderr
 */
int
rgt_tmpls_lib_parse(const char **files, struct log_tmpl *tmpls, int tmpl_num)
{
    FILE *fd;
    int   i;
    long  fsize; /* Number of bytes in a template file */
    char *cur_ptr;
    char *var_ptr;
    char *p_tmp;

    memset(tmpls, 0, sizeof(*tmpls) * tmpl_num);

    for (i = 0; i < tmpl_num; i++)
    {
        /* Open file with a template and parse it */
        if ((fd = fopen(files[i], "r")) == NULL)
        {
            perror(files[i]);
            rgt_tmpls_lib_free(tmpls, tmpl_num);
            return 1;
        }

        fseek(fd, 0L, SEEK_END);

        /* 
         * Allocate memory under the template and plus one byte 
         * for the trailing '\0' character
         */
        tmpls[i].raw_ptr = (char *)malloc((fsize = ftell(fd)) + 1);
        if (tmpls[i].raw_ptr == NULL || 
            (tmpls[i].fname = (char *)malloc(strlen(files[i]) + 1)) == NULL)
        {
            fprintf(stderr, "Not enough memory\n");
            fclose(fd);
            rgt_tmpls_lib_free(tmpls, tmpl_num);
            return 1;
        }
        tmpls[i].raw_ptr[fsize] = '\0';
        strcpy(tmpls[i].fname, files[i]);
        fseek(fd, 0L, SEEK_SET);

        if (fread(tmpls[i].raw_ptr, 1, fsize, fd) != (size_t )fsize)
        {
            fprintf(stderr, "Cannot read the whole template %s\n", files[i]);
            fclose(fd);
            rgt_tmpls_lib_free(tmpls, tmpl_num);
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
                tmpls[i].blocks = (struct block *)realloc(tmpls[i].blocks,
                            sizeof(struct block) * (tmpls[i].n_blocks + 1));
                if (tmpls[i].blocks == NULL)
                {
                    fprintf(stderr, "Not enough memory\n");
                    if (p_tmp != NULL)
                        free(p_tmp);
                    rgt_tmpls_lib_free(tmpls, tmpl_num);
                    return 1;
                }
                tmpls[i].blocks[tmpls[i].n_blocks].blk_type = BLK_TYPE_STR;
                tmpls[i].blocks[tmpls[i].n_blocks].start_data = cur_ptr;
                tmpls[i].n_blocks++;
            }

            if (var_ptr != NULL)
            {
                char *end_var_ptr;
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
                    rgt_tmpls_lib_free(tmpls, tmpl_num);
                    return 1;
                }

                /* Check that variable name hasn't got any space characters */
                for (p_tmp = var_ptr; p_tmp != end_var_ptr; p_tmp++)
                {
                    if (isspace(*p_tmp))
                    {
                        get_error_point(files[i], p_tmp - tmpls[i].raw_ptr,
                                        &n_row, &n_col);
                        fprintf(stderr, "Variable name cannot contain any "
                                "space characters\n%s:%d:%d\n",
                                files[i], n_row, n_col);
                        rgt_tmpls_lib_free(tmpls, tmpl_num);
                        return 1;
                    }
                }

                p_tmp = (char *)tmpls[i].blocks;
                tmpls[i].blocks = (struct block *)realloc(tmpls[i].blocks,
                            sizeof(struct block) * (tmpls[i].n_blocks + 1));
                if (tmpls[i].blocks == NULL)
                {
                    fprintf(stderr, "Not enough memory\n");
                    if (p_tmp != NULL)
                        free(p_tmp);
                    rgt_tmpls_lib_free(tmpls, tmpl_num);
                    return 1;
                }
                tmpls[i].blocks[tmpls[i].n_blocks].blk_type = BLK_TYPE_VAR;
                tmpls[i].blocks[tmpls[i].n_blocks].var_name = var_ptr;
                tmpls[i].n_blocks++;
                /* Terminate variable name with '\0' */
                end_var_ptr[0] = '\0';
                cur_ptr = end_var_ptr + 2;
            }
        } while (var_ptr != NULL);
    }

    return 0;
}

/**
 * Frees internal representation of log templates
 *
 * @param  tmpls     Pointer to an array of templates
 * @param  tmpl_num  Number of templates in the array
 */
void
rgt_tmpls_lib_free(struct log_tmpl *tmpls, int tmpl_num)
{
    int i;

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
 * Outputs a template block by block into specified file.
 *
 * @param out_fd    Output file descriptor
 * @param tmpl      Pointer to a template to be output
 * @param vars      Pointer to an array of pairs (variable name, 
 *                  variable value).  Variable names have even indeces
 *                  and variable values odd ones.
 * @param user_vars TODO
 *
 * @return  Status of the operation
 *
 * @retval  0  On success
 * @retval  1  On failure
 *
 * @se  If an error occures the function output error message into stderr 
 *
 * @todo Document user_vars parameter.
 */
int
rgt_tmpls_lib_output(FILE *out_fd, struct log_tmpl *tmpl,
                     const char **vars, const char **user_vars)
{
    int i;

    for (i = 0; i < tmpl->n_blocks; i++)
    {
        if (tmpl->blocks[i].blk_type == BLK_TYPE_STR)
        {
            fprintf(out_fd, "%s", tmpl->blocks[i].start_data);
        }
        else
        {
            int j;

            for (j = 0;
                 vars != NULL && vars[j] != NULL && vars[j + 1] != NULL;
                 j += 2)
            {
                if (strcmp(vars[j], tmpl->blocks[i].var_name) == 0)
                {
                    fprintf(out_fd, "%s", vars[j + 1]);
                    break;
                }
            }

            if (vars == NULL || vars[j] == NULL || vars[j + 1] == NULL)
            {
                /* Try to find variable between user variables */
                for (j = 0;
                     user_vars != NULL && user_vars[j] != NULL &&
                     user_vars[j + 1] != NULL;
                     j += 2)
                {
                    if (strcmp(user_vars[j], tmpl->blocks[i].var_name) == 0)
                    {
                        fprintf(out_fd, "%s", user_vars[j + 1]);
                        break;
                    }
                }

                if (user_vars == NULL || user_vars[j] == NULL || 
                    user_vars[j + 1] == NULL)
                {
                    int n_row;
                    int n_col;
                
                    get_error_point(tmpl->fname,
                                    tmpl->blocks[i].var_name - tmpl->raw_ptr,
                                    &n_row, &n_col);
                    fprintf(stderr, "Variable %s isn't specified in context\n"
                            "%s:%d:%d\n",
                            tmpl->blocks[i].var_name,
                            tmpl->fname, n_row, n_col);
                    return 1;
                }
            }
        }
    }

    return 0;
}

/**
 * Determines a line and column in a file by offset from the beginning.
 *
 * @param  file    File name
 * @param  offset  Offset from the beginning of the file which line and column
 *                 to be determined
 * @param  n_row   Placeholder for line number
 * @param  n_col   Placeholder for column number
 *
 * @return  Nothing
 */
static void
get_error_point(const char *file, unsigned long offset, int *n_row, int *n_col)
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
