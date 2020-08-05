/** @file 
 * @brief Test Environment: Get capture files path.
 *
 * Parse Logger configuration file to get path to capture logs. Print the 
 * directory path to stdout.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Andrey Dmitrov  <Andrey.Dmitrov@oktetlabs.ru>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libxml/parserInternals.h>
#include "te_defs.h"

/* Path to capture files */
char *path = NULL;

/**
 * User call back called when an opening tag has been processed.
 *
 * @param in_ctx    XML parser context
 * @param xml_tag   Element name
 * @param xml_atts  Array with element attributes
 */
static void
te_log_start_element(void *in_ctx, const xmlChar  *xml_tag,
                      const xmlChar **xml_atts)
{
    const char       *tag  = (const char *)xml_tag;
    const char      **atts = (const char **)xml_atts;
    int               i;
    UNUSED(in_ctx);

    if (strcmp(tag, "snif_path") == 0 && atts != NULL)
    {
        for (i = 0; atts[i] != NULL; i++)
        {
            if ((strcmp(atts[i], "default") == 0 ||
                strcmp(atts[i], "value") == 0) && atts[i + 1] != NULL)
            {
                free(path);
                path = strdup(atts[i + 1]);
            }
        }
    }
}

static xmlSAXHandler sax_handler = {
    .internalSubset         = NULL,
    .isStandalone           = NULL,
    .hasInternalSubset      = NULL,
    .hasExternalSubset      = NULL,
    .resolveEntity          = NULL,
    .getEntity              = NULL,
    .entityDecl             = NULL,
    .notationDecl           = NULL,
    .attributeDecl          = NULL,
    .elementDecl            = NULL,
    .unparsedEntityDecl     = NULL,
    .setDocumentLocator     = NULL,
    .startDocument          = NULL,
    .endDocument            = NULL,
    .startElement           = te_log_start_element,
    .endElement             = NULL,
    .reference              = NULL,
    .characters             = NULL,
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

int
main(int argc, char **argv)
{
    if (argc != 2)
    {
        fputs("Usage: te_log_get_path logger.conf\n", stderr);
        return EXIT_FAILURE;
    }

    xmlSAXUserParseFile(&sax_handler, NULL, argv[1]);
    xmlCleanupParser();
    xmlMemoryDump();
    if (path != NULL)
    {
        puts(path);
        free(path);
    }
    return EXIT_SUCCESS;
}
