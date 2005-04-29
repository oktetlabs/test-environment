/** @file 
 * @brief Test Environment: xml2html multidocument utility callbacks.
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

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>

#include "xml2gen.h"
#include "xml2html-multi.h"

/** Structure to keep basic user data in general parsing context */
typedef struct gen_ctx_user {
    FILE *js_fd; /**< File descriptor of JavaScript file */
} gen_ctx_user_t;

/** Structure to keep user data in depth-specific context */
typedef struct depth_ctx_user {
    FILE *fd; /**< File descriptor of the node currently being processed
                   on the particular depth */
    char *log_level; /**< Log level value in string representation */
} depth_ctx_user_t;


static depth_ctx_user_t *alloc_depth_user_data(uint32_t depth);
static void free_depth_user_data();

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

        if (stat("images", &stat_buf) != 0)
        {
            system("mkdir images");
        }

        snprintf(buf, sizeof(buf), "cp %s/misc/* .", INSTALL_DIR);
        system(buf);

        snprintf(buf, sizeof(buf), "cp %s/images/* images", INSTALL_DIR);
        system(buf);
    }


    /* Initialize depth-specific user data pointer */
    depth_ctx->user_data = alloc_depth_user_data(ctx->depth);
    depth_user = depth_ctx->user_data;

    if ((depth_user->fd = fopen("node_1_0.html", "w")) == NULL)
    {
        exit(1);
    }

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_fstr(attrs, "reporter", "TE start-up");
    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[DOC_START], attrs);
    rgt_tmpls_attrs_free(attrs);

    if ((gen_user->js_fd = fopen("nodes_tree.js", "w")) == NULL)
    {
        perror("nodes_tree.js");
        exit(1);
    }

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_uint32(attrs, "depth", ctx->depth);
    rgt_tmpls_attrs_add_uint32(attrs, "seq", depth_ctx->seq);
    rgt_tmpls_output(gen_user->js_fd, &xml2fmt_tmpls[JS_DOC_START], attrs);
    rgt_tmpls_attrs_free(attrs);
}

RGT_DEF_FUNC(proc_document_end)
{
    FILE *js_fd = ((gen_ctx_user_t *)ctx->user_data)->js_fd;
    FILE *fd = ((depth_ctx_user_t *)depth_ctx->user_data)->fd;

    RGT_FUNC_UNUSED_PRMS();

    fclose(js_fd);
    rgt_tmpls_output(fd, &xml2fmt_tmpls[DOC_END], NULL);
    fclose(fd);

    free_depth_user_data();
}

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
    char              fname[255];
    rgt_attrs_t      *attrs;

    assert(ctx->depth >= 2);

    depth_user = depth_ctx->user_data = alloc_depth_user_data(ctx->depth);

    if (name == NULL)
        name = "session";

    prev_depth_ctx = &g_array_index(ctx->depth_info,
                                    rgt_depth_ctx_t, (ctx->depth - 2));
    prev_depth_user = prev_depth_ctx->user_data;

    snprintf(fname, sizeof(fname), "node_%d_%d.html",
             ctx->depth, depth_ctx->seq);
    if ((depth_user->fd = fopen(fname, "w")) == NULL)
    {
        fprintf(stderr, "Cannot create %s file: %s\n", fname, strerror(errno));
        exit(1);
    }

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "reporter", "%s %s", node_type,
                             name == NULL ? "<anonimous>" : name);
    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[DOC_START], attrs);
    rgt_tmpls_attrs_free(attrs);

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "node_type", node_type);
    rgt_tmpls_attrs_add_fstr(attrs, "name", name);
    rgt_tmpls_attrs_add_fstr(attrs, "result", result);
    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[DOC_CNTRL_NODE_TITLE], attrs);
    rgt_tmpls_attrs_free(attrs);

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "node_type", node_type);
    rgt_tmpls_attrs_add_uint32(attrs, "depth", ctx->depth);
    rgt_tmpls_attrs_add_uint32(attrs, "seq", depth_ctx->seq);
    rgt_tmpls_attrs_add_fstr(attrs, "fname", fname);
    rgt_tmpls_attrs_add_fstr(attrs, "name", name);
    rgt_tmpls_attrs_add_fstr(attrs, "result", result);
    rgt_tmpls_output(prev_depth_user->fd, &xml2fmt_tmpls[DOC_REF_TO_NODE], attrs);
    rgt_tmpls_attrs_free(attrs);

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_uint32(attrs, "depth", ctx->depth);
    rgt_tmpls_attrs_add_uint32(attrs, "prev_depth", ctx->depth - 1);
    rgt_tmpls_attrs_add_uint32(attrs, "seq", depth_ctx->seq);
    rgt_tmpls_attrs_add_fstr(attrs, "fname", fname);
    rgt_tmpls_attrs_add_fstr(attrs, "name", name);
    rgt_tmpls_attrs_add_fstr(attrs, "result", result);

    if (strcmp(node_type, "Test") == 0)
        rgt_tmpls_output(gen_user->js_fd, &xml2fmt_tmpls[JS_ADD_TEST_NODE], attrs);
    else
        rgt_tmpls_output(gen_user->js_fd, &xml2fmt_tmpls[JS_ADD_FOLDER_NODE], attrs);
    rgt_tmpls_attrs_free(attrs);
}

static void
control_node_end(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
                 const char **xml_attrs, const char *node_type)
{
    FILE *fd = ((depth_ctx_user_t *)depth_ctx->user_data)->fd;

    UNUSED(ctx);
    UNUSED(xml_attrs);
    UNUSED(node_type);

    rgt_tmpls_output(fd, &xml2fmt_tmpls[DOC_END], NULL);
    fclose(fd);
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
    const char       *level = rgt_tmpls_xml_attrs_get(xml_attrs, "level");
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    assert(level != NULL);
    depth_user->log_level = strdup(level);

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOG_MSG_START], attrs);
    rgt_tmpls_attrs_free(attrs);
}

RGT_DEF_FUNC(proc_log_msg_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "level", depth_user->log_level);

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOG_MSG_END], attrs);

    rgt_tmpls_attrs_free(attrs);
    free(depth_user->log_level);
}

RGT_DEF_DUMMY_FUNC(proc_branch_start)
RGT_DEF_DUMMY_FUNC(proc_branch_end)

RGT_DEF_FUNC(proc_meta_param_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[PARAM_START], attrs);
    rgt_tmpls_attrs_free(attrs);
}

RGT_DEF_DUMMY_FUNC(proc_meta_param_end)

RGT_DEF_FUNC(proc_logs_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;
    rgt_attrs_t      *attrs;

    RGT_FUNC_UNUSED_PRMS();

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOGS_START], attrs);
    rgt_tmpls_attrs_free(attrs);
}

RGT_DEF_DUMMY_FUNC(proc_logs_end)
RGT_DEF_DUMMY_FUNC(proc_meta_start)
RGT_DEF_DUMMY_FUNC(proc_meta_end)

RGT_DEF_FUNC(proc_meta_start_ts_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_START_TS_START], NULL);
}

RGT_DEF_FUNC(proc_meta_start_ts_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_START_TS_END], NULL);
}

RGT_DEF_FUNC(proc_meta_end_ts_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_END_TS_START], NULL);
}

RGT_DEF_FUNC(proc_meta_end_ts_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_END_TS_END], NULL);
}

RGT_DEF_FUNC(proc_meta_objective_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_OBJ_START], NULL);
}

RGT_DEF_FUNC(proc_meta_objective_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_OBJ_END], NULL);
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

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "name", name);

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_AUTHORS_START], attrs);

    rgt_tmpls_attrs_free(attrs);

    free(name);
}

RGT_DEF_DUMMY_FUNC(proc_meta_author_end)

RGT_DEF_FUNC(proc_meta_authors_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_AUTHORS_START], NULL);
}

RGT_DEF_FUNC(proc_meta_authors_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_AUTHORS_END], NULL);
}

RGT_DEF_FUNC(proc_meta_params_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_PARAMS_START], NULL);
}

RGT_DEF_FUNC(proc_meta_params_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[META_PARAMS_END], NULL);
}

RGT_DEF_FUNC(proc_mem_dump_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[MEM_DUMP_START], NULL);
}

RGT_DEF_FUNC(proc_mem_dump_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[MEM_DUMP_END], NULL);
}

RGT_DEF_FUNC(proc_mem_row_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[MEM_ROW_START], NULL);
}

RGT_DEF_FUNC(proc_mem_row_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[MEM_ROW_END], NULL);
}

RGT_DEF_FUNC(proc_mem_elem_start)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[MEM_ELEM_START], NULL);
}

RGT_DEF_FUNC(proc_mem_elem_end)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[MEM_ELEM_END], NULL);
}

RGT_DEF_FUNC(proc_log_msg_br)
{
    depth_ctx_user_t *depth_user = (depth_ctx_user_t *)depth_ctx->user_data;

    RGT_FUNC_UNUSED_PRMS();

    rgt_tmpls_output(depth_user->fd, &xml2fmt_tmpls[LOG_BR], NULL);
}

void
proc_chars(rgt_gen_ctx_t *ctx, rgt_depth_ctx_t *depth_ctx,
           const xmlChar *ch, size_t len)
{
    FILE *fd = ((depth_ctx_user_t *)depth_ctx->user_data)->fd;

    UNUSED(ctx);

    fwrite(ch, len, 1, fd);
}

static GPtrArray *depth_array = NULL;

static depth_ctx_user_t *
alloc_depth_user_data(uint32_t depth)
{
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

    return g_ptr_array_index(depth_array, depth - 1);
}

static void
free_depth_user_data_part(gpointer data, gpointer user_data)
{
    UNUSED(user_data);

    g_free(data);
}
static void
free_depth_user_data()
{
    g_ptr_array_foreach(depth_array, free_depth_user_data_part, NULL);
    g_ptr_array_free(depth_array, TRUE);
    depth_array = NULL;
}
