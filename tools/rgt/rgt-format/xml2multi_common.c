/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Common API for rgt-xml2html-multi and rgt-xml2json.
 */

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "xml2gen.h"
#include "xml2multi_common.h"
#include "te_string.h"

/* See description in xml2multi_common.h */
void
rgt_xml2multi_opts_free(rgt_xml2multi_opts *opts)
{
    free(opts->shared_url);
    opts->shared_url = NULL;
    free(opts->docs_url);
    opts->docs_url = NULL;
    free(opts->match_id);
    opts->match_id = NULL;
}

/* See description in xml2multi_common.h */
void
rgt_xml2multi_process_cmdline(rgt_xml2multi_opts *opts,
                              poptContext con, int val)
{
    size_t len;

    switch (val)
    {
        case 'i':
        {
            char *shared_url;

            shared_url = poptGetOptArg(con);
            len = strlen(shared_url);
            if (len > 0 && shared_url[len - 1] != '/')
            {
                fprintf(stderr, "Warning: URL for shared files is not "
                        "a directory (or trailing '/' is missing)\n");
            }
            else if (len == 0)
            {
                free(shared_url);
                shared_url = NULL;
            }

            opts->shared_url = shared_url;
            break;
        }

        case 'd':
        {
            char *docs_url;

            docs_url = poptGetOptArg(con);
            len = strlen(docs_url);
            if (len > 0 && docs_url[len - 1] != '/')
            {
                fprintf(stderr, "Warning: URL for test descriptions is not "
                        "a directory (or trailing '/' is missing)\n");
            }
            else if (len == 0)
            {
                free(docs_url);
                docs_url = NULL;
            }

            opts->docs_url = docs_url;
            break;
        }

        case 'n':
        {
            char *match_exp;

            if ((match_exp = poptGetOptArg(con)) == NULL)
                usage(con, 1, "Specify node matching expression", NULL);

            if (strchr(match_exp, '_') == NULL)
            {
                if (strcmp_start(RGT_NODE_ID_PREFIX, match_exp) == 0)
                {
                    opts->match_id = strdup(match_exp +
                                            strlen(RGT_NODE_ID_PREFIX));
                    assert(opts->match_id != NULL);
                    free(match_exp);
                    opts->match_type = RGT_MATCH_NODE_ID;
                }
                else
                {
                    opts->match_id = match_exp;
                    opts->match_type = RGT_MATCH_TIN;
                }
            }
            else
            {
                sscanf(match_exp, "%u_%u",
                       &opts->match_depth,
                       &opts->match_seq);
                free(match_exp);
                opts->match_type = RGT_MATCH_DEPTH_SEQ;
            }

            opts->single_node_match = true;
            break;
        }

        case 'x':
        {
            opts->index_only = true;
            break;
        }

        case 'p':
        {
            const char *page_selector;

            if ((page_selector = poptGetOptArg(con)) == NULL)
                usage(con, 1, "Specify page selector", NULL);

            opts->page_selector_set = true;
            if (strcmp(page_selector, "all") == 0)
            {
                opts->cur_page = 0;
                opts->pages_count = 0;
            }
            else
            {
                sscanf(page_selector, "%u/%u",
                       &opts->cur_page, &opts->pages_count);
            }

            break;
        }
    }
}

/* See description in xml2multi_common.h */
bool
rgt_xml2multi_match_node(rgt_xml2multi_opts *opts,
                         const char *tin, const char *node_id,
                         uint32_t depth, uint32_t seq)
{
    if (opts->index_only)
        return false;

    if (opts->single_node_match)
    {
        switch (opts->match_type)
        {
            case RGT_MATCH_TIN:
                if (tin == NULL || opts->match_id == NULL ||
                    strcmp(opts->match_id, tin) != 0)
                    return false;
                break;

            case RGT_MATCH_NODE_ID:
                if (node_id == NULL || opts->match_id == NULL ||
                    strcmp(opts->match_id, node_id) != 0)
                    return false;
                break;

            case RGT_MATCH_DEPTH_SEQ:
                if (opts->match_depth != depth ||
                    opts->match_seq != seq)
                    return false;
                break;
        }
    }

    return true;
}

/* See description in xml2multi_common.h */
void
rgt_xml2multi_fname(char *fname, size_t len,
                    rgt_xml2multi_opts *opts,
                    rgt_gen_ctx_t *ctx,
                    rgt_depth_ctx_t *depth_ctx,
                    const char *tin, const char *node_id,
                    const char *extension)
{
    te_string page_str = TE_STRING_INIT_STATIC(256);
    te_string fname_str = TE_STRING_EXT_BUF_INIT(fname, len);
    rgt_match_type name_type;

    if (opts->page_selector_set)
    {
        if (opts->cur_page == 0)
            te_string_append(&page_str, "_all");
        else if (opts->cur_page > 1)
            te_string_append(&page_str, "_p%u", opts->cur_page);
    }

    /*
     * Default file name format. TIN is chosen here for backward
     * compatibility with old XML logs in which there is no node IDs.
     */
    if (opts->depth_seq_names)
        name_type = RGT_MATCH_DEPTH_SEQ;
    else
        name_type = RGT_MATCH_TIN;

    if (opts->single_node_match && !opts->index_only)
    {
        /*
         * If single log node was requested, use name format corresponding
         * to how that node was specified.
         */
        name_type = opts->match_type;
    }
    else if (!opts->depth_seq_names &&
             depth_ctx->type == NT_TEST &&
             node_id != NULL)
    {
        /*
         * Otherwise use node_id<NODE_ID>.<extension> format if possible.
         */
        name_type = RGT_MATCH_NODE_ID;
    }

    /*
     * Fall back to node_<DEPTH>_<SEQ>.<extension> name format if no
     * TIN or node ID is available for the desired name format.
     */
    if ((name_type == RGT_MATCH_TIN && tin == NULL) ||
        (name_type == RGT_MATCH_NODE_ID && node_id == NULL))
    {
        name_type = RGT_MATCH_DEPTH_SEQ;
    }

    switch (name_type)
    {
        case RGT_MATCH_TIN:
            te_string_append(&fname_str, "node_%s%s", tin,
                             te_string_value(&page_str));
            break;

        case RGT_MATCH_NODE_ID:
            te_string_append(&fname_str, "node_id%s%s",
                             node_id, te_string_value(&page_str));
            break;

        case RGT_MATCH_DEPTH_SEQ:
            te_string_append(&fname_str, "node_%d_%d%s",
                             ctx->depth, depth_ctx->seq,
                             te_string_value(&page_str));
            break;
    }

    te_string_append(&fname_str, ".%s", extension);
}

/* See description in xml2multi_common.h */
void
rgt_xml2multi_setup_outdir(rgt_gen_ctx_t *ctx,
                           rgt_xml2multi_opts *opts,
                           bool shared_files)
{
    int rc;
    struct stat stat_buf;
    char buf[1024];
    char prefix[PATH_MAX];
    const char *snprintf_error = "Error writing command to buffer\n";
    int n;

    if (ctx->out_fname == NULL)
        ctx->out_fname = "html";

    rc = stat(ctx->out_fname, &stat_buf);
    if (rc == -1)
    {
        if (errno != ENOENT)
        {
            perror(ctx->out_fname);
            exit(EXIT_FAILURE);
        }
        if (mkdir(ctx->out_fname, 0777) < 0)
        {
            perror(ctx->out_fname);
            exit(EXIT_FAILURE);
        }
    }
    if (stat(ctx->out_fname, &stat_buf) != 0)
    {
        perror(ctx->out_fname);
        exit(EXIT_FAILURE);
    }
    if (!S_ISDIR(stat_buf.st_mode))
    {
        fprintf(stderr, "File %s already exists and "
                "it is not a directory", ctx->out_fname);
        exit(EXIT_FAILURE);
    }

    if (chdir(ctx->out_fname) < 0)
    {
        perror(ctx->out_fname);
        exit(EXIT_FAILURE);
    }

    if (shared_files)
    {
        if (rgt_resource_files_prefix_get(NULL, NULL, sizeof(prefix), prefix))
        {
            fprintf(stderr, "Failed to get resource files path prefix\n");
            exit(EXIT_FAILURE);
        }

        if (opts->shared_url == NULL)
        {
            n = snprintf(buf, sizeof(buf), "cp %s/misc/* .", prefix);
            if (n < 0 || (size_t)n >= sizeof(buf))
            {
                fputs(snprintf_error, stderr);
                exit(EXIT_FAILURE);
            }
            system(buf);

            if (stat("images", &stat_buf) != 0)
            {
                system("mkdir images");
            }

            n = snprintf(buf, sizeof(buf), "cp %s/images/* images", prefix);
            if (n < 0 || (size_t)n >= sizeof(buf))
            {
                fputs(snprintf_error, stderr);
                exit(EXIT_FAILURE);
            }
            system(buf);
        }

        n = snprintf(buf, sizeof(buf), "for i in %s/tmpls-simple/* ; do "
                     "cat $i | sed -e 's;@@SHARED_URL@@;%s;g' "
                     "> `basename $i` ; done",
                     prefix,
                     (opts->shared_url == NULL) ? "" : opts->shared_url);
        if (n < 0 || (size_t)n >= sizeof(buf))
        {
            fputs(snprintf_error, stderr);
            exit(EXIT_FAILURE);
        }
        system(buf);
    }
}
