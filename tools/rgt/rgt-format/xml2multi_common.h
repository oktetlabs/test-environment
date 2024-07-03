/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Common declarations for rgt-xml2html-multi and rgt-xml2json.
 */

#ifndef __TE_RGT_XML2MULTI_COMMON_H__
#define __TE_RGT_XML2MULTI_COMMON_H__

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <popt.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Note: option values from the table in process_cmd_line_opts() from
 * xml2fmt_core.c must not be reused here.
 */

/** Common options for rgt-xml2html-multi and rgt-xml2json */
#define XML2MULTI_COMMON_OPTS \
    { "docs-url", 'd', POPT_ARG_STRING, NULL, 'd',          \
      "URL of directory with test descriptions", NULL },    \
    { "single-node", 'n', POPT_ARG_STRING, NULL, 'n',       \
      "Output only specified log node.", NULL },            \
    { "page-selector", 'p', POPT_ARG_STRING, NULL, 'p',     \
      "Show page selector.", NULL },

/** Options specific to rgt-xml2html-multi */
#define XML2MULTI_HTML_SPECIFIC_OPTS \
    { "shared-url", 'i', POPT_ARG_STRING, NULL, 'i',              \
      "URL of directory with shared files (images etc.)", NULL }, \
    { "index-only", 'x', POPT_ARG_NONE, NULL, 'x',                \
      "Output only index pages.", NULL },

/** Prefix used for node ID in log file name */
#define RGT_NODE_ID_PREFIX "id"

/** Storage for parsed command line options */
typedef struct rgt_xml2multi_opts {
    /**
     * URL for common files (images, styles etc.).
     * If this value is NULL, all of the files are copied
     * from the tool installation directory to report output
     * directory.
     */
    char *shared_url;

    /** Base URL for doxygen-generated documentation for tests */
    char *docs_url;

    /**
     * Name all files by depth and sequence numbers in tree,
     * including test iteration nodes. If this is turned off,
     * then test iteration nodes will be named by node ID.
     * If this is turned on, references to logs files in
     * TRC report will be broken.
     */
    bool depth_seq_names;

    /** If @c true, output HTML index files only */
    bool index_only;
    /** If @c true, output log only for specified log node */
    bool single_node_match;
    /** How a single log node was specified */
    rgt_match_type match_type;
    /** ID of log node */
    char *match_id;
    /** Depth of log node */
    uint32_t match_depth;
    /** Sequential number of log node */
    uint32_t match_seq;

    /** Output page selector allowing to select page of large HTML log */
    bool page_selector_set;
    /** Current page number */
    uint32_t cur_page;
    /** Total pages count */
    uint32_t pages_count;
} rgt_xml2multi_opts;

/** Initializer for rgt_xml2multi_opts */
#define RGT_XML2MULTI_OPTS_INIT { .depth_seq_names = false, }

/* Root log node depth in the tree of log nodes */
#define ROOT_NODE_DEPTH   1
/* Root log node sequential number */
#define ROOT_NODE_SEQ     0

/**
 * Release memory allocated for rgt_xml2multi_opts.
 *
 * @param opts        Pointer to options structure
 */
extern void rgt_xml2multi_opts_free(rgt_xml2multi_opts *opts);

/**
 * Parse command line options and store them in rgt_xml2multi_opts.
 *
 * @param opts        Options storage
 * @param con         popt context
 * @param val         Option identifier
 */
extern void rgt_xml2multi_process_cmdline(rgt_xml2multi_opts *opts,
                                          poptContext con, int val);

/**
 * Check whether a given log node should be output.
 *
 * @param ctx       Context with matching parameters
 * @param tin       TIN of test iteration represented by this node
 * @param node_id   Node ID
 * @param depth     Node depth in log tree
 * @param seq       Node sequential number (in the list of children of its
 *                  parent)
 *
 * @return @c true if node should be output, @c false otherwise.
 */
extern bool rgt_xml2multi_match_node(rgt_xml2multi_opts *opts,
                                        const char *tin,
                                        const char *node_id,
                                        uint32_t depth,
                                        uint32_t seq);

/**
 * Obtain file name to use for a given log node.
 *
 * @param fname       Buffer for file name
 * @param len         Length of the buffer
 * @param opts        Command line options
 * @param ctx         Generic rgt-format context
 * @param depth_ctx   Log depth specific rgt-format context
 * @param tin         Test iteration number
 * @param node_id     Log node identifier
 * @param extension   File extension (e.g. "html" or "json")
 */
extern void rgt_xml2multi_fname(char *fname, size_t len,
                                rgt_xml2multi_opts *opts,
                                rgt_gen_ctx_t *ctx,
                                rgt_depth_ctx_t *depth_ctx,
                                const char *tin, const char *node_id,
                                const char *extension);

/**
 * Set up output directory.
 *
 * @param ctx             Generic rgt-format context
 * @param opts            Command line options
 * @param shared_files    If @c true, there are some shared files which
 *                        should be copied to destination directory unless
 *                        URL to common location is provided
 */
extern void rgt_xml2multi_setup_outdir(rgt_gen_ctx_t *ctx,
                                       rgt_xml2multi_opts *opts,
                                       bool shared_files);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_RGT_XML2MULTI_COMMON_H__ */
