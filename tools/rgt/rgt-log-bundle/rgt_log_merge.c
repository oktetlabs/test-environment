/** @file
 * @brief Test Environment: implementation of merging fragments into raw log.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "te_string.h"
#include "rgt_log_bundle_common.h"

DEFINE_LGR_ENTITY("RGT LOG MERGE");

/** Whether to search log node to be merged by TIN or by depth/seq */
static int use_tin = 0;
/** TIN of log node to be merged */
static unsigned int filter_tin = (unsigned int)-1;
/** Depth of log node to be merged */
static int filter_depth = 0;
/** Sequential number of log node to be merged */
static int filter_seq = 0;
/** Number of inner fragment file to be merged */
static int64_t filter_frag_num = 0;

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
static int
merge(const char *split_log_path,
      FILE *f_raw_gist, FILE *f_frags_list, FILE *f_result)
{
    char s[DEF_STR_LEN];
    char frag_name[DEF_STR_LEN];
    char frag_path[DEF_STR_LEN];
    char *name_suff;

    uint64_t cum_length = 0;
    uint64_t length;
    uint64_t start_len;
    uint64_t frags_cnt;
    uint64_t i;

    unsigned int tin;
    int depth;
    int seq;

    off_t raw_fp;

    assert(f_raw_gist != NULL && f_frags_list != NULL && f_result != NULL);

    while (!feof(f_frags_list))
    {
        if (fgets(s, sizeof(s), f_frags_list) == NULL)
            break;
        if (sscanf(s, "%s %u %d %d %" PRIu64 " %" PRIu64 " %" PRIu64,
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

                snprintf(frag_path, sizeof(frag_path),
                         "%s/%s_inner_%" PRIu64,
                         split_log_path, frag_name, i);
                f = fopen(frag_path, "r");
                if (f == NULL)
                {
                    fprintf(stderr, "Failed to open %s for reading\n",
                            frag_path);
                    exit(1);
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

    te_string path = TE_STRING_INIT;

    process_cmd_line_opts(argc, argv);

    te_string_append(&path, "%s/log_gist.raw", split_log_path);
    f_raw_gist = fopen(path.ptr, "r");
    if (f_raw_gist == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for reading\n", path.ptr);
        exit(1);
    }
    te_string_free(&path);

    te_string_append(&path, "%s/frags_list", split_log_path);
    f_frags_list = fopen(path.ptr, "r");
    if (f_frags_list == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for reading\n", path.ptr);
        exit(1);
    }
    te_string_free(&path);

    f_result = fopen(output_path, "w");
    if (f_result == NULL)
    {
        fprintf(stderr, "Failed to open %s for writing\n", output_path);
        exit(1);
    }

    merge(split_log_path, f_raw_gist, f_frags_list, f_result);

    fclose(f_result);
    fclose(f_frags_list);
    fclose(f_raw_gist);

    free(split_log_path);
    free(output_path);

    return 0;
}
