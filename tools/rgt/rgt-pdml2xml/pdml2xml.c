/** @file 
 * @brief Test Environment: pdml->te_xml conversion utility.
 *
 * Implementation of capture log converter.
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
 * @author Andrey Dmitrov  <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <stdbool.h>
#include <unistd.h>

#include <libxml/parserInternals.h>
#include "te_defs.h"

/** Base size of a memory block to contain a packet */
#define RGT_BASE_PACKET_SIZE 1024

/** Maximal timstamp string length */
#define RGT_TS_LEN 255

/** Size of memory chunk to read and process */
#define RGT_CHUNK_SIZE 256

/** Malloc macro */
#define RGT_XML_MALLOC(_ptr, _size)       \
    if ((_ptr = malloc(_size)) == NULL)   \
        assert(0);

/** REALLOC macro */
#define RGT_XML_REALLOC(_ptr, _size)       \
    if ((_ptr = realloc(_ptr, _size)) == NULL)   \
        assert(0);

/** The list of possible states in XML processing state machine */
typedef enum rgt_log_state_t {
    RGT_LOG_STATE_BASE,
    RGT_LOG_STATE_GENINFO,
    RGT_LOG_STATE_MARK_PACKET,
    RGT_LOG_STATE_INFO_PACKET,
    RGT_LOG_STATE_PACKET,
} rgt_log_state_t;

/** List of structures that keeps context of the file streams */
typedef struct rgt_user_ctx {
    char                 ts_val[RGT_TS_LEN + 1]; /**< Timestamp value */
    char                 ts_str[RGT_TS_LEN + 1]; /**< Timestamp string */
    const char          *agent;                  /**< Agent name */
    const char          *interface;              /**< Interface name */
    const char          *sniffer;                /**< Sniffer name */

    rgt_log_state_t      state;                  /**< Context state */
} rgt_user_ctx;

/* File descriptor of the resulting file. */
static FILE *res_fd;

/** Struct contains a buffer for packet */
typedef struct rgt_packet_buff_t {
    char     *p;        /**< Pointer to the buffer */
    unsigned  size;     /**< Current size of the buffer */
    unsigned  offset;   /**< Current offset */
} rgt_packet_buff_t;

/* Common buffer to keep the current packet */
static rgt_packet_buff_t pbuff;

/** Legth of the saved attributes list */
#define RGT_ATTS_LIST_LEN 3

/* List of the attributes, which should be saved */
static const char *atts_list[RGT_ATTS_LIST_LEN] = {
    "name",
    "show",
    "showname",
};

/**
 * Get the attribute value.
 * 
 * @param name      The attribute name
 * @param atts      Attributes array
 * 
 * @return Attribute value or NULL in case of failure
 */
static const char *
rgt_get_attr_val(const char *name, const char **atts)
{
    int  i;
    bool attr_name = true;

    if (atts == NULL)
        return NULL;
    for (i = 0; atts[i] != NULL; i++)
    {
        if (attr_name && strcmp(atts[i], name) == 0)
            return atts[i + 1];
        attr_name = !attr_name;
    }
    return NULL;
}

/**
 * Find the ts_val attribute and get timestamp value of the message.
 * 
 * @param atts  Array with element attributes
 * @param ctx   Stream context
 */
static void
rgt_get_msg_ts(const char **atts, rgt_user_ctx *ctx)
{
    char   *str = NULL;
    char    tmp[RGT_TS_LEN];

    str = (char *)rgt_get_attr_val("value", atts);
    assert(strlen(str) <= RGT_TS_LEN);
    memcpy(ctx->ts_val, str, strlen(str));
    /* Truncate the extra precision */
    ctx->ts_val[strlen(str) - 3] = '\0';

    str = (char *)rgt_get_attr_val("show", atts);
    assert(strlen(str) <= RGT_TS_LEN);
    memcpy(tmp, str, strlen(str));
    /* Truncate the extra precision */
    tmp[strlen(str) - 6] = '\0';

    assert((str = strrchr(tmp, ' ')) != NULL);
    snprintf(ctx->ts_str, RGT_TS_LEN, "%s ms", str + 1);
    str = strchr(ctx->ts_str, '.');
    *str = ' ';
}

/* Pointer to current position indicator for the packet buffer. */
#define CURR (pbuff.p + pbuff.offset)
/* Size of remainder of the buffer. */
#define SIZE (pbuff.size - pbuff.offset)

/** 
 * Macro to save fmt string into the packet buffer. Buffer can be increased,
 * if it is necessary.
 * 
 * @param fmt   fmt string.
 */
#define RGT_SAVE_STR(_fmt...) \
{ \
    static int _res; \
    while (1) \
    { \
        _res = snprintf(CURR, SIZE, _fmt); \
        assert(_res >= 0); \
        if (pbuff.offset + _res >= pbuff.size) \
        { \
            pbuff.size = pbuff.size << 1; \
            RGT_XML_REALLOC(pbuff.p, pbuff.size); \
        } \
        else \
        { \
            pbuff.offset += _res; \
            break; \
        } \
    } \
}

/**
 * Save the last tag to context to print later.
 * 
 * @param tag   The tag name
 * @param atts  Array with element attributes
 * @param ctx   Stream context
 */
static void
rgt_save_tag(const char *tag, const char **atts)
{
    int  ai;
    int  ti;
    bool save_attr;
    bool attr_name;

    RGT_SAVE_STR("<%s", tag);
    if (atts != NULL)
    {
        attr_name = true;
        save_attr = false;
        for (ai = 0; atts[ai] != NULL; ai++)
        {
            if (attr_name)
            {
                save_attr = false;
                for (ti = 0; ti < RGT_ATTS_LIST_LEN; ti++)
                {
                    if (strcmp(atts_list[ti], atts[ai]) == 0)
                    {
                        save_attr = true;
                        RGT_SAVE_STR(" %s=", atts[ai]);
                        break;
                    }
                }
            }
            else if (save_attr)
            {
                RGT_SAVE_STR("\"%s\"", atts[ai]);
            }
            attr_name = !attr_name;
        }
    }
    RGT_SAVE_STR(">");
}

/**
 * Decode hex symbols separated by ':' to ASCII chars.
 * 
 * @param hex_data      Source string
 * 
 * @return Decoded string or NULL in case of failure
 */
static char *
rgt_data_decoding(const char *hex_data)
{
    int     len  = strlen(hex_data);
    int     offt = 0;
    char   *str;
    char   *p;

    if (len < 2)
        return NULL;
    RGT_XML_MALLOC(str, len / 3 + 2);
    memset(str, 0, len / 3 + 2);
    p = str;
    while (offt < len)
    {
        *p = (char)strtol(hex_data + offt, NULL, 16);
        p++;
        offt += 3;
    }

    return str;
}

/**
 * Parse info string to get agent, interface and sniffer names of the
 * capture file.
 * 
 * @param info  String with info
 * @param ctx   User context
 */
static void
rgt_parse_info_str(char *info, rgt_user_ctx *ctx)
{
    char *p;

    assert((p = strchr(info, ';')) != NULL);
    *p = '\0';
    ctx->agent = strdup(info);
    info = p + 1;

    assert((p = strchr(info, ';')) != NULL);
    *p = '\0';
    ctx->interface = strdup(info);
    info = p + 1;

    ctx->sniffer = strdup(info);
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
    rgt_user_ctx     *ctx  = (rgt_user_ctx *)in_ctx;
    const char       *tag  = (const char *)xml_tag;
    const char      **atts = (const char **)xml_atts;
    const char       *field_name;
    const char       *attr_val;
    char             *info_str;
    static bool       info_pack = true;

    switch (ctx->state)
    {
        case RGT_LOG_STATE_BASE :
            if (strcmp(tag, "packet") == 0)
            {
                if (info_pack == true)
                {
                    ctx->state = RGT_LOG_STATE_INFO_PACKET;
                    info_pack = false;
                }
                else
                {
                    ctx->state = RGT_LOG_STATE_PACKET;
                    rgt_save_tag(tag, atts);
                }
            }
            break;

        case RGT_LOG_STATE_PACKET :
            {
                if (strcmp(tag, "proto") == 0 &&
                    (field_name = rgt_get_attr_val("name", atts)) != NULL &&
                    strcmp(field_name, "geninfo") == 0)
                {
                    ctx->state = RGT_LOG_STATE_GENINFO;
                    break;
                }
                else if ((field_name = rgt_get_attr_val("name", atts))
                         != NULL && strcmp(field_name, "ip.proto") == 0)
                {
                    attr_val = rgt_get_attr_val("show", atts);
                    assert(attr_val != NULL);
                    if (strtol(attr_val, NULL, 16) == 0x3d)
                        ctx->state = RGT_LOG_STATE_MARK_PACKET;
                }

                rgt_save_tag(tag, atts);
                break;
            }

        case RGT_LOG_STATE_INFO_PACKET :
            if ((field_name = rgt_get_attr_val("name", atts)) != NULL &&
                strcmp(field_name, "data.data") == 0)
            {
                attr_val = rgt_get_attr_val("show", atts);
                assert(attr_val != NULL);
                info_str = rgt_data_decoding(attr_val);
                assert(info_str != NULL);
                rgt_parse_info_str(info_str, ctx);
                free(info_str);
            }
            break;

        case RGT_LOG_STATE_GENINFO :
            if ((field_name = rgt_get_attr_val("name", atts)) != NULL &&
                    strcmp(field_name, "timestamp") == 0)
            {
                rgt_get_msg_ts(atts, ctx);
            }
            break;

        case RGT_LOG_STATE_MARK_PACKET :
            if ((field_name = rgt_get_attr_val("name", atts)) != NULL &&
                    strcmp(field_name, "data.data") == 0)
            {
                pbuff.offset = 0;
                attr_val = rgt_get_attr_val("show", atts);
                assert(attr_val != NULL);
                info_str = rgt_data_decoding(attr_val);
                assert(info_str != NULL);
                RGT_SAVE_STR("User marker packet.<br/>"
                             "%s", info_str);
                free(info_str);
            }
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
    rgt_user_ctx *ctx  = (rgt_user_ctx *)in_ctx;

    if (ctx->state == RGT_LOG_STATE_PACKET)
    {
        while ((unsigned)len >= SIZE)
        {
            pbuff.size = pbuff.size << 2;
            if ((unsigned)len < SIZE)
                RGT_XML_REALLOC(pbuff.p, pbuff.size);
        }
        memcpy(CURR, ch, len);
        pbuff.offset += len;
    }
}

/**
 * Print the saved packet to the output file.
 * 
 * @param ctx   The user context
 */
static void
rgt_print_saved_packet(rgt_user_ctx *ctx)
{
    fprintf(res_fd, "<msg level=\"PACKET\" entity=\"%s\" user=\"%s/%s\""
            " ts_val=\"%s\" ts=\"%s\">", ctx->agent,
            ctx->interface, ctx->sniffer, ctx->ts_val, ctx->ts_str);
    assert(fprintf(res_fd, "%s</msg>\n", pbuff.p) == (int)(pbuff.offset+7));
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
    rgt_user_ctx     *ctx  = (rgt_user_ctx *)in_ctx;
    const char       *tag  = (const char *)xml_tag;

    switch (ctx->state)
    {
        case RGT_LOG_STATE_BASE :
            break;

        case RGT_LOG_STATE_INFO_PACKET :
            if (strcmp(tag, "packet") == 0)
            {
                ctx->state = RGT_LOG_STATE_BASE;
                pbuff.offset = 0;
            }
            break;

        case RGT_LOG_STATE_PACKET :
            RGT_SAVE_STR("</%s>", tag);
            if (strcmp(tag, "packet") == 0)
            {
                ctx->state = RGT_LOG_STATE_BASE;
                rgt_print_saved_packet(ctx);
                pbuff.offset = 0;
            }
            break;

        case RGT_LOG_STATE_GENINFO :
            if (strcmp(tag, "proto") == 0)
                ctx->state = RGT_LOG_STATE_PACKET;
            break;

        case RGT_LOG_STATE_MARK_PACKET :
            if (strcmp(tag, "packet") == 0)
            {
                ctx->state = RGT_LOG_STATE_BASE;
                rgt_print_saved_packet(ctx);
                pbuff.offset = 0;
             }            
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
    UNUSED(in_ctx);
    fprintf(res_fd, "<?xml version=\"1.0\"?>\n" \
                    "<proteos:log_report><logs>\n");
}

/**
 * User call back called when an closing document tag has been processed.
 *
 * @param in_ctx    XML parser context
 */
static void
rgt_log_end_document(void *in_ctx)
{
    UNUSED(in_ctx);
    fprintf(res_fd, "</logs></proteos:log_report>\n");
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
rgt_get_entity(void *in_ctx, const xmlChar *xml_name)
{
    UNUSED(in_ctx);
    static xmlEntity  ent;
    const char       *name = (const char *)xml_name;

#define FILL_ENT(ent_, name_) \
    do {                                        \
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
    .endDocument            = rgt_log_end_document,
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
    fprintf(stderr, "Usage: pdml2xml source.pdml destination.xml \n");
    exit(0);
}

/**
 * Parse pdml log from stdin stream and convert it to TE XML log.
 */
static void
rgt_parse_input_stream(void)
{
    xmlParserCtxtPtr xml_ctxt;
    rgt_user_ctx     user_ctx;
    int              res        = 100;
    char             chunk[RGT_CHUNK_SIZE + 1];

    memset(chunk, 0, RGT_CHUNK_SIZE + 1);
    user_ctx.state = RGT_LOG_STATE_BASE;

    res = read(0, chunk, 4);
    xml_ctxt = xmlCreatePushParserCtxt(&sax_handler, &user_ctx, chunk, res,
                                       NULL);

    /* Option to process and escape the <, >, ", ', & symbols. */
    xmlCtxtUseOptions(xml_ctxt, XML_PARSE_OLDSAX);

    while ((res = read(0, chunk, RGT_CHUNK_SIZE)) > 0)
    {
        xmlParseChunk(xml_ctxt, chunk, res, 0);
    }

    xmlParseChunk(xml_ctxt, chunk, 0, 1);
    xmlFreeParserCtxt(xml_ctxt);
    xmlCleanupParser();
}

/**
 * Parse pdml file to convert into TE XML log file.
 * 
 * @param fname     Input file name.
 */
static void
rgt_parse_pdml_file(const char *fname)
{
    xmlParserCtxtPtr xml_ctxt;
    rgt_user_ctx     user_ctx;
    int              ret       = 0;

    user_ctx.state = RGT_LOG_STATE_BASE;

    xml_ctxt = xmlCreateFileParserCtxt(fname);
    if (xml_ctxt == NULL)
        return;

    /* Option to process and escape the <, >, ", ', & symbols. */
    xmlCtxtUseOptions(xml_ctxt, XML_PARSE_OLDSAX);

    if (xml_ctxt->sax != (xmlSAXHandlerPtr) &xmlDefaultSAXHandler)
        xmlFree(xml_ctxt->sax);
    xml_ctxt->sax = &sax_handler;
    xml_ctxt->userData = (void *)&user_ctx;

    xmlParseDocument(xml_ctxt);

    if (xml_ctxt->wellFormed)
        ret = 0;
    else
        ret = xml_ctxt->errNo != 0 ? xml_ctxt->errNo : 1;

    xml_ctxt->sax = NULL;
    if (xml_ctxt->myDoc != NULL)
    {
        xmlFreeDoc(xml_ctxt->myDoc);
        xml_ctxt->myDoc = NULL;
    }

    xmlFreeParserCtxt(xml_ctxt);
    xmlCleanupParser();
}

int
main(int argc, char **argv)
{
    if (argc != 3)
        usage();

    RGT_XML_MALLOC(pbuff.p, RGT_BASE_PACKET_SIZE);
    pbuff.size = RGT_BASE_PACKET_SIZE;
    pbuff.offset = 0;

    if (strcmp(argv[2], "-") == 0)
        res_fd = stdout;
    else
    {
        res_fd = fopen(argv[2], "wb");
        if (res_fd == NULL)
        {
            fprintf(stderr, "Couldn't open resulting file: %s\n", argv[1]);
            return -1;
        }
    }

    if (strcmp(argv[1], "-") == 0)
        rgt_parse_input_stream();
    else 
        rgt_parse_pdml_file(argv[1]);

    fclose(res_fd);
    free(pbuff.p);
    return 0;
}
