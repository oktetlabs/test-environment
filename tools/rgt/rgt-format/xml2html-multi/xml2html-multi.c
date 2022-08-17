/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: xml2html multidocument utility callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "logger_defs.h"
#include "te_raw_log.h"
#include "te_dbuf.h"
#include "te_string.h"

#include "capture.h"

#include "xml2gen.h"
#include "xml2html-multi.h"
#include "mi_msg.h"

/*
 * Name all files by depth and sequence numbers in tree,
 * including test iteration nodes. If this is turned off,
 * then test iteration nodes will be named by node ID.
 * If this is turned on, references to logs files in
 * TRC report will be broken.
 */
static te_bool rgt_html_depth_seq_names = FALSE;

/* Root log node depth in the tree of log nodes */
#define ROOT_NODE_DEPTH   1
/* Root log node sequential number */
#define ROOT_NODE_SEQ     0

/*
 * Max attribute length in one line. Zero means it is not limited and
 * attributes will not be splitted into multiple lines. Splitting can
 * break HTML links if an attribute is used inside href.
 */
int  rgt_max_attribute_length = 0;
/* A tag to separate lines */
const char *rgt_line_separator = "<br>";
/* Flag turning on detailed packet dumps in log. */
int detailed_packets = 1;

/** Structure to keep basic user data in general parsing context */
typedef struct gen_ctx_user {
    FILE *js_fd; /**< File descriptor of JavaScript file */
    GStringChunk *strings; /**< Chunk of strings that are log message
                                entity and user names */
    GHashTable *log_names; /**< Hash table for all log names:
                                key - entity name,
                                value - array of user names */
} gen_ctx_user_t;

/** Structure to keep user data in depth-specific context */
typedef struct depth_ctx_user {
    FILE *fd; /**< File descriptor of the node currently being processed
                   on the particular depth */
    FILE *dir_fd; /**< File descriptor of the node currently being processed
                       on the particular depth */
    char *name;   /**< Value of name XML attribute */
    char *fname;  /**< File name of HTML log */
    te_bool is_test; /**< Is this test iteration? */
    char *log_level; /**< Log level value in string representation */

    GHashTable *depth_log_names; /**< Hash table for log names for
                                      the particular node:
                                      key - entity name,
                                      value - array of user names */
    uint32_t linum; /**< Line number in HTML */

    te_bool ignore_artifact; /**< If @c TRUE, the currently processed
                                  artifact should be ignored - this
                                  is used for filtering out MI
                                  artifacts in summary */

    te_dbuf json_data;    /**< Buffer for collecting JSON before it can
                               be parsed */

    te_bool no_logs;      /**< No logs were added yet */
} depth_ctx_user_t;

/**< Struct to keep values related to log message name JS callback */
typedef struct log_msg_name {
    FILE       *fd; /**< File pointer used for JS output */
    GHashTable *entity_hash; /**< Pointer to entity name hash */
} log_msg_name_t;

/* Values for node class (now - only by presence of 'err' attribute) */
#define NODE_CLASS_STD  "std"
#define NODE_CLASS_ERR  "err"

/*
 * URL for common files (images, styles etc.)
 * If this value is NULL, all of the files are copied
 * to report output directory.
 */
char *shared_url = NULL;

/*
 * Base URL for doxygen-generated documentation for tests
 */
char *docs_url = NULL;

/* Forward declaration */
static depth_ctx_user_t *alloc_depth_user_data(uint32_t depth);
static void free_depth_user_data(void);
static void add_log_user(gen_ctx_user_t *gen_user,
                         depth_ctx_user_t *depth_user,
                         const char *entity, const char *user);
static void output_log_names(GHashTable **entity_hash,
                             uint32_t depth, uint32_t seq);
static void lf_start(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
                     const char *result, const char *node_class,
                     rgt_depth_ctx_t *prev_depth_ctx);

static te_log_level te_log_level_str2h(const char *ll);

/* RGT format-specific options table */
struct poptOption rgt_options_table[] = {
    { "shared-url", 'i', POPT_ARG_STRING, NULL, 'i',
      "URL of directory for shared files (images etc.)", NULL },
    { "docs-url", 'd', POPT_ARG_STRING, NULL, 'd',
      "URL of directory for test descriptions", NULL },
    { "single-node", 'n', POPT_ARG_STRING, NULL, 'n',
      "Output only specified node.", NULL },
    { "page-selector", 'p', POPT_ARG_STRING, NULL, 'p',
      "Show page selector.", NULL },
    { "index-only", 'x', POPT_ARG_NONE, NULL, 'x',
      "Output only index pages.", NULL },

    POPT_TABLEEND
};

/* Process format-specific options */
void rgt_process_cmdline(rgt_gen_ctx_t *ctx, poptContext con, int val)
{
    size_t len;

    if (val == 'i')
    {
        shared_url = poptGetOptArg(con);
        len = strlen(shared_url);
        if (len > 0 && shared_url[len - 1] != '/')
        {
            fprintf(stderr, "Warning: URL for shared files is not "
                    "a directory (or trailing '/' is missing)");
        }
        else if (len == 0)
        {
            free(shared_url);
            shared_url = NULL;
        }
    }
    else if (val == 'd')
    {
        docs_url = poptGetOptArg(con);
        len = strlen(docs_url);
        if (len > 0 && docs_url[len - 1] != '/')
        {
            fprintf(stderr, "Warning: URL for test descriptions is not "
                    "a directory (or trailing '/' is missing)");
        }
        else if (len == 0)
        {
            free(docs_url);
            docs_url = NULL;
        }
    }
    else if (val == 'n')
    {
        char *match_exp;

        if ((match_exp = poptGetOptArg(con)) == NULL)
            usage(con, 1, "Specify node matching expression", NULL);

        if (strchr(match_exp, '_') == NULL)
        {
            if (strcmp_start(RGT_NODE_ID_PREFIX, match_exp) == 0)
            {
                ctx->match_id = strdup(match_exp +
                                       strlen(RGT_NODE_ID_PREFIX));
                assert(ctx->match_id != NULL);
                free(match_exp);
                ctx->match_type = RGT_MATCH_NODE_ID;
            }
            else
            {
                ctx->match_id = match_exp;
                ctx->match_type = RGT_MATCH_TIN;
            }
        }
        else
        {
            sscanf(match_exp, "%u_%u",
                   &ctx->match_depth,
                   &ctx->match_seq);
            free(match_exp);
            ctx->match_type = RGT_MATCH_DEPTH_SEQ;
        }
        ctx->single_node_match = TRUE;
    }
    else if (val == 'x')
    {
        ctx->index_only = TRUE;
    }
    else if (val == 'p')
    {
        const char *page_selector;

        if ((page_selector = poptGetOptArg(con)) == NULL)
            usage(con, 1, "Specify page selector", NULL);

        ctx->page_selector_set = TRUE;
        if (strcmp(page_selector, "all") == 0)
        {
            ctx->cur_page = 0;
            ctx->pages_count = 0;
        }
        else
            sscanf(page_selector, "%u/%u",
                   &ctx->cur_page, &ctx->pages_count);
    }
}

/* Add common global template parameters */
void rgt_tmpls_attrs_add_globals(rgt_attrs_t *attrs)
{
    rgt_tmpls_attrs_add_fstr(attrs, "shared_url", shared_url);
    rgt_tmpls_attrs_add_fstr(attrs, "docs_url", docs_url);
}

/**
 * Check whether a given log node (HTML log file) should be output.
 *
 * @param ctx       RGT context
 * @param tin       TIN of test iteration represented by this node
 * @param node_id   Node ID
 * @param depth     Node depth in log tree
 * @param seq       Node sequential number (in then list of chidren of its
 *                  parent)
 *
 * return TRUE if node should be output, FALSE otherwise.
 */
static inline te_bool
match_node(rgt_gen_ctx_t *ctx, const char *tin, const char *node_id,
           uint32_t depth, uint32_t seq)
{
    if (ctx->index_only)
        return FALSE;

    if (ctx->single_node_match)
    {
        switch (ctx->match_type)
        {
            case RGT_MATCH_TIN:
                if (tin == NULL || ctx->match_id == NULL ||
                    strcmp(ctx->match_id, tin) != 0)
                    return FALSE;
                break;

            case RGT_MATCH_NODE_ID:
                if (node_id == NULL || ctx->match_id == NULL ||
                    strcmp(ctx->match_id, node_id) != 0)
                    return FALSE;
                break;

            case RGT_MATCH_DEPTH_SEQ:
                if (ctx->match_depth != depth ||
                    ctx->match_seq != seq)
                    return FALSE;
                break;
        }
    }

    return TRUE;
}

RGT_DEF_FUNC(proc_document_start)
{
    static gen_ctx_user_t user_ctx;

    gen_ctx_user_t   *gen_user;
    depth_ctx_user_t *depth_user;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    /* Initialize a pointer to generic user-specific data */
    ctx->user_data = gen_user = &user_ctx;

    /* Leave XML entities as they are, without any substitution */
    ctx->expand_entities = FALSE;

    /* Copy all the aux files */
    if (ctx->out_fname == NULL)
        ctx->out_fname = "html";

    {
        int         rc;
        struct stat stat_buf;
        char        buf[1024];
        char        prefix[PATH_MAX];
        const char *snprintf_error = "Error writing command to buffer\n";
        int         n;

        if (rgt_resource_files_prefix_get(NULL, NULL, sizeof(prefix), prefix))
        {
            fprintf(stderr, "Failed to get resource files path prefix\n");
            exit(EXIT_FAILURE);
        }

        rc = stat(ctx->out_fname, &stat_buf);
        if (rc == -1)
        {
            if (errno != ENOENT)
            {
                perror(ctx->out_fname);
                exit(1);
            }
            if (mkdir(ctx->out_fname, 0777) < 0)
            {
                perror(ctx->out_fname);
                exit(1);
            }
        }
        if (stat(ctx->out_fname, &stat_buf) != 0)
        {
            perror(ctx->out_fname);
            exit(1);
        }
        if (!S_ISDIR(stat_buf.st_mode))
        {
            fprintf(stderr, "File %s already exists and "
                    "it is not a directory", ctx->out_fname);
            exit(1);
        }

        if (chdir(ctx->out_fname) < 0)
        {
            perror(ctx->out_fname);
            exit(1);
        }

        if (shared_url == NULL)
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
                     prefix, (shared_url == NULL) ? "" : shared_url);
        if (n < 0 || (size_t)n >= sizeof(buf))
        {
            fputs(snprintf_error, stderr);
            exit(EXIT_FAILURE);
        }
        system(buf);
    }


    /* Initialize depth-specific user data pointer */
    depth_ctx->user_data = alloc_depth_user_data(ctx->depth);
    depth_user = depth_ctx->user_data;
    depth_user->depth_log_names = NULL;
    depth_user->is_test = FALSE;
    depth_user->name = strdup("SUITE");

    lf_start(ctx, depth_ctx, NULL, NULL, NULL);

    if (match_node(ctx, NULL, NULL, ROOT_NODE_DEPTH, ROOT_NODE_SEQ))
    {
        char fname[255];

        if (ctx->page_selector_set &&
            (ctx->cur_page > 1 || ctx->cur_page == 0))
        {
            if (ctx->cur_page == 0)
                snprintf(fname, sizeof(fname),
                         "node_1_0_all.html");
            else
                snprintf(fname, sizeof(fname),
                         "node_1_0_p%u.html", ctx->cur_page);
        }
        else
            snprintf(fname, sizeof(fname), "%s",
                     "node_1_0.html");

        if ((depth_user->fname = strdup(fname)) == NULL ||
            (depth_user->fd = fopen(depth_user->fname, "w")) == NULL)
            exit(1);

        attrs = rgt_tmpls_attrs_new(NULL);
        rgt_tmpls_attrs_add_globals(attrs);
        rgt_tmpls_attrs_add_fstr(attrs, "reporter", "TE start-up");
        rgt_tmpls_attrs_add_uint32(attrs, "depth", ctx->depth);
        rgt_tmpls_attrs_add_uint32(attrs, "seq", depth_ctx->seq);
        rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[DOC_START], attrs);
        rgt_tmpls_attrs_free(attrs);
    }
    else
        depth_user->fd = NULL;

    if ((gen_user->js_fd = fopen("nodes_tree.js", "w")) == NULL)
    {
        perror("nodes_tree.js");
        exit(1);
    }

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_globals(attrs);
    rgt_tmpls_attrs_add_uint32(attrs, "depth", ctx->depth);
    rgt_tmpls_attrs_add_uint32(attrs, "seq", depth_ctx->seq);
    rgt_tmpls_output(gen_user->js_fd, &xml2fmt_tmpls[JS_DOC_START], attrs);
    rgt_tmpls_attrs_free(attrs);

    /* Initialize log names hash */
    gen_user->strings = g_string_chunk_new(1024);
    if (gen_user->strings == NULL)
        exit(1);

    gen_user->log_names = g_hash_table_new(g_str_hash, g_str_equal);
    if (gen_user->log_names == NULL)
        exit(1);
}

/**
 * Compares two strings passed by pointer.
 *
 * @param a  Pointer to the first string (const char **)
 * @param b  Pointer to the second string (const char **)
 *
 * @return The same result as strcmp.
 */
static gint
ptr_strcmp(gconstpointer a, gconstpointer b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

/**
 * Auxiliary function that appends string key into array of pointers.
 *
 * @param key        String value representing string key
 *                   (entity or user name)
 * @param value      Unused (when key is entity name, it is the hash of
 *                   user names for this entity name.
 *                   when key is user name, it is the same as key)
 * @param user_data  Pointer to an array of pointers where we want to
 *                   append the key to
 *
 * @note The function is intended to be used as callback for
 * g_hash_table_foreach() calls.
 */
static void
key_append_to_array(gpointer key, gpointer value, gpointer user_data)
{
    GPtrArray *array = (GPtrArray *)user_data;

    UNUSED(value);

    g_ptr_array_add(array, key);
}

/**
 * Output the information about a single entity name into file
 * prepared by user (JavaScript file).
 *
 * @param data       Pointer to string representing entity name for
 *                   which all the pairs (entity name, user name)
 *                   should be output (const char *)
 * @param user_data  Pointer to user callback data,
 *                   here it is the data of type (log_msg_name_t *)
 *
 * @note The function is intended to be used as callback for
 * g_ptr_array_foreach() function.
 *
 * @se The function destroys user names hash passed via @p user_data.
 */
static void
log_entity_out(gpointer data, gpointer user_data)
{
    const char     *entity = (const char *)data;
    log_msg_name_t *cb_data = (log_msg_name_t *)user_data;
    FILE           *fd = cb_data->fd;
    GHashTable     *entity_hash = cb_data->entity_hash;
    rgt_attrs_t    *attrs;
    GHashTable     *user_hash;
    GPtrArray      *users;
    const char     *user;
    guint           i;

    user_hash = (GHashTable *)g_hash_table_lookup(entity_hash, entity);

    assert(user_hash != NULL);

    users = g_ptr_array_new();
    assert(users != NULL);
    g_hash_table_foreach(user_hash, key_append_to_array, users);
    g_ptr_array_sort(users, (GCompareFunc)ptr_strcmp);

    for (i = 0; i < users->len; i++)
    {
        user = g_ptr_array_index(users, i);

        attrs = rgt_tmpls_attrs_new(NULL);
        rgt_tmpls_attrs_add_fstr(attrs, "user", "%s", user);
        rgt_tmpls_output(fd, &xml2fmt_tmpls[JS_LOG_NAMES_USER], attrs);
        rgt_tmpls_attrs_free(attrs);
    }
    g_ptr_array_free(users, TRUE);
    g_hash_table_destroy(user_hash);

    /* Output line about entity entry */
    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_fstr(attrs, "entity", "%s", entity);
    rgt_tmpls_output(fd, &xml2fmt_tmpls[JS_LOG_NAMES_ENTITY], attrs);
    rgt_tmpls_attrs_free(attrs);
}

/**
 * Output the information about all entity and user names kept
 * in @p entity_hash hash.
 * The file name is structured as follows:
 * node_${depth}_${seq}_log_names.js
 *
 * @param entity_hash  Hash of entity names (value of each element in
 *                     the hash is a hash of all user names for this
 *                     particular entity name)
 * @param depth        The depth of processing
 * @param seq          The sequence number of processing
 *
 * @note Even for enpth hash it generates JaveScript file.
 *
 * @se The function destroys @p entity_hash and updates it with @c NULL.
 */
static void
output_log_names(GHashTable **entity_hash, uint32_t depth, uint32_t seq)
{
    log_msg_name_t  cb_data;
    GPtrArray      *entries;
    char            fname[256];

    assert(entity_hash != NULL);

    cb_data.entity_hash = *entity_hash;

    snprintf(fname, sizeof(fname), "node_%u_%u_log_names.js", depth, seq);
    if ((cb_data.fd = fopen(fname, "w")) == NULL)
    {
        perror(fname);
        exit(2);
    }

    rgt_tmpls_output(cb_data.fd,
                     &xml2fmt_tmpls[JS_LOG_NAMES_START], NULL);

    if (cb_data.entity_hash == NULL)
    {
        /* Create the array with no log names */
        fclose(cb_data.fd);
        return;
    }

    /* Flush the list of log names into the file */

    entries = g_ptr_array_new();
    g_hash_table_foreach(cb_data.entity_hash, key_append_to_array, entries);
    g_ptr_array_sort(entries, (GCompareFunc)ptr_strcmp);

    /* Now we have sorted array of entity names */
    g_ptr_array_foreach(entries, log_entity_out, &cb_data);
    g_ptr_array_free(entries, TRUE);

    g_hash_table_destroy(cb_data.entity_hash);
    *entity_hash = NULL;

    fclose(cb_data.fd);
}

RGT_DEF_FUNC(proc_document_end)
{
    gen_ctx_user_t   *gen_user = (gen_ctx_user_t *)ctx->user_data;
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    FILE             *js_fd = gen_user->js_fd;
    FILE             *fd = ((depth_ctx_user_t *)depth_ctx->user_data)->fd;

    RGT_FUNC_UNUSED_PRMS();

    if (js_fd != NULL)
        fclose(js_fd);

    if (fd != NULL)
    {
        rgt_tmpls_output(fd, &xml2fmt_tmpls[DOC_END], NULL);

        /* Output the list of log names for root node */
        output_log_names(&(depth_user->depth_log_names),
                         ctx->depth, depth_ctx->seq);

        fclose(fd);
    }

    /* Output the list of accumulated log names */
    output_log_names(&(gen_user->log_names), 0, 0);

    g_string_chunk_free(gen_user->strings);

    free_depth_user_data();
}

static void
lf_start(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx, const char *result,
     const char *node_class, rgt_depth_ctx_t *prev_depth_ctx)
{
    depth_ctx_user_t *depth_user = depth_ctx->user_data;
    te_bool           is_test = depth_user->is_test;
    char              fname[255];
    rgt_attrs_t      *attrs;
    unsigned int      i;

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_globals(attrs);

    if (!is_test)
    {
        depth_ctx_user_t *cur_user;
        rgt_depth_ctx_t  *cur_ctx;

        snprintf(fname, sizeof(fname), "n_%d_%d.html",
                 ctx->depth, depth_ctx->seq);
        if ((depth_user->dir_fd = fopen(fname, "w")) == NULL)
        {
            fprintf(stderr, "Cannot create %s file: %s\n",
                    fname, strerror(errno));
            exit(1);
        }

        rgt_tmpls_attrs_set_uint32(attrs, "depth", ctx->depth);
        rgt_tmpls_attrs_set_uint32(attrs, "seq", depth_ctx->seq);
        rgt_tmpls_output(depth_user->dir_fd,
                     &xml2fmt_tmpls[LF_DOC_START], attrs);

        for (i = 0; i < ctx->depth; i++)
        {
            cur_ctx = &g_array_index(ctx->depth_info,
                                     rgt_depth_ctx_t, i);
            cur_user = cur_ctx->user_data;

            rgt_tmpls_attrs_set_uint32(attrs, "depth", i + 1);
            rgt_tmpls_attrs_set_uint32(attrs, "seq", cur_ctx->seq);
            rgt_tmpls_attrs_set_fstr(attrs, "name", cur_user->name);

            rgt_tmpls_output(depth_user->dir_fd,
                             &xml2fmt_tmpls[LF_REF_PART], attrs);
        }

        rgt_tmpls_output(depth_user->dir_fd,
                         &xml2fmt_tmpls[LF_START_TABLE], NULL);

        if (prev_depth_ctx != NULL)
        {
            rgt_tmpls_attrs_set_uint32(attrs, "depth", ctx->depth - 1);
            rgt_tmpls_attrs_set_uint32(attrs, "seq", prev_depth_ctx->seq);
            rgt_tmpls_attrs_set_fstr(attrs, "name", "..");
            rgt_tmpls_attrs_set_fstr(attrs, "class", NODE_CLASS_STD);
            rgt_tmpls_output(depth_user->dir_fd,
                             &xml2fmt_tmpls[LF_ROW_FOLDER], attrs);
        }
    }

    if (prev_depth_ctx != NULL)
    {
        depth_ctx_user_t *prev_depth_user = prev_depth_ctx->user_data;

        rgt_tmpls_attrs_set_uint32(attrs, "depth", ctx->depth);
        rgt_tmpls_attrs_set_uint32(attrs, "seq", depth_ctx->seq);
        rgt_tmpls_attrs_set_fstr(attrs, "name", depth_user->name);
        rgt_tmpls_attrs_set_fstr(attrs, "class", node_class);
        if (is_test)
        {
            rgt_tmpls_attrs_add_fstr(attrs, "result", result);
            rgt_tmpls_output(prev_depth_user->dir_fd,
                             &xml2fmt_tmpls[LF_ROW_TEST], attrs);
        }
        else
        {
            rgt_tmpls_output(prev_depth_user->dir_fd,
                             &xml2fmt_tmpls[LF_ROW_FOLDER], attrs);
        }
    }

    rgt_tmpls_attrs_free(attrs);
}

static void
lf_end(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx)
{
    depth_ctx_user_t *depth_user = depth_ctx->user_data;

    UNUSED(ctx);

    if (!depth_user->is_test)
    {
        rgt_tmpls_output(depth_user->dir_fd,
                         &xml2fmt_tmpls[LF_DOC_END], NULL);

        fclose(depth_user->dir_fd);
    }
}

/**
 * Function for processing start of control node event.
 *
 * @param ctx        Pointer to the global processing context
 * @param depth_ctx  Pointer to the depth processing context
 * @param xml_attrs  Pointer to the list of attributes in libxml style
 * @param node_type  String representation of the node type
 */
static void
control_node_start(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
                   const char **xml_attrs, const char *node_type)
{
    gen_ctx_user_t   *gen_user = (gen_ctx_user_t *)ctx->user_data;
    depth_ctx_user_t *depth_user;
    depth_ctx_user_t *prev_depth_user;
    rgt_depth_ctx_t  *prev_depth_ctx;
    const char       *name = rgt_tmpls_xml_attrs_get(xml_attrs, "name");
    const char       *result = rgt_tmpls_xml_attrs_get(xml_attrs, "result");
    const char       *tin = rgt_tmpls_xml_attrs_get(xml_attrs, "tin");
    const char       *node_id = rgt_tmpls_xml_attrs_get(xml_attrs,
                                                        "test_id");
    const char       *err = rgt_tmpls_xml_attrs_get(xml_attrs, "err");
    const char       *hash = rgt_tmpls_xml_attrs_get(xml_attrs, "hash");
    const char       *node_class;
    char              fname[500] = "";
    char              page_str[255] = "";
    rgt_attrs_t      *attrs;
    te_bool           matched = TRUE;
    rgt_match_type    name_type;

    assert(ctx->depth >= 2);

    depth_user = depth_ctx->user_data = alloc_depth_user_data(ctx->depth);
    depth_user->depth_log_names = NULL;

    if (name == NULL)
        name = "session";

    depth_user->is_test = strcmp(node_type, "Test") == 0;

    if ((err != NULL && err[0] != '\0') ||
        strcasecmp(result, "INCOMPLETE") == 0)
        node_class = NODE_CLASS_ERR;
    else
        node_class = NODE_CLASS_STD;

    prev_depth_ctx = &g_array_index(ctx->depth_info,
                                    rgt_depth_ctx_t, (ctx->depth - 2));
    prev_depth_user = prev_depth_ctx->user_data;

    if (ctx->page_selector_set)
    {
        if (ctx->cur_page == 0)
            snprintf(page_str, sizeof(page_str), "_all");
        else if (ctx->cur_page > 1)
            snprintf(page_str, sizeof(page_str), "_p%u", ctx->cur_page);
    }

    /*
     * Default file name format. TIN is chosen here for backward
     * compatibility with old XML logs in which there is no node IDs.
     */
    if (rgt_html_depth_seq_names)
        name_type = RGT_MATCH_DEPTH_SEQ;
    else
        name_type = RGT_MATCH_TIN;

    if (ctx->single_node_match && !(ctx->index_only))
    {
        /*
         * If single log node was requested, use name format corresponding
         * to how that node was specified.
         */
        name_type = ctx->match_type;
    }
    else if (!rgt_html_depth_seq_names && depth_user->is_test &&
             node_id != NULL)
    {
        /*
         * Otherwise use node_id<NODE_ID>.html format if possible.
         */
        name_type = RGT_MATCH_NODE_ID;
    }

    /*
     * Fall back to node_<DEPTH>_<SEQ>.html name format if no
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
            snprintf(fname, sizeof(fname), "node_%s%s.html", tin, page_str);
            break;

        case RGT_MATCH_NODE_ID:
            snprintf(fname, sizeof(fname), "node_id%s%s.html",
                     node_id, page_str);
            break;

        case RGT_MATCH_DEPTH_SEQ:
            snprintf(fname, sizeof(fname), "node_%d_%d%s.html",
                     ctx->depth, depth_ctx->seq, page_str);
            break;
    }

    depth_user->name = strdup(name);

    matched = match_node(ctx, tin, node_id, ctx->depth, depth_ctx->seq);

    if (matched)
    {
        if ((depth_user->fname = strdup(fname)) == NULL ||
            (depth_user->fd = fopen(depth_user->fname, "w")) == NULL)
        {
            fprintf(stderr, "Cannot create %s file: %s\n",
                    fname, strerror(errno));
            exit(1);
        }

    }
    else
        depth_user->fd = NULL;


    lf_start(ctx, depth_ctx, result, node_class, prev_depth_ctx);

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_globals(attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "reporter", "%s %s", node_type,
                             name == NULL ? "<anonimous>" : name);
    rgt_tmpls_attrs_add_uint32(attrs, "depth", ctx->depth);
    rgt_tmpls_attrs_add_uint32(attrs, "seq", depth_ctx->seq);
    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[DOC_START], attrs);

    rgt_tmpls_attrs_add_fstr(attrs, "node_type", node_type);
    rgt_tmpls_attrs_add_fstr(attrs, "name", name);
    rgt_tmpls_attrs_add_fstr(attrs, "result", result);
    rgt_tmpls_attrs_add_fstr(attrs, "tin", tin);
    rgt_tmpls_attrs_add_fstr(attrs, "test_id", node_id);
    rgt_tmpls_attrs_add_fstr(attrs, "err", err);
    rgt_tmpls_output(depth_user->fd,
                     &xml2fmt_tmpls[DOC_CNTRL_NODE_TITLE], attrs);

    if (hash != NULL && hash[0] != 0)
    {
        rgt_tmpls_output(depth_user->fd,
                         &xml2fmt_tmpls[DOC_CNTRL_NODE_HASH], attrs);
    }

    rgt_tmpls_attrs_add_fstr(attrs, "fname", fname);
    rgt_tmpls_attrs_add_fstr(attrs, "class", node_class);

    fname[0] = '\0';
    if (depth_user->is_test)
    {
        snprintf(fname, sizeof(fname), "n_%d_%d",
                 ctx->depth - 1, prev_depth_ctx->seq);
    }
    rgt_tmpls_attrs_add_fstr(attrs, "par_name", fname);

    rgt_tmpls_output(prev_depth_user->fd,
                     &xml2fmt_tmpls[DOC_REF_TO_NODE], attrs);

    rgt_tmpls_attrs_add_uint32(attrs, "prev_depth", ctx->depth - 1);

    if (depth_user->is_test)
    {
        rgt_tmpls_output(gen_user->js_fd,
                         &xml2fmt_tmpls[JS_ADD_TEST_NODE], attrs);
    }
    else
    {
        rgt_tmpls_output(gen_user->js_fd,
                         &xml2fmt_tmpls[JS_ADD_FOLDER_NODE], attrs);
    }

    rgt_tmpls_attrs_free(attrs);
}

/**
 * Function for processing end of control node event.
 *
 * @param ctx        Pointer to the global processing context
 * @param depth_ctx  Pointer to the depth processing context
 * @param xml_attrs  Pointer to the list of attributes in libxml style
 * @param node_type  String representation of the node type
 */
static void
control_node_end(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
                 const char **xml_attrs, const char *node_type)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    FILE             *fd = depth_user->fd;

    UNUSED(ctx);
    UNUSED(xml_attrs);
    UNUSED(node_type);

    if (fd != NULL)
    {
        output_log_names(&(depth_user->depth_log_names),
                         ctx->depth, depth_ctx->seq);

        rgt_tmpls_output(fd, &xml2fmt_tmpls[DOC_END], NULL);
        fclose(fd);
        free(depth_user->fname);
    }

    lf_end(ctx, depth_ctx);
}

RGT_DEF_FUNC(proc_session_start)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_start(ctx, depth_ctx, xml_attrs, "Session");
}

RGT_DEF_FUNC(proc_session_end)
{
    RGT_FUNC_UNUSED_PRMS();
    control_node_end(ctx, depth_ctx, xml_attrs, "Session");
}

RGT_DEF_FUNC(proc_pkg_start)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_start(ctx, depth_ctx, xml_attrs, "Package");
}

RGT_DEF_FUNC(proc_pkg_end)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_end(ctx, depth_ctx, xml_attrs, "Package");
}

RGT_DEF_FUNC(proc_test_start)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_start(ctx, depth_ctx, xml_attrs, "Test");
}

RGT_DEF_FUNC(proc_test_end)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_end(ctx, depth_ctx, xml_attrs, "Test");
}

RGT_DEF_FUNC(proc_log_msg_start)
{
    gen_ctx_user_t   *gen_user = (gen_ctx_user_t *)ctx->user_data;
    const char       *level = rgt_tmpls_xml_attrs_get(xml_attrs, "level");
    const char       *entity = rgt_tmpls_xml_attrs_get(xml_attrs, "entity");
    const char       *user = rgt_tmpls_xml_attrs_get(xml_attrs, "user");
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    assert(level != NULL && entity != NULL && user != NULL);
    depth_user->log_level = strdup(level);

    /* Add information about entity/user name into users tree */
    add_log_user(gen_user, depth_user, entity, user);

    if (depth_user->fd != NULL)
    {
        attrs = rgt_tmpls_attrs_new(xml_attrs);
        rgt_tmpls_attrs_add_uint32(attrs, "level_id",
                                   te_log_level_str2h(level));
        rgt_tmpls_attrs_add_uint32(attrs, "linum",
                                   depth_user->linum++);
        rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOG_MSG_START],
                         attrs);

        rgt_tmpls_attrs_free(attrs);
    }
}

/**
 * Print a measurement value.
 *
 * @param fd      File where to print.
 * @param value   Value to print.
 * @param prefix  Prefix string (may be @c NULL).
 */
static void
print_mi_meas_value(FILE *fd, te_rgt_mi_meas_value *value, const char *prefix)
{
    if (!(value->defined))
        return;

    fprintf(fd, "<li>");
    if (prefix != NULL)
        fprintf(fd, "%s: ", prefix);

    if (value->specified)
        fprintf(fd, "%f", value->value);
    else
        fprintf(fd, "[failed to obtain]");

    if (value->multiplier != NULL && *(value->multiplier) != '\0' &&
        strcmp(value->multiplier, "1") != 0)
        fprintf(fd, " * %s", value->multiplier);
    if (value->base_units != NULL && *(value->base_units) != '\0')
        fprintf(fd, " %s", value->base_units);

    fprintf(fd, "</li>\n");
}

/**
 * Print out comma-separated list of measured parameter values
 * used for Javascript array initialization.
 *
 * @param fd      File where to print the list.
 * @param param   Measured parameter.
 */
static void
print_mi_meas_param_vals_array(FILE *fd, te_rgt_mi_meas_param *param)
{
    size_t j;
    te_bool first_val = TRUE;

    for (j = 0; j < param->values_num; j++)
    {
        te_rgt_mi_meas_value *value = &param->values[j];

        if (value->defined && value->specified)
        {
            if (!first_val)
                fprintf(fd, ", ");

            fprintf(fd, "%f * %s", value->value,
                    (value->multiplier != NULL &&
                     *(value->multiplier) != '\0' ?
                     value->multiplier : "1"));
            first_val = FALSE;
        }
    }
}

/**
 * Print graph of values of a parameter from measurement MI artifact.
 *
 * @param fd      File where to print.
 * @param meas    Data from parsed MI measurement artifact.
 * @param view    Graph description.
 * @param linum   Line number (used for creating unique ID in HTML file).
 * @param index   Index of the parameter in the list of all parameters.
 */
static void
print_mi_meas_line_graph(FILE *fd, te_rgt_mi_meas *meas,
                         te_rgt_mi_meas_view *view,
                         unsigned int linum, unsigned int index)
{
    const char *graph_width = "90%";
    const char *graph_height = "25em";
    const char *graph_padding_top = "2em";
    size_t i;
    ssize_t j;
    size_t axis_y_num;
    te_bool first_y = TRUE;

    ssize_t check_len;
    te_string warns = TE_STRING_INIT;

    const char *colors[] = { "crimson", "blue", "darkgreen",
                             "chocolate", "blueviolet",
                             "darkred", "deeppink",
                             "orange" };
    const char *color;

    te_rgt_mi_meas_view_line_graph *line_graph = &view->data.line_graph;
    te_rgt_mi_meas_param *param;

    if (meas->params_num == 0)
        return;

    fprintf(fd, "<div style=\"width: %s; height: %s; padding-top: %s;\">\n",
            graph_width, graph_height, graph_padding_top);
    fprintf(fd,
            "<canvas id=\"mi_line_graph_%u_%u\" "
            "style=\"border-style: solid;\"></canvas>\n",
            linum, index);
    fprintf(fd, "</div>\n");

    fprintf(fd, "<script>\nvar axis_x = {};\n");
    if (line_graph->axis_x == TE_RGT_MI_GRAPH_AXIS_AUTO_SEQNO)
    {
        fprintf(fd, "axis_x.auto_seqno = true;\n");
        fprintf(fd, "axis_x.label = \"Sequence number\";\n");
        check_len = -1;
    }
    else
    {
        param = &meas->params[line_graph->axis_x];

        fprintf(fd, "axis_x.auto_seqno = false;\n");
        fprintf(fd, "axis_x.label = \"%s\";\n",
                te_rgt_mi_meas_param_name(param));
        fprintf(fd, "axis_x.values = param_%u_%zd;\n",
                linum, line_graph->axis_x);
        check_len = param->values_num;
    }

    axis_y_num = line_graph->axis_y_num;
    /*
     * If "axis_y" is omitted, display all parameters
     * except that which is assigned for axis X
     */
    if (axis_y_num == 0)
        axis_y_num = meas->params_num;

    fprintf(fd, "var axis_y = [\n");
    for (i = 0; i < axis_y_num; i++)
    {
        if (line_graph->axis_y_num == 0)
        {
            if (i == (size_t)(line_graph->axis_x))
                continue;

            j = i;
        }
        else
        {
            j = line_graph->axis_y[i];
        }

        param = &meas->params[j];

        if (check_len > 0 && param->values_num != (size_t)check_len)
        {
            te_string_append(&warns,
                             "Parameter '%s' on axis Y has %zu values "
                             "while on axis X there is "
                             "%zd values.\n<br>",
                             te_rgt_mi_meas_param_name(param),
                             param->values_num, check_len);
        }

        color = colors[j % TE_ARRAY_LEN(colors)];

        if (!first_y)
            fprintf(fd, ",\n");

        first_y = FALSE;

        fprintf(fd, "{ label: \"%s\", color: \"%s\", "
                "values: param_%u_%zd }",
                te_rgt_mi_meas_param_name(param), color, linum, j);
    }
    fprintf(fd, "];\n");

    fprintf(fd, "add_graph(\"mi_line_graph_%u_%u\", \"%s\", "
            "axis_x, axis_y);\n", linum, index, view->title);

    fprintf(fd, "</script>\n");

    if (warns.len > 0)
        fprintf(fd, "<span class=\"graph_warn\">%s</span>\n", warns.ptr);

    te_string_free(&warns);
}

/**
 * Print a header inside log message.
 *
 * @param _hlevel     Header level. CSS class log_hN should be
 *                    defined for it, see misc/log_style.css.
 * @param _fd         File where to print.
 * @param _format...  Format string and arguments.
 */
#define FPRINTF_HEADER(_hlevel, _fd, _format...) \
    do {                                                      \
        fprintf(_fd, "<span class=\"log_h%d\">", _hlevel);    \
        fprintf(_fd, _format);                                \
        fprintf(_fd, "</span>\n");                            \
    } while (0)

/**
 * Log parsed MI artifact of type "measurement".
 *
 * @param fd        File where to print log.
 * @param mi        Structure with parsed MI artifact.
 * @param linum     Line number (used for unique IDs).
 */
static void
log_mi_measurement(FILE *fd, te_rgt_mi *mi, unsigned int linum)
{
    te_rgt_mi_meas *meas = &mi->data.measurement;
    size_t i;
    size_t j;
    te_rgt_mi_meas_view *view;

    /*
     * If list of single values is longer than that, it is hidden
     * by default.
     */
    const unsigned int max_showed_vals = 15;

    FPRINTF_HEADER(1, fd, "Measurements from tool %s", meas->tool);
    for (i = 0; i < meas->params_num; i++)
    {
        te_rgt_mi_meas_param *param;

        param = &meas->params[i];

        FPRINTF_HEADER(
                2, fd, "Measured parameter: \"%s\"",
                te_rgt_mi_meas_param_name(param));

        fprintf(fd, "<ul style=\"list-style-type:none;\">\n");

        if (param->stats_present)
        {
            fprintf(fd, "<li>\n");
            FPRINTF_HEADER(3, fd, "Statistics:");
            fprintf(fd, "<ul style=\"list-style-type:none;\">\n");
            print_mi_meas_value(fd, &param->min, "min");
            print_mi_meas_value(fd, &param->max, "max");
            print_mi_meas_value(fd, &param->mean, "mean");
            print_mi_meas_value(fd, &param->median, "median");
            print_mi_meas_value(fd, &param->stdev, "stdev");
            print_mi_meas_value(fd, &param->cv, "cv");
            print_mi_meas_value(fd, &param->out_of_range, "out of range");
            print_mi_meas_value(fd, &param->percentile, "percentile");
            fprintf(fd, "</ul>\n");
            fprintf(fd, "</li>\n");
        }

        if (param->values_num > 0)
        {
            if (param->in_graph)
            {
                fprintf(fd, "<script>var param_%u_%zu = [",
                        linum, i);
                print_mi_meas_param_vals_array(fd, param);
                fprintf(fd, "];</script>");
            }

            fprintf(fd, "<li>\n");

            FPRINTF_HEADER(3, fd, "Values:");

            fprintf(fd, "<span class=\"%s_link\" "
                    "onclick=\"show_hide_list(this, "
                    "'meas_param_list_%u_%zu', 'Hide %zu values', "
                    "'Show %zu values');\">%s %zu values"
                    "</span>\n",
                    (param->values_num > max_showed_vals ?
                        "show" : "hide"),
                    linum, i, param->values_num, param->values_num,
                    (param->values_num > max_showed_vals ?
                        "Show" : "Hide"),
                    param->values_num);

            fprintf(fd, "<ul id=\"meas_param_list_%u_%zu\" "
                    "style=\"display:%s; list-style-type:none;\">\n",
                    linum, i,
                    (param->values_num > max_showed_vals ?
                                            "none" : "block"));
            for (j = 0; j < param->values_num; j++)
            {
                print_mi_meas_value(fd, &param->values[j], NULL);
            }

            fprintf(fd, "</ul>\n");
            fprintf(fd, "</li>\n");
        }

        fprintf(fd, "</ul>\n");
    }

    for (i = 0; i < meas->views_num; i++)
    {
        view = &meas->views[i];

        if (view->type != NULL &&
            strcmp(view->type, "line-graph") == 0)
        {
            print_mi_meas_line_graph(fd, meas, view, linum, i);
        }
    }

    if (meas->keys_num > 0)
    {
        FPRINTF_HEADER(2, fd, "Keys:");
        fprintf(fd, "<ul style=\"list-style-type:none;\">\n");
        for (i = 0; i < meas->keys_num; i++)
        {
            fprintf(fd, "<li>\"%s\" : \"%s\"\n</li>", meas->keys[i].key,
                    meas->keys[i].value);
        }
        fprintf(fd, "</ul>\n");
    }

    if (meas->comments_num > 0)
    {
        FPRINTF_HEADER(2, fd, "Comments:");
        fprintf(fd, "<ul style=\"list-style-type:none;\">\n");
        for (i = 0; i < meas->comments_num; i++)
        {
            fprintf(fd, "<li>\"%s\" : \"%s\"</li>\n", meas->comments[i].key,
                    meas->comments[i].value);
        }
        fprintf(fd, "</ul>\n");
    }
}

/**
 * Log parsed MI artifact.
 *
 * @param fd        File where to print log.
 * @param mi        Structure with parsed MI artifact.
 * @param buf       Buffer with not parsed MI artifact data.
 * @param len       Size of the buffer.
 * @param linum     Line number (used for unique IDs).
 * @param attrs     Attributes for HTML templates.
 */
static void
log_mi_artifact(FILE *fd, te_rgt_mi *mi, void *buf, size_t len,
                unsigned int linum, rgt_attrs_t *attrs)
{
    int json_show_level = 1;

    if (mi->parse_failed)
    {
        fprintf(fd, "Failed to parse JSON: %s<br>\n", mi->parse_err);
        fwrite(buf, len, 1, fd);
        return;
    }
    else if (mi->rc != 0)
    {
        if (mi->rc == TE_EOPNOTSUPP)
        {
            fprintf(fd, "Cannot process MI artifact without "
                    "libjansson<br>\n");
        }
        else
        {
            fprintf(fd, "Failed to process MI artifact, error = %s<br>\n",
                    te_rc_err2str(mi->rc));
        }
    }
    else if (mi->type == TE_RGT_MI_TYPE_MEASUREMENT)
    {
        log_mi_measurement(fd, mi, linum);

        /*
         * If textual representation was printed successfully, JSON object
         * view should be presented as a single-line link.
         */
        json_show_level = 0;
    }

    rgt_tmpls_attrs_add_uint32(attrs, "json_show_level",
                               json_show_level);

    rgt_tmpls_output(fd,
                     &xml2fmt_tmpls[JSON_START],
                     attrs);

    fwrite(buf, len, 1, fd);

    rgt_tmpls_output(fd,
                     &xml2fmt_tmpls[JSON_END],
                     attrs);
}

RGT_DEF_FUNC(proc_log_msg_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_user->fd != NULL)
    {
        attrs = rgt_tmpls_attrs_new(xml_attrs);

        if (strcmp(depth_user->log_level, "MI") == 0)
        {
            if (depth_user->json_data.ptr != NULL)
            {
                te_rgt_mi mi;

                te_rgt_parse_mi_message(
                                (char *)(depth_user->json_data.ptr),
                                depth_user->json_data.len, &mi);

                rgt_tmpls_attrs_add_uint32(attrs, "linum",
                                           depth_user->linum);
                log_mi_artifact(depth_user->fd, &mi,
                                (char *)(depth_user->json_data.ptr),
                                depth_user->json_data.len,
                                depth_user->linum, attrs);

                te_rgt_mi_clean(&mi);
                te_dbuf_reset(&depth_user->json_data);
            }
        }

        rgt_tmpls_attrs_add_fstr(attrs, "level", depth_user->log_level);
        rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOG_MSG_END],
                         attrs);
        rgt_tmpls_attrs_free(attrs);
    }

    free(depth_user->log_level);
    depth_user->log_level = NULL;
}

RGT_DEF_DUMMY_FUNC(proc_branch_start)
RGT_DEF_DUMMY_FUNC(proc_branch_end)

RGT_DEF_FUNC(proc_meta_param_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_user->fd != NULL)
    {
        attrs = rgt_tmpls_attrs_new(xml_attrs);
        rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[PARAM_START],
                         attrs);
        rgt_tmpls_attrs_free(attrs);
    }
}

RGT_DEF_DUMMY_FUNC(proc_meta_param_end)

RGT_DEF_FUNC(proc_logs_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    depth_user->linum = 1;

    if (depth_user->fd != NULL)
    {
        attrs = rgt_tmpls_attrs_new(xml_attrs);
        if (ctx->page_selector_set && ctx->cur_page >= 1)
        {
            rgt_tmpls_attrs_add_fstr(attrs, "selector_name", "top");
            rgt_tmpls_attrs_add_fstr(attrs, "fname", depth_user->fname);
            rgt_tmpls_attrs_add_uint32(attrs, "cur_page", ctx->cur_page);
            rgt_tmpls_attrs_add_uint32(attrs, "pages_count",
                                       ctx->pages_count);
            rgt_tmpls_output(depth_user->fd,
                             &xml2fmt_tmpls[PAGE_SELECTOR], attrs);
        }

        if (depth_user->no_logs)
        {
            /*
             * Add buttons for filtering logs only before the first log
             * messages table. Multiple tables may be present in case of
             * a session or package, for example one table for messages
             * which came before tests and another table for messages which
             * came after tests.
             */

            rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOGS_FILTER],
                             attrs);
            depth_user->no_logs = FALSE;
        }
        rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOGS_START], attrs);
        rgt_tmpls_attrs_free(attrs);
    }
}

RGT_DEF_FUNC(proc_logs_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_user->fd != NULL)
    {
        attrs = rgt_tmpls_attrs_new(xml_attrs);
        rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOGS_END], attrs);
        if (ctx->page_selector_set && ctx->cur_page >= 1)
        {
            rgt_tmpls_attrs_add_fstr(attrs, "selector_name", "bottom");
            rgt_tmpls_attrs_add_fstr(attrs, "fname", depth_user->fname);
            rgt_tmpls_attrs_add_uint32(attrs, "cur_page", ctx->cur_page);
            rgt_tmpls_attrs_add_uint32(attrs, "pages_count",
                                       ctx->pages_count);
            rgt_tmpls_output(depth_user->fd,
                             &xml2fmt_tmpls[PAGE_SELECTOR], attrs);
        }
        rgt_tmpls_attrs_free(attrs);
    }
}

RGT_DEF_DUMMY_FUNC(proc_meta_start)
RGT_DEF_DUMMY_FUNC(proc_meta_end)

#define DEF_FUNC_WITHOUT_ATTRS(name_, enum_const_) \
RGT_DEF_FUNC(name_)                                            \
{                                                              \
    FILE *fd = ((depth_ctx_user_t *)depth_ctx->user_data)->fd; \
                                                               \
    RGT_FUNC_UNUSED_PRMS();                                    \
                                                               \
    if (fd != NULL)                                            \
        rgt_tmpls_output(fd, &xml2fmt_tmpls[enum_const_],      \
                         NULL);                                \
}

DEF_FUNC_WITHOUT_ATTRS(proc_meta_start_ts_start, META_START_TS_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_start_ts_end, META_START_TS_END)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_end_ts_start, META_END_TS_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_end_ts_end, META_END_TS_END)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_duration_start, META_DURATION_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_duration_end, META_DURATION_END)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_objective_start, META_OBJ_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_objective_end, META_OBJ_END)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_page_end, META_PAGE_END)
DEF_FUNC_WITHOUT_ATTRS(proc_log_packet_end, LOG_PACKET_END)
DEF_FUNC_WITHOUT_ATTRS(proc_log_packet_proto_end, LOG_PACKET_PROTO_END)

RGT_DEF_FUNC(proc_meta_page_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_user->fd != NULL)
    {
        attrs = rgt_tmpls_attrs_new(xml_attrs);
        rgt_tmpls_attrs_add_globals(attrs);
        rgt_tmpls_output(depth_user->fd,
                         &xml2fmt_tmpls[META_PAGE_START], attrs);
        rgt_tmpls_attrs_free(attrs);
    }
}

RGT_DEF_FUNC(proc_meta_author_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    const char       *email = rgt_tmpls_xml_attrs_get(xml_attrs, "email");
    char             *name = strdup(email);
    char             *ptr;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    ptr = strchr(name, '@');
    if (ptr == NULL)
        assert(0);

    *ptr = '\0';

    if (depth_user->fd != NULL)
    {
        attrs = rgt_tmpls_attrs_new(xml_attrs);
        rgt_tmpls_attrs_add_fstr(attrs, "name", name);

        rgt_tmpls_output(depth_user->fd,
                         &xml2fmt_tmpls[META_AUTHOR_START], attrs);

        rgt_tmpls_attrs_free(attrs);
    }

    free(name);
}

RGT_DEF_DUMMY_FUNC(proc_meta_author_end)

DEF_FUNC_WITHOUT_ATTRS(proc_meta_authors_start, META_AUTHORS_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_authors_end, META_AUTHORS_END)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_verdicts_start, META_VERDICTS_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_verdicts_end, META_VERDICTS_END)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_verdict_start, META_VERDICT_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_verdict_end, META_VERDICT_END)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_artifacts_start, META_ARTIFACTS_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_artifacts_end, META_ARTIFACTS_END)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_params_start, META_PARAMS_START)
DEF_FUNC_WITHOUT_ATTRS(proc_meta_params_end, META_PARAMS_END)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_dump_start, MEM_DUMP_START)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_dump_end, MEM_DUMP_END)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_row_start, MEM_ROW_START)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_row_end, MEM_ROW_END)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_elem_start, MEM_ELEM_START)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_elem_end, MEM_ELEM_END)
DEF_FUNC_WITHOUT_ATTRS(proc_log_msg_br, LOG_BR)

#define DEF_FUNC_WITH_ATTRS(name_, enum_const_) \
RGT_DEF_FUNC(name_)                                                   \
{                                                                     \
    FILE        *fd = ((depth_ctx_user_t *)depth_ctx->user_data)->fd; \
    rgt_attrs_t *attrs;                                               \
                                                                      \
    RGT_FUNC_UNUSED_PRMS();                                           \
                                                                      \
    if (fd != NULL)                                                   \
    {                                                                 \
        attrs = rgt_tmpls_attrs_new(xml_attrs);                       \
        rgt_tmpls_output(fd, &xml2fmt_tmpls[enum_const_], attrs);     \
        rgt_tmpls_attrs_free(attrs);                                  \
    }                                                                 \
}

DEF_FUNC_WITH_ATTRS(proc_log_msg_file_start, LOG_MSG_FILE_START)
DEF_FUNC_WITHOUT_ATTRS(proc_log_msg_file_end, LOG_MSG_FILE_END)

RGT_DEF_FUNC(proc_meta_artifact_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    const char *level = rgt_tmpls_xml_attrs_get(xml_attrs, "level");
    FILE *fd = depth_user->fd;

    RGT_FUNC_UNUSED_PRMS();

    if (level != NULL && strcmp(level, "MI") == 0)
        depth_user->ignore_artifact = TRUE;
    else if (fd != NULL)
        rgt_tmpls_output(fd, &xml2fmt_tmpls[META_ARTIFACT_START], NULL);
}

RGT_DEF_FUNC(proc_meta_artifact_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    FILE *fd = depth_user->fd;

    RGT_FUNC_UNUSED_PRMS();

    if (!(depth_user->ignore_artifact) && fd != NULL)
        rgt_tmpls_output(fd, &xml2fmt_tmpls[META_ARTIFACT_END], NULL);

    depth_user->ignore_artifact = FALSE;
}

/**
 * Write a string to HTML file. If a sequence of multiple consecutive
 * spaces is encountered, replace every second space with "&nbsp;" so
 * that HTML will not replace such a sequence with a single space.
 *
 * @param fd      FILE to which to write
 * @param ch      String to write
 * @param len     Length of the string
 */
static void
write_chars_preserve_spaces(FILE *fd, const rgt_xmlChar *ch, size_t len)
{
    te_dbuf chars_dbuf = TE_DBUF_INIT(50);
    size_t i;
    size_t processed_len;
    te_bool prev_space = FALSE;
    static const char nbsp[] = "&nbsp;";

    for (i = 0, processed_len = 0; i < len; i++)
    {
        if (prev_space && ch[i] == ' ')
        {
            te_dbuf_append(&chars_dbuf, &ch[processed_len],
                           i - processed_len);
            te_dbuf_append(&chars_dbuf, nbsp, sizeof(nbsp) - 1);
            processed_len = i + 1;
        }

        if (ch[i] == ' ')
            prev_space = !prev_space;
        else
            prev_space = FALSE;
    }

    if (chars_dbuf.len == 0)
    {
        fwrite(ch, len, 1, fd);
    }
    else
    {
        if (len > processed_len)
        {
            te_dbuf_append(&chars_dbuf, &ch[processed_len],
                           len - processed_len);
        }

        fwrite(chars_dbuf.ptr, chars_dbuf.len, 1, fd);
    }

    te_dbuf_free(&chars_dbuf);
}

void
proc_chars(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
           const rgt_xmlChar *ch, size_t len)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    FILE *fd = depth_user->fd;
#if 0 && (defined WITH_LIBXML) && LIBXML_VERSION >= 20700
    const char        *entity = NULL;
    const rgt_xmlChar *p, *ch_begin, *ch_end;
#endif

    UNUSED(ctx);
    TE_COMPILE_TIME_ASSERT(sizeof(rgt_xmlChar) == 1);

    if (fd == NULL)
        return;

    if (depth_user->ignore_artifact)
        return;

    if (depth_user->log_level != NULL &&
        strcmp(depth_user->log_level, "MI") == 0)
    {
        te_dbuf_append(&depth_user->json_data, ch, len);
        return;
    }

#if 0 && (defined WITH_LIBXML) && LIBXML_VERSION >= 20700
    /* Additional conversion currently not needed
       as XML_PARSE_OLDSAX option passed to parser */

    /* libxml2 has a function xmlEncodeSpecialChars() for such a
       transformation but that function allocates each time new buffer
       in memory and copies converted string to it.
     */
    ch_begin = ch;
    ch_end = ch + len;
    for (p = ch; p < ch_end; p++)
    {
        switch (*p)
        {
            case '&':
                entity = "&amp;";
                break;

            case '<':
                entity = "&lt;";
                break;

            case '>':
                entity = "&gt;";
                break;

            default:
                break;
        }
        if (entity != NULL)
        {
            if (p > ch_begin)
                fwrite(ch_begin, p - ch_begin, 1, fd);

            fwrite(entity, strlen(entity), 1, fd);
            ch_begin = p + 1;
            entity = NULL;
        }
    }
    if (p > ch_begin)
        fwrite(ch_begin, p - ch_begin, 1, fd);
#else
    write_chars_preserve_spaces(fd, ch, len);
#endif
}

te_bool
proc_expand_entities(void)
{
    return FALSE;
}

/**
 * Add entity/user name pair to specific hash (can be duplicate).
 *
 * @param hash    Hash where we want to add the pair to
 * @param entity  Entity name
 * @param user    User name
 */
static void
add_log_user_to_hash(GHashTable *hash, gchar *entity, gchar *user)
{
    GHashTable *user_hash;

    user_hash = (GHashTable *)g_hash_table_lookup(hash, entity);
    if (user_hash == NULL)
    {
        /* Add a new entry */
        user_hash = g_hash_table_new(g_str_hash, g_str_equal);
        assert(user_hash != NULL);
    }
    g_hash_table_insert(user_hash, user, user);
    g_hash_table_insert(hash, entity, user_hash);
}

/**
 * Accept information about entity/user name pair.
 *
 * @param gen_user    Pointer to global-user-specific data structure
 * @param depth_user  Pointer to depth-user-specific data structure
 * @param entity      Entity name
 * @param user        User name
 *
 * @alg It adds the pair into node log messages hash, and into global
 * log messages hash.
 */
static void
add_log_user(gen_ctx_user_t *gen_user, depth_ctx_user_t *depth_user,
             const char *entity, const char *user)
{
    gchar *entity_cp;
    gchar *user_cp;

    entity_cp = g_string_chunk_insert_const(gen_user->strings, entity);
    user_cp = g_string_chunk_insert_const(gen_user->strings, user);

    assert(entity_cp != NULL && user_cp != NULL);

    add_log_user_to_hash(gen_user->log_names, entity_cp, user_cp);

    if (depth_user->depth_log_names == NULL)
    {
        depth_user->depth_log_names =
            g_hash_table_new(g_str_hash, g_str_equal);
        assert(depth_user->depth_log_names != NULL);
    }

    add_log_user_to_hash(depth_user->depth_log_names, entity_cp, user_cp);
}

/**
 * Array of pointer to depth-specific user data
 * (it is used as stack).
 */
static GPtrArray *depth_array = NULL;

/**
 * Returns a pointer to depth-specific user data
 *
 * @param depth  Processing depth value
 *
 * @return Pointer to depth-specific user data (not initialized)
 */
static depth_ctx_user_t *
alloc_depth_user_data(uint32_t depth)
{
    depth_ctx_user_t *depth_user;

    assert(depth >= 1);

    if (depth_array == NULL)
    {
        depth_array = g_ptr_array_new();
        if (depth_array == NULL)
            exit(1);
    }

    if (depth_array->len < depth)
    {
        assert((depth_array->len + 1) == depth);

        /* Create a new element in the array */
        g_ptr_array_set_size(depth_array, depth);
        g_ptr_array_index(depth_array, depth - 1) =
            (depth_ctx_user_t *)g_malloc(sizeof(depth_ctx_user_t));
    }

    assert(g_ptr_array_index(depth_array, depth - 1) != NULL);

    depth_user = g_ptr_array_index(depth_array, depth - 1);
    depth_user->ignore_artifact = FALSE;
    depth_user->log_level = NULL;
    depth_user->json_data = (te_dbuf)TE_DBUF_INIT(0);
    depth_user->no_logs = TRUE;

    return depth_user;
}

/**
 * Free resources allocated under depth-specific user data -
 * on the particular depth.
 *
 * @note The function is intended to be used as callback for
 * g_ptr_array_foreach() function.
 */
static void
free_depth_user_data_part(gpointer data, gpointer user_data)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)data;

    UNUSED(user_data);

    if (depth_user != NULL)
        te_dbuf_free(&depth_user->json_data);

    g_free(data);
}

/**
 * Free resources allocated under depth-specific user data -
 * for all the depths.
 */
static void
free_depth_user_data()
{
    g_ptr_array_foreach(depth_array, free_depth_user_data_part, NULL);
    g_ptr_array_free(depth_array, TRUE);
    depth_array = NULL;
}

static te_log_level
te_log_level_str2h(const char *ll)
{
    struct ll_map {
        const char   *str;
        te_log_level  num;
    } maps[] = {
#define MAP_ENTRY(ll_) \
        { TE_LL_ ## ll_ ## _STR, TE_LL_ ## ll_ }

        MAP_ENTRY(ERROR),
        MAP_ENTRY(WARN),
        MAP_ENTRY(RING),
        MAP_ENTRY(INFO),
        MAP_ENTRY(VERB),
        MAP_ENTRY(ENTRY_EXIT),
        MAP_ENTRY(PACKET),
        MAP_ENTRY(MI),

#undef MAP_ENTRY
    };
    unsigned int i;

    for (i = 0; i < sizeof(maps) / sizeof(maps[0]); i++)
    {
        if (strcmp(maps[i].str, ll) == 0)
            return maps[i].num;
    }

    assert(0);

    return 0;
}
