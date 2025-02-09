/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test Environment: xml2json utility callbacks.
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
#include "te_json.h"
#include "te_str.h"
#include "te_vector.h"

#include "xml2gen.h"
#include "mi_msg.h"
#include "xml2multi_common.h"

/*
 * These variables are defined only to make compilation possible.
 * Core xml2fmt code assumes that every tool uses templates and defines
 * these variables. However for this tool templates are not good, it
 * is more convenient to use TE API to generate JSON instead of them.
 */
const char *xml2fmt_files[] = { NULL };
rgt_tmpl_t xml2fmt_tmpls[] = { { .fname = NULL }, };
size_t xml2fmt_tmpls_num = 0;

/**
 * User context structure associated with a given depth
 * in the log.
 */
typedef struct depth_ctx_user {
    /** File where to save JSON for the current log node. */
    FILE *f;
    /** Index in array of JSON files information */
    int file_idx;
    /** Line number of the current message */
    int linum;

    /** @c true if we are inside log message */
    bool in_msg;
    /**
      *  Number of log messages which are not finished yet
      * (current message and all its parents in messages hierarchy).
      */
    int opened_msgs;
    /** @c true if we started filling content in a message */
    bool in_content;
    /**
     * @c true if characters from XML log should be appended to
     * JSON
     */
    bool append_chars;
    /**
     * @c true if XML characters should be interpreted as raw JSON rather
     * than a string which needs escaping
     */
    bool append_json;
    /** @c true if list of child entities is started */
    bool entity_list;

    /** Current nesting level */
    int cur_nl;

    /**
     * Stack with nesting levels of messages for which children lists
     * are currently filled.
     */
    te_vec nl_stack;

    /** TE JSON context */
    te_json_ctx_t json_ctx;
} depth_ctx_user;

/** JSON file information (used for building tree of JSON files) */
typedef struct file_descr {
    /** File name */
    char *fname;
    /** Indexes of children in array of files information */
    te_vec children;
    /** Node type (package, session, test) */
    rgt_node_t type;
    /** Package/session/test name */
    char *name;
    /** Test result */
    char *result;
    /** True if error occurred during test execution */
    bool has_err;
} file_descr;

/** Array of JSON files information */
static te_vec files = TE_VEC_INIT(file_descr);

/* Storage of depth-specific user data */
static rgt_depth_data_storage depth_data =
          RGT_DEPTH_DATA_STORAGE_INIT(sizeof(depth_ctx_user));

/* Command line options */
static rgt_xml2multi_opts multi_opts = RGT_XML2MULTI_OPTS_INIT;

/* Maximum attribute length in one line (not used) */
int rgt_max_attribute_length = 0;
/* A tag to separate lines (not used) */
const char *rgt_line_separator = "";

/* RGT format-specific options table */
struct poptOption rgt_options_table[] = {
    XML2MULTI_COMMON_OPTS
    POPT_TABLEEND
};

/* Process format-specific options */
void rgt_process_cmdline(rgt_gen_ctx_t *ctx, poptContext con, int val)
{
    UNUSED(ctx);

    rgt_xml2multi_process_cmdline(&multi_opts, con, val);
}

/* Allocate new user context or reuse the old one for a given depth */
static depth_ctx_user *
alloc_depth_user_data(uint32_t depth)
{
    depth_ctx_user *depth_user;
    bool reused = false;

    assert(depth >= 1);

    depth_user = rgt_xml2fmt_alloc_depth_data(&depth_data, depth, &reused);

    if (reused)
        te_vec_reset(&depth_user->nl_stack);
    else
        depth_user->nl_stack = (te_vec)TE_VEC_INIT(int);

    depth_user->linum = 1;
    depth_user->file_idx = -1;
    return depth_user;
}

/* Start root object */
static void
root_start(te_json_ctx_t *json_ctx)
{
    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "version", "v1");
    te_json_add_key(json_ctx, "root");
    te_json_start_array(json_ctx);

    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "type", "te-log");

    if (multi_opts.page_selector_set)
    {
        te_json_add_key(json_ctx, "pagination");
        te_json_start_object(json_ctx);

        te_json_add_key(json_ctx, "cur_page");
        te_json_add_integer(json_ctx, multi_opts.cur_page);
        te_json_add_key(json_ctx, "pages_count");
        te_json_add_integer(json_ctx, multi_opts.pages_count);

        te_json_end(json_ctx);
    }

    te_json_add_key(json_ctx, "content");
    te_json_start_array(json_ctx);
}

/* Finish root object */
static void
root_end(te_json_ctx_t *json_ctx)
{
    /* End of content array */
    te_json_end(json_ctx);
    /* End of te-log object */
    te_json_end(json_ctx);
    /* End of root array */
    te_json_end(json_ctx);
    /* End of main object */
    te_json_end(json_ctx);
}

/*
 * Start list of child entities (packages/sessions/tests) if it is
 * not already started
 */
static void
maybe_start_entity_list(depth_ctx_user *depth_user)
{
    te_json_ctx_t *json_ctx = &depth_user->json_ctx;

    if (!depth_user->entity_list)
    {
        te_json_start_object(json_ctx);
        te_json_add_key_str(json_ctx, "type", "te-log-entity-list");
        te_json_add_key(json_ctx, "items");
        te_json_start_array(json_ctx);

        depth_user->entity_list = true;
    }
}

/* Terminate list of child entities if it is started */
static void
maybe_end_entity_list(depth_ctx_user *depth_user)
{
    te_json_ctx_t *json_ctx = &depth_user->json_ctx;

    if (depth_user->entity_list)
    {
        te_json_end(json_ctx);
        te_json_end(json_ctx);
        depth_user->entity_list = false;
    }
}

/* Add child entity to a list */
static void
add_entity(te_json_ctx_t *json_ctx, const char *id,
           const char *name, const char *type,
           const char *result, const char *error,
           const char *tin, const char *hash)
{
    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "id", id);
    te_json_add_key_str(json_ctx, "name", name);
    te_json_add_key_str(json_ctx, "entity", type);
    te_json_add_key_str(json_ctx, "result", result);
    te_json_add_key_str(json_ctx, "error", error);

    te_json_add_key(json_ctx, "extended_properties");
    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "tin", tin);
    te_json_add_key_str(json_ctx, "hash", hash);
    te_json_end(json_ctx);

    te_json_end(json_ctx);
}

RGT_DEF_FUNC(proc_document_start)
{
    depth_ctx_user *depth_user;

    RGT_FUNC_UNUSED_PRMS();

    /*
     * XML character escapings like &gt; should be converted back
     * to characters.
     */
    ctx->expand_entities = true;

    /* Create output directory */
    rgt_xml2multi_setup_outdir(ctx, &multi_opts, false);

    /* Initialize depth-specific user data pointer */
    depth_ctx->user_data = alloc_depth_user_data(ctx->depth);
    depth_user = depth_ctx->user_data;

    if (rgt_xml2multi_match_node(
                    &multi_opts,
                    NULL, NULL,
                    ROOT_NODE_DEPTH, ROOT_NODE_SEQ))
    {
        char fname[256];

        rgt_xml2multi_fname(fname, sizeof(fname),
                            &multi_opts, ctx, depth_ctx,
                            NULL, NULL, "json");

        if ((depth_user->f = fopen(fname, "w")) == NULL)
        {
            fprintf(stderr, "Cannot create %s file: %s\n",
                    fname, strerror(errno));
            abort();
        }

        depth_user->json_ctx =
                (te_json_ctx_t)TE_JSON_INIT_FILE(depth_user->f);

        root_start(&depth_user->json_ctx);
    }
    else
    {
        depth_user->f = NULL;
    }
}

/* Release resources associated with given log depth. */
static void
free_depth_user_data(void *data)
{
    depth_ctx_user *depth_user = data;

    te_vec_free(&depth_user->nl_stack);
}

/**
 * Save tree of JSON files to tree.json file.
 */
static void
save_json_tree(void)
{
    static FILE *f_tree = NULL;
    static te_json_ctx_t tree_json_ctx;
    file_descr *file;
    int *file_idx;

    if (te_vec_size(&files) == 0)
        return;

    f_tree = fopen("tree.json", "w");
    if (f_tree == NULL)
    {
        fprintf(stderr, "Cannot create tree.json: %s\n",
                strerror(errno));
        return;
    }

    tree_json_ctx = (te_json_ctx_t)TE_JSON_INIT_FILE(f_tree);
    te_json_start_object(&tree_json_ctx);

    file = te_vec_get(&files, 0);
    te_json_add_key_str(&tree_json_ctx, "main_package", file->fname);

    te_json_add_key(&tree_json_ctx, "tree");
    te_json_start_object(&tree_json_ctx);

    TE_VEC_FOREACH(&files, file)
    {
        te_json_add_key(&tree_json_ctx, file->fname);
        te_json_start_object(&tree_json_ctx);

        te_json_add_key_str(&tree_json_ctx, "id", file->fname);
        te_json_add_key_str(&tree_json_ctx, "name", file->name);
        te_json_add_key(&tree_json_ctx, "has_error");
        te_json_add_bool(&tree_json_ctx, file->has_err);

        te_json_add_key(&tree_json_ctx, "skipped");
        if (file->result != NULL && strcmp(file->result, "SKIPPED") == 0)
            te_json_add_bool(&tree_json_ctx, true);
        else
            te_json_add_bool(&tree_json_ctx, false);

        te_json_add_key(&tree_json_ctx, "entity");
        if (file->type == NT_PACKAGE)
            te_json_add_string(&tree_json_ctx, "pkg");
        else if (file->type == NT_SESSION)
            te_json_add_string(&tree_json_ctx, "session");
        else if (file->type == NT_TEST)
            te_json_add_string(&tree_json_ctx, "test");
        else
            te_json_add_string(&tree_json_ctx, "unknown");

        if (te_vec_size(&file->children) > 0)
        {
            te_json_add_key(&tree_json_ctx, "children");
            te_json_start_array(&tree_json_ctx);

            TE_VEC_FOREACH(&file->children, file_idx)
            {
                file_descr *child;

                child = te_vec_get(&files, *file_idx);
                te_json_add_string(&tree_json_ctx, "%s", child->fname);
            }

            te_json_end(&tree_json_ctx);
        }

        te_json_end(&tree_json_ctx);
    }

    te_json_end(&tree_json_ctx);
    te_json_end(&tree_json_ctx);
    fclose(f_tree);
}

/**
 * Clear array of JSON files information.
 */
static void
free_files_list(void)
{
    file_descr *file;

    TE_VEC_FOREACH(&files, file)
    {
        te_vec_free(&file->children);
        free(file->fname);
        free(file->name);
        free(file->result);
    }

    te_vec_free(&files);
}

RGT_DEF_FUNC(proc_document_end)
{
    depth_ctx_user *depth_user = (depth_ctx_user *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_user->f != NULL)
    {
        maybe_end_entity_list(depth_user);
        root_end(&depth_user->json_ctx);

        fclose(depth_user->f);
        depth_user->f = NULL;
    }

    if (!multi_opts.single_node_match)
    {
        save_json_tree();
        free_files_list();
    }

    rgt_xml2fmt_free_depth_data(&depth_data, &free_depth_user_data);
}

/* Start of package/session/test */
static void
control_node_start(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
                   const char **xml_attrs)
{
    depth_ctx_user *depth_user = NULL;

    const char *name = rgt_tmpls_xml_attrs_get(xml_attrs, "name");
    const char *result = rgt_tmpls_xml_attrs_get(xml_attrs, "result");
    const char *tin = rgt_tmpls_xml_attrs_get(xml_attrs, "tin");
    const char *node_id = rgt_tmpls_xml_attrs_get(xml_attrs,
                                                  "test_id");
    const char *err = rgt_tmpls_xml_attrs_get(xml_attrs, "err");
    const char *hash = rgt_tmpls_xml_attrs_get(xml_attrs, "hash");
    char fname[500] = "";
    bool matched;

    rgt_depth_ctx_t *prev_depth_ctx;
    depth_ctx_user *prev_depth_user;

    const char *node_type_str = rgt_node2str(depth_ctx->type);

    if (name == NULL)
        name = "session";

    depth_user = depth_ctx->user_data = alloc_depth_user_data(ctx->depth);

    rgt_xml2multi_fname(fname, sizeof(fname), &multi_opts,
                        ctx, depth_ctx, tin, node_id, "json");

    matched = rgt_xml2multi_match_node(
                              &multi_opts,
                              tin, node_id,
                              ctx->depth, depth_ctx->seq);

    if (matched)
    {
        te_json_ctx_t *json_ctx = &depth_user->json_ctx;

        if ((depth_user->f = fopen(fname, "w")) == NULL)
        {
            fprintf(stderr, "Cannot create %s file: %s\n",
                    fname, strerror(errno));
            abort();
        }

        depth_user->json_ctx =
                (te_json_ctx_t)TE_JSON_INIT_FILE(depth_user->f);

        root_start(json_ctx);

        te_json_start_object(json_ctx);
        te_json_add_key_str(json_ctx, "type", "te-log-meta");
        te_json_add_key(json_ctx, "entity_model");
        add_entity(json_ctx, node_id, name, node_type_str, result, err,
                   tin, hash);
    }
    else
    {
        depth_user->f = NULL;
        depth_user->file_idx = -1;
    }

    prev_depth_ctx = &g_array_index(ctx->depth_info,
                                    rgt_depth_ctx_t, ctx->depth - 2);
    prev_depth_user = prev_depth_ctx->user_data;

    if (prev_depth_user->f != NULL)
    {
        te_json_ctx_t *json_ctx = &prev_depth_user->json_ctx;

        maybe_start_entity_list(prev_depth_user);

        add_entity(json_ctx, node_id, name, node_type_str, result, err,
                   tin, hash);
    }

    if (!multi_opts.single_node_match)
    {
        file_descr file;

        memset(&file, 0, sizeof(file));
        file.fname = strdup(fname);
        file.type = depth_ctx->type;
        file.name = strdup(name);
        file.result = strdup(result);
        file.children = (te_vec)TE_VEC_INIT(int);
        file.has_err = (te_str_is_null_or_empty(err) ? false : true);

        te_vec_append(&files, &file);
        depth_user->file_idx = te_vec_size(&files) - 1;

        if (prev_depth_user->file_idx >= 0)
        {
            file_descr *parent_file;

            parent_file = te_vec_get(&files, prev_depth_user->file_idx);
            te_vec_append(&parent_file->children,
                          &depth_user->file_idx);
        }
    }
}

/* End of package/session/test */
static void
control_node_end(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
                 const char **xml_attrs)
{
    depth_ctx_user *depth_user = (depth_ctx_user *)depth_ctx->user_data;
    FILE *f = depth_user->f;

    UNUSED(ctx);
    UNUSED(xml_attrs);

    if (f != NULL)
    {
        maybe_end_entity_list(depth_user);

        root_end(&depth_user->json_ctx);
        fclose(f);
        depth_user->f = NULL;
    }
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

/* Start text message content if no content is already started */
static void
maybe_start_text_msg_content(depth_ctx_user *depth_user)
{
    te_json_ctx_t *json_ctx = &depth_user->json_ctx;

    if (!depth_user->in_msg)
        return;

    if (!depth_user->in_content)
    {
        te_json_start_object(json_ctx);
        te_json_add_key_str(json_ctx, "type", "te-log-table-content-text");
        te_json_add_key(json_ctx, "content");
        te_json_start_string(json_ctx);

        depth_user->in_content = true;
    }
}

/* Terminate message content if its filling is in progress */
static void
maybe_end_msg_content(depth_ctx_user *depth_user)
{
    te_json_ctx_t *json_ctx = &depth_user->json_ctx;

    if (!depth_user->in_msg)
        return;

    if (depth_user->in_content)
    {
        te_json_end(json_ctx);
        te_json_end(json_ctx);

        depth_user->in_content = false;
    }
}

/*
 * Terminate the previous log message if it is not terminated it.
 * If the current nesting level decreases, also terminate children
 * lists of previous messages with greater or equal nesting level.
 */
static void
maybe_terminate_msg(depth_ctx_user *depth_user, int nl_num)
{
    te_json_ctx_t *json_ctx = &depth_user->json_ctx;

    if (depth_user->opened_msgs <= 0)
        return;

    /* Terminate the previous message */
    te_json_end(json_ctx);
    depth_user->opened_msgs--;

    /*
     * Terminate children lists of previous messages with
     * greater or equal nesting level.
     */

    if (depth_user->cur_nl > nl_num)
    {
        unsigned int i;
        unsigned int vec_size = te_vec_size(&depth_user->nl_stack);
        int prev_nl;

        /*
         * Log can contain unexpected nesting "jumps" like
         *
         * message N: nesting level 0
         * message N+1: nesting level 2
         *
         * This tool should not crash on encountering this, so a stack is
         * used to keep track of nesting levels of messages for which
         * lists of children are currently filled. It allows to decide
         * which messages (and their lists of children) should be
         * terminated when nesting level decreases.
         */

        for (i = vec_size; i > 0; i--)
        {
            prev_nl = TE_VEC_GET(int, &depth_user->nl_stack,
                                 i - 1);
            if (prev_nl >= nl_num)
            {
                /* Terminate children */
                te_json_end(json_ctx);
                /* Terminate message */
                te_json_end(json_ctx);

                depth_user->opened_msgs--;
                assert(depth_user->opened_msgs >= 0);
            }
            else
            {
                break;
            }
        }

        if (i < vec_size)
            te_vec_remove(&depth_user->nl_stack, i, vec_size - i);
    }
}

/* Define callback for processing a tag in XML log */
#define RGT_XML2JSON_CB(_name, _code) \
RGT_DEF_FUNC(_name)                                                   \
{                                                                     \
    depth_ctx_user *depth_user =                                      \
          (depth_ctx_user *)depth_ctx->user_data;                     \
                                                                      \
    RGT_FUNC_UNUSED_PRMS();                                           \
                                                                      \
    if (depth_user->f != NULL)                                        \
    {                                                                 \
        te_json_ctx_t *json_ctx = &depth_user->json_ctx;              \
                                                                      \
        UNUSED(json_ctx);                                             \
        _code                                                         \
    }                                                                 \
}

RGT_XML2JSON_CB(proc_log_msg_start,
{
    const char *level = rgt_tmpls_xml_attrs_get(xml_attrs, "level");
    const char *entity = rgt_tmpls_xml_attrs_get(xml_attrs, "entity");
    const char *user = rgt_tmpls_xml_attrs_get(xml_attrs, "user");
    const char *ts = rgt_tmpls_xml_attrs_get(xml_attrs, "ts");
    const char *ts_val = rgt_tmpls_xml_attrs_get(xml_attrs, "ts_val");
    const char *nl = rgt_tmpls_xml_attrs_get(xml_attrs, "nl");
    int nl_num;

    assert(te_strtoi(nl, 10, &nl_num) == 0);

    if (depth_user->cur_nl < nl_num)
    {
        if (depth_user->cur_nl != nl_num - 1)
        {
            fprintf(stderr, "Message at %d has nesting level %d "
                    "while the current nesting level is %d\n",
                    depth_user->linum, nl_num, depth_user->cur_nl);
        }

        /* Start filling children of the previous message */
        te_json_add_key(json_ctx, "children");
        te_json_start_array(json_ctx);

        TE_VEC_APPEND(&depth_user->nl_stack, depth_user->cur_nl);
    }
    else
    {
        maybe_terminate_msg(depth_user, nl_num);
    }

    depth_user->cur_nl = nl_num;

    te_json_start_object(json_ctx);

    te_json_add_key(json_ctx, "line_number");
    te_json_add_integer(json_ctx, depth_user->linum);

    te_json_add_key_str(json_ctx, "level", level);
    te_json_add_key_str(json_ctx, "entity_name", entity);
    te_json_add_key_str(json_ctx, "user_name", user);

    if (te_str_is_null_or_empty(ts_val))
    {
        te_json_add_key_str(json_ctx, "timestamp", ts);
    }
    else
    {
        te_json_add_key(json_ctx, "timestamp");
        te_json_start_object(json_ctx);

        te_json_add_key(json_ctx, "timestamp");
        te_json_start_raw(json_ctx);
        te_json_append_raw(json_ctx, ts_val, 0);
        te_json_end(json_ctx);

        te_json_add_key_str(json_ctx, "formatted", ts);

        te_json_end(json_ctx);
    }

    te_json_add_key(json_ctx, "log_content");
    te_json_start_array(json_ctx);

    depth_user->in_msg = true;
    depth_user->opened_msgs++;

    depth_user->in_content = false;
    depth_user->append_chars = true;

    if (strcmp(level, "MI") == 0)
    {
        depth_user->in_content = true;
        depth_user->append_json = true;

        te_json_start_object(json_ctx);
        te_json_add_key_str(json_ctx, "type", "te-log-table-content-mi");
        te_json_add_key(json_ctx, "content");
        te_json_start_raw(json_ctx);
    }

    depth_user->linum++;
})

RGT_XML2JSON_CB(proc_log_msg_end,
{
    maybe_end_msg_content(depth_user);
    te_json_end(json_ctx);

    depth_user->append_chars = false;
    depth_user->in_msg = false;
    depth_user->in_content = false;
    depth_user->append_json = false;
})

RGT_XML2JSON_CB(proc_log_msg_br,
{
    if (depth_user->append_chars)
    {
        maybe_start_text_msg_content(depth_user);
        te_json_append_string(json_ctx, "%s", "\n");
    }
})

RGT_DEF_DUMMY_FUNC(proc_branch_start)
RGT_DEF_DUMMY_FUNC(proc_branch_end)

RGT_XML2JSON_CB(proc_meta_start,
{
    te_json_add_key(json_ctx, "meta");
    te_json_start_object(json_ctx);
})

RGT_XML2JSON_CB(proc_meta_end,
{
    /* Finalize meta object started in proc_meta_start() */
    te_json_end(json_ctx);

    /*
     * Finalize te-log-meta object which was started in
     * control_node_start().
     */
    te_json_end(json_ctx);
})

/* Define callbacks for simple property in <meta> */
#define RGT_XML2JSON_META_PROP_CB(_name, _json_name) \
RGT_XML2JSON_CB(proc_meta_ ## _name ## _start,                        \
{                                                                     \
    te_json_add_key(json_ctx, #_json_name);                           \
    te_json_start_string(json_ctx);                                   \
    depth_user->append_chars = true;                                  \
})                                                                    \
                                                                      \
RGT_XML2JSON_CB(proc_meta_ ## _name ## _end,                          \
{                                                                     \
    te_json_end(json_ctx);                                            \
    depth_user->append_chars = false;                                 \
})

RGT_XML2JSON_META_PROP_CB(start_ts, start)
RGT_XML2JSON_META_PROP_CB(end_ts, end)
RGT_XML2JSON_META_PROP_CB(duration, duration)
RGT_XML2JSON_META_PROP_CB(objective, objective)

/* Define callbacks for array property in <meta> */
#define RGT_XML2JSON_META_ARRAY_PROP_CB(_name, _json_name) \
RGT_XML2JSON_CB(proc_meta_ ## _name ## _start,     \
{                                                  \
    te_json_add_key(json_ctx, #_json_name);        \
    te_json_start_array(json_ctx);                 \
})                                                 \
                                                   \
RGT_XML2JSON_CB(proc_meta_ ## _name ## _end,       \
{                                                  \
    te_json_end(json_ctx);                         \
})

RGT_XML2JSON_META_ARRAY_PROP_CB(params, parameters)

RGT_XML2JSON_CB(proc_meta_param_start,
{
    const char *name = rgt_tmpls_xml_attrs_get(xml_attrs, "name");
    const char *value = rgt_tmpls_xml_attrs_get(xml_attrs, "value");

    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "name", name);
    te_json_add_key_str(json_ctx, "value", value);
    te_json_end(json_ctx);
})

RGT_DEF_DUMMY_FUNC(proc_meta_param_end)

RGT_XML2JSON_META_ARRAY_PROP_CB(reqs, requirements)

RGT_XML2JSON_CB(proc_meta_req_start,
{
    const char *id = rgt_tmpls_xml_attrs_get(xml_attrs, "id");

    te_json_add_string(json_ctx, "%s", id);
})

RGT_DEF_DUMMY_FUNC(proc_meta_req_end)

RGT_XML2JSON_META_ARRAY_PROP_CB(authors, authors)

RGT_XML2JSON_CB(proc_meta_author_start,
{
    const char *name = rgt_tmpls_xml_attrs_get(xml_attrs, "name");
    const char *email = rgt_tmpls_xml_attrs_get(xml_attrs, "email");

    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "author_name", name);
    te_json_add_key_str(json_ctx, "email", email);
    te_json_end(json_ctx);
})

RGT_DEF_DUMMY_FUNC(proc_meta_author_end)

/* Define callbacks for verdict or artifact in <meta> */
#define RGT_XML2JSON_VERDICT_OR_ARTIFACT_CB(_name) \
RGT_XML2JSON_CB(proc_meta_ ## _name ## _start,                          \
{                                                                       \
    const char *level = rgt_tmpls_xml_attrs_get(xml_attrs, "level");    \
                                                                        \
    te_json_start_object(json_ctx);                                     \
    te_json_add_key_str(json_ctx, "level", level);                      \
    te_json_add_key(json_ctx, #_name);                                  \
    te_json_start_string(json_ctx);                                     \
    depth_user->append_chars = true;                                    \
})                                                                      \
                                                                        \
RGT_XML2JSON_CB(proc_meta_ ## _name ## _end,                            \
{                                                                       \
    te_json_end(json_ctx);                                              \
    te_json_end(json_ctx);                                              \
    depth_user->append_chars = false;                                   \
})

RGT_XML2JSON_META_ARRAY_PROP_CB(verdicts, verdicts)
RGT_XML2JSON_VERDICT_OR_ARTIFACT_CB(verdict)

RGT_XML2JSON_META_ARRAY_PROP_CB(artifacts, artifacts)
RGT_XML2JSON_VERDICT_OR_ARTIFACT_CB(artifact)

RGT_XML2JSON_CB(proc_meta_page_start,
{
    if (multi_opts.docs_url != NULL)
    {
        te_json_add_key(json_ctx, "description");
        te_json_start_object(json_ctx);
        te_json_add_key_str(json_ctx, "text", "See page");
        te_json_add_key(json_ctx, "url");
        te_json_start_string(json_ctx);
        te_json_append_string(json_ctx, "%s", multi_opts.docs_url);
        depth_user->append_chars = true;
    }
})

RGT_XML2JSON_CB(proc_meta_page_end,
{
    if (multi_opts.docs_url != NULL)
    {
        te_json_append_string(json_ctx, ".html");
        te_json_end(json_ctx);
        te_json_end(json_ctx);
        depth_user->append_chars = false;
    }
})

RGT_XML2JSON_CB(proc_logs_start,
{
    maybe_end_entity_list(depth_user);
    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "type", "te-log-table");
    te_json_add_key(json_ctx, "data");
    te_json_start_array(json_ctx);
})

RGT_XML2JSON_CB(proc_logs_end,
{
    maybe_terminate_msg(depth_user, -1);
    assert(depth_user->opened_msgs == 0);

    te_json_end(&depth_user->json_ctx);
    te_json_end(&depth_user->json_ctx);
})

RGT_XML2JSON_CB(proc_log_packet_start,
{
    maybe_end_msg_content(depth_user);

    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "type", "te-log-table-content-packet-sniffer");
    te_json_add_key(json_ctx, "content");
    te_json_start_array(json_ctx);
})

RGT_XML2JSON_CB(proc_log_packet_end,
{
    te_json_end(json_ctx);
    te_json_end(json_ctx);
})

RGT_XML2JSON_CB(proc_log_packet_proto_start,
{
    const char *label = rgt_tmpls_xml_attrs_get(xml_attrs, "showname");

    if (label == NULL)
        label = rgt_tmpls_xml_attrs_get(xml_attrs, "name");

    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "label", label);
    te_json_add_key(json_ctx, "content");
    te_json_start_array(json_ctx);
})

RGT_XML2JSON_CB(proc_log_packet_proto_end,
{
    te_json_end(json_ctx);
    te_json_end(json_ctx);
})

RGT_XML2JSON_CB(proc_log_packet_field_start,
{
    const char *label = rgt_tmpls_xml_attrs_get(xml_attrs, "showname");

    if (label != NULL)
        te_json_add_string(json_ctx, "%s", label);
})

RGT_XML2JSON_CB(proc_log_msg_file_start,
{
    maybe_end_msg_content(depth_user);

    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "type", "te-log-table-content-file");
    te_json_add_key(json_ctx, "content");
    te_json_start_string(json_ctx);
    depth_user->in_content = true;
})

RGT_XML2JSON_CB(proc_log_msg_file_end,
{
    maybe_end_msg_content(depth_user);
})

RGT_XML2JSON_CB(proc_mem_dump_start,
{
    maybe_end_msg_content(depth_user);

    te_json_start_object(json_ctx);
    te_json_add_key_str(json_ctx, "type",
                        "te-log-table-content-memory-dump");
    te_json_add_key(json_ctx, "dump");
    te_json_start_array(json_ctx);
    depth_user->in_content = true;
})

RGT_XML2JSON_CB(proc_mem_dump_end,
{
    maybe_end_msg_content(depth_user);
})

RGT_XML2JSON_CB(proc_mem_row_start,
{
    te_json_start_array(json_ctx);
})

RGT_XML2JSON_CB(proc_mem_row_end,
{
    te_json_end(json_ctx);
})

RGT_XML2JSON_CB(proc_mem_elem_start,
{
    te_json_start_string(json_ctx);
})

RGT_XML2JSON_CB(proc_mem_elem_end,
{
    te_json_end(json_ctx);
})

/* Process characters in XML log */
void
proc_chars(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
           const rgt_xmlChar *ch, size_t len)

{
    depth_ctx_user *depth_user = (depth_ctx_user *)depth_ctx->user_data;
    te_json_ctx_t *json_ctx = &depth_user->json_ctx;

    if (depth_user->f != NULL && depth_user->append_chars)
    {
        maybe_start_text_msg_content(depth_user);

        if (depth_user->append_json)
        {
            te_json_append_raw(json_ctx, (const char *)ch, len);
        }
        else
        {
            te_json_append_string(json_ctx, "%.*s", (int)len,
                                  (const char *)ch);
        }
    }

    return;
}
