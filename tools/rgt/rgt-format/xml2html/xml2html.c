/** @file 
 * @brief Test Environment: xml2html utility callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>

#include "xml2gen.h"
#include "xml2html.h"

/** Enumeration for colours of the log entries */
typedef enum rgt_row_colour {
    RGT_ROW_COL_LIGHT, /**< Entry should be drawn in a light hue */
    RGT_ROW_COL_DARK, /**< Entry should be drawn in a dark hue */
} rgt_row_colour_t;

/** Structure to keep basic user data in general parsing context */
typedef struct gen_ctx_user {
    FILE             *fd; /**< File descriptor of the document to output 
                               the result */
    rgt_row_colour_t  col; /**< The colour of the current log message */
} gen_ctx_user_t;


RGT_DEF_FUNC(proc_document_start)
{
    static gen_ctx_user_t  user_ctx;
    gen_ctx_user_t        *gen_user;
    rgt_attrs_t           *attrs;

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

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "DATADIR", INSTALL_DIR);
    rgt_tmpls_output(gen_user->fd, &xml2fmt_tmpls[DOCUMENT_START], attrs);
    rgt_tmpls_attrs_free(attrs);
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

RGT_DEF_FUNC(proc_log_msg_start)
{
    gen_ctx_user_t *gen_user = (gen_ctx_user_t *)ctx->user_data;
    rgt_attrs_t    *attrs;

    RGT_FUNC_UNUSED_PRMS();

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "row_class",
        (gen_user->col == RGT_ROW_COL_LIGHT) ? "tdlight" : "tddark");
    rgt_tmpls_output(gen_user->fd, &xml2fmt_tmpls[LOG_MSG_START], attrs);
    rgt_tmpls_attrs_free(attrs);
}

RGT_DEF_FUNC(proc_log_msg_end)
{
    gen_ctx_user_t *gen_user = (gen_ctx_user_t *)ctx->user_data;
    rgt_attrs_t    *attrs;

    RGT_FUNC_UNUSED_PRMS();

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "row_class",
        (gen_user->col == RGT_ROW_COL_LIGHT) ? "tdlight" : "tddark");
    rgt_tmpls_output(gen_user->fd, &xml2fmt_tmpls[LOG_MSG_END], attrs);
    rgt_tmpls_attrs_free(attrs);
    
    gen_user->col = (gen_user->col == RGT_ROW_COL_LIGHT) ?
        RGT_ROW_COL_DARK : RGT_ROW_COL_LIGHT;
}

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
RGT_DEF_DUMMY_FUNC(proc_meta_params_start)
RGT_DEF_DUMMY_FUNC(proc_meta_params_end)

#define DEF_FUNC_WITHOUT_ATTRS(name_, enum_const_) \
RGT_DEF_FUNC(name_)                                          \
{                                                            \
    FILE *fd = ((gen_ctx_user_t *)ctx->user_data)->fd;       \
                                                             \
    RGT_FUNC_UNUSED_PRMS();                                  \
                                                             \
    rgt_tmpls_output(fd, &xml2fmt_tmpls[enum_const_], NULL); \
}

DEF_FUNC_WITHOUT_ATTRS(proc_mem_dump_start, MEM_DUMP_START)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_dump_end, MEM_DUMP_END)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_row_start, MEM_DUMP_ROW_START)

RGT_DEF_FUNC(proc_mem_row_end)
{
    FILE *fd = ((gen_ctx_user_t *)ctx->user_data)->fd;
    int   i;
    int   unfilled = ctx->mem_ctx.mem_width - ctx->mem_ctx.cur_num;

    RGT_FUNC_UNUSED_PRMS();

    assert(unfilled >= 0);

    for (i = 0; i < unfilled; i++)
    {
        rgt_tmpls_output(fd, &xml2fmt_tmpls[MEM_DUMP_ELEM_EMPTY], NULL);
    }

    rgt_tmpls_output(fd, &xml2fmt_tmpls[MEM_DUMP_ROW_END], NULL);
}

DEF_FUNC_WITHOUT_ATTRS(proc_mem_elem_start, MEM_DUMP_ELEM_START)
DEF_FUNC_WITHOUT_ATTRS(proc_mem_elem_end, MEM_DUMP_ELEM_END)
DEF_FUNC_WITHOUT_ATTRS(proc_log_msg_br, BR)

void
proc_chars(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
           const xmlChar *ch, size_t len)
{
    FILE *fd = ((gen_ctx_user_t *)ctx->user_data)->fd;

    UNUSED(depth_ctx);

    fwrite(ch, len, 1, fd);
}
