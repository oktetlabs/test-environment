/** @file 
 * @brief Test Environment: Get capture files path.
 *
 * Parse Logger configuration file to get path to capture logs. Print the 
 * directory path to stdout.
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

#include <libxml/parserInternals.h>

/* Path to capture files */
const char *path = NULL;

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

    if (strcmp(tag, "snif_path") == 0 && atts != NULL)
    {
        for (i = 0; atts[i] != NULL; i++)
        {
            if ((strcmp(atts[i], "default") == 0 ||
                strcmp(atts[i], "value") == 0) && atts[i + 1] != NULL)
                path = strdup(atts[i + 1]);
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
        fprintf(stderr, "Usage: get_capture_path logger.conf\n");
        exit(1);
    }

    xmlSAXUserParseFile(&sax_handler, NULL, argv[1]);
    xmlCleanupParser();
    xmlMemoryDump();
    printf("%s", path);
    if (path != NULL)
        free((void *)path);
    return 0;
}
