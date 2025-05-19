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
#include "te_str.h"

#include "capture.h"

#include "xml2gen.h"
#include "xml2multi_common.h"
#include "xml2html-multi.h"
#include "mi_msg.h"

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
    bool is_test; /**< Is this test iteration? */
    char *log_level; /**< Log level value in string representation */

    GHashTable *depth_log_names; /**< Hash table for log names for
                                      the particular node:
                                      key - entity name,
                                      value - array of user names */
    uint32_t linum; /**< Line number in HTML */
    unsigned int req_idx; /**< Current requirement index */

    te_dbuf json_data;    /**< Buffer for collecting JSON before it can
                               be parsed */

    bool no_logs;      /**< No logs were added yet */
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
 * Context with common parameters.
 * It is pointless to pass it as context argument to callbacks
 * as there is currently no way to pass it to the first place
 * where it is needed - rgt_process_cmdline().
 */
static rgt_xml2multi_opts multi_opts = RGT_XML2MULTI_OPTS_INIT;

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
    XML2MULTI_COMMON_OPTS
    XML2MULTI_HTML_SPECIFIC_OPTS

    POPT_TABLEEND
};

/* Process format-specific options */
void rgt_process_cmdline(rgt_gen_ctx_t *ctx, poptContext con, int val)
{
    UNUSED(ctx);

    rgt_xml2multi_process_cmdline(&multi_opts, con, val);
}

/* Add common global template parameters */
void rgt_tmpls_attrs_add_globals(rgt_attrs_t *attrs)
{
    rgt_tmpls_attrs_add_str(attrs, "shared_url",
                            multi_opts.shared_url);
    rgt_tmpls_attrs_add_str(attrs, "docs_url",
                            multi_opts.docs_url);
}

RGT_DEF_FUNC(proc_document_start)
{
    static gen_ctx_user_t user_ctx;

    gen_ctx_user_t   *gen_user;
    depth_ctx_user_t *depth_user;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    /* Initialize a pointer to generic user-specific data */
    memset(&user_ctx, 0, sizeof(user_ctx));
    ctx->user_data = gen_user = &user_ctx;

    /* Leave XML entities as they are, without any substitution */
    ctx->expand_entities = false;

    /* Set up output directory */
    rgt_xml2multi_setup_outdir(ctx, &multi_opts, true);

    /* Initialize depth-specific user data pointer */
    depth_ctx->user_data = alloc_depth_user_data(ctx->depth);
    depth_user = depth_ctx->user_data;
    depth_user->depth_log_names = NULL;
    depth_user->is_test = false;
    depth_user->name = strdup("SUITE");

    lf_start(ctx, depth_ctx, NULL, NULL, NULL);

    if (rgt_xml2multi_match_node(&multi_opts, NULL, NULL,
                                 ROOT_NODE_DEPTH, ROOT_NODE_SEQ))
    {
        char fname[255];

        rgt_xml2multi_fname(fname, sizeof(fname),
                            &multi_opts, ctx, depth_ctx,
                            NULL, NULL, "html");

        if ((depth_user->fname = strdup(fname)) == NULL ||
            (depth_user->fd = fopen(depth_user->fname, "w")) == NULL)
            exit(1);

        attrs = rgt_tmpls_attrs_new(NULL);
        rgt_tmpls_attrs_add_globals(attrs);
        rgt_tmpls_attrs_add_str(attrs, "reporter", "TE start-up");
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
        rgt_tmpls_attrs_add_str(attrs, "user", user);
        rgt_tmpls_output(fd, &xml2fmt_tmpls[JS_LOG_NAMES_USER], attrs);
        rgt_tmpls_attrs_free(attrs);
    }
    g_ptr_array_free(users, true);
    g_hash_table_destroy(user_hash);

    /* Output line about entity entry */
    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_str(attrs, "entity", entity);
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
    g_ptr_array_free(entries, true);

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

    rgt_xml2multi_opts_free(&multi_opts);
}

static void
lf_start(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx, const char *result,
     const char *node_class, rgt_depth_ctx_t *prev_depth_ctx)
{
    depth_ctx_user_t *depth_user = depth_ctx->user_data;
    bool is_test = depth_user->is_test;
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
            rgt_tmpls_attrs_set_str(attrs, "name", cur_user->name);

            rgt_tmpls_output(depth_user->dir_fd,
                             &xml2fmt_tmpls[LF_REF_PART], attrs);
        }

        rgt_tmpls_output(depth_user->dir_fd,
                         &xml2fmt_tmpls[LF_START_TABLE], NULL);

        if (prev_depth_ctx != NULL)
        {
            rgt_tmpls_attrs_set_uint32(attrs, "depth", ctx->depth - 1);
            rgt_tmpls_attrs_set_uint32(attrs, "seq", prev_depth_ctx->seq);
            rgt_tmpls_attrs_set_str(attrs, "name", "..");
            rgt_tmpls_attrs_set_str(attrs, "class", NODE_CLASS_STD);
            rgt_tmpls_output(depth_user->dir_fd,
                             &xml2fmt_tmpls[LF_ROW_FOLDER], attrs);
        }
    }

    if (prev_depth_ctx != NULL)
    {
        depth_ctx_user_t *prev_depth_user = prev_depth_ctx->user_data;

        rgt_tmpls_attrs_set_uint32(attrs, "depth", ctx->depth);
        rgt_tmpls_attrs_set_uint32(attrs, "seq", depth_ctx->seq);
        rgt_tmpls_attrs_set_str(attrs, "name", depth_user->name);
        rgt_tmpls_attrs_set_str(attrs, "class", node_class);
        if (is_test)
        {
            rgt_tmpls_attrs_add_str(attrs, "result", result);
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
 */
static void
control_node_start(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
                   const char **xml_attrs)
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
    rgt_attrs_t      *attrs;
    bool matched = true;

    const char *node_type_str = rgt_node2str(depth_ctx->type);

    assert(ctx->depth >= 2);

    depth_user = depth_ctx->user_data = alloc_depth_user_data(ctx->depth);
    depth_user->depth_log_names = NULL;

    if (name == NULL)
        name = "session";

    depth_user->is_test = (depth_ctx->type == NT_TEST);

    if ((err != NULL && err[0] != '\0') ||
        strcasecmp(result, "INCOMPLETE") == 0)
        node_class = NODE_CLASS_ERR;
    else
        node_class = NODE_CLASS_STD;

    prev_depth_ctx = &g_array_index(ctx->depth_info,
                                    rgt_depth_ctx_t, (ctx->depth - 2));
    prev_depth_user = prev_depth_ctx->user_data;

    rgt_xml2multi_fname(fname, sizeof(fname), &multi_opts,
                        ctx, depth_ctx, tin, node_id, "html");

    depth_user->name = strdup(name);

    matched = rgt_xml2multi_match_node(&multi_opts, tin, node_id,
                                       ctx->depth, depth_ctx->seq);

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
    rgt_tmpls_attrs_add_fstr(attrs, "reporter", "%s %s", node_type_str,
                             name == NULL ? "<anonimous>" : name);
    rgt_tmpls_attrs_add_uint32(attrs, "depth", ctx->depth);
    rgt_tmpls_attrs_add_uint32(attrs, "seq", depth_ctx->seq);
    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[DOC_START], attrs);

    rgt_tmpls_attrs_add_str(attrs, "node_type", node_type_str);
    rgt_tmpls_attrs_add_str(attrs, "name", name);
    rgt_tmpls_attrs_add_str(attrs, "result", result);
    rgt_tmpls_attrs_add_str(attrs, "tin", tin);
    rgt_tmpls_attrs_add_str(attrs, "test_id", node_id);
    rgt_tmpls_attrs_add_str(attrs, "err", err);
    rgt_tmpls_output(depth_user->fd,
                     &xml2fmt_tmpls[DOC_CNTRL_NODE_TITLE], attrs);

    if (hash != NULL && hash[0] != 0)
    {
        rgt_tmpls_output(depth_user->fd,
                         &xml2fmt_tmpls[DOC_CNTRL_NODE_HASH], attrs);
    }

    rgt_tmpls_attrs_add_str(attrs, "fname", fname);
    rgt_tmpls_attrs_add_str(attrs, "class", node_class);

    fname[0] = '\0';
    if (depth_user->is_test)
    {
        snprintf(fname, sizeof(fname), "n_%d_%d",
                 ctx->depth - 1, prev_depth_ctx->seq);
    }
    rgt_tmpls_attrs_add_str(attrs, "par_name", fname);

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
 */
static void
control_node_end(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
                 const char **xml_attrs)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    FILE             *fd = depth_user->fd;

    UNUSED(ctx);
    UNUSED(xml_attrs);

    if (fd != NULL)
    {
        output_log_names(&(depth_user->depth_log_names),
                         ctx->depth, depth_ctx->seq);

        rgt_tmpls_output(fd, &xml2fmt_tmpls[DOC_END], NULL);
        fclose(fd);
        free(depth_user->fname);
        depth_user->fname = NULL;
    }

    free(depth_user->name);
    depth_user->name = NULL;

    lf_end(ctx, depth_ctx);
}

RGT_DEF_FUNC(proc_session_start)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_start(ctx, depth_ctx, xml_attrs);
}

RGT_DEF_FUNC(proc_session_end)
{
    RGT_FUNC_UNUSED_PRMS();
    control_node_end(ctx, depth_ctx, xml_attrs);
}

RGT_DEF_FUNC(proc_pkg_start)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_start(ctx, depth_ctx, xml_attrs);
}

RGT_DEF_FUNC(proc_pkg_end)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_end(ctx, depth_ctx, xml_attrs);
}

RGT_DEF_FUNC(proc_test_start)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_start(ctx, depth_ctx, xml_attrs);
}

RGT_DEF_FUNC(proc_test_end)
{
    RGT_FUNC_UNUSED_PRMS();

    control_node_end(ctx, depth_ctx, xml_attrs);
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

        if (strcmp(user, "Verdict") == 0 || strcmp(user, "Artifact") == 0)
        {
            rgt_tmpls_attrs_add_fstr(attrs, "style_class_add", " %s",
                                     user);
        }
        else
        {
            rgt_tmpls_attrs_add_str(attrs, "style_class_add", "");
        }

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
    bool first_val = true;

    for (j = 0; j < param->values_num; j++)
    {
        te_rgt_mi_meas_value *value = &param->values[j];

        if (value->defined && value->specified)
        {
            double multiplier_value = 1.0;

            if (!first_val)
                fprintf(fd, ", ");

            /*
             * The multiplier may be a hexadecimal float which
             * is not understood by JavaScript, so here we re-parse
             * it and always print as an ordinary float.
             */
            if (value->multiplier != NULL && *value->multiplier != '\0')
            {
                te_errno rc = te_strtod(value->multiplier, &multiplier_value);

                if (rc != 0)
                {
                    fprintf(stderr, "Invalid multiplier value '%s'",
                            value->multiplier);
                    exit(1);
                }
            }

            fprintf(fd, "%f * %.9f", value->value, multiplier_value);
            first_val = false;
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
    bool first_y = true;

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

        first_y = false;

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

        rgt_tmpls_attrs_add_str(attrs, "level", depth_user->log_level);
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

RGT_DEF_FUNC(proc_meta_reqs_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    depth_user->req_idx = 0;

    if (depth_user->fd != NULL)
    {
        rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_REQS_START],
                         NULL);
    }
}

RGT_DEF_FUNC(proc_meta_req_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_user->fd != NULL)
    {
        if (depth_user->req_idx > 0)
            fprintf(depth_user->fd, ", ");

        attrs = rgt_tmpls_attrs_new(xml_attrs);
        rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[REQ_START],
                         attrs);
        rgt_tmpls_attrs_free(attrs);
    }

    depth_user->req_idx++;
}

RGT_DEF_DUMMY_FUNC(proc_meta_req_end)

RGT_DEF_FUNC(proc_logs_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_user->fd != NULL)
    {
        attrs = rgt_tmpls_attrs_new(xml_attrs);
        if (multi_opts.page_selector_set && multi_opts.cur_page >= 1)
        {
            rgt_tmpls_attrs_add_str(attrs, "selector_name", "top");
            rgt_tmpls_attrs_add_str(attrs, "fname", depth_user->fname);
            rgt_tmpls_attrs_add_uint32(attrs, "cur_page",
                                       multi_opts.cur_page);
            rgt_tmpls_attrs_add_uint32(attrs, "pages_count",
                                       multi_opts.pages_count);
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
            depth_user->no_logs = false;
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
        if (multi_opts.page_selector_set && multi_opts.cur_page >= 1)
        {
            rgt_tmpls_attrs_add_str(attrs, "selector_name", "bottom");
            rgt_tmpls_attrs_add_str(attrs, "fname", depth_user->fname);
            rgt_tmpls_attrs_add_uint32(attrs, "cur_page",
                                       multi_opts.cur_page);
            rgt_tmpls_attrs_add_uint32(attrs, "pages_count",
                                       multi_opts.pages_count);
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
        rgt_tmpls_attrs_add_str(attrs, "name", name);

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
DEF_FUNC_WITHOUT_ATTRS(proc_meta_reqs_end, META_REQS_END)
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
    FILE *fd = depth_user->fd;

    RGT_FUNC_UNUSED_PRMS();

    if (fd != NULL)
        rgt_tmpls_output(fd, &xml2fmt_tmpls[META_ARTIFACT_START], NULL);
}

RGT_DEF_FUNC(proc_meta_artifact_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    FILE *fd = depth_user->fd;

    RGT_FUNC_UNUSED_PRMS();

    if (fd != NULL)
        rgt_tmpls_output(fd, &xml2fmt_tmpls[META_ARTIFACT_END], NULL);
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
    bool prev_space = false;
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
            prev_space = false;
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

bool
proc_expand_entities(void)
{
    return false;
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

/** Storage of depth-specific user data */
static rgt_depth_data_storage depth_data =
          RGT_DEPTH_DATA_STORAGE_INIT(sizeof(depth_ctx_user_t));

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
    bool reused = false;

    assert(depth >= 1);

    depth_user = rgt_xml2fmt_alloc_depth_data(&depth_data, depth, &reused);

    depth_user->log_level = NULL;
    depth_user->no_logs = true;
    depth_user->linum = 1;

    if (!reused)
        depth_user->json_data = (te_dbuf)TE_DBUF_INIT(0);

    return depth_user;
}

/**
 * Free resources allocated under depth-specific user data -
 * on the particular depth.
 */
static void
free_depth_user_data_cb(void *data)
{
    depth_ctx_user_t *depth_user = data;

    free(depth_user->name);
    free(depth_user->fname);

    te_dbuf_free(&depth_user->json_data);
}

/**
 * Free resources allocated under depth-specific user data -
 * for all the depths.
 */
static void
free_depth_user_data()
{
    rgt_xml2fmt_free_depth_data(&depth_data, &free_depth_user_data_cb);
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
