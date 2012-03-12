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

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <popt.h>

#include "xml2gen.h"

#define UTILITY_NAME "xml-processor"

/* Max attribute length in one line */
extern int rgt_max_attribute_length;
/* A tag to separate lines */
extern const char *rgt_line_separator;

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
    rgt_gen_ctx_t   *ctx = (rgt_gen_ctx_t *)user_data;
    rgt_depth_ctx_t  new_depth_ctx;
    rgt_depth_ctx_t *depth_ctx;

    ctx->depth++;

    memset(&new_depth_ctx, 0, sizeof(new_depth_ctx));

    /* Create root element in the array */
    g_array_append_val(ctx->depth_info, new_depth_ctx);
    depth_ctx = &g_array_index(ctx->depth_info,
                               rgt_depth_ctx_t, (ctx->depth - 1));

    proc_document_start(ctx, depth_ctx, NULL);
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
    rgt_gen_ctx_t   *ctx = (rgt_gen_ctx_t *)user_data;
    rgt_depth_ctx_t *depth_ctx = 
        &g_array_index(ctx->depth_info,
                       rgt_depth_ctx_t, (ctx->depth - 1));

    proc_document_end(ctx, depth_ctx, NULL);
    ctx->depth--;
}

static void
rgt_process_cntrl_start(rgt_gen_ctx_t *ctx,
                        const char *tag, const char **attrs)
{
    rgt_depth_ctx_t *depth_ctx;

    assert(ctx->depth >= 1);

    ctx->depth++;

    if (ctx->depth_info->len < ctx->depth)
    {
        rgt_depth_ctx_t new_depth_ctx;

        memset(&new_depth_ctx, 0, sizeof(new_depth_ctx));

        /* Create a new element in the array */
        g_array_append_val(ctx->depth_info, new_depth_ctx);
    }
    depth_ctx = &g_array_index(ctx->depth_info,
                               rgt_depth_ctx_t, (ctx->depth - 1));

    if (strcmp(tag, "test") == 0)
    {
        depth_ctx->type = NT_TEST;
        proc_test_start(ctx, depth_ctx, attrs);
    }
    else if (strcmp(tag, "pkg") == 0)
    {
        depth_ctx->type = NT_PACKAGE;
        proc_pkg_start(ctx, depth_ctx, attrs);
    }
    else if (strcmp(tag, "session") == 0)
    {
        depth_ctx->type = NT_SESSION;
        proc_session_start(ctx, depth_ctx, attrs);
    }
    else
    {
        assert(0);
    }
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
rgt_log_end_element(void *user_data, const rgt_xmlChar *xml_tag)
{
    rgt_gen_ctx_t   *ctx = (rgt_gen_ctx_t *)user_data;
    rgt_depth_ctx_t *depth_ctx;
    const char      *tag = (const char *)xml_tag;

    depth_ctx = &g_array_index(ctx->depth_info,
                               rgt_depth_ctx_t, (ctx->depth - 1));

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_LOGS:
            assert(strcmp(tag, "logs") == 0);

            proc_logs_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_BLOCK;
            break;

        case RGT_XML2HTML_STATE_LOG_MSG:
            assert(ctx->depth >= 1);
            if (strcmp(tag, "msg") == 0)
            {
                proc_log_msg_end(ctx, depth_ctx, NULL);
                ctx->state = RGT_XML2HTML_STATE_LOGS;
            }
            else
            {
                assert(strcmp(tag, "br") == 0);
                /* Do nothing */
            }
            break;

        case RGT_XML2HTML_STATE_PACKET:
            if (strcmp(tag, "packet") == 0)
            {
                proc_log_packet_end(ctx, depth_ctx, NULL);
                ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            }
            break;

        case RGT_XML2HTML_STATE_PACKET_PROTO:
            if (strcmp(tag, "proto") == 0)
            {
                proc_log_packet_proto_end(ctx, depth_ctx, NULL);
                ctx->state = RGT_XML2HTML_STATE_PACKET;
            }
            break;

        case RGT_XML2HTML_STATE_MEM_DUMP:
            assert(strcmp(tag, "mem-dump") == 0);
            proc_mem_dump_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            break;
        
        case RGT_XML2HTML_STATE_MEM_DUMP_ROW:
            assert(strcmp(tag, "row") == 0);
            proc_mem_row_end(ctx, depth_ctx, NULL);
            ctx->mem_ctx.first_row = FALSE;
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP;
            break;

        case RGT_XML2HTML_STATE_MEM_DUMP_ELEM:
            assert(strcmp(tag, "elem") == 0);

            if (ctx->mem_ctx.first_row)
            {
                ctx->mem_ctx.mem_width++;
            }
            
            proc_mem_elem_end(ctx, depth_ctx, NULL);
            
            ctx->mem_ctx.cur_num++;
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ROW;
            break;

        case RGT_XML2HTML_STATE_BLOCK:
        {
            rgt_xml2fmt_cb_func cb_func;

            if ((cb_func = proc_test_end, strcmp(tag, "test") == 0) ||
                (cb_func = proc_pkg_end, strcmp(tag, "pkg") == 0) ||
                (cb_func = proc_session_end, strcmp(tag, "session") == 0))
            {
                cb_func(ctx, depth_ctx, NULL);
                depth_ctx->seq++;
                ctx->depth--;
            }
            else if (strcmp(tag, "branch") == 0)
            {
                proc_branch_end(ctx, depth_ctx, NULL);
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

            proc_meta_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_BLOCK;
            break;
        
        case RGT_XML2HTML_STATE_START_TS:
            assert(ctx->depth >= 1);
            proc_meta_start_ts_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_END_TS:
            assert(ctx->depth >= 1);
            proc_meta_end_ts_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_DURATION:
            assert(ctx->depth >= 1);
            proc_meta_duration_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_OBJECTIVE:
            assert(strcmp(tag, "objective") == 0);
            assert(ctx->depth >= 1);
            proc_meta_objective_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_PAGE:
            assert(strcmp(tag, "page") == 0);
            assert(ctx->depth >= 1);
            proc_meta_page_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_AUTHORS:
            assert(strcmp(tag, "authors") == 0);
            assert(ctx->depth >= 1);
            proc_meta_authors_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_AUTHOR:
            assert(strcmp(tag, "author") == 0);
            proc_meta_author_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_AUTHORS;
            break;

        case RGT_XML2HTML_STATE_VERDICTS:
            assert(strcmp(tag, "verdicts") == 0);
            assert(ctx->depth >= 1);
            proc_meta_verdicts_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;

        case RGT_XML2HTML_STATE_VERDICT:
            if (strcmp(tag, "verdict") == 0)
            {
                proc_meta_verdict_end(ctx, depth_ctx, NULL);
                ctx->state = RGT_XML2HTML_STATE_VERDICTS;
            }
            else
                assert(strcmp(tag, "br") == 0);
            break;

        case RGT_XML2HTML_STATE_PARAMS:
            assert(ctx->depth >= 1);

            if (strcmp(tag, "params") != 0)
            {
                assert(strcmp(tag, "param") == 0);
                proc_meta_param_end(ctx, depth_ctx, NULL);
                break;
            }

            proc_meta_params_end(ctx, depth_ctx, NULL);
            ctx->state = RGT_XML2HTML_STATE_META;
            break;
            
        case RGT_XML2HTML_STATE_FILE:
            if (strcmp(tag, "file") == 0)
            {
                proc_log_msg_file_end(ctx, depth_ctx, NULL);
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
 * @param  attrs       An array of attribute name, attribute value pairs
 *
 * @return Nothing
 */
static void
rgt_log_start_element(void *user_data,
                      const rgt_xmlChar *xml_tag, const rgt_xmlChar **attrs)
{
    rgt_gen_ctx_t   *ctx = (rgt_gen_ctx_t *)user_data;
    rgt_depth_ctx_t *depth_ctx;
    const char      *tag = (const char *)xml_tag;

    depth_ctx = &g_array_index(ctx->depth_info,
                               rgt_depth_ctx_t, (ctx->depth - 1));

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_INITIAL:
            assert(strcmp(tag, "proteos:log_report") == 0);
            ctx->state = RGT_XML2HTML_STATE_BLOCK;
            break;

        case RGT_XML2HTML_STATE_BLOCK:
            if (strcmp(tag, "meta") == 0)
            {
                proc_meta_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_META;
                break;
            }
            else if (strcmp(tag, "logs") == 0)
            {
                assert(ctx->depth >= 1);
                proc_logs_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_LOGS;
                break;
            }
            else if (strcmp(tag, "branch") == 0)
            {
                assert(ctx->depth >= 1);
                proc_branch_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
                break;
            }

            /* Control message */
            rgt_process_cntrl_start(ctx, tag, RGT_XML2CHAR(attrs));
            break;
        
        case RGT_XML2HTML_STATE_META:
            assert(ctx->depth >= 1);

            if (strcmp(tag, "start-ts") == 0)
            {
                proc_meta_start_ts_start(ctx, depth_ctx,
                                         RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_START_TS;
            }
            else if (strcmp(tag, "end-ts") == 0)
            {
                proc_meta_end_ts_start(ctx, depth_ctx,
                                       RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_END_TS;
            }
            else if (strcmp(tag, "duration") == 0)
            {
                proc_meta_duration_start(ctx, depth_ctx,
                                         RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_DURATION;
            }
            else if (strcmp(tag, "objective") == 0)
            {
                proc_meta_objective_start(ctx, depth_ctx,
                                          RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_OBJECTIVE;
            }
            else if (strcmp(tag, "page") == 0)
            {
                proc_meta_page_start(ctx, depth_ctx,
                                     RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_PAGE;
            }
            else if (strcmp(tag, "authors") == 0)
            {
                proc_meta_authors_start(ctx, depth_ctx,
                                        RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_AUTHORS;
            }
            else if (strcmp(tag, "verdicts") == 0)
            {
                proc_meta_verdicts_start(ctx, depth_ctx,
                                         RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_VERDICTS;
            }
            else if (strcmp(tag, "params") == 0)
            {
                proc_meta_params_start(ctx, depth_ctx,
                                       RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_PARAMS;
            }
            else
            {
                fprintf(stderr, "Unexpected TAG '%s' in META state\n",
                        tag);
                assert(0);
            }

            break;
            
        case RGT_XML2HTML_STATE_AUTHORS:
            assert(strcmp(tag, "author") == 0);
            proc_meta_author_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
            ctx->state = RGT_XML2HTML_STATE_AUTHOR;
            break;

        case RGT_XML2HTML_STATE_VERDICTS:
            assert(strcmp(tag, "verdict") == 0);
            proc_meta_verdict_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
            ctx->state = RGT_XML2HTML_STATE_VERDICT;
            break;

        case RGT_XML2HTML_STATE_VERDICT:
            assert(strcmp(tag, "br") == 0);
            proc_log_msg_br(ctx, depth_ctx, RGT_XML2CHAR(attrs));
            break;

        case RGT_XML2HTML_STATE_PARAMS:
            assert(strcmp(tag, "param") == 0);
            proc_meta_param_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
            break;

        case RGT_XML2HTML_STATE_LOGS:
            assert(strcmp(tag, "msg") == 0);

            proc_log_msg_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
            ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            break;

        case RGT_XML2HTML_STATE_LOG_MSG:
            if (strcmp(tag, "br") == 0)
            {
                proc_log_msg_br(ctx, depth_ctx, RGT_XML2CHAR(attrs));
            }
            else if (strcmp(tag, "mem-dump") == 0)
            {
                ctx->mem_ctx.first_row = TRUE;
                ctx->mem_ctx.mem_width = 0;

                proc_mem_dump_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_MEM_DUMP;
            }
            else if (strcmp(tag, "file") == 0)
            {
                proc_log_msg_file_start(ctx, depth_ctx,
                                        RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_FILE;
            }
            else if (strcmp(tag, "packet") == 0)
            {
                proc_log_packet_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_PACKET;
            }
            else
                assert(0);

            break;

        case RGT_XML2HTML_STATE_PACKET:
            if (strcmp(tag, "proto") == 0)
            {
                proc_log_packet_proto_start(ctx, depth_ctx,
                                            RGT_XML2CHAR(attrs));
                ctx->state = RGT_XML2HTML_STATE_PACKET_PROTO;
            }
            break;

        case RGT_XML2HTML_STATE_PACKET_PROTO:
            if (strcmp(tag, "field") == 0)
            {
                proc_log_packet_field_start(ctx, depth_ctx,
                                            RGT_XML2CHAR(attrs));
            }
            break;

        case RGT_XML2HTML_STATE_MEM_DUMP:
            assert(strcmp(tag, "row") == 0);
            ctx->mem_ctx.cur_num = 0;
            proc_mem_row_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ROW;
            break;

        case RGT_XML2HTML_STATE_MEM_DUMP_ROW:
            assert(strcmp(tag, "elem") == 0);
            proc_mem_elem_start(ctx, depth_ctx, RGT_XML2CHAR(attrs));
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ELEM;
            break;
            
        case RGT_XML2HTML_STATE_FILE:
            assert(strcmp(tag, "br") == 0);
            proc_log_msg_br(ctx, depth_ctx, RGT_XML2CHAR(attrs));
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
rgt_log_characters(void *user_data, const rgt_xmlChar *ch, int len)
{
    rgt_gen_ctx_t   *ctx = (rgt_gen_ctx_t *)user_data;
    rgt_depth_ctx_t *depth_ctx;

    depth_ctx = &g_array_index(ctx->depth_info,
                               rgt_depth_ctx_t, (ctx->depth - 1));

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_START_TS:
        case RGT_XML2HTML_STATE_END_TS:
        case RGT_XML2HTML_STATE_DURATION:
        case RGT_XML2HTML_STATE_OBJECTIVE:
        case RGT_XML2HTML_STATE_PAGE:
        case RGT_XML2HTML_STATE_VERDICT:
        case RGT_XML2HTML_STATE_LOG_MSG:
        case RGT_XML2HTML_STATE_MEM_DUMP_ELEM:
        case RGT_XML2HTML_STATE_FILE:
        {
            assert(ctx->depth >= 1);
            proc_chars(ctx, depth_ctx, ch, len);
            break;
        }
        default:
            break;
    }
}

#if (defined WITH_LIBXML)
/**
 * The callback is called for resolving entities (& NAME ;)
 * In case of SAX parser it converts standard entities into their values
 * (&gt; -> ">", &lt; -> "<", &amp; -> "&"),
 * but in case of HTML we must not convert them, but leave them as they 
 * are, so that this function makes a HACK for that.
 * If you want how to force libxml2 SAX parser leave standard entries 
 * without expanding - update this code!
 */
static xmlEntityPtr
rgt_get_entity(void *user_data, const rgt_xmlChar *xml_name)
{
    static xmlEntity  ent;
    rgt_gen_ctx_t    *ctx = (rgt_gen_ctx_t *)user_data;
    const char       *name = (const char *)xml_name;

    if (ctx->expand_entities)
        return xmlGetPredefinedEntity(xml_name);

#define FILL_ENT(ent_, name_) \
    do {                                            \
        ent_.name = (const rgt_xmlChar *)(name_);   \
        ent_.orig = (rgt_xmlChar *)("&" name_ ";"); \
    } while (0)

    if (strcmp(name, "lt") == 0)
        FILL_ENT(ent, "lt");
    else if (strcmp(name, "gt") == 0)
        FILL_ENT(ent, "gt");
    else if (strcmp(name, "amp") == 0)
        FILL_ENT(ent, "amp");
    else if (strcmp(name, "quot") == 0)
        FILL_ENT(ent, "quot");
    else if (strcmp(name, "apos") == 0)
        FILL_ENT(ent, "apos");
    else
    {
        assert(0);
        return NULL;
    }
#undef FILL_ENT

    /*
     * The following lines are dirty hack in order to fix a bug
     * found in libxml2 (see http://bugzilla.gnome.org BugId 389843).
     * It works very badly with entities in attribute values, and 
     * looking at the code of libxml2 I've found a work-around:
     * - In order to keep attribute entity unconverted we can use
     *   entity data structure filled as following: 
     *   "etype" (entity type) is set to "XML_INTERNAL_GENERAL_ENTITY";
     *   "content" is set to NULL;
     * Here we assume that entities can present only in attributes of
     * <param> TAG (<param name="" value="HERE WE EXPECT ENTITIES">).
     *
     * @note The behaviour of libxml2 can change in the further versions,
     * that's why this code should be reviewed and fixed later.
     *
     * This workaround works for libxml2 up to 2.6.27 (think this will
     * work later as well, but who knows).
     */

    if (ctx->state == RGT_XML2HTML_STATE_PARAMS)
    {
        ent.etype = XML_INTERNAL_GENERAL_ENTITY;
        ent.content = NULL;
    }
    else
    {
        ent.etype = XML_INTERNAL_PREDEFINED_ENTITY;
        ent.content = ent.orig;
    }

    return &ent;
}

static void
rgt_report_problem(void *user_data, const char *msg, ...)
{
    va_list ap;

    UNUSED(user_data);
    
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

/**
 * The structure specifies all types callback routines that
 * should be called.
 */
static xmlSAXHandler sax_handler = {
    .internalSubset         = NULL,
    .isStandalone           = NULL,
    .hasInternalSubset      = NULL,
    .hasExternalSubset      = NULL,
    .resolveEntity          = NULL,
    .getEntity              = rgt_get_entity,
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
    .warning                = rgt_report_problem, /* warningDebug, */
    .error                  = rgt_report_problem, /* errorDebug, */
    .fatalError             = rgt_report_problem, /* fatalErrorDebug, */
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
#endif /* WITH_LIBXML */

/**
 * Parses XML file specified in context.
 * 
 * @param gen_ctx  Context set up by main() entry point
 *
 * @return Status of the operation
 * @retval 0  A file has been successfully parsed
 * @retval 1  An error has happened during a file parsing
 */
#if (defined WITH_LIBXML)
static int
rgt_parse_file(rgt_gen_ctx_t *gen_ctx)
{
    xmlParserCtxtPtr ctxt;
    int              ret    = 0;

    ctxt = xmlCreateFileParserCtxt(gen_ctx->xml_fname);
    if (ctxt == NULL)
        return 1;

    /* Option to process and escape the <, >, ", ', & symbols. */
    xmlCtxtUseOptions(ctxt, XML_PARSE_OLDSAX);

    if (ctxt->sax != (xmlSAXHandlerPtr) &xmlDefaultSAXHandler)
        xmlFree(ctxt->sax);
    ctxt->sax = &sax_handler;
    if (gen_ctx != NULL)
        ctxt->userData = (void *)gen_ctx;

    xmlParseDocument(ctxt);
    if (ctxt->wellFormed)
        ret = 0;
    else
        ret = ctxt->errNo != 0 ? ctxt->errNo : 1;

    ctxt->sax = NULL;
    if (ctxt->myDoc != NULL)
    {
        xmlFreeDoc(ctxt->myDoc);
        ctxt->myDoc = NULL;
    }

    xmlFreeParserCtxt(ctxt);
    xmlCleanupParser();
    return ret;
}
#elif (defined WITH_EXPAT)
static int
rgt_parse_file(rgt_gen_ctx_t *gen_ctx)
{
#define BUF_BLOCK_LEN 1024 /* 1Kb */
    XML_Parser     p = NULL;
    FILE          *fd = NULL;
    size_t         n;
    char          *buf = NULL;
    size_t         buf_len = BUF_BLOCK_LEN * 1024; /* 1Mb */
    te_bool        err = FALSE;

    if ((fd = fopen(gen_ctx->xml_fname, "r")) == NULL)
    {
        fprintf(stderr, "Cannot open %s file: %s\n",
                gen_ctx->xml_fname, strerror(errno));
        return 1;
    }
    if ((p = XML_ParserCreate(NULL)) == NULL)
    {
        fprintf(stderr, "Cannot create parser\n");
        fclose(fd);
        return 1;
    }

    assert((buf_len % BUF_BLOCK_LEN) == 0);

    while ((buf = malloc(buf_len)) == NULL && buf_len != 0)
    {
        buf_len -= BUF_BLOCK_LEN;
    }
    if (buf_len == 0)
    {
        fprintf(stderr, "Cannot allocate enough memory\n");
        XML_ParserFree(p);
        fclose(fd);
        return 1;
    }
    assert(buf != NULL);

    XML_SetUserData(p, gen_ctx);
    XML_SetElementHandler(p, rgt_log_start_element, rgt_log_end_element);
    if (proc_expand_entities())
        XML_SetCharacterDataHandler(p, rgt_log_characters);
    else
        XML_SetDefaultHandler(p, rgt_log_characters);

    rgt_log_start_document(gen_ctx);

    do {
        n = fread(buf, 1, buf_len, fd);

        if (XML_Parse(p, buf, n, (n == 0)) == 0)
        {
            fprintf(stderr, "Parse error at line %d:\n%s\n",
                    XML_GetCurrentLineNumber(p),
                    XML_ErrorString(XML_GetErrorCode(p)));
            err = TRUE;
            break;
        }
    } while (n != 0);

    if (!err)
        rgt_log_end_document(gen_ctx);

    free(buf);
    XML_ParserFree(p);
    fclose(fd);

    return 0;
}
#endif

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
 * @se Frees popt Context (specified in optCon) and 
 * exits with specified code.
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
 * The procedure contains "Option table" that should be updated 
 * if some new options are going to be added.
 *
 * @param  argc   Number of elements in array "argv".
 * @param  argv   Array of strings that represents all command 
 *                line arguments.
 *
 * @return  Nothing.
 *
 * @se
 *    The function updates the following global variables according to 
 *    the command line arguments specified in argv:
 *      html_tmpls    - Array of template file names
 *
 *    In the case of an error it calls exit() function with code 1.
 */
static void
process_cmd_line_opts(int argc, char **argv, rgt_gen_ctx_t *ctx)
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

        { "version", 'v', POPT_ARG_NONE, NULL, 'v', 
          "Display version information.", NULL },

        { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rgt_options_table, 0,
          "Format-specific options:", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            optionsTable, 0);

    poptSetOtherOptionHelp(optCon,
                "[OPTIONS...] [<xml report file>] [<output file>]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 'f')
        {
            if ((ctx->xml_fname = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify XML report file", NULL);
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
        else
            rgt_process_cmdline(optCon, rc);
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

    if (ctx->xml_fname == NULL && 
        (ctx->xml_fname = poptGetArg(optCon)) == NULL)
    {
        usage(optCon, 1, "Specify XML report file", NULL);
    }

    /* Get output file name */
    ctx->out_fname = poptGetArg(optCon);
    
    if (ctx->out_fname != NULL && opt_out_file_name != NULL)
    {
        usage(optCon, 1, "Output file name specified twice: "
              "with -f option and as a command line argument", NULL);
    }

    if (opt_out_file_name != NULL)
        ctx->out_fname = opt_out_file_name;

    if (poptPeekArg(optCon) != NULL)
    {
        usage(optCon, 1, "Too many parameters specified", NULL);
    }

    poptFreeContext(optCon);
}

/* The description see in xml2gen.h */
int
rgt_xml2fmt_files_get_idx(const char* short_name)
{
    char filename[256];
    int  i;

    if (snprintf(filename, 256, "/%s.tmpl", short_name) > 256)
        return -1;

    for (i = 0; (unsigned)i < xml2fmt_tmpls_num; i++)
    {
        if(strstr(xml2fmt_files[i], filename) != NULL)
            return i;
    }

    return -1;
}

int
main(int argc, char **argv)
{
    rgt_gen_ctx_t gen_ctx;
    int           rc;

    memset(&gen_ctx, 0, sizeof(gen_ctx));

    process_cmd_line_opts(argc, argv, &gen_ctx);

    if (rgt_tmpls_parse(xml2fmt_files, xml2fmt_tmpls,
                        xml2fmt_tmpls_num) != 0)
    {
        assert(0);
    }

    gen_ctx.state = RGT_XML2HTML_STATE_INITIAL;
    gen_ctx.depth = 0;
    gen_ctx.depth_info = g_array_new(FALSE, FALSE, 
                                     sizeof(rgt_depth_ctx_t));
    if (gen_ctx.depth_info == NULL)
    {
        fprintf(stderr, "Cannot allocate resourses for the programm\n");
        exit(1);
    }
    rgt_attr_settings_init(rgt_line_separator, rgt_max_attribute_length);

    rc = rgt_parse_file(&gen_ctx);

    assert(gen_ctx.depth == 0);

    rgt_tmpls_free(xml2fmt_tmpls, xml2fmt_tmpls_num);

    g_array_free(gen_ctx.depth_info, TRUE);

    return rc;
}
