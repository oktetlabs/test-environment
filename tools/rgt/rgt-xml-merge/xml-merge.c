/** @file 
 * @brief Test Environment: xml-merge utility callbacks.
 *
 * Copyright (C) 2012 Test Environment authors (see file AUTHORS in the
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
 * @author Andrey Dmitrov  <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id: $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <stdbool.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <libxml/parserInternals.h>

/** Size of a block chars that should be read from a file for each call */
#define BL_SIZE 4

/** Base size of a memory block to contain the <msg> tag */
#define RGT_MSG_SIZE 512

/** Size of memory chunk to read and process */
#define RGT_CHUNK_SIZE 256

/** Malloc macro */
#define RGT_XML_MALLOC(_ptr, _size)       \
    if ((_ptr = malloc(_size)) == NULL)   \
        assert(0);

/** The list of possible states in XML processing state machine */
typedef enum rgt_merge_state_t {
    RGT_MERGE_STATE_BASE,
    RGT_MERGE_STATE_MSG,
    RGT_MERGE_STATE_MSG_PRINT,
} rgt_merge_state_t;

/** List of structures that keeps context of the file streams */
typedef struct rgt_file_ctx {
    const char          *name;     /**< File name */
    FILE                *fd;       /**< The file descriptor */
    xmlParserCtxtPtr     xmlctx;      /**< Pointer to parsing context */
    char                *last_tag; /**< Keep the last gotten XML tag */
    unsigned             ts[2];    /**< Timestamp of the current packet */
    bool                 main_st;  /**< Main stream flag */
    rgt_merge_state_t    state;    /**< Context state */

    SLIST_ENTRY(rgt_file_ctx)  ent_l;
} rgt_file_ctx;
SLIST_HEAD(flist_h_t, rgt_file_ctx);
typedef struct flist_h_t flist_h_t;

/** Head of the list keeps streams context */
static flist_h_t rgt_flist_h;

/** File descriptor of the resulting file. */
static FILE *res_fd;

/** Current file stream to read data */
static rgt_file_ctx *curr_stream;

/** A stream with previous minimal timestamp */
static rgt_file_ctx *prev_stream;

/**
 * Macro compare the current timestamp value with a prvious. If the current
 * timestamp less, then continue to print messages from the current stream.
 * Else change stream to previous. If stream has been already changed to
 * previous, then performs search for a context with minimal timestamp.
 */
#define RGT_CHECK_CURR_STREAM(_ctx) \
{ \
    if (_ctx != prev_stream && \
        (_ctx->ts[0] < prev_stream->ts[0] || \
        (_ctx->ts[0] == prev_stream->ts[0] && \
         _ctx->ts[1] < prev_stream->ts[1]))) \
        curr_stream = _ctx; \
    else if (_ctx != prev_stream) \
        curr_stream = prev_stream; \
    else \
        rgt_check_curr_stream(); \
}

/**
 * Find the ts_val attribute and get timestamp value of the message.
 * 
 * @param atts  Array with element attributes
 * @param ctx   Stream context
 */
static void
rgt_update_msg_ts(const char **atts, rgt_file_ctx *ctx)
{
    int     i;
    char   *ts_str = NULL;
    char   *dotp;
    char   *endptr;

    for (i = 0; atts[i] != NULL; i++)
    {
        if (strcmp(atts[i], "ts_val") == 0)
        {
            ts_str = (char *)atts[i + 1];
            break;
        }
    }
    assert(ts_str != NULL);

    dotp = strchr(ts_str, '.');
    assert(dotp != NULL);

    ctx->ts[0] = strtoul(ts_str, NULL, 10);
    ctx->ts[1] = strtoul(dotp + 1, &endptr, 10);
}

/**
 * Find the minimal timestam and set current stream context.
 */
static inline void
rgt_check_curr_stream(void)
{
    rgt_file_ctx *ctx;
    int           i     = 0;

    curr_stream = SLIST_FIRST(&rgt_flist_h);
    if (curr_stream == NULL)
        return;
    prev_stream = curr_stream;

    SLIST_FOREACH(ctx, &rgt_flist_h, ent_l)
    {
        if (curr_stream->ts[0] > ctx->ts[0] ||
            (curr_stream->ts[0] == ctx->ts[0] &&
             curr_stream->ts[1] > ctx->ts[1]))
        {
            prev_stream = curr_stream;
            curr_stream = ctx;
        }
        else if (prev_stream->ts[0] > ctx->ts[0] ||
                 (prev_stream->ts[0] == ctx->ts[0] &&
                 prev_stream->ts[1] > ctx->ts[1]))
        {
            prev_stream = ctx;
        }
        i++;
    }
}

/**
 * Save the last tag to context to print later.
 * 
 * @param tag   The tag name
 * @param atts  Array with element attributes
 * @param ctx   Stream context
 */
static void
rgt_save_tag(const char *tag, const char **atts, rgt_file_ctx *ctx)
{
    int     size;
    int     i;
    int     clen;
    int     res;

    if (ctx->last_tag != NULL)
        free(ctx->last_tag);
    RGT_XML_MALLOC(ctx->last_tag, RGT_MSG_SIZE);
    size = RGT_MSG_SIZE;

    clen = snprintf(ctx->last_tag, size, "<%s", tag);
    
    if (atts != NULL)
    {
        bool attr_name = true;
        for (i = 0; atts[i] != NULL; i++)
        {
            if (attr_name)
                res = snprintf(ctx->last_tag + clen, size - clen, " %s=",
                               atts[i]);
            else
                res = snprintf(ctx->last_tag + clen, size - clen, "\"%s\"",
                               atts[i]);
            attr_name = !attr_name;
            if (clen + res > size)
            {
                size *= 2;
                ctx->last_tag = realloc(ctx->last_tag, size);
                i--;
            }
            else
                clen += res;
        }
    }
    if (clen == size)
    {
        size += 2;
        ctx->last_tag = realloc(ctx->last_tag, size);
    }
    snprintf(ctx->last_tag + clen, size - clen, ">");
}

/**
 * Print the tag with attributes.
 * 
 * @param tag   The tag name
 * @param atts  Array with element attributes
 */
static void
rgt_print_tag(const char *tag, const char **atts)
{
    int i;

    fprintf(res_fd, "<%s", tag);
    if (atts != NULL)
    {
        bool attr_name = true;
        for (i = 0; atts[i] != NULL; i++)
        {
            if (attr_name)
                fprintf(res_fd, " %s=", atts[i]);
            else
                fprintf(res_fd, "\"%s\"", atts[i]);
            attr_name = !attr_name;
        }
    }
    fprintf(res_fd, ">");
}

/**
 * User call back called when an opening tag has been processed.
 *
 * @param in_ctx    XML parser context
 * @param xml_tag   Element name
 * @param xml_atts  Array with element attributes
 */
static void
rgt_log_start_element(void *in_ctx, const xmlChar  *xml_tag,
                      const xmlChar **xml_atts)
{
    rgt_file_ctx     *ctx  = (rgt_file_ctx *)in_ctx;
    const char       *tag  = (const char *)xml_tag;
    const char      **atts = (const char **)xml_atts;

    switch (ctx->state)
    {
        case RGT_MERGE_STATE_BASE :
            if (strcmp(tag, "msg") == 0)
            {
                assert(atts != NULL);
                rgt_update_msg_ts(atts, ctx);
                ctx->state = RGT_MERGE_STATE_MSG;

                RGT_CHECK_CURR_STREAM(ctx);
                //~ rgt_check_curr_stream();
                if (curr_stream == ctx)
                {
                    ctx->state = RGT_MERGE_STATE_MSG_PRINT;
                    rgt_print_tag(tag, atts);
                }
                else
                    rgt_save_tag(tag, atts, ctx);
            }
            else if (ctx->main_st)
                rgt_print_tag(tag, atts);
            break;

        case RGT_MERGE_STATE_MSG :
            fprintf(res_fd, "%s", ctx->last_tag);
            rgt_print_tag(tag, atts);
            ctx->state = RGT_MERGE_STATE_MSG_PRINT;
            break;

        case RGT_MERGE_STATE_MSG_PRINT :
            rgt_print_tag(tag, atts);
            break;

        default:
            assert(0);
    }
}


/**
 * Callback function that is called when XML parser meets the characters
 * element.
 *
 * @param  in_ctx     Pointer to user-specific data (user context)
 * @param  ch         Pointer to characters block
 * @param  len        Length of the block
 */
static void
rgt_log_characters(void *in_ctx, const xmlChar *ch, int len)
{
    rgt_file_ctx *ctx  = (rgt_file_ctx *)in_ctx;
    char          str[len + 1];

    if (ctx->state == RGT_MERGE_STATE_MSG)
    {
        fprintf(res_fd, "%s", ctx->last_tag);
        ctx->state = RGT_MERGE_STATE_MSG_PRINT;
    }

    if (ctx->main_st || ctx->state == RGT_MERGE_STATE_MSG_PRINT)
    {
        memcpy(str, ch, len);
        str[len] = '\0';
        fprintf(res_fd, "%s", str);
    }
}

/**
 * Callback function that is called when XML parser meets the end of 
 * an element.
 *
 * @param  in_ctx     Pointer to user-specific data (user context)
 * @param  xml_tag    The element name
 */
static void
rgt_log_end_element(void *in_ctx, const xmlChar *xml_tag)
{
    rgt_file_ctx     *ctx  = (rgt_file_ctx *)in_ctx;
    const char       *tag  = (const char *)xml_tag;

    switch (ctx->state)
    {
        case RGT_MERGE_STATE_BASE :
            if (ctx->main_st && strcmp(tag, "proteos:log_report") != 0)
                fprintf(res_fd, "</%s>", xml_tag);
            break;

        case RGT_MERGE_STATE_MSG :
        case RGT_MERGE_STATE_MSG_PRINT :
            if (strcmp(tag, "msg") == 0)
            {
                ctx->state = RGT_MERGE_STATE_BASE;
                //~ if (!ctx->main_st)
                    fprintf(res_fd, "</%s>\n", xml_tag);
                //~ else
                    //~ fprintf(res_fd, "</%s>", xml_tag);
            }
            else
                fprintf(res_fd, "</%s>", xml_tag);
            break;

        default:
            assert(0);
    }
}

/**
 * User call back called when an opening document tag has been processed.
 *
 * @param in_ctx    XML parser context
 */
static void
rgt_log_start_document(void *in_ctx)
{
    rgt_file_ctx     *ctx  = (rgt_file_ctx *)in_ctx;

    if (ctx->main_st)
        fprintf(res_fd, "<?xml version=\"1.0\"?>\n");
}

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
rgt_get_entity(void *user_data, const xmlChar *xml_name)
{
    static xmlEntity  ent;
    const char       *name = (const char *)xml_name;

#define FILL_ENT(ent_, name_) \
    do {                                            \
        ent_.name = (const xmlChar *)(name_);   \
        ent_.orig = (xmlChar *)("&" name_ ";"); \
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

    ent.etype = XML_INTERNAL_PREDEFINED_ENTITY;
    ent.content = ent.orig;

    return &ent;
}

static xmlSAXHandler sax_handler = {
    .internalSubset         = NULL,
    .isStandalone           = NULL,
    .hasInternalSubset      = NULL,
    .hasExternalSubset      = NULL,
    .resolveEntity          = NULL,
    .getEntity              = rgt_get_entity,
    .entityDecl             = NULL,
    .notationDecl           = NULL,
    .attributeDecl          = NULL,
    .elementDecl            = NULL,
    .unparsedEntityDecl     = NULL,
    .setDocumentLocator     = NULL,
    .startDocument          = rgt_log_start_document,
    .endDocument            = NULL,
    .startElement           = rgt_log_start_element,
    .endElement             = rgt_log_end_element,
    .reference              = NULL,
    .characters             = rgt_log_characters,
    .ignorableWhitespace    = NULL,
    .processingInstruction  = NULL,
    .comment                = NULL,
    .warning                = NULL,
    .error                  = NULL,
    .fatalError             = NULL,
    .getParameterEntity     = NULL,
    .cdataBlock             = NULL,
    .externalSubset         = NULL,
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
 * Help to use description.
 */
static void
usage(void)
{
    fprintf(stderr, "Usage: rgt-xml-merge output_file te_log.xml"
            " capture1.xml [capture2.xml] ...\n");
    exit(0);
}

/**
 * Function initialize a file context and add It to the list.
 * 
 * @param fname     The file name
 * 
 * @return Status code.
 * @retval 0    Success
 * @retval -1   Errors: couldn't open the file; couldn't read a block chars
 *              from the file; couldn't creat the parser context.
 */
static int
rgt_add_input_file(const char *fname, bool main_stream)
{
    rgt_file_ctx   *new_ctx;
    char            chars[BL_SIZE + 1];
    int             res;
    int             rc                  = -1;

    RGT_XML_MALLOC(new_ctx, sizeof(rgt_file_ctx));
    new_ctx->ts[0] = 0;
    new_ctx->ts[1] = 0;
    new_ctx->last_tag = NULL;
    new_ctx->state = RGT_MERGE_STATE_BASE;
    new_ctx->name = fname;
    new_ctx->main_st = main_stream;
    if (strcmp(new_ctx->name, "-") != 0)
        new_ctx->fd = fopen(fname, "rb");
    else
        new_ctx->fd = stdin;

    if (new_ctx->fd == NULL)
        goto rgt_add_input_file_cleanup;

    chars[BL_SIZE] = '\0';
    res = fread(chars, 1, BL_SIZE, new_ctx->fd);
    if (res <= 0)
        goto rgt_add_input_file_cleanup;

    new_ctx->xmlctx = xmlCreatePushParserCtxt(&sax_handler, new_ctx,
                                              chars, res, fname);
    if (new_ctx->xmlctx == NULL)
        goto rgt_add_input_file_cleanup;
    /* Option to process and escape the <, >, ", ', & symbols. */
    xmlCtxtUseOptions(new_ctx->xmlctx, XML_PARSE_OLDSAX);

    SLIST_INSERT_HEAD(&rgt_flist_h, new_ctx, ent_l);

    if (new_ctx->main_st == true)
    {
        curr_stream = new_ctx;
        prev_stream = new_ctx;
    }

    return 0;

rgt_add_input_file_cleanup:
    free(new_ctx);
    return rc;
}

/**
 * Release stream context.
 * 
 * @param ctx   Context of the stream
 */
static void
rgt_release_ctx(rgt_file_ctx *ctx)
{
    if (ctx->last_tag != NULL)
        free(ctx->last_tag);
    fclose(ctx->fd);
    SLIST_REMOVE(&rgt_flist_h, ctx, rgt_file_ctx, ent_l);
    xmlFreeParserCtxt(ctx->xmlctx);
    free(ctx);
}

/** 
 * Parse and process the input files.
 */
static void
rgt_parse_xml_files(void)
{
    int         res;
    char       *chars    = NULL;
    unsigned    size;
    bool        main_st;


    while (!SLIST_EMPTY(&rgt_flist_h))
    {
        while ((res = getdelim(&chars, &size, '>', curr_stream->fd)) > 0)
        {
            xmlParseChunk(curr_stream->xmlctx, chars, res, 0);
        }

        main_st = curr_stream->main_st;
        /*
         * there is no more input, indicate the parsing is finished.
         */
        xmlParseChunk(curr_stream->xmlctx, chars, 0, 1);
        rgt_release_ctx(curr_stream);

        rgt_check_curr_stream();
        if (main_st && curr_stream != NULL)
            fprintf(res_fd, "<logs>");
    }
    if (!main_st)
        fprintf(res_fd, "</logs>");
    fprintf(res_fd, "</proteos:log_report>\n");
    free(chars);
}

/**
 * Copy data from input file/stream to output file/stream.
 * 
 * @param out_str   Output file name or "-" for stdout stream.
 * @param in_str    Input file name or "-" for stdin stream.
 * 
 * @return Status code
 * @retval 0        Success
 * @retval -1       Couldn't open file
 */
static int
rgt_stream_copy(const char *out_str, const char *in_str)
{
    int          in_fd;
    int          out_fd;
    int          res;
    struct stat  st;
    char         chunk[RGT_CHUNK_SIZE + 1];

    chunk[RGT_CHUNK_SIZE] = '\0';
    if (strcmp(out_str, "-") != 0)
    {
        out_fd = open(out_str, O_WRONLY);
        if (out_fd == -1)
        {
            fprintf(stderr, "Couldn't open the file: %s; %s\n",
                    out_str, strerror(errno));
            return -1;
        }
    }
    else
        out_fd = 0;

    if (strcmp(in_str, "-") != 0 && stat(in_str, &st) == 0)
    {
        in_fd = open(in_str, O_RDONLY);
        if (in_fd == -1)
        {
            fprintf(stderr, "Couldn't open the file: %s\n",in_str);
            return -1;
        }
        res = sendfile(out_fd, in_fd, 0, st.st_size);
        if (res < st.st_size)
            fprintf(stderr, "Couldn't complete the file copy: %d, %s\n",
                    res, in_str);
        close(in_fd);
    }
    else if (strcmp(in_str, "-") == 0)
    {
        in_fd = 0;
        while ((res = read(in_fd, chunk, RGT_CHUNK_SIZE)) > 0)
        {
            write(out_fd, chunk, res);
        }
    }

    if (strcmp(out_str, "-") != 0)
        close(out_fd);
    return 0;
}

int
main(int argc, char **argv)
{
    int         i;

    if (argc < 3)
        usage();
    else if (argc == 3)
    {
        rgt_stream_copy(argv[1], argv[2]);
        exit(0);
    }

    if (strcmp(argv[1], "-") != 0)
    {
        res_fd = fopen(argv[1], "wb");
        if (res_fd == NULL)
        {
            fprintf(stderr, "Couldn't open resulting file: %s\n", argv[1]);
            return -1;
        }
    }
    else
        res_fd = stdout;

    SLIST_INIT(&rgt_flist_h);

    rgt_add_input_file(argv[2], true);
    for (i = 3; i < argc; i++)
        rgt_add_input_file(argv[i], false);
    rgt_parse_xml_files();

    xmlCleanupParser();
    xmlMemoryDump();
    fclose(res_fd);
    return 0;
}
