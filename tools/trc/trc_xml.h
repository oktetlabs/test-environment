/** @file
 * @brief Testing Results Comparator
 *
 * Interface and additional functions to libxml2.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TOOLS_TRC_XML_H__
#define __TE_TOOLS_TRC_XML_H__

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


#define CONST_CHAR2XML  (const xmlChar *)
#define XML2CHAR(p)     ((char *)p)
#define XML2CHAR_DUP(p) XML2CHAR(xmlStrdup(p))


/**
 * Skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return Current, one of next or NULL.
 */
static inline xmlNodePtr
xmlNodeSkipComment(xmlNodePtr node)
{
    while ((node != NULL) &&
           (xmlStrcmp(node->name, CONST_CHAR2XML("comment")) == 0))
    {
        node = node->next;
    }
    return node;
}

/**
 * Go to the next XML node, skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return The next XML node.
 */
static inline xmlNodePtr
xmlNodeChildren(xmlNodePtr node)
{
    assert(node != NULL);
    return xmlNodeSkipComment(node->children);
}

/**
 * Go to the next XML node, skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return The next XML node.
 */
static inline xmlNodePtr
xmlNodeNext(xmlNodePtr node)
{
    assert(node != NULL);
    return xmlNodeSkipComment(node->next);
}

#endif /* !__TE_TOOLS_TRC_XML_H__ */
