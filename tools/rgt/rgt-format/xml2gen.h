/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: XML2FORMAT module.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RGT_XML2GEN_H__
#define __TE_RGT_XML2GEN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if (defined WITH_LIBXML)
# include <libxml/parser.h>
# include <libxml/parserInternals.h>
typedef xmlChar rgt_xmlChar;
#elif (defined WITH_EXPAT)
# include <expat.h>
typedef char rgt_xmlChar;
#else
# error Not Libxml nor Expat is selected
#endif

#include <glib.h>
#include <popt.h>

#include "rgt_tmpls_lib.h"
#include "te_defs.h"
#include "te_stdint.h"

/** The list of possible states in XML processing state machine */
typedef enum rgt_state {
    RGT_XML2HTML_STATE_INITIAL, /**< Initial state */

    RGT_XML2HTML_STATE_BLOCK,
    RGT_XML2HTML_STATE_LOGS, /**< state on entering "logs" TAG */

    RGT_XML2HTML_STATE_META, /**< state on entering "meta" TAG */

    RGT_XML2HTML_STATE_START_TS,
    RGT_XML2HTML_STATE_END_TS,
    RGT_XML2HTML_STATE_DURATION,
    RGT_XML2HTML_STATE_AUTHORS,
    RGT_XML2HTML_STATE_AUTHOR,
    RGT_XML2HTML_STATE_VERDICTS,
    RGT_XML2HTML_STATE_VERDICT,
    RGT_XML2HTML_STATE_ARTIFACTS,
    RGT_XML2HTML_STATE_ARTIFACT,
    RGT_XML2HTML_STATE_OBJECTIVE,
    RGT_XML2HTML_STATE_PAGE,
    RGT_XML2HTML_STATE_PARAMS,

    RGT_XML2HTML_STATE_CNTRL_MSG,
    RGT_XML2HTML_STATE_LOG_MSG,
    RGT_XML2HTML_STATE_MEM_DUMP,
    RGT_XML2HTML_STATE_MEM_DUMP_ROW,
    RGT_XML2HTML_STATE_MEM_DUMP_ELEM,
    RGT_XML2HTML_STATE_FILE,
    RGT_XML2HTML_STATE_PACKET,
    RGT_XML2HTML_STATE_PACKET_PROTO,
    RGT_XML2HTML_STATE_PACKET_FIELD,
} rgt_state_t;

/** Possible node types */
typedef enum rgt_node {
    NT_SESSION, /**< Node of session type */
    NT_PACKAGE, /**< Node of package type */
    NT_TEST,    /**< Node of test type */
    NT_BRANCH,  /**< It is used only for generation events
                     "branch start" / "branch end" */
    NT_LAST     /**< Last marker - the biggest value of the all evements */
} rgt_node_t;

/**
 * Structure that keeps values specific for the particular depth of
 * processing XML file.
 */
typedef struct rgt_depth_ctx {
    rgt_node_t  type; /**< Current node type */
    uint32_t    seq; /**< Current sequence number used on the
                               particular depth */
    void       *user_data; /**< User-specific data pointer */
} rgt_depth_ctx_t;

/**
 * Structure that keeps context of mem-dump processing sequence.
 */
typedef struct rgt_mem_ctx {
    uint32_t mem_width; /**< Number of elements in a memory row */
    uint32_t cur_num; /**< Current element number in memory row */
    te_bool  first_row; /* Wheter we are working with the first
                           memory row or not */
} rgt_mem_ctx_t;

/**
 * Variants how a single log file may be specified.
 */
typedef enum {
    RGT_MATCH_TIN,        /**< Test Identification Number */
    RGT_MATCH_NODE_ID,    /**< Log node ID */
    RGT_MATCH_DEPTH_SEQ,  /**< Depth and sequential numbers in
                               log nodes tree */
} rgt_match_type;

/** Prefix used for node ID in HTML log file name */
#define RGT_NODE_ID_PREFIX "id"

/**
 * Structure that keeps basic data used in processing XML file.
 */
typedef struct rgt_gen_ctx {
    const char     *xml_fname; /**< XML file name */
    const char     *out_fname; /**< Output file name */
    te_bool         expand_entities; /**< Wheter to expand standard
                                          XML entities
                                          like &lt; and &gt; or not? */

    uint32_t        depth; /**< The current processing depth
                                in the node tree */
    GArray         *depth_info; /**< Array of information about
                                     the particular depth */
    rgt_state_t     state; /**< Current processing state */

    rgt_mem_ctx_t   mem_ctx; /**< Memory element processing context.
                                  Make sense only in STATE_MEM_... */

    void           *user_data; /**< User-specific data pointer */

    te_bool         index_only;           /**< If @c TRUE, output HTML
                                               index files only */
    te_bool         single_node_match;    /**< Output HTML page only
                                               for specified log node */
    rgt_match_type  match_type;           /**< How a single log node
                                               was specified */
    char           *match_id;             /**< ID of log node */
    uint32_t        match_depth;          /**< Depth of log node */
    uint32_t        match_seq;            /**< Sequential number of
                                               log node */
    te_bool         page_selector_set;    /**< Output page selector
                                               allowing to select page
                                               of large HTML log */
    uint32_t        cur_page;             /**< Current page number */
    uint32_t        pages_count;          /**< Total pages count */
} rgt_gen_ctx_t;


extern const char *xml2fmt_files[];
extern rgt_tmpl_t  xml2fmt_tmpls[];
extern size_t      xml2fmt_tmpls_num;

/* Additional format-specific options table */
extern struct poptOption rgt_options_table[];


/* Print "usage" how to and exit */
extern void usage(poptContext optCon, int exitcode,
                  char *error, char *addl);

/* Process additional format-specific options */
extern void rgt_process_cmdline(rgt_gen_ctx_t *ctx,
                                poptContext con, int val);

/* Template for formatter programs without specific options */
#define RGT_PROCESS_CMDLINE_DUMMY() \
    struct poptOption rgt_options_table[] = {           \
        POPT_TABLEEND                                   \
    };                                                  \
                                                        \
    void rgt_process_cmdline(rgt_gen_ctx_t *ctx,        \
                             poptContext con, int val)  \
    {                                                   \
        UNUSED(ctx);                                    \
        UNUSED(con);                                    \
        UNUSED(val);                                    \
    }


#define RGT_XML2CHAR(x_) ((const char **)(x_))

typedef void (* rgt_xml2fmt_cb_func)(rgt_gen_ctx_t *ctx,
                                     rgt_depth_ctx_t *depth_ctx,
                                     const char **xml_attrs);

#define RGT_EXTERN_FUNC(name_) \
    extern void name_(rgt_gen_ctx_t *ctx,               \
                      rgt_depth_ctx_t *depth_ctx,       \
                      const char **xml_attrs)

#define RGT_DEF_FUNC(name_) \
    void name_(rgt_gen_ctx_t *ctx,                \
               rgt_depth_ctx_t *depth_ctx,        \
               const char **xml_attrs)

/**
 * Macro to define empty callback function
 *
 * @param name_  Function name to be defined
 */
#define RGT_DEF_DUMMY_FUNC(name_) \
    RGT_DEF_FUNC(name_)           \
    {                             \
        UNUSED(ctx);              \
        UNUSED(depth_ctx);        \
        UNUSED(xml_attrs);        \
    }

#define RGT_FUNC_UNUSED_PRMS() \
    do {                       \
        UNUSED(ctx);           \
        UNUSED(depth_ctx);     \
        UNUSED(xml_attrs);     \
    } while (0)

RGT_EXTERN_FUNC(proc_document_start);
RGT_EXTERN_FUNC(proc_document_end);
RGT_EXTERN_FUNC(proc_session_start);
RGT_EXTERN_FUNC(proc_session_end);
RGT_EXTERN_FUNC(proc_pkg_start);
RGT_EXTERN_FUNC(proc_pkg_end);
RGT_EXTERN_FUNC(proc_test_start);
RGT_EXTERN_FUNC(proc_test_end);
RGT_EXTERN_FUNC(proc_branch_start);
RGT_EXTERN_FUNC(proc_branch_end);
RGT_EXTERN_FUNC(proc_logs_start);
RGT_EXTERN_FUNC(proc_logs_end);
RGT_EXTERN_FUNC(proc_log_msg_start);
RGT_EXTERN_FUNC(proc_log_msg_end);
RGT_EXTERN_FUNC(proc_meta_start);
RGT_EXTERN_FUNC(proc_meta_end);
RGT_EXTERN_FUNC(proc_meta_params_start);
RGT_EXTERN_FUNC(proc_meta_params_end);
RGT_EXTERN_FUNC(proc_meta_param_start);
RGT_EXTERN_FUNC(proc_meta_param_end);
RGT_EXTERN_FUNC(proc_meta_start_ts_start);
RGT_EXTERN_FUNC(proc_meta_start_ts_end);
RGT_EXTERN_FUNC(proc_meta_end_ts_start);
RGT_EXTERN_FUNC(proc_meta_end_ts_end);
RGT_EXTERN_FUNC(proc_meta_duration_start);
RGT_EXTERN_FUNC(proc_meta_duration_end);
RGT_EXTERN_FUNC(proc_meta_objective_start);
RGT_EXTERN_FUNC(proc_meta_objective_end);
RGT_EXTERN_FUNC(proc_meta_authors_start);
RGT_EXTERN_FUNC(proc_meta_authors_end);
RGT_EXTERN_FUNC(proc_meta_author_start);
RGT_EXTERN_FUNC(proc_meta_author_end);
RGT_EXTERN_FUNC(proc_meta_verdicts_start);
RGT_EXTERN_FUNC(proc_meta_verdicts_end);
RGT_EXTERN_FUNC(proc_meta_verdict_start);
RGT_EXTERN_FUNC(proc_meta_verdict_end);
RGT_EXTERN_FUNC(proc_meta_artifacts_start);
RGT_EXTERN_FUNC(proc_meta_artifacts_end);
RGT_EXTERN_FUNC(proc_meta_artifact_start);
RGT_EXTERN_FUNC(proc_meta_artifact_end);
RGT_EXTERN_FUNC(proc_meta_page_start);
RGT_EXTERN_FUNC(proc_meta_page_end);
RGT_EXTERN_FUNC(proc_mem_dump_start);
RGT_EXTERN_FUNC(proc_mem_dump_end);
RGT_EXTERN_FUNC(proc_mem_row_start);
RGT_EXTERN_FUNC(proc_mem_row_end);
RGT_EXTERN_FUNC(proc_mem_elem_start);
RGT_EXTERN_FUNC(proc_mem_elem_end);
RGT_EXTERN_FUNC(proc_log_msg_br);
RGT_EXTERN_FUNC(proc_log_msg_file_start);
RGT_EXTERN_FUNC(proc_log_msg_file_end);

RGT_EXTERN_FUNC(proc_log_packet_start);
RGT_EXTERN_FUNC(proc_log_packet_end);
RGT_EXTERN_FUNC(proc_log_packet_proto_start);
RGT_EXTERN_FUNC(proc_log_packet_proto_end);
RGT_EXTERN_FUNC(proc_log_packet_field_start);

/**
 * Callback function for processing a group of characters.
 *
 * @param ctx        Global processing context
 * @param depth_ctx  Depth-specific context
 * @param ch         Pointer to the start of a character string
 * @param len        The number of characters in @p ch string
 */
extern void proc_chars(rgt_gen_ctx_t *ctx,
                       rgt_depth_ctx_t *depth_ctx,
                       const rgt_xmlChar *ch, size_t len);

/**
 * Callback function that tells whether a program wants entities
 * to be expanded or not.
 * For instance XML, HTML generating programs do not want entities
 * to be expanded, TEXT generating programs do.
 *
 * @return TRUE if entities should be expanded, or FALSE otherwise.
 */
extern te_bool proc_expand_entities(void);

/**
 * Get index of the element in xml2fmt_files massive. Search by short name.
 *
 * @param short_name    Short name of the template
 *
 * @return Index
 * @retval -1   In case of failure
 */
extern int rgt_xml2fmt_files_get_idx(const char* short_name);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RGT_XML2GEN_H__ */
