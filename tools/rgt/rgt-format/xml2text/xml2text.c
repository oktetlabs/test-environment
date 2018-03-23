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

#include "xml2gen.h"
#include "xml2text.h"

#include "capture.h"

/* Max attribute length in one line */
int rgt_max_attribute_length = 76;
/* A tag to separate lines */
const char *rgt_line_separator = "\n";
/* Flag turning on detailed packet dumps in log. */
int detailed_packets = 0;

/** Structure to keep basic user data in general parsing context */
typedef struct gen_ctx_user {
    FILE             *fd; /**< File descriptor of the document to output 
                               the result */
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
    FILE *fd = ((gen_ctx_user_t *)ctx->user_data)->fd;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(fd, &xml2fmt_tmpls[DOCUMENT_END], NULL);
    fclose(fd);
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

DEF_FUNC_WITH_ATTRS(proc_log_msg_start, LOG_MSG_START)
DEF_FUNC_WITHOUT_ATTRS(proc_log_msg_end, LOG_MSG_END)

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

void
proc_chars(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
           const rgt_xmlChar *ch, size_t len)
{
    FILE *fd = ((gen_ctx_user_t *)ctx->user_data)->fd;

    UNUSED(ctx);
    UNUSED(depth_ctx);

    fwrite(ch, len, 1, fd);
}

te_bool
proc_expand_entities(void)
{
    return TRUE;
}
