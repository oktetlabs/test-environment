/** @file 
 * @brief Test Environment: RGT - log index in-memory sorting utility
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
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
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "te_defs.h"

#include "common.h"

#define MIN_BUF_SIZE    16384

te_bool
read_whole_fd(int fd, void **pbuf, size_t *psize)
{
    void       *buf;
    void       *new_buf;
    size_t      alloc;
    size_t      size;
    ssize_t     rc;

    size = 0;
    alloc = MIN_BUF_SIZE;
    buf = malloc(alloc);
    if (buf == NULL)
        return FALSE;

    while (TRUE)
    {
        rc = read(fd, buf + size, alloc - size);
        if (rc <= 0)
        {
            if (rc == 0)
                break;
            free(buf);
            return FALSE;
        }

        size += rc;
        if (size >= alloc)
        {
            alloc += alloc/2;
            new_buf = realloc(buf, alloc);
            if (new_buf == NULL)
            {
                free(buf);
                return FALSE;
            }
            buf = new_buf;
        }
    }

    if (size < alloc)
        buf = realloc(buf, size);

    *pbuf = buf;
    *psize = size;

    return TRUE;
}


te_bool
read_whole_file(const char *name, void **pbuf, size_t *psize)
{
    te_bool result;

    if (name[0] == '-' && name[1] == '\0')
        result = read_whole_fd(STDIN_FILENO, pbuf, psize);
    else
    {
        int fd;

        fd = open(name, O_RDONLY);
        if (fd < 0)
            return FALSE;

        result = read_whole_fd(fd, pbuf, psize);

        close(fd);
    }

    return result;
}


te_bool
write_whole_fd(int fd, void *buf, size_t size)
{
    ssize_t     rc;

    while (size > 0)
    {
        rc = write(fd, buf, size);
        if (rc < 0)
            return FALSE;
        buf += rc;
        size -= rc;
    }

    return TRUE;
}


te_bool
write_whole_file(const char *name, void *buf, size_t size)
{
    te_bool result;

    if (name[0] == '-' && name[1] == '\0')
        result = write_whole_fd(STDOUT_FILENO, buf, size);
    else
    {
        int fd;

        fd = open(name,
                  O_WRONLY | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR |
                  S_IRGRP | S_IWGRP |
                  S_IROTH | S_IWOTH);
        if (fd < 0)
            return FALSE;

        result = write_whole_fd(fd, buf, size);

        close(fd);
    }

    return result;
}


/** Merge list */
entry  *merge_list  = NULL;


static void
merge_sort(entry *list, size_t len)
{
    size_t  left, right;

    if (len <= 1)
        return;

    left = len/2;
    right = len - left;

    merge_sort(list, left);
    merge_sort(list + left, right);

    /* If the left half is less than or equal to the right half */
    if (memcmp(list[left - 1] + 1, list[left] + 1, sizeof(**list)) <= 0)
        /* Nothing to do */
        return;

    /* If the right half is less than or equal to the left half */
    if (memcmp(list[len - 1] + 1, list[0] + 1, sizeof(**list)) <= 0)
    {
        /* Swap them */
        memcpy(merge_list, list, left);
        memmove(list, list + left, right);
        memcpy(list + right, merge_list, left);
        return;
    }

    /*
     * Merge halves
     */
    {
        size_t  l, r, m;

        for (l = 0, r = left, m = 0; l < left && r < len; m++)
        {
            /* This condition is important to stability */
            if (memcmp(list[l] + 1, list[r] + 1, sizeof(**list)) <= 0)
                memcpy(merge_list + m, list + l++, sizeof(*list));
            else
                memcpy(merge_list + m, list + r++, sizeof(*list));
        }
        if (l < left)
            memcpy(merge_list + m, list + l, left - l);
        else if (r < len)
            memcpy(merge_list + m, list + r, len - r);
    }

    memcpy(list, merge_list, len);
}


int
run(const char *input_name, const char *output_name)
{
    int                 result      = 1;
    entry              *list        = NULL;
    size_t              size;

    if (!read_whole_file(input_name, (void **)&list, &size))
        ERROR_CLEANUP("Failed reading input: %s", strerror(errno));

    if (size % sizeof(*list) != 0)
        ERROR_CLEANUP("Invalid input length");

    merge_list = malloc(size);
    if (merge_list == NULL)
        ERROR_CLEANUP("Failed allocating memory for merge list");
    merge_sort(list, size/sizeof(*list));
    free(merge_list);
    merge_list = NULL;

    if (!write_whole_file(output_name, list, size))
        ERROR_CLEANUP("Failed writing output: %s", strerror(errno));

    result = 0;

cleanup:

    free(list);

    return result;
}


static int
usage(FILE *stream, const char *progname)
{
    return 
        fprintf(
            stream, 
            "Usage: %s [OPTION]... [INPUT [OUTPUT]]\n"
            "Sort a TE log index in memory.\n"
            "\n"
            "With no INPUT, or when INPUT is -, read standard input.\n"
            "With no OUTPUT, or when OUTPUT is -, write standard output.\n"
            "\n"
            "Options:\n"
            "  -h, --help           this help message\n"
            "\n",
            progname);
}


typedef enum opt_val {
    OPT_VAL_HELP        = 'h',
} opt_val;


int
main(int argc, char * const argv[])
{
    static const struct option  long_opt_list[] = {
        {.name      = "help",
         .has_arg   = no_argument,
         .flag      = NULL,
         .val       = OPT_VAL_HELP},
    };
    static const char          *short_opt_list = "h";

    int         c;
    const char *input_name      = "-";
    const char *output_name     = "-";

    /*
     * Read command line arguments
     */
    while ((c = getopt_long(argc, argv,
                            short_opt_list, long_opt_list, NULL)) >= 0)
    {
        switch (c)
        {
            case OPT_VAL_HELP:
                usage(stdout, program_invocation_short_name);
                return 0;
                break;
            case '?':
                usage(stderr, program_invocation_short_name);
                return 1;
                break;
        }
    }

    if (optind < argc)
    {
        input_name  = argv[optind++];
        if (optind < argc)
        {
            output_name = argv[optind++];
            if (optind < argc)
                ERROR_USAGE_RETURN("Too many arguments");
        }
    }

    /*
     * Verify command line arguments
     */
    if (*input_name == '\0')
        ERROR_USAGE_RETURN("Empty input file name");
    if (*output_name == '\0')
        ERROR_USAGE_RETURN("Empty output file name");

    /*
     * Run
     */
    return run(input_name, output_name);
}


