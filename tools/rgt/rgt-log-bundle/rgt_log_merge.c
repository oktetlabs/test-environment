/** @file 
 * @brief Test Environment: implementation of merging fragments into raw log.
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "rgt_log_bundle_common.h"

DEFINE_LGR_ENTITY("RGT LOG MERGE");

/** Whether to search log node to be merged by TIN or by depth/seq */
int use_tin = 0;
/** TIN of log node to be merged */
unsigned int filter_tin = (unsigned int)-1;
/** Depth of log node to be merged */
int filter_depth = 0;
/** Sequential number of log node to be merged */
int filter_seq = 0;
/** Number of inner fragment file to be merged */
long long int filter_frag_num = 0;

/**
 * Merge inner log fragments into "raw gist log" consisting
 * of starting and terminating fragments only.
 *
 * @param split_log_path      Where log fragments are to be found
 * @param f_raw_gist          "raw gist" log
 * @param f_frags_list        File with list of starting and terminating
 *                            fragments (in the same order they appear in
 *                            "raw gist" log)
 * @param f_result            Where to store merged raw log
 *
 * @return 0 on success, -1 on failure
 */
int
merge(const char *split_log_path,
      FILE *f_raw_gist, FILE *f_frags_list, FILE *f_result)
{
    char s[256];
    char frag_name[256];
    char frag_path[256];
    char *name_suff;

    long long unsigned int cum_length = 0;

    long long unsigned int length;
    long long unsigned int start_len;
    long long unsigned int frags_cnt;
    long long unsigned int i;
    unsigned int tin;
    int depth;
    int seq;

    off_t raw_fp;

    while (!feof(f_frags_list))
    {
        if (fgets(s, sizeof(s), f_frags_list) == NULL)
            break;
        if (sscanf(s, "%s %u %d %d %llu %llu %llu",
                   frag_name, &tin, &depth, &seq,
                   &length, &start_len, &frags_cnt) <= 0)
            break;

        cum_length += length;

        if ((name_suff = strstr(frag_name, "_start")) != NULL &&
            ((use_tin && filter_tin == tin) ||
             (!use_tin && filter_depth == depth && filter_seq == seq)))
        {
            FILE *f;
            off_t frag_len;

            cum_length -= (length - start_len);
            file2file(f_result, f_raw_gist, -1, -1, cum_length);
            raw_fp = ftello(f_raw_gist);
            raw_fp += (length - start_len);
            fseeko(f_raw_gist, raw_fp, SEEK_SET);
            cum_length = 0;

            *name_suff = '\0';
            for (i = 0; i < frags_cnt; i++)
            {
                if (filter_frag_num >= 0)
                    i = filter_frag_num;

                snprintf(frag_path, sizeof(frag_path), "%s/%s_inner_%llu",
                         split_log_path, frag_name, i);
                f = fopen(frag_path, "r");
                if (f == NULL)
                {
                    fprintf(stderr, "Failed to open %s for reading\n",
                            frag_path);
                    return -1;
                }
                fseeko(f, 0LLU, SEEK_END);
                frag_len = ftello(f);
                fseeko(f, 0LLU, SEEK_SET);
                file2file(f_result, f, -1, -1, frag_len);
                fclose(f);

                if (filter_frag_num >= 0)
                    break;
            }
        }
    }

    if (cum_length > 0)
        file2file(f_result, f_raw_gist, -1, -1, cum_length);

    return 0;
}

/** Where to find raw log fragments and "raw gist" log  */
static char *split_log_path = NULL;
/** Where to store merged raw log */
static char *output_path = NULL;

/**
 * Parse command line.
 *
 * @param argc    Number of arguments
 * @param argv    Array of command line arguments
 */
static void
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon; /* Context for parsing command-line options */
    int          rc;

    /* Option Table */
    struct poptOption optionsTable[] = {
        { "split-log", 's', POPT_ARG_STRING, NULL, 's', 
          "Path to split raw log.", NULL },

        { "filter", 'f', POPT_ARG_STRING, NULL, 'f', 
          "Either 'TIN' or 'depth_seq'", NULL },

        { "page", 'p', POPT_ARG_STRING, NULL, 'p', 
          "Either page number or 'all' to merge all pages at once", NULL },

        { "output", 'o', POPT_ARG_STRING, NULL, 'o', 
          "Where to save merged raw log.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            optionsTable, 0);

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 's')
            split_log_path = poptGetOptArg(optCon);
        else if (rc == 'f')
        {
            char *filter = poptGetOptArg(optCon);

            if (filter == NULL)
                usage(optCon, 1, "--filter value is not specified", NULL);

            if (strchr(filter, '_') != NULL)
            {
                sscanf(filter, "%d_%d", &filter_depth, &filter_seq);
                use_tin = 0;
            }
            else
            {
                sscanf(filter, "%u", &filter_tin);
                use_tin = 1;
            } 

            free(filter);
        }
        else if (rc == 'p')
        {
            char *page = poptGetOptArg(optCon);
        
            if (page == NULL)
                usage(optCon, 1, "--page value is not specified", NULL);

            if (strcasecmp(page, "all") == 0)
                filter_frag_num = -1;
            else
                filter_frag_num = atoll(page);

            free(page);
        }
        else if (rc == 'o')
            output_path = poptGetOptArg(optCon);
    }

    if (split_log_path == NULL || output_path == NULL)
        usage(optCon, 1, "Specify all the required parameters", NULL);

    if (rc < -1)
    {
        /* An error occurred during option processing */
        fprintf(stderr, "%s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        exit(1);
    }

    if (poptPeekArg(optCon) != NULL)
        usage(optCon, 1, "Too many parameters specified", NULL);

    poptFreeContext(optCon);
}

int
main(int argc, char **argv)
{
    FILE *f_raw_gist;
    FILE *f_frags_list;
    FILE *f_result;

    char buf[1024];

    process_cmd_line_opts(argc, argv);

    snprintf(buf, sizeof(buf), "%s/log_gist.raw", split_log_path);
    f_raw_gist = fopen(buf, "r");

    snprintf(buf, sizeof(buf), "%s/frags_list", split_log_path);
    f_frags_list = fopen(buf, "r");

    f_result = fopen(output_path, "w");
    if (f_result == NULL)
    {
        fprintf(stderr, "Failed to open %s for writing\n", output_path);
        return 1;
    }

    merge(split_log_path, f_raw_gist, f_frags_list, f_result);

    fclose(f_result);
    fclose(f_frags_list);
    fclose(f_raw_gist);

    free(split_log_path);
    free(output_path);

    return 0;
}
