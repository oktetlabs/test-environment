/** @file 
 * @brief Test Environment: Engine of RGT format module.
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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <libxml/parser.h>

#include <glib.h>
#include <popt.h>

#define UTILITY_NAME "xml-processor"

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

#ifndef DEBUG
#define DEBUG
#endif

#ifndef DEBUG
#define assert
#endif

typedef enum parser_state {
    RGT_XML2HTML_STATE_INITIAL,
    
    RGT_XML2HTML_STATE_BLOCK,
    RGT_XML2HTML_STATE_LOGS,

    RGT_XML2HTML_STATE_META,
    
    RGT_XML2HTML_STATE_START_TS,
    RGT_XML2HTML_STATE_END_TS,
    RGT_XML2HTML_STATE_AUTHORS,
    RGT_XML2HTML_STATE_AUTHOR,
    RGT_XML2HTML_STATE_OBJECTIVE,
    RGT_XML2HTML_STATE_PARAMS,

    RGT_XML2HTML_STATE_CNTRL_MSG,
    RGT_XML2HTML_STATE_LOG_MSG,
    RGT_XML2HTML_STATE_MEM_DUMP,
    RGT_XML2HTML_STATE_MEM_DUMP_ROW,
    RGT_XML2HTML_STATE_MEM_DUMP_ELEM,
    RGT_XML2HTML_STATE_FILE,
} parser_state_t;

enum e_element_type {
    XML_ELEMENT_START,
    XML_ELEMENT_END
};


/** Name of the XML report file */
static const char *xml_file_name = NULL;

/* Output file name passed as an argument */
static const char *out_file_name = NULL;

static const char *aux_path = NULL;

/** Possible node types */
typedef enum node_type {
    NT_SESSION, /**< Node of session type */
    NT_PACKAGE, /**< Node of package type */
    NT_TEST,    /**< Node of test type */
    NT_BRANCH,  /**< It is used only for generation events 
                     "branch start" / "branch end" */
    NT_LAST     /**< Last marker - the biggest value of the all evements */
} node_type_t;

typedef struct depth_context {
    int   seq; /**< Current sequence number used on the particular depth */
    node_type_t type; /**< Current node type */

    /* @todo User DATA */
    FILE *fd; /**< File descriptor of the node currently being processed
                   on the particular depth */
    char *level; /**< Log level */
} depth_context_t;

/** 
 * The structure represents a global context that is used in all handlers
 * to determine the current state, values some element specific variables
 */
typedef struct global_context {
    int             depth; /**< The current processing depth 
                                in the node tree */
    GArray         *depth_info; /**< Array of information about 
                                     the particular depth */
    parser_state_t  state;

    /* @todo User DATA */
    FILE           *js_fd; /**< File descriptor of JavaScript file */
} global_context_t;

typedef struct mem_ctx_s {
    int mem_width; /**< Number of elements in a memory row.
                        (Used only for mem-dump element processing) */
    int cur_num; /**< Current number of elements in memory row */
    int first_row; /* @todo MAke it te_bool */
} mem_ctx_t;

static mem_ctx_t mem_dump_ctx;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static struct global_context global_ctx;


#define DEF_DEPTH_CTX \
    depth_context_t *depth_ctx =                           \
        &g_array_index(ctx->depth_info,                    \
                       depth_context_t, (ctx->depth - 1))

static const xmlChar *get_attr_value(const xmlChar **atts, const xmlChar *name)
{
    if (atts == NULL)
        return NULL;

    while (*atts != NULL)
    {
        if (strcmp(name, *atts) == 0)
        {
            assert(*(atts + 1) != NULL);
            return *(atts + 1);
        }

        atts += 2;
    }

    return NULL;
} 

static void
write_document_header(FILE *fd, const char *obj_name)
{
    fprintf(fd,
"<html>\n"
"<head>\n"
"<title>Logs from %s</title>\n"
"<link rel='stylesheet' type='text/css' href='style.css'>\n"
"<script>\n"
"var hide = new Array();\n"
"\n"
"function getAllLogs()\n"
"{\n"
"    if (document.getElementsByTagName)\n"
"    {\n"
"        return document.getElementsByTagName(\"div\");\n"
"    }\n"
"    else if (document.getElementsById)\n"
"    {\n"
"        alert(\"Extend sources with getElementsById\");\n"
"    }\n"
"    else\n"
"    {\n"
"        alert(\"Not supported\");\n"
"    }\n"
"}\n"
"\n"
"function open_filter_window()\n"
"{\n"
"    window.open(\"filter_conf_local.html\", \"parwin\", \"dependent,toolbar=no,menubar=no,status=no,width=400,height=300,screenX=300,screenY=100,scrollbars=no\");\n"
"}\n"
"\n"
"function activate_link(name, doc_name)\n"
"{\n"
"    parent.treeframe.activate_node(name);\n"
"    document.location.href=doc_name;\n"
"}\n"
"\n"
"</script>\n"
"\n"
"</head>\n"
"\n"
"<body>\n"

"<table border='0' cellpadding='0' cellspacing='0' width='100%%'>\n"
"<tr class='tdheading'>\n"
"    <td width='100%%' class='heading' height='25'>&nbsp;&nbsp;&nbsp;Logs from %s</td>\n"
"</table>\n"
"<br>\n"
"\n"
"<form name='form0'>\n"
"    <input type='button' value='Log Filter' onClick='javascript:open_filter_window()'>\n"
"</form>\n"
"<br/>",
obj_name, obj_name);

}

static void
write_document_footer(FILE *fd)
{
    fprintf(fd, "</body></html>\n");
}

static void
proc_document_start(struct global_context *ctx, const xmlChar **atts)
{
    depth_context_t *depth_ctx;

    UNUSED(atts);

    assert(global_ctx.depth >= 1);
    depth_ctx = &g_array_index(ctx->depth_info,
                               depth_context_t, (global_ctx.depth - 1));
    if ((depth_ctx->fd = fopen("node_0_0.html", "w")) == NULL)
    {
        exit(1);
    }
    write_document_header(depth_ctx->fd, "TE start-up");

    if ((ctx->js_fd = fopen("oleg.js", "w")) == NULL)
    {
        exit(2);
    }
    fprintf(ctx->js_fd,
            "ICONPATH='images/'\n"
            "USETEXTLINKS = 1\n"
            "HIGHLIGHT=1\n\n"
            "a%d = gFld('<i>Test results</i>', 'node_0_0.html')\n"
            "a%d.xID = 'n_0_0'\n",
            ctx->depth, ctx->depth);
}
static void
proc_document_end(struct global_context *ctx, const xmlChar **atts)
{
    depth_context_t *depth_ctx;

    UNUSED(atts);

    assert(global_ctx.depth >= 1);

    fclose(ctx->js_fd);

    depth_ctx = &g_array_index(global_ctx.depth_info,
                               depth_context_t, (global_ctx.depth - 1));
    write_document_footer(depth_ctx->fd);
    fclose(depth_ctx->fd);
}

static void
control_node_start(struct global_context *ctx, const xmlChar **atts,
                   const char *node_type, const char *tree_func_name /* make it user data (void *)*/)
{
    static char      obj_name[255];
    depth_context_t *prev_depth_ctx;
    const xmlChar   *name = get_attr_value(atts, "name");
    const xmlChar   *result = get_attr_value(atts, "result");
    char             fname[255];
    depth_context_t *depth_ctx;

    assert(global_ctx.depth >= 2);

    snprintf(obj_name, sizeof(obj_name), "%s %s", node_type,
             ((name != NULL) ? (const char *)name : "<anonimous>"));

    if (name == NULL)
        name = "session";

    prev_depth_ctx = &g_array_index(ctx->depth_info,
                                    depth_context_t, (ctx->depth - 2));

    depth_ctx = &g_array_index(ctx->depth_info,
                               depth_context_t, (ctx->depth - 1));
    
    snprintf(fname, sizeof(fname), "node_%d_%d.html",
             ctx->depth, depth_ctx->seq);
    if ((depth_ctx->fd = fopen(fname, "w")) == NULL)
    {
        fprintf(stderr, "Cannot create %s file: %s\n", fname, strerror(errno));
        exit(1);
    }
    write_document_header(depth_ctx->fd, obj_name);
    
#if 0
    fprintf(stderr, "File %s (%p) opened in array element %d\n",
            fname, depth_ctx->fd, (ctx->depth - 1));
#endif
    
    fprintf(prev_depth_ctx->fd,
            "<i><b>%s</b></i>: "
            "<b><a href='javascript:activate_link(\"n_%d_%d\", \"%s\")'>%s</a>"
            "</b> -- %s<br/>\n",
            node_type, ctx->depth, depth_ctx->seq,
            fname, name, result);

    fprintf(depth_ctx->fd,
            "<i><b>%s</b></i>: <b>%s</b><br/>\n",
            node_type, name);

    fprintf(ctx->js_fd,
            "a%d = insDoc(a%d, g%s(%s'%s', '%s'))\n"
            "a%d.xID = 'n_%d_%d'\n",
            
            ctx->depth, ctx->depth - 1, tree_func_name,
            strcmp(node_type, "Test") == 0 ? "'R', " : "",
            name, fname,
            ctx->depth, ctx->depth, depth_ctx->seq);
}

static void
control_node_end(struct global_context *ctx, const xmlChar **atts,
                 const char *node_type)
{
    depth_context_t *depth_ctx;

    UNUSED(atts);
    UNUSED(node_type);

    assert(ctx->depth >= 1);
    depth_ctx = &g_array_index(ctx->depth_info,
                               depth_context_t, (ctx->depth - 1));
    write_document_footer(depth_ctx->fd);

    fclose(depth_ctx->fd);
}

static void
proc_session_start(struct global_context *ctx, const xmlChar **atts)
{
    control_node_start(ctx, atts, "Session", "Fld");
}
static void
proc_session_end(struct global_context *ctx, const xmlChar **atts)
{
    control_node_end(ctx, atts, "Session");
}
static void
proc_pkg_start(struct global_context *ctx, const xmlChar **atts)
{
    control_node_start(ctx, atts, "Package", "Fld");
}
static void
proc_pkg_end(struct global_context *ctx, const xmlChar **atts)
{
    control_node_end(ctx, atts, "Package");
}
static void
proc_test_start(struct global_context *ctx, const xmlChar **atts)
{
    control_node_start(ctx, atts, "Test", "Lnk");
}
static void
proc_test_end(struct global_context *ctx, const xmlChar **atts)
{
    control_node_end(ctx, atts, "Test");
}

static void
proc_log_msg_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    const char *level = get_attr_value(atts, "level");
    const char *entity = get_attr_value(atts, "entity");
    const char *user = get_attr_value(atts, "user");
    const char *ts = get_attr_value(atts, "ts");
    
    depth_ctx->level = strdup(level);
    assert(depth_ctx->level != NULL);
    
    fprintf(depth_ctx->fd, 
"<div level='%s' entityname='%s' username='%s'>\n"
"  <table border='0' cellpadding='0' cellspacing='0' width='100%%'>\n"
"  <tr>\n"
"    <td class='%s'>\n"
"      <table border='0' cellpadding='0' cellspacing='0'>\n"
"      <tr>\n"
"        <td width='50' class='level'>%s</td>\n"
"        <td width='100' class='entityname'>%s</td>\n"
"        <td width='100' class='username'>%s</td>\n"
"        <td width='150' class='timestamp'>%s</td>\n"
"        <td class='log'>\n",
           level, entity, user,
           level,
           level, entity, user, ts);
}

static void
proc_log_msg_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, 
"        </td>\n"
"      </tr>\n"
"      </table>\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class='%s'>&nbsp;</td>\n"
"  </tr>\n"
"  </table>\n"
"</div>\n", depth_ctx->level);
    
    free(depth_ctx->level);
}

static void
proc_branch_start(struct global_context *ctx, const xmlChar **atts)
{
    depth_context_t *depth_ctx;

    UNUSED(atts);

    depth_ctx = &g_array_index(ctx->depth_info,
                               depth_context_t, (ctx->depth - 1));

    assert(depth_ctx->type != NT_TEST);
}

static void
proc_branch_end(struct global_context *ctx, const xmlChar **atts)
{
    UNUSED(ctx);
    UNUSED(atts);
}

static void
proc_meta_param_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    const xmlChar   *name = get_attr_value(atts, "name");
    const xmlChar   *value = get_attr_value(atts, "value");

    assert(name != NULL && value != NULL);

    fprintf(depth_ctx->fd, "<tr><td>%s</td><td>%s</td></tr>\n",
            name, value);
}

static void
proc_meta_param_end(struct global_context *ctx, const xmlChar **atts)
{
    UNUSED(ctx);
    UNUSED(atts);
    /* Do nothing */
}

static void
proc_logs_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, 
"<table border='0' cellpadding='0' cellspacing='0' width='100%%'>\n"
"<tr>\n"
"    <td class='tdsubheading'>\n"
"        <table border='0' cellpadding='0' cellspacing='0'>\n"
"        <tr>\n"
"            <td width='50' class='tdsubhead'>Level</td>\n"
"            <td width='100' class='tdsubhead'>Entity Name</td>\n"
"            <td width='100' class='tdsubhead>User Name</td>\n"
"            <td width='150' class='tdsubhead'>Timestamp</td>\n"
"            <td class='tdsubhead'>Log content</td>\n"
"        </tr>\n"
"        </table>\n"
"    </td>\n"
"</tr>\n"
"</table>\n"
"<br>\n");

}

static void
proc_logs_end(struct global_context *ctx, const xmlChar **atts)
{
    UNUSED(ctx);
    UNUSED(atts);
}

static void
proc_meta_start(struct global_context *ctx, const xmlChar **atts)
{
    const xmlChar *nbranches = get_attr_value(atts, "nbranches");
    UNUSED(ctx);
    
    if (nbranches != NULL)
    {
        /* Do smth. */
    }
}

static void
proc_meta_end(struct global_context *ctx, const xmlChar **atts)
{
    UNUSED(ctx);
    UNUSED(atts);
}

static void
proc_meta_start_ts_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<b>start time</b>:");
}

static void
proc_meta_start_ts_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<br/>\n");
}

static void
proc_meta_end_ts_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<b>end time</b>:");
}

static void
proc_meta_end_ts_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<br/>\n");
}

static void
proc_meta_objective_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<b>objective</b>:");
}

static void
proc_meta_objective_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<br/>\n");
}

static void
proc_meta_author_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    const xmlChar *email = get_attr_value(atts, "email");
    char          *name = strdup(email);
    char          *ptr;

    ptr = strchr(name, '@');
    if (ptr == NULL)
        assert(0);
    
    *ptr = '\0';

    fprintf(depth_ctx->fd, "<tr><td><b>%s</b></td>"
                           "<td><a href=\"mailto:%s\">%s</a></td></tr>",
            name, email, email);

    free(name);
}

static void
proc_meta_author_end(struct global_context *ctx, const xmlChar **atts)
{
    UNUSED(ctx);
    UNUSED(atts);
}

static void
proc_meta_authors_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<table><tr><th>Authors</th><th>E-mail</th></tr>");
}

static void
proc_meta_authors_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);
    
    fprintf(depth_ctx->fd, "</table>\n");
}

static void
proc_meta_params_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<table>\n");
}

static void
proc_meta_params_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "</table>\n");
}

static void
proc_mem_dump_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);
    
    mem_dump_ctx.first_row = TRUE;
    mem_dump_ctx.mem_width = 0;

    fprintf(depth_ctx->fd,
            "<table border='0' cellspacing='2' cellpadding='0'>\n");
}

static void
proc_mem_dump_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);
    fprintf(depth_ctx->fd, "</table>\n");
}

static void
proc_mem_row_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);
    
    mem_dump_ctx.cur_num = 0;

    fprintf(depth_ctx->fd, "<tr>");
}

static void
proc_mem_row_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);
    
    mem_dump_ctx.first_row = FALSE;

    fprintf(depth_ctx->fd, "</tr>\n");
}

static void
proc_mem_elem_start(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);
    fprintf(depth_ctx->fd,
            "<td class='mem_dump'>");
}

static void
proc_mem_elem_end(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);
    
    if (mem_dump_ctx.first_row)
    {
        mem_dump_ctx.mem_width++;
    }
    mem_dump_ctx.cur_num++;

    fprintf(depth_ctx->fd, "</td>");
}

static void
proc_log_msg_br(struct global_context *ctx, const xmlChar **atts)
{
    DEF_DEPTH_CTX;
    UNUSED(atts);

    fprintf(depth_ctx->fd, "<br/>");
}

static void
proc_chars(struct global_context *ctx, const xmlChar *ch, size_t len)
{
    DEF_DEPTH_CTX;

    fwrite(ch, len, 1, depth_ctx->fd);
}


/**
 * Callback function that is called before parsing the document.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 *
 * @return Nothing
 */
static void
rgt_log_start_document(void *user_data)
{
    struct global_context *ctx = (struct global_context *)user_data;
    depth_context_t        new_depth_ctx;
    depth_context_t       *depth_ctx;

    ctx->depth++;

    memset(&new_depth_ctx, 0, sizeof(new_depth_ctx));

    /* Create root element in the array */
    g_array_append_val(ctx->depth_info, new_depth_ctx);
    depth_ctx = &g_array_index(ctx->depth_info,
                               depth_context_t, (ctx->depth - 1));
    depth_ctx->seq++;

    proc_document_start(ctx, NULL);
}

/**
 * Callback function that is called when XML parser reaches the end 
 * of the document.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 *
 * @return Nothing
 */
static void
rgt_log_end_document(void *user_data)
{
    struct global_context *ctx = (struct global_context *)user_data;

    proc_document_end(ctx, NULL);
    ctx->depth--;
}

static void
rgt_process_cntrl_start(struct global_context *ctx,
                        const xmlChar *tag, const xmlChar **atts)
{
    depth_context_t *depth_ctx;
#ifdef DEBUG
    const xmlChar   *name = get_attr_value(atts, "name");
    const xmlChar   *result = get_attr_value(atts, "result");
#endif /* DEBUG */

    assert(result != NULL);
    assert(ctx->depth >= 1);

    ctx->depth++;

    if ((int)ctx->depth_info->len < ctx->depth)
    {
        depth_context_t new_depth_ctx;

        memset(&new_depth_ctx, 0, sizeof(new_depth_ctx));

        /* Create a new element in the array */
        g_array_append_val(ctx->depth_info, new_depth_ctx);
    }
    depth_ctx = &g_array_index(ctx->depth_info,
                               depth_context_t, (ctx->depth - 1));

    if (strcmp(tag, "test") == 0)
    {
        assert(name != NULL);

        depth_ctx->type = NT_TEST;
        proc_test_start(ctx, atts);
    }
    else if (strcmp(tag, "pkg") == 0)
    {
        assert(name != NULL);

        depth_ctx->type = NT_PACKAGE;
        proc_pkg_start(ctx, atts);
    }
    else if (strcmp(tag, "session") == 0)
    {
        depth_ctx->type = NT_SESSION;
        proc_session_start(ctx, atts);
    }
    else
    {
        assert(0);
    }
    depth_ctx->seq++;
}

/**
 * Callback function that is called when XML parser meets the end of 
 * an element.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  name       The element name
 *
 * @return Nothing
 */
static void
rgt_log_end_element(void *user_data, const xmlChar *tag)
{
    global_context_t *ctx = (struct global_context *)user_data;

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_LOGS:
            assert(strcmp(tag, "logs") == 0);

            proc_logs_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_BLOCK;
            break;

        case RGT_XML2HTML_STATE_LOG_MSG:
            assert(ctx->depth >= 1);
            if (strcmp(tag, "msg") == 0)
            {
                proc_log_msg_end(ctx, NULL);
                ctx->state = RGT_XML2HTML_STATE_LOGS;
            }
            else
            {
                assert(strcmp(tag, "br") == 0);
                /* Do nothing */
            }
            break;

        case RGT_XML2HTML_STATE_MEM_DUMP:
            assert(strcmp(tag, "mem-dump") == 0);
            proc_mem_dump_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            break;
        
        case RGT_XML2HTML_STATE_MEM_DUMP_ROW:
            assert(strcmp(tag, "row") == 0);
            proc_mem_row_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP;
            break;

        case RGT_XML2HTML_STATE_MEM_DUMP_ELEM:
            assert(strcmp(tag, "elem") == 0);
            proc_mem_elem_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ROW;
            break;

        case RGT_XML2HTML_STATE_BLOCK:
        {
            void (*cb_func)(struct global_context *ctx, const xmlChar **atts);

            if ((cb_func = proc_test_end, strcmp(tag, "test") == 0) ||
                (cb_func = proc_pkg_end, strcmp(tag, "pkg") == 0) ||
                (cb_func = proc_session_end, strcmp(tag, "session") == 0))
            {
                cb_func(ctx, NULL);
                ctx->depth--;
            }
            else if (strcmp(tag, "branch") == 0)
            {
                proc_branch_end(ctx, NULL);
            }
            else if (strcmp(tag, "proteos:log_report") == 0)
            {
                /* Do nothing - end of file */
                ctx->state = RGT_XML2HTML_STATE_INITIAL;
            }
            else
            {
                fprintf(stderr, "Unexpected closing TAG %s in "
                        "BLOCK state\n", tag);
                exit(0);
            }

            break;
        }

        case RGT_XML2HTML_STATE_META:
            assert(strcmp(tag, "meta") == 0);
            assert(ctx->depth >= 1);

            proc_meta_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_BLOCK;
            break;
        
        case RGT_XML2HTML_STATE_START_TS:
            assert(ctx->depth >= 1);
            proc_meta_start_ts_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_END_TS:
            assert(ctx->depth >= 1);
            proc_meta_end_ts_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_OBJECTIVE:
            assert(strcmp(tag, "objective") == 0);
            assert(ctx->depth >= 1);
            proc_meta_objective_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_AUTHORS:
            assert(strcmp(tag, "authors") == 0);
            assert(ctx->depth >= 1);
            proc_meta_authors_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_AUTHOR:
            assert(strcmp(tag, "author") == 0);
            proc_meta_author_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_AUTHORS;
            break;

        case RGT_XML2HTML_STATE_PARAMS:
            assert(ctx->depth >= 1);

            if (strcmp(tag, "params") != 0)
            {
                assert(strcmp(tag, "param") == 0);
                proc_meta_param_end(ctx, NULL);
                break;
            }

            proc_meta_params_end(ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;
            
        case RGT_XML2HTML_STATE_FILE:
            if (strcmp(tag, "file") == 0)
            {
                ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            }
            else
                assert(strcmp(tag, "br") == 0);
            break;

        default:
            assert(0);
    }
}

/**
 * Callback function that is called when XML parser meets an opening tag.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  name       The element name
 * @param  atts       An array of attribute name, attribute value pairs
 *
 * @return Nothing
 */
static void
rgt_log_start_element(void *user_data,
                      const xmlChar *tag, const xmlChar **atts)
{
    struct global_context *ctx = (struct global_context *)user_data;

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_INITIAL:
            assert(strcmp(tag, "proteos:log_report") == 0);
            global_ctx.state = RGT_XML2HTML_STATE_BLOCK;
            break;

        case RGT_XML2HTML_STATE_BLOCK:
            if (strcmp(tag, "meta") == 0)
            {
                proc_meta_start(ctx, atts);
                ctx->state = RGT_XML2HTML_STATE_META;
                break;
            }
            else if (strcmp(tag, "logs") == 0)
            {
                assert(ctx->depth >= 1);
                proc_logs_start(ctx, atts);
                ctx->state = RGT_XML2HTML_STATE_LOGS;
                break;
            }
            else if (strcmp(tag, "branch") == 0)
            {
                assert(ctx->depth >= 1);
                proc_branch_start(ctx, atts);
                break;
            }

            /* Control message */
            rgt_process_cntrl_start(ctx, tag, atts);
            break;
        
        case RGT_XML2HTML_STATE_META:
            assert(ctx->depth >= 1);

            if (strcmp(tag, "start-ts") == 0)
            {
                proc_meta_start_ts_start(ctx, atts);
                ctx->state = RGT_XML2HTML_STATE_START_TS;
            }
            else if (strcmp(tag, "end-ts") == 0)
            {
                proc_meta_end_ts_start(ctx, atts);
                ctx->state = RGT_XML2HTML_STATE_END_TS;
            }
            else if (strcmp(tag, "objective") == 0)
            {
                proc_meta_objective_start(ctx, atts);
                ctx->state = RGT_XML2HTML_STATE_OBJECTIVE;
            }
            else if (strcmp(tag, "authors") == 0)
            {
                proc_meta_authors_start(ctx, atts);
                ctx->state = RGT_XML2HTML_STATE_AUTHORS;
            }
            else if (strcmp(tag, "params") == 0)
            {
                proc_meta_params_start(ctx, atts);
                ctx->state = RGT_XML2HTML_STATE_PARAMS;
            }
            else
            {
                fprintf(stderr, "Unexpected TAG '%s' in META state\n", tag);
                assert(0);
            }

            break;
            
        case RGT_XML2HTML_STATE_AUTHORS:
            assert(strcmp(tag, "author") == 0);
            proc_meta_author_start(ctx, atts);
            ctx->state = RGT_XML2HTML_STATE_AUTHOR;
            break;

        case RGT_XML2HTML_STATE_PARAMS:
            assert(strcmp(tag, "param") == 0);
            proc_meta_param_start(ctx, atts);
            break;

        case RGT_XML2HTML_STATE_LOGS:
            assert(strcmp(tag, "msg") == 0);

            proc_log_msg_start(ctx, atts);
            ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            break;

        case RGT_XML2HTML_STATE_LOG_MSG:
            if (strcmp(tag, "br") == 0)
            {
                proc_log_msg_br(ctx, atts);
            }
            else if (strcmp(tag, "mem-dump") == 0)
            {
                proc_mem_dump_start(ctx, atts);
                ctx->state = RGT_XML2HTML_STATE_MEM_DUMP;
            }
            else if (strcmp(tag, "file") == 0)
            {
                ctx->state = RGT_XML2HTML_STATE_FILE;
            }
            else
                assert(0);

            break;

        case RGT_XML2HTML_STATE_MEM_DUMP:
            assert(strcmp(tag, "row") == 0);
            proc_mem_row_start(ctx, atts);
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ROW;
            break;

        case RGT_XML2HTML_STATE_MEM_DUMP_ROW:
            assert(strcmp(tag, "elem") == 0);
            proc_mem_elem_start(ctx, atts);
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ELEM;
            break;
            
        case RGT_XML2HTML_STATE_FILE:
            assert(strcmp(tag, "br") == 0);
            break;

        default:
            break;
    }
}

/**
 * Callback function that is called when XML parser meets character data.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  ch         Pointer to the string
 * @param  len        Number of the characters in the string
 *
 * @return Nothing
 */
static void
rgt_log_characters(void *user_data, const xmlChar *ch, int len)
{
    global_context_t *ctx = (global_context_t *)user_data;

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_START_TS:
        case RGT_XML2HTML_STATE_END_TS:
        case RGT_XML2HTML_STATE_OBJECTIVE:
        case RGT_XML2HTML_STATE_LOG_MSG:
        case RGT_XML2HTML_STATE_MEM_DUMP_ELEM:
        case RGT_XML2HTML_STATE_FILE:
        {
            assert(ctx->depth >= 1);
            proc_chars(ctx, ch, len);
            break;
        }
        default:
            break;
    }
}

/** The structure specifies all types callback routines that should be called */
static xmlSAXHandler sax_handler = {
    .internalSubset         = NULL,
    .isStandalone           = NULL,
    .hasInternalSubset      = NULL,
    .hasExternalSubset      = NULL,
    .resolveEntity          = NULL,
    .getEntity              = NULL,
    .entityDecl             = NULL,
    .notationDecl           = NULL,
    .attributeDecl          = NULL, /* attributeDeclDebug, */
    .elementDecl            = NULL, /* elementDeclDebug, */
    .unparsedEntityDecl     = NULL, /* unparsedEntityDeclDebug, */
    .setDocumentLocator     = NULL,
    .startDocument          = rgt_log_start_document,
    .endDocument            = rgt_log_end_document,
    .startElement           = rgt_log_start_element,
    .endElement             = rgt_log_end_element,
    .reference              = NULL, /* referenceDebug, */
    .characters             = rgt_log_characters,
    .ignorableWhitespace    = NULL, /* ignorableWhitespaceDebug, */
    .processingInstruction  = NULL, /* processingInstructionDebug, */
    .comment                = NULL, /* commentDebug, */
    .warning                = NULL, /* warningDebug, */
    .error                  = NULL, /* errorDebug, */
    .fatalError             = NULL, /* fatalErrorDebug, */
    .getParameterEntity     = NULL, /* getParameterEntityDebug, */
    .cdataBlock             = NULL, /* cdataBlockDebug, */
    .externalSubset         = NULL, /* externalSubsetDebug, */
    .initialized            = 1,
    /* The following fields are extensions available only on version 2 */
#if HAVE___STRUCT__XMLSAXHANDLER__PRIVATE
    ._private               = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_STARTELEMENTNS
    .startElementNs         = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_ENDELEMENTNS
    .endElementNs           = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_SERROR___
    .serror                 = NULL
#endif
};

/**
 * Print "usage" how to.
 *
 * @param  optCon    Context for parsing command line arguments.
 * @param  exitcode  Code that is passed to the "exit" call.
 * @param  error     Error message string.
 * @param  addl      Additional notes that is output.
 *
 * @return  Nothing.
 *
 * @se  Frees popt Context (specified in optCon) and exits with specified code.
 */
static void
usage(poptContext optCon, int exitcode, char *error, char *addl)
{
    poptSetOtherOptionHelp(optCon, 
        "[<xml report file>] [<output file>]");
    poptPrintUsage(optCon, stderr, 0);
    if (error)
    {
        fprintf(stderr, "%s", error);
        if (addl != NULL)
            fprintf(stderr, ": %s", addl);
        fprintf(stderr, "\n");
    }

    poptFreeContext(optCon);

    exit(exitcode);
}

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated if some new
 * options are going to be added.
 *
 * @param  argc   Number of elements in array "argv".
 * @param  argv   Array of strings that represents all command line arguments.
 *
 * @return  Nothing.
 *
 * @se
 *    The function updates the following global variables according to 
 *    the command line arguments specified in argv:
 *      html_tmpls    - Array of template file names
 *      xml_file_name - XML report file name
 *      out_file_name - Output file name (if out_fd is not stdout)
 *      out_fd        - Output file descriptor
 *
 *    In the case of an error it calls exit() function with code 1.
 */
static void
process_cmd_line_opts(int argc, char **argv)
{
    const char  *opt_out_file_name = NULL; /* Output file name passed
                                              with -o option */
    poptContext  optCon; /* Context for parsing command-line options */
    int          rc;
    
    /* Option Table */
    struct poptOption optionsTable[] = {
        { "xml-report-file", 'f', POPT_ARG_STRING, NULL, 'f', 
          "XML report file name.", "FILE" },

        { "output", 'o', POPT_ARG_STRING, NULL, 'o', 
          "Output file name.", "FILE" },

        { "auxpath", 'd', POPT_ARG_STRING, NULL, 'd', 
          "Aux dir.", "FILE" },
          
        { "version", 'v', POPT_ARG_NONE, NULL, 'v', 
          "Display version information.", NULL },

        POPT_AUTOHELP

        { NULL, 0, 0, NULL, 0, NULL, NULL },
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv, optionsTable, 0);

    poptSetOtherOptionHelp(optCon,
                           "[OPTIONS...] [<xml report file>] [<output file>]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 'f')
        {
            if ((xml_file_name = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify XML report file", NULL);
            }
        }
        else if (rc == 'd')
        {
            if ((aux_path = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify Aux path", NULL);
            }
        }
        else if (rc == 'o')
        {
            if ((opt_out_file_name = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify Output file name", NULL);
            }
        }
        else if (rc == 'v')
        {
            printf("Package %s: %s version %s\n%s\n", 
                   PACKAGE, UTILITY_NAME, VERSION, TE_COPYRIGHT);
            poptFreeContext(optCon);
            exit(0);
        }
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        fprintf(stderr, "%s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        exit(1);
    }

    if (xml_file_name == NULL && (xml_file_name = poptGetArg(optCon)) == NULL)
    {
        usage(optCon, 1, "Specify XML report file", NULL);
    }

    /* Prepend xml file with ".." */
    {
        char *tmp = (char *)malloc(strlen(xml_file_name) + 4);
        
        if (tmp == NULL)
            exit(3);
        
        snprintf(tmp, strlen(xml_file_name) + 4, "../%s", xml_file_name);
        xml_file_name = tmp;
    }

    /* Get output file name */
    out_file_name = poptGetArg(optCon);
    
    if (out_file_name != NULL && opt_out_file_name != NULL)
    {
        usage(optCon, 1, "Output file name specified twice: "
              "with -f option and as a command line argument", NULL);
    }

    if (out_file_name != NULL)
        opt_out_file_name = out_file_name;

    if (opt_out_file_name == NULL)
        opt_out_file_name = "html";

    {
        int         rc;
        struct stat stat_buf;
        
        rc = stat(opt_out_file_name, &stat_buf);
        if (rc == -1)
        {
            if (errno != ENOENT)
            {
                perror(opt_out_file_name);                
                poptFreeContext(optCon);
                exit(1);
            }
            if (mkdir(opt_out_file_name, 0777) < 0)
            {
                perror(opt_out_file_name);
                poptFreeContext(optCon);
                exit(1);
            }
        }
        if (stat(opt_out_file_name, &stat_buf) != 0)
        {
            perror(opt_out_file_name);
            poptFreeContext(optCon);
            exit(1);
        }
        if (!S_ISDIR(stat_buf.st_mode))
        {
            fprintf(stderr, "File %s already exists and "
                    "it is not a directory", opt_out_file_name);
            poptFreeContext(optCon);
            exit(1);
        }

        if (chdir(opt_out_file_name) < 0)
        {
            perror(opt_out_file_name);
            poptFreeContext(optCon);
            exit(1);
        }

        if (stat("images", &stat_buf) != 0)
        {
            system("mkdir images");
        }
    }

    if (aux_path == NULL)
    {
        fprintf(stderr, "-d option is mandatory");
        exit(3);
    }
    
    /* Copy all the aux files */
    {
        char buf[1024];

        snprintf(buf, sizeof(buf), "cp %s/misc/* .", aux_path);
        system(buf);
        
        snprintf(buf, sizeof(buf), "cp %s/pics/* images", aux_path);
        system(buf);

        snprintf(buf, sizeof(buf), "cp %s/style/* .", aux_path);
        system(buf);
    }

    out_file_name = opt_out_file_name;

    if (poptPeekArg(optCon) != NULL)
    {
        usage(optCon, 1, "Too many parameters specified", NULL);
    }

    poptFreeContext(optCon);
}

int
main(int argc, char **argv)
{
    int              rc = 0;

    process_cmd_line_opts(argc, argv);

    global_ctx.state = RGT_XML2HTML_STATE_INITIAL;
    global_ctx.depth = 0;
    global_ctx.depth_info = g_array_new(FALSE, FALSE, sizeof(depth_context_t));
    if (global_ctx.depth_info == NULL)
    {
        fprintf(stderr, "Cannot allocate resourses for the programm\n");
        exit(1);
    }
    
    if (xmlSAXUserParseFile(&sax_handler, &global_ctx, xml_file_name) != 0)
    {
        fprintf(stderr, "Cannot parse XML document\n");
        rc = 1;
    }
    assert(global_ctx.depth == 0);

    g_array_free(global_ctx.depth_info, TRUE);
    xmlCleanupParser();

    return rc;
}
