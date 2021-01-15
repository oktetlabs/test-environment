/** @file
 * @brief Test Environment: xml2text utility callbacks.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>

#ifdef HAVE_LIBJANSSON
#include <jansson.h>
#endif

#include "te_defs.h"
#include "te_dbuf.h"
#include "xml2gen.h"
#include "xml2text.h"

#include "te_queue.h"
#include "te_alloc.h"
#include "te_string.h"

#include "capture.h"
#include "mi_msg.h"

/* Max attribute length in one line */
int rgt_max_attribute_length = 76;
/* A tag to separate lines */
const char *rgt_line_separator = "\n";
/* Flag turning on detailed packet dumps in log. */
int detailed_packets = 0;

/**
 * Flag turning on printing a prefix before each line of the message
 * in log.
 */
static int line_prefix = 0;

/** Description of a flow tree node */
typedef struct flow_item {
    SLIST_ENTRY(flow_item) links; /**< Link to the description of the
                                       parent node */

    int   id;   /**< Node ID */
    int   pid;  /**< Parent node ID */
    char *type; /**< Node type: PACKAGE, SESSION, TEST */
    char *name; /**< Node name */
} flow_item_t;

/** Stack of flow tree nodes that allows simple flow tree traversal */
typedef SLIST_HEAD(flow_stack, flow_item) flow_stack_t;

/**
 * Push a flow tree node description into the stack.
 *
 * If the current top item of the stack has the same parent ID,
 * it is automatically removed.
 *
 * @param stack         Flow tree node stack.
 * @param id            Node ID.
 * @param pid           Parent node ID.
 * @param type          Node type.
 * @param name          Node name.
 *
 * @returns Status code.
 */
static te_errno
flow_stack_push(flow_stack_t *stack, int id, int pid, const char *type, const char *name)
{
    flow_item_t *item;
    char        *ntype;
    char        *nname;

    ntype = strdup(type);
    nname = strdup(name);
    if (ntype == NULL || nname == NULL)
    {
        free(ntype);
        free(nname);
        return TE_ENOMEM;
    }

    item = SLIST_FIRST(stack);
    if (item == NULL || item->pid != pid)
    {
        item = TE_ALLOC(sizeof(*item));
        if (item == NULL)
        {
            free(ntype);
            free(nname);
            return TE_ENOMEM;
        }

        SLIST_INSERT_HEAD(stack, item, links);
    }
    else
    {
        free(item->type);
        free(item->name);
    }

    item->id = id;
    item->pid = pid;
    item->type = ntype;
    item->name = nname;

    return 0;
}

/**
 * Get an item from the stack with the given node ID.
 *
 * This function does not look deeper than the second item of the stack.
 * This is because the node with the given ID can be either:
 *   a) a test, in which case it will be at the top of the stack;
 *   b) an empty session or package, then it will be at the top too;
 *   c) a non-empty session or package, then its child will be at the top,
 *      and the session or package itself will be the second item.
 *
 * @param stack         Flow tree node stack.
 * @param id            Node ID.
 *
 * @returns Pointer to item or NULL.
 */
static flow_item_t *
flow_stack_pop(flow_stack_t *stack, int id)
{
    flow_item_t *top;
    flow_item_t *next;

    top = SLIST_FIRST(stack);
    if (top == NULL)
        return NULL;

    if (top->id == id)
        return top;

    next = SLIST_NEXT(top, links);
    if (next == NULL)
        return NULL;

    if (next->id == id)
    {
        free(top->type);
        free(top->name);
        SLIST_REMOVE_HEAD(stack, links);
        free(top);
        return next;
    }

    return NULL;
}

/** Structure to keep basic user data in general parsing context */
typedef struct gen_ctx_user {
    FILE             *fd; /**< File descriptor of the document to output
                               the result */

    te_bool mi_artifact;  /**< If @c TRUE, MI artifact is processed */
    te_dbuf json_data;    /**< Buffer for collecting JSON before it can
                               be parsed */

    flow_stack_t flow_stack; /**< Flow tree traversal stack */

    char msg_prefix[1024];   /**< Prefix to be printed before every line
                                  of the message */
    int msg_prefix_len;      /**< Length of the message prefix */
} gen_ctx_user_t;

/* RGT format-specific options table */
struct poptOption rgt_options_table[] = {
    { "detailed-packets", 'P', POPT_ARG_NONE, &detailed_packets, 0,
      "Print more detailed packet dumps", NULL },
    { "line-prefix", 'L', POPT_ARG_NONE, &line_prefix, 0,
      "Print prefix before every message line", NULL },
    POPT_TABLEEND
};

/**
 * Output a string to text log (either null-terminated or
 * of specified number of bytes), inserting line prefix
 * after every new line if requested.
 *
 * @param ctx           Logging context.
 * @param p             Pointer to the string.
 * @param size          if not negative, specifies the number
 *                      of bytes to output (otherwise string
 *                      is considered to be null-terminated).
 */
static void
fputs_log(gen_ctx_user_t *ctx, const char *p, ssize_t size)
{
    size_t i;
    size_t prev_pos = 0;

    if (p == NULL)
        return;

    if (ctx->msg_prefix_len == 0)
    {
        if (size >= 0)
            fwrite(p, 1, size, ctx->fd);
        else
            fputs(p, ctx->fd);

        return;
    }

    for (i = 0; (size < 0 && p[i] != '\0') ||
                (size > 0 && i < (size_t)size); i++)
    {
        if (p[i] == '\n')
        {
            fwrite(p + prev_pos, i - prev_pos + 1, 1, ctx->fd);
            fwrite(ctx->msg_prefix, 1, ctx->msg_prefix_len,
                   ctx->fd);
            prev_pos = i + 1;
        }
    }

    if (i > prev_pos)
    {
        fwrite(p + prev_pos, i - prev_pos, 1, ctx->fd);
    }
}

/**
 * Output a sequence of values in text log, inserting line prefix
 * after every new line if requested.
 *
 * @param ctx           Logging context.
 * @param data          Pointer to the sequence.
 * @param size          Size of a sequence element in bytes.
 * @param nmemb         Number of elements in a sequence.
 */
static void
fwrite_log(gen_ctx_user_t *ctx, const void *data, size_t size,
           size_t nmemb)
{
    fputs_log(ctx, (char *)data, size * nmemb);
}

/**
 * Output a formatted string in text log, inserting line
 * prefix after every new line if requested.
 *
 * @param ctx           Logging context.
 * @param fmt           Format string.
 * @param ...           Format arguments.
 */
static void
fprintf_log(gen_ctx_user_t *ctx, const char *fmt, ...)
{
    te_string str = TE_STRING_INIT;
    va_list ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = te_string_append_va(&str, fmt, ap);
    va_end(ap);

    if (rc != 0)
    {
        va_start(ap, fmt);
        vfprintf(ctx->fd, fmt, ap);
        va_end(ap);
    }
    else
    {
        fwrite_log(ctx, str.ptr, str.len, 1);
    }

    te_string_free(&str);
}

/**
 * Wrapper over rgt_tmpls_output() inserting line
 * prefix after every new line if requested.
 *
 * @param ctx           Logging context.
 * @param tmpl          Template to output.
 * @param attr          Template's attributes.
 */
static void
rgt_tmpls_output_log(gen_ctx_user_t *ctx, rgt_tmpl_t *tmpl,
                     const rgt_attrs_t *attrs)
{
    te_string str = TE_STRING_INIT;

    rgt_tmpls_output_str(&str, tmpl, attrs);
    fwrite_log(ctx, str.ptr, str.len, 1);
    te_string_free(&str);
}

void rgt_process_cmdline(rgt_gen_ctx_t *ctx, poptContext con, int val) {
    UNUSED(ctx);
    UNUSED(con);
    UNUSED(val);
}

RGT_DEF_FUNC(proc_document_start)
{
    static gen_ctx_user_t  user_ctx;
    gen_ctx_user_t        *gen_user;

    RGT_FUNC_UNUSED_PRMS();

    /* Initialize a pointer to generic user-specific data */
    ctx->user_data = gen_user = &user_ctx;

    user_ctx.json_data = (te_dbuf)TE_DBUF_INIT(0);
    user_ctx.mi_artifact = FALSE;

    user_ctx.msg_prefix_len = 0;

    /* In text output all XML entities should be expanded */
    ctx->expand_entities = TRUE;

    if (ctx->out_fname == NULL)
        gen_user->fd = stdout;
    else
        gen_user->fd = fopen(ctx->out_fname, "w");

    if (gen_user->fd == NULL)
    {
        perror(ctx->out_fname);
        exit(2);
    }

    rgt_tmpls_output(gen_user->fd, &xml2fmt_tmpls[DOCUMENT_START], NULL);
}

RGT_DEF_FUNC(proc_document_end)
{
    gen_ctx_user_t *user_ctx = (gen_ctx_user_t *)(ctx->user_data);
    FILE *fd = user_ctx->fd;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(fd, &xml2fmt_tmpls[DOCUMENT_END], NULL);
    fclose(fd);

    te_dbuf_free(&user_ctx->json_data);
}

RGT_DEF_DUMMY_FUNC(proc_session_start)
RGT_DEF_DUMMY_FUNC(proc_session_end)
RGT_DEF_DUMMY_FUNC(proc_pkg_start)
RGT_DEF_DUMMY_FUNC(proc_pkg_end)
RGT_DEF_DUMMY_FUNC(proc_test_start)
RGT_DEF_DUMMY_FUNC(proc_test_end)

#define DEF_FUNC_WITH_ATTRS(name_, enum_const_) \
RGT_DEF_FUNC(name_)                                                 \
{                                                                   \
    rgt_attrs_t    *attrs;                                          \
    gen_ctx_user_t *user_ctx = (gen_ctx_user_t *)(ctx->user_data);  \
                                                                    \
    RGT_FUNC_UNUSED_PRMS();                                         \
                                                                    \
    attrs = rgt_tmpls_attrs_new(xml_attrs);                         \
    rgt_tmpls_output_log(user_ctx,                                  \
                         &xml2fmt_tmpls[enum_const_],               \
                         attrs);                                    \
    rgt_tmpls_attrs_free(attrs);                                    \
}


#define DEF_FUNC_WITHOUT_ATTRS(name_, enum_const_) \
RGT_DEF_FUNC(name_)                                          \
{                                                            \
    gen_ctx_user_t *user_ctx =                               \
                      (gen_ctx_user_t *)(ctx->user_data);    \
    RGT_FUNC_UNUSED_PRMS();                                  \
                                                             \
    rgt_tmpls_output_log(user_ctx,                           \
                         &xml2fmt_tmpls[enum_const_],        \
                         NULL);                              \
}

RGT_DEF_DUMMY_FUNC(proc_log_packet_end)
RGT_DEF_DUMMY_FUNC(proc_log_packet_proto_end)

RGT_DEF_DUMMY_FUNC(proc_branch_start)
RGT_DEF_DUMMY_FUNC(proc_branch_end)
RGT_DEF_DUMMY_FUNC(proc_meta_param_start)
RGT_DEF_DUMMY_FUNC(proc_meta_param_end)
RGT_DEF_DUMMY_FUNC(proc_logs_start)
RGT_DEF_DUMMY_FUNC(proc_logs_end)
RGT_DEF_DUMMY_FUNC(proc_meta_start)
RGT_DEF_DUMMY_FUNC(proc_meta_end)
RGT_DEF_DUMMY_FUNC(proc_meta_start_ts_start)
RGT_DEF_DUMMY_FUNC(proc_meta_start_ts_end)
RGT_DEF_DUMMY_FUNC(proc_meta_end_ts_start)
RGT_DEF_DUMMY_FUNC(proc_meta_end_ts_end)
RGT_DEF_DUMMY_FUNC(proc_meta_duration_start)
RGT_DEF_DUMMY_FUNC(proc_meta_duration_end)
RGT_DEF_DUMMY_FUNC(proc_meta_objective_start)
RGT_DEF_DUMMY_FUNC(proc_meta_objective_end)
RGT_DEF_DUMMY_FUNC(proc_meta_author_start)
RGT_DEF_DUMMY_FUNC(proc_meta_author_end)
RGT_DEF_DUMMY_FUNC(proc_meta_authors_start)
RGT_DEF_DUMMY_FUNC(proc_meta_authors_end)
RGT_DEF_DUMMY_FUNC(proc_meta_verdict_start)
RGT_DEF_DUMMY_FUNC(proc_meta_verdict_end)
RGT_DEF_DUMMY_FUNC(proc_meta_verdicts_start)
RGT_DEF_DUMMY_FUNC(proc_meta_verdicts_end)
RGT_DEF_DUMMY_FUNC(proc_meta_artifact_start)
RGT_DEF_DUMMY_FUNC(proc_meta_artifact_end)
RGT_DEF_DUMMY_FUNC(proc_meta_artifacts_start)
RGT_DEF_DUMMY_FUNC(proc_meta_artifacts_end)
RGT_DEF_DUMMY_FUNC(proc_meta_params_start)
RGT_DEF_DUMMY_FUNC(proc_meta_params_end)
RGT_DEF_DUMMY_FUNC(proc_meta_page_start)
RGT_DEF_DUMMY_FUNC(proc_meta_page_end)

DEF_FUNC_WITHOUT_ATTRS(proc_mem_dump_start, MEM_DUMP_START)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_dump_end, MEM_DUMP_END)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_row_start, MEM_DUMP_ROW_START)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_row_end, MEM_DUMP_ROW_END)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_elem_start, MEM_DUMP_ELEM_START)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_elem_end, MEM_DUMP_ELEM_END)
DEF_FUNC_WITHOUT_ATTRS(proc_log_msg_br, BR)

DEF_FUNC_WITH_ATTRS(proc_log_msg_file_start, LOG_MSG_FILE_START)
DEF_FUNC_WITHOUT_ATTRS(proc_log_msg_file_end, LOG_MSG_FILE_END)

RGT_DEF_FUNC(proc_log_msg_start)
{
    const char *level = rgt_tmpls_xml_attrs_get(xml_attrs, "level");
    gen_ctx_user_t *user_ctx = (gen_ctx_user_t *)(ctx->user_data);
    FILE *fd = user_ctx->fd;
    rgt_attrs_t *attrs;

    te_string str = TE_STRING_BUF_INIT(user_ctx->msg_prefix);

    RGT_FUNC_UNUSED_PRMS();

    if (level != NULL && strcmp(level, "MI") == 0)
        user_ctx->mi_artifact = TRUE;

    attrs = rgt_tmpls_attrs_new(xml_attrs);

    if (line_prefix)
    {
        te_string_append(&str, "[");
        rgt_tmpls_output_str(&str,
                             &xml2fmt_tmpls[LOG_MSG_START_LINE_PREFIX],
                             attrs);
        te_string_append(&str, "]: ");
        user_ctx->msg_prefix_len = str.len;
        fwrite(user_ctx->msg_prefix, 1, user_ctx->msg_prefix_len, fd);
    }
    else
    {
        rgt_tmpls_output(fd, &xml2fmt_tmpls[LOG_MSG_START], attrs);
    }

    rgt_tmpls_attrs_free(attrs);
}

/**
 * Print a measurement value.
 *
 * @param ctx     Logging context.
 * @param value   Value to print.
 * @param prefix  Prefix string (may be @c NULL).
 */
static void
print_mi_meas_value(gen_ctx_user_t *ctx, te_rgt_mi_meas_value *value,
                    const char *prefix)
{
    if (!(value->defined))
        return;

    if (prefix != NULL)
        fprintf_log(ctx, "%15s: ", prefix);

    if (value->specified)
        fprintf_log(ctx, "%f", value->value);
    else
        fprintf_log(ctx, "[failed to obtain]");

    if (value->multiplier != NULL && *(value->multiplier) != '\0' &&
        strcmp(value->multiplier, "1") != 0)
        fprintf_log(ctx, " * %s", value->multiplier);
    if (value->base_units != NULL && *(value->base_units) != '\0')
        fprintf_log(ctx, " %s", value->base_units);

    fprintf_log(ctx, "\n");
}

/**
 * Log MI result.
 *
 * @param ctx     Logging context.
 * @param result  Result that should be printed.
 */
static void
log_mi_result(gen_ctx_user_t *ctx, te_rgt_mi_test_result *result)
{
    size_t i;

    fprintf_log(ctx, "Status: %s", result->status);

    if (result->verdicts != NULL)
    {
        fprintf_log(ctx, "\nVerdicts:");
        for (i = 0; i < result->verdicts_num; i++)
            fprintf_log(ctx, "\n *  %s", result->verdicts[i]);
    }

    if (result->notes != NULL)
        fprintf_log(ctx, "\nNotes: %s", result->notes);
    if (result->key != NULL)
        fprintf_log(ctx, "\nKey: %s", result->key);
}

/** Transform node type from MI message to human-readable form */
static const char *
node_type2str(const char *node_type)
{
    if (strcmp(node_type, "pkg") == 0)
        return "PACKAGE";
    else if (strcmp(node_type, "session") == 0)
        return "SESSION";
    else if (strcmp(node_type, "test") == 0)
        return "TEST";

    return node_type;
}

/**
 * Log MI test start message.
 *
 * @param ctx     Logging context.
 * @param mi      Structure with data from parsed MI artifact.
 */
static void
log_mi_test_start(gen_ctx_user_t *ctx, te_rgt_mi *mi)
{
    te_rgt_mi_test_start *data = &mi->data.test_start;

    size_t   i;
    te_errno rc;

    fprintf_log(ctx, "%s \"%s\" started\n",
                node_type2str(data->node_type),
                data->name);
    fprintf_log(ctx, "Node ID %d, Parent ID %d", data->node_id,
                data->parent_id);
    if (data->plan_id != -1)
        fprintf_log(ctx, ", Plan ID %d", data->plan_id);

    rc = flow_stack_push(&ctx->flow_stack, data->node_id, data->parent_id,
                         data->node_type, data->name);
    if (rc != 0)
    {
        fprintf_log(ctx, "\nRGT ERROR: Failed to push the flow item: %s",
                    te_rc_err2str(mi->rc));
    }

    if (data->authors != NULL)
    {
        fprintf_log(ctx, "\nAuthors:");
        for(i = 0; i < data->authors_num; i++)
        {
            const char *name  = data->authors[i].name;
            const char *email = data->authors[i].email;

            if (name == NULL)
                name = "";
            if (email == NULL)
                email = "";

            fprintf_log(ctx, "\n *  %s <%s>", name, email);
        }
    }

    if (data->objective != NULL)
        fprintf_log(ctx, "\nObjective: %s", data->objective);
    if (data->page != NULL)
        fprintf_log(ctx, "\nPage: %s", data->page);
    if (data->tin != -1)
        fprintf_log(ctx, "\nTIN: %d", data->tin);
    if (data->hash != NULL)
        fprintf_log(ctx, "\nHash: %s", data->hash);

    if (data->params != NULL)
    {
        fprintf_log(ctx, "\nParameters:");
        for(i = 0; i < data->params_num; i++)
        {
            fprintf_log(ctx, "\n *  %s = %s",
                        data->params[i].key,
                        data->params[i].value);
        }
    }
}

/**
 * Log MI test end message.
 *
 * @param ctx     Logging context.
 * @param mi      Structure with data from parsed MI artifact.
 */
static void
log_mi_test_end(gen_ctx_user_t *ctx, te_rgt_mi *mi)
{
    te_rgt_mi_test_end *data = &mi->data.test_end;
    flow_item_t        *item;
    size_t              i;

    item = flow_stack_pop(&ctx->flow_stack, data->node_id);
    if (item != NULL)
    {
        fprintf_log(ctx, "%s \"%s\" finished\n",
                    node_type2str(item->type),
                    item->name);
        fprintf_log(ctx, "Node ID %d, Parent ID %d", data->node_id,
                    data->parent_id);
        if (data->plan_id != -1)
            fprintf_log(ctx, ", Plan ID %d", data->plan_id);
    }
    else
    {
        fprintf_log(ctx, "(%d, %d) finished with status \"%s\"",
                    data->node_id, data->parent_id,
                    data->obtained.status);
    }

    if (data->tags_expr != NULL)
    {
        fprintf_log(ctx, "\nMatched tags expression: %s",
                    data->tags_expr);
    }

    fprintf_log(ctx, "\n\nObtained result:\n");
    log_mi_result(ctx, &data->obtained);

    if (data->error != NULL)
        fprintf_log(ctx, "\n\nERROR: %s", data->error);

    if (data->expected != NULL)
    {
        fprintf_log(ctx, "\n\nExpected results:");
        for (i = 0; i < data->expected_num; i++)
        {
            fprintf_log(ctx, "\n\n");
            log_mi_result(ctx, &data->expected[i]);
        }
    }
}

/**
 * Log MI artifact.
 *
 * @param ctx     Logging context.
 * @param mi      Structure with data from parsed MI artifact.
 */
static void
log_mi_artifact(gen_ctx_user_t *ctx, te_rgt_mi *mi)
{
    char *s = NULL;

    if (mi->parse_failed)
    {
        fprintf_log(ctx, "Failed to parse JSON: %s\n", mi->parse_err);
        fwrite_log(ctx, ctx->json_data.ptr, ctx->json_data.len, 1);
        return;
    }

    if (mi->type == TE_RGT_MI_TYPE_UNKNOWN || mi->rc != 0)
    {
        if (mi->rc != 0)
        {
            if (mi->rc == TE_EOPNOTSUPP)
            {
                fprintf_log(ctx, "Cannot parse MI artifact without "
                            "libjansson\n");
            }
            else
            {
                fprintf_log(ctx, "Failed to process MI artifact, "
                            "error = %s\n", te_rc_err2str(mi->rc));
            }
        }

#ifdef HAVE_LIBJANSSON
        s = json_dumps((json_t *)(mi->json_obj), JSON_INDENT(2));
        if (s != NULL)
        {
            fputs_log(ctx, s, -1);
            free(s);
        }
#endif

        if (s == NULL)
            fwrite_log(ctx, ctx->json_data.ptr, ctx->json_data.len, 1);
    }
    else if (mi->type == TE_RGT_MI_TYPE_MEASUREMENT)
    {
        te_rgt_mi_meas *meas = &mi->data.measurement;
        size_t i;
        size_t j;

        fprintf_log(ctx, "Measurements from tool %s\n", meas->tool);
        for (i = 0; i < meas->params_num; i++)
        {
            te_rgt_mi_meas_param *param;

            param = &meas->params[i];

            fprintf_log(ctx, "\nMeasured parameter: \"%s\"\n",
                        te_rgt_mi_meas_param_name(param));

            if (param->stats_present)
            {
                fprintf_log(ctx, "Statistics:\n");
                print_mi_meas_value(ctx, &param->min, "min");
                print_mi_meas_value(ctx, &param->max, "max");
                print_mi_meas_value(ctx, &param->mean, "mean");
                print_mi_meas_value(ctx, &param->median, "median");
                print_mi_meas_value(ctx, &param->stdev, "stdev");
                print_mi_meas_value(ctx, &param->cv, "cv");
                print_mi_meas_value(ctx, &param->out_of_range,
                                    "out of range");
                print_mi_meas_value(ctx, &param->percentile, "percentile");
            }

            if (param->values_num > 0)
            {
                fprintf_log(ctx, "Values:\n");
                for (j = 0; j < param->values_num; j++)
                {
                    print_mi_meas_value(ctx, &param->values[j], NULL);
                }
            }
        }

        if (meas->keys_num > 0)
        {
            fprintf_log(ctx, "\nKeys:\n");
            for (i = 0; i < meas->keys_num; i++)
            {
                fprintf_log(ctx, "\"%s\" : \"%s\"\n", meas->keys[i].key,
                            meas->keys[i].value);
            }
        }

        if (meas->comments_num > 0)
        {
            fprintf_log(ctx, "\nComments:\n");
            for (i = 0; i < meas->comments_num; i++)
            {
                fprintf_log(ctx, "\"%s\" : \"%s\"\n",
                            meas->comments[i].key,
                            meas->comments[i].value);
            }
        }
    }
    else if (mi->type == TE_RGT_MI_TYPE_TEST_START)
    {
        log_mi_test_start(ctx, mi);
    }
    else if (mi->type == TE_RGT_MI_TYPE_TEST_END)
    {
        log_mi_test_end(ctx, mi);
    }
}

RGT_DEF_FUNC(proc_log_msg_end)
{
    gen_ctx_user_t *user_ctx = (gen_ctx_user_t *)(ctx->user_data);
    FILE *fd = user_ctx->fd;
    rgt_attrs_t *attrs;

    RGT_FUNC_UNUSED_PRMS();

    if (user_ctx->mi_artifact)
    {
        if (user_ctx->json_data.ptr != NULL)
        {
            te_rgt_mi mi;

            te_rgt_parse_mi_message((char *)(user_ctx->json_data.ptr),
                                    user_ctx->json_data.len, &mi);

            log_mi_artifact(user_ctx, &mi);

            te_rgt_mi_clean(&mi);
        }

        user_ctx->mi_artifact = FALSE;
        te_dbuf_reset(&user_ctx->json_data);
    }

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_output(fd,
                     &xml2fmt_tmpls[line_prefix ?
                                          LOG_MSG_END_LINE_PREFIX :
                                          LOG_MSG_END],
                     attrs);
    rgt_tmpls_attrs_free(attrs);

    user_ctx->msg_prefix_len = 0;
}

void
proc_chars(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
           const rgt_xmlChar *ch, size_t len)
{
    gen_ctx_user_t *user_ctx = (gen_ctx_user_t *)(ctx->user_data);

    UNUSED(ctx);
    UNUSED(depth_ctx);

    if (user_ctx->mi_artifact)
    {
        te_dbuf_append(&user_ctx->json_data, ch, len);
        return;
    }

    fwrite_log(user_ctx, ch, len, 1);
}

te_bool
proc_expand_entities(void)
{
    return TRUE;
}
