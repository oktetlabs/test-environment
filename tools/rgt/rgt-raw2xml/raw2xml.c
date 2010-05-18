/** @file 
 * @brief Test Environment: RGT - log dumping utility
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

#include "te_defs.h"
#include "te_raw_log.h"

/*
 *                 node
 *                  |
 *             ,----^----.
 *             |         |  
 *       internal_node   |  
 *             |         |  
 *         ,---^---.     |  
 *         |       |     |  
 * root package session test logs
 *
 * node:
 *  fields:
 *      name        - name <string>
 *      tin         - test identification <number>
 *      objective   - objective <string>
 *      page        - <string> what's this?
 *      params      - <param> list
 *      authors     - author <string> list
 *      verdicts    - verdict <string> list
 *      start       - start <timestamp>
 *      end         - end <timestamp>
 *      head        - head <chunk>
 *      body        - body <chunk>
 *      tail        - tail <chunk>
 *
 * internal_node:
 *  fields:
 *      branches    - <node> list
 *
 * root:
 *  fields:
 *      map         - ID <number> - <node> map with weak values
 */

static int
usage(FILE *stream, const char *progname)
{
    return 
        fprintf(
            stream, 
            "Usage: %s [OPTION]... [INPUT_RAW [OUTPUT_XML]]\n"
            "Convert a raw TE log file to XML.\n"
            "\n"
            "With no INPUT_RAW, or when INPUT_RAW is -, "
            "read standard input.\n"
            "With no OUTPUT_XML, or when OUTPUT_XML is -, "
            "write standard output.\n"
            "\n"
            "Options:\n"
            "  -h, --help       this help message\n"
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
    const char *input_name  = "-";
    const char *output_name = "-";

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


