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

#include "capture.h"
#include "mi_msg.h"

/* Max attribute length in one line */
int rgt_max_attribute_length = 76;
/* A tag to separate lines */
const char *rgt_line_separator = "\n";
/* Flag turning on detailed packet dumps in log. */
int detailed_packets = 0;

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
} gen_ctx_user_t;

/* RGT format-specific options table */
struct poptOption rgt_options_table[] = {
    { "detailed-packets", 'P', POPT_ARG_NONE, &detailed_packets, 0,
      "Print more detailed packet dumps", NULL },
    POPT_TABLEEND
};

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
RGT_DEF_FUNC(name_)                                           \
{                                                             \
    FILE *fd = ((gen_ctx_user_t *)ctx->user_data)->fd;        \
    rgt_attrs_t    *attrs;                                    \
                                                              \
    RGT_FUNC_UNUSED_PRMS();                                   \
                                                              \
    attrs = rgt_tmpls_attrs_new(xml_attrs);                   \
    rgt_tmpls_output(fd, &xml2fmt_tmpls[enum_const_], attrs); \
    rgt_tmpls_attrs_free(attrs);                              \
}


#define DEF_FUNC_WITHOUT_ATTRS(name_, enum_const_) \
RGT_DEF_FUNC(name_)                                          \
{                                                            \
    FILE *fd = ((gen_ctx_user_t *)ctx->user_data)->fd;       \
                                                             \
    RGT_FUNC_UNUSED_PRMS();                                  \
                                                             \
    rgt_tmpls_output(fd, &xml2fmt_tmpls[enum_const_], NULL); \
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

    RGT_FUNC_UNUSED_PRMS();

    if (level != NULL && strcmp(level, "MI") == 0)
        user_ctx->mi_artifact = TRUE;

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_output(fd, &xml2fmt_tmpls[LOG_MSG_START], attrs);
    rgt_tmpls_attrs_free(attrs);
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

    if (prefix != NULL)
        fprintf(fd, "%15s: ", prefix);

    if (value->specified)
        fprintf(fd, "%f", value->value);
    else
        fprintf(fd, "[failed to obtain]");

    if (value->multiplier != NULL && *(value->multiplier) != '\0' &&
        strcmp(value->multiplier, "1") != 0)
        fprintf(fd, " * %s", value->multiplier);
    if (value->base_units != NULL && *(value->base_units) != '\0')
        fprintf(fd, " %s", value->base_units);

    fprintf(fd, "\n");
}

/**
 * Log MI result.
 *
 * @param fd      Where to print a log.
 * @param result  Result that should be printed.
 */
static void
log_mi_result(FILE *fd, te_rgt_mi_test_result *result)
{
    size_t i;

    fprintf(fd, "Status: %s", result->status);

    if (result->verdicts != NULL)
    {
        fprintf(fd, "\nVerdicts:");
        for (i = 0; i < result->verdicts_num; i++)
            fprintf(fd, "\n *  %s", result->verdicts[i]);
    }

    if (result->notes != NULL)
        fprintf(fd, "\nNotes: %s", result->notes);
    if (result->key != NULL)
        fprintf(fd, "\nKey: %s", result->key);
}

/**
 * Log MI test start message.
 *
 * @param fd      Where to print a log.
 * @param mi      Structure with data from parsed MI artifact.
 * @param ctx     Logging context.
 */
static void
log_mi_test_start(FILE *fd, te_rgt_mi *mi, gen_ctx_user_t *ctx)
{
    te_rgt_mi_test_start *data = &mi->data.test_start;

    size_t   i;
    te_errno rc;

    fprintf(fd, "%s \"%s\" started\n", data->node_type, data->name);
    fprintf(fd, "Node ID %d, Parent ID %d", data->node_id, data->parent_id);
    if (data->plan_id != -1)
        fprintf(fd, ", Plan ID %d", data->plan_id);

    rc = flow_stack_push(&ctx->flow_stack, data->node_id, data->parent_id,
                         data->node_type, data->name);
    if (rc != 0)
    {
        fprintf(fd, "\nRGT ERROR: Failed to push the flow item: %s",
                te_rc_err2str(mi->rc));
    }

    if (data->authors != NULL)
    {
        fprintf(fd, "\nAuthors:");
        for(i = 0; i < data->authors_num; i++)
        {
            const char *name  = data->authors[i].name;
            const char *email = data->authors[i].email;

            if (name == NULL)
                name = "";
            if (email == NULL)
                email = "";

            fprintf(fd, "\n *  %s <%s>", name, email);
        }
    }

    if (data->objective != NULL)
        fprintf(fd, "\nObjective: %s", data->objective);
    if (data->page != NULL)
        fprintf(fd, "\nPage: %s", data->page);
    if (data->tin != -1)
        fprintf(fd, "\nTIN: %d", data->tin);
    if (data->hash != NULL)
        fprintf(fd, "\nHash: %s", data->hash);

    if (data->params != NULL)
    {
        fprintf(fd, "\nParameters:");
        for(i = 0; i < data->params_num; i++)
        {
            fprintf(fd, "\n *  %s = %s",
                    data->params[i].key,
                    data->params[i].value);
        }
    }
}

/**
 * Log MI test end message.
 *
 * @param fd      Where to print a log.
 * @param mi      Structure with data from parsed MI artifact.
 * @param ctx     Logging context.
 */
static void
log_mi_test_end(FILE *fd, te_rgt_mi *mi, gen_ctx_user_t *ctx)
{
    te_rgt_mi_test_end *data = &mi->data.test_end;
    flow_item_t        *item;
    size_t              i;

    item = flow_stack_pop(&ctx->flow_stack, data->node_id);
    if (item != NULL)
    {
        fprintf(fd, "%s \"%s\" finished\n", item->type, item->name);
        fprintf(fd, "Node ID %d, Parent ID %d", data->node_id,
                data->parent_id);
        if (data->plan_id != -1)
            fprintf(fd, ", Plan ID %d", data->plan_id);
    }
    else
    {
        fprintf(fd, "(%d, %d) finished with status \"%s\"", data->node_id,
                data->parent_id, data->obtained.status);
    }

    if (data->tags_expr != NULL)
        fprintf(fd, "\nMatched tags expression: %s", data->tags_expr);

    fprintf(fd, "\n\nObtained result:\n");
    log_mi_result(fd, &data->obtained);

    if (data->error != NULL)
        fprintf(fd, "\n\nERROR: %s", data->error);

    if (data->expected != NULL)
    {
        fprintf(fd, "\n\nExpected results:");
        for (i = 0; i < data->expected_num; i++)
        {
            fprintf(fd, "\n\n");
            log_mi_result(fd, &data->expected[i]);
        }
    }
}

/**
 * Log MI artifact.
 *
 * @param fd      Where to print a log.
 * @param mi      Structure with data from parsed MI artifact.
 * @param ctx     Logging context.
 */
static void
log_mi_artifact(FILE *fd, te_rgt_mi *mi, gen_ctx_user_t *ctx)
{
    int res = -1;

    if (mi->parse_failed)
    {
        fprintf(fd, "Failed to parse JSON: %s\n", mi->parse_err);
        fwrite(ctx->json_data.ptr, ctx->json_data.len, 1, fd);
        return;
    }

    if (mi->type == TE_RGT_MI_TYPE_UNKNOWN || mi->rc != 0)
    {
        if (mi->rc != 0)
        {
            if (mi->rc == TE_EOPNOTSUPP)
            {
                fprintf(fd, "Cannot parse MI artifact without "
                        "libjansson\n");
            }
            else
            {
                fprintf(fd, "Failed to process MI artifact, error = %s\n",
                        te_rc_err2str(mi->rc));
            }
        }

#ifdef HAVE_LIBJANSSON
        res = json_dumpf((json_t *)(mi->json_obj), fd, JSON_INDENT(2));
#endif

        if (res < 0)
            fwrite(ctx->json_data.ptr, ctx->json_data.len, 1, fd);
    }
    else if (mi->type == TE_RGT_MI_TYPE_MEASUREMENT)
    {
        te_rgt_mi_meas *meas = &mi->data.measurement;
        size_t i;
        size_t j;

        fprintf(fd, "Measurements from tool %s\n", meas->tool);
        for (i = 0; i < meas->params_num; i++)
        {
            te_rgt_mi_meas_param *param;

            param = &meas->params[i];

            fprintf(fd, "\nMeasured parameter: \"%s\"\n",
                    te_rgt_mi_meas_param_name(param));

            if (param->stats_present)
            {
                fprintf(fd, "Statistics:\n");
                print_mi_meas_value(fd, &param->min, "min");
                print_mi_meas_value(fd, &param->max, "max");
                print_mi_meas_value(fd, &param->mean, "mean");
                print_mi_meas_value(fd, &param->median, "median");
                print_mi_meas_value(fd, &param->stdev, "stdev");
                print_mi_meas_value(fd, &param->cv, "cv");
                print_mi_meas_value(fd, &param->out_of_range, "out of range");
                print_mi_meas_value(fd, &param->percentile, "percentile");
            }

            if (param->values_num > 0)
            {
                fprintf(fd, "Values:\n");
                for (j = 0; j < param->values_num; j++)
                {
                    print_mi_meas_value(fd, &param->values[j], NULL);
                }
            }
        }

        if (meas->keys_num > 0)
        {
            fprintf(fd, "\nKeys:\n");
            for (i = 0; i < meas->keys_num; i++)
            {
                fprintf(fd, "\"%s\" : \"%s\"\n", meas->keys[i].key,
                        meas->keys[i].value);
            }
        }

        if (meas->comments_num > 0)
        {
            fprintf(fd, "\nComments:\n");
            for (i = 0; i < meas->comments_num; i++)
            {
                fprintf(fd, "\"%s\" : \"%s\"\n", meas->comments[i].key,
                        meas->comments[i].value);
            }
        }
    }
    else if (mi->type == TE_RGT_MI_TYPE_TEST_START)
    {
        log_mi_test_start(fd, mi, ctx);
    }
    else if (mi->type == TE_RGT_MI_TYPE_TEST_END)
    {
        log_mi_test_end(fd, mi, ctx);
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

            log_mi_artifact(fd, &mi, user_ctx);

            te_rgt_mi_clean(&mi);
        }

        user_ctx->mi_artifact = FALSE;
        te_dbuf_reset(&user_ctx->json_data);
    }

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_output(fd, &xml2fmt_tmpls[LOG_MSG_END], attrs);
    rgt_tmpls_attrs_free(attrs);
}

void
proc_chars(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
           const rgt_xmlChar *ch, size_t len)
{
    gen_ctx_user_t *user_ctx = (gen_ctx_user_t *)(ctx->user_data);
    FILE *fd = user_ctx->fd;

    UNUSED(ctx);
    UNUSED(depth_ctx);

    if (user_ctx->mi_artifact)
    {
        te_dbuf_append(&user_ctx->json_data, ch, len);
        return;
    }

    fwrite(ch, len, 1, fd);
}

te_bool
proc_expand_entities(void)
{
    return TRUE;
}
