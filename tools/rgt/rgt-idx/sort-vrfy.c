/** @file 
 * @brief Test Environment: RGT - log index sorting verification utility
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

#define BUF_SIZE    4096

int
run(const char *input_name)
{
    int         result      = 1;
    FILE       *input       = NULL;
    void       *input_buf   = NULL;
    uint64_t    offset      = 0;
    entry       prev_e      = {0, 0};
    entry       e;

    /* Open input */
    if (input_name[0] == '-' && input_name[1] == '\0')
        input = stdin;
    else
    {
        input = fopen(input_name, "r");
        if (input == NULL)
            ERROR_CLEANUP("Failed to open input: %s", strerror(errno));
    }

    /* Set input buffer */
    input_buf = malloc(BUF_SIZE);
    setvbuf(input, input_buf, _IOFBF, BUF_SIZE);

    while (fread(&e, sizeof(e), 1, input) == 1)
    {
        if (memcmp(prev_e + 1, e + 1, sizeof(*e)) > 0)
            ERROR_CLEANUP("An entry at offset %llu/0x%llX is out of order",
                          offset, offset);

        memcpy(prev_e + 1, e + 1, sizeof(*e));
        offset += sizeof(e);
    }

    if (!feof(input))
        ERROR_CLEANUP("Failed to read input at %llu: %s",
                      offset, strerror(errno));

    result = 0;

cleanup:

    if (input != NULL)
        fclose(input);
    free(input_buf);

    return result;
}


static int
usage(FILE *stream, const char *progname)
{
    return 
        fprintf(
            stream, 
            "Usage: %s [OPTION]... [INPUT]\n"
            "Verify a TE log index sorting order.\n"
            "\n"
            "With no INPUT, or when INPUT is -, read standard input.\n"
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
        input_name  = argv[optind++];

    /*
     * Verify command line arguments
     */
    if (*input_name == '\0')
        ERROR_USAGE_RETURN("Empty input file name");

    /*
     * Run
     */
    return run(input_name);
}


